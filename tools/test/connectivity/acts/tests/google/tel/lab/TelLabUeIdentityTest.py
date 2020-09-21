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
Tests for reading UE Identity
"""
import time
from acts.controllers.anritsu_lib._anritsu_utils import AnritsuError
from acts.controllers.anritsu_lib.md8475a import MD8475A
from acts.controllers.anritsu_lib.md8475a import UEIdentityType
from acts.test_utils.tel.anritsu_utils import WAIT_TIME_ANRITSU_REG_AND_OPER
from acts.test_utils.tel.anritsu_utils import read_ue_identity
from acts.test_utils.tel.anritsu_utils import set_system_model_gsm
from acts.test_utils.tel.anritsu_utils import set_system_model_lte
from acts.test_utils.tel.anritsu_utils import set_system_model_wcdma
from acts.test_utils.tel.tel_defines import NETWORK_MODE_CDMA
from acts.test_utils.tel.tel_defines import NETWORK_MODE_GSM_ONLY
from acts.test_utils.tel.tel_defines import NETWORK_MODE_GSM_UMTS
from acts.test_utils.tel.tel_defines import NETWORK_MODE_LTE_GSM_WCDMA
from acts.test_utils.tel.tel_defines import RAT_1XRTT
from acts.test_utils.tel.tel_defines import RAT_GSM
from acts.test_utils.tel.tel_defines import RAT_LTE
from acts.test_utils.tel.tel_defines import RAT_WCDMA
from acts.test_utils.tel.tel_defines import RAT_FAMILY_CDMA2000
from acts.test_utils.tel.tel_defines import RAT_FAMILY_GSM
from acts.test_utils.tel.tel_defines import RAT_FAMILY_LTE
from acts.test_utils.tel.tel_defines import RAT_FAMILY_UMTS
from acts.test_utils.tel.tel_test_utils import ensure_network_rat
from acts.test_utils.tel.tel_test_utils import ensure_phones_idle
from acts.test_utils.tel.tel_test_utils import toggle_airplane_mode
from acts.test_utils.tel.TelephonyBaseTest import TelephonyBaseTest


class TelLabUeIdentityTest(TelephonyBaseTest):

    CELL_PARAM_FILE = 'C:\\MX847570\\CellParam\\ACTS\\2cell_param.wnscp'

    def __init__(self, controllers):
        TelephonyBaseTest.__init__(self, controllers)
        self.ad = self.android_devices[0]
        self.md8475a_ip_address = self.user_params[
            "anritsu_md8475a_ip_address"]
        self.ad.sim_card = getattr(self.ad, "sim_card", None)

    def setup_class(self):
        try:
            self.anritsu = MD8475A(self.md8475a_ip_address, self.log)
        except AnritsuError:
            self.log.error("Error in connecting to Anritsu Simulator")
            return False
        return True

    def setup_test(self):
        ensure_phones_idle(self.log, self.android_devices)
        toggle_airplane_mode(self.log, self.ad, True)
        self.ad.adb.shell("setprop net.lte.ims.volte.provisioned 1",
                          ignore_status=True)
        return True

    def teardown_test(self):
        self.log.info("Stopping Simulation")
        self.anritsu.stop_simulation()
        toggle_airplane_mode(self.log, self.ad, True)
        return True

    def teardown_class(self):
        self.anritsu.disconnect()
        return True

    def _read_identity(self, set_simulation_func, rat, identity_type):
        try:
            self.anritsu.reset()
            self.anritsu.load_cell_paramfile(self.CELL_PARAM_FILE)
            set_simulation_func(self.anritsu, self.user_params,
                                self.ad.sim_card)
            self.anritsu.start_simulation()

            if rat == RAT_LTE:
                preferred_network_setting = NETWORK_MODE_LTE_GSM_WCDMA
                rat_family = RAT_FAMILY_LTE
            elif rat == RAT_WCDMA:
                preferred_network_setting = NETWORK_MODE_GSM_UMTS
                rat_family = RAT_FAMILY_UMTS
            elif rat == RAT_GSM:
                preferred_network_setting = NETWORK_MODE_GSM_ONLY
                rat_family = RAT_FAMILY_GSM
            elif rat == RAT_1XRTT:
                preferred_network_setting = NETWORK_MODE_CDMA
                rat_family = RAT_FAMILY_CDMA2000
            else:
                self.log.error("Invalid RAT - Please specify a valid RAT")
                return False

            self.ad.droid.telephonyToggleDataConnection(True)
            if not ensure_network_rat(
                    self.log,
                    self.ad,
                    preferred_network_setting,
                    rat_family,
                    toggle_apm_after_setting=True):
                self.log.error(
                    "Failed to set rat family {}, preferred network:{}".format(
                        rat_family, preferred_network_setting))
                return False
            self.anritsu.wait_for_registration_state()
            time.sleep(WAIT_TIME_ANRITSU_REG_AND_OPER)
            identity = read_ue_identity(self.log, self.ad, self.anritsu,
                                        identity_type)
            if identity is None:
                self.log.error("Phone {} Failed to get {}"
                               .format(self.ad.serial, identity_type.value))
                return False
            else:
                self.log.info("{}".format(identity))
        except AnritsuError as e:
            self.log.error("Error in connection with Anritsu Simulator: " +
                           str(e))
            return False
        except Exception as e:
            self.log.error("Exception during reading identity: " + str(e))
            return False
        return True

    """ Tests Begin """

    @TelephonyBaseTest.tel_test_wrap
    def test_read_imsi_lte(self):
        """Reading the IMSI of UE on LTE

        Reads the IMSI of the UE when device is camped on LTE newtork

        Steps:
        1. Make Sure Phone is camped on LTE network
        2. Send IMSI request from Anritsu
        3. Display the IMSI returned by UE

        Expected Result:
        UE sends the correct IMSI to the Anritsu

        Returns:
            True if pass; False if fail
        """
        return self._read_identity(set_system_model_lte, RAT_LTE,
                                   UEIdentityType.IMSI)

    @TelephonyBaseTest.tel_test_wrap
    def test_read_imei_lte(self):
        """Reading the IMEI of UE on LTE

        Reads the IMEI of the UE when device is camped on LTE newtork

        Steps:
        1. Make Sure Phone is camped on LTE network
        2. Send IMEI request from Anritsu
        3. Display the IMEI returned by UE

        Expected Result:
        UE sends the correct IMEI to the Anritsu

        Returns:
            True if pass; False if fail
        """
        return self._read_identity(set_system_model_lte, RAT_LTE,
                                   UEIdentityType.IMEI)

    @TelephonyBaseTest.tel_test_wrap
    def test_read_imeisv_lte(self):
        """Reading the IMEISV of UE on LTE

        Reads the IMEISV of the UE when device is camped on LTE newtork

        Steps:
        1. Make Sure Phone is camped on LTE network
        2. Send IMEISV request from Anritsu
        3. Display the IMEISV returned by UE

        Expected Result:
        UE sends the correct IMEISV to the Anritsu

        Returns:
            True if pass; False if fail
        """
        return self._read_identity(set_system_model_lte, RAT_LTE,
                                   UEIdentityType.IMEISV)

    @TelephonyBaseTest.tel_test_wrap
    def test_read_imsi_wcdma(self):
        """Reading the IMSI of UE on WCDMA

        Reads the IMSI of the UE when device is camped on WCDMA newtork

        Steps:
        1. Make Sure Phone is camped on WCDMA network
        2. Send IMSI request from Anritsu
        3. Display the IMSI returned by UE

        Expected Result:
        UE sends the correct IMSI to the Anritsu

        Returns:
            True if pass; False if fail
        """
        return self._read_identity(set_system_model_wcdma, RAT_WCDMA,
                                   UEIdentityType.IMSI)

    @TelephonyBaseTest.tel_test_wrap
    def test_read_imei_wcdma(self):
        """Reading the IMEI of UE on WCDMA

        Reads the IMEI of the UE when device is camped on WCDMA newtork

        Steps:
        1. Make Sure Phone is camped on WCDMA network
        2. Send IMEI request from Anritsu
        3. Display the IMEI returned by UE

        Expected Result:
        UE sends the correct IMEI to the Anritsu

        Returns:
            True if pass; False if fail
        """
        return self._read_identity(set_system_model_wcdma, RAT_WCDMA,
                                   UEIdentityType.IMEI)

    @TelephonyBaseTest.tel_test_wrap
    def test_read_imeisv_wcdma(self):
        """Reading the IMEISV of UE on WCDMA

        Reads the IMEISV of the UE when device is camped on WCDMA newtork

        Steps:
        1. Make Sure Phone is camped on WCDMA network
        2. Send IMEISV request from Anritsu
        3. Display the IMEISV returned by UE

        Expected Result:
        UE sends the correct IMEISV to the Anritsu

        Returns:
            True if pass; False if fail
        """
        return self._read_identity(set_system_model_wcdma, RAT_WCDMA,
                                   UEIdentityType.IMEISV)

    @TelephonyBaseTest.tel_test_wrap
    def test_read_imsi_gsm(self):
        """Reading the IMSI of UE on GSM

        Reads the IMSI of the UE when device is camped on GSM newtork

        Steps:
        1. Make Sure Phone is camped on GSM network
        2. Send IMSI request from Anritsu
        3. Display the IMSI returned by UE

        Expected Result:
        UE sends the correct IMSI to the Anritsu

        Returns:
            True if pass; False if fail
        """
        return self._read_identity(set_system_model_gsm, RAT_GSM,
                                   UEIdentityType.IMSI)

    @TelephonyBaseTest.tel_test_wrap
    def test_read_imei_gsm(self):
        """Reading the IMEI of UE on GSM

        Reads the IMEI of the UE when device is camped on GSM newtork

        Steps:
        1. Make Sure Phone is camped on GSM network
        2. Send IMEI request from Anritsu
        3. Display the IMEI returned by UE

        Expected Result:
        UE sends the correct IMEI to the Anritsu

        Returns:
            True if pass; False if fail
        """
        return self._read_identity(set_system_model_gsm, RAT_GSM,
                                   UEIdentityType.IMEI)

    @TelephonyBaseTest.tel_test_wrap
    def test_read_imeisv_gsm(self):
        """Reading the IMEISV of UE on GSM

        Reads the IMEISV of the UE when device is camped on GSM newtork

        Steps:
        1. Make Sure Phone is camped on GSM network
        2. Send IMEISV request from Anritsu
        3. Display the IMEISV returned by UE

        Expected Result:
        UE sends the correct IMEISV to the Anritsu

        Returns:
            True if pass; False if fail
        """
        return self._read_identity(set_system_model_gsm, RAT_GSM,
                                   UEIdentityType.IMEISV)

    """ Tests End """
