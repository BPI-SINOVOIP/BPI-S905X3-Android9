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
Script for initializing a cmd line tool for PTS and other purposes.
Required custom config parameters:
'target_mac_address': '00:00:00:00:00:00'

Optional config parameters:
'sim_conf_file' : '/path_to_config/'
"""
from acts.test_utils.bt.BluetoothBaseTest import BluetoothBaseTest
from cmd_input import CmdInput
from queue import Empty

import os
import uuid

from acts.test_utils.tel.tel_test_utils import setup_droid_properties


class BtCmdLineTest(BluetoothBaseTest):
    target_mac_address = ""

    def __init__(self, controllers):
        BluetoothBaseTest.__init__(self, controllers)
        if not "target_mac_address" in self.user_params.keys():
            self.log.warning("Missing user config \"target_mac_address\"!")
            self.target_mac_address = ""
        else:
            self.target_mac_address = self.user_params[
                "target_mac_address"].upper()

        self.android_devices[0].droid.bluetoothSetLocalName("CMD LINE Test")
        if len(self.android_devices) > 1:
            #Setup for more complex testcases
            if not "sim_conf_file" in self.user_params.keys():
                self.log.error(
                    "Missing mandatory user config \"sim_conf_file\"!")
                return False
            sim_conf_file = self.user_params["sim_conf_file"][0]
            # If the sim_conf_file is not a full path, attempt to find it
            # relative to the config file.
            if not os.path.isfile(sim_conf_file):
                sim_conf_file = os.path.join(
                    self.user_params[Config.key_config_path], sim_conf_file)
                if not os.path.isfile(sim_conf_file):
                    log.error("Unable to load user config " + sim_conf_file +
                              " from test config file.")
                    return False
            for ad in self.android_devices:
                setup_droid_properties(self.log, ad, sim_conf_file)
            music_path_str = "music_path"
            android_music_path = "/sdcard/Music/"
            if music_path_str not in self.user_params:
                self.log.error("Need music for A2DP testcases")
                return False
            music_path = self.user_params[music_path_str]
            self._add_music_to_primary_android_device(music_path,
                                                      android_music_path)

    def _add_music_to_primary_android_device(self, music_path,
                                             android_music_path):
        music_list = []
        for dirname, dirnames, filenames in os.walk(music_path):
            for filename in filenames:
                file = os.path.join(dirname, filename)
                #TODO: Handle file paths with spaces
                self.android_devices[0].adb.push("{} {}".format(
                    file, android_music_path))

    def setup_class(self):
        return True

    def test_pts_cmd_line_helper(self):
        cmd_line = CmdInput()
        cmd_line.setup_vars(self.android_devices, self.target_mac_address,
                            self.log)
        cmd_line.cmdloop()
        return True
