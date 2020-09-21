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
import time

from acts.test_decorators import test_tracker_info
from acts.controllers.anritsu_lib._anritsu_utils import AnritsuError
from acts.controllers.anritsu_lib.md8475a import MD8475A
from acts.controllers.anritsu_lib.md8475a import BtsNumber
from acts.test_utils.tel.anritsu_utils import WAIT_TIME_ANRITSU_REG_AND_CALL
from acts.test_utils.tel.anritsu_utils import handover_tc
from acts.test_utils.tel.anritsu_utils import make_ims_call
from acts.test_utils.tel.anritsu_utils import tear_down_call
from acts.test_utils.tel.anritsu_utils import set_system_model_lte_lte
from acts.test_utils.tel.anritsu_utils import set_system_model_lte_wcdma
from acts.test_utils.tel.anritsu_utils import set_system_model_lte_gsm
from acts.test_utils.tel.anritsu_utils import set_system_model_lte_1x
from acts.test_utils.tel.anritsu_utils import set_system_model_lte_evdo
from acts.test_utils.tel.anritsu_utils import set_usim_parameters
from acts.test_utils.tel.anritsu_utils import set_post_sim_params
from acts.test_utils.tel.tel_defines import CALL_TEARDOWN_PHONE
from acts.test_utils.tel.tel_defines import RAT_FAMILY_CDMA2000
from acts.test_utils.tel.tel_defines import RAT_FAMILY_GSM
from acts.test_utils.tel.tel_defines import RAT_FAMILY_LTE
from acts.test_utils.tel.tel_defines import RAT_FAMILY_UMTS
from acts.test_utils.tel.tel_defines import RAT_1XRTT
from acts.test_utils.tel.tel_defines import NETWORK_MODE_CDMA
from acts.test_utils.tel.tel_defines import NETWORK_MODE_GSM_ONLY
from acts.test_utils.tel.tel_defines import NETWORK_MODE_GSM_UMTS
from acts.test_utils.tel.tel_defines import NETWORK_MODE_LTE_CDMA_EVDO
from acts.test_utils.tel.tel_defines import NETWORK_MODE_LTE_CDMA_EVDO_GSM_WCDMA
from acts.test_utils.tel.tel_defines import NETWORK_MODE_LTE_GSM_WCDMA
from acts.test_utils.tel.tel_defines import WAIT_TIME_IN_CALL
from acts.test_utils.tel.tel_defines import WAIT_TIME_IN_CALL_FOR_IMS
from acts.test_utils.tel.tel_test_utils import ensure_network_rat
from acts.test_utils.tel.tel_test_utils import ensure_phones_idle
from acts.test_utils.tel.tel_test_utils import toggle_airplane_mode_by_adb
from acts.test_utils.tel.tel_test_utils import toggle_volte
from acts.test_utils.tel.tel_test_utils import run_multithread_func
from acts.test_utils.tel.tel_test_utils import iperf_test_by_adb
from acts.test_utils.tel.tel_test_utils import set_preferred_apn_by_adb
from acts.test_utils.tel.tel_test_utils import start_qxdm_loggers
from acts.test_utils.tel.tel_voice_utils import phone_idle_volte
from acts.test_utils.tel.TelephonyBaseTest import TelephonyBaseTest
from acts.utils import adb_shell_ping
from acts.utils import rand_ascii_str
from acts.controllers import iperf_server
from acts.utils import exe_cmd

DEFAULT_CALL_NUMBER = "+11234567891"
DEFAULT_PING_DURATION = 5
WAITTIME_BEFORE_HANDOVER = 20
WAITTIME_AFTER_HANDOVER = 20


class TelLabMobilityTest(TelephonyBaseTest):
    def __init__(self, controllers):
        TelephonyBaseTest.__init__(self, controllers)
        self.ad = self.android_devices[0]
        self.ad.sim_card = getattr(self.ad, "sim_card", None)
        self.md8475a_ip_address = self.user_params[
            "anritsu_md8475a_ip_address"]
        self.wlan_option = self.user_params.get("anritsu_wlan_option", False)
        self.voice_call_number = self.user_params.get('voice_call_number',
                                                      DEFAULT_CALL_NUMBER)
        self.ip_server = self.iperf_servers[0]
        self.port_num = self.ip_server.port
        self.log.info("Iperf Port is %s", self.port_num)
        if self.ad.sim_card == "VzW12349":
            set_preferred_apn_by_adb(self.ad, "VZWINTERNET")

    def setup_class(self):
        try:
            self.anritsu = MD8475A(self.md8475a_ip_address, self.log,
                                   self.wlan_option)
        except AnritsuError:
            self.log.error("Error in connecting to Anritsu Simulator")
            return False
        return True

    def setup_test(self):
        try:
            if getattr(self, "qxdm_log", True):
                start_qxdm_loggers(self.log, self.android_devices)
            self.ad.droid.telephonyFactoryReset()
        except Exception as e:
            self.ad.log.error(e)
        toggle_airplane_mode_by_adb(self.log, self.ad, True)
        self.ad.adb.shell(
            "setprop net.lte.ims.volte.provisioned 1", ignore_status=True)
        # get a handle to virtual phone
        self.virtualPhoneHandle = self.anritsu.get_VirtualPhone()
        return True

    def teardown_test(self):
        self.log.info("Stopping Simulation")
        self.anritsu.stop_simulation()
        toggle_airplane_mode_by_adb(self.log, self.ad, True)
        return True

    def teardown_class(self):
        self.anritsu.disconnect()
        return True

    def active_handover(self,
                        set_simulation_func,
                        phone_setup_func,
                        phone_idle_func_after_registration=None,
                        volte=True,
                        iperf=True,
                        all_bands=False,
                        is_wait_for_registration=True,
                        voice_number=DEFAULT_CALL_NUMBER,
                        teardown_side=CALL_TEARDOWN_PHONE,
                        wait_time_in_call=WAIT_TIME_IN_CALL):
        try:
            bts = set_simulation_func(self.anritsu, self.user_params,
                                      self.ad.sim_card)
            set_usim_parameters(self.anritsu, self.ad.sim_card)
            set_post_sim_params(self.anritsu, self.user_params,
                                self.ad.sim_card)
            self.anritsu.start_simulation()
            self.anritsu.send_command("IMSSTARTVN 1")
            self.anritsu.send_command("IMSSTARTVN 2")
            self.anritsu.send_command("IMSSTARTVN 3")
            # turn off all other BTS to ensure UE registers on BTS1
            simmodel = self.anritsu.get_simulation_model().split(',')
            no_of_bts = len(simmodel)
            for i in range(2, no_of_bts + 1):
                self.anritsu.send_command("OUTOFSERVICE OUT,BTS{}".format(i))
            if phone_setup_func is not None:
                if not phone_setup_func(self.ad):
                    self.log.warning("phone_setup_func failed. Rebooting UE")
                    self.ad.reboot()
                    time.sleep(30)
                    if self.ad.sim_card == "VzW12349":
                        set_preferred_apn_by_adb(self.ad, "VZWINTERNET")
                    if not phone_setup_func(self.ad):
                        self.log.error("phone_setup_func failed.")
            if is_wait_for_registration:
                self.anritsu.wait_for_registration_state()
            if phone_idle_func_after_registration:
                if not phone_idle_func_after_registration(self.log, self.ad):
                    self.log.error("phone_idle_func failed.")
            for i in range(2, no_of_bts + 1):
                self.anritsu.send_command("OUTOFSERVICE IN,BTS{}".format(i))
            time.sleep(WAIT_TIME_ANRITSU_REG_AND_CALL)
            if iperf:  # setup iPerf server
                server_ip = self.iperf_setup()
                if not server_ip:
                    self.log.error("iperf server can not be reached by ping")
                    return False
            if volte:  # make a VoLTE MO call
                if not make_ims_call(self.log, self.ad, self.anritsu,
                                     voice_number):
                    self.log.error("Phone {} Failed to make volte call to {}"
                                   .format(self.ad.serial, voice_number))
                    return False
            if all_bands and (simmodel[1] == "WCDMA"):
                band = []
                for rat in simmodel[:2]:
                    band.append(self.anritsu.get_supported_bands(rat))
                self.log.info("UE reported LTE bands are {}".format(band[0]))
                self.log.info("UE reported WCDMA bands are {}".format(band[1]))
                current_lte_band = bts[0].band
                # move current LTE band to the last in the list
                band[0].remove(current_lte_band)
                band[0].append(current_lte_band)
                n = max(len(band[0]), len(band[1]))
            else:
                n = 1  # n is the number of LTE->WCDMA->LTE handovers

            for i in range(n):
                if all_bands:
                    bts[1].band = band[1][i % len(band[1])]
                if not iperf:  # VoLTE only
                    result = handover_tc(self.log, self.anritsu,
                                         WAITTIME_BEFORE_HANDOVER,
                                         BtsNumber.BTS1, BtsNumber.BTS2)
                    time.sleep(WAITTIME_AFTER_HANDOVER)
                else:  # with iPerf
                    iperf_task = (self._iperf_task,
                                  (server_ip, WAITTIME_BEFORE_HANDOVER +
                                   WAITTIME_AFTER_HANDOVER - 10))
                    ho_task = (handover_tc, (self.log, self.anritsu,
                                             WAITTIME_BEFORE_HANDOVER,
                                             BtsNumber.BTS1, BtsNumber.BTS2))
                    result = run_multithread_func(self.log,
                                                  [ho_task, iperf_task])
                    if not result[1]:
                        self.log.error("iPerf failed.")
                        return False

                self.log.info(
                    "handover test case result code {}.".format(result[0]))
                if volte:
                    # check if the phone stay in call
                    if not self.ad.droid.telecomIsInCall():
                        self.log.error("Call is already ended in the phone.")
                        return False

                    if not tear_down_call(self.log, self.ad, self.anritsu):
                        self.log.error("Phone {} Failed to tear down"
                                       .format(self.ad.serial, voice_number))
                        return False
                if simmodel[1] == "WCDMA" and iperf:
                    if all_bands:
                        bts[0].band = band[0][i % len(band[0])]
                    iperf_task = (self._iperf_task,
                                  (server_ip, WAITTIME_BEFORE_HANDOVER +
                                   WAITTIME_AFTER_HANDOVER - 10))
                    ho_task = (handover_tc, (self.log, self.anritsu,
                                             WAITTIME_BEFORE_HANDOVER,
                                             BtsNumber.BTS2, BtsNumber.BTS1))
                    result = run_multithread_func(self.log,
                                                  [ho_task, iperf_task])
                    if not result[1]:
                        self.log.error("iPerf failed.")
                        return False
                    self.log.info(
                        "handover test case result code {}.".format(result[0]))

        except AnritsuError as e:
            self.log.error("Error in connection with Anritsu Simulator: " +
                           str(e))
            return False
        except Exception as e:
            self.log.error("Exception during voice call procedure: " + str(e))
            return False
        return True

    def iperf_setup(self):
        # Fetch IP address of the host machine
        cmd = "|".join(("ifconfig", "grep eth0 -A1", "grep inet",
                        "cut -d ':' -f2", "cut -d ' ' -f 1"))
        destination_ip = exe_cmd(cmd)
        destination_ip = (destination_ip.decode("utf-8")).split("\n")[0]
        self.log.info("Dest IP is %s", destination_ip)

        if not adb_shell_ping(self.ad, DEFAULT_PING_DURATION, destination_ip):
            self.log.error("Pings failed to Destination.")
            return False

        return destination_ip

    def _iperf_task(self, destination_ip, duration):
        self.log.info("Starting iPerf task")
        self.ip_server.start()
        tput_dict = {"Uplink": 0, "Downlink": 0}
        if iperf_test_by_adb(
                self.log,
                self.ad,
                destination_ip,
                self.port_num,
                True,  # reverse = true
                duration,
                rate_dict=tput_dict):
            uplink = tput_dict["Uplink"]
            downlink = tput_dict["Downlink"]
            self.ip_server.stop()
            return True
        else:
            self.log.error("iperf failed to Destination.")
            self.ip_server.stop()
            return False

    def _phone_setup_lte_wcdma(self, ad):
        return ensure_network_rat(
            self.log,
            ad,
            NETWORK_MODE_LTE_GSM_WCDMA,
            RAT_FAMILY_LTE,
            toggle_apm_after_setting=True)

    def _phone_setup_lte_1x(self, ad):
        return ensure_network_rat(
            self.log,
            ad,
            NETWORK_MODE_LTE_CDMA_EVDO,
            RAT_FAMILY_LTE,
            toggle_apm_after_setting=True)

    def _phone_setup_wcdma(self, ad):
        return ensure_network_rat(
            self.log,
            ad,
            NETWORK_MODE_GSM_UMTS,
            RAT_FAMILY_UMTS,
            toggle_apm_after_setting=True)

    def _phone_setup_gsm(self, ad):
        return ensure_network_rat(
            self.log,
            ad,
            NETWORK_MODE_GSM_ONLY,
            RAT_FAMILY_GSM,
            toggle_apm_after_setting=True)

    def _phone_setup_1x(self, ad):
        return ensure_network_rat(
            self.log,
            ad,
            NETWORK_MODE_CDMA,
            RAT_FAMILY_CDMA2000,
            toggle_apm_after_setting=True)

    def _phone_setup_airplane_mode(self, ad):
        return toggle_airplane_mode_by_adb(self.log, ad, True)

    def _phone_setup_volte_airplane_mode(self, ad):
        toggle_volte(self.log, ad, True)
        return toggle_airplane_mode_by_adb(self.log, ad, True)

    def _phone_setup_volte(self, ad):
        ad.droid.telephonyToggleDataConnection(True)
        toggle_volte(self.log, ad, True)
        return ensure_network_rat(
            self.log,
            ad,
            NETWORK_MODE_LTE_CDMA_EVDO_GSM_WCDMA,
            RAT_FAMILY_LTE,
            toggle_apm_after_setting=True)

    """ Tests Begin """

    @test_tracker_info(uuid="bd014822-2c09-4503-9e01-594513ea6808")
    @TelephonyBaseTest.tel_test_wrap
    def test_volte_iperf_handover(self):
        """ Test VoLTE to VoLTE Inter-Freq handover with iPerf data
        Steps:
        1. Setup CallBox for 2 LTE cells with 2 different bands.
        2. Turn on DUT and enable VoLTE. Make an voice call to DEFAULT_CALL_NUMBER.
        3. Check if VoLTE voice call connected successfully.
        4. Start iPerf data transfer
        5. Handover the call to BTS2 and check if the call is still up.
        6. Check iPerf data throughput
        7. Tear down the call.

        Expected Results:
        1. VoLTE Voice call is made successfully.
        2. After handover, the call is not dropped.
        3. Tear down call succeed.

        Returns:
            True if pass; False if fail
        """
        return self.active_handover(
            set_system_model_lte_lte,
            self._phone_setup_volte,
            phone_idle_volte,
            volte=True,
            iperf=True)

    @test_tracker_info(uuid="a5a15947-40eb-4a70-b652-0b52a548c3c1")
    @TelephonyBaseTest.tel_test_wrap
    def test_volte_handover(self):
        """ Test VoLTE to VoLTE Inter-Freq handover without iPerf data
        Steps:
        1. Setup CallBox for 2 LTE cells with 2 different bands.
        2. Turn on DUT and enable VoLTE. Make an voice call to DEFAULT_CALL_NUMBER.
        3. Check if VoLTE voice call connected successfully.
        4. Handover the call to BTS2 and check if the call is still up.
        5. Tear down the call.

        Expected Results:
        1. VoLTE Voice call is made successfully.
        2. After handover, the call is not dropped.
        3. Tear down call succeed.

        Returns:
            True if pass; False if fail
        """
        return self.active_handover(
            set_system_model_lte_lte,
            self._phone_setup_volte,
            phone_idle_volte,
            volte=True,
            iperf=False)

    @test_tracker_info(uuid="382521d9-d991-49bc-8347-2e766ec0db74")
    @TelephonyBaseTest.tel_test_wrap
    def test_iperf_handover(self):
        """ Test Inter-Freq handover with iPerf data
        Steps:
        1. Setup CallBox for 2 LTE cells with 2 different bands.
        2. Turn on DUT and enable VoLTE.
        3. Start iPerf data transfer
        4. Handover the call to BTS2
        5. Check iPerf data throughput

        Expected Results:
        1. Data call is made successfully.
        2. After handover, the data is not dropped.

        Returns:
            True if pass; False if fail
        """
        return self.active_handover(
            set_system_model_lte_lte,
            self._phone_setup_volte,
            phone_idle_volte,
            volte=False,
            iperf=True)

    @test_tracker_info(uuid="d255a58b-8697-4d0a-9bc0-1e7ffa4cccaf")
    @TelephonyBaseTest.tel_test_wrap
    def test_volte_iperf_handover_wcdma(self):
        """ Test VoLTE to WCDMA to LTE handover with iPerf data
        Steps:
        1. Setup CallBox for LTE and WCDMA simulation.
        2. Turn on DUT and enable VoLTE. Make an voice call to DEFAULT_CALL_NUMBER.
        3. Check if VoLTE voice call connected successfully.
        4. Start iPerf data transfer
        5. SRVCC to WCDMA and check if the call is still up.
        6. Check iPerf data throughput
        7. Tear down the call.
        8. Handover back to LTE with iPerf

        Expected Results:
        1. VoLTE Voice call is made successfully.
        2. After handover, the call is not dropped.
        3. iPerf continue after handover

        Returns:
            True if pass; False if fail
        """
        return self.active_handover(
            set_system_model_lte_wcdma,
            self._phone_setup_volte,
            phone_idle_volte,
            volte=True,
            iperf=True)

    @test_tracker_info(uuid="28bc2e85-602e-4143-afe7-6dd442bef5c8")
    @TelephonyBaseTest.tel_test_wrap
    def test_volte_handover_wcdma(self):
        """ Test VoLTE to WCDMA handover (SRVCC)
        Steps:
        1. Setup CallBox for LTE and WCDMA simulation.
        2. Turn on DUT and enable VoLTE. Make an voice call to DEFAULT_CALL_NUMBER.
        3. Check if VoLTE voice call connected successfully.
        4. Start iPerf data transfer
        5. Handover the call to BTS2 and check if the call is still up.
        6. Check iPerf data throughput
        7. Tear down the call.

        Expected Results:
        1. VoLTE Voice call is made successfully.
        2. After handover, the call is not dropped.
        3. Tear down call succeed.

        Returns:
            True if pass; False if fail
        """
        return self.active_handover(
            set_system_model_lte_wcdma,
            self._phone_setup_volte,
            phone_idle_volte,
            volte=True,
            iperf=False)

    @test_tracker_info(uuid="3ef15650-8e44-4b75-b809-8d7dec5a41e3")
    @TelephonyBaseTest.tel_test_wrap
    def test_iperf_handover_wcdma(self):
        """ Test LTE to WCDMA to LTE handovers with iPerf data
        Steps:
        1. Setup CallBox for LTE and WCDMA simulation.
        2. Turn on DUT and register on LTE BTS.
        3. Start iPerf data transfer
        4. Handover to WCDMA.
        5. Stop and check iPerf data throughput
        6. Start iPerf data transfer
        7. Handover to WCDMA.
        8. Stop and check iPerf data throughput


        Expected Results:
        1. Each handover is successful
        2. After each handover, the iPerf continues successfully.

        Returns:
            True if pass; False if fail
        """
        return self.active_handover(
            set_system_model_lte_wcdma,
            self._phone_setup_volte,
            phone_idle_volte,
            volte=False,
            iperf=True)

    @test_tracker_info(uuid="2bfad82d-1797-474b-9bf7-c14602b061cd")
    @TelephonyBaseTest.tel_test_wrap
    def test_iperf_handover_wcdma_all_bands(self):
        """ Test LTE->WCDMA->LTE handovers through all bands UE supports with iPerf data
        Steps:
        1. Setup CallBox for LTE and WCDMA simulation.
        2. Turn on DUT and register on LTE BTS.
        3. Query MD8475A for UE supported bands contained in UE Capability Information
        4. Set target WCDMA band with first band in WCDMA supported band list
        5. Start iPerf data transfer
        6. Handover to WCDMA.
        7. Stop and check iPerf data throughput
        8. Set target LTE band with first band in LTE supported band list
        9. Start iPerf data transfer
        10. Handover to LTE.
        11. Stop and check iPerf data throughput
        12. Repeat step 4-11 with second WCDMA/LTE bands in supported band lists
        13. Repeat step 12 until all bands are tested. Reuse the begining of the shorter list to match the longer list.

        Expected Results:
        1. Each handover is successful
        2. After each handover, the iPerf continues successfully.

        Returns:
            True if pass; False if fail
        """
        return self.active_handover(
            set_system_model_lte_wcdma,
            self._phone_setup_volte,
            phone_idle_volte,
            volte=False,
            iperf=True,
            all_bands=True)

    """ Tests End """
