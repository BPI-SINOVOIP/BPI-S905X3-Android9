#!/usr/bin/env python3.4
#
# Copyright 2018 Google, Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

import logging
import time
import pprint

from enum import IntEnum
from queue import Empty

from acts import asserts
from acts import signals
from acts import utils
from acts.controllers import attenuator
from acts.test_utils.wifi import wifi_constants
from acts.test_utils.tel import tel_defines
from acts.test_utils.wifi import wifi_test_utils as wutils

ARDUINO = "/root/arduino/arduino-1.8.5/arduino "
CONNECT_WIFI = "/arduino/connect_wifi/connect_wifi.ino"
DISCONNECT_WIFI = "/arduino/disconnect_wifi/disconnect_wifi.ino"
SSID = wutils.WifiEnums.SSID_KEY
PWD = wutils.WifiEnums.PWD_KEY

def connect_wifi(wd, network=None):
    """Connect wifi on arduino wifi dongle

    Args:
        wd - wifi dongle object
        network - wifi network to connect to
    """
    wd.log.info("Flashing connect_wifi.ino onto dongle")
    cmd = "locate %s" % CONNECT_WIFI
    file_path = utils.exe_cmd(cmd).decode("utf-8", "ignore").rstrip()
    write_status = wd.write(ARDUINO, file_path, network)
    asserts.assert_true(write_status, "Failed to flash connect wifi")
    wd.log.info("Flashing complete")
    wifi_status = wd.wifi_status()
    asserts.assert_true(wifi_status, "Failed to connect to %s" % network)
    ping_status = wd.ping_status()
    asserts.assert_true(ping_status, "Failed to connect to internet")

def disconnect_wifi(wd):
    """Disconnect wifi on arduino wifi dongle

    Args:
        wd - wifi dongle object

    Returns:
        True - if wifi is disconnected
        False - if not
    """
    wd.log.info("Flashing disconnect_wifi.ino onto dongle")
    cmd = "locate %s" % DISCONNECT_WIFI
    file_path = utils.exe_cmd(cmd).decode("utf-8", "ignore").rstrip()
    write_status = wd.write(ARDUINO, file_path)
    asserts.assert_true(write_status, "Failed to flash disconnect wifi")
    wd.log.info("Flashing complete")
    wifi_status = wd.wifi_status(False)
    asserts.assert_true(not wifi_status, "Failed to disconnect wifi")
