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
    Base Class for Defining Common Telephony Test Functionality
"""

import logging
import os
import re
import shutil
import traceback

import acts.controllers.diag_logger

from acts import asserts
from acts import logger as acts_logger
from acts.base_test import BaseTestClass
from acts.controllers.android_device import DEFAULT_QXDM_LOG_PATH
from acts.keys import Config
from acts.signals import TestSignal
from acts.signals import TestAbortClass
from acts.signals import TestAbortAll
from acts.signals import TestBlocked
from acts import records
from acts import utils

from acts.test_utils.tel.tel_subscription_utils import \
    initial_set_up_for_subid_infomation
from acts.test_utils.tel.tel_test_utils import abort_all_tests
from acts.test_utils.tel.tel_test_utils import ensure_phones_default_state
from acts.test_utils.tel.tel_test_utils import ensure_phones_idle
from acts.test_utils.tel.tel_test_utils import print_radio_info
from acts.test_utils.tel.tel_test_utils import reboot_device
from acts.test_utils.tel.tel_test_utils import refresh_sl4a_session
from acts.test_utils.tel.tel_test_utils import run_multithread_func
from acts.test_utils.tel.tel_test_utils import setup_droid_properties
from acts.test_utils.tel.tel_test_utils import set_phone_screen_on
from acts.test_utils.tel.tel_test_utils import set_phone_silent_mode
from acts.test_utils.tel.tel_test_utils import set_qxdm_logger_command
from acts.test_utils.tel.tel_test_utils import start_qxdm_loggers
from acts.test_utils.tel.tel_test_utils import stop_qxdm_loggers
from acts.test_utils.tel.tel_test_utils import unlock_sim
from acts.test_utils.tel.tel_defines import PRECISE_CALL_STATE_LISTEN_LEVEL_BACKGROUND
from acts.test_utils.tel.tel_defines import PRECISE_CALL_STATE_LISTEN_LEVEL_FOREGROUND
from acts.test_utils.tel.tel_defines import PRECISE_CALL_STATE_LISTEN_LEVEL_RINGING
from acts.test_utils.tel.tel_defines import WIFI_VERBOSE_LOGGING_ENABLED
from acts.test_utils.tel.tel_defines import WIFI_VERBOSE_LOGGING_DISABLED


class TelephonyBaseTest(BaseTestClass):
    def __init__(self, controllers):

        BaseTestClass.__init__(self, controllers)
        self.logger_sessions = []

        self.log_path = getattr(logging, "log_path", None)
        self.qxdm_log = self.user_params.get("qxdm_log", True)
        qxdm_log_mask_cfg = self.user_params.get("qxdm_log_mask_cfg", None)
        if isinstance(qxdm_log_mask_cfg, list):
            qxdm_log_mask_cfg = qxdm_log_mask_cfg[0]
        if qxdm_log_mask_cfg and "dev/null" in qxdm_log_mask_cfg:
            qxdm_log_mask_cfg = None
        stop_qxdm_loggers(self.log, self.android_devices)
        for ad in self.android_devices:
            try:
                ad.adb.shell("killall -9 tcpdump")
            except AdbError:
                ad.log.warn("Killing existing tcpdump processes failed")
            if not hasattr(ad, "init_log_path"):
                ad.init_log_path = ad.log_path
            ad.log_path = self.log_path
            print_radio_info(ad)
            if not unlock_sim(ad):
                abort_all_tests(ad.log, "unable to unlock SIM")
            ad.wakeup_screen()
            ad.adb.shell("input keyevent 82")
            ad.qxdm_log = getattr(ad, "qxdm_log", self.qxdm_log)
            if ad.qxdm_log:
                qxdm_log_mask = getattr(ad, "qxdm_log_mask", None)
                if qxdm_log_mask_cfg:
                    qxdm_mask_path = DEFAULT_QXDM_LOG_PATH
                    ad.adb.shell("mkdir %s" % qxdm_mask_path)
                    ad.log.info("Push %s to %s", qxdm_log_mask_cfg,
                                qxdm_mask_path)
                    ad.adb.push("%s %s" % (qxdm_log_mask_cfg, qxdm_mask_path))
                    mask_file_name = os.path.split(qxdm_log_mask_cfg)[-1]
                    qxdm_log_mask = os.path.join(qxdm_mask_path,
                                                 mask_file_name)
                set_qxdm_logger_command(ad, mask=qxdm_log_mask)
                ad.adb.pull(
                    "/firmware/image/qdsp6m.qdb %s" % ad.init_log_path,
                    ignore_status=True)

        start_qxdm_loggers(self.log, self.android_devices,
                           utils.get_current_epoch_time())
        self.skip_reset_between_cases = self.user_params.get(
            "skip_reset_between_cases", True)

    # Use for logging in the test cases to facilitate
    # faster log lookup and reduce ambiguity in logging.
    @staticmethod
    def tel_test_wrap(fn):
        def _safe_wrap_test_case(self, *args, **kwargs):
            test_id = "%s:%s:%s" % (self.__class__.__name__, self.test_name,
                                    self.log_begin_time.replace(' ', '-'))
            self.test_id = test_id
            self.result_detail = ""
            tries = 2 if self.user_params.get("telephony_auto_rerun") else 1
            for i in range(tries):
                result = True
                log_string = "[Test ID] %s" % test_id
                if i > 1:
                    log_string = "[Rerun]%s" % log_string
                    self.teardown_test()
                    self.setup_test()
                self.log.info(log_string)
                for ad in self.android_devices:
                    ad.log_path = self.log_path
                    try:
                        ad.droid.logI("Started %s" % log_string)
                    except Exception as e:
                        ad.log.warning(e)
                        refresh_sl4a_session(ad)
                try:
                    result = fn(self, *args, **kwargs)
                except (TestSignal, TestAbortClass, TestAbortAll) as signal:
                    if self.result_detail:
                        signal.details = self.result_detail
                    raise
                except Exception as e:
                    self.log.error(traceback.format_exc())
                    asserts.fail(self.result_detail)
                for ad in self.android_devices:
                    try:
                        ad.droid.logI("Finished %s" % log_string)
                    except Exception as e:
                        ad.log.warning(e)
                        refresh_sl4a_session(ad)
                if result: break
            if self.user_params.get("check_crash", True):
                new_crash = ad.check_crash_report(self.test_name,
                                                  self.begin_time, True)
                if new_crash:
                    msg = "Find new crash reports %s" % new_crash
                    ad.log.error(msg)
                    self.result_detail = "%s %s %s" % (self.result_detail,
                                                       ad.serial, msg)
                    result = False
            if result:
                asserts.explicit_pass(self.result_detail)
            else:
                asserts.fail(self.result_detail)

        return _safe_wrap_test_case

    def setup_class(self):
        sim_conf_file = self.user_params.get("sim_conf_file")
        if not sim_conf_file:
            self.log.info("\"sim_conf_file\" is not provided test bed config!")
        else:
            if isinstance(sim_conf_file, list):
                sim_conf_file = sim_conf_file[0]
            # If the sim_conf_file is not a full path, attempt to find it
            # relative to the config file.
            if not os.path.isfile(sim_conf_file):
                sim_conf_file = os.path.join(
                    self.user_params[Config.key_config_path], sim_conf_file)
                if not os.path.isfile(sim_conf_file):
                    self.log.error("Unable to load user config %s ",
                                   sim_conf_file)

        setattr(self, "diag_logger",
                self.register_controller(
                    acts.controllers.diag_logger, required=False))

        if not self.user_params.get("Attenuator"):
            ensure_phones_default_state(self.log, self.android_devices)
        else:
            ensure_phones_idle(self.log, self.android_devices)
        for ad in self.android_devices:
            setup_droid_properties(self.log, ad, sim_conf_file)

            # Setup VoWiFi MDN for Verizon. b/33187374
            build_id = ad.build_info["build_id"]
            if "vzw" in [
                    sub["operator"] for sub in ad.cfg["subscription"].values()
            ] and ad.is_apk_installed("com.google.android.wfcactivation"):
                ad.log.info("setup VoWiFi MDN per b/33187374")
                ad.adb.shell("setprop dbg.vzw.force_wfc_nv_enabled true")
                ad.adb.shell("am start --ei EXTRA_LAUNCH_CARRIER_APP 0 -n "
                             "\"com.google.android.wfcactivation/"
                             ".VzwEmergencyAddressActivity\"")
            # Sub ID setup
            initial_set_up_for_subid_infomation(self.log, ad)
            if "enable_wifi_verbose_logging" in self.user_params:
                ad.droid.wifiEnableVerboseLogging(WIFI_VERBOSE_LOGGING_ENABLED)
            # If device is setup already, skip the following setup procedures
            if getattr(ad, "telephony_test_setup", None):
                continue
            # Disable Emergency alerts
            # Set chrome browser start with no-first-run verification and
            # disable-fre. Give permission to read from and write to storage.
            for cmd in (
                    "pm disable com.android.cellbroadcastreceiver",
                    "pm grant com.android.chrome "
                    "android.permission.READ_EXTERNAL_STORAGE",
                    "pm grant com.android.chrome "
                    "android.permission.WRITE_EXTERNAL_STORAGE",
                    "rm /data/local/chrome-command-line",
                    "am set-debug-app --persistent com.android.chrome",
                    'echo "chrome --no-default-browser-check --no-first-run '
                    '--disable-fre" > /data/local/tmp/chrome-command-line'):
                ad.adb.shell(cmd)

            # Curl for 2016/7 devices
            try:
                if int(ad.adb.getprop("ro.product.first_api_level")) >= 25:
                    out = ad.adb.shell("/data/curl --version")
                    if not out or "not found" in out:
                        tel_data = self.user_params.get("tel_data", "tel_data")
                        if isinstance(tel_data, list):
                            tel_data = tel_data[0]
                        curl_file_path = os.path.join(tel_data, "curl")
                        if not os.path.isfile(curl_file_path):
                            curl_file_path = os.path.join(
                                self.user_params[Config.key_config_path],
                                curl_file_path)
                        if os.path.isfile(curl_file_path):
                            ad.log.info("Pushing Curl to /data dir")
                            ad.adb.push("%s /data" % (curl_file_path))
                            ad.adb.shell(
                                "chmod 777 /data/curl", ignore_status=True)
            except Exception:
                ad.log.info("Failed to push curl on this device")

            # Ensure that a test class starts from a consistent state that
            # improves chances of valid network selection and facilitates
            # logging.
            try:
                if not set_phone_screen_on(self.log, ad):
                    self.log.error("Failed to set phone screen-on time.")
                    return False
                if not set_phone_silent_mode(self.log, ad):
                    self.log.error("Failed to set phone silent mode.")
                    return False
                ad.droid.telephonyAdjustPreciseCallStateListenLevel(
                    PRECISE_CALL_STATE_LISTEN_LEVEL_FOREGROUND, True)
                ad.droid.telephonyAdjustPreciseCallStateListenLevel(
                    PRECISE_CALL_STATE_LISTEN_LEVEL_RINGING, True)
                ad.droid.telephonyAdjustPreciseCallStateListenLevel(
                    PRECISE_CALL_STATE_LISTEN_LEVEL_BACKGROUND, True)
            except Exception as e:
                self.log.error("Failure with %s", e)
            setattr(ad, "telephony_test_setup", True)

        return True

    def teardown_class(self):
        stop_qxdm_loggers(self.log, self.android_devices)
        ensure_phones_default_state(self.log, self.android_devices)
        try:
            for ad in self.android_devices:
                ad.droid.disableDevicePassword()
                if "enable_wifi_verbose_logging" in self.user_params:
                    ad.droid.wifiEnableVerboseLogging(
                        WIFI_VERBOSE_LOGGING_DISABLED)
                if hasattr(ad, "init_log_path"):
                    ad.log_path = ad.init_log_path
            return True
        except Exception as e:
            self.log.error("Failure with %s", e)

    def setup_test(self):
        for ad in self.android_devices:
            ad.ed.clear_all_events()
            output = ad.adb.logcat("-t 1")
            match = re.search(r"\d+-\d+\s\d+:\d+:\d+.\d+", output)
            if match:
                ad.test_log_begin_time = match.group(0)
        if getattr(self, "qxdm_log", True):
            start_qxdm_loggers(self.log, self.android_devices, self.begin_time)
        if getattr(self, "diag_logger", None):
            for logger in self.diag_logger:
                self.log.info("Starting a diagnostic session %s", logger)
                self.logger_sessions.append((logger, logger.start()))
        if self.skip_reset_between_cases:
            ensure_phones_idle(self.log, self.android_devices)
        else:
            ensure_phones_default_state(self.log, self.android_devices)

    def on_exception(self, test_name, begin_time):
        self._pull_diag_logs(test_name, begin_time)
        self._take_bug_report(test_name, begin_time)
        self._cleanup_logger_sessions()

    def on_fail(self, test_name, begin_time):
        self._pull_diag_logs(test_name, begin_time)
        self._take_bug_report(test_name, begin_time)
        self._cleanup_logger_sessions()

    def on_blocked(self, test_name, begin_time):
        self.on_fail(test_name, begin_time)

    def _ad_take_extra_logs(self, ad, test_name, begin_time):
        extra_qxdm_logs_in_seconds = self.user_params.get(
            "extra_qxdm_logs_in_seconds", 60 * 3)
        result = True
        if getattr(ad, "qxdm_log", True):
            # Gather qxdm log modified 3 minutes earlier than test start time
            if begin_time:
                qxdm_begin_time = begin_time - 1000 * extra_qxdm_logs_in_seconds
            else:
                qxdm_begin_time = None
            try:
                ad.get_qxdm_logs(test_name, qxdm_begin_time)
            except Exception as e:
                ad.log.error("Failed to get QXDM log for %s with error %s",
                             test_name, e)
                result = False

        try:
            ad.check_crash_report(test_name, begin_time, log_crash_report=True)
        except Exception as e:
            ad.log.error("Failed to check crash report for %s with error %s",
                         test_name, e)
            result = False

        log_begin_time = getattr(
            ad, "test_log_begin_time", None
        ) or acts_logger.epoch_to_log_line_timestamp(begin_time - 1000 * 60)
        log_path = os.path.join(self.log_path, test_name,
                                "%s_%s.logcat" % (ad.serial, begin_time))
        try:
            ad.adb.logcat(
                'b all -d -v year -t "%s" > %s' % (log_begin_time, log_path),
                timeout=120)
        except Exception as e:
            ad.log.error("Failed to get logcat with error %s", e)
            result = False
        return result

    def _take_bug_report(self, test_name, begin_time):
        if self._skip_bug_report():
            return
        dev_num = getattr(self, "number_of_devices", None) or len(
            self.android_devices)
        tasks = [(self._ad_take_bugreport, (ad, test_name, begin_time))
                 for ad in self.android_devices[:dev_num]]
        tasks.extend([(self._ad_take_extra_logs, (ad, test_name, begin_time))
                      for ad in self.android_devices[:dev_num]])
        run_multithread_func(self.log, tasks)
        for ad in self.android_devices[:dev_num]:
            if getattr(ad, "reboot_to_recover", False):
                reboot_device(ad)
                ad.reboot_to_recover = False
        if not self.user_params.get("zip_log", False): return
        src_dir = os.path.join(self.log_path, test_name)
        file_name = "%s_%s" % (src_dir, begin_time)
        self.log.info("Zip folder %s to %s.zip", src_dir, file_name)
        shutil.make_archive(file_name, "zip", src_dir)
        shutil.rmtree(src_dir)

    def _block_all_test_cases(self, tests):
        """Over-write _block_all_test_case in BaseTestClass."""
        for (i, (test_name, test_func)) in enumerate(tests):
            signal = TestBlocked("Failed class setup")
            record = records.TestResultRecord(test_name, self.TAG)
            record.test_begin()
            # mark all test cases as FAIL
            record.test_fail(signal)
            self.results.add_record(record)
            # only gather bug report for the first test case
            if i == 0:
                self.on_fail(test_name, record.begin_time)

    def on_pass(self, test_name, begin_time):
        self._cleanup_logger_sessions()

    def get_stress_test_number(self):
        """Gets the stress_test_number param from user params.

        Gets the stress_test_number param. If absent, returns default 100.
        """
        return int(self.user_params.get("stress_test_number", 100))
