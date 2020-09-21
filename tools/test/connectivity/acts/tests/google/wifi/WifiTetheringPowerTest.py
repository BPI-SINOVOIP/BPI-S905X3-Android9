#!/usr/bin/env python3.4
#
#   Copyright 2017 - The Android Open Source Project
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

import os
import threading
import time

from acts import base_test
from acts import asserts
from acts.controllers import adb
from acts.controllers import monsoon
from acts.test_decorators import test_tracker_info
from acts.test_utils.wifi import wifi_test_utils as wutils
from acts.test_utils.tel import tel_data_utils as tel_utils
from acts.test_utils.tel.tel_test_utils import WIFI_CONFIG_APBAND_2G
from acts.test_utils.tel.tel_test_utils import WIFI_CONFIG_APBAND_5G
from acts.test_utils.tel.tel_test_utils import http_file_download_by_chrome
from acts.utils import force_airplane_mode
from acts.utils import set_adaptive_brightness
from acts.utils import set_ambient_display
from acts.utils import set_auto_rotate
from acts.utils import set_location_service


class WifiTetheringPowerTest(base_test.BaseTestClass):

    def setup_class(self):
        self.hotspot_device = self.android_devices[0]
        self.tethered_devices = self.android_devices[1:]
        req_params = ("ssid", "password", "url")
        self.unpack_userparams(req_params)
        self.network = { "SSID": self.ssid, "password": self.password }

        self.offset = 1 * 60
        self.hz = 5000
        self.duration = 9 * 60 + self.offset
        self.mon_data_path = os.path.join(self.log_path, "Monsoon")
        self.mon = self.monsoons[0]
        self.mon.set_voltage(4.2)
        self.mon.set_max_current(7.8)
        self.mon.attach_device(self.hotspot_device)

        asserts.assert_true(self.mon.usb("auto"),
            "Failed to turn USB mode to auto on monsoon.")
        set_location_service(self.hotspot_device, False)
        set_adaptive_brightness(self.hotspot_device, False)
        set_ambient_display(self.hotspot_device, False)
        self.hotspot_device.adb.shell("settings put system screen_brightness 0")
        set_auto_rotate(self.hotspot_device, False)
        wutils.wifi_toggle_state(self.hotspot_device, False)
        self.hotspot_device.droid.telephonyToggleDataConnection(True)
        tel_utils.wait_for_cell_data_connection(self.log, self.hotspot_device, True)
        asserts.assert_true(
            tel_utils.verify_http_connection(self.log, self.hotspot_device),
            "HTTP verification failed on cell data connection")
        for ad in self.tethered_devices:
            wutils.reset_wifi(ad)

    def teardown_class(self):
        self.mon.usb("on")
        wutils.wifi_toggle_state(self.hotspot_device, True)

    def on_fail(self, test_name, begin_time):
        self.hotspot_device.take_bug_report(test_name, begin_time)

    def on_pass(self, test_name, begin_time):
        self.hotspot_device.take_bug_report(test_name, begin_time)

    """ Helper functions """
    def _measure_and_process_result(self):
        """ Measure the current drawn by the device for the period of
            self.duration, at the frequency of self.hz.
        """
        tag = self.current_test_name
        result = self.mon.measure_power(self.hz,
                                        self.duration,
                                        tag=tag,
                                        offset=self.offset)
        asserts.assert_true(result,
                            "Got empty measurement data set in %s." % tag)
        self.log.info(repr(result))
        data_path = os.path.join(self.mon_data_path, "%s.txt" % tag)
        monsoon.MonsoonData.save_to_text_file([result], data_path)
        actual_current = result.average_current
        actual_current_str = "%.2fmA" % actual_current
        result_extra = {"Average Current": actual_current_str}

    def _start_wifi_tethering(self, wifi_band):
        """ Start wifi tethering on hotspot device

            Args:
              1. wifi_band: specifies the wifi band to start the hotspot
              on. The current options are 2G and 5G
        """
        wutils.start_wifi_tethering(self.hotspot_device,
                                    self.ssid,
                                    self.password,
                                    wifi_band)

    def _start_traffic_on_device(self, ad):
        """ Start traffic on the device by downloading data
            Run the traffic continuosly for self.duration

            Args:
              1. ad device object
        """
        timeout = time.time() + self.duration
        while True:
            if time.time() > timeout:
                break
            http_file_download_by_chrome(ad, self.url)

    def _start_traffic_measure_power(self, ad_list):
        """ Start traffic on the tethered devices and measure power

            Args:
              1. ad_list: list of tethered devices to run traffic on
        """
        threads = []
        for ad in ad_list:
            t = threading.Thread(target = self._start_traffic_on_device,
                                 args = (ad,))
            t.start()
            threads.append(t)
        try:
            self._measure_and_process_result()
        finally:
            for t in threads:
                t.join()


    """ Tests begin """
    @test_tracker_info(uuid="ebb74144-e22a-46e1-b8c1-9ada22b13133")
    def test_power_wifi_tethering_2ghz_no_devices_connected(self):
        """ Steps:
              1. Start wifi hotspot with 2.4Ghz band
              2. No devices connected to hotspot
              3. Measure power
        """
        self._start_wifi_tethering(WIFI_CONFIG_APBAND_2G)
        self._measure_and_process_result()
        wutils.stop_wifi_tethering(self.hotspot_device)

    @test_tracker_info(uuid="2560c088-4010-4354-ade3-6aaac83b1cfd")
    def test_power_wifi_tethering_5ghz_no_devices_connected(self):
        """ Steps:
              1. Start wifi hotspot with 5Ghz band
              2. No devices connected to hotspot
              3. Measure power
        """
        self._start_wifi_tethering(WIFI_CONFIG_APBAND_5G)
        self._measure_and_process_result()
        wutils.stop_wifi_tethering(self.hotspot_device)

    @test_tracker_info(uuid="644795b0-cd30-4a8f-82ee-cc0618c41c6b")
    def test_power_wifi_tethering_2ghz_connect_1device(self):
        """ Steps:
              1. Start wifi hotspot with 2.4GHz band
              2. Connect 1 device to hotspot
              3. Measure power
        """
        self._start_wifi_tethering(WIFI_CONFIG_APBAND_2G)
        wutils.wifi_connect(self.tethered_devices[0], self.network)
        self._measure_and_process_result()
        wutils.stop_wifi_tethering(self.hotspot_device)

    @test_tracker_info(uuid="8fca9898-f493-44c3-810f-d2262ac72187")
    def test_power_wifi_tethering_5ghz_connect_1device(self):
        """ Steps:
              1. Start wifi hotspot with 5GHz band
              2. Connect 1 device to hotspot
              3. Measure power
        """
        self._start_wifi_tethering(WIFI_CONFIG_APBAND_5G)
        wutils.wifi_connect(self.tethered_devices[0], self.network)
        self._measure_and_process_result()
        wutils.stop_wifi_tethering(self.hotspot_device)

    @test_tracker_info(uuid="16ef5f63-1a7a-44ae-bf8d-c3a181c89b63")
    def test_power_wifi_tethering_2ghz_connect_5devices(self):
        """ Steps:
              1. Start wifi hotspot with 2GHz band
              2. Connect 5 devices to hotspot
              3. Measure power
        """
        self._start_wifi_tethering(WIFI_CONFIG_APBAND_2G)
        for ad in self.tethered_devices:
            wutils.wifi_connect(ad, self.network)
        self._measure_and_process_result()
        wutils.stop_wifi_tethering(self.hotspot_device)

    @test_tracker_info(uuid="769aedfc-d309-40e0-95dd-51ff40f4e097")
    def test_power_wifi_tethering_5ghz_connect_5devices(self):
        """ Steps:
              1. Start wifi hotspot with 5GHz band
              2. Connect 5 devices to hotspot
              3. Measure power
        """
        self._start_wifi_tethering(WIFI_CONFIG_APBAND_5G)
        for ad in self.tethered_devices:
            wutils.wifi_connect(ad, self.network)
        self._measure_and_process_result()
        wutils.stop_wifi_tethering(self.hotspot_device)

    @test_tracker_info(uuid="e5b71f34-1dc0-4045-a45e-48c1e9426ec3")
    def test_power_wifi_tethering_2ghz_connect_1device_with_traffic(self):
        """ Steps:
              1. Start wifi hotspot with 2GHz band
              2. Connect 1 device to hotspot device
              3. Start traffic and measure power
        """
        self._start_wifi_tethering(WIFI_CONFIG_APBAND_2G)
        wutils.wifi_connect(self.tethered_devices[0], self.network)
        self._start_traffic_measure_power(self.tethered_devices[0:1])
        wutils.stop_wifi_tethering(self.hotspot_device)

    @test_tracker_info(uuid="29c5cd6e-8df1-46e5-a735-526dc9154f6e")
    def test_power_wifi_tethering_5ghz_connect_1device_with_traffic(self):
        """ Steps:
              1. Start wifi hotspot with 5GHz band
              2. Connect 1 device to hotspot device
              3. Start traffic and measure power
        """
        self._start_wifi_tethering(WIFI_CONFIG_APBAND_5G)
        wutils.wifi_connect(self.tethered_devices[0], self.network)
        self._start_traffic_measure_power(self.tethered_devices[0:1])
        wutils.stop_wifi_tethering(self.hotspot_device)

    @test_tracker_info(uuid="da71b06f-7b98-4c14-a2e2-361f395b39a8")
    def test_power_wifi_tethering_2ghz_connect_5devices_with_traffic(self):
        """ Steps:
              1. Start wifi hotspot with 2GHz band
              2. Connect 5 devices to hotspot device
              3. Start traffic and measure power
        """
        self._start_wifi_tethering(WIFI_CONFIG_APBAND_2G)
        for ad in self.tethered_devices:
            wutils.wifi_connect(ad, self.network)
        self._start_traffic_measure_power(self.tethered_devices)
        wutils.stop_wifi_tethering(self.hotspot_device)

    @test_tracker_info(uuid="7f3173ab-fd8f-4579-8c45-f9a8c5cd17f7")
    def test_power_wifi_tethering_5ghz_connect_5devices_with_traffic(self):
        """ Steps:
              1. Start wifi hotspot with 2GHz band
              2. Connect 5 devices to hotspot device
              3. Start traffic and measure power
        """
        self._start_wifi_tethering(WIFI_CONFIG_APBAND_5G)
        for ad in self.tethered_devices:
            wutils.wifi_connect(ad, self.network)
        self._start_traffic_measure_power(self.tethered_devices)
        wutils.stop_wifi_tethering(self.hotspot_device)
