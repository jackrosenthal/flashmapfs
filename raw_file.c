#include <fuse.h>
#include <fmap.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#include "arena.h"
#include "route.h"
#include "raw_file.h"

struct raw_file_priv {
	void *mem;
	size_t size;
};

static size_t get_size(void *param)
{
	struct raw_file_priv *priv = param;

	return priv->size;
}

static int raw_file_read(char *buf, size_t n_bytes, off_t offset,
			 struct fuse_file_info *fi, void *param)
{
	struct raw_file_priv *priv = param;

	if (offset > priv->size)
		return 0;

	if (n_bytes + offset >= priv->size)
		n_bytes = priv->size - offset;

	memcpy(buf, priv->mem + offset, n_bytes);
	return n_bytes;
}

static struct file_ops ops = {
	.get_size = get_size,
	.read = raw_file_read,
};

void add_raw_file(struct arena *arena, struct directory *basedir,
		  const char *name, void *mem, size_t size)
{
	struct raw_file_priv *priv =
		arena_malloc(arena, sizeof(struct raw_file_priv), 1);

	priv->mem = mem;
	priv->size = size;

	route_new_file(arena, basedir, name, &ops, priv);
}
