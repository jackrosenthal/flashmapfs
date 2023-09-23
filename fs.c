#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <fmap.h>
#include <fuse.h>
#include <fuse_log.h>

#include "arena.h"
#include "boolean_flag_file.h"
#include "fs.h"
#include "gbb.h"
#include "mmap_file.h"
#include "route.h"
#include "raw_file.h"
#include "str_file.h"
#include "version_file.h"

static int fmap_load(uint8_t *image, size_t image_size, struct fmap **fmap_out)
{
	ssize_t fmap_offset;
	struct fmap *fmap;

	fmap_offset = fmap_find(image, image_size);
	if (fmap_offset < 0) {
		fuse_log(FUSE_LOG_ERR, "Unable to find valid FMAP structure");
		return -1;
	}

	fmap = (struct fmap *)(&image[fmap_offset]);

	fuse_log(FUSE_LOG_DEBUG, "FMAP found at offset 0x%08x!",
		 (unsigned)fmap_offset);
	fuse_log(FUSE_LOG_DEBUG, "FMAP signature: %-.*s",
		 (int)sizeof(fmap->signature), (char *)fmap->signature);
	fuse_log(FUSE_LOG_DEBUG, "FMAP version %hhu.%hhu", fmap->ver_major,
		 fmap->ver_minor);
	fuse_log(FUSE_LOG_DEBUG, "FMAP base: 0x%016lx", fmap->base);
	fuse_log(FUSE_LOG_DEBUG, "FMAP size: 0x%08x", fmap->size);
	fuse_log(FUSE_LOG_DEBUG, "FMAP name: %-.*s", (int)sizeof(fmap->name),
		 (char *)fmap->name);

	for (size_t i = 0; i < fmap->nareas; i++) {
		struct fmap_area *area = &fmap->areas[i];

		fuse_log(
			FUSE_LOG_DEBUG,
			"FMAP region %-.*s: offset=0x%08x, size=0x%08x, flags=0x%x",
			(int)sizeof(area->name), (char *)area->name,
			area->offset, area->size, area->flags);
		if (area->offset + area->size > image_size) {
			fuse_log(
				FUSE_LOG_ERR,
				"FMAP region %-.*s is located outside of the image",
				(int)sizeof(area->name), (char *)area->name);
			return -1;
		}
	}

	*fmap_out = fmap;
	return 0;
}

int fmapfs_load_image(struct fmapfs_state *state, const char *image_path)
{
	struct directory *areas_dir;

	state->image_size = mmap_file_path(image_path, O_RDWR, &state->image);
	if (state->image_size < 0) {
		fuse_log(FUSE_LOG_ERR, "Failed to mmap image file: %s",
			 image_path);
		return -1;
	}

	if (fmap_load(state->image, state->image_size, &state->fmap) < 0) {
		fuse_log(FUSE_LOG_ERR,
			 "Failed to load fmap from image file: %s", image_path);
		munmap(state->image, state->image_size);
		return -1;
	}

	state->rootdir = route_new_root(&state->arena);
	add_version_file(&state->arena, state->rootdir->dir, "version",
			 state->fmap);
	add_raw_file(&state->arena, state->rootdir->dir, "raw", state->fmap,
		     fmap_size(state->fmap));
	add_str_file(&state->arena, state->rootdir->dir, "name",
		     (char *)state->fmap->name, sizeof(state->fmap->name),
		     true);

	areas_dir = route_new_subdirectory(&state->arena, state->rootdir->dir,
					   "areas");

	for (size_t i = 0; i < state->fmap->nareas; i++) {
		struct fmap_area *area = &state->fmap->areas[i];
		char *area_name = arena_strndup(&state->arena,
						(const char *)area->name,
						sizeof(area->name));
		struct directory *area_dir = route_new_subdirectory(
			&state->arena, areas_dir, area_name);

		add_raw_file(&state->arena, area_dir, "raw",
			     state->image + area->offset, area->size);
		add_boolean_flag_file(&state->arena, area_dir, "static",
				      &area->flags,
				      __builtin_ctz(FMAP_AREA_STATIC));
		add_boolean_flag_file(&state->arena, area_dir, "compressed",
				      &area->flags,
				      __builtin_ctz(FMAP_AREA_COMPRESSED));
		add_boolean_flag_file(&state->arena, area_dir, "ro",
				      &area->flags,
				      __builtin_ctz(FMAP_AREA_RO));
		add_boolean_flag_file(&state->arena, area_dir, "preserve",
				      &area->flags,
				      __builtin_ctz(FMAP_AREA_PRESERVE));

		if (!strcmp(area_name, "GBB")) {
			setup_gbb_files(&state->arena, area_dir,
					state->image + area->offset,
					area->size);
		}
	}

	return 0;
}

static void fill_statbuf_with_dirent(struct directory_entry *entry,
				     struct stat *st)
{
	st->st_mode = entry->mode;
	st->st_uid = geteuid();
	st->st_gid = getegid();

	/* TODO: don't hardcode this? */
	if (S_ISDIR(entry->mode)) {
		st->st_nlink = 2;
	} else if (S_ISREG(entry->mode)) {
		struct file_ops *ops = entry->reg_file.ops;
		void *param = entry->reg_file.param;

		st->st_nlink = 1;
		if (ops->get_size) {
			st->st_size = ops->get_size(param);
		} else if (ops->read) {
			char buf[1024];
			off_t offset = 0;
			size_t bytes_read;

			st->st_size = 0;

			do {
				bytes_read = ops->read(buf, sizeof(buf), offset,
						       NULL, param);

				st->st_size += bytes_read;
				offset += bytes_read;
			} while (bytes_read == sizeof(buf));
		} else {
			st->st_size = 0;
		}
	}
}

static int fmapfs_getattr(const char *path, struct stat *st,
			  struct fuse_file_info *fi)
{
	struct fmapfs_state *state = fuse_get_context()->private_data;
	struct directory_entry *entry;

	entry = route_lookup_path(state->rootdir, path);
	if (!entry) {
		fuse_log(FUSE_LOG_ERR, "Route not found for %s", path);
		return -ENOENT;
	}

	fill_statbuf_with_dirent(entry, st);

	return 0;
}

static int fmapfs_readdir(const char *path, void *buffer,
			  fuse_fill_dir_t filler, off_t offset,
			  struct fuse_file_info *fi,
			  enum fuse_readdir_flags flags)
{
	struct fmapfs_state *state = fuse_get_context()->private_data;
	struct directory_entry *entry;

	entry = route_lookup_path(state->rootdir, path);
	if (!entry) {
		fuse_log(FUSE_LOG_ERR, "Route not found for %s", path);
		return -ENOENT;
	}

	if (!S_ISDIR(entry->mode)) {
		fuse_log(FUSE_LOG_ERR, "%s is not a directory", path);
		return -ENOTDIR;
	}

	filler(buffer, ".", NULL, 0, 0);
	filler(buffer, "..", NULL, 0, 0);

	for (struct dir_list *ent = entry->dir->entries; ent; ent = ent->next) {
		if (flags & FUSE_FILL_DIR_PLUS) {
			struct stat statbuf;

			fill_statbuf_with_dirent(ent->entry, &statbuf);
			filler(buffer, ent->entry->name, &statbuf, 0,
			       FUSE_FILL_DIR_PLUS);
		} else {
			filler(buffer, ent->entry->name, NULL, 0, 0);
		}
	}

	return 0;
}

static int fmapfs_open(const char *path, struct fuse_file_info *fi)
{
	struct fmapfs_state *state = fuse_get_context()->private_data;
	struct directory_entry *entry;
	mode_t accmode;

	entry = route_lookup_path(state->rootdir, path);
	if (!entry) {
		fuse_log(FUSE_LOG_ERR, "Route not found for %s", path);
		return -ENOENT;
	}

	if (!S_ISREG(entry->mode)) {
		fuse_log(FUSE_LOG_ERR, "%s is not a regular file", path);
		return -EISDIR;
	}

	accmode = fi->flags & O_ACCMODE;

	if ((accmode & O_RDONLY) && !entry->reg_file.ops->read) {
		fuse_log(FUSE_LOG_ERR, "No read operation on %s", path);
		return -EACCES;
	}

	if ((accmode & O_WRONLY) && !entry->reg_file.ops->write) {
		fuse_log(FUSE_LOG_ERR, "No write operation on %s", path);
		return -EACCES;
	}

	return 0;
}

static int fmapfs_read(const char *path, char *buf, size_t n_bytes,
		       off_t offset, struct fuse_file_info *fi)
{
	struct fmapfs_state *state = fuse_get_context()->private_data;
	struct directory_entry *entry;

	entry = route_lookup_path(state->rootdir, path);
	if (!entry) {
		fuse_log(FUSE_LOG_ERR, "Route not found for %s", path);
		return -ENOENT;
	}

	if (!S_ISREG(entry->mode)) {
		fuse_log(FUSE_LOG_ERR, "%s is not a regular file", path);
		return -EISDIR;
	}

	if (!entry->reg_file.ops->read) {
		fuse_log(FUSE_LOG_ERR, "%s does not support reading", path);
		return -EOPNOTSUPP;
	}

	return entry->reg_file.ops->read(buf, n_bytes, offset, fi,
					 entry->reg_file.param);
}

static int fmapfs_write(const char *path, const char *buf, size_t n_bytes,
			off_t offset, struct fuse_file_info *fi)
{
	struct fmapfs_state *state = fuse_get_context()->private_data;
	struct directory_entry *entry;

	entry = route_lookup_path(state->rootdir, path);
	if (!entry) {
		fuse_log(FUSE_LOG_ERR, "Route not found for %s", path);
		return -ENOENT;
	}

	if (!S_ISREG(entry->mode)) {
		fuse_log(FUSE_LOG_ERR, "%s is not a regular file", path);
		return -EISDIR;
	}

	if (!entry->reg_file.ops->write) {
		fuse_log(FUSE_LOG_ERR, "%s does not support writing", path);
		return -EOPNOTSUPP;
	}

	return entry->reg_file.ops->write(buf, n_bytes, offset, fi,
					  entry->reg_file.param);
}

const struct fuse_operations fmapfs_ops = {
	.getattr = fmapfs_getattr,
	.readdir = fmapfs_readdir,
	.open = fmapfs_open,
	.read = fmapfs_read,
	.write = fmapfs_write,
};
