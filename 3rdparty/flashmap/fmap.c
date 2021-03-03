/* Copyright 2010, Google Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *    * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *    * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *    * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Alternatively, this software may be distributed under the terms of the
 * GNU General Public License ("GPL") version 2 as published by the Free
 * Software Foundation.
 */

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "fmap.h"

/* returns size of fmap data structure if successful, <0 to indicate error */
int fmap_size(struct fmap *fmap)
{
	if (!fmap)
		return -1;

	return sizeof(*fmap) + (fmap->nareas * sizeof(struct fmap_area));
}

/* brute force linear search */
static long int fmap_lsearch(const uint8_t *image, size_t len)
{
	long int offset;
	int fmap_found = 0;

	for (offset = 0; offset < len - strlen(FMAP_SIGNATURE); offset++) {
		if (!memcmp(&image[offset],
		            FMAP_SIGNATURE,
		            strlen(FMAP_SIGNATURE))) {
			fmap_found = 1;
			break;
		}
	}

	if (!fmap_found)
		return -1;

	if (offset + fmap_size((struct fmap *)&image[offset]) > len)
		return -1;

	return offset;
}

/* if image length is a power of 2, use binary search */
static long int fmap_bsearch(const uint8_t *image, size_t len)
{
	long int offset = -1;
	int fmap_found = 0, stride;

	/*
	 * For efficient operation, we start with the largest stride possible
	 * and then decrease the stride on each iteration. Also, check for a
	 * remainder when modding the offset with the previous stride. This
	 * makes it so that each offset is only checked once.
	 */
	for (stride = len / 2; stride >= 1; stride /= 2) {
		if (fmap_found)
			break;

		for (offset = 0;
		     offset < len - strlen(FMAP_SIGNATURE);
		     offset += stride) {
			if ((offset % (stride * 2) == 0) && (offset != 0))
					continue;
			if (!memcmp(&image[offset],
			            FMAP_SIGNATURE,
			            strlen(FMAP_SIGNATURE))) {
				fmap_found = 1;
				break;
			}
		}
	}

	if (!fmap_found)
		return -1;

	if (offset + fmap_size((struct fmap *)&image[offset]) > len)
		return -1;

	return offset;
}

static int popcnt(unsigned int u)
{
	int count;

	/* K&R method */
	for (count = 0; u; count++)
		u &= (u - 1);

	return count;
}

long int fmap_find(const uint8_t *image, unsigned int image_len)
{
	long int ret = -1;

	if ((image == NULL) || (image_len == 0))
		return -1;

	if (popcnt(image_len) == 1)
		ret = fmap_bsearch(image, image_len);
	else
		ret = fmap_lsearch(image, image_len);

	return ret;
}

/* allocate and initialize a new fmap structure */
struct fmap *fmap_create(uint64_t base, uint32_t size, uint8_t *name)
{
	struct fmap *fmap;

	fmap = malloc(sizeof(*fmap));
	if (!fmap)
		return NULL;

	memset(fmap, 0, sizeof(*fmap));
	memcpy(&fmap->signature, FMAP_SIGNATURE, strlen(FMAP_SIGNATURE));
	fmap->ver_major = FMAP_VER_MAJOR;
	fmap->ver_minor = FMAP_VER_MINOR;
	fmap->base = base;
	fmap->size = size;
	memccpy(&fmap->name, name, '\0', FMAP_STRLEN);

	return fmap;
}

/* free memory used by an fmap structure */
void fmap_destroy(struct fmap *fmap) {
	free(fmap);
}

/* append area to existing structure, return new total size if successful */
int fmap_append_area(struct fmap **fmap,
                     uint32_t offset, uint32_t size,
                     const uint8_t *name, uint16_t flags)
{
	struct fmap_area *area;
	int orig_size, new_size;

	if ((fmap == NULL || *fmap == NULL) || (name == NULL))
		return -1;

	/* too many areas */
	if ((*fmap)->nareas >= 0xffff)
		return -1;

	orig_size = fmap_size(*fmap);
	new_size = orig_size + sizeof(*area);

	*fmap = realloc(*fmap, new_size);
	if (*fmap == NULL)
		return -1;

	area = (struct fmap_area *)((uint8_t *)*fmap + orig_size);
	memset(area, 0, sizeof(*area));
	memcpy(&area->offset, &offset, sizeof(area->offset));
	memcpy(&area->size, &size, sizeof(area->size));
	memccpy(&area->name, name, '\0', FMAP_STRLEN);
	memcpy(&area->flags, &flags, sizeof(area->flags));

	(*fmap)->nareas++;
	return new_size;
}

const struct fmap_area *fmap_find_area(const struct fmap *fmap,
				       const char *name)
{
	int i;
	const struct fmap_area *area = NULL;

	if (!fmap || !name)
		return NULL;

	for (i = 0; i < fmap->nareas; i++) {
		if (!strcmp((const char *)fmap->areas[i].name, name)) {
			area = &fmap->areas[i];
			break;
		}
	}

	return area;
}
