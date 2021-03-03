#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "arena.h"

struct arena_header {
	void *ptr;
	size_t bytes_left;
	struct arena_header *parent;
};

static size_t increase_size_to_align(size_t size)
{
	if (size % 8 != 0)
		size += 8 - (size % 8);
	return size;
}

static struct arena_header *finalize_page(struct arena_header *page)
{
	size_t bytes_used;

	if (!page)
		return NULL;

	if (!page->bytes_left)
		return page;

	bytes_used = page->ptr - (void *)page;
	page = realloc(page, bytes_used);
	page->bytes_left = 0;
	return page;
}

static void arena_new_page(struct arena *arena, size_t page_size)
{
	const size_t header_size =
		increase_size_to_align(sizeof(struct arena_header));
	struct arena_header *page = malloc(header_size + page_size);

	page->ptr = ((void *)page) + header_size;
	page->bytes_left = page_size;
	page->parent = finalize_page(arena->pages);
	arena->pages = page;
}

void *arena_malloc(struct arena *arena, size_t member_size, size_t count)
{
	size_t allocation_size = member_size * count;
	size_t bytes_left = 0;
	void *result;

	/* Check for multiplication overflow or zero allocation */
	if (!count || !member_size || member_size != allocation_size / count)
		return NULL;

	allocation_size = increase_size_to_align(allocation_size);

	if (arena->pages)
		bytes_left = arena->pages->bytes_left;

	if (allocation_size > bytes_left) {
		size_t new_page_size = arena->page_size;

		if (new_page_size < allocation_size)
			new_page_size = allocation_size;
		arena_new_page(arena, new_page_size);
	}

	result = arena->pages->ptr;
	arena->pages->ptr += allocation_size;
	arena->pages->bytes_left -= allocation_size;
	return result;
}

void *arena_calloc(struct arena *arena, size_t member_size, size_t count)
{
	void *buf = arena_malloc(arena, member_size, count);

	memset(buf, 0, member_size * count);
	return buf;
}

char *arena_strdup(struct arena *arena, const char *str)
{
	size_t size = strlen(str) + 1;
	char *buf = arena_malloc(arena, sizeof(char), size);

	memcpy(buf, str, size);
	return buf;
}

char *arena_strndup(struct arena *arena, const char *str, size_t maxlen)
{
	size_t len = strnlen(str, maxlen);
	char *buf = arena_malloc(arena, sizeof(char), len + 1);

	memcpy(buf, str, len);
	buf[len] = '\0';
	return buf;
}

void arena_free(struct arena *arena)
{
	struct arena_header *page = arena->pages;
	struct arena_header *parent;

	while (page) {
		parent = page->parent;
		free(page);
		page = parent;
	}
	arena->pages = NULL;
}
