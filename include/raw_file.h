#ifndef _FMAPFS_RAW_FILE_H_
#define _FMAPFS_RAW_FILE_H_

void add_raw_file(struct arena *arena, struct directory *basedir,
		  const char *name, void *mem, size_t size);

#endif /* _FMAPFS_RAW_FILE_H_ */
