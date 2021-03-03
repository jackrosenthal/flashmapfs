#include <fuse.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "arena.h"
#include "array_size.h"
#include "fs.h"
#include "log.h"

static void show_help(char *progname)
{
	char *argv[] = {
		progname,
		"--help",
		NULL,
	};

	fprintf(stderr,
		"Usage: %s [fuse options...] [firmware file] [mount path]\n\n",
		progname);
	fuse_main(ARRAY_SIZE(argv) - 1, argv, &fmapfs_ops, NULL);
}

int main(int argc, char *argv[])
{
	int i;
	int rv;
	const char *image_path = NULL;
	struct fmapfs_state fs_state = {
		.arena = ARENA_INIT(),
	};

	bool help_requested = false;

	for (i = 1; i < argc && argv[i][0] == '-'; i++) {
		if (!strcmp(argv[i], "--help") || !strcmp(argv[i], "-h"))
			help_requested = true;
		if (!strcmp(argv[i], "-o"))
			i++;
	}

	if (help_requested || i + 2 != argc) {
		show_help(argv[0]);
		return !help_requested;
	}

	image_path = argv[i];
	argv[i] = argv[i + 1];
	argv[i + 1] = NULL;

	if (fmapfs_load_image(&fs_state, image_path) < 0)
		return 1;

	rv = fuse_main(argc - 1, argv, &fmapfs_ops, &fs_state);
	arena_free(&fs_state.arena);

	return rv;
}
