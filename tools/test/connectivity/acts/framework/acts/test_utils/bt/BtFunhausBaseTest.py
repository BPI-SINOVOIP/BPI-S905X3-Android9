# /usr/bin/env python3.4
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
Test script to automate the Bluetooth Audio Funhaus.
"""
from acts.keys import Config
from acts.test_utils.bt.BtMetricsBaseTest import BtMetricsBaseTest
from acts.test_utils.bt.bt_test_utils import bluetooth_enabled_check
from acts.utils import bypass_setup_wizard
from acts.utils import create_dir
from acts.utils import exe_cmd
from acts.utils import sync_device_time
import json
import time
import os

BT_CONF_PATH = "/data/misc/bluedroid/bt_config.conf"


class BtFunhausBaseTest(BtMetricsBaseTest):
    """
    Base class for Bluetooth A2DP audio tests, this class is in charge of
    pushing link key to Android device so that it could be paired with remote
    A2DP device, pushing music to Android device, playing audio, monitoring
    audio play, and stop playing audio
    """
    music_file_to_play = ""
    device_fails_to_connect_list = []

    def __init__(self, controllers):
        BtMetricsBaseTest.__init__(self, controllers)
        self.ad = self.android_devices[0]
        self.dongle = self.relay_devices[0]

    def _pair_devices(self):
        self.ad.droid.bluetoothStartPairingHelper(False)
        self.dongle.enter_pairing_mode()

        self.ad.droid.bluetoothBond(self.dongle.mac_address)

        end_time = time.time() + 20
        self.ad.log.info("Verifying devices are bonded")
        while time.time() < end_time:
            bonded_devices = self.ad.droid.bluetoothGetBondedDevices()

            for d in bonded_devices:
                if d['address'] == self.dongle.mac_address:
                    self.ad.log.info("Successfully bonded to device.")
                    self.log.info("Bonded devices:\n{}".format(bonded_devices))
                return True
        self.ad.log.info("Failed to bond devices.")
        return False

    def setup_test(self):
        super(BtFunhausBaseTest, self).setup_test()
        self.dongle.setup()
        tries = 5
        # Since we are not concerned with pairing in this test, try 5 times.
        while tries > 0:
            if self._pair_devices():
                return True
            else:
                tries -= 1
        return False

    def teardown_test(self):
        super(BtFunhausBaseTest, self).teardown_test()
        self.dongle.clean_up()
        return True

    def on_fail(self, test_name, begin_time):
        self.dongle.clean_up()
        self._collect_bluetooth_manager_dumpsys_logs(self.android_devices)
        super(BtFunhausBaseTest, self).on_fail(test_name, begin_time)

    def setup_class(self):
        if not super(BtFunhausBaseTest, self).setup_class():
            return False
        for ad in self.android_devices:
            sync_device_time(ad)
            # Disable Bluetooth HCI Snoop Logs for audio tests
            ad.adb.shell("setprop persist.bluetooth.btsnoopenable false")
            if not bypass_setup_wizard(ad):
                self.log.debug(
                    "Failed to bypass setup wizard, continuing test.")
                # Add music to the Android device
        return self._add_music_to_android_device(ad)

    def _add_music_to_android_device(self, ad):
        """
        Add music to Android device as specified by the test config
        :param ad: Android device
        :return: True on success, False on failure
        """
        self.log.info("Pushing music to the Android device.")
        music_path_str = "bt_music"
        android_music_path = "/sdcard/Music/"
        if music_path_str not in self.user_params:
            self.log.error("Need music for audio testcases...")
            return False
        music_path = self.user_params[music_path_str]
        if type(music_path) is list:
            self.log.info("Media ready to push as is.")
        elif not os.path.isdir(music_path):
            music_path = os.path.join(self.user_params[Config.key_config_path],
                                      music_path)
            if not os.path.isdir(music_path):
                self.log.error(
                    "Unable to find music directory {}.".format(music_path))
                return False
        if type(music_path) is list:
            for item in music_path:
                self.music_file_to_play = item
                ad.adb.push("{} {}".format(item, android_music_path))
        else:
            for dirname, dirnames, filenames in os.walk(music_path):
                for filename in filenames:
                    self.music_file_to_play = filename
                    file = os.path.join(dirname, filename)
                    # TODO: Handle file paths with spaces
                    ad.adb.push("{} {}".format(file, android_music_path))
        ad.reboot()
        return True

    def _collect_bluetooth_manager_dumpsys_logs(self, ads):
        """
        Collect "adb shell dumpsys bluetooth_manager" logs
        :param ads: list of active Android devices
        :return: None
        """
        for ad in ads:
            serial = ad.serial
            out_name = "{}_{}".format(serial, "bluetooth_dumpsys.txt")
            dumpsys_path = ''.join((ad.log_path, "/BluetoothDumpsys"))
            create_dir(dumpsys_path)
            cmd = ''.join(
                ("adb -s ", serial, " shell dumpsys bluetooth_manager > ",
                 dumpsys_path, "/", out_name))
            exe_cmd(cmd)

    def start_playing_music_on_all_devices(self):
        """
        Start playing music
        :return: None
        """
        self.ad.droid.mediaPlayOpen("file:///sdcard/Music/{}".format(
            self.music_file_to_play.split("/")[-1]))
        self.ad.droid.mediaPlaySetLooping(True)
        self.ad.log.info("Music is now playing.")

    def monitor_music_play_util_deadline(self, end_time, sleep_interval=1):
        """
        Monitor music play on all devices, if a device's Bluetooth adapter is
        OFF or if a device is not connected to any remote Bluetooth devices,
        we add them to failure lists bluetooth_off_list and
        device_not_connected_list respectively
        :param end_time: The deadline in epoch floating point seconds that we
            must stop playing
        :param sleep_interval: How often to monitor, too small we may drain
            too much resources on Android, too big the deadline might be passed
            by a maximum of this amount
        :return:
            status: False iff all devices are off or disconnected otherwise True
            bluetooth_off_list: List of ADs that have Bluetooth at OFF state
            device_not_connected_list: List of ADs with no remote device
                                        connected
        """
        device_not_connected_list = []
        while time.time() < end_time:
            if not self.ad.droid.bluetoothCheckState():
                self.ad.log.error("Device {}'s Bluetooth state is off.".format(
                    self.ad.serial))
                return False
            if self.ad.droid.bluetoothGetConnectedDevices() == 0:
                self.ad.log.error(
                    "Bluetooth device not connected. Failing test.")
            time.sleep(sleep_interval)
        return True

    def play_music_for_duration(self, duration, sleep_interval=1):
        """
        A convenience method for above methods. It starts run music on all
        devices, monitors the health of music play and stops playing them when
        time passes the duration
        :param duration: Duration in floating point seconds
        :param sleep_interval: How often to check the health of music play
        :return:
            status: False iff all devices are off or disconnected otherwise True
            bluetooth_off_list: List of ADs that have Bluetooth at OFF state
            device_not_connected_list: List of ADs with no remote device
                                        connected
        """
        start_time = time.time()
        end_time = start_time + duration
        self.start_playing_music_on_all_devices()
        status = self.monitor_music_play_util_deadline(end_time,
                                                       sleep_interval)
        self.ad.droid.mediaPlayStopAll()
        return status
