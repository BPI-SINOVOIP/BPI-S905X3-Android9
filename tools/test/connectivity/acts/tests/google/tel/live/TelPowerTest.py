#!/usr/bin/env python3.4
#
#   Copyright 2016 - The Android Open Source Project
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

import math
import os
from acts.test_utils.tel.TelephonyBaseTest import TelephonyBaseTest
from acts.test_utils.tel.tel_defines import WFC_MODE_WIFI_PREFERRED
from acts.test_utils.tel.tel_test_utils import call_setup_teardown
from acts.test_utils.tel.tel_test_utils import ensure_phones_default_state
from acts.test_utils.tel.tel_test_utils import ensure_phones_idle
from acts.test_utils.tel.tel_test_utils import ensure_wifi_connected
from acts.test_utils.tel.tel_test_utils import hangup_call
from acts.test_utils.tel.tel_test_utils import is_wfc_enabled
from acts.test_utils.tel.tel_test_utils import set_phone_screen_on
from acts.test_utils.tel.tel_test_utils import set_wfc_mode
from acts.test_utils.tel.tel_test_utils import toggle_airplane_mode
from acts.test_utils.tel.tel_test_utils import verify_incall_state
from acts.test_utils.tel.tel_voice_utils import is_phone_in_call_3g
from acts.test_utils.tel.tel_voice_utils import is_phone_in_call_2g
from acts.test_utils.tel.tel_voice_utils import is_phone_in_call_iwlan
from acts.test_utils.tel.tel_voice_utils import is_phone_in_call_volte
from acts.test_utils.tel.tel_voice_utils import phone_idle_2g
from acts.test_utils.tel.tel_voice_utils import phone_idle_3g
from acts.test_utils.tel.tel_voice_utils import phone_idle_iwlan
from acts.test_utils.tel.tel_voice_utils import phone_idle_volte
from acts.test_utils.tel.tel_voice_utils import phone_setup_csfb
from acts.test_utils.tel.tel_voice_utils import phone_setup_iwlan
from acts.test_utils.tel.tel_voice_utils import phone_setup_voice_3g
from acts.test_utils.tel.tel_voice_utils import phone_setup_voice_2g
from acts.test_utils.tel.tel_voice_utils import phone_setup_volte
from acts.utils import create_dir
from acts.utils import disable_doze
from acts.utils import get_current_human_time
from acts.utils import set_adaptive_brightness
from acts.utils import set_ambient_display
from acts.utils import set_auto_rotate
from acts.utils import set_location_service
from acts.utils import set_mobile_data_always_on

# Monsoon output Voltage in V
MONSOON_OUTPUT_VOLTAGE = 4.2
# Monsoon output max current in A
MONSOON_MAX_CURRENT = 7.8

# Default power test pass criteria
DEFAULT_POWER_PASS_CRITERIA = 999

# Sampling rate in Hz
ACTIVE_CALL_TEST_SAMPLING_RATE = 100
# Sample duration in seconds
ACTIVE_CALL_TEST_SAMPLE_TIME = 300
# Offset time in seconds
ACTIVE_CALL_TEST_OFFSET_TIME = 180

# Sampling rate in Hz
IDLE_TEST_SAMPLING_RATE = 100
# Sample duration in seconds
IDLE_TEST_SAMPLE_TIME = 2400
# Offset time in seconds
IDLE_TEST_OFFSET_TIME = 360

# Constant for RAT
RAT_LTE = 'lte'
RAT_3G = '3g'
RAT_2G = '2g'
# Constant for WIFI
WIFI_5G = '5g'
WIFI_2G = '2g'

# For wakeup ping test, the frequency to wakeup. In unit of second.
WAKEUP_PING_TEST_WAKEUP_FREQ = 60

WAKEUP_PING_TEST_NUMBER_OF_ALARM = math.ceil(
    (IDLE_TEST_SAMPLE_TIME * 60 + IDLE_TEST_OFFSET_TIME) /
    WAKEUP_PING_TEST_WAKEUP_FREQ)


class TelPowerTest(TelephonyBaseTest):
    def __init__(self, controllers):
        TelephonyBaseTest.__init__(self, controllers)

    def setup_class(self):
        super().setup_class()
        self.mon = self.monsoons[0]
        self.mon.set_voltage(MONSOON_OUTPUT_VOLTAGE)
        self.mon.set_max_current(MONSOON_MAX_CURRENT)
        # Monsoon phone
        self.mon.dut = self.ad = self.android_devices[0]
        self.ad.reboot()
        set_adaptive_brightness(self.ad, False)
        set_ambient_display(self.ad, False)
        set_auto_rotate(self.ad, False)
        set_location_service(self.ad, False)
        # This is not needed for AOSP build
        disable_doze(self.ad)
        set_phone_screen_on(self.log, self.ad, 15)

        self.wifi_network_ssid_2g = self.user_params["wifi_network_ssid_2g"]
        self.wifi_network_pass_2g = self.user_params["wifi_network_pass_2g"]
        self.wifi_network_ssid_5g = self.user_params["wifi_network_ssid_5g"]
        self.wifi_network_pass_5g = self.user_params["wifi_network_pass_5g"]

        self.monsoon_log_path = os.path.join(self.log_path, "MonsoonLog")
        create_dir(self.monsoon_log_path)
        return True

    def _save_logs_for_power_test(self, monsoon_result, bug_report):
        if monsoon_result and "monsoon_log_for_power_test" in self.user_params:
            monsoon_result.save_to_text_file(
                [monsoon_result],
                os.path.join(self.monsoon_log_path, self.test_id))
        if bug_report and "bug_report_for_power_test" in self.user_params:
            self.android_devices[0].take_bug_report(self.test_name,
                                                    self.begin_time)

    def _setup_apm(self):
        if not toggle_airplane_mode(self.log, self.ad, True):
            self.log.error("Phone failed to turn on airplane mode.")
            return False
        else:
            self.log.info("Airplane mode is enabled successfully.")
            return True

    def _setup_wifi(self, wifi):
        if wifi == WIFI_5G:
            network_ssid = self.wifi_network_ssid_5g
            network_pass = self.wifi_network_pass_5g
        else:
            network_ssid = self.wifi_network_ssid_2g
            network_pass = self.wifi_network_pass_2g
        if not ensure_wifi_connected(
                self.log, self.ad, network_ssid, network_pass, retry=3):
            self.log.error("Wifi %s connection fails." % wifi)
            return False
        self.log.info("WIFI %s is connected successfully." % wifi)
        return True

    def _setup_wfc(self):
        if not set_wfc_mode(self.log, self.ad, WFC_MODE_WIFI_PREFERRED):
            self.log.error("Phone failed to enable Wifi-Calling.")
            return False
        self.log.info("Phone is set in Wifi-Calling successfully.")
        if not phone_idle_iwlan(self.log, self.ad):
            self.log.error("DUT not in WFC enabled state.")
            return False
        return True

    def _setup_lte_volte_enabled(self):
        if not phone_setup_volte(self.log, self.ad):
            self.log.error("Phone failed to enable VoLTE.")
            return False
        self.log.info("VOLTE is enabled successfully.")
        return True

    def _setup_lte_volte_disabled(self):
        if not phone_setup_csfb(self.log, self.ad):
            self.log.error("Phone failed to setup CSFB.")
            return False
        self.log.info("VOLTE is disabled successfully.")
        return True

    def _setup_3g(self):
        if not phone_setup_voice_3g(self.log, self.ad):
            self.log.error("Phone failed to setup 3g.")
            return False
        self.log.info("RAT 3G is enabled successfully.")
        return True

    def _setup_2g(self):
        if not phone_setup_voice_2g(self.log, self.ad):
            self.log.error("Phone failed to setup 2g.")
            return False
        self.log.info("RAT 2G is enabled successfully.")
        return True

    def _setup_rat(self, rat, volte):
        if rat == RAT_3G:
            return self._setup_3g()
        elif rat == RAT_2G:
            return self._setup_2g()
        elif rat == RAT_LTE and volte:
            return self._setup_lte_volte_enabled()
        elif rat == RAT_LTE and not volte:
            return self._setup_lte_volte_disabled()

    def _start_alarm(self):
        # TODO: This one works with a temporary SL4A API to start alarm.
        # https://googleplex-android-review.git.corp.google.com/#/c/1562684/
        # simulate normal user behavior -- wake up every 1 minutes and do ping
        # (transmit data)
        try:
            alarm_id = self.ad.droid.telephonyStartRecurringAlarm(
                WAKEUP_PING_TEST_NUMBER_OF_ALARM,
                1000 * WAKEUP_PING_TEST_WAKEUP_FREQ, "PING_GOOGLE", None)
        except:
            self.log.error("Failed to setup periodic ping.")
            return False
        if alarm_id is None:
            self.log.error("Start alarm for periodic ping failed.")
            return False
        self.log.info("Set up periodic ping successfully.")
        return True

    def _setup_phone_active_call(self):
        if not call_setup_teardown(
                self.log, self.ad, self.android_devices[1], ad_hangup=None):
            self.log.error("Setup Call failed.")
            return False
        self.log.info("Setup active call successfully.")
        return True

    def _test_setup(self,
                    apm=False,
                    rat=None,
                    volte=False,
                    wifi=None,
                    wfc=False,
                    mobile_data_always_on=False,
                    periodic_ping=False,
                    active_call=False):
        if not ensure_phones_default_state(self.log, self.android_devices):
            self.log.error("Fail to set phones in default state")
            return False
        else:
            self.log.info("Set phones in default state successfully")
        if apm and not self._setup_apm(): return False
        if rat and not self._setup_rat(rat, volte): return False
        if wifi and not self._setup_wifi(wifi): return False
        if wfc and not self._setup_wfc(): return False
        if mobile_data_always_on: set_mobile_data_always_on(self.ad, True)
        if periodic_ping and not self._start_alarm(): return False
        if active_call and not self._setup_phone_active_call(): return False
        self.ad.droid.goToSleepNow()
        return True

    def _power_test(self, phone_check_func_after_power_test=None, **kwargs):
        # Test passing criteria can be defined in test config file with the
        # maximum mA allowed for the test case in "pass_criteria"->test_name
        # field. By default it will set to 999.
        pass_criteria = self._get_pass_criteria(self.test_name)
        bug_report = True
        active_call = kwargs.get('active_call')
        average_current = 0
        result = None
        self.log.info("Test %s: %s" % (self.test_name, kwargs))
        if active_call:
            sample_rate = ACTIVE_CALL_TEST_SAMPLING_RATE
            sample_time = ACTIVE_CALL_TEST_SAMPLE_TIME
            offset_time = ACTIVE_CALL_TEST_OFFSET_TIME
        else:
            sample_rate = IDLE_TEST_SAMPLING_RATE
            sample_time = IDLE_TEST_SAMPLE_TIME
            offset_time = IDLE_TEST_OFFSET_TIME
        try:
            if not self._test_setup(**kwargs):
                self.log.error("DUT Failed to Set Up Properly.")
                return False

            if ((phone_check_func_after_power_test is not None) and
                (not phone_check_func_after_power_test(
                    self.log, self.android_devices[0]))):
                self.log.error(
                    "Phone is not in correct status before power test.")
                return False

            result = self.mon.measure_power(sample_rate, sample_time,
                                            self.test_id, offset_time)
            average_current = result.average_current
            if ((phone_check_func_after_power_test is not None) and
                (not phone_check_func_after_power_test(
                    self.log, self.android_devices[0]))):
                self.log.error(
                    "Phone is not in correct status after power test.")
                return False
            if active_call:
                if not verify_incall_state(self.log, [
                        self.android_devices[0], self.android_devices[1]
                ], True):
                    self.log.error("Call drop during power test.")
                    return False
                else:
                    hangup_call(self.log, self.android_devices[1])
            if (average_current <= pass_criteria):
                bug_report = False
                return True
        finally:
            self._save_logs_for_power_test(result, bug_report)
            self.log.info("{} Result: {} mA, pass criteria: {} mA".format(
                self.test_id, average_current, pass_criteria))

    def _get_pass_criteria(self, test_name):
        """Get the test passing criteria.
           Test passing criteria can be defined in test config file with the
           maximum mA allowed for the test case in "pass_criteria"->test_name
           field. By default it will set to 999.
        """
        try:
            pass_criteria = int(self.user_params["pass_criteria"][test_name])
        except KeyError:
            pass_criteria = DEFAULT_POWER_PASS_CRITERIA
        return pass_criteria

    @TelephonyBaseTest.tel_test_wrap
    def test_power_active_call_volte(self):
        """Power measurement test for active VoLTE call.

        Steps:
        1. DUT idle, in LTE mode, VoLTE enabled.
        2. Make a phone Call from DUT to PhoneB. Answer on PhoneB.
            Make sure DUT is in VoLTE call.
        3. Turn off screen and wait for 3 minutes.
            Then measure power consumption for 5 minutes and get average.

        Expected Results:
        Average power consumption should be within pre-defined limit.

        Returns:
        True if Pass, False if Fail.

        Note: Please calibrate your test environment and baseline pass criteria.
        Pass criteria info should be in test config file.
        """
        return self._power_test(
            rat=RAT_LTE,
            volte=True,
            active_call=True,
            phone_check_func_after_power_test=is_phone_in_call_volte)

    @TelephonyBaseTest.tel_test_wrap
    def test_power_active_call_3g(self):
        """Power measurement test for active CS(3G) call.

        Steps:
        1. DUT idle, in 3G mode.
        2. Make a phone Call from DUT to PhoneB. Answer on PhoneB.
            Make sure DUT is in CS(3G) call.
        3. Turn off screen and wait for 3 minutes.
            Then measure power consumption for 5 minutes and get average.

        Expected Results:
        Average power consumption should be within pre-defined limit.

        Returns:
        True if Pass, False if Fail.

        Note: Please calibrate your test environment and baseline pass criteria.
        Pass criteria info should be in test config file.
        """
        return self._power_test(
            rat=RAT_3G,
            active_call=True,
            phone_check_func_after_power_test=is_phone_in_call_3g)

    @TelephonyBaseTest.tel_test_wrap
    def test_power_active_call_2g(self):
        """Power measurement test for active CS(2G) call.

        Steps:
        1. DUT idle, in 2G mode.
        2. Make a phone Call from DUT to PhoneB. Answer on PhoneB.
            Make sure DUT is in CS(2G) call.
        3. Turn off screen and wait for 3 minutes.
            Then measure power consumption for 5 minutes and get average.

        Expected Results:
        Average power consumption should be within pre-defined limit.

        Returns:
        True if Pass, False if Fail.

        Note: Please calibrate your test environment and baseline pass criteria.
        Pass criteria info should be in test config file.
        """
        return self._power_test(
            rat=RAT_2G,
            active_call=True,
            phone_check_func_after_power_test=is_phone_in_call_2g)

    @TelephonyBaseTest.tel_test_wrap
    def test_power_active_call_wfc_wifi2g_apm(self):
        """Power measurement test for active WiFi call.

        Steps:
        1. DUT idle, in Airplane mode, connect to 2G WiFi,
            WiFi Calling enabled (WiFi-preferred mode).
        2. Make a phone Call from DUT to PhoneB. Answer on PhoneB.
            Make sure DUT is in WFC call.
        3. Turn off screen and wait for 3 minutes.
            Then measure power consumption for 5 minutes and get average.

        Expected Results:
        Average power consumption should be within pre-defined limit.

        Returns:
        True if Pass, False if Fail.

        Note: Please calibrate your test environment and baseline pass criteria.
        Pass criteria info should be in test config file.
        """
        return self._power_test(
            apm=True,
            wifi=WIFI_2G,
            wfc=True,
            active_call=True,
            phone_check_func_after_power_test=is_phone_in_call_iwlan)

    @TelephonyBaseTest.tel_test_wrap
    def test_power_active_call_wfc_wifi2g_lte_volte_enabled(self):
        """Power measurement test for active WiFi call.

        Steps:
        1. DUT idle, LTE cellular data network, VoLTE is On, connect to 2G WiFi,
            WiFi Calling enabled (WiFi-preferred).
        2. Make a phone Call from DUT to PhoneB. Answer on PhoneB.
            Make sure DUT is in WFC call.
        3. Turn off screen and wait for 3 minutes.
            Then measure power consumption for 5 minutes and get average.

        Expected Results:
        Average power consumption should be within pre-defined limit.

        Returns:
        True if Pass, False if Fail.

        Note: Please calibrate your test environment and baseline pass criteria.
        Pass criteria info should be in test config file.
        """
        return self._power_test(
            rat=RAT_LTE,
            volte=True,
            wifi=WIFI_2G,
            wfc=True,
            active_call=True,
            phone_check_func_after_power_test=is_phone_in_call_iwlan)

    @TelephonyBaseTest.tel_test_wrap
    def test_power_active_call_wfc_wifi5g_apm(self):
        """Power measurement test for active WiFi call.

        Steps:
        1. DUT idle, in Airplane mode, connect to 5G WiFi,
            WiFi Calling enabled (WiFi-preferred mode).
        2. Make a phone Call from DUT to PhoneB. Answer on PhoneB.
            Make sure DUT is in WFC call.
        3. Turn off screen and wait for 3 minutes.
            Then measure power consumption for 5 minutes and get average.

        Expected Results:
        Average power consumption should be within pre-defined limit.

        Returns:
        True if Pass, False if Fail.

        Note: Please calibrate your test environment and baseline pass criteria.
        Pass criteria info should be in test config file.
        """
        return self._power_test(
            apm=True,
            wifi=WIFI_5G,
            wfc=True,
            active_call=True,
            phone_check_func_after_power_test=is_phone_in_call_iwlan)

    @TelephonyBaseTest.tel_test_wrap
    def test_power_active_call_wfc_wifi5g_lte_volte_enabled(self):
        """Power measurement test for active WiFi call.

        Steps:
        1. DUT idle, LTE cellular data network, VoLTE is On, connect to 5G WiFi,
            WiFi Calling enabled (WiFi-preferred).
        2. Make a phone Call from DUT to PhoneB. Answer on PhoneB.
            Make sure DUT is in WFC call.
        3. Turn off screen and wait for 3 minutes.
            Then measure power consumption for 5 minutes and get average.

        Expected Results:
        Average power consumption should be within pre-defined limit.

        Returns:
        True if Pass, False if Fail.

        Note: Please calibrate your test environment and baseline pass criteria.
        Pass criteria info should be in test config file.
        """
        return self._power_test(
            rat=RAT_LTE,
            volte=True,
            wifi=WIFI_5G,
            wfc=True,
            active_call=True,
            phone_check_func_after_power_test=is_phone_in_call_iwlan)

    @TelephonyBaseTest.tel_test_wrap
    def test_power_idle_baseline(self):
        """Power measurement test for phone idle baseline.

        Steps:
        1. DUT idle, in Airplane mode. WiFi disabled, WiFi Calling disabled.
        2. Turn off screen and wait for 6 minutes. Then measure power
            consumption for 40 minutes and get average.

        Expected Results:
        Average power consumption should be within pre-defined limit.

        Returns:
        True if Pass, False if Fail.

        Note: Please calibrate your test environment and baseline pass criteria.
        Pass criteria info should be in test config file.
        """
        return self._power_test(apm=True)

    @TelephonyBaseTest.tel_test_wrap
    def test_power_idle_baseline_wifi2g_connected(self):
        """Power measurement test for phone idle baseline (WiFi connected).

        Steps:
        1. DUT idle, in Airplane mode. WiFi connected to 2.4G WiFi,
            WiFi Calling disabled.
        2. Turn off screen and wait for 6 minutes. Then measure power
            consumption for 40 minutes and get average.

        Expected Results:
        Average power consumption should be within pre-defined limit.

        Returns:
        True if Pass, False if Fail.

        Note: Please calibrate your test environment and baseline pass criteria.
        Pass criteria info should be in test config file.
        """
        return self._power_test(apm=True, wifi=WIFI_2G)

    @TelephonyBaseTest.tel_test_wrap
    def test_power_idle_wfc_wifi2g_apm(self):
        """Power measurement test for phone idle WiFi Calling Airplane Mode.

        Steps:
        1. DUT idle, in Airplane mode. Connected to 2G WiFi,
            WiFi Calling enabled (WiFi preferred).
        2. Turn off screen and wait for 6 minutes. Then measure power
            consumption for 40 minutes and get average.

        Expected Results:
        Average power consumption should be within pre-defined limit.

        Returns:
        True if Pass, False if Fail.

        Note: Please calibrate your test environment and baseline pass criteria.
        Pass criteria info should be in test config file.
        """
        return self._power_test(
            apm=True,
            wifi=WIFI_2G,
            wfc=True,
            phone_check_func_after_power_test=phone_idle_iwlan)

    @TelephonyBaseTest.tel_test_wrap
    def test_power_idle_wfc_wifi2g_lte_volte_enabled(self):
        """Power measurement test for phone idle WiFi Calling LTE VoLTE enabled.

        Steps:
        1. DUT idle, in LTE mode, VoLTE enabled. Connected to 2G WiFi,
            WiFi Calling enabled (WiFi preferred).
        2. Turn off screen and wait for 6 minutes. Then measure power
            consumption for 40 minutes and get average.

        Expected Results:
        Average power consumption should be within pre-defined limit.

        Returns:
        True if Pass, False if Fail.

        Note: Please calibrate your test environment and baseline pass criteria.
        Pass criteria info should be in test config file.
        """
        return self._power_test(
            rat=RAT_LTE,
            volte=True,
            wifi=WIFI_2G,
            wfc=True,
            phone_check_func_after_power_test=phone_idle_iwlan)

    @TelephonyBaseTest.tel_test_wrap
    def test_power_idle_lte_volte_enabled(self):
        """Power measurement test for phone idle LTE VoLTE enabled.

        Steps:
        1. DUT idle, in LTE mode, VoLTE enabled. WiFi disabled,
            WiFi Calling disabled.
        2. Turn off screen and wait for 6 minutes. Then measure power
            consumption for 40 minutes and get average.

        Expected Results:
        Average power consumption should be within pre-defined limit.

        Returns:
        True if Pass, False if Fail.

        Note: Please calibrate your test environment and baseline pass criteria.
        Pass criteria info should be in test config file.
        """
        return self._power_test(
            rat=RAT_LTE,
            volte=True,
            phone_check_func_after_power_test=phone_idle_volte)

    @TelephonyBaseTest.tel_test_wrap
    def test_power_idle_lte_volte_enabled_wifi2g(self):
        """Power measurement test for phone idle LTE VoLTE enabled.

        Steps:
        1. DUT idle, in LTE mode, VoLTE enabled. WiFi enabled,
            WiFi Calling disabled.
        2. Turn off screen and wait for 6 minutes. Then measure power
            consumption for 40 minutes and get average.

        Expected Results:
        Average power consumption should be within pre-defined limit.

        Returns:
        True if Pass, False if Fail.

        Note: Please calibrate your test environment and baseline pass criteria.
        Pass criteria info should be in test config file.
        """
        return self._power_test(
            rat=RAT_LTE,
            volte=True,
            wifi=WIFI_2G,
            phone_check_func_after_power_test=phone_idle_volte)

    @TelephonyBaseTest.tel_test_wrap
    def test_power_idle_lte_volte_disabled(self):
        """Power measurement test for phone idle LTE VoLTE disabled.

        Steps:
        1. DUT idle, in LTE mode, VoLTE disabled. WiFi disabled,
            WiFi Calling disabled.
        2. Turn off screen and wait for 6 minutes. Then measure power
            consumption for 40 minutes and get average.

        Expected Results:
        Average power consumption should be within pre-defined limit.

        Returns:
        True if Pass, False if Fail.

        Note: Please calibrate your test environment and baseline pass criteria.
        Pass criteria info should be in test config file.
        """
        return self._power_test(rat=RAT_LTE)

    @TelephonyBaseTest.tel_test_wrap
    def test_power_idle_3g(self):
        """Power measurement test for phone idle 3G.

        Steps:
        1. DUT idle, in 3G mode. WiFi disabled, WiFi Calling disabled.
        2. Turn off screen and wait for 6 minutes. Then measure power
            consumption for 40 minutes and get average.

        Expected Results:
        Average power consumption should be within pre-defined limit.

        Returns:
        True if Pass, False if Fail.

        Note: Please calibrate your test environment and baseline pass criteria.
        Pass criteria info should be in test config file.
        """
        return self._power_test(
            rat=RAT_3G, phone_check_func_after_power_test=phone_idle_3g)

    @TelephonyBaseTest.tel_test_wrap
    def test_power_idle_3g_wifi2g(self):
        """Power measurement test for phone idle 3G.

        Steps:
        1. DUT idle, in 3G mode. WiFi enabled, WiFi Calling disabled.
        2. Turn off screen and wait for 6 minutes. Then measure power
            consumption for 40 minutes and get average.

        Expected Results:
        Average power consumption should be within pre-defined limit.

        Returns:
        True if Pass, False if Fail.

        Note: Please calibrate your test environment and baseline pass criteria.
        Pass criteria info should be in test config file.
        """
        return self._power_test(
            rat=RAT_3G,
            wifi=WIFI_2G,
            phone_check_func_after_power_test=phone_idle_3g)

    @TelephonyBaseTest.tel_test_wrap
    def test_power_idle_2g(self):
        """Power measurement test for phone idle 3G.

        Steps:
        1. DUT idle, in 2G mode. WiFi disabled, WiFi Calling disabled.
        2. Turn off screen and wait for 6 minutes. Then measure power
            consumption for 40 minutes and get average.

        Expected Results:
        Average power consumption should be within pre-defined limit.

        Returns:
        True if Pass, False if Fail.

        Note: Please calibrate your test environment and baseline pass criteria.
        Pass criteria info should be in test config file.
        """
        return self._power_test(
            rat=RAT_2G, phone_check_func_after_power_test=phone_idle_2g)

    @TelephonyBaseTest.tel_test_wrap
    def test_power_idle_2g_wifi2g(self):
        """Power measurement test for phone idle 3G.

        Steps:
        1. DUT idle, in 2G mode. WiFi disabled, WiFi Calling disabled.
        2. Turn off screen and wait for 6 minutes. Then measure power
            consumption for 40 minutes and get average.

        Expected Results:
        Average power consumption should be within pre-defined limit.

        Returns:
        True if Pass, False if Fail.

        Note: Please calibrate your test environment and baseline pass criteria.
        Pass criteria info should be in test config file.
        """
        return self._power_test(
            rat=RAT_2G,
            wifi=WIFI_2G,
            phone_check_func_after_power_test=phone_idle_2g)

    # TODO: This one is not working right now. Requires SL4A API to start alarm.
    @TelephonyBaseTest.tel_test_wrap
    def test_power_idle_lte_volte_enabled_wakeup_ping(self):
        """Power measurement test for phone LTE VoLTE enabled Wakeup Ping every
        1 minute.

        Steps:
        1. DUT idle, in LTE mode, VoLTE enabled. WiFi disabled,
            WiFi Calling disabled.
        2. Start script to wake up AP every 1 minute, after wakeup,
            DUT send http Request to Google.com then go to sleep.
        3. Turn off screen and wait for 6 minutes. Then measure power
            consumption for 40 minutes and get average.

        Expected Results:
        Average power consumption should be within pre-defined limit.

        Returns:
        True if Pass, False if Fail.

        Note: Please calibrate your test environment and baseline pass criteria.
        Pass criteria info should be in test config file.
        """
        return self._power_test(
            rat=RAT_LTE,
            volte=True,
            periodic_ping=True,
            phone_check_func_after_power_test=phone_idle_iwlan)

    # TODO: This one is not working right now. Requires SL4A API to start alarm.
    @TelephonyBaseTest.tel_test_wrap
    def test_power_idle_lte_volte_disabled_wakeup_ping(self):
        """Power measurement test for phone LTE VoLTE disabled Wakeup Ping every
        1 minute.

        Steps:
        1. DUT idle, in LTE mode, VoLTE disabled. WiFi disabled,
            WiFi Calling disabled.
        2. Start script to wake up AP every 1 minute, after wakeup,
            DUT send http Request to Google.com then go to sleep.
        3. Turn off screen and wait for 6 minutes. Then measure power
            consumption for 40 minutes and get average.

        Expected Results:
        Average power consumption should be within pre-defined limit.

        Returns:
        True if Pass, False if Fail.

        Note: Please calibrate your test environment and baseline pass criteria.
        Pass criteria info should be in test config file.
        """
        return self._power_test(rat=RAT_LTE, periodic_ping=True)

    # TODO: This one is not working right now. Requires SL4A API to start alarm.
    @TelephonyBaseTest.tel_test_wrap
    def test_power_idle_3g_wakeup_ping(self):
        """Power measurement test for phone 3G Wakeup Ping every 1 minute.

        Steps:
        1. DUT idle, in 3G mode. WiFi disabled, WiFi Calling disabled.
        2. Start script to wake up AP every 1 minute, after wakeup,
            DUT send http Request to Google.com then go to sleep.
        3. Turn off screen and wait for 6 minutes. Then measure power
            consumption for 40 minutes and get average.

        Expected Results:
        Average power consumption should be within pre-defined limit.

        Returns:
        True if Pass, False if Fail.

        Note: Please calibrate your test environment and baseline pass criteria.
        Pass criteria info should be in test config file.
        """
        return self._power_test(
            rat=RAT_3G,
            periodic_ping=True,
            phone_check_func_after_power_test=phone_idle_3g)

    @TelephonyBaseTest.tel_test_wrap
    def test_power_idle_lte_volte_enabled_mobile_data_always_on(self):
        """Power measurement test for phone idle LTE VoLTE enabled.

        Steps:
        1. DUT idle, in LTE mode, VoLTE enabled. WiFi disabled,
            WiFi Calling disabled.
        2. Turn on mobile data always on
        3. Turn off screen and wait for 6 minutes. Then measure power
            consumption for 40 minutes and get average.

        Expected Results:
        Average power consumption should be within pre-defined limit.

        Returns:
        True if Pass, False if Fail.

        Note: Please calibrate your test environment and baseline pass criteria.
        Pass criteria info should be in test config file.
        """
        return self._power_test(
            rat=RAT_LTE,
            volte=True,
            mobile_data_always_on=True,
            phone_check_func_after_power_test=phone_idle_volte)

    @TelephonyBaseTest.tel_test_wrap
    def test_power_idle_lte_volte_enabled_wifi2g_mobile_data_always_on(self):
        """Power measurement test for phone idle LTE VoLTE enabled.

        Steps:
        1. DUT idle, in LTE mode, VoLTE enabled. WiFi enabled,
            WiFi Calling disabled.
        2. Turn on mobile data always on
        3. Turn off screen and wait for 6 minutes. Then measure power
            consumption for 40 minutes and get average.

        Expected Results:
        Average power consumption should be within pre-defined limit.

        Returns:
        True if Pass, False if Fail.

        Note: Please calibrate your test environment and baseline pass criteria.
        Pass criteria info should be in test config file.
        """
        return self._power_test(
            rat=RAT_LTE,
            volte=True,
            wifi=WIFI_2G,
            mobile_data_always_on=True,
            phone_check_func_after_power_test=phone_idle_volte)

    @TelephonyBaseTest.tel_test_wrap
    def test_power_idle_lte_volte_disabled_mobile_data_always_on(self):
        """Power measurement test for phone idle LTE VoLTE disabled.

        Steps:
        1. DUT idle, in LTE mode, VoLTE disabled. WiFi disabled,
            WiFi Calling disabled.
        2. Turn on mobile data always on
        3. Turn off screen and wait for 6 minutes. Then measure power
            consumption for 40 minutes and get average.

        Expected Results:
        Average power consumption should be within pre-defined limit.

        Returns:
        True if Pass, False if Fail.

        Note: Please calibrate your test environment and baseline pass criteria.
        Pass criteria info should be in test config file.
        """
        return self._power_test(rat=RAT_LTE, mobile_data_always_on=True)

    @TelephonyBaseTest.tel_test_wrap
    def test_power_idle_3g_mobile_data_always_on(self):
        """Power measurement test for phone idle 3G.

        Steps:
        1. DUT idle, in 3G mode. WiFi disabled, WiFi Calling disabled.
        2. Turn on mobile data always on
        3. Turn off screen and wait for 6 minutes. Then measure power
            consumption for 40 minutes and get average.

        Expected Results:
        Average power consumption should be within pre-defined limit.

        Returns:
        True if Pass, False if Fail.

        Note: Please calibrate your test environment and baseline pass criteria.
        Pass criteria info should be in test config file.
        """
        return self._power_test(
            rat=RAT_3G,
            mobile_data_always_on=True,
            phone_check_func_after_power_test=phone_idle_3g)

    @TelephonyBaseTest.tel_test_wrap
    def test_power_idle_3g_wifi2g_mobile_data_always_on(self):
        """Power measurement test for phone idle 3G.

        Steps:
        1. DUT idle, in 3G mode. WiFi enabled, WiFi Calling disabled.
        2. Turn on mobile data always on
        3. Turn off screen and wait for 6 minutes. Then measure power
            consumption for 40 minutes and get average.

        Expected Results:
        Average power consumption should be within pre-defined limit.

        Returns:
        True if Pass, False if Fail.

        Note: Please calibrate your test environment and baseline pass criteria.
        Pass criteria info should be in test config file.
        """
        return self._power_test(
            rat=RAT_3G,
            wifi=WIFI_2G,
            mobile_data_always_on=True,
            phone_check_func_after_power_test=phone_idle_3g)

    @TelephonyBaseTest.tel_test_wrap
    def test_power_idle_2g_mobile_data_always_on(self):
        """Power measurement test for phone idle 3G.

        Steps:
        1. DUT idle, in 2G mode. WiFi disabled, WiFi Calling disabled.
        2. Turn on mobile data always on
        3. Turn off screen and wait for 6 minutes. Then measure power
            consumption for 40 minutes and get average.

        Expected Results:
        Average power consumption should be within pre-defined limit.

        Returns:
        True if Pass, False if Fail.

        Note: Please calibrate your test environment and baseline pass criteria.
        Pass criteria info should be in test config file.
        """
        return self._power_test(
            rat=RAT_2G,
            mobile_data_always_on=True,
            phone_check_func_after_power_test=phone_idle_2g)

    @TelephonyBaseTest.tel_test_wrap
    def test_power_idle_2g_wifi2g_mobile_data_always_on(self):
        """Power measurement test for phone idle 3G.

        Steps:
        1. DUT idle, in 2G mode. WiFi enabled, WiFi Calling disabled.
        2. Turn on mobile data always on
        3. Turn off screen and wait for 6 minutes. Then measure power
            consumption for 40 minutes and get average.

        Expected Results:
        Average power consumption should be within pre-defined limit.

        Returns:
        True if Pass, False if Fail.

        Note: Please calibrate your test environment and baseline pass criteria.
        Pass criteria info should be in test config file.
        """
        return self._power_test(
            rat=RAT_2G,
            wifi=WIFI_2G,
            mobile_data_always_on=True,
            phone_check_func_after_power_test=phone_idle_2g)

    # TODO: This one is not working right now. Requires SL4A API to start alarm.
    @TelephonyBaseTest.tel_test_wrap
    def test_power_idle_lte_volte_enabled_wakeup_ping_mobile_data_always_on(
            self):
        """Power measurement test for phone LTE VoLTE enabled Wakeup Ping every
        1 minute.

        Steps:
        1. DUT idle, in LTE mode, VoLTE enabled. WiFi disabled,
            WiFi Calling disabled.
        2. Start script to wake up AP every 1 minute, after wakeup,
            DUT send http Request to Google.com then go to sleep.
        3. Turn off screen and wait for 6 minutes. Then measure power
            consumption for 40 minutes and get average.

        Expected Results:
        Average power consumption should be within pre-defined limit.

        Returns:
        True if Pass, False if Fail.

        Note: Please calibrate your test environment and baseline pass criteria.
        Pass criteria info should be in test config file.
        """
        return self._power_test(
            rat=RAT_LTE,
            volte=True,
            mobile_data_always_on=True,
            periodic_ping=True,
            phone_check_func_after_power_test=phone_idle_volte)

    # TODO: This one is not working right now. Requires SL4A API to start alarm.
    @TelephonyBaseTest.tel_test_wrap
    def test_power_idle_lte_volte_enabled_wifi2g_wakeup_ping_mobile_data_always_on(
            self):
        """Power measurement test for phone LTE VoLTE enabled Wakeup Ping every
        1 minute.

        Steps:
        1. DUT idle, in LTE mode, VoLTE enabled. WiFi enabled,
            WiFi Calling disabled.
        2. Start script to wake up AP every 1 minute, after wakeup,
            DUT send http Request to Google.com then go to sleep.
        3. Turn off screen and wait for 6 minutes. Then measure power
            consumption for 40 minutes and get average.

        Expected Results:
        Average power consumption should be within pre-defined limit.

        Returns:
        True if Pass, False if Fail.

        Note: Please calibrate your test environment and baseline pass criteria.
        Pass criteria info should be in test config file.
        """
        return self._power_test(
            rat=RAT_LTE,
            volte=True,
            wifi=WIFI_2G,
            mobile_data_always_on=True,
            periodic_ping=True,
            phone_check_func_after_power_test=phone_idle_volte)

    # TODO: This one is not working right now. Requires SL4A API to start alarm.
    @TelephonyBaseTest.tel_test_wrap
    def test_power_idle_lte_volte_disabled_wakeup_ping_mobile_data_always_on(
            self):
        """Power measurement test for phone LTE VoLTE disabled Wakeup Ping every
        1 minute.

        Steps:
        1. DUT idle, in LTE mode, VoLTE disabled. WiFi disabled,
            WiFi Calling disabled.
        2. Start script to wake up AP every 1 minute, after wakeup,
            DUT send http Request to Google.com then go to sleep.
        3. Turn off screen and wait for 6 minutes. Then measure power
            consumption for 40 minutes and get average.

        Expected Results:
        Average power consumption should be within pre-defined limit.

        Returns:
        True if Pass, False if Fail.

        Note: Please calibrate your test environment and baseline pass criteria.
        Pass criteria info should be in test config file.
        """
        return self._power_test(
            rat=RAT_LTE, mobile_data_always_on=True, periodic_ping=True)

    # TODO: This one is not working right now. Requires SL4A API to start alarm.
    @TelephonyBaseTest.tel_test_wrap
    def test_power_idle_3g_wakeup_ping_mobile_data_always_on(self):
        """Power measurement test for phone 3G Wakeup Ping every 1 minute.

        Steps:
        1. DUT idle, in 3G mode. WiFi disabled, WiFi Calling disabled.
        2. Start script to wake up AP every 1 minute, after wakeup,
            DUT send http Request to Google.com then go to sleep.
        3. Turn off screen and wait for 6 minutes. Then measure power
            consumption for 40 minutes and get average.

        Expected Results:
        Average power consumption should be within pre-defined limit.

        Returns:
        True if Pass, False if Fail.

        Note: Please calibrate your test environment and baseline pass criteria.
        Pass criteria info should be in test config file.
        """
        return self._power_test(
            rat=RAT_3G,
            mobile_data_always_on=True,
            periodic_ping=True,
            phone_check_func_after_power_test=phone_idle_3g)

    # TODO: This one is not working right now. Requires SL4A API to start alarm.
    @TelephonyBaseTest.tel_test_wrap
    def test_power_idle_3g_wifi2g_wakeup_ping_mobile_data_always_on(self):
        """Power measurement test for phone 3G Wakeup Ping every 1 minute.

        Steps:
        1. DUT idle, in 3G mode. WiFi enabled, WiFi Calling disabled.
        2. Start script to wake up AP every 1 minute, after wakeup,
            DUT send http Request to Google.com then go to sleep.
        3. Turn off screen and wait for 6 minutes. Then measure power
            consumption for 40 minutes and get average.

        Expected Results:
        Average power consumption should be within pre-defined limit.

        Returns:
        True if Pass, False if Fail.

        Note: Please calibrate your test environment and baseline pass criteria.
        Pass criteria info should be in test config file.
        """
        return self._power_test(
            rat=RAT_3G,
            wifi=WIFI_2G,
            mobile_data_always_on=True,
            periodic_ping=True,
            phone_check_func_after_power_test=phone_idle_3g)

    # TODO: This one is not working right now. Requires SL4A API to start alarm.
    @TelephonyBaseTest.tel_test_wrap
    def test_power_idle_2g_wakeup_ping_mobile_data_always_on(self):
        """Power measurement test for phone 3G Wakeup Ping every 1 minute.

        Steps:
        1. DUT idle, in 2G mode. WiFi disabled, WiFi Calling disabled.
        2. Start script to wake up AP every 1 minute, after wakeup,
            DUT send http Request to Google.com then go to sleep.
        3. Turn off screen and wait for 6 minutes. Then measure power
            consumption for 40 minutes and get average.

        Expected Results:
        Average power consumption should be within pre-defined limit.

        Returns:
        True if Pass, False if Fail.

        Note: Please calibrate your test environment and baseline pass criteria.
        Pass criteria info should be in test config file.
        """
        return self._power_test(
            rat=RAT_2G,
            mobile_data_always_on=True,
            periodic_ping=True,
            phone_check_func_after_power_test=phone_idle_2g)

    # TODO: This one is not working right now. Requires SL4A API to start alarm.
    @TelephonyBaseTest.tel_test_wrap
    def test_power_idle_2g_wifi2g_wakeup_ping_mobile_data_always_on(self):
        """Power measurement test for phone 3G Wakeup Ping every 1 minute.

        Steps:
        1. DUT idle, in 2G mode. WiFi enabled, WiFi Calling disabled.
        2. Start script to wake up AP every 1 minute, after wakeup,
            DUT send http Request to Google.com then go to sleep.
        3. Turn off screen and wait for 6 minutes. Then measure power
            consumption for 40 minutes and get average.

        Expected Results:
        Average power consumption should be within pre-defined limit.

        Returns:
        True if Pass, False if Fail.

        Note: Please calibrate your test environment and baseline pass criteria.
        Pass criteria info should be in test config file.
        """
        return self._power_test(
            rat=RAT_2G,
            wifi=WIFI_2G,
            mobile_data_always_on=True,
            periodic_ping=True,
            phone_check_func_after_power_test=phone_idle_2g)

