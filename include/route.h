#ifndef _FMAPFS_ROUTE_H_
#define _FMAPFS_ROUTE_H_

#include <fuse.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "arena.h"

struct directory;

struct file_ops {
	size_t (*get_size)(void *param);
	int (*read)(char *buf, size_t n_bytes, off_t offset,
		    struct fuse_file_info *fi, void *param);
	int (*write)(const char *buf, size_t n_bytes, off_t offset,
		     struct fuse_file_info *fi, void *param);
};

struct directory_entry {
	mode_t mode;
	char *name;
	union {
		struct {
			struct file_ops *ops;
			void *param;
		} reg_file;
		struct directory *dir;
	};
};

struct dir_list {
	struct directory_entry *entry;
	struct dir_list *next;
};

struct directory {
	struct dir_list *entries;
};

struct directory_entry *route_new_root(struct arena *arena);
void route_add_entry_to_directory(struct arena *arena,
				  struct directory *basedir,
				  struct directory_entry *entry);
struct directory *route_new_subdirectory(struct arena *arena,
					 struct directory *basedir,
					 const char *name);
void route_new_file(struct arena *arena, struct directory *basedir,
		    const char *name, struct file_ops *ops, void *param);

struct directory_entry *route_lookup_path(struct directory_entry *root,
					  const char *path);

#endif /* _FMAPFS_ROUTE_H_ */
