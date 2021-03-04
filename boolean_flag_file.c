#include <ctype.h>
#include <fuse.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#include "arena.h"
#include "boolean_flag_file.h"
#include "log.h"
#include "route.h"

struct flag_priv {
	uint16_t *val;
	uint16_t mask;
};

static size_t get_size(void *param)
{
	return 2;
}

static int bool_read(char *buf, size_t n_bytes, off_t offset,
		     struct fuse_file_info *fi, void *priv_in)
{
	struct flag_priv *priv = priv_in;
	char val_buf[3] = { 0 };

	if (offset >= 2)
		return 0;

	snprintf(val_buf, sizeof(val_buf), "%d\n",
		 !!(*priv->val & priv->mask));

	if (n_bytes + offset >= 2)
		n_bytes = 2 - offset;

	memcpy(buf, val_buf + offset, n_bytes);
	return n_bytes;
}

static int bool_write(const char *buf, size_t n_bytes, off_t offset,
		      struct fuse_file_info *fi, void *priv_in)
{
	struct flag_priv *priv = priv_in;
	char val;

	if (offset == 1)
		return 1;
	if (offset > 1)
		return 0;
	if (n_bytes == 0)
		return 0;

	val = tolower(buf[0]);
	LOG_DBG("boolean set \"%-.*s\"", (int)n_bytes, buf);
	LOG_DBG("current flags %04X", *priv->val);

	if (val == '0' || val == 't' || val == 'y')
		*priv->val &= ~priv->mask;
	else if (val == '1' || val == 'f' || val == 'n')
		*priv->val |= priv->mask;
	else
		return 0;

	LOG_DBG("new flags %04X", *priv->val);

	return n_bytes;
}

static struct file_ops ops = {
	.get_size = get_size,
	.read = bool_read,
	.write = bool_write,
};

void add_boolean_flag_file(struct arena *arena, struct directory *basedir,
			   const char *name, uint16_t *val, uint16_t mask)
{
	struct flag_priv *priv =
		arena_calloc(arena, sizeof(struct flag_priv), 1);

	priv->val = val;
	priv->mask = mask;

	route_new_file(arena, basedir, name, &ops, priv);
}
