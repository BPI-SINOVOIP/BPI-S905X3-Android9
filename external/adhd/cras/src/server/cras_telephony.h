/* Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef CRAS_TELEPHONY_H_
#define CRAS_TELEPHONY_H_

#include <dbus/dbus.h>

/* Handle object to hold required info to handle telephony status which
 * is required for responsing HFP query commands.
 * Args:
 *    call - standard call status indicator, where
 *        0: no call active
 *        1: call is active
 *    callsetup - call set up status indicator.
 *        0: not currently in call set up
 *        1: an incoming call prcess ongoing
 *        2: an outgoing call set up is ongoing
 *    callhold - call hold status indicator.
 *        0: no call hold
 *        1: call is placed on hold or active/held calls swapped
 *           (The AG has both and active AND a held call)
 *        2: call on hold, no active call
 *    dial_number - phone number, used on fake memory storage and last phone
 *    number storage.
 *    dbus_conn - dus connetion which is used in whole telephony module.
 */
struct cras_telephony_handle {
	int call;
	int callsetup;
	int callheld;
	char *dial_number;

	DBusConnection *dbus_conn;
};


void cras_telephony_start(DBusConnection *conn);

void cras_telephony_stop();

struct cras_telephony_handle* cras_telephony_get();

/* Stores dial number in telephony module. */
void cras_telephony_store_dial_number(int len, const char* num);

/* Handles answer call event from dbus or HF */
int cras_telephony_event_answer_call();

/* Handles answer call event from dbus or HF */
int cras_telephony_event_terminate_call();

#endif /* CRAS_TELEPHONY_H_ */
