/* Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef CRAS_HFP_SLC_H_
#define CRAS_HFP_SLC_H_

struct hfp_slc_handle;

/* Callback to call when service level connection initialized. */
typedef int (*hfp_slc_init_cb)(struct hfp_slc_handle *handle);

/* Callback to call when service level connection disconnected. */
typedef int (*hfp_slc_disconnect_cb)(struct hfp_slc_handle *handle);

/* Creates an hfp_slc_handle to poll the RFCOMM file descriptor
 * to read and handle received AT commands.
 * Args:
 *    fd - the rfcomm fd used to initialize service level connection
 *    is_hsp - if the slc handle is created for headset profile
 *    device - The bt device associated with the created slc object
 *    init_cb - the callback function to be triggered when a service level
 *        connection is initialized.
 *    disconnect_cb - the callback function to be triggered when the service
 *        level connection is disconnected.
 */
struct hfp_slc_handle *hfp_slc_create(int fd, int is_hsp,
				      struct cras_bt_device *device,
				      hfp_slc_init_cb init_cb,
				      hfp_slc_disconnect_cb disconnect_cb);

/* Destroys an hfp_slc_handle. */
void hfp_slc_destroy(struct hfp_slc_handle *handle);

/* Sets the call status to notify handsfree device. */
int hfp_set_call_status(struct hfp_slc_handle *handle, int call);

/* Fakes the incoming call event for qualification test. */
int hfp_event_incoming_call(struct hfp_slc_handle *handle,
			    const char *number,
			    int type);

/* Handles the call status changed event.
 * AG will send notification to HF accordingly. */
int hfp_event_update_call(struct hfp_slc_handle *handle);

/* Handles the call setup status changed event.
 * AG will send notification to HF accordingly. */
int hfp_event_update_callsetup(struct hfp_slc_handle *handle);

/* Handles the call held status changed event.
 * AG will send notification to HF accordingly. */
int hfp_event_update_callheld(struct hfp_slc_handle *handle);

/* Sets battery level which is required for qualification test. */
int hfp_event_set_battery(struct hfp_slc_handle *handle, int value);

/* Sets signal strength which is required for qualification test. */
int hfp_event_set_signal(struct hfp_slc_handle *handle, int value);

/* Sets service availability which is required for qualification test. */
int hfp_event_set_service(struct hfp_slc_handle *handle, int value);

/* Sets speaker gain value to headsfree device. */
int hfp_event_speaker_gain(struct hfp_slc_handle *handle, int gain);

#endif /* CRAS_HFP_SLC_H_ */
