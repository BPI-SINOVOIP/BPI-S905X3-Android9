/* Copyright 2016 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <stdlib.h>
#include <string.h>
#include <syslog.h>

#include "cras_alsa_ucm_section.h"
#include "cras_alsa_mixer_name.h"
#include "utlist.h"

static void ucm_section_free(struct ucm_section *section) {
	if (section->name)
		free((void *)section->name);
	if (section->jack_name)
		free((void *)section->jack_name);
	if (section->jack_type)
		free((void *)section->jack_type);
	if (section->mixer_name)
		free((void *)section->mixer_name);
	mixer_name_free(section->coupled);
	free(section);
}

void ucm_section_free_list(struct ucm_section *sections)
{
	struct ucm_section *section;
	DL_FOREACH(sections, section) {
		DL_DELETE(sections, section);
		ucm_section_free(section);
	}
}

struct ucm_section *ucm_section_create(const char *name,
				       int dev_idx,
				       enum CRAS_STREAM_DIRECTION dir,
				       const char *jack_name,
				       const char *jack_type)
{
	struct ucm_section *section_list = NULL;
	struct ucm_section *section;

	if (!name)
		return NULL;

	section = (struct ucm_section *)
		  calloc(1, sizeof(struct ucm_section));
	if (!section)
		return NULL;

	section->dev_idx = dev_idx;
	section->dir = dir;
	section->name = strdup(name);
	if (!section->name)
		goto error;

	if (jack_name) {
		section->jack_name = strdup(jack_name);
		if (!section->jack_name)
			goto error;
	}
	if (jack_type) {
		section->jack_type = strdup(jack_type);
		if (!section->jack_type)
			goto error;
	}
	/* Default to -1 which means auto-detect. */
	section->jack_switch = -1;

	/* Make sure to initialize this item as a list. */
	DL_APPEND(section_list, section);
	return section_list;

error:
	ucm_section_free(section);
	return NULL;
}

int ucm_section_set_mixer_name(struct ucm_section *section,
			       const char *name)
{
	if (!section || !name)
		return -EINVAL;

	if (section->mixer_name)
		free((void *)section->mixer_name);
	section->mixer_name = strdup(name);
	if (!section->mixer_name)
		return -ENOMEM;
	return 0;
}

int ucm_section_add_coupled(struct ucm_section *section,
			    const char *name,
			    mixer_name_type type)
{
	struct mixer_name *m_name;

	if (!section || !name || type == MIXER_NAME_UNDEFINED)
		return -EINVAL;

	m_name = mixer_name_add(NULL, name, section->dir, type);
	if (!m_name)
		return -ENOMEM;
	DL_APPEND(section->coupled, m_name);
	return 0;
}

int ucm_section_concat_coupled(struct ucm_section *section,
			       struct mixer_name *coupled)
{
	if (!section || !coupled)
		return -EINVAL;
	DL_CONCAT(section->coupled, coupled);
	return 0;
}

void ucm_section_dump(struct ucm_section *section)
{
	syslog(LOG_DEBUG, "section: %s [%d] (%s)",
		section->name, section->dev_idx,
		section->dir == CRAS_STREAM_OUTPUT ? "output" : "input");
	syslog(LOG_DEBUG, "  jack: %s %s",
		section->jack_name, section->jack_type);
	syslog(LOG_DEBUG, "  mixer_name: %s", section->mixer_name);
	mixer_name_dump(section->coupled, "  coupled");
}
