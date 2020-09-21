/* Copyright (c) 2013 The Chromium Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef CRAS_BT_CONSTANTS_H_
#define CRAS_BT_CONSTANTS_H_

#define BLUEZ_SERVICE "org.bluez"

#define BLUEZ_INTERFACE_ADAPTER "org.bluez.Adapter1"
#define BLUEZ_INTERFACE_DEVICE "org.bluez.Device1"
#define BLUEZ_INTERFACE_MEDIA "org.bluez.Media1"
#define BLUEZ_INTERFACE_MEDIA_ENDPOINT "org.bluez.MediaEndpoint1"
#define BLUEZ_INTERFACE_MEDIA_TRANSPORT "org.bluez.MediaTransport1"
#define BLUEZ_INTERFACE_PLAYER "org.bluez.MediaPlayer1"
#define BLUEZ_INTERFACE_PROFILE "org.bluez.Profile1"
#define BLUEZ_PROFILE_MGMT_INTERFACE "org.bluez.ProfileManager1"
/* Remove once our D-Bus header files are updated to define this. */
#ifndef DBUS_INTERFACE_OBJECT_MANAGER
#define DBUS_INTERFACE_OBJECT_MANAGER "org.freedesktop.DBus.ObjectManager"
#endif

/* UUIDs taken from lib/uuid.h in the BlueZ source */
#define HSP_HS_UUID             "00001108-0000-1000-8000-00805f9b34fb"
#define HSP_AG_UUID             "00001112-0000-1000-8000-00805f9b34fb"

#define HFP_HF_UUID             "0000111e-0000-1000-8000-00805f9b34fb"
#define HFP_AG_UUID             "0000111f-0000-1000-8000-00805f9b34fb"

#define A2DP_SOURCE_UUID        "0000110a-0000-1000-8000-00805f9b34fb"
#define A2DP_SINK_UUID          "0000110b-0000-1000-8000-00805f9b34fb"

#define AVRCP_REMOTE_UUID       "0000110e-0000-1000-8000-00805f9b34fb"
#define AVRCP_TARGET_UUID       "0000110c-0000-1000-8000-00805f9b34fb"

#define GENERIC_AUDIO_UUID	"00001203-0000-1000-8000-00805f9b34fb"

#endif /* CRAS_BT_CONSTANTS_H_ */
