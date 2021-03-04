#ifndef _FMAPFS_STR_FILE_H_
#define _FMAPFS_STR_FILE_H_

#include <stdbool.h>

void add_str_file(struct arena *arena, struct directory *basedir,
		  const char *name, char *str, size_t max_size,
		  bool add_newline);

#endif /* _FMAPFS_STR_FILE_H_ */
