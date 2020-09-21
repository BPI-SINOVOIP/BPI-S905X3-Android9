#/usr/bin/env python3.4
#
# Copyright (C) 2016 The Android Open Source Project
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
"""
Bluetooth Config Pusher
"""

from acts.test_utils.bt.bt_gatt_utils import disconnect_gatt_connection
from acts.test_utils.bt.bt_gatt_utils import setup_gatt_connection
from acts.test_utils.bt.bt_gatt_utils import setup_gatt_mtu
from acts.test_utils.bt.bt_gatt_utils import log_gatt_server_uuids

import time
import os


class ConfigLib():
    bluetooth_config_path = "/system/etc/bluetooth/bt_stack.conf"
    conf_path = "{}/configs".format(
        os.path.dirname(os.path.realpath(__file__)))
    reset_config_path = "{}/bt_stack.conf".format(conf_path)
    non_bond_config_path = "{}/non_bond_bt_stack.conf".format(conf_path)
    disable_mitm_config_path = "{}/dis_mitm_bt_stack.conf".format(conf_path)

    def __init__(self, log, dut):
        self.dut = dut
        self.log = log

    def _reset_bluetooth(self):
        self.dut.droid.bluetoothToggleState(False)
        self.dut.droid.bluetoothToggleState(True)

    def reset(self):
        self.dut.adb.push("{} {}".format(self.reset_config_path,
                                         self.bluetooth_config_path))
        self._reset_bluetooth()

    def set_nonbond(self):
        self.dut.adb.push("{} {}".format(self.non_bond_config_path,
                                         self.bluetooth_config_path))
        self._reset_bluetooth()

    def set_disable_mitm(self):
        self.dut.adb.push("{} {}".format(self.disable_mitm_config_path,
                                         self.bluetooth_config_path))
        self._reset_bluetooth()
