/* Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <iniparser.h>

#include "cras_device_blacklist.h"
#include "utlist.h"

/* Allocate 63 chars + 1 for null where declared. */
static const unsigned int MAX_INI_NAME_LEN = 63;
static const unsigned int MAX_KEY_LEN = 63;

struct cras_device_blacklist {
	dictionary *ini;
};

/*
 * Exported Interface
 */

struct cras_device_blacklist *cras_device_blacklist_create(
		const char *config_path)
{
	struct cras_device_blacklist *blacklist;
	char ini_name[MAX_INI_NAME_LEN + 1];

	blacklist = calloc(1, sizeof(*blacklist));
	if (!blacklist)
		return NULL;

	snprintf(ini_name, MAX_INI_NAME_LEN, "%s/%s",
		 config_path, "device_blacklist");
	ini_name[MAX_INI_NAME_LEN] = '\0';
	blacklist->ini = iniparser_load(ini_name);

	return blacklist;
}

void cras_device_blacklist_destroy(struct cras_device_blacklist *blacklist)
{
	if (blacklist && blacklist->ini)
		iniparser_freedict(blacklist->ini);
	free(blacklist);
}

int cras_device_blacklist_check(struct cras_device_blacklist *blacklist,
				unsigned vendor_id,
				unsigned product_id,
				unsigned desc_checksum,
				unsigned device_index)
{
	char ini_key[MAX_KEY_LEN + 1];

	if (!blacklist)
		return 0;

	snprintf(ini_key, MAX_KEY_LEN, "USB_Outputs:%04x_%04x_%08x_%u",
		 vendor_id, product_id, desc_checksum, device_index);
	ini_key[MAX_KEY_LEN] = 0;
	return iniparser_getboolean(blacklist->ini, ini_key, 0);
}
