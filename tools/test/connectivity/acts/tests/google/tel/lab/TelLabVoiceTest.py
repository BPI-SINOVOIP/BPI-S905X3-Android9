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
from acts.test_utils.tel.tel_test_utils import set_preferred_apn_by_adb
from acts.test_utils.tel.tel_test_utils import start_qxdm_loggers
from acts.test_utils.tel.tel_voice_utils import phone_idle_volte
from acts.test_utils.tel.TelephonyBaseTest import TelephonyBaseTest

DEFAULT_CALL_NUMBER = "0123456789"


class TelLabVoiceTest(TelephonyBaseTest):
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

        setattr(self, 'voice_call_number', DEFAULT_CALL_NUMBER)
        if 'voice_call_number' in self.user_params:
            self.voice_call_number = self.user_params['voice_call_number']
            self.log.info("Using provided voice call number: {}".format(
                self.voice_call_number))

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
            if self.ad.sim_card == "VzW12349":
                self.ad.droid.imsSetVolteProvisioning(True)
        except Exception as e:
            self.ad.log.error(e)
        toggle_airplane_mode_by_adb(self.log, self.ad, True)
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

    def _setup_voice_call(self,
                          set_simulation_func,
                          phone_setup_func,
                          phone_idle_func_after_registration=None,
                          is_ims_call=False,
                          is_wait_for_registration=True,
                          csfb_type=None,
                          srvcc=None,
                          mo=True,
                          voice_number=DEFAULT_CALL_NUMBER,
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
            if srvcc != None:
                if srvcc == "Alert":
                    self.anritsu.send_command("IMSCSCFAUTOANSWER 1,DISABLE")
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
                if srvcc:
                    if not ims_call_cs_teardown(
                            self.log, self.ad, self.anritsu, voice_number,
                            CALL_TEARDOWN_PHONE, False, check_ims_reg,
                            check_ims_calling, srvcc, mo,
                            WAIT_TIME_IN_CALL_FOR_IMS, WAIT_TIME_IN_CALL):
                        if mo:
                            self.log.error(
                                "Phone {} Failed to make voice call to {}"
                                .format(self.ad.serial, voice_number))
                        else:
                            self.log.error(
                                "Phone {} failed to answer voice call."
                                .format(self.ad.serial))
                        continue
                else:
                    if not call_mo_setup_teardown(
                            self.log, self.ad, self.anritsu, voice_number,
                            CALL_TEARDOWN_PHONE, False, WAIT_TIME_IN_CALL,
                            is_ims_call):
                        self.log.error(
                            "Phone {} Failed to make voice call to {}"
                            .format(self.ad.serial, voice_number))
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
            self.log.error("Exception during voice call procedure: " + str(e))
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

    @test_tracker_info(uuid="56c42e16-3936-4c51-8b8b-4866f54cc0bc")
    @TelephonyBaseTest.tel_test_wrap
    def test_voice_call_lte_wcdma_csfb_redirection(self):
        """ Test Voice call functionality on LTE (CSFB to WCDMA).
            CSFB type is REDIRECTION

        Steps:
        1. Setup CallBox on LTE and WCDMA network, make sure DUT register on LTE network.
        2. Make an voice call to DEFAULT_CALL_NUMBER. Make sure DUT CSFB to WCDMA.
        3. Make sure Anritsu receives the call and accept.
        4. Tear down the call.

        Expected Results:
        2. Voice call succeed. DUT CSFB to WCDMA.
        3. Anritsu can accept the call.
        4. Tear down call succeed.

        Returns:
            True if pass; False if fail
        """
        return self._setup_voice_call(
            set_system_model_lte_wcdma,
            self._phone_setup_lte_wcdma,
            voice_number=self.voice_call_number,
            csfb_type=CsfbType.CSFB_TYPE_REDIRECTION)

    @test_tracker_info(uuid="dcc1428f-9b7d-4064-8313-f1f5e428e0c7")
    @TelephonyBaseTest.tel_test_wrap
    def test_voice_call_lte_wcdma_csfb_handover(self):
        """ Test Voice call functionality on LTE (CSFB to WCDMA).
            CSFB type is HANDOVER

        Steps:
        1. Setup CallBox on LTE and WCDMA network, make sure DUT register on LTE network.
        2. Make an voice call to DEFAULT_CALL_NUMBER. Make sure DUT CSFB to WCDMA.
        3. Make sure Anritsu receives the call and accept.
        4. Tear down the call.

        Expected Results:
        2. Voice call succeed. DUT CSFB to WCDMA.
        3. Anritsu can accept the call.
        4. Tear down call succeed.

        Returns:
            True if pass; False if fail
        """
        return self._setup_voice_call(
            set_system_model_lte_wcdma,
            self._phone_setup_lte_wcdma,
            voice_number=self.voice_call_number,
            csfb_type=CsfbType.CSFB_TYPE_HANDOVER)

    @test_tracker_info(uuid="e250b134-d5e9-48ca-b224-eb0e07648275")
    @TelephonyBaseTest.tel_test_wrap
    def test_voice_call_lte_1x_csfb(self):
        """ Test Voice call functionality on LTE (CSFB to 1x).

        Steps:
        1. Setup CallBox on LTE and CDMA 1X network, make sure DUT register on LTE network.
        2. Make an voice call to DEFAULT_CALL_NUMBER. Make sure DUT CSFB to 1x.
        3. Make sure Anritsu receives the call and accept.
        4. Tear down the call.

        Expected Results:
        2. Voice call succeed. DUT CSFB to 1x.
        3. Anritsu can accept the call.
        4. Tear down call succeed.

        Returns:
            True if pass; False if fail
        """
        return self._setup_voice_call(
            set_system_model_lte_1x,
            self._phone_setup_lte_1x,
            voice_number=self.voice_call_number)

    @test_tracker_info(uuid="fcbe0f5d-51c2-46c8-8ff3-2daa1d91b936")
    @TelephonyBaseTest.tel_test_wrap
    def test_voice_call_wcdma(self):
        """ Test Voice call functionality on WCDMA

        Steps:
        1. Setup CallBox on WCDMA network, make sure DUT register on WCDMA network.
        2. Make an voice call to DEFAULT_CALL_NUMBER.
        3. Make sure Anritsu receives the call and accept.
        4. Tear down the call.

        Expected Results:
        2. Voice call succeed.
        3. Anritsu can accept the call.
        4. Tear down call succeed.

        Returns:
            True if pass; False if fail
        """
        return self._setup_voice_call(
            set_system_model_wcdma,
            self._phone_setup_wcdma,
            voice_number=self.voice_call_number)

    @test_tracker_info(uuid="077f851b-2c8e-4b1d-adc2-0326d3346157")
    @TelephonyBaseTest.tel_test_wrap
    def test_voice_call_gsm(self):
        """ Test Voice call functionality on GSM

        Steps:
        1. Setup CallBox on GSM network, make sure DUT register on GSM network.
        2. Make an voice call to DEFAULT_CALL_NUMBER.
        3. Make sure Anritsu receives the call and accept.
        4. Tear down the call.

        Expected Results:
        2. Voice call succeed.
        3. Anritsu can accept the call.
        4. Tear down call succeed.

        Returns:
            True if pass; False if fail
        """
        return self._setup_voice_call(
            set_system_model_gsm,
            self._phone_setup_gsm,
            voice_number=self.voice_call_number)

    @test_tracker_info(uuid="80376fb3-44fc-43b7-be99-2ccd3bd2913e")
    @TelephonyBaseTest.tel_test_wrap
    def test_voice_call_1x(self):
        """ Test Voice call functionality on CDMA 1X

        Steps:
        1. Setup CallBox on 1x network, make sure DUT register on 1x network.
        2. Make an voice call to DEFAULT_CALL_NUMBER.
        3. Make sure Anritsu receives the call and accept.
        4. Tear down the call.

        Expected Results:
        2. Voice call succeed.
        3. Anritsu can accept the call.
        4. Tear down call succeed.

        Returns:
            True if pass; False if fail
        """
        return self._setup_voice_call(
            set_system_model_1x,
            self._phone_setup_1x,
            voice_number=self.voice_call_number)

    @test_tracker_info(uuid="1f8ae218-042d-4114-9fc7-4401a503b3b4")
    @TelephonyBaseTest.tel_test_wrap
    def test_voice_call_1x_evdo(self):
        """ Test Voice call functionality on CDMA 1X with EVDO

        Steps:
        1. Setup CallBox on 1x and EVDO network, make sure DUT register on 1x network.
        2. Make an voice call to DEFAULT_CALL_NUMBER.
        3. Make sure Anritsu receives the call and accept.
        4. Tear down the call.

        Expected Results:
        2. Voice call succeed.
        3. Anritsu can accept the call.
        4. Tear down call succeed.

        Returns:
            True if pass; False if fail
        """
        return self._setup_voice_call(
            set_system_model_1x_evdo,
            self._phone_setup_1x,
            voice_number=self.voice_call_number)

    @test_tracker_info(uuid="7641ffc0-c1b3-42b8-92d6-00ae53719f8d")
    @TelephonyBaseTest.tel_test_wrap
    def test_voice_call_volte_wcdma_srvcc(self):
        """ Test Voice call functionality,
        VoLTE to WCDMA SRVCC
        Steps:
        1. Setup CallBox on VoLTE network with WCDMA.
        2. Turn on DUT and enable VoLTE. Make an voice call to DEFAULT_CALL_NUMBER.
        3. Check if VoLTE voice call connected successfully.
        4. Handover the call to WCDMA and check if the call is still up.
        5. Tear down the call.

        Expected Results:
        1. VoLTE Voice call is made successfully.
        2. After SRVCC, the DEFAULT_CALL_NUMBER call is not dropped.
        3. Tear down call succeed.

        Returns:
            True if pass; False if fail
        """
        return self._setup_voice_call(
            set_system_model_lte_wcdma,
            self._phone_setup_volte,
            phone_idle_volte,
            srvcc="InCall",
            voice_number=self.voice_call_number,
            wait_time_in_call=WAIT_TIME_IN_CALL_FOR_IMS)

    @test_tracker_info(uuid="0d63e797-b4bc-4094-98c3-70060e5ea91b")
    @TelephonyBaseTest.tel_test_wrap
    def test_voice_call_volte_gsm_srvcc(self):
        """ Test Voice call functionality,
        VoLTE to GSM SRVCC
        Steps:
        1. Setup CallBox on VoLTE network with GSM.
        2. Turn on DUT and enable VoLTE. Make an voice call to DEFAULT_CALL_NUMBER.
        3. Check if VoLTE voice call connected successfully.
        4. Handover the call to GSM and check if the call is still up.
        5. Tear down the call.

        Expected Results:
        1. VoLTE Voice call is made successfully.
        2. After SRVCC, the DEFAULT_CALL_NUMBER call is not dropped.
        3. Tear down call succeed.

        Returns:
            True if pass; False if fail
        """
        return self._setup_voice_call(
            set_system_model_lte_gsm,
            self._phone_setup_volte,
            phone_idle_volte,
            srvcc="InCall",
            voice_number=self.voice_call_number,
            wait_time_in_call=WAIT_TIME_IN_CALL_FOR_IMS)

    @test_tracker_info(uuid="f73f6cc0-79c8-47b3-9867-ea7390dfee41")
    @TelephonyBaseTest.tel_test_wrap
    def test_voice_call_volte_wcdma_asrvcc(self):
        """ Test Voice call functionality,
        VoLTE to WCDMA aSRVCC
        Steps:
        1. Setup CallBox on VoLTE network with WCDMA.
        2. Turn on DUT and enable VoLTE. Make an voice call to DEFAULT_CALL_NUMBER.
        3. Check if Virtual UA in CSCF server rings.
        4. Handover the call to WCDMA and check if the call is connected.
        5. Tear down the call.

        Expected Results:
        1. Virtual UA is rining.
        2. After aSRVCC, the DEFAULT_CALL_NUMBER call is connected.
        3. Tear down call succeed.

        Returns:
            True if pass; False if fail
        """
        return self._setup_voice_call(
            set_system_model_lte_wcdma,
            self._phone_setup_volte,
            phone_idle_volte,
            srvcc="Alert",
            voice_number=self.voice_call_number)

    @test_tracker_info(uuid="823e8e10-58bd-476d-ba4b-ec436ac424fb")
    @TelephonyBaseTest.tel_test_wrap
    def test_voice_call_volte_gsm_asrvcc(self):
        """ Test Voice call functionality,
        VoLTE to GSM aSRVCC
        Steps:
        1. Setup CallBox on VoLTE network with GSM.
        2. Turn on DUT and enable VoLTE. Make an voice call to DEFAULT_CALL_NUMBER.
        3. Check if Virtual UA in CSCF server rings.
        4. Handover the call to GSM and check if the call is connected.
        5. Tear down the call.

        Expected Results:
        1. Virtual UA is rining.
        2. After aSRVCC, the DEFAULT_CALL_NUMBER call is connected.
        3. Tear down call succeed.

        Returns:
            True if pass; False if fail
        """
        return self._setup_voice_call(
            set_system_model_lte_gsm,
            self._phone_setup_volte,
            phone_idle_volte,
            srvcc="Alert",
            voice_number=self.voice_call_number,
            wait_time_in_call=WAIT_TIME_IN_CALL_FOR_IMS)

    @test_tracker_info(uuid="cd066cb1-6d12-4e29-90b9-f44054f00a00")
    @TelephonyBaseTest.tel_test_wrap
    def test_voice_call_volte_wcdma_asrvcc_mt(self):
        """ Test Voice call functionality,
        MT VoLTE to WCDMA aSRVCC
        Steps:
        1. Setup CallBox on VoLTE network with WCDMA.
        2. Turn on DUT and enable VoLTE. Make a VoLTE call from MD8475A to UE.
        3. Check if Virtual UA in CSCF server calling.
        4. Handover the call to WCDMA and check if the call is connected.
        5. Tear down the call.

        Expected Results:
        1. Virtual UA is rining.
        2. After aSRVCC, the DEFAULT_CALL_NUMBER call is connected.
        3. Tear down call succeed.

        Returns:
            True if pass; False if fail
        """
        return self._setup_voice_call(
            set_system_model_lte_wcdma,
            self._phone_setup_volte,
            phone_idle_volte,
            srvcc="Alert",
            mo=False,
            voice_number=self.voice_call_number)

    @test_tracker_info(uuid="b23ebec3-7e5c-4aca-a749-e34307c56d58")
    @TelephonyBaseTest.tel_test_wrap
    def test_voice_call_volte_gsm_asrvcc_mt(self):
        """ Test Voice call functionality,
        MT VoLTE to GSM aSRVCC
        Steps:
        1. Setup CallBox on VoLTE network with GSM.
        2. Turn on DUT and enable VoLTE. Make a VoLTE call from MD8475A to UE.
        3. Check if Virtual UA in CSCF server calling.
        4. Handover the call to GSM and check if the call is connected.
        5. Tear down the call.

        Expected Results:
        1. Virtual UA is rining.
        2. After aSRVCC, the DEFAULT_CALL_NUMBER call is connected.
        3. Tear down call succeed.

        Returns:
            True if pass; False if fail
        """
        return self._setup_voice_call(
            set_system_model_lte_gsm,
            self._phone_setup_volte,
            phone_idle_volte,
            srvcc="Alert",
            mo=False,
            voice_number=self.voice_call_number,
            wait_time_in_call=WAIT_TIME_IN_CALL_FOR_IMS)

    @test_tracker_info(uuid="b81b3a0e-a7e3-4b30-889f-7c015bdc6980")
    @TelephonyBaseTest.tel_test_wrap
    def test_voice_call_volte(self):
        """ Test Voice call functionality on VoLTE

        Steps:
        1. Setup CallBox on VoLTE network.
        2. Turn on DUT and enable VoLTE. Make an voice call to DEFAULT_CALL_NUMBER.
        3. Make sure Anritsu receives the call and accept.
        4. Tear down the call.

        Expected Results:
        2. Voice call succeed.
        3. Anritsu can accept the call.
        4. Tear down call succeed.

        Returns:
            True if pass; False if fail
        """
        return self._setup_voice_call(
            set_system_model_lte,
            self._phone_setup_volte,
            phone_idle_volte,
            is_ims_call=True,
            voice_number=self.voice_call_number,
            wait_time_in_call=WAIT_TIME_IN_CALL_FOR_IMS)

    """ Tests End """
