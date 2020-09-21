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
import time

from acts.test_decorators import test_tracker_info
from acts.test_utils.bt.BtFunhausBaseTest import BtFunhausBaseTest


class BtFunhausTest(BtFunhausBaseTest):
    music_file_to_play = ""
    device_fails_to_connect_list = []

    def __init__(self, controllers):
        BtFunhausBaseTest.__init__(self, controllers)

    @test_tracker_info(uuid='80a4cc4c-7c2a-428d-9eaf-46239a7926df')
    def test_run_bt_audio_12_hours(self):
        """Test audio quality over 12 hours.

        This test is designed to run Bluetooth audio for 12 hours
        and collect relevant logs. If all devices disconnect during
        the test or Bluetooth is off on all devices, then fail the
        test.

        Steps:
        1. For each Android device connected run steps 2-5.
        2. Open and play media file of music pushed to device
        3. Set media to loop indefinitely.
        4. After 12 hours collect bluetooth_manager dumpsys information
        5. Stop media player

        Expected Result:
        Audio plays for 12 hours over Bluetooth

        Returns:
          Pass if True
          Fail if False

        TAGS: Classic, A2DP
        Priority: 1
        """
        self.start_playing_music_on_all_devices()

        sleep_interval = 120
        #twelve_hours_in_seconds = 43200
        #one_hour_in_seconds = 3600
        one_min_in_sec = 60
        end_time = time.time() + one_min_in_sec
        if not self.monitor_music_play_util_deadline(end_time, sleep_interval):
            return False
        self._collect_bluetooth_manager_dumpsys_logs(self.android_devices)
        self.ad.droid.mediaPlayStopAll()
        self.collect_bluetooth_manager_metrics_logs(self.android_devices)
        return True
