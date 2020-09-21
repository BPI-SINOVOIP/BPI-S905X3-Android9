/* Copyright (c) 2015 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "cras_rstream.h"
#include "cras_tm.h"
#include "cras_types.h"
#include "stream_list.h"
#include "utlist.h"

struct stream_list {
	struct cras_rstream *streams;
	struct cras_rstream *streams_to_delete;
	stream_callback *stream_added_cb;
	stream_callback *stream_removed_cb;
	stream_create_func *stream_create_cb;
	stream_destroy_func *stream_destroy_cb;
	struct cras_tm *timer_manager;
	struct cras_timer *drain_timer;
};

static void delete_streams(struct cras_timer *timer, void *data)
{
	struct cras_rstream *to_delete;
	struct stream_list *list = (struct stream_list *)data;
	int max_drain_delay = 0;

	DL_FOREACH(list->streams_to_delete, to_delete) {
		int drain_delay;

		drain_delay = list->stream_removed_cb(to_delete);
		if (drain_delay) {
			max_drain_delay = MAX(max_drain_delay, drain_delay);
			continue;
		}
		DL_DELETE(list->streams_to_delete, to_delete);
		list->stream_destroy_cb(to_delete);
	}

	list->drain_timer = NULL;
	if (max_drain_delay)
		list->drain_timer = cras_tm_create_timer(list->timer_manager,
				MAX(max_drain_delay, 10), delete_streams, list);
}

/*
 * Exported Interface
 */

struct stream_list *stream_list_create(stream_callback *add_cb,
				       stream_callback *rm_cb,
				       stream_create_func *create_cb,
				       stream_destroy_func *destroy_cb,
				       struct cras_tm *timer_manager)
{
	struct stream_list *list = calloc(1, sizeof(struct stream_list));

	list->stream_added_cb = add_cb;
	list->stream_removed_cb = rm_cb;
	list->stream_create_cb = create_cb;
	list->stream_destroy_cb = destroy_cb;
	list->timer_manager = timer_manager;
	return list;
}

void stream_list_destroy(struct stream_list *list)
{
	free(list);
}

struct cras_rstream *stream_list_get(struct stream_list *list)
{
	return list->streams;
}

int stream_list_add(struct stream_list *list,
		    struct cras_rstream_config *stream_config,
		    struct cras_rstream **stream)
{
	int rc;

	rc = list->stream_create_cb(stream_config, stream);
	if (rc)
		return rc;

	DL_APPEND(list->streams, *stream);
	rc = list->stream_added_cb(*stream);
	if (rc) {
		DL_DELETE(list->streams, *stream);
		list->stream_destroy_cb(*stream);
	}

	return rc;
}

int stream_list_rm(struct stream_list *list, cras_stream_id_t id)
{
	struct cras_rstream *to_remove;

	DL_SEARCH_SCALAR(list->streams, to_remove, stream_id, id);
	if (!to_remove)
		return -EINVAL;
	DL_DELETE(list->streams, to_remove);
	DL_APPEND(list->streams_to_delete, to_remove);
	if (list->drain_timer) {
		cras_tm_cancel_timer(list->timer_manager, list->drain_timer);
		list->drain_timer = NULL;
	}
	delete_streams(NULL, list);

	return 0;
}

int stream_list_rm_all_client_streams(struct stream_list *list,
				      struct cras_rclient *rclient)
{
	struct cras_rstream *to_remove;
	int rc = 0;

	DL_FOREACH(list->streams, to_remove) {
		if (to_remove->client == rclient) {
			DL_DELETE(list->streams, to_remove);
			DL_APPEND(list->streams_to_delete, to_remove);
		}
	}
	if (list->drain_timer) {
		cras_tm_cancel_timer(list->timer_manager, list->drain_timer);
		list->drain_timer = NULL;
	}
	delete_streams(NULL, list);

	return rc;

}

