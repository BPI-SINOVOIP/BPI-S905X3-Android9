# Copyright (C) 2017 The Android Open Source Project
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
import time

from acts.test_decorators import test_tracker_info
from acts import asserts
from acts.test_utils.bt.BtFunhausBaseTest import BtFunhausBaseTest


class BtFunhausMetricsTest(BtFunhausBaseTest):
    def __init__(self, controllers):
        BtFunhausBaseTest.__init__(self, controllers)
        self.bluetooth_proto_path = None
        self.metrics_path = None
        self.bluetooth_proto_module = None

    def setup_class(self):
        return super(BtFunhausMetricsTest, self).setup_class()

    def setup_test(self):
        return super(BtFunhausMetricsTest, self).setup_test()

    @test_tracker_info(uuid='b712ed0e-c1fb-4bc8-9dee-83891aa22205')
    def test_run_bt_audio(self):
        """Test run Bluetooth A2DP audio for one iteration

        This test runs Bluetooth A2DP Audio for 60 seconds and sleep for 20
        seconds.

        Steps:
        1. For the first Android device, run audio for 60 seconds
        2. Sleep while connected to A2DP sink for 20 seconds
        3. Pull Bluetooth metrics
        4. Verify metrics values

        Expected Result:
        The correct metrics should have one Bluetooth session with
            audio_duration_millis = 60000 +/- 10000
            session_duration_sec = 80 +/- 10

        Returns:
          Pass if True
          Fail if False

        TAGS: Classic, A2DP
        Priority: 1
        """
        play_duration_seconds = 60
        start_time = time.time()
        if not self.play_music_for_duration(play_duration_seconds):
            return False
        self.ad.droid.mediaPlayStopAll()
        time.sleep(20)
        bt_duration = time.time() - start_time
        bluetooth_logs, bluetooth_logs_ascii = \
            self.collect_bluetooth_manager_metrics_logs(
                [self.ad])
        bluetooth_log = bluetooth_logs[0]
        bluetooth_log_ascii = bluetooth_logs_ascii[0]
        self.log.info(bluetooth_log_ascii)
        asserts.assert_equal(len(bluetooth_log.session), 1)
        a2dp_session_log = bluetooth_log.session[0]
        asserts.assert_almost_equal(
            a2dp_session_log.session_duration_sec, bt_duration, delta=10)
        asserts.assert_almost_equal(
            a2dp_session_log.a2dp_session.audio_duration_millis,
            play_duration_seconds * 1000,
            delta=10000)
        return True

    @test_tracker_info(uuid='ab6b8c61-057b-4bf6-b0cf-8bec3ae3a7eb')
    def test_run_multiple_bt_audio(self):
        """Test metrics for multiple Bluetooth audio sessions

        This test will run Bluetooth A2DP audio for five 30 seconds sessions
        and collect metrics after that

        Steps:
        1. For the first Android device connected, run A2DP audio for 30
        seconds for 5 times
        2. Dump and compare metrics

        Expected Result:
        There should be a single Bluetooth session with
            session_duration_sec = 250 +/- 10
            audio_duration_millis = 150000 +/- 20000

        Note: The discrepancies are mainly due to command delays

        Returns:
          Pass if True
          Fail if False

        TAGS: Classic, A2DP
        Priority: 1
        """
        num_play = 5
        play_duration_seconds = 30
        bt_duration = 0
        a2dp_duration = 0
        for i in range(num_play):
            start_time = time.time()
            if not self.play_music_for_duration(play_duration_seconds):
                return False
            a2dp_duration += (time.time() - start_time)
            time.sleep(20)
            bt_duration += (time.time() - start_time)
        bluetooth_logs, bluetooth_logs_ascii = \
            self.collect_bluetooth_manager_metrics_logs(
                [self.android_devices[0]])
        bluetooth_log = bluetooth_logs[0]
        bluetooth_log_ascii = bluetooth_logs_ascii[0]
        self.log.info(bluetooth_log_ascii)
        asserts.assert_equal(len(bluetooth_log.session), 1)
        a2dp_session_log = bluetooth_log.session[0]
        asserts.assert_almost_equal(
            a2dp_session_log.session_duration_sec, bt_duration, delta=10)
        asserts.assert_almost_equal(
            a2dp_session_log.a2dp_session.audio_duration_millis,
            a2dp_duration * 1000,
            delta=20000)
        return True

    def test_run_multiple_bt_audio_dump_each(self):
        """Test run Bluetooth A2DP audio multiple times and dump metrics each time

        Steps:
        1. For the first Android device connected, run A2DP audio for 30 seconds
        2. Sleep for 20 seconds
        3. Dump metrics and compare
        4. Repeate steps 1-3 five times

        Expected Result:
        Each time, we should observe the following:
            session_duration_sec = 50 +/- 10
            audio_duration_millis = 30 +/- 5

        Returns:
          Pass if True
          Fail if False

        TAGS: Classic, A2DP
        Priority: 1
        """
        num_play = 5
        play_duration_seconds = 30
        for i in range(num_play):
            start_time = time.time()
            if not self.play_music_for_duration(play_duration_seconds):
                return False
            time.sleep(20)
            bt_duration = time.time() - start_time
            bluetooth_logs, bluetooth_logs_ascii = \
                self.collect_bluetooth_manager_metrics_logs(
                    [self.android_devices[0]])
            bluetooth_log = bluetooth_logs[0]
            bluetooth_log_ascii = bluetooth_logs_ascii[0]
            self.log.info(bluetooth_log_ascii)
            asserts.assert_equal(len(bluetooth_log.session), 1)
            a2dp_session_log = bluetooth_log.session[0]
            asserts.assert_almost_equal(
                a2dp_session_log.session_duration_sec, bt_duration, delta=10)
            asserts.assert_almost_equal(
                a2dp_session_log.a2dp_session.audio_duration_millis,
                play_duration_seconds * 1000,
                delta=5000)
        return True
