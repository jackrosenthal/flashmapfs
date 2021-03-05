#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "arena.h"
#include "array_size.h"
#include "boolean_flag_file.h"
#include "gbb.h"
#include "log.h"
#include "route.h"
#include "str_file.h"

#define GBB_SIGNATURE "$GBB"

const static struct {
	int bit;
	const char *filename;
} gbb_flags[] = {
	{ 0, "dev-screen-short-delay" },
	{ 1, "load-option-roms" },
	{ 2, "enable-alternate-os" },
	{ 3, "force-dev-mode" },
	{ 4, "force-dev-boot-usb" },
	{ 5, "disable-fw-rollback-check" },
	{ 6, "enter-triggers-tonorm" },
	{ 7, "force-dev-boot-altfw" },
	{ 8, "running-faft" },
	{ 9, "disable-ec-software-sync" },
	{ 10, "default-boot-altfw" },
	{ 11, "disable-auxfw-software-sync" },
	{ 12, "disable-shutdown-on-lid-close" },
	{ 14, "force-manual-recovery" },
	{ 15, "disable-fwmp" },
	{ 16, "enable-udc" },
};

struct gbb_field_locator {
	uint32_t offset;
	uint32_t size;
} __attribute__((packed));

struct gbb_header {
	uint8_t signature[__builtin_strlen(GBB_SIGNATURE)];
	uint16_t major_version;
	uint16_t minor_version;
	uint32_t header_size;

	uint32_t flags;

	struct gbb_field_locator hwid;
	struct gbb_field_locator root_key;
	struct gbb_field_locator bmp_fv;
	struct gbb_field_locator recovery_key;

	/* Added in version 1.2 */
	uint8_t hwid_digest[32];

	uint8_t reserved[48];
} __attribute__((packed));

_Static_assert(sizeof(struct gbb_header) == 128,
	       "GBB header should be 128 bytes in size");

int setup_gbb_files(struct arena *arena, struct directory *basedir,
		    void *gbb_mem, size_t gbb_size)
{
	struct gbb_header *header;
	struct directory *gbb_dir;
	struct directory *flags_dir;

	if (gbb_size < sizeof(struct gbb_header)) {
		LOG_ERR("GBB area is too small to be a GBB partition");
		return -1;
	}

	if (strncmp((const char *)gbb_mem, GBB_SIGNATURE,
		    __builtin_strlen(GBB_SIGNATURE))) {
		LOG_ERR("GBB header has invalid signature");
		return -1;
	}

	header = gbb_mem;
	gbb_dir = route_new_subdirectory(arena, basedir, "gbb-data");
	flags_dir = route_new_subdirectory(arena, gbb_dir, "flags");

	for (size_t i = 0; i < ARRAY_SIZE(gbb_flags); i++) {
		add_boolean_flag_file(arena, flags_dir, gbb_flags[i].filename,
				      &header->flags, gbb_flags[i].bit);
	}

	add_str_file(arena, gbb_dir, "hwid",
		     (char *)gbb_mem + header->hwid.offset, header->hwid.size,
		     true);

	LOG_INF("GBB format detected and setup");

	return 0;
}
