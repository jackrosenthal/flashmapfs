#ifndef _FMAPFS_GBB_H_
#define _FMAPFS_GBB_H_

#include <sys/types.h>

struct arena;
struct directory;

int setup_gbb_files(struct arena *arena, struct directory *basedir,
		    void *gbb_mem, size_t gbb_size);

#endif /* _FMAPFS_GBB_H_ */
