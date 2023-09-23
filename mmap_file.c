#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>

#include <fuse_log.h>

#include "mmap_file.h"

static ssize_t fd_seek_size(int fd)
{
	ssize_t file_size;

	file_size = lseek(fd, 0, SEEK_END);
	if (file_size < 0)
		return -1;

	if (lseek(fd, 0, SEEK_SET) < 0)
		return -1;

	return file_size;
}

ssize_t mmap_file_path(const char *path, int flags, void **ptr_out)
{
	int fd;
	ssize_t file_size;
	void *buf;
	int mmap_prot = 0;

	fd = open(path, flags, 0666);
	if (fd < 0) {
		fuse_log(FUSE_LOG_ERR, "Unable to open %s: %s", path,
			 strerror(errno));
		return -1;
	}

	file_size = fd_seek_size(fd);
	if (file_size < 0) {
		fuse_log(FUSE_LOG_ERR, "Unable to seek on %s: %s", path,
			 strerror(errno));
		goto exit;
	}

	if (flags | O_RDONLY)
		mmap_prot |= PROT_READ;
	if (flags | O_WRONLY)
		mmap_prot |= PROT_WRITE;

	buf = mmap(NULL, file_size, mmap_prot, MAP_SHARED, fd, 0);
	if (buf == MAP_FAILED) {
		fuse_log(FUSE_LOG_ERR, "Failed to mmap %s: %s", path,
			 strerror(errno));
		file_size = -1;
		goto exit;
	}

	*ptr_out = buf;

exit:
	close(fd);
	return file_size;
}
