#ifndef _FMAPFS_FS_H_
#define _FMAPFS_FS_H_

#include <stdint.h>
#include <sys/types.h>

#include "arena.h"

struct fmap;
struct directory_entry;
struct fmapfs_state {
	void *image;
	ssize_t image_size;
	struct fmap *fmap;
	struct directory_entry *rootdir;
	struct arena arena;
};

int fmapfs_load_image(struct fmapfs_state *state, const char *image_path);

struct fuse_operations;
extern const struct fuse_operations fmapfs_ops;

#endif /* _FMAPFS_FS_H_ */
