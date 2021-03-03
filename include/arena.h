#ifndef _ARENA_H_
#define _ARENA_H_

#include <stddef.h>

struct arena_header;

struct arena {
	size_t page_size;
	struct arena_header *pages;
};

/* Page size is optional */
#define ARENA_INIT(args...) _ARENA_INIT1(garbage, ##args, (1 << 20))
#define _ARENA_INIT1(garbage, _page_size, ...) \
	{                                      \
		.page_size = _page_size,       \
	}

void *arena_malloc(struct arena *arena, size_t member_size, size_t count);
void *arena_calloc(struct arena *arena, size_t member_size, size_t count);
char *arena_strdup(struct arena *arena, const char *str);
char *arena_strndup(struct arena *arena, const char *str, size_t maxlen);
void arena_free(struct arena *arena);

#endif /* _ARENA_H_ */
