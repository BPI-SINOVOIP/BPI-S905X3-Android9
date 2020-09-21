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

from acts.controllers.anritsu_lib._anritsu_utils import AnritsuError
from acts.controllers.anritsu_lib.md8475a import CsfbType
from acts.controllers.anritsu_lib.md8475a import MD8475A
from acts.controllers.anritsu_lib.md8475a import VirtualPhoneAutoAnswer
from acts.controllers.anritsu_lib.md8475a import VirtualPhoneStatus
from acts.test_utils.tel.anritsu_utils import WAIT_TIME_ANRITSU_REG_AND_CALL
from acts.test_utils.tel.anritsu_utils import call_mo_setup_teardown
from acts.test_utils.tel.anritsu_utils import ims_call_cs_teardown
from acts.test_utils.tel.anritsu_utils import call_mt_setup_teardown
from acts.test_utils.tel.anritsu_utils import set_system_model_1x
from acts.test_utils.tel.anritsu_utils import set_system_model_1x_evdo
from acts.test_utils.tel.anritsu_utils import set_system_model_gsm
from acts.test_utils.tel.anritsu_utils import set_system_model_lte
from acts.test_utils.tel.anritsu_utils import set_system_model_lte_1x
from acts.test_utils.tel.anritsu_utils import set_system_model_lte_wcdma
from acts.test_utils.tel.anritsu_utils import set_system_model_lte_gsm
from acts.test_utils.tel.anritsu_utils import set_system_model_wcdma
from acts.test_utils.tel.anritsu_utils import set_usim_parameters
from acts.test_utils.tel.anritsu_utils import set_post_sim_params
from acts.test_utils.tel.tel_defines import CALL_TEARDOWN_PHONE
from acts.test_utils.tel.tel_defines import DEFAULT_EMERGENCY_CALL_NUMBER
from acts.test_utils.tel.tel_defines import EMERGENCY_CALL_NUMBERS
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
from acts.test_utils.tel.tel_test_utils import ensure_phone_default_state
from acts.test_utils.tel.tel_test_utils import ensure_phones_idle
from acts.test_utils.tel.tel_test_utils import toggle_airplane_mode_by_adb
from acts.test_utils.tel.tel_test_utils import toggle_volte
from acts.test_utils.tel.tel_test_utils import check_apm_mode_on_by_serial
from acts.test_utils.tel.tel_test_utils import set_apm_mode_on_by_serial
from acts.test_utils.tel.tel_test_utils import set_preferred_apn_by_adb
from acts.test_utils.tel.tel_test_utils import start_qxdm_loggers
from acts.test_utils.tel.tel_voice_utils import phone_idle_volte
from acts.test_utils.tel.TelephonyBaseTest import TelephonyBaseTest
from acts.test_decorators import test_tracker_info
from acts.utils import exe_cmd


class TelLabEmergencyCallTest(TelephonyBaseTest):
    def __init__(self, controllers):
        TelephonyBaseTest.__init__(self, controllers)
        try:
            self.stress_test_number = int(
                self.user_params["stress_test_number"])
            self.log.info("Executing {} calls per test in stress test mode".
                          format(self.stress_test_number))
        except KeyError:
            self.stress_test_number = 0
            self.log.info(
                "No 'stress_test_number' defined: running single iteration tests"
            )

        self.ad = self.android_devices[0]
        self.ad.sim_card = getattr(self.ad, "sim_card", None)
        self.md8475a_ip_address = self.user_params[
            "anritsu_md8475a_ip_address"]
        self.wlan_option = self.user_params.get("anritsu_wlan_option", False)

        setattr(self, 'emergency_call_number', DEFAULT_EMERGENCY_CALL_NUMBER)
        if 'emergency_call_number' in self.user_params:
            self.emergency_call_number = self.user_params[
                'emergency_call_number']
            self.log.info("Using provided emergency call number: {}".format(
                self.emergency_call_number))
        if not self.emergency_call_number in EMERGENCY_CALL_NUMBERS:
            self.log.warning("Unknown Emergency Number {}".format(
                self.emergency_call_number))

        # Check for all adb devices on the linux machine, and set APM ON
        cmd = "|".join(("adb devices", "grep -i device$", "cut -f1"))
        output = exe_cmd(cmd)
        list_of_devices = output.decode("utf-8").split("\n")
        if len(list_of_devices) > 1:
            for i in range(len(list_of_devices) - 1):
                self.log.info("Serial %s", list_of_devices[i])
                if check_apm_mode_on_by_serial(self.ad, list_of_devices[i]):
                    self.log.info("Device is already in APM ON")
                else:
                    self.log.info("Device is not in APM, turning it ON")
                    set_apm_mode_on_by_serial(self.ad, list_of_devices[i])
                    if check_apm_mode_on_by_serial(self.ad,
                                                   list_of_devices[i]):
                        self.log.info("Device is now in APM ON")

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
        if getattr(self, "qxdm_log", True):
            start_qxdm_loggers(self.log, self.android_devices)
        ensure_phone_default_state(self.log, self.ad, check_subscription=False)
        toggle_airplane_mode_by_adb(self.log, self.ad, True)
        try:
            if self.ad.sim_card == "VzW12349":
                self.ad.droid.imsSetVolteProvisioning(True)
        except Exception as e:
            self.ad.log.error(e)
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

    def _setup_emergency_call(self,
                              set_simulation_func,
                              phone_setup_func,
                              phone_idle_func_after_registration=None,
                              is_ims_call=False,
                              is_wait_for_registration=True,
                              csfb_type=None,
                              srlte_csfb=None,
                              srvcc=None,
                              emergency_number=DEFAULT_EMERGENCY_CALL_NUMBER,
                              teardown_side=CALL_TEARDOWN_PHONE,
                              wait_time_in_call=WAIT_TIME_IN_CALL):
        try:
            set_simulation_func(self.anritsu, self.user_params,
                                self.ad.sim_card)
            set_usim_parameters(self.anritsu, self.ad.sim_card)
            if is_ims_call or srvcc or csfb_type:
                set_post_sim_params(self.anritsu, self.user_params,
                                    self.ad.sim_card)
            self.virtualPhoneHandle.auto_answer = (VirtualPhoneAutoAnswer.ON,
                                                   2)
            if csfb_type:
                self.anritsu.csfb_type = csfb_type
            if srlte_csfb == "lte_call_failure":
                self.anritsu.send_command("IMSPSAPAUTOANSWER 1,DISABLE")
                self.anritsu.start_simulation()
                self.anritsu.send_command("IMSSTARTVN 1")
                check_ims_reg = True
                check_ims_calling = True
            elif srlte_csfb == "ims_unregistered":
                self.anritsu.start_simulation()
                self.anritsu.send_command("IMSSTARTVN 1")
                self.anritsu.send_command(
                    "IMSCSCFBEHAVIOR 1,SENDERRORRESPONSE603")
                check_ims_reg = False
                check_ims_calling = False
            elif srlte_csfb == "ps911_unsupported":
                self.anritsu.send_command("EMCBS NOTSUPPORT,BTS1")
                self.anritsu.start_simulation()
                self.anritsu.send_command("IMSSTARTVN 1")
                check_ims_reg = True
                check_ims_calling = False
            elif srlte_csfb == "emc_barred":
                self.anritsu.send_command("ACBARRED USERSPECIFIC,BTS1")
                self.anritsu.send_command("LTEEMERGENCYACBARRED BARRED,BTS1")
                self.anritsu.start_simulation()
                self.anritsu.send_command("IMSSTARTVN 1")
                check_ims_reg = True
                check_ims_calling = False
            elif srvcc == "InCall":
                self.anritsu.start_simulation()
                self.anritsu.send_command("IMSSTARTVN 1")
                self.anritsu.send_command("IMSSTARTVN 2")
                self.anritsu.send_command("IMSSTARTVN 3")
                check_ims_reg = True
                check_ims_calling = True
            else:
                self.anritsu.start_simulation()
            if is_ims_call or csfb_type:
                self.anritsu.send_command("IMSSTARTVN 1")
                self.anritsu.send_command("IMSSTARTVN 2")
                self.anritsu.send_command("IMSSTARTVN 3")

            iterations = 1
            if self.stress_test_number > 0:
                iterations = self.stress_test_number
            successes = 0
            for i in range(1, iterations + 1):
                if self.stress_test_number:
                    self.log.info(
                        "Running iteration {} of {}".format(i, iterations))
                # FIXME: There's no good reason why this must be true;
                # I can only assume this was done to work around a problem
                self.ad.droid.telephonyToggleDataConnection(False)

                # turn off all other BTS to ensure UE registers on BTS1
                sim_model = (self.anritsu.get_simulation_model()).split(",")
                no_of_bts = len(sim_model)
                for i in range(2, no_of_bts + 1):
                    self.anritsu.send_command(
                        "OUTOFSERVICE OUT,BTS{}".format(i))

                if phone_setup_func is not None:
                    if not phone_setup_func(self.ad):
                        self.log.warning(
                            "phone_setup_func failed. Rebooting UE")
                        self.ad.reboot()
                        time.sleep(30)
                        if self.ad.sim_card == "VzW12349":
                            set_preferred_apn_by_adb(self.ad, "VZWINTERNET")
                        if not phone_setup_func(self.ad):
                            self.log.error("phone_setup_func failed.")
                            continue

                if is_wait_for_registration:
                    self.anritsu.wait_for_registration_state()

                if phone_idle_func_after_registration:
                    if not phone_idle_func_after_registration(self.log,
                                                              self.ad):
                        continue

                for i in range(2, no_of_bts + 1):
                    self.anritsu.send_command(
                        "OUTOFSERVICE IN,BTS{}".format(i))

                time.sleep(WAIT_TIME_ANRITSU_REG_AND_CALL)
                if srlte_csfb or srvcc:
                    if not ims_call_cs_teardown(
                            self.log, self.ad, self.anritsu, emergency_number,
                            CALL_TEARDOWN_PHONE, True, check_ims_reg,
                            check_ims_calling, srvcc,
                            WAIT_TIME_IN_CALL_FOR_IMS, WAIT_TIME_IN_CALL):
                        self.log.error(
                            "Phone {} Failed to make emergency call to {}"
                            .format(self.ad.serial, emergency_number))
                        continue
                else:
                    if not call_mo_setup_teardown(
                            self.log, self.ad, self.anritsu, emergency_number,
                            CALL_TEARDOWN_PHONE, True, WAIT_TIME_IN_CALL,
                            is_ims_call):
                        self.log.error(
                            "Phone {} Failed to make emergency call to {}"
                            .format(self.ad.serial, emergency_number))
                        continue
                successes += 1
                if self.stress_test_number:
                    self.log.info("Passed iteration {}".format(i))
            if self.stress_test_number:
                self.log.info("Total of {} successes out of {} attempts".
                              format(successes, iterations))
            return True if successes == iterations else False

        except AnritsuError as e:
            self.log.error("Error in connection with Anritsu Simulator: " +
                           str(e))
            return False
        except Exception as e:
            self.log.error("Exception during emergency call procedure: " + str(
                e))
            return False
        return True

    def _phone_setup_lte_wcdma(self, ad):
        toggle_volte(self.log, ad, False)
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

    def _phone_disable_airplane_mode(self, ad):
        return toggle_airplane_mode_by_adb(self.log, ad, False)

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

    @test_tracker_info(uuid="f5c93228-3b43-48a3-b509-796d41625171")
    @TelephonyBaseTest.tel_test_wrap
    def test_emergency_call_lte_wcdma_csfb_redirection(self):
        """ Test Emergency call functionality on LTE.
            CSFB type is REDIRECTION

        Steps:
        1. Setup CallBox on LTE and WCDMA network, make sure DUT register on LTE network.
        2. Make an emergency call to 911. Make sure DUT does not CSFB to WCDMA.
        3. Make sure Anritsu receives the call and accept.
        4. Tear down the call.

        Expected Results:
        2. Emergency call succeed. DUT does not CSFB to WCDMA.
        3. Anritsu can accept the call.
        4. Tear down call succeed.

        Returns:
            True if pass; False if fail
        """
        return self._setup_emergency_call(
            set_system_model_lte_wcdma,
            self._phone_setup_lte_wcdma,
            emergency_number=self.emergency_call_number,
            csfb_type=CsfbType.CSFB_TYPE_REDIRECTION,
            is_ims_call=True)

    @test_tracker_info(uuid="8deb6b21-2cb0-4241-bcad-6cd62a340b07")
    @TelephonyBaseTest.tel_test_wrap
    def test_emergency_call_lte_wcdma_csfb_handover(self):
        """ Test Emergency call functionality on LTE.
            CSFB type is HANDOVER

        Steps:
        1. Setup CallBox on LTE and WCDMA network, make sure DUT register on LTE network.
        2. Make an emergency call to 911. Make sure DUT does not CSFB to WCDMA.
        3. Make sure Anritsu receives the call and accept.
        4. Tear down the call.

        Expected Results:
        2. Emergency call succeed. DUT does not CSFB to WCDMA.
        3. Anritsu can accept the call.
        4. Tear down call succeed.

        Returns:
            True if pass; False if fail
        """
        return self._setup_emergency_call(
            set_system_model_lte_wcdma,
            self._phone_setup_lte_wcdma,
            emergency_number=self.emergency_call_number,
            csfb_type=CsfbType.CSFB_TYPE_HANDOVER,
            is_ims_call=True)

    @test_tracker_info(uuid="52b6b783-de77-497d-87e0-63c930e6c9bb")
    @TelephonyBaseTest.tel_test_wrap
    def test_emergency_call_lte_1x_csfb(self):
        """ Test Emergency call functionality on LTE (CSFB to 1x).

        Steps:
        1. Setup CallBox on LTE and CDMA 1X network, make sure DUT register on LTE network.
        2. Make an emergency call to 911. Make sure DUT CSFB to 1x.
        3. Make sure Anritsu receives the call and accept.
        4. Tear down the call.

        Expected Results:
        2. Emergency call succeed. DUT CSFB to 1x.
        3. Anritsu can accept the call.
        4. Tear down call succeed.

        Returns:
            True if pass; False if fail
        """
        return self._setup_emergency_call(
            set_system_model_lte_1x,
            self._phone_setup_lte_1x,
            emergency_number=self.emergency_call_number)

    @test_tracker_info(uuid="fcdd5a4f-fdf2-44dc-b1b2-44ab175791ce")
    @TelephonyBaseTest.tel_test_wrap
    def test_emergency_call_wcdma(self):
        """ Test Emergency call functionality on WCDMA

        Steps:
        1. Setup CallBox on WCDMA network, make sure DUT register on WCDMA network.
        2. Make an emergency call to 911.
        3. Make sure Anritsu receives the call and accept.
        4. Tear down the call.

        Expected Results:
        2. Emergency call succeed.
        3. Anritsu can accept the call.
        4. Tear down call succeed.

        Returns:
            True if pass; False if fail
        """
        return self._setup_emergency_call(
            set_system_model_wcdma,
            self._phone_setup_wcdma,
            emergency_number=self.emergency_call_number)

    @test_tracker_info(uuid="295c3188-24e2-4c53-80c6-3d3001e2ff16")
    @TelephonyBaseTest.tel_test_wrap
    def test_emergency_call_gsm(self):
        """ Test Emergency call functionality on GSM

        Steps:
        1. Setup CallBox on GSM network, make sure DUT register on GSM network.
        2. Make an emergency call to 911.
        3. Make sure Anritsu receives the call and accept.
        4. Tear down the call.

        Expected Results:
        2. Emergency call succeed.
        3. Anritsu can accept the call.
        4. Tear down call succeed.

        Returns:
            True if pass; False if fail
        """
        return self._setup_emergency_call(
            set_system_model_gsm,
            self._phone_setup_gsm,
            emergency_number=self.emergency_call_number)

    @test_tracker_info(uuid="4f21bfc0-a0a9-43b8-9285-dcfb131a3a04")
    @TelephonyBaseTest.tel_test_wrap
    def test_emergency_call_1x(self):
        """ Test Emergency call functionality on CDMA 1X

        Steps:
        1. Setup CallBox on 1x network, make sure DUT register on 1x network.
        2. Make an emergency call to 911.
        3. Make sure Anritsu receives the call and accept.
        4. Tear down the call.

        Expected Results:
        2. Emergency call succeed.
        3. Anritsu can accept the call.
        4. Tear down call succeed.

        Returns:
            True if pass; False if fail
        """
        return self._setup_emergency_call(
            set_system_model_1x,
            self._phone_setup_1x,
            emergency_number=self.emergency_call_number)

    @test_tracker_info(uuid="6dfd3e1d-7faa-426f-811a-ebd5e54f9c9a")
    @TelephonyBaseTest.tel_test_wrap
    def test_emergency_call_1x_evdo(self):
        """ Test Emergency call functionality on CDMA 1X with EVDO

        Steps:
        1. Setup CallBox on 1x and EVDO network, make sure DUT register on 1x network.
        2. Make an emergency call to 911.
        3. Make sure Anritsu receives the call and accept.
        4. Tear down the call.

        Expected Results:
        2. Emergency call succeed.
        3. Anritsu can accept the call.
        4. Tear down call succeed.

        Returns:
            True if pass; False if fail
        """
        return self._setup_emergency_call(
            set_system_model_1x_evdo,
            self._phone_setup_1x,
            emergency_number=self.emergency_call_number)

    @test_tracker_info(uuid="40f9897e-4924-4896-9f2c-0b4f45331251")
    @TelephonyBaseTest.tel_test_wrap
    def test_emergency_call_1x_apm(self):
        """ Test Emergency call functionality on Airplane mode

        Steps:
        1. Setup CallBox on 1x network.
        2. Turn on Airplane mode on DUT. Make an emergency call to 911.
        3. Make sure Anritsu receives the call and accept.
        4. Tear down the call.

        Expected Results:
        2. Emergency call succeed.
        3. Anritsu can accept the call.
        4. Tear down call succeed.

        Returns:
            True if pass; False if fail
        """
        return self._setup_emergency_call(
            set_system_model_1x,
            self._phone_setup_airplane_mode,
            is_wait_for_registration=False,
            emergency_number=self.emergency_call_number)

    @test_tracker_info(uuid="5997c004-449b-4a5e-816e-2ff0c0eb928d")
    @TelephonyBaseTest.tel_test_wrap
    def test_emergency_call_wcdma_apm(self):
        """ Test Emergency call functionality on Airplane mode

        Steps:
        1. Setup CallBox on WCDMA network.
        2. Make an emergency call to 911.
        3. Make sure Anritsu receives the call and accept.
        4. Tear down the call.

        Expected Results:
        2. Emergency call succeed.
        3. Anritsu can accept the call.
        4. Tear down call succeed.

        Returns:
            True if pass; False if fail
        """
        return self._setup_emergency_call(
            set_system_model_wcdma,
            self._phone_setup_airplane_mode,
            is_wait_for_registration=False,
            emergency_number=self.emergency_call_number)

    @test_tracker_info(uuid="9afde8d3-bdf0-41d9-af8c-2a8f36aae169")
    @TelephonyBaseTest.tel_test_wrap
    def test_emergency_call_volte_wcdma_srvcc(self):
        """ Test Emergency call functionality,
        VoLTE to WCDMA SRVCC
        Steps:
        1. Setup CallBox on VoLTE network with WCDMA.
        2. Turn on DUT and enable VoLTE. Make an emergency call to 911.
        3. Check if VoLTE emergency call connected successfully.
        4. Handover the call to WCDMA and check if the call is still up.
        5. Tear down the call.

        Expected Results:
        1. VoLTE Emergency call is made successfully.
        2. After SRVCC, the 911 call is not dropped.
        3. Tear down call succeed.

        Returns:
            True if pass; False if fail
        """
        return self._setup_emergency_call(
            set_system_model_lte_wcdma,
            self._phone_setup_volte,
            phone_idle_volte,
            srvcc="InCall",
            emergency_number=self.emergency_call_number,
            wait_time_in_call=WAIT_TIME_IN_CALL_FOR_IMS)

    @test_tracker_info(uuid="2e454f11-e77d-452a-bcac-7271378953ed")
    @TelephonyBaseTest.tel_test_wrap
    def test_emergency_call_volte_gsm_srvcc(self):
        """ Test Emergency call functionality,
        VoLTE to GSM SRVCC
        Steps:
        1. Setup CallBox on VoLTE network with GSM.
        2. Turn on DUT and enable VoLTE. Make an emergency call to 911.
        3. Check if VoLTE emergency call connected successfully.
        4. Handover the call to GSM and check if the call is still up.
        5. Tear down the call.

        Expected Results:
        1. VoLTE Emergency call is made successfully.
        2. After SRVCC, the 911 call is not dropped.
        3. Tear down call succeed.

        Returns:
            True if pass; False if fail
        """
        return self._setup_emergency_call(
            set_system_model_lte_gsm,
            self._phone_setup_volte,
            phone_idle_volte,
            srvcc="InCall",
            emergency_number=self.emergency_call_number,
            wait_time_in_call=WAIT_TIME_IN_CALL_FOR_IMS)

    @test_tracker_info(uuid="4dae7e62-b73e-4ba1-92ee-ecc121b898b3")
    @TelephonyBaseTest.tel_test_wrap
    def test_emergency_call_csfb_1x_lte_call_failure(self):
        """ Test Emergency call functionality,
        CSFB to CDMA1x after VoLTE call failure
        Ref: VzW LTE E911 test plan, 2.23, VZ_TC_LTEE911_7481
        Steps:
        1. Setup CallBox on VoLTE network with CDMA1x.
        2. Turn on DUT and enable VoLTE. Make an emergency call to 911.
        3. Make sure Anritsu IMS server does not answer the call
        4. The DUT requests CSFB to 1XCDMA and Anritsu accepts the call.
        5. Tear down the call.

        Expected Results:
        1. VoLTE Emergency call is made.
        2. Anritsu receive the call but does not answer.
        3. The 911 call CSFB to CDMA1x and answered successfully.
        4. Tear down call succeed.

        Returns:
            True if pass; False if fail
        """
        return self._setup_emergency_call(
            set_system_model_lte_1x,
            self._phone_setup_volte,
            phone_idle_volte,
            srlte_csfb="lte_call_failure",
            emergency_number=self.emergency_call_number,
            wait_time_in_call=WAIT_TIME_IN_CALL_FOR_IMS)

    @test_tracker_info(uuid="e98c9101-1ab6-497e-9d67-a4ff62d28fea")
    @TelephonyBaseTest.tel_test_wrap
    def test_emergency_call_csfb_1x_ims_unregistered(self):
        """ Test Emergency call functionality,
        CSFB to CDMA1x because ims registration delcined
        Ref: VzW LTE E911 test plan, 2.25, VZ_TC_LTEE911_7483
        Steps:
        1. Setup CallBox on VoLTE network with CDMA1x.
        2. Setup Anritsu IMS server to decline IMS registration
        3. Turn on DUT and enable VoLTE. Make an emergency call to 911.
        4. The DUT requests CSFB to 1XCDMA and Anritsu accepts the call.
        5. Tear down the call.

        Expected Results:
        1. Phone registers on LTE network with VoLTE enabled.
        2. When Emergency call is made, phone request CSFB to CDMA1x.
        3. The 911 call on CDMA1x is answered successfully.
        4. Tear down call succeed.

        Returns:
            True if pass; False if fail
        """
        return self._setup_emergency_call(
            set_system_model_lte_1x,
            self._phone_setup_volte,
            None,
            srlte_csfb="ims_unregistered",
            emergency_number=self.emergency_call_number,
            wait_time_in_call=WAIT_TIME_IN_CALL_FOR_IMS)

    @test_tracker_info(uuid="d96e1b9b-2f6d-49f3-a203-f4277056869e")
    @TelephonyBaseTest.tel_test_wrap
    def test_emergency_call_csfb_1x_ps911_unsupported(self):
        """ Test Emergency call functionality,
        CSFB to CDMA1x because MME does not support PS911, by setting
        Emergency Bearer Service not supported in EPS network feature
        Ref: VzW LTE E911 test plan, 2.26, VZ_TC_LTEE911_8357
        Steps:
        1. Setup CallBox on VoLTE network with CDMA1x.
        2. Setup Emergency Bearer Service not supported in EPS network feature
        3. Turn on DUT and enable VoLTE. Make an emergency call to 911.
        4. The DUT requests CSFB to 1XCDMA and Anritsu accepts the call.
        5. Tear down the call.

        Expected Results:
        1. Phone registers on LTE network with VoLTE enabled.
        2. When Emergency call is made, phone request CSFB to CDMA1x.
        3. The 911 call on CDMA1x is answered successfully.
        4. Tear down call succeed.

        Returns:
            True if pass; False if fail
        """
        return self._setup_emergency_call(
            set_system_model_lte_1x,
            self._phone_setup_volte,
            phone_idle_volte,
            srlte_csfb="ps911_unsupported",
            emergency_number=self.emergency_call_number,
            wait_time_in_call=WAIT_TIME_IN_CALL_FOR_IMS)

    @test_tracker_info(uuid="f7d48841-b8ef-4031-99de-28534aaf4c44")
    @TelephonyBaseTest.tel_test_wrap
    def test_emergency_call_csfb_1x_emc_barred(self):
        """ Test Emergency call functionality,
        CSFB to CDMA1x because SIB2 Emergency Barred,
        by setting Access Class Barred for Emergency
        Ref: VzW LTE E911 test plan, 2.27, VZ_TC_LTEE911_8358
        Steps:
        1. Setup CallBox on VoLTE network with CDMA1x.
        2. Set Access Class Barred for Emergency in SIB2
        3. Turn on DUT and enable VoLTE. Make an emergency call to 911.
        4. The DUT requests CSFB to 1XCDMA and Anritsu accepts the call.
        5. Tear down the call.

        Expected Results:
        1. Phone registers on LTE network with VoLTE enabled.
        2. When Emergency call is made, phone request CSFB to CDMA1x.
        3. The 911 call on CDMA1x is answered successfully.
        4. Tear down call succeed.

        Returns:
            True if pass; False if fail
        """
        return self._setup_emergency_call(
            set_system_model_lte_1x,
            self._phone_setup_volte,
            phone_idle_volte,
            srlte_csfb="emc_barred",
            emergency_number=self.emergency_call_number,
            wait_time_in_call=WAIT_TIME_IN_CALL_FOR_IMS)

    @test_tracker_info(uuid="5bbbecec-0fef-430b-acbd-01ef7c7055c0")
    @TelephonyBaseTest.tel_test_wrap
    def test_emergency_call_volte_1x(self):
        """ Test Emergency call functionality on VoLTE with CDMA1x
        Ref: VzW LTE E911 test plan, 2.24, VZ_TC_LTEE911_7482
        Steps:
        1. Setup CallBox on VoLTE network with CDMA1x.
        2. Turn on DUT and enable VoLTE. Make an emergency call to 911.
        3. Make sure Anritsu receives the call and accept.
        4. Tear down the call.

        Expected Results:
        2. Emergency call succeed.
        3. Anritsu can accept the call.
        4. Tear down call succeed.

        Returns:
            True if pass; False if fail
        """
        return self._setup_emergency_call(
            set_system_model_lte_1x,
            self._phone_setup_volte,
            phone_idle_volte,
            is_ims_call=True,
            emergency_number=self.emergency_call_number,
            wait_time_in_call=WAIT_TIME_IN_CALL_FOR_IMS)

    @test_tracker_info(uuid="e32862e0-ec11-4de8-8b9a-851bab9feb29")
    @TelephonyBaseTest.tel_test_wrap
    def test_emergency_call_volte(self):
        """ Test Emergency call functionality on VoLTE

        Steps:
        1. Setup CallBox on VoLTE network.
        2. Turn on DUT and enable VoLTE. Make an emergency call to 911.
        3. Make sure Anritsu receives the call and accept.
        4. Tear down the call.

        Expected Results:
        2. Emergency call succeed.
        3. Anritsu can accept the call.
        4. Tear down call succeed.

        Returns:
            True if pass; False if fail
        """
        return self._setup_emergency_call(
            set_system_model_lte,
            self._phone_setup_volte,
            phone_idle_volte,
            is_ims_call=True,
            emergency_number=self.emergency_call_number,
            wait_time_in_call=WAIT_TIME_IN_CALL_FOR_IMS)

    @test_tracker_info(uuid="fa5b4e52-c249-42ec-a9d3-9523336f88f7")
    @TelephonyBaseTest.tel_test_wrap
    def test_emergency_call_volte_apm(self):
        """ Test Emergency call functionality on VoLTE

        Steps:
        1. Setup CallBox on VoLTE network.
        2. Turn on Airplane mode on DUT. Make an emergency call to 911.
        3. Make sure Anritsu receives the call and accept.
        4. Tear down the call.

        Expected Results:
        2. Emergency call succeed.
        3. Anritsu can accept the call.
        4. Tear down call succeed.

        Returns:
            True if pass; False if fail
        """
        return self._setup_emergency_call(
            set_system_model_lte,
            self._phone_setup_volte_airplane_mode,
            is_ims_call=True,
            is_wait_for_registration=False,
            emergency_number=self.emergency_call_number,
            wait_time_in_call=WAIT_TIME_IN_CALL_FOR_IMS)

    @test_tracker_info(uuid="260e4892-fdae-4d49-bfc6-04fe72a5a715")
    @TelephonyBaseTest.tel_test_wrap
    def test_emergency_call_no_sim_wcdma(self):
        """ Test Emergency call functionality with no SIM.

        Steps:
        1. Setup CallBox on WCDMA network.
        2. Make an emergency call to 911.
        3. Make sure Anritsu receives the call and accept.
        4. Tear down the call.

        Expected Results:
        2. Emergency call succeed.
        3. Anritsu can accept the call.
        4. Tear down call succeed.

        Returns:
            True if pass; False if fail
        """
        return self._setup_emergency_call(
            set_system_model_wcdma,
            self._phone_disable_airplane_mode,
            emergency_number=self.emergency_call_number,
            is_wait_for_registration=False)

    @test_tracker_info(uuid="2dbbbde5-c298-4a2d-ae65-b739d1dd1445")
    @TelephonyBaseTest.tel_test_wrap
    def test_emergency_call_no_sim_1x(self):
        """ Test Emergency call functionality with no SIM.

        Steps:
        1. Setup CallBox on 1x network.
        2. Make an emergency call to 911.
        3. Make sure Anritsu receives the call and accept.
        4. Tear down the call.

        Expected Results:
        2. Emergency call succeed.
        3. Anritsu can accept the call.
        4. Tear down call succeed.

        Returns:
            True if pass; False if fail
        """
        return self._setup_emergency_call(
            set_system_model_1x,
            self._phone_disable_airplane_mode,
            emergency_number=self.emergency_call_number,
            is_wait_for_registration=False)

    @test_tracker_info(uuid="fa77dee2-3235-42e0-b24d-c8fdb3fbef2f")
    @TelephonyBaseTest.tel_test_wrap
    def test_emergency_call_no_sim_gsm(self):
        """ Test Emergency call functionality with no SIM.

        Steps:
        1. Setup CallBox on GSM network.
        2. Make an emergency call to 911.
        3. Make sure Anritsu receives the call and accept.
        4. Tear down the call.

        Expected Results:
        2. Emergency call succeed.
        3. Anritsu can accept the call.
        4. Tear down call succeed.

        Returns:
            True if pass; False if fail
        """
        return self._setup_emergency_call(
            set_system_model_gsm,
            self._phone_disable_airplane_mode,
            emergency_number=self.emergency_call_number,
            is_wait_for_registration=False)

    @test_tracker_info(uuid="9ead5c6a-b4cb-40d5-80b0-54d3c7ad31ff")
    @TelephonyBaseTest.tel_test_wrap
    def test_emergency_call_no_sim_volte(self):
        """ Test Emergency call functionality with no SIM.

        Steps:
        1. Setup CallBox on VoLTE network.
        2. Make an emergency call to 911.
        3. Make sure Anritsu receives the call and accept.
        4. Tear down the call.

        Expected Results:
        2. Emergency call succeed.
        3. Anritsu can accept the call.
        4. Tear down call succeed.

        Returns:
            True if pass; False if fail
        """
        return self._setup_emergency_call(
            set_system_model_lte,
            self._phone_disable_airplane_mode,
            is_wait_for_registration=False,
            is_ims_call=True,
            emergency_number=self.emergency_call_number,
            wait_time_in_call=WAIT_TIME_IN_CALL_FOR_IMS)

    @test_tracker_info(uuid="d6bdc1d7-0a08-4e6e-9de8-4084abb48bad")
    @TelephonyBaseTest.tel_test_wrap
    def test_emergency_call_no_sim_1x_ecbm(self):
        """ Test Emergency call functionality with no SIM.

        Steps:
        1. Setup CallBox on 1x network.
        2. Make an emergency call to 911.
        3. Make sure Anritsu receives the call and accept.
        4. Tear down the call.
        5. Make a call from Callbox to DUT.
        6. Verify DUT receive the incoming call.
        7. Answer on DUT, verify DUT can answer the call correctly.
        8. Hangup the call on DUT.

        Expected Results:
        2. Emergency call succeed.
        3. Anritsu can accept the call.
        4. Tear down call succeed.
        6. DUT receive incoming call.
        7. DUT answer the call correctly.
        8. Tear down call succeed.

        Returns:
            True if pass; False if fail
        """
        if not self._setup_emergency_call(
                set_system_model_1x,
                self._phone_disable_airplane_mode,
                emergency_number=self.emergency_call_number,
                is_wait_for_registration=False):
            self.log.error("Failed to make 911 call.")
            return False
        return call_mt_setup_teardown(self.log, self.ad,
                                      self.anritsu.get_VirtualPhone(), None,
                                      CALL_TEARDOWN_PHONE, RAT_1XRTT)

    """ Tests End """
