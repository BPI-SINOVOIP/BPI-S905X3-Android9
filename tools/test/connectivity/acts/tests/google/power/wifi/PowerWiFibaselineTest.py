#!/usr/bin/env python3.4
#
#   Copyright 2018 - The Android Open Source Project
#
#   Licensed under the Apache License, Version 2.0 (the 'License');
#   you may not use this file except in compliance with the License.
#   You may obtain a copy of the License at
#
#       http://www.apache.org/licenses/LICENSE-2.0
#
#   Unless required by applicable law or agreed to in writing, software
#   distributed under the License is distributed on an 'AS IS' BASIS,
#   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#   See the License for the specific language governing permissions and
#   limitations under the License.

from acts.test_utils.power import PowerWiFiBaseTest as PWBT
from acts.test_utils.wifi import wifi_test_utils as wutils
from acts.test_decorators import test_tracker_info


class PowerWiFibaselineTest(PWBT.PowerWiFiBaseTest):
    """Power baseline tests for rockbottom state.
    Rockbottom for wifi on/off, screen on/off, everything else turned off

    """

    def rockbottom_test_func(self):
        """Test function for baseline rockbottom tests.

        Decode the test config from the test name, set device to desired state.
        Measure power and validate results.
        """

        attrs = ['screen_status', 'wifi_status']
        indices = [3, 5]
        self.decode_test_configs(attrs, indices)
        if self.test_configs.wifi_status == 'ON':
            wutils.wifi_toggle_state(self.dut, True)
        if self.test_configs.screen_status == 'OFF':
            self.dut.droid.goToSleepNow()
            self.dut.log.info('Screen is OFF')
        self.measure_power_and_validate()

    # Test cases
    @test_tracker_info(uuid='e7ab71f4-1e14-40d2-baec-cde19a3ac859')
    def test_rockbottom_screen_OFF_wifi_OFF(self):

        self.rockbottom_test_func()

    @test_tracker_info(uuid='167c847d-448f-4c7c-900f-82c552d7d9bb')
    def test_rockbottom_screen_OFF_wifi_ON(self):

        self.rockbottom_test_func()

    @test_tracker_info(uuid='2cd25820-8548-4e60-b0e3-63727b3c952c')
    def test_rockbottom_screen_ON_wifi_OFF(self):

        self.rockbottom_test_func()

    @test_tracker_info(uuid='d7d90a1b-231a-47c7-8181-23814c8ff9b6')
    def test_rockbottom_screen_ON_wifi_ON(self):

        self.rockbottom_test_func()
