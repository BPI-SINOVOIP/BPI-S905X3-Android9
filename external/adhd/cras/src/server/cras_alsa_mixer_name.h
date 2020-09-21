/* Copyright 2016 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef _CRAS_ALSA_MIXER_NAME_H
#define _CRAS_ALSA_MIXER_NAME_H

#include "cras_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Type of mixer control. */
typedef enum mixer_name_type {
	MIXER_NAME_UNDEFINED,
	MIXER_NAME_MAIN_VOLUME,
	MIXER_NAME_VOLUME,
} mixer_name_type;

/* Represents a list of mixer names found in ALSA. */
struct mixer_name {
	const char* name;
	enum CRAS_STREAM_DIRECTION dir;
	mixer_name_type type;
	struct mixer_name *prev, *next;
};

/* Add a name to the list.
 *
 * Args:
 *    names - A list of controls (may be NULL).
 *    name - The name to add.
 *    dir - The direction for this control.
 *    type - The type control being added.
 *
 * Returns:
 *    Returns the new head of the list (which changes only
 *    when names is NULL).
 */
struct mixer_name *mixer_name_add(struct mixer_name *names,
				  const char *name,
				  enum CRAS_STREAM_DIRECTION dir,
				  mixer_name_type type);

/* Add an array of name to the list.
 *
 * Args:
 *    names - A list of controls (may be NULL).
 *    name_array - The names to add.
 *    name_array_size - The size of name_array.
 *    dir - The direction for these controls.
 *    type - The type controls being added.
 *
 * Returns:
 *    Returns the new head of the list (which changes only
 *    when names is NULL).
 */
struct mixer_name *mixer_name_add_array(struct mixer_name *names,
					const char * const *name_array,
					size_t name_array_size,
					enum CRAS_STREAM_DIRECTION dir,
					mixer_name_type type);

/* Frees a list of names.
 *
 * Args:
 *    names - A list of names.
 */
void mixer_name_free(struct mixer_name *names);

/* Find the mixer_name for the given direction, name, and type.
 *
 * Args:
 *    names - A list of names (may be NULL).
 *    name - The name to find, or NULL to match by type.

 *    dir - The direction to match.
 *    type - The type to match, or MIXER_NAME_UNDEFINED to
 *           match by name only.
 *
 * Returns:
 *    Returns a pointer to the matching struct mixer_name or NULL if
 *    not found.
 */
struct mixer_name *mixer_name_find(struct mixer_name *names,
				   const char *name,
				   enum CRAS_STREAM_DIRECTION dir,
				   mixer_name_type type);

/* Dump the list of mixer names to DEBUG logs.
 *
 * Args:
 *    names - A list of names to dump.
 *    message - A message to print beforehand.
 */
void mixer_name_dump(struct mixer_name *names, const char *message);

#ifdef __cplusplus
}
#endif

#endif /* _CRAS_ALSA_MIXER_NAME_H */
