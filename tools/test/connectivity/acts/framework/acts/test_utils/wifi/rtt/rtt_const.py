#!/usr/bin/env python3.4
#
#   Copyright 2017 - Google
#
#   Licensed under the Apache License, Version 2.0 (the "License");
#   you may not use this file except in compliance with the License.
#   You may obtain a copy of the License at
#
#       http://www.apache.org/licenses/LICENSE-2.0
#
#   Unless required by applicable law or agreed to in writing, software
#   distributed under the License is distributed on an "AS IS" BASIS,
#   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#   See the License for the specific language governing permissions and
#   limitations under the License.

######################################################
# Broadcast events
######################################################
BROADCAST_WIFI_RTT_AVAILABLE = "WifiRttAvailable"
BROADCAST_WIFI_RTT_NOT_AVAILABLE = "WifiRttNotAvailable"

######################################################
# RangingResultCallback events
######################################################
EVENT_CB_RANGING_ON_FAIL = "WifiRttRangingFailure"
EVENT_CB_RANGING_ON_RESULT = "WifiRttRangingResults"

EVENT_CB_RANGING_KEY_RESULTS = "Results"

EVENT_CB_RANGING_KEY_STATUS = "status"
EVENT_CB_RANGING_KEY_DISTANCE_MM = "distanceMm"
EVENT_CB_RANGING_KEY_DISTANCE_STD_DEV_MM = "distanceStdDevMm"
EVENT_CB_RANGING_KEY_RSSI = "rssi"
EVENT_CB_RANGING_KEY_NUM_ATTEMPTED_MEASUREMENTS = "numAttemptedMeasurements"
EVENT_CB_RANGING_KEY_NUM_SUCCESSFUL_MEASUREMENTS = "numSuccessfulMeasurements"
EVENT_CB_RANGING_KEY_LCI = "lci"
EVENT_CB_RANGING_KEY_LCR = "lcr"
EVENT_CB_RANGING_KEY_TIMESTAMP = "timestamp"
EVENT_CB_RANGING_KEY_MAC = "mac"
EVENT_CB_RANGING_KEY_PEER_ID = "peerId"
EVENT_CB_RANGING_KEY_MAC_AS_STRING = "macAsString"

EVENT_CB_RANGING_STATUS_SUCCESS = 0
EVENT_CB_RANGING_STATUS_FAIL = 1
EVENT_CB_RANGING_STATUS_RESPONDER_DOES_NOT_SUPPORT_IEEE80211MC = 2

######################################################
# status codes
######################################################

RANGING_FAIL_CODE_GENERIC = 1
RANGING_FAIL_CODE_RTT_NOT_AVAILABLE = 2

######################################################
# ScanResults keys
######################################################

SCAN_RESULT_KEY_RTT_RESPONDER = "is80211McRTTResponder"