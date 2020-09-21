/* Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef CRAS_HFP_AG_PROFILE_H_
#define CRAS_HFP_AG_PROFILE_H_

#include <dbus/dbus.h>

#include "cras_bt_device.h"

/* Hands-free Audio Gateway feature bits, listed in according
 * to their order in the bitmap defined in HFP spec.
 */
/* Call waiting and 3-way calling */
#define HFP_THREE_WAY_CALLING           0x0001
/* EC and/or NR function */
#define HFP_EC_ANDOR_NR                 0x0002
/* Voice recognition activation */
#define HFP_VOICE_RECOGNITION           0x0004
/* Inband ringtone */
#define HFP_INBAND_RINGTONE             0x0008
/* Attach a number to voice tag */
#define HFP_ATTACH_NUMBER_TO_VOICETAG   0x0010
/* Ability to reject a call */
#define HFP_REJECT_A_CALL               0x0020
/* Enhanced call status */
#define HFP_ENHANCED_CALL_STATUS        0x0040
/* Enhanced call control */
#define HFP_ENHANCED_CALL_CONTRO        0x0080
/* Extended error result codes */
#define HFP_EXTENDED_ERROR_RESULT_CODES 0x0100
/* Codec negotiation */
#define HFP_CODEC_NEGOTIATION           0x0200

#define HFP_SUPPORTED_FEATURE           (HFP_ENHANCED_CALL_STATUS)

struct hfp_slc_handle;

/* Adds a profile instance for HFP AG (Hands-Free Profile Audio Gateway). */
int cras_hfp_ag_profile_create(DBusConnection *conn);


/* Adds a profile instance for HSP AG (Headset Profile Audio Gateway). */
int cras_hsp_ag_profile_create(DBusConnection *conn);

/* Starts the HFP audio gateway for audio input/output. */
int cras_hfp_ag_start(struct cras_bt_device *device);

/* Suspends all connected audio gateways, used to stop HFP/HSP audio when
 * an A2DP only device is connected. */
void cras_hfp_ag_suspend();

/* Suspends audio gateway associated with given bt device. */
void cras_hfp_ag_suspend_connected_device(struct cras_bt_device *device);

/* Gets the active SLC handle. Used for HFP qualification. */
struct hfp_slc_handle *cras_hfp_ag_get_active_handle();

/* Gets the SLC handle for given cras_bt_device. */
struct hfp_slc_handle *cras_hfp_ag_get_slc(struct cras_bt_device *device);

#endif /* CRAS_HFP_AG_PROFILE_H_ */
