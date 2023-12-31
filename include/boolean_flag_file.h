#ifndef _FMAPFS_BOOLEAN_FLAG_FILE_H_
#define _FMAPFS_BOOLEAN_FLAG_FILE_H_

#include <stdint.h>

struct arena;
struct directory;

void add_boolean_flag_file(struct arena *arena, struct directory *basedir,
			   const char *name, void *val, unsigned int mask);

#endif /* _FMAPFS_BOOLEAN_FLAG_FILE_H_ */
