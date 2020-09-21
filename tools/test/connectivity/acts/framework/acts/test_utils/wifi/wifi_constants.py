#!/usr/bin/env python3.4
#
#   Copyright 2016 - Google
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

# Constants for Wifi related events.
WIFI_CONNECTED = "WifiNetworkConnected"
WIFI_DISCONNECTED = "WifiNetworkDisconnected"
SUPPLICANT_CON_CHANGED = "SupplicantConnectionChanged"
WIFI_STATE_CHANGED = "WifiStateChanged"
WIFI_FORGET_NW_SUCCESS = "WifiManagerForgetNetworkOnSuccess"

# These constants will be used by the ACTS wifi tests.
CONNECT_BY_CONFIG_SUCCESS = 'WifiManagerConnectByConfigOnSuccess'
CONNECT_BY_NETID_SUCCESS = 'WifiManagerConnectByNetIdOnSuccess'

# AP related constants
AP_MAIN = "main_AP"
AP_AUX = "aux_AP"
SSID = "SSID"
