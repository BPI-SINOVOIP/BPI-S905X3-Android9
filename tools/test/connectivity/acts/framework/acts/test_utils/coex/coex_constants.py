# /usr/bin/env python3.4
#
# Copyright (C) 2018 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License"); you may not
# use this file except in compliance with the License. You may obtain a copy of
# the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
# License for the specific language governing permissions and limitations under
# the License.

AUDIO_ROUTE_SPEAKER = "SPEAKER"
AUDIO_ROUTE_BLUETOOTH = "BLUETOOTH"

CALL_WAIT_TIME = 10
DISCOVERY_TIME = 13
WAIT_TIME = 3

OBJECT_MANGER = "org.freedesktop.DBus.ObjectManager"
PROPERTIES = "org.freedesktop.DBus.Properties"
PROPERTIES_CHANGED = "PropertiesChanged"
SERVICE_NAME = "org.bluez"
CALL_MANAGER = "org.ofono.VoiceCallManager"
VOICE_CALL = "org.ofono.VoiceCall"
OFONO_MANAGER = "org.ofono.Manager"

ADAPTER_INTERFACE = SERVICE_NAME + ".Adapter1"
DBUS_INTERFACE = "org.freedesktop.DBus.Properties"
DEVICE_INTERFACE = SERVICE_NAME + ".Device1"
MEDIA_CONTROL_INTERACE = SERVICE_NAME +".MediaControl1"
MEDIA_PLAY_INTERFACE = SERVICE_NAME + ".MediaPlayer1"

bluetooth_profiles = {
    "A2DP_SRC": "0000110a-0000-1000-8000-00805f9b34fb",
    "HFP_AG": "0000111f-0000-1000-8000-00805f9b34fb"
}
