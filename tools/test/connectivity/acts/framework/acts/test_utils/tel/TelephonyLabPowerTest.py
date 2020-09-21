#/usr/bin/env python3.4
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
"""
Sanity tests for voice tests in telephony
"""
import time, os

from acts.controllers.anritsu_lib._anritsu_utils import AnritsuError
from acts.controllers.anritsu_lib.md8475a import MD8475A
from acts.controllers.anritsu_lib.md8475a import BtsBandwidth
from acts.test_utils.tel.anritsu_utils import set_system_model_lte
from acts.test_utils.tel.anritsu_utils import set_usim_parameters
from acts.test_utils.tel.tel_defines import RAT_FAMILY_LTE
from acts.test_utils.tel.tel_defines import NETWORK_MODE_LTE_CDMA_EVDO
from acts.test_utils.tel.tel_defines import NETWORK_MODE_LTE_CDMA_EVDO_GSM_WCDMA
from acts.test_utils.tel.tel_test_utils import ensure_network_rat
from acts.test_utils.tel.tel_test_utils import set_phone_screen_on
from acts.test_utils.tel.tel_test_utils import toggle_volte
from acts.test_utils.tel.tel_voice_utils import phone_idle_volte
from acts.test_utils.tel.tel_voice_utils import phone_setup_volte
from acts.test_utils.tel.TelephonyBaseTest import TelephonyBaseTest
from acts.utils import create_dir
from acts.utils import disable_doze
from acts.utils import set_adaptive_brightness
from acts.utils import set_ambient_display
from acts.utils import set_auto_rotate
from acts.utils import set_location_service

DEFAULT_CALL_NUMBER = "+11234567891"

# Monsoon output Voltage in V
MONSOON_OUTPUT_VOLTAGE = 4.2
# Monsoon output max current in A
MONSOON_MAX_CURRENT = 7.8

# Sampling rate in Hz
ACTIVE_CALL_TEST_SAMPLING_RATE = 100
# Sample duration in seconds
ACTIVE_CALL_TEST_SAMPLE_TIME = 10
# Offset time in seconds
ACTIVE_CALL_TEST_OFFSET_TIME = 10


class TelephonyLabPowerTest(TelephonyBaseTest):
    def __init__(self, controllers):
        TelephonyBaseTest.__init__(self, controllers)
        self.ad = self.android_devices[0]
        self.ad.sim_card = getattr(self.ad, "sim_card", None)
        self.md8475a_ip_address = self.user_params[
            "anritsu_md8475a_ip_address"]
        self.wlan_option = self.user_params.get("anritsu_wlan_option", False)

    def _configure_dut(self):
        try:
            self.log.info("Rebooting DUT")
            self.ad.reboot()
            self.log.info("DUT rebooted")
            set_adaptive_brightness(self.ad, False)
            set_ambient_display(self.ad, False)
            set_auto_rotate(self.ad, False)
            set_location_service(self.ad, False)
            # This is not needed for AOSP build
            disable_doze(self.ad)
            set_phone_screen_on(self.log, self.ad, 15)
            self.ad.droid.telephonyFactoryReset()
        except Exception as e:
            self.ad.log.error(e)
            return False
        return True

    def _configure_dut_network_mode_for_data_volte(self):
        self._configure_dut()
        try:
            # TODO do what is needed to verify connected for LTE data transfer
            self.log.info("setting back to LTE")
            self.ad.droid.telephonySetPreferredNetworkTypesForSubscription(
                "NETWORK_MODE_LTE_CDMA_EVDO",
                self.ad.droid.subscriptionGetDefaultSubId())
            self.ad.adb.shell(
                "setprop net.lte.ims.volte.provisioned 1", ignore_status=True)
        except Exception as e:
            self.ad.log.error(e)
            return False
        return True

    def _configure_simulation(self):
        try:
            self.anritsu = MD8475A(self.md8475a_ip_address, self.log,
                                   self.wlan_option)
            [lte_bts] = set_system_model_lte(self.anritsu, self.user_params,
                                             self.ad.sim_card)
            self.bts = lte_bts
            lte_bts.bandwidth = BtsBandwidth.LTE_BANDWIDTH_10MHz
            set_usim_parameters(self.anritsu, self.ad.sim_card)
            self.anritsu.start_simulation()
            self.anritsu.send_command("IMSSTARTVN 1")
        except AnritsuError:
            self.log.error("Error in connecting to Anritsu Simulator")
            return False
        return True

    def _dut_setup_data_volte(self, ad):
        ad.droid.telephonyToggleDataConnection(True)
        toggle_volte(self.log, ad, True)
        return ensure_network_rat(
            self.log,
            ad,
            NETWORK_MODE_LTE_CDMA_EVDO_GSM_WCDMA,
            RAT_FAMILY_LTE,
            toggle_apm_after_setting=True)

    def setup_class(self):
        # Monsoon setup
        self.log.info("Starting Monsoon setup")
        self.mon = self.monsoons[0]
        self.mon.set_voltage(MONSOON_OUTPUT_VOLTAGE)
        self.mon.set_max_current(MONSOON_MAX_CURRENT)
        self.mon.dut = self.ad = self.android_devices[0]
        self.monsoon_log_path = os.path.join(self.log_path, "MonsoonLog")
        create_dir(self.monsoon_log_path)
        self.log.info("Conffiguring MD8475A network simulator")
        self._configure_simulation()
        self.log.info("Setting DUT's network mode for data and volte")
        self._configure_dut_network_mode_for_data_volte()
        self.log.info("Enabling DUT for data and VoLTE")
        if not self._dut_setup_data_volte(self.ad):
            self.log.error("phone_setup_volte failed.")
        self.log.info("Waiting for DUT to register on MD8475A")
        self.anritsu.wait_for_registration_state()
        self.log.info("Waiting for DUT to register with IMS server")
        if not phone_idle_volte(self.log, self.ad):
            self.log.error("phone_idle_volte failed.")

    def setup_test(self):
        self.log.info("Bypassing empty setup_test() in TelephonyLabPowerTest")

    def teardown_class(self):
        self.log.info("Stopping Simulation and disconnect MD8475A")
        self.anritsu.stop_simulation()
        self.anritsu.disconnect()
        return True

    def _save_logs_for_power_test(self, monsoon_result, bug_report):
        if monsoon_result and "monsoon_log_for_power_test" in self.user_params:
            monsoon_result.save_to_text_file(
                [monsoon_result],
                os.path.join(self.monsoon_log_path, self.test_id))
        if bug_report and "bug_report_for_power_test" in self.user_params:
            self.android_devices[0].take_bug_report(self.test_name,
                                                    self.begin_time)

    def power_test(self,
                   olvl,
                   rflvl,
                   sch_mode="DYNAMIC",
                   sample_rate=ACTIVE_CALL_TEST_SAMPLING_RATE,
                   sample_time=ACTIVE_CALL_TEST_SAMPLE_TIME,
                   offset_time=ACTIVE_CALL_TEST_OFFSET_TIME):
        """ Set Output(DL)/InputDL(UL) power and scheduling mode of BTS,
            and samping parameters of Monsoon
            Args: ovlv: Output (DL) level in dBm
                  rflvl: Input (UL) level in dBm
                  sch_mode: Scheduling mode, either "STATIC" or "DYNAMIC"
                  sample_rate: Sampling rate in Hz
                  sample_time: Sample duration in seconds
                  offset_time: Offset time in seconds
            Return: True if no exception
        """
        self.bts.output_level = olvl
        self.bts.input_level = rflvl
        self.bts.lte_scheduling_mode = sch_mode
        bug_report = True
        average_current = 0
        result = None
        self.log.info("Test %s" % self.test_name)
        try:
            result = self.mon.measure_power(sample_rate, sample_time,
                                            self.test_id, offset_time)
            average_current = result.average_current
            self._save_logs_for_power_test(result, bug_report)
            self.log.info("{} Result: {} mA".format(self.test_id,
                                                    average_current))
        except Exception as e:
            self.log.error("Exception during power consumption measurement: " +
                           str(e))
            return False
        return True
