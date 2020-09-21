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
"""
    Test Script for Telephony Pre Flight check.
"""

import time
from queue import Empty

from acts import signals
from acts import utils
from acts.controllers.android_device import get_info
from acts.libs.ota import ota_updater
from acts.test_decorators import test_tracker_info
from acts.test_utils.tel.TelephonyBaseTest import TelephonyBaseTest
from acts.test_utils.tel.tel_defines import AOSP_PREFIX
from acts.test_utils.tel.tel_defines import CAPABILITY_PHONE
from acts.test_utils.tel.tel_defines import CAPABILITY_VOLTE
from acts.test_utils.tel.tel_defines import CAPABILITY_VT
from acts.test_utils.tel.tel_defines import CAPABILITY_WFC
from acts.test_utils.tel.tel_defines import CAPABILITY_MSIM
from acts.test_utils.tel.tel_defines import CAPABILITY_OMADM
from acts.test_utils.tel.tel_defines import INVALID_SUB_ID
from acts.test_utils.tel.tel_defines import MAX_WAIT_TIME_NW_SELECTION
from acts.test_utils.tel.tel_defines import PRECISE_CALL_STATE_LISTEN_LEVEL_BACKGROUND
from acts.test_utils.tel.tel_defines import PRECISE_CALL_STATE_LISTEN_LEVEL_FOREGROUND
from acts.test_utils.tel.tel_defines import PRECISE_CALL_STATE_LISTEN_LEVEL_RINGING
from acts.test_utils.tel.tel_defines import WAIT_TIME_AFTER_REBOOT
from acts.test_utils.tel.tel_lookup_tables import device_capabilities
from acts.test_utils.tel.tel_lookup_tables import operator_capabilities
from acts.test_utils.tel.tel_test_utils import abort_all_tests
from acts.test_utils.tel.tel_test_utils import ensure_phones_default_state
from acts.test_utils.tel.tel_test_utils import ensure_phone_subscription
from acts.test_utils.tel.tel_test_utils import ensure_wifi_connected
from acts.test_utils.tel.tel_test_utils import get_operator_name
from acts.test_utils.tel.tel_test_utils import is_sim_locked
from acts.test_utils.tel.tel_test_utils import run_multithread_func
from acts.test_utils.tel.tel_test_utils import setup_droid_properties
from acts.test_utils.tel.tel_test_utils import set_phone_screen_on
from acts.test_utils.tel.tel_test_utils import set_phone_silent_mode
from acts.test_utils.tel.tel_test_utils import toggle_airplane_mode_by_adb
from acts.test_utils.tel.tel_test_utils import unlock_sim
from acts.test_utils.tel.tel_test_utils import verify_http_connection
from acts.test_utils.tel.tel_test_utils import wait_for_voice_attach_for_subscription
from acts.test_utils.tel.tel_test_utils import wait_for_wifi_data_connection
from acts.test_utils.tel.tel_test_utils import wifi_toggle_state
from acts.test_utils.tel.tel_voice_utils import phone_setup_volte
from acts.asserts import abort_all
from acts.asserts import fail


class TelLivePreflightTest(TelephonyBaseTest):
    def __init__(self, controllers):
        TelephonyBaseTest.__init__(self, controllers)

        self.wifi_network_ssid = self.user_params.get(
            "wifi_network_ssid") or self.user_params.get(
                "wifi_network_ssid_2g") or self.user_params.get(
                    "wifi_network_ssid_5g")
        self.wifi_network_pass = self.user_params.get(
            "wifi_network_pass") or self.user_params.get(
                "wifi_network_pass_2g") or self.user_params.get(
                    "wifi_network_ssid_5g")

    def setup_class(self):
        for ad in self.android_devices:
            toggle_airplane_mode_by_adb(self.log, ad, False)

    def teardown_class(self):
        pass

    def setup_test(self):
        pass

    """ Tests Begin """

    @test_tracker_info(uuid="cb897221-99e1-4697-927e-02d92d969440")
    @TelephonyBaseTest.tel_test_wrap
    def test_ota_upgrade(self):
        ota_package = self.user_params.get("ota_package")
        if isinstance(ota_package, list):
            ota_package = ota_package[0]
        if ota_package and "dev/null" not in ota_package:
            self.log.info("Upgrade with ota_package %s", ota_package)
            self.log.info("Before OTA upgrade: %s",
                          get_info(self.android_devices))
        else:
            raise signals.TestSkip("No ota_package is defined")
        ota_util = self.user_params.get("ota_util")
        if isinstance(ota_util, list):
            ota_util = ota_util[0]
        if ota_util:
            if "update_engine_client.zip" in ota_util:
                self.user_params["UpdateDeviceOtaTool"] = ota_util
                self.user_params["ota_tool"] = "UpdateDeviceOtaTool"
            else:
                self.user_params["AdbSideloadOtaTool"] = ota_util
                self.user_params["ota_tool"] = "AdbSideloadOtaTool"
        self.log.info("OTA upgrade with %s by %s", ota_package,
                      self.user_params["ota_tool"])
        ota_updater.initialize(self.user_params, self.android_devices)
        tasks = [(ota_updater.update, [ad]) for ad in self.android_devices]
        try:
            run_multithread_func(self.log, tasks)
        except Exception as err:
            abort_all_tests(self.log, "Unable to do ota upgrade: %s" % err)
        device_info = get_info(self.android_devices)
        self.log.info("After OTA upgrade: %s", device_info)
        self.results.add_controller_info("AndroidDevice", device_info)
        for ad in self.android_devices:
            if is_sim_locked(ad):
                ad.log.info("After OTA, SIM keeps the locked state")
            elif getattr(ad, "is_sim_locked", False):
                ad.log.error("After OTA, SIM loses the locked state")
            if not unlock_sim(ad):
                abort_all_tests(ad.log, "unable to unlock SIM")
        return True

    @test_tracker_info(uuid="8390a2eb-a744-4cda-bade-f94a2cc83f02")
    @TelephonyBaseTest.tel_test_wrap
    def test_check_environment(self):
        ad = self.android_devices[0]
        # Check WiFi environment.
        # 1. Connect to WiFi.
        # 2. Check WiFi have Internet access.
        toggle_airplane_mode(self.log, ad, False, strict_checking=False)
        try:
            if not ensure_wifi_connected(self.log, ad, self.wifi_network_ssid,
                                         self.wifi_network_pass):
                abort_all_tests(ad.log, "WiFi connect fail")
            if (not wait_for_wifi_data_connection(self.log, ad, True) or
                    not verify_http_connection(self.log, ad)):
                abort_all_tests(ad.log, "Data not available on WiFi")
        finally:
            wifi_toggle_state(self.log, ad, False)
        # TODO: add more environment check here.
        return True

    @test_tracker_info(uuid="7bb23ac7-6b7b-4d5e-b8d6-9dd10032b9ad")
    @TelephonyBaseTest.tel_test_wrap
    def test_pre_flight_check(self):
        for ad in self.android_devices:
            #check for sim and service
            if not ensure_phone_subscription(self.log, ad):
                abort_all_tests(ad.log, "Unable to find a valid subscription!")
        return True

    @test_tracker_info(uuid="1070b160-902b-43bf-92a0-92cc2d05bb13")
    @TelephonyBaseTest.tel_test_wrap
    def test_check_crash(self):
        result = True
        begin_time = None
        for ad in self.android_devices:
            output = ad.adb.shell("cat /proc/uptime")
            epoch_up_time = utils.get_current_epoch_time() - 1000 * float(
                output.split(" ")[0])
            ad.crash_report_preflight = ad.check_crash_report(
                self.test_name,
                begin_time=epoch_up_time,
                log_crash_report=True)
            if ad.crash_report_preflight:
                msg = "Find crash reports %s before test starts" % (
                    ad.crash_report_preflight)
                ad.log.warn(msg)
                result = False
        return result
