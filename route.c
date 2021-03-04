#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "arena.h"
#include "log.h"
#include "route.h"

struct directory_entry *route_new_root(struct arena *arena)
{
	struct directory *dir =
		arena_calloc(arena, sizeof(struct directory), 1);
	struct directory_entry *ent =
		arena_calloc(arena, sizeof(struct directory_entry), 1);

	ent->name = arena_strdup(arena, "ROOTDIR");
	ent->mode = S_IFDIR | 0750;
	ent->dir = dir;

	return ent;
}

void route_add_entry_to_directory(struct arena *arena,
				  struct directory *basedir,
				  struct directory_entry *entry)
{
	struct dir_list *node = arena_calloc(arena, sizeof(struct dir_list), 1);

	node->entry = entry;
	node->next = basedir->entries;
	basedir->entries = node;
}

struct directory *route_new_subdirectory(struct arena *arena,
					 struct directory *basedir,
					 const char *name)
{
	struct directory_entry *entry =
		arena_calloc(arena, sizeof(struct directory_entry), 1);
	struct directory *subdir =
		arena_calloc(arena, sizeof(struct directory), 1);

	entry->mode = S_IFDIR | 0550;
	entry->name = arena_strdup(arena, name);
	entry->dir = subdir;

	route_add_entry_to_directory(arena, basedir, entry);

	return subdir;
}

void route_new_file(struct arena *arena, struct directory *basedir,
		    const char *name, struct file_ops *ops, void *param)
{
	struct directory_entry *entry =
		arena_calloc(arena, sizeof(struct directory_entry), 1);
	mode_t permissions = 0;

	if (ops->read)
		permissions |= 0440;
	if (ops->write)
		permissions |= 0600;

	entry->mode = S_IFREG | permissions;
	entry->name = arena_strdup(arena, name);
	entry->reg_file.ops = ops;
	entry->reg_file.param = param;

	route_add_entry_to_directory(arena, basedir, entry);
}

struct directory_entry *route_lookup_path(struct directory_entry *root,
					  const char *path)
{
	size_t word_len;

	LOG_DBG("Lookup %s in %s", path, root->name);

	path += strspn(path, "/");
	word_len = strcspn(path, "/");

	if (!word_len)
		return root;

	if (!S_ISDIR(root->mode))
		return NULL;

	for (struct dir_list *ent = root->dir->entries; ent; ent = ent->next) {
		/* Disallow partial-path matches */
		if (word_len != strlen(ent->entry->name))
			continue;

		if (!strncmp(ent->entry->name, path, word_len))
			return route_lookup_path(ent->entry, path + word_len);
	}

	return NULL;
}
