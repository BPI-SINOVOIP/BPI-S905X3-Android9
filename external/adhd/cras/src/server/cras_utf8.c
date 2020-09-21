/* Copyright 2016 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <stdlib.h>
#include <stdint.h>
#include <sys/types.h>

#ifdef CRAS_DBUS
#include <dbus/dbus.h>
#endif

#include "cras_utf8.h"
#include "cras_util.h"

static const uint8_t kUTF8ByteOrderMask[3] = { 0xef, 0xbb, 0xbf };

typedef struct u8range {
	uint8_t min;
	uint8_t max;
} u8range_t;

static const u8range_t kUTF8TwoByteSeq[] = {
	{ 0xc2, 0xdf },
	{ 0x80, 0xbf },
	{ 0, 0 }
};

static const u8range_t kUTF8ByteSeqE0[] = {
	{ 0xe0, 0xe0 },
	{ 0xa0, 0xbf },
	{ 0x80, 0xbf },
	{ 0, 0 }
};

static const u8range_t kUTF8ByteSeqE1EC[] = {
	{ 0xe1, 0xec },
	{ 0x80, 0xbf },
	{ 0x80, 0xbf },
	{ 0, 0 }
};

static const u8range_t kUTF8ByteSeqED[] = {
	{ 0xed, 0xed },
	{ 0x80, 0x9f },
	{ 0x80, 0xbf },
	{ 0, 0 }
};

static const u8range_t kUTF8ByteSeqEEEF[] = {
	{ 0xee, 0xef },
	{ 0x80, 0xbf },
	{ 0x80, 0xbf },
	{ 0, 0 }
};

static const u8range_t kUTF8ByteSeqF0[] = {
	{ 0xf0, 0xf0 },
	{ 0x90, 0xbf },
	{ 0x80, 0xbf },
	{ 0x80, 0xbf },
	{ 0, 0 }
};

static const u8range_t kUTF8ByteSeqF1F3[] = {
	{ 0xf1, 0xf3 },
	{ 0x80, 0xbf },
	{ 0x80, 0xbf },
	{ 0x80, 0xbf },
	{ 0, 0 }
};

static const u8range_t kUTF8ByteSeqF4[] = {
	{ 0xf4, 0xf4 },
	{ 0x80, 0x8f },
	{ 0x80, 0xbf },
	{ 0x80, 0xbf },
	{ 0, 0 }
};

static const u8range_t kUTF8NullRange[] = {
	{ 0, 0 }
};

typedef struct utf8seq {
	const u8range_t *ranges;
} utf8seq_t;

static const utf8seq_t kUTF8Sequences[] = {
	{ kUTF8TwoByteSeq },
	{ kUTF8ByteSeqE0 },
	{ kUTF8ByteSeqE1EC },
	{ kUTF8ByteSeqED },
	{ kUTF8ByteSeqEEEF },
	{ kUTF8ByteSeqF0 },
	{ kUTF8ByteSeqF1F3 },
	{ kUTF8ByteSeqF4 },
	{ kUTF8NullRange }
};

int valid_utf8_string(const char *string, size_t *bad_pos)
{
	int bom_chars = 0;
	uint8_t byte;
	const char *pos = string;
	int ret = 1;
	const utf8seq_t *seq = NULL;
	const u8range_t *range = NULL;

	if (!pos) {
		ret = 0;
		goto error;
	}

	while ((byte = (uint8_t)*(pos++))) {
		if (!range || range->min == 0) {
			if (byte < 128) {
				/* Ascii character. */
				continue;
			}

			if (bom_chars < ARRAY_SIZE(kUTF8ByteOrderMask)) {
				if (byte == kUTF8ByteOrderMask[bom_chars]) {
					bom_chars++;
					continue;
				} else {
					/* Characters not matching BOM.
					 * Rewind and assume that there is
					 * no BOM. */
					bom_chars =
					        ARRAY_SIZE(kUTF8ByteOrderMask);
                                        pos = string;
					continue;
				}
			}

			/* Find the matching sequence of characters by
			 * matching the first character in the sequence.
			 */
			seq = kUTF8Sequences;
			while (seq->ranges->min != 0) {
				if (byte >= seq->ranges->min &&
				    byte <= seq->ranges->max) {
					/* Matching sequence. */
					break;
				}
				seq++;
			}

			if (seq->ranges->min == 0) {
				/* Could not find a matching sequence. */
				ret = 0;
				goto error;
			}

			/* Found the appropriate sequence. */
			range = seq->ranges + 1;
			continue;
		}

		if (byte >= range->min && byte <= range->max) {
			range++;
			continue;
		}

		/* This character doesn't belong in UTF8. */
		ret = 0;
		goto error;
	}

	if (range && range->min != 0) {
	        /* Stopped in the middle of a sequence. */
	        ret = 0;
	}

error:
	if (bad_pos)
		*bad_pos = pos - string - 1;
	return ret;
}

#ifdef CRAS_DBUS
/* Use the DBus implementation if available to ensure that the UTF-8
 * sequences match those expected by the DBus implementation. */

int is_utf8_string(const char *string)
{
	return !!dbus_validate_utf8(string, NULL);
}

#else

int is_utf8_string (const char *string) {
	return valid_utf8_string(string, NULL);
}

#endif
