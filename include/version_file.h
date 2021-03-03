#ifndef _FMAPFS_VERSION_FILE_H_
#define _FMAPFS_VERSION_FILE_H_

void add_version_file(struct arena *arena, struct directory *basedir,
		      const char *name, struct fmap *fmap);

#endif /* _FMAPFS_VERSION_FILE_H_ */
