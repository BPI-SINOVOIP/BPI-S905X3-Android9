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
    Test Script for Telephony Pre Check In Sanity
"""

import collections
import time

from acts import signals
from acts.test_decorators import test_tracker_info
from acts.controllers.sl4a_lib.sl4a_types import Sl4aNetworkInfo
from acts.test_utils.tel.TelephonyBaseTest import TelephonyBaseTest
from acts.test_utils.tel.tel_data_utils import wifi_tethering_setup_teardown
from acts.test_utils.tel.tel_defines import CAPABILITY_VOLTE
from acts.test_utils.tel.tel_defines import CAPABILITY_VT
from acts.test_utils.tel.tel_defines import CAPABILITY_WFC
from acts.test_utils.tel.tel_defines import CAPABILITY_OMADM
from acts.test_utils.tel.tel_defines import MAX_WAIT_TIME_PROVISIONING
from acts.test_utils.tel.tel_defines import MAX_WAIT_TIME_TETHERING_ENTITLEMENT_CHECK
from acts.test_utils.tel.tel_defines import TETHERING_MODE_WIFI
from acts.test_utils.tel.tel_defines import WAIT_TIME_AFTER_REBOOT
from acts.test_utils.tel.tel_defines import WAIT_TIME_AFTER_CRASH
from acts.test_utils.tel.tel_defines import WFC_MODE_WIFI_PREFERRED
from acts.test_utils.tel.tel_defines import VT_STATE_BIDIRECTIONAL
from acts.test_utils.tel.tel_lookup_tables import device_capabilities
from acts.test_utils.tel.tel_lookup_tables import operator_capabilities
from acts.test_utils.tel.tel_test_utils import call_setup_teardown
from acts.test_utils.tel.tel_test_utils import ensure_phone_subscription
from acts.test_utils.tel.tel_test_utils import get_model_name
from acts.test_utils.tel.tel_test_utils import get_operator_name
from acts.test_utils.tel.tel_test_utils import get_outgoing_voice_sub_id
from acts.test_utils.tel.tel_test_utils import get_slot_index_from_subid
from acts.test_utils.tel.tel_test_utils import is_sim_locked
from acts.test_utils.tel.tel_test_utils import power_off_sim
from acts.test_utils.tel.tel_test_utils import power_on_sim
from acts.test_utils.tel.tel_test_utils import reboot_device
from acts.test_utils.tel.tel_test_utils import sms_send_receive_verify
from acts.test_utils.tel.tel_test_utils import mms_send_receive_verify
from acts.test_utils.tel.tel_test_utils import toggle_airplane_mode
from acts.test_utils.tel.tel_test_utils import wait_for_cell_data_connection
from acts.test_utils.tel.tel_test_utils import verify_http_connection
from acts.test_utils.tel.tel_test_utils import toggle_airplane_mode_by_adb
from acts.test_utils.tel.tel_test_utils import trigger_modem_crash
from acts.test_utils.tel.tel_test_utils import trigger_modem_crash_by_modem
from acts.test_utils.tel.tel_test_utils import unlock_sim
from acts.test_utils.tel.tel_test_utils import wait_for_state
from acts.test_utils.tel.tel_voice_utils import is_phone_in_call_3g
from acts.test_utils.tel.tel_voice_utils import is_phone_in_call_iwlan
from acts.test_utils.tel.tel_voice_utils import is_phone_in_call_volte
from acts.test_utils.tel.tel_voice_utils import phone_setup_voice_3g
from acts.test_utils.tel.tel_voice_utils import phone_setup_csfb
from acts.test_utils.tel.tel_voice_utils import phone_setup_iwlan
from acts.test_utils.tel.tel_voice_utils import phone_setup_voice_general
from acts.test_utils.tel.tel_voice_utils import phone_setup_volte
from acts.test_utils.tel.tel_video_utils import video_call_setup_teardown
from acts.test_utils.tel.tel_video_utils import phone_setup_video
from acts.test_utils.tel.tel_video_utils import \
    is_phone_in_call_video_bidirectional

from acts.utils import get_current_epoch_time
from acts.utils import rand_ascii_str


class TelLiveRebootStressTest(TelephonyBaseTest):
    def __init__(self, controllers):
        TelephonyBaseTest.__init__(self, controllers)

        self.stress_test_number = self.get_stress_test_number()
        self.wifi_network_ssid = self.user_params["wifi_network_ssid"]

        try:
            self.wifi_network_pass = self.user_params["wifi_network_pass"]
        except KeyError:
            self.wifi_network_pass = None

        self.dut = self.android_devices[0]
        self.ad_reference = self.android_devices[1] if len(
            self.android_devices) > 1 else None
        self.dut_model = get_model_name(self.dut)
        self.dut_operator = get_operator_name(self.log, self.dut)
        self.dut_capabilities = set(
            device_capabilities.get(
                self.dut_model, device_capabilities["default"])) & set(
                    operator_capabilities.get(
                        self.dut_operator, operator_capabilities["default"]))
        self.user_params["check_crash"] = False
        self.skip_reset_between_cases = False

    def setup_class(self):
        TelephonyBaseTest.setup_class(self)
        methods = [("check_subscription",
                    self._check_subscription), ("check_data",
                                                self._check_data),
                   ("check_call_setup_teardown",
                    self._check_call_setup_teardown), ("check_sms",
                                                       self._check_sms),
                   ("check_mms", self._check_mms), ("check_lte_data",
                                                    self._check_lte_data),
                   ("check_volte",
                    self._check_volte), ("check_vt",
                                         self._check_vt), ("check_wfc",
                                                           self._check_wfc),
                   ("check_3g", self._check_3g), ("check_tethering",
                                                  self._check_tethering)]
        self.testing_methods = []
        for name, func in methods:
            check_result = func()
            self.dut.log.info("%s is %s before tests start", name,
                              check_result)
            if check_result:
                self.testing_methods.append((name, func))
        self.log.info("Working features: %s", self.testing_methods)

    def feature_validator(self, **kwargs):
        failed_tests = []
        for name, func in self.testing_methods:
            if kwargs.get(name, True):
                if not func():
                    self.log.error("%s failed", name)
                    failed_tests.append(name)
                else:
                    self.log.info("%s succeeded", name)
        if failed_tests:
            self.log.error("%s failed", failed_tests)
        return failed_tests

    def _check_subscription(self):
        if not ensure_phone_subscription(self.log, self.dut):
            self.dut.log.error("Subscription check failed")
            return False
        else:
            return True

    def _check_provision(self):
        if CAPABILITY_OMADM in self.dut_capabilities:
            if not wait_for_state(self.dut.droid.imsIsVolteProvisionedOnDevice,
                                  True):
                self.log.error("VoLTE provisioning check fails.")
                return False
            else:
                return True
        return False

    def _clear_provisioning(self):
        if CAPABILITY_OMADM in self.dut_capabilities:
            self.log.info("Clear Provisioning bit")
            self.dut.droid.imsSetVolteProvisioning(False)
        return True

    def _check_call_setup_teardown(self):
        if not call_setup_teardown(self.log, self.dut, self.ad_reference,
                                   self.dut):
            self.log.error("Phone call test failed.")
            return False
        return True

    def _check_sms(self):
        if not sms_send_receive_verify(self.log, self.dut, self.ad_reference,
                                       [rand_ascii_str(180)]):
            self.log.error("SMS test failed")
            return False
        return True

    def _check_mms(self):
        message_array = [("Test Message", rand_ascii_str(180), None)]
        if not mms_send_receive_verify(self.log, self.dut, self.ad_reference,
                                       message_array):
            self.log.error("MMS test failed")
            return False
        return True

    def _check_data(self):
        if not verify_http_connection(self.log, self.dut):
            self.log.error("Data connection is not available.")
            return False
        return True

    def _get_list_average(self, input_list):
        total_sum = float(sum(input_list))
        total_count = float(len(input_list))
        if input_list == []:
            return False
        return float(total_sum / total_count)

    def _check_lte_data(self):
        self.log.info("Check LTE data.")
        if not phone_setup_csfb(self.log, self.dut):
            self.log.error("Failed to setup LTE data.")
            return False
        if not verify_http_connection(self.log, self.dut):
            self.log.error("Data not available on cell.")
            return False
        return True

    def _check_volte(self):
        if CAPABILITY_VOLTE in self.dut_capabilities:
            self.log.info("Check VoLTE")
            if not phone_setup_volte(self.log, self.dut):
                self.log.error("Failed to setup VoLTE.")
                return False
            time.sleep(5)
            if not call_setup_teardown(self.log, self.dut, self.ad_reference,
                                       self.dut, is_phone_in_call_volte):
                self.log.error("VoLTE Call Failed.")
                return False
            if not sms_send_receive_verify(self.log, self.dut,
                                           self.ad_reference,
                                           [rand_ascii_str(50)]):
                self.log.error("SMS failed")
                return False
        return True

    def _check_vt(self):
        if CAPABILITY_VT in self.dut_capabilities:
            self.log.info("Check VT")
            if not phone_setup_video(self.log, self.dut):
                self.dut.log.error("Failed to setup VT.")
                return False
            time.sleep(5)
            if not video_call_setup_teardown(
                    self.log,
                    self.dut,
                    self.ad_reference,
                    self.dut,
                    video_state=VT_STATE_BIDIRECTIONAL,
                    verify_caller_func=is_phone_in_call_video_bidirectional,
                    verify_callee_func=is_phone_in_call_video_bidirectional):
                self.log.error("VT Call Failed.")
                return False
        return True

    def _check_wfc(self):
        if CAPABILITY_WFC in self.dut_capabilities:
            self.log.info("Check WFC")
            if not phone_setup_iwlan(
                    self.log, self.dut, True, WFC_MODE_WIFI_PREFERRED,
                    self.wifi_network_ssid, self.wifi_network_pass):
                self.log.error("Failed to setup WFC.")
                return False
            if not call_setup_teardown(self.log, self.dut, self.ad_reference,
                                       self.dut, is_phone_in_call_iwlan):
                self.log.error("WFC Call Failed.")
                return False
            if not sms_send_receive_verify(self.log, self.dut,
                                           self.ad_reference,
                                           [rand_ascii_str(50)]):
                self.log.error("SMS failed")
                return False
        return True

    def _check_3g(self):
        self.log.info("Check 3G data and CS call")
        if not phone_setup_voice_3g(self.log, self.dut):
            self.log.error("Failed to setup 3G")
            return False
        if not verify_http_connection(self.log, self.dut):
            self.log.error("Data not available on cell.")
            return False
        if not call_setup_teardown(self.log, self.dut, self.ad_reference,
                                   self.dut, is_phone_in_call_3g):
            self.log.error("WFC Call Failed.")
            return False
        if not sms_send_receive_verify(self.log, self.dut, self.ad_reference,
                                       [rand_ascii_str(50)]):
            self.log.error("SMS failed")
            return False
        return True

    def _check_tethering(self):
        self.log.info("Check tethering")
        for i in range(3):
            try:
                if not self.dut.droid.carrierConfigIsTetheringModeAllowed(
                        TETHERING_MODE_WIFI,
                        MAX_WAIT_TIME_TETHERING_ENTITLEMENT_CHECK):
                    self.log.error("Tethering Entitlement check failed.")
                    if i == 2: return False
                    time.sleep(10)
            except Exception as e:
                if i == 2:
                    self.dut.log.error(e)
                    return False
                time.sleep(10)
        if not wifi_tethering_setup_teardown(
                self.log,
                self.dut, [self.ad_reference],
                check_interval=5,
                check_iteration=1):
            self.log.error("Tethering check failed.")
            return False
        return True

    def _check_data_roaming_status(self):
        if not self.dut.droid.telephonyIsDataEnabled():
            self.log.info("Enabling Cellular Data")
            telephonyToggleDataConnection(True)
        else:
            self.log.info("Cell Data is Enabled")
        self.log.info("Waiting for cellular data to be connected")
        if not wait_for_cell_data_connection(self.log, self.dut, state=True):
            self.log.error("Failed to enable cell data")
            return False
        self.log.info("Cellular data connected, checking NetworkInfos")
        roaming_state = self.dut.droid.telephonyCheckNetworkRoaming()
        for network_info in self.dut.droid.connectivityNetworkGetAllInfo():
            sl4a_network_info = Sl4aNetworkInfo.from_dict(network_info)
            if sl4a_network_info.isRoaming:
                self.log.warning("We don't expect to be roaming")
            if sl4a_network_info.isRoaming != roaming_state:
                self.log.error(
                    "Mismatched Roaming Status Information Telephony: {}, NetworkInfo {}".
                    format(roaming_state, sl4a_network_info.isRoaming))
                self.log.error(network_info)
                return False
        return True

    def _telephony_monitor_test(self, negative_test=False):
        """
        Steps -
        1. Reboot the phone
        2. Start Telephony Monitor using adb/developer options
        3. Verify if it is running
        4. Phone Call from A to B
        5. Answer on B
        6. Trigger ModemSSR on B
        7. There will be a call drop with Media Timeout/Server Unreachable
        8. Parse logcat to confirm that

        Expected Results:
            UI Notification is received by User

        Returns:
            True is pass, False if fail.
        """
        self.number_of_devices = 2
        ads = self.android_devices
        # Ensure apk is running/not running
        monitor_apk = None
        for apk in ("com.google.telephonymonitor",
                    "com.google.android.connectivitymonitor"):
            if ads[0].is_apk_installed(apk):
                ads[0].log.info("apk %s is installed", apk)
                monitor_apk = apk
                break
        if not monitor_apk:
            ads[0].log.info(
                "ConnectivityMonitor|TelephonyMonitor is not installed")
            return False

        ads[0].adb.shell(
            "am start -n com.android.settings/.DevelopmentSettings",
            ignore_status=True)
        cmd = "setprop persist.radio.enable_tel_mon user_enabled"
        ads[0].log.info(cmd)
        ads[0].adb.shell(cmd)

        if not ads[0].is_apk_running(monitor_apk):
            ads[0].log.info("%s is not running", monitor_apk)
            # Reboot
            ads = self.android_devices
            ads[0].log.info("reboot to bring up %s", monitor_apk)
            reboot_device(ads[0])
            for i in range(30):
                if ads[0].is_apk_running(monitor_apk):
                    ads[0].log.info("%s is running after reboot", monitor_apk)
                    break
                elif i == 19:
                    ads[0].log.error("%s is not running after reboot",
                                     monitor_apk)
                    return False
                else:
                    ads[0].log.info(
                        "%s is not running after reboot. Wait and check again",
                        monitor_apk)
                    time.sleep(30)

        ads[0].adb.shell(
            "am start -n com.android.settings/.DevelopmentSettings",
            ignore_status=True)
        monitor_setting = ads[0].adb.getprop("persist.radio.enable_tel_mon")
        ads[0].log.info("radio.enable_tel_mon setting is %s", monitor_setting)
        expected_monitor_setting = "disabled" if negative_test else "user_enabled"
        cmd = "setprop persist.radio.enable_tel_mon %s" % (
            expected_monitor_setting)
        if monitor_setting != expected_monitor_setting:
            ads[0].log.info(cmd)
            ads[0].adb.shell(cmd)

        if not call_setup_teardown(
                self.log, ads[0], ads[1], ad_hangup=None,
                wait_time_in_call=10):
            self.log.error("Call setup failed")
            return False

        # Modem SSR
        time.sleep(5)
        ads[0].log.info("Triggering ModemSSR")
        if (not ads[0].is_apk_installed("com.google.mdstest")
            ) or ads[0].adb.getprop("ro.build.version.release")[0] in (
                "8", "O", "7", "N") or self.dut.model in ("angler", "bullhead",
                                                          "sailfish",
                                                          "marlin"):
            trigger_modem_crash(self.dut)
        else:
            trigger_modem_crash_by_modem(self.dut)

        try:
            if ads[0].droid.telecomIsInCall():
                ads[0].log.info("Still in call after call drop trigger event")
                return False
            else:
                reasons = self.dut.search_logcat(
                    "qcril_qmi_voice_map_qmi_to_ril_last_call_failure_cause")
                if reasons:
                    ads[0].log.info(reasons[-1]["log_message"])
        except Exception as e:
            ads[0].log.error(e)
        # Parse logcat for UI notification
        result = True
        if not negative_test:
            if ads[0].search_logcat("Bugreport notification title Call Drop:"):
                ads[0].log.info(
                    "User got Call Drop Notification with TelephonyMonitor on")
            else:
                ads[0].log.error(
                    "User didn't get Call Drop Notify with TelephonyMonitor on"
                )
                result = False
        else:
            if ads[0].search_logcat("Bugreport notification title Call Drop:"):
                ads[0].log.error("User got the Call Drop Notification with "
                                 "TelephonyMonitor/ConnectivityMonitor off")
                result = False
            else:
                ads[0].log.info("User still get Call Drop Notify with "
                                "TelephonyMonitor/ConnectivityMonitor off")
        reboot_device(ads[0])
        return result

    def _reboot_stress_test(self, **kwargs):
        """Reboot Reliability Test

        Arguments:
            check_provision: whether to check provisioning after reboot.
            check_call_setup_teardown: whether to check setup and teardown a call.
            check_lte_data: whether to check the LTE data.
            check_volte: whether to check Voice over LTE.
            check_wfc: whether to check Wifi Calling.
            check_3g: whether to check 3G.
            check_tethering: whether to check Tethering.
            check_data_roaming: whether to check Data Roaming.
            clear_provision: whether to clear provisioning before reboot.

        Expected Results:
            No crash happens in stress test.

        Returns:
            True is pass, False if fail.
        """
        self.number_of_devices = 2
        toggle_airplane_mode(self.log, self.dut, False)
        phone_setup_voice_general(self.log, self.ad_reference)
        fail_count = collections.defaultdict(int)
        test_result = True

        for i in range(1, self.stress_test_number + 1):
            self.log.info("Reboot Stress Test %s Iteration: <%s> / <%s>",
                          self.test_name, i, self.stress_test_number)
            begin_time = get_current_epoch_time()
            self.dut.log.info("Reboot")
            reboot_device(self.dut)
            self.log.info("{} wait {}s for radio up.".format(
                self.dut.serial, WAIT_TIME_AFTER_REBOOT))
            time.sleep(WAIT_TIME_AFTER_REBOOT)
            failed_tests = self.feature_validator(**kwargs)
            for test in failed_tests:
                fail_count[test] += 1

            crash_report = self.dut.check_crash_report(
                "%s_%s" % (self.test_name, i),
                begin_time,
                log_crash_report=True)
            if crash_report:
                fail_count["crashes"] += 1
            if failed_tests or crash_report:
                self.log.error(
                    "Reboot Stress Test Iteration <%s> / <%s> FAIL",
                    i,
                    self.stress_test_number,
                )
                self._take_bug_report("%s_%s" % (self.test_name, i),
                                      begin_time)
            else:
                self.log.info(
                    "Reboot Stress Test Iteration <%s> / <%s> PASS",
                    i,
                    self.stress_test_number,
                )
            self.log.info("Total failure count: %s", list(fail_count))

        for failure, count in fail_count.items():
            if count:
                self.log.error("%s %s failures in %s iterations", count,
                               failure, self.stress_test_number)
                test_result = False
        return test_result

    def _crash_recovery_test(self, process="modem", **kwargs):
        """Crash Recovery Test

        Arguments:
            process: the process to be killed. For example:
                "rild", "netmgrd", "com.android.phone", "imsqmidaemon",
                "imsdatadaemon", "ims_rtp_daemon",
                "com.android.ims.rcsservice", "system_server", "cnd",
                "modem"
            check_lte_data: whether to check the LTE data.
            check_volte: whether to check Voice over LTE.
            check_vt: whether to check VT
            check_wfc: whether to check Wifi Calling.

        Expected Results:
            All Features should work as intended post crash recovery

        Returns:
            True is pass, False if fail.
        """
        self.number_of_devices = 2

        if process == "modem":
            self.user_params["check_crash"] = False
            self.dut.log.info("Crash modem from kernal")
            trigger_modem_crash(self.dut)
        elif process == "modem-crash":
            self.user_params["check_crash"] = False
            self.dut.log.info("Crash modem from modem")
            trigger_modem_crash_by_modem(self.dut)
        elif process == "sim":
            self.user_params["check_crash"] = True
            sub_id = get_outgoing_voice_sub_id(self.dut)
            slot_index = get_slot_index_from_subid(self.log, self.dut, sub_id)
            if not power_off_sim(self.dut, slot_index):
                self.dut.log.warning("Fail to power off SIM")
                raise signals.TestSkip("Power cycle SIM not working")
            if not power_on_sim(self.dut, slot_index):
                self.dut.log.error("Fail to power on SIM slot")
                setattr(self.dut, "reboot_to_recover", True)
                return False
        else:
            self.dut.log.info("Crash recover test by killing process <%s>",
                              process)
            process_pid = self.dut.adb.shell("pidof %s" % process)
            self.dut.log.info("Pid of %s is %s", process, process_pid)
            if not process_pid:
                self.dut.log.error("Process %s not running", process)
                return False
            if process in ("netd", "system_server"):
                self.dut.stop_services()
            self.dut.adb.shell("kill -9 %s" % process_pid, ignore_status=True)
            self.dut.log.info("Wait %s sec for process %s come up.",
                              WAIT_TIME_AFTER_CRASH, process)
            time.sleep(WAIT_TIME_AFTER_CRASH)
            if process in ("netd", "system_server"):
                self.dut.ensure_screen_on()
                try:
                    self.dut.start_services(self.dut.skip_sl4a)
                except Exception as e:
                    self.dut.log.warning(e)
            process_pid_new = self.dut.adb.shell("pidof %s" % process)
            if process_pid == process_pid_new:
                self.dut.log.error(
                    "Process %s has the same pid: old:%s new:%s", process,
                    process_pid, process_pid_new)
        try:
            self.dut.droid.logI("Start testing after restarting %s" % process)
        except Exception:
            self.dut.ensure_screen_on()
            self.dut.start_services(self.dut.skip_sl4a)
            if is_sim_locked(self.dut):
                unlock_sim(self.dut)

        begin_time = get_current_epoch_time()
        failed_tests = self.feature_validator(**kwargs)
        crash_report = self.dut.check_crash_report(
            self.test_name, begin_time, log_crash_report=True)
        if failed_tests or crash_report:
            if failed_tests:
                self.dut.log.error("%s failed after %s restart", failed_tests,
                                   process)
                setattr(self.dut, "reboot_to_recover", True)
            if crash_report:
                self.dut.log.error("Crash %s found after %s restart",
                                   crash_report, process)
            return False
        else:
            return True

    def _telephony_bootup_time_test(self, **kwargs):
        """Telephony Bootup Perf Test

        Arguments:
            check_lte_data: whether to check the LTE data.
            check_volte: whether to check Voice over LTE.
            check_wfc: whether to check Wifi Calling.

        Expected Results:
            Time

        Returns:
            True is pass, False if fail.
        """
        self.number_of_devices = 1
        ad = self.dut
        toggle_airplane_mode(self.log, ad, False)
        if not phone_setup_volte(self.log, ad):
            ad.log.error("Failed to setup VoLTE.")
            return False
        fail_count = collections.defaultdict(int)
        test_result = True
        keyword_time_dict = {}

        for i in range(1, self.stress_test_number + 1):
            ad.log.info("Telephony Bootup Time Test %s Iteration: %d / %d",
                        self.test_name, i, self.stress_test_number)
            ad.log.info("reboot!")
            reboot_device(ad)
            iteration_result = "pass"

            time.sleep(30)
            text_search_mapping = {
                'boot_complete': "processing action (sys.boot_completed=1)",
                'Voice_Reg':
                "< VOICE_REGISTRATION_STATE {.regState = REG_HOME",
                'Data_Reg': "< DATA_REGISTRATION_STATE {.regState = REG_HOME",
                'Data_Call_Up': "onSetupConnectionCompleted result=SUCCESS",
                'VoLTE_Enabled': "isVolteEnabled=true",
            }

            text_obj_mapping = {
                "boot_complete": None,
                "Voice_Reg": None,
                "Data_Reg": None,
                "Data_Call_Up": None,
                "VoLTE_Enabled": None,
            }
            blocked_for_calculate = ["boot_complete"]

            for tel_state in text_search_mapping:
                dict_match = ad.search_logcat(text_search_mapping[tel_state])
                if len(dict_match) != 0:
                    text_obj_mapping[tel_state] = dict_match[0]['datetime_obj']
                else:
                    ad.log.error("Cannot Find Text %s in logcat",
                                 text_search_mapping[tel_state])
                    blocked_for_calculate.append(tel_state)

            for tel_state in text_search_mapping:
                if tel_state not in blocked_for_calculate:
                    time_diff = text_obj_mapping[tel_state] - \
                                text_obj_mapping['boot_complete']
                    if time_diff.seconds > 100:
                        continue
                    if tel_state in keyword_time_dict:
                        keyword_time_dict[tel_state].append(time_diff.seconds)
                    else:
                        keyword_time_dict[tel_state] = [
                            time_diff.seconds,
                        ]

            ad.log.info("Telephony Bootup Time Test %s Iteration: %d / %d %s",
                        self.test_name, i, self.stress_test_number,
                        iteration_result)

        for tel_state in text_search_mapping:
            if tel_state not in blocked_for_calculate:
                avg_time = self._get_list_average(keyword_time_dict[tel_state])
                if avg_time < 12.0:
                    ad.log.info("Average %s for %d iterations = %.2f seconds",
                                tel_state, self.stress_test_number, avg_time)
                else:
                    ad.log.error("Average %s for %d iterations = %.2f seconds",
                                 tel_state, self.stress_test_number, avg_time)
                    fail_count[tel_state] += 1

        ad.log.info("Bootup Time Dict {}".format(keyword_time_dict))
        for failure, count in fail_count.items():
            if count:
                ad.log.error("%d %d failures in %d iterations", count, failure,
                             self.stress_test_number)
                test_result = False
        return test_result

    """ Tests Begin """

    @test_tracker_info(uuid="4d9b425b-f804-45f4-8f47-0ba3f01a426b")
    @TelephonyBaseTest.tel_test_wrap
    def test_reboot_stress(self):
        """Reboot Reliability Test

        Steps:
            1. Reboot DUT.
            2. Check Provisioning bit (if support provisioning)
            3. Wait for DUT to camp on LTE, Verify Data.
            4. Enable VoLTE, check IMS registration. Wait for DUT report VoLTE
                enabled, make VoLTE call. And verify VoLTE SMS.
                (if support VoLTE)
            5. Connect WiFi, enable WiFi Calling, wait for DUT report WiFi
                Calling enabled and make a WFC call and verify SMS.
                Disconnect WiFi. (if support WFC)
            6. Wait for DUT to camp on 3G, Verify Data.
            7. Make CS call and verify SMS.
            8. Verify Tethering Entitlement Check and Verify WiFi Tethering.
            9. Check crashes.
            10. Repeat Step 1~9 for N times. (before reboot, clear Provisioning
                bit if provisioning is supported)

        Expected Results:
            No crash happens in stress test.

        Returns:
            True is pass, False if fail.
        """
        return self._reboot_stress_test(
            check_provision=True,
            check_call_setup_teardown=True,
            check_lte_data=True,
            check_volte=True,
            check_wfc=True,
            check_3g=True,
            check_tethering=True,
            check_data_roaming=False,
            clear_provision=True)

    @test_tracker_info(uuid="8b0e2c06-02bf-40fd-a374-08860e482757")
    @TelephonyBaseTest.tel_test_wrap
    def test_reboot_stress(self):
        """Reboot Reliability Test

        Steps:
            1. Reboot DUT.
            2. Check phone call .
            3. Check crashes.
            4. Repeat Step 1~9 for N times. (before reboot, clear Provisioning
                bit if provisioning is supported)

        Expected Results:
            No crash happens in stress test.

        Returns:
            True is pass, False if fail.
        """
        return self._reboot_stress_test()

    @test_tracker_info(uuid="109d59ff-a488-4a68-87fd-2d8d0c035326")
    @TelephonyBaseTest.tel_test_wrap
    def test_bootup_optimized_stress(self):
        """Bootup Optimized Reliability Test

        Steps:
            1. Reboot DUT.
            2. Check Provisioning bit (if support provisioning)
            3. Wait for DUT to camp on LTE, Verify Data.
            4. Enable VoLTE, check IMS registration. Wait for DUT report VoLTE
                enabled, make VoLTE call. And verify VoLTE SMS.
                (if support VoLTE)
            5. Connect WiFi, enable WiFi Calling, wait for DUT report WiFi
                Calling enabled and make a WFC call and verify SMS.
                Disconnect WiFi. (if support WFC)
            6. Wait for DUT to camp on 3G, Verify Data.
            7. Make CS call and verify SMS.
            8. Verify Tethering Entitlement Check and Verify WiFi Tethering.
            9. Check crashes.
            10. Repeat Step 1~9 for N times. (before reboot, clear Provisioning
                bit if provisioning is supported)

        Expected Results:
            No crash happens in stress test.

        Returns:
            True is pass, False if fail.
        """
        return self._telephony_bootup_time_test()

    @test_tracker_info(uuid="08752fac-dbdb-4d5b-91f6-4ffc3a3ac6d6")
    @TelephonyBaseTest.tel_test_wrap
    def test_crash_recovery_modem(self):
        """Crash Recovery Test

        Steps:
            1. Crash modem
            2. Post crash recovery, verify Voice, Data, SMS, VoLTE, VT

        Expected Results:
            No crash happens in functional test, features work fine.

        Returns:
            True is pass, False if fail.
        """
        return self._crash_recovery_test(process="modem")

    @test_tracker_info(uuid="ce5f4d63-7f3d-48b7-831d-2c1d5db60733")
    @TelephonyBaseTest.tel_test_wrap
    def test_crash_recovery_crash_modem_from_modem(self):
        """Crash Recovery Test

        Steps:
            1. Crash modem
            2. Post crash recovery, verify Voice, Data, SMS, VoLTE, VT

        Expected Results:
            No crash happens in functional test, features work fine.

        Returns:
            True is pass, False if fail.
        """
        if (not self.dut.is_apk_installed("com.google.mdstest")) or (
                self.dut.adb.getprop("ro.build.version.release")[0] in
            ("8", "O", "7", "N")) or self.dut.model in ("angler", "bullhead",
                                                        "sailfish", "marlin"):
            raise signals.TestSkip(
                "com.google.mdstest not installed or supported")
        return self._crash_recovery_test(process="modem-crash")

    @test_tracker_info(uuid="489284e8-77c9-4961-97c8-b6f1a833ff90")
    @TelephonyBaseTest.tel_test_wrap
    def test_crash_recovery_rild(self):
        """Crash Recovery Test

        Steps:
            1. Crash rild
            2. Post crash recovery, verify Voice, Data, SMS, VoLTE, VT

        Expected Results:
            No crash happens in functional test, features work fine.

        Returns:
            True is pass, False if fail.
        """
        return self._crash_recovery_test(process="rild")

    @test_tracker_info(uuid="e1b34b2c-99e6-4966-a11c-88cedc953b47")
    @TelephonyBaseTest.tel_test_wrap
    def test_crash_recovery_netmgrd(self):
        """Crash Recovery Test

        Steps:
            1. Crash netmgrd
            2. Post crash recovery, verify Voice, Data, SMS, VoLTE, VT

        Expected Results:
            No crash happens in functional test, features work fine.

        Returns:
            True is pass, False if fail.
        """
        return self._crash_recovery_test(process="netmgrd")

    @test_tracker_info(uuid="fa34f994-bc49-4444-9187-87691c94b4f4")
    @TelephonyBaseTest.tel_test_wrap
    def test_crash_recovery_phone(self):
        """Crash Recovery Test

        Steps:
            1. Crash com.android.phone
            2. Post crash recovery, verify Voice, Data, SMS, VoLTE, VT

        Expected Results:
            No crash happens in functional test, features work fine.

        Returns:
            True is pass, False if fail.
        """
        return self._crash_recovery_test(process="com.android.phone")

    @test_tracker_info(uuid="6f5a24bb-3cf3-4362-9675-36a6be90282f")
    @TelephonyBaseTest.tel_test_wrap
    def test_crash_recovery_imsqmidaemon(self):
        """Crash Recovery Test

        Steps:
            1. Crash imsqmidaemon
            2. Post crash recovery, verify Voice, Data, SMS, VoLTE, VT

        Expected Results:
            No crash happens in functional test, features work fine.

        Returns:
            True is pass, False if fail.
        """
        return self._crash_recovery_test(process="imsqmidaemon")

    @test_tracker_info(uuid="7a8dc971-054b-47e7-9e57-3bb7b39937d3")
    @TelephonyBaseTest.tel_test_wrap
    def test_crash_recovery_imsdatadaemon(self):
        """Crash Recovery Test

        Steps:
            1. Crash imsdatadaemon
            2. Post crash recovery, verify Voice, Data, SMS, VoLTE, VT

        Expected Results:
            No crash happens in functional test, features work fine.

        Returns:
            True is pass, False if fail.
        """
        return self._crash_recovery_test(process="imsdatadaemon")

    @test_tracker_info(uuid="350ca58c-01f2-4a61-baff-530b8b24f1f6")
    @TelephonyBaseTest.tel_test_wrap
    def test_crash_recovery_ims_rtp_daemon(self):
        """Crash Recovery Test

        Steps:
            1. Crash imsdatadaemon
            2. Post crash recovery, verify Voice, Data, SMS, VoLTE, VT

        Expected Results:
            No crash happens in functional test, features work fine.

        Returns:
            True is pass, False if fail.
        """
        return self._crash_recovery_test(process="ims_rtp_daemon")

    @test_tracker_info(uuid="af78f33a-2b50-4c55-a302-3701b655c557")
    @TelephonyBaseTest.tel_test_wrap
    def test_crash_recovery_ims_rcsservice(self):
        """Crash Recovery Test

        Steps:
            1. Crash imsdatadaemon
            2. Post crash recovery, verify Voice, Data, SMS, VoLTE, VT

        Expected Results:
            No crash happens in functional test, features work fine.

        Returns:
            True is pass, False if fail.
        """
        return self._crash_recovery_test(process="com.android.ims.rcsservice")

    @test_tracker_info(uuid="8119aeef-84ba-415c-88ea-6eba35bd91fd")
    @TelephonyBaseTest.tel_test_wrap
    def test_crash_recovery_system_server(self):
        """Crash Recovery Test

        Steps:
            1. Crash system_server
            2. Post crash recovery, verify Voice, Data, SMS, VoLTE, VT

        Expected Results:
            No crash happens in functional test, features work fine.

        Returns:
            True is pass, False if fail.
        """
        return self._crash_recovery_test(process="system_server")

    @test_tracker_info(uuid="c3891aca-9e1a-4e37-9f2f-23f12ef0a86f")
    @TelephonyBaseTest.tel_test_wrap
    def test_crash_recovery_cnd(self):
        """Crash Recovery Test

        Steps:
            1. Crash cnd
            2. Post crash recovery, verify Voice, Data, SMS, VoLTE, VT

        Expected Results:
            No crash happens in functional test, features work fine.

        Returns:
            True is pass, False if fail.
        """
        return self._crash_recovery_test(process="cnd")

    @test_tracker_info(uuid="c1b661b9-d5cf-4a22-90a9-3fd55ddc2f3f")
    @TelephonyBaseTest.tel_test_wrap
    def test_sim_slot_power_cycle(self):
        """SIM slot power cycle Test

        Steps:
            1. Power cycle SIM slot to simulate SIM resit
            2. Post power cycle SIM, verify Voice, Data, SMS, VoLTE, VT

        Expected Results:
            No crash happens in functional test, features work fine.

        Returns:
            True is pass, False if fail.
        """
        return self._crash_recovery_test(process="sim")

    @test_tracker_info(uuid="b6d2fccd-5dfd-4637-aa3b-257837bfba54")
    @TelephonyBaseTest.tel_test_wrap
    def test_telephonymonitor_functional(self):
        """Telephony Monitor Functional Test

        Steps:
            1. Verify Telephony Monitor functionality is working or not
            2. Force Trigger a call drop : media timeout and ensure it is
               notified by Telephony Monitor

        Expected Results:
            feature work fine, and does report to User about Call Drop

        Returns:
            True is pass, False if fail.
        """
        return self._telephony_monitor_test()

    @test_tracker_info(uuid="f048189b-e4bb-46f7-b150-37acf020af6e")
    @TelephonyBaseTest.tel_test_wrap
    def test_telephonymonitor_negative(self):
        """Telephony Monitor Functional Test

        Steps:
            1. Verify Telephony Monitor functionality is working or not
            2. Force Trigger a call drop : media timeout and ensure it is
               not notified by Telephony Monitor

        Expected Results:
            feature work fine, and does not report to User about Call Drop

        Returns:
            True is pass, False if fail.
        """
        return self._telephony_monitor_test(negative_test=True)


""" Tests End """
