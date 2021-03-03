#ifndef _FMAPFS_MMAP_FILE_H_
#define _FMAPFS_MMAP_FILE_H_

#include <sys/types.h>

ssize_t mmap_file_path(const char *path, int flags, void **ptr_out);

#endif /* _FMAPFS_MMAP_FILE_H_ */
