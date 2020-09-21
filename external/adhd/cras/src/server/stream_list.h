/* Copyright (c) 2015 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "cras_types.h"
#include "utlist.h"

struct cras_rclient;
struct cras_rstream;
struct cras_rstream_config;
struct cras_audio_format;
struct stream_list;

typedef int (stream_callback)(struct cras_rstream *rstream);
typedef int (stream_create_func)(struct cras_rstream_config *stream_config,
				 struct cras_rstream **rstream);
typedef void (stream_destroy_func)(struct cras_rstream *rstream);

struct stream_list *stream_list_create(stream_callback *add_cb,
				       stream_callback *rm_cb,
				       stream_create_func *create_cb,
				       stream_destroy_func *destroy_cb,
				       struct cras_tm *timer_manager);

void stream_list_destroy(struct stream_list *list);

struct cras_rstream *stream_list_get(struct stream_list *list);

int stream_list_add(struct stream_list *list,
		    struct cras_rstream_config *stream_config,
		    struct cras_rstream **stream);

int stream_list_rm(struct stream_list *list, cras_stream_id_t id);

int stream_list_rm_all_client_streams(struct stream_list *list,
				      struct cras_rclient *rclient);
