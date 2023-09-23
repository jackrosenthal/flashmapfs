#include <fmap.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#include <fuse.h>
#include <fuse_log.h>

#include "arena.h"
#include "route.h"
#include "str_file.h"

struct str_file_priv {
	char *str;
	size_t max_size;
	bool add_newline;
};

static size_t get_size(void *param)
{
	struct str_file_priv *priv = param;

	return strnlen(priv->str, priv->max_size) + priv->add_newline;
}

static int str_file_read(char *buf, size_t n_bytes, off_t offset,
			 struct fuse_file_info *fi, void *param)
{
	struct str_file_priv *priv = param;
	size_t size = get_size(priv);

	if (offset > size)
		return 0;

	if (n_bytes + offset >= size)
		n_bytes = size - offset;

	if (!n_bytes)
		return 0;

	if (offset + n_bytes == size) {
		memcpy(buf, priv->str + offset, n_bytes - 1);
		buf[n_bytes - 1] = '\n';
	} else {
		fuse_log(
			FUSE_LOG_DEBUG,
			"Newline not added (offset=%zu, n_bytes=%zu, size=%zu)",
			offset, n_bytes, size);
		memcpy(buf, priv->str + offset, n_bytes);
	}

	return n_bytes;
}

static int str_file_write(const char *buf, size_t n_bytes, off_t offset,
			  struct fuse_file_info *fi, void *param)
{
	struct str_file_priv *priv = param;
	size_t len;
	bool newline_chomped = false;

	if (offset >= priv->max_size)
		return 0;

	len = strnlen(buf, n_bytes);

	if (priv->add_newline && len && buf[len - 1] == '\n') {
		fuse_log(FUSE_LOG_DEBUG, "chomped newline on write");
		n_bytes -= 1;
		newline_chomped = true;
	}

	if (n_bytes + offset >= priv->max_size)
		n_bytes = priv->max_size - offset;

	memcpy(priv->str + offset, buf, n_bytes);
	memset(priv->str + offset + n_bytes, 0,
	       priv->max_size - n_bytes - offset);

	return n_bytes + newline_chomped;
}

static struct file_ops ops = {
	.get_size = get_size,
	.read = str_file_read,
	.write = str_file_write,
};

void add_str_file(struct arena *arena, struct directory *basedir,
		  const char *name, char *str, size_t max_size,
		  bool add_newline)
{
	struct str_file_priv *priv =
		arena_malloc(arena, sizeof(struct str_file_priv), 1);

	priv->str = str;
	priv->max_size = max_size;
	priv->add_newline = add_newline;

	route_new_file(arena, basedir, name, &ops, priv);
}
