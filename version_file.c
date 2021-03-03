#include <fuse.h>
#include <fmap.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#include "arena.h"
#include "log.h"
#include "route.h"
#include "version_file.h"

static int version_read(char *buf, size_t n_bytes, off_t offset,
			struct fuse_file_info *fi, void *fmap_in)
{
	struct fmap *fmap = fmap_in;
	size_t ver_len;
	char ver_buf[9] = { 0 };

	LOG_DBG("read %zu bytes, at offset %zu", n_bytes, offset);

	snprintf(ver_buf, sizeof(ver_buf), "%hhu.%hhu\n", fmap->ver_major,
		 fmap->ver_minor);
	ver_len = strlen(ver_buf);

	if (offset >= ver_len)
		return 0;

	if (n_bytes + offset >= ver_len) {
		n_bytes = ver_len - offset;
		LOG_DBG("adjust n_bytes to %zu", n_bytes);
	}

	memcpy(buf, ver_buf + offset, n_bytes);
	return n_bytes;
}

static int version_write(const char *buf, size_t n_bytes, off_t offset,
			 struct fuse_file_info *fi, void *fmap_in)
{
	struct fmap *fmap = fmap_in;
	char ver_buf[16] = { 0 };

	version_read(ver_buf, sizeof(ver_buf), 0, fi, fmap);
	if (offset >= sizeof(ver_buf) - 1)
		return 0;
	if (n_bytes + offset >= sizeof(ver_buf) - 1)
		n_bytes = sizeof(ver_buf) - offset - 1;

	memcpy(ver_buf + offset, buf, n_bytes);
	sscanf(ver_buf, "%hhu.%hhu", &fmap->ver_major, &fmap->ver_minor);

	return n_bytes;
}

static struct file_ops ops = {
	.read = version_read,
	.write = version_write,
};

void add_version_file(struct arena *arena, struct directory *basedir,
		      const char *name, struct fmap *fmap)
{
	route_new_file(arena, basedir, name, &ops, fmap);
}
