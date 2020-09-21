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

import time
from acts.test_decorators import test_tracker_info
from acts.test_utils.power import PowerWiFiBaseTest as PWBT
from acts.test_utils.wifi import wifi_power_test_utils as wputils


class PowerWiFidtimTest(PWBT.PowerWiFiBaseTest):
    def dtim_test_func(self, dtim_max=10):
        """A reusable function for DTIM test.
        Covering different DTIM value, with screen ON or OFF and 2g/5g network

        Args:
            dtim: the value for DTIM set on the phone
            screen_status: screen on or off
            network: a dict of information for the network to connect
        """
        attrs = ['screen_status', 'wifi_band', 'dtim']
        indices = [2, 4, 6]
        self.decode_test_configs(attrs, indices)
        # Initialize the dut to rock-bottom state
        rebooted = wputils.change_dtim(
            self.dut,
            gEnableModulatedDTIM=int(self.test_configs.dtim),
            gMaxLIModulatedDTIM=dtim_max)
        if rebooted:
            self.dut_rockbottom()
        self.dut.log.info('DTIM value of the phone is now {}'.format(
            self.test_configs.dtim))
        self.setup_ap_connection(
            self.main_network[self.test_configs.wifi_band])
        if self.test_configs.screen_status == 'OFF':
            self.dut.droid.goToSleepNow()
            self.dut.log.info('Screen is OFF')
        time.sleep(5)
        self.measure_power_and_validate()

    # Test cases
    @test_tracker_info(uuid='2a70a78b-93a8-46a6-a829-e1624b8239d2')
    def test_screen_OFF_band_2g_dtim_1(self):
        self.dtim_test_func()

    @test_tracker_info(uuid='b6c4114d-984a-4269-9e77-2bec0e4b6e6f')
    def test_screen_OFF_band_2g_dtim_2(self):
        self.dtim_test_func()

    @test_tracker_info(uuid='2ae5bc29-3d5f-4fbb-9ff6-f5bd499a9d6e')
    def test_screen_OFF_band_2g_dtim_4(self):
        self.dtim_test_func()

    @test_tracker_info(uuid='b37fa75f-6166-4247-b15c-adcda8c7038e')
    def test_screen_OFF_band_2g_dtim_5(self):
        self.dtim_test_func()

    @test_tracker_info(uuid='384d3b0f-4335-4b00-8363-308ec27a150c')
    def test_screen_ON_band_2g_dtim_1(self):
        self.dtim_test_func()

    @test_tracker_info(uuid='79d0f065-2c46-4400-b02c-5ad60e79afea')
    def test_screen_ON_band_2g_dtim_4(self):
        self.dtim_test_func()

    @test_tracker_info(uuid='5e2f73cb-7e4e-4a25-8fd5-c85adfdf466e')
    def test_screen_OFF_band_5g_dtim_1(self):
        self.dtim_test_func()

    @test_tracker_info(uuid='017f57c3-e133-461d-80be-d025d1491d8a')
    def test_screen_OFF_band_5g_dtim_2(self):
        self.dtim_test_func()

    @test_tracker_info(uuid='b84a1cb3-9573-4bfd-9875-0f33cb171cc5')
    def test_screen_OFF_band_5g_dtim_4(self):
        self.dtim_test_func()

    @test_tracker_info(uuid='75644df4-2cc8-4bbd-8985-0656a4f9d056')
    def test_screen_OFF_band_5g_dtim_5(self):
        self.dtim_test_func()

    @test_tracker_info(uuid='327af44d-d9e7-49e0-9bda-accad6241dc7')
    def test_screen_ON_band_5g_dtim_1(self):
        self.dtim_test_func()

    @test_tracker_info(uuid='8b32585f-2517-426b-a2c9-8087093cf991')
    def test_screen_ON_band_5g_dtim_4(self):
        self.dtim_test_func()
