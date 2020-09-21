/* Copyright (c) 2016 The Chromium Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#include <dbus/dbus.h>
#include <errno.h>
#include <stdlib.h>
#include <syslog.h>

#include "cras_bt_constants.h"
#include "cras_bt_adapter.h"
#include "cras_bt_player.h"
#include "cras_dbus_util.h"
#include "utlist.h"

#define CRAS_DEFAULT_PLAYER "/org/chromium/Cras/Bluetooth/DefaultPlayer"


static void cras_bt_on_player_registered(DBusPendingCall *pending_call,
					 void *data)
{
	DBusMessage *reply;

	reply = dbus_pending_call_steal_reply(pending_call);
	dbus_pending_call_unref(pending_call);

	if (dbus_message_get_type(reply) == DBUS_MESSAGE_TYPE_ERROR) {
		syslog(LOG_ERR, "RegisterPlayer returned error: %s",
		       dbus_message_get_error_name(reply));
		dbus_message_unref(reply);
		return;
	}

	dbus_message_unref(reply);
}

static int cras_bt_add_player(DBusConnection *conn,
			      const struct cras_bt_adapter *adapter,
			      struct cras_bt_player *player)
{
	const char *adapter_path;
	DBusMessage *method_call;
	DBusMessageIter message_iter, dict;
	DBusPendingCall *pending_call;

	adapter_path = cras_bt_adapter_object_path(adapter);
	method_call = dbus_message_new_method_call(BLUEZ_SERVICE,
						   adapter_path,
						   BLUEZ_INTERFACE_MEDIA,
						   "RegisterPlayer");
	if (!method_call)
		return -ENOMEM;

	dbus_message_iter_init_append(method_call, &message_iter);
	dbus_message_iter_append_basic(&message_iter,
				       DBUS_TYPE_OBJECT_PATH,
				       &player->object_path);

	dbus_message_iter_open_container(&message_iter,
					 DBUS_TYPE_ARRAY,
					 DBUS_DICT_ENTRY_BEGIN_CHAR_AS_STRING
					 DBUS_TYPE_STRING_AS_STRING
					 DBUS_TYPE_VARIANT_AS_STRING
					 DBUS_DICT_ENTRY_END_CHAR_AS_STRING,
					 &dict);

	append_key_value(&dict, "PlaybackStatus", DBUS_TYPE_STRING,
			 DBUS_TYPE_STRING_AS_STRING,
			 &player->playback_status);
	append_key_value(&dict, "Identity", DBUS_TYPE_STRING,
			 DBUS_TYPE_STRING_AS_STRING,
			 &player->identity);
	append_key_value(&dict, "LoopStatus", DBUS_TYPE_STRING,
			 DBUS_TYPE_STRING_AS_STRING,
			 &player->loop_status);
	append_key_value(&dict, "Position", DBUS_TYPE_INT64,
			 DBUS_TYPE_INT64_AS_STRING, &player->position);
	append_key_value(&dict, "Shuffle", DBUS_TYPE_BOOLEAN,
			 DBUS_TYPE_BOOLEAN_AS_STRING, &player->shuffle);
	append_key_value(&dict, "CanGoNext", DBUS_TYPE_BOOLEAN,
			 DBUS_TYPE_BOOLEAN_AS_STRING, &player->can_go_next);
	append_key_value(&dict, "CanGoPrevious", DBUS_TYPE_BOOLEAN,
			 DBUS_TYPE_BOOLEAN_AS_STRING, &player->can_go_prev);
	append_key_value(&dict, "CanPlay", DBUS_TYPE_BOOLEAN,
			 DBUS_TYPE_BOOLEAN_AS_STRING, &player->can_play);
	append_key_value(&dict, "CanPause", DBUS_TYPE_BOOLEAN,
			 DBUS_TYPE_BOOLEAN_AS_STRING, &player->can_pause);
	append_key_value(&dict, "CanControl", DBUS_TYPE_BOOLEAN,
			 DBUS_TYPE_BOOLEAN_AS_STRING, &player->can_control);

	dbus_message_iter_close_container(&message_iter, &dict);

	if (!dbus_connection_send_with_reply(conn, method_call, &pending_call,
					     DBUS_TIMEOUT_USE_DEFAULT)) {
		dbus_message_unref(method_call);
		return -ENOMEM;
	}

	dbus_message_unref(method_call);
	if (!pending_call)
		return -EIO;

	if (!dbus_pending_call_set_notify(pending_call,
					  cras_bt_on_player_registered,
					  player, NULL)) {
		dbus_pending_call_cancel(pending_call);
		dbus_pending_call_unref(pending_call);
		return -ENOMEM;
	}
	return 0;
}


/* Note that player properties will be used mostly for AVRCP qualification and
 * not for normal use cases. The corresponding media events won't be routed by
 * CRAS until we have a plan to provide general system API to handle media
 * control.
 */
static struct cras_bt_player player = {
	.object_path = CRAS_DEFAULT_PLAYER,
	.playback_status = "playing",
	.identity = "DefaultPlayer",
	.loop_status = "None",
	.shuffle = 0,
	.position = 0,
	.can_go_next = 0,
	.can_go_prev = 0,
	.can_play = 0,
	.can_pause = 0,
	.can_control = 0,
	.message_cb = NULL,
};

static DBusHandlerResult cras_bt_player_handle_message(DBusConnection *conn,
						       DBusMessage *message,
						       void *arg)
{
	const char *msg = dbus_message_get_member(message);

	if (player.message_cb)
		player.message_cb(msg);

	return DBUS_HANDLER_RESULT_HANDLED;
}

int cras_bt_player_create(DBusConnection *conn)
{
	static const DBusObjectPathVTable player_vtable = {
	        .message_function = cras_bt_player_handle_message
	};

	DBusError dbus_error;
	struct cras_bt_adapter **adapters;
	size_t num_adapters, i;

	dbus_error_init(&dbus_error);

	if (!dbus_connection_register_object_path(conn,
						  player.object_path,
						  &player_vtable,
						  &dbus_error)) {
		syslog(LOG_ERR, "Cannot register player %s",
		       player.object_path);
		dbus_error_free(&dbus_error);
		return -ENOMEM;
	}

	num_adapters = cras_bt_adapter_get_list(&adapters);
	for (i = 0; i < num_adapters; ++i)
		cras_bt_add_player(conn, adapters[i], &player);
	free(adapters);
	return 0;
}

int cras_bt_register_player(DBusConnection *conn,
			    const struct cras_bt_adapter *adapter)
{
	return cras_bt_add_player(conn, adapter, &player);
}
