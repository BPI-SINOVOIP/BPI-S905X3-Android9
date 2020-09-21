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
    Test Script for Telephony Pre Check In Sanity
"""

import math
import time
from acts.test_decorators import test_tracker_info
from acts.controllers.anritsu_lib._anritsu_utils import AnritsuError
from acts.controllers.anritsu_lib.md8475a import CTCHSetup
from acts.controllers.anritsu_lib.md8475a import BtsBandwidth
from acts.controllers.anritsu_lib.md8475a import BtsPacketRate
from acts.controllers.anritsu_lib.md8475a import BtsServiceState
from acts.controllers.anritsu_lib.md8475a import MD8475A
from acts.controllers.anritsu_lib.mg3710a import MG3710A
from acts.test_utils.tel.anritsu_utils import LTE_BAND_2
from acts.test_utils.tel.anritsu_utils import set_system_model_gsm
from acts.test_utils.tel.anritsu_utils import set_system_model_lte
from acts.test_utils.tel.anritsu_utils import set_system_model_lte_lte
from acts.test_utils.tel.anritsu_utils import set_system_model_lte_wcdma
from acts.test_utils.tel.anritsu_utils import set_system_model_wcdma
from acts.test_utils.tel.anritsu_utils import set_system_model_wcdma_gsm
from acts.test_utils.tel.anritsu_utils import set_system_model_wcdma_wcdma
from acts.test_utils.tel.anritsu_utils import set_usim_parameters
from acts.test_utils.tel.tel_defines import NETWORK_MODE_CDMA
from acts.test_utils.tel.tel_defines import NETWORK_MODE_GSM_ONLY
from acts.test_utils.tel.tel_defines import NETWORK_MODE_GSM_UMTS
from acts.test_utils.tel.tel_defines import NETWORK_MODE_LTE_GSM_WCDMA
from acts.test_utils.tel.tel_defines import RAT_FAMILY_CDMA2000
from acts.test_utils.tel.tel_defines import RAT_FAMILY_GSM
from acts.test_utils.tel.tel_defines import RAT_FAMILY_LTE
from acts.test_utils.tel.tel_defines import RAT_FAMILY_UMTS
from acts.test_utils.tel.tel_test_utils import ensure_network_rat
from acts.test_utils.tel.tel_test_utils import ensure_phones_idle
from acts.test_utils.tel.tel_test_utils import toggle_airplane_mode
from acts.test_utils.tel.TelephonyBaseTest import TelephonyBaseTest
from acts.controllers.anritsu_lib.cell_configurations import \
    gsm_band850_ch128_fr869_cid58_cell
from acts.controllers.anritsu_lib.cell_configurations import \
    gsm_band850_ch251_fr893_cid59_cell
from acts.controllers.anritsu_lib.cell_configurations import \
    gsm_band1900_ch512_fr1930_cid51_cell
from acts.controllers.anritsu_lib.cell_configurations import \
    gsm_band1900_ch512_fr1930_cid52_cell
from acts.controllers.anritsu_lib.cell_configurations import \
    gsm_band1900_ch512_fr1930_cid53_cell
from acts.controllers.anritsu_lib.cell_configurations import \
    gsm_band1900_ch512_fr1930_cid54_cell
from acts.controllers.anritsu_lib.cell_configurations import \
    gsm_band1900_ch640_fr1955_cid56_cell
from acts.controllers.anritsu_lib.cell_configurations import \
    gsm_band1900_ch750_fr1977_cid57_cell
from acts.controllers.anritsu_lib.cell_configurations import \
    lte_band2_ch900_fr1960_pcid9_cell
from acts.controllers.anritsu_lib.cell_configurations import \
    lte_band4_ch2000_fr2115_pcid1_cell
from acts.controllers.anritsu_lib.cell_configurations import \
    lte_band4_ch2000_fr2115_pcid2_cell
from acts.controllers.anritsu_lib.cell_configurations import \
    lte_band4_ch2000_fr2115_pcid3_cell
from acts.controllers.anritsu_lib.cell_configurations import \
    lte_band4_ch2000_fr2115_pcid4_cell
from acts.controllers.anritsu_lib.cell_configurations import \
    lte_band4_ch2050_fr2120_pcid7_cell
from acts.controllers.anritsu_lib.cell_configurations import \
    lte_band4_ch2050_fr2120_pcid7_cell
from acts.controllers.anritsu_lib.cell_configurations import \
    lte_band4_ch2250_fr2140_pcid8_cell
from acts.controllers.anritsu_lib.cell_configurations import \
    lte_band12_ch5095_fr737_pcid10_cell
from acts.controllers.anritsu_lib.cell_configurations import \
    wcdma_band1_ch10700_fr2140_cid31_cell
from acts.controllers.anritsu_lib.cell_configurations import \
    wcdma_band1_ch10700_fr2140_cid32_cell
from acts.controllers.anritsu_lib.cell_configurations import \
    wcdma_band1_ch10700_fr2140_cid33_cell
from acts.controllers.anritsu_lib.cell_configurations import \
    wcdma_band1_ch10700_fr2140_cid34_cell
from acts.controllers.anritsu_lib.cell_configurations import \
    wcdma_band1_ch10575_fr2115_cid36_cell
from acts.controllers.anritsu_lib.cell_configurations import \
    wcdma_band1_ch10800_fr2160_cid37_cell
from acts.controllers.anritsu_lib.cell_configurations import \
    wcdma_band2_ch9800_fr1960_cid38_cell
from acts.controllers.anritsu_lib.cell_configurations import \
    wcdma_band2_ch9900_fr1980_cid39_cell


class TelLabNeighborCellTest(TelephonyBaseTest):

    # This is the default offset between CallBox Power Level and Phone measure
    # Power Level.
    # TODO: Probably need to further tune those values.
    _LTE_RSSI_OFFSET = -39
    _WCDMA_RSSI_OFFSET = -31
    _GSM_RSSI_OFFSET = -30

    _ANRITSU_SETTLING_TIME = 15
    _SETTLING_TIME = 75
    _LTE_MCS_DL = 5
    _LTE_MCS_UL = 5
    _NRB_DL = 50
    _NRB_UL = 50
    _CELL_PARAM_FILE = 'C:\\MX847570\\CellParam\\NEIGHBOR_CELL_TEST_TMO.wnscp'

    # Below are keys should be included in expected cell info.
    # TARGET_RSSI: This is expected RSSI.
    TARGET_RSSI = 'target_rssi'
    # MAX_ERROR_RSSI: This is max error between 'each sample of reported RSSI'
    # and 'expected RSSI'
    MAX_ERROR_RSSI = 'max_error_rssi'
    # MAX_ERROR_AVERAGE_RSSI: This is max error between
    # 'average value of reported RSSI' and 'expected RSSI'
    MAX_ERROR_AVERAGE_RSSI = 'max_error_average_rssi'
    # REPORT_RATE: expected report rate for neighbor cell.
    REPORT_RATE = 'report_rate'
    # RAT: expected network rat.
    RAT = 'rat'
    # IS_REGISTERED: is the cell registered.
    # For serving cell, this value should be True; for neighbor cell, should be
    # False
    IS_REGISTERED = 'registered'
    # CID: cell CID info.
    CID = 'cid'
    # PCID: cell PCID info.
    PCID = 'pcid'
    # PSC: cell PSC info.
    PSC = 'psc'

    # Keys for calculate average RSSI. Only used in _verify_cell_info
    RSSI = 'rssi'
    COUNT = 'count'

    # Pre-defined invalid value. If this value is reported in cell info, just
    # discard this value. E.g. if in reported cell info, cid is reported as
    # 0x7fffffff, need to discard this value when calculating unique_id
    INVALID_VALUE = 0x7fffffff

    def __init__(self, controllers):
        TelephonyBaseTest.__init__(self, controllers)
        self.ad = self.android_devices[0]
        self.ad.sim_card = getattr(self.ad, "sim_card", None)
        self.md8475a_ip_address = self.user_params[
            "anritsu_md8475a_ip_address"]
        self.mg3710a_ip_address = self.user_params[
            "anritsu_mg3710a_ip_address"]
        self.wlan_option = self.user_params.get("anritsu_wlan_option", False)

        if "lte_rssi_offset" in self.user_params:
            self._LTE_RSSI_OFFSET = int(self.user_params["lte_rssi_offset"])
        if "wcdma_rssi_offset" in self.user_params:
            self._WCDMA_RSSI_OFFSET = int(self.user_params[
                "wcdma_rssi_offset"])
        if "gsm_rssi_offset" in self.user_params:
            self._GSM_RSSI_OFFSET = int(self.user_params["gsm_rssi_offset"])

    def setup_class(self):
        self.md8475a = None
        self.mg3710a = None
        try:
            self.md8475a = MD8475A(self.md8475a_ip_address, self.log)
        except AnritsuError as e:
            self.log.error("Error in connecting to Anritsu MD8475A:{}".format(
                e))
            return False

        try:
            self.mg3710a = MG3710A(self.mg3710a_ip_address, self.log)
        except AnritsuError as e:
            self.log.error("Error in connecting to Anritsu MG3710A :{}".format(
                e))
            return False
        return True

    def setup_test(self):
        self.turn_off_3710a_sg(1)
        self.turn_off_3710a_sg(2)
        self.mg3710a.set_arb_pattern_aorb_state("A", "OFF", 1)
        self.mg3710a.set_arb_pattern_aorb_state("B", "OFF", 1)
        self.mg3710a.set_arb_pattern_aorb_state("A", "OFF", 2)
        self.mg3710a.set_arb_pattern_aorb_state("B", "OFF", 2)
        self.mg3710a.set_freq_relative_display_status("OFF", 1)
        self.mg3710a.set_freq_relative_display_status("OFF", 2)
        self.ad.droid.telephonySetPreferredNetworkTypes(
            NETWORK_MODE_LTE_GSM_WCDMA)
        ensure_phones_idle(self.log, self.android_devices)
        toggle_airplane_mode(self.log, self.ad, True)
        self.ad.droid.telephonyToggleDataConnection(True)
        self.ad.adb.shell("setprop net.lte.ims.volte.provisioned 1",
                          ignore_status=True)
        return True

    def teardown_test(self):
        toggle_airplane_mode(self.log, self.ad, True)
        self.turn_off_3710a_sg(1)
        self.turn_off_3710a_sg(2)
        self.log.info("Stopping Simulation")
        self.md8475a.stop_simulation()
        return True

    def teardown_class(self):
        if self.md8475a is not None:
            self.md8475a.disconnect()
        if self.mg3710a is not None:
            self.mg3710a.disconnect()
        return True

    def _setup_lte_serving_cell(self, bts, dl_power, cell_id, physical_cellid):
        bts.output_level = dl_power
        bts.bandwidth = BtsBandwidth.LTE_BANDWIDTH_10MHz
        bts.packet_rate = BtsPacketRate.LTE_MANUAL
        bts.lte_mcs_dl = self._LTE_MCS_DL
        bts.lte_mcs_ul = self._LTE_MCS_UL
        bts.nrb_dl = self._NRB_DL
        bts.nrb_ul = self._NRB_UL
        bts.cell_id = cell_id
        bts.physical_cellid = physical_cellid
        bts.neighbor_cell_mode = "DEFAULT"

    def _setup_lte_neighbhor_cell_md8475a(self, bts, band, dl_power, cell_id,
                                          physical_cellid):
        bts.output_level = dl_power
        bts.band = band
        bts.bandwidth = BtsBandwidth.LTE_BANDWIDTH_10MHz
        bts.cell_id = cell_id
        bts.physical_cellid = physical_cellid
        bts.neighbor_cell_mode = "DEFAULT"
        bts.packet_rate = BtsPacketRate.LTE_MANUAL
        bts.lte_mcs_dl = self._LTE_MCS_DL
        bts.lte_mcs_ul = self._LTE_MCS_UL
        bts.nrb_dl = self._NRB_DL
        bts.nrb_ul = self._NRB_UL

    def _setup_wcdma_serving_cell(self, bts, dl_power, cell_id):
        bts.output_level = dl_power
        bts.cell_id = cell_id
        bts.neighbor_cell_mode = "DEFAULT"

    def _setup_wcdma_neighbhor_cell_md8475a(self, bts, band, dl_power,
                                            cell_id):
        bts.output_level = dl_power
        bts.band = band
        bts.cell_id = cell_id
        bts.neighbor_cell_mode = "DEFAULT"

    def _setup_lte_cell_md8475a(self, bts, params, dl_power):
        bts.output_level = dl_power
        bts.band = params['band']
        bts.bandwidth = params['bandwidth']
        bts.cell_id = params['cid']
        bts.physical_cellid = params['pcid']
        bts.mcc = params['mcc']
        bts.mnc = params['mnc']
        bts.tac = params['tac']
        bts.neighbor_cell_mode = "DEFAULT"
        bts.dl_channel = params['channel']
        bts.packet_rate = BtsPacketRate.LTE_MANUAL
        bts.lte_mcs_dl = self._LTE_MCS_DL
        bts.lte_mcs_ul = self._LTE_MCS_UL
        bts.nrb_dl = self._NRB_DL
        bts.nrb_ul = self._NRB_UL

    def _setup_wcdma_cell_md8475a(self, bts, params, dl_power):
        bts.output_level = dl_power
        bts.band = params['band']
        bts.cell_id = params['cid']
        bts.mcc = params['mcc']
        bts.mnc = params['mnc']
        bts.lac = params['lac']
        bts.rac = params['rac']
        bts.neighbor_cell_mode = "DEFAULT"
        bts.primary_scrambling_code = params['psc']
        bts.dl_channel = params['channel']

    def _setup_gsm_cell_md8475a(self, bts, params, dl_power):
        bts.output_level = params['power']
        bts.band = params['band']
        bts.cell_id = params['cid']
        bts.mcc = params['mcc']
        bts.mnc = params['mnc']
        bts.lac = params['lac']
        bts.rac = params['rac']
        bts.neighbor_cell_mode = "DEFAULT"

    def setup_3710a_waveform(self, sg_number, memory, frequency, power_level,
                             wave_package_name, wave_pattern_name):
        self.mg3710a.set_frequency(frequency, sg_number)
        self.mg3710a.set_arb_state("ON", sg_number)
        self.mg3710a.set_arb_combination_mode("EDIT", sg_number)
        self.mg3710a.select_waveform(wave_package_name, wave_pattern_name,
                                     memory, sg_number)
        self.mg3710a.set_arb_pattern_aorb_state(memory, "ON", sg_number)
        self.mg3710a.set_arb_level_aorb(memory, power_level, sg_number)

    def turn_on_3710a_sg(self, sg_number):
        self.mg3710a.set_modulation_state("ON", sg_number)
        self.mg3710a.set_rf_output_state("ON", sg_number)

    def turn_off_3710a_sg(self, sg_number):
        self.mg3710a.set_modulation_state("OFF", sg_number)
        self.mg3710a.set_rf_output_state("OFF", sg_number)

    def _is_matching_cell(self, expected_cell_info, input_cell_info):
        """Return if 'input_cell_info' matches 'expected_cell_info'.

        Args:
            expected_cell_info: expected cell info. (dictionary)
            input_cell_info: input cell info to test. (dictionary)

        Returns:
            True if:
                for each key in key_list, if key exist in expected_cell_info,
                it should also exist in input_cell_info, and the values should
                equal in expected_cell_info and input_cell_info
            False otherwise.
        """
        for key in [
                self.CID, self.PCID, self.RAT, self.PSC, self.IS_REGISTERED
        ]:
            if key in expected_cell_info:
                if key not in input_cell_info:
                    return False
                if input_cell_info[key] != expected_cell_info[key]:
                    return False
        return True

    def _unique_cell_id(self, input_cell_info):
        """Get the unique id for cell_info, based on cid, pcid, rat, psc and
        is_registered.

        Args:
            input_cell_info: cell info to get unique id.

        Returns:
            unique id (string)
        """
        unique_id = ""
        for key in [
                self.CID, self.PCID, self.RAT, self.PSC, self.IS_REGISTERED
        ]:
            if key in input_cell_info:
                if input_cell_info[key] != self.INVALID_VALUE:
                    unique_id += key + ":" + str(input_cell_info[key]) + ":"
        return unique_id

    def _get_rssi_from_cell_info(self, cell_info):
        """Return the RSSI value reported in cell_info.

        Args:
            cell_info: cell info to get RSSI.

        Returns:
            RSSI reported in this cell info.
        """
        rat_to_rssi_tbl = {
            'lte': 'rsrp',
            'wcdma': 'signal_strength',
            'gsm': 'signal_strength'
        }
        try:
            return cell_info[rat_to_rssi_tbl[cell_info[self.RAT]]]
        except KeyError:
            return None

    def _is_rssi_in_expected_range(self, actual_rssi, expected_rssi,
                                   max_error):
        """Return if actual_rssi is within expected range.

        Args:
            actual_rssi: the rssi value to test.
            expected_rssi: expected rssi value.
            max_error: max error.

        Returns:
            if the difference between actual_rssi and expected_rssi is within
            max_error, return True. Otherwise False.
        """
        if abs(actual_rssi - expected_rssi) > max_error:
            self.log.error(
                "Expected RSSI: {}, max error: {}, reported RSSI: {}".format(
                    expected_rssi, max_error, actual_rssi))
            return False
        else:
            return True

    def _verify_cell_info(self,
                          ad,
                          expected_cell_info_stats,
                          number_of_sample=10,
                          delay_each_sample=5):
        """Return if reported cell info from ad matches expected_cell_info_stats
        or not.

        Args:
            ad: android device object.
            expected_cell_info_stats: expected cell info list.
            number_of_sample: number of sample to take from DUT.
            delay_each_sample: time delay between each sample.

        Returns:
            True if reported cell info matches with expected_cell_info_stats.
            False otherwise.

        """

        cell_info_stats = {}

        for index in range(number_of_sample):
            info_list = ad.droid.telephonyGetAllCellInfo()

            self.log.info("Received Cell Info List: {}".format(info_list))

            for sample in info_list:
                rssi = self._get_rssi_from_cell_info(sample)
                unique_id = self._unique_cell_id(sample)

                # check cell logic
                if not unique_id in expected_cell_info_stats:
                    self.log.error("Found unexpected cell!")
                    return False
                elif not self._is_matching_cell(
                        expected_cell_info_stats[unique_id], sample):
                    self.log.error("Mismatched Cell Info")
                    return False

                # Check RSSI within expected range
                if not self._is_rssi_in_expected_range(
                        self._get_rssi_from_cell_info(sample),
                        expected_cell_info_stats[unique_id][self.TARGET_RSSI],
                        expected_cell_info_stats[unique_id][
                            self.MAX_ERROR_RSSI]):
                    self.log.error("Cell Info: {}. Cell RSSI not" +
                                   " in expected range".format(sample))
                    return False
                if unique_id not in cell_info_stats:
                    cell_info_stats[unique_id] = {self.RSSI: 0, self.COUNT: 0}
                cell_info_stats[unique_id][self.RSSI] += rssi
                cell_info_stats[unique_id][self.COUNT] += 1

            time.sleep(delay_each_sample)

        try:
            for unique_id in expected_cell_info_stats.keys():
                expected_cell_info = expected_cell_info_stats[unique_id]

                expected_number_of_sample_reported = math.floor(
                    expected_cell_info[self.REPORT_RATE] * number_of_sample)

                if cell_info_stats[unique_id][
                        self.COUNT] < expected_number_of_sample_reported:
                    self.log.error(
                        "Insufficient reports {}/{} for {}, expected: {}",
                        expected_cell_info[unique_id][self.COUNT],
                        number_of_sample, expected_cell_info,
                        expected_number_of_sample_reported)
                    return False

                average_rssi = cell_info_stats[unique_id][self.RSSI] / \
                               cell_info_stats[unique_id][self.COUNT]

                # Check Average RSSI within expected range
                if not self._is_rssi_in_expected_range(
                        average_rssi, expected_cell_info[self.TARGET_RSSI],
                        expected_cell_info[self.MAX_ERROR_AVERAGE_RSSI]):
                    self.log.error("Cell Average RSSI not in expected range.")
                    return False
        except KeyError as unique_id:
            self.log.error("Failed to find key {}".format(unique_id))
            self.log.error("Expected cell info not reported {}.".format(
                expected_cell_info))
            return False

        return True

    def lte_intra_freq_ncell(self,
                             ad,
                             pcid_list,
                             init_pwr,
                             power_seq,
                             interval=3,
                             err_margin=1):
        """Return True if UE measured RSSI follows the powers in BTS simulator

        Args:
            ad: android device object.
            pcid: list of PCID of LTE BTS
            init_pwr: initial downlinl power in dBm for serving and neighbor cell
            power_seq: power change sequence in dB.
            interval: time delay in seocnd between each power change.
            error_margin: error margin in dB to determine measurement pass or fail

        Returns:
            True if all measurments are within margin.
            False otherwise.

        """
        bts = set_system_model_lte_lte(self.md8475a, self.user_params,
                                       self.ad.sim_card)

        self._setup_lte_serving_cell(bts[0], init_pwr, pcid_list[0],
                                     pcid_list[0])
        self._setup_lte_neighbhor_cell_md8475a(bts[1], LTE_BAND_2, init_pwr,
                                               pcid_list[1], pcid_list[1])
        set_usim_parameters(self.md8475a, self.ad.sim_card)
        self.md8475a.send_command("IMSSTARTVN 1")
        self.md8475a.start_simulation()
        bts[1].service_state = BtsServiceState.SERVICE_STATE_OUT
        self.ad.droid.telephonyToggleDataConnection(True)
        if not ensure_network_rat(
                self.log,
                self.ad,
                NETWORK_MODE_LTE_GSM_WCDMA,
                RAT_FAMILY_LTE,
                toggle_apm_after_setting=True):
            self.log.error("Failed to set rat family {}, preferred network:{}".
                           format(RAT_FAMILY_LTE, NETWORK_MODE_LTE_GSM_WCDMA))
            return False
        self.md8475a.wait_for_registration_state()
        time.sleep(1)
        bts[1].service_state = BtsServiceState.SERVICE_STATE_IN

        n_bts = len(pcid_list)  # number of total BTS
        if n_bts >= 3:
            self.setup_3710a_waveform("1", "A", "1960MHZ", init_pwr, "LTE",
                                      "10M_B1_CID2")
            self.turn_on_3710a_sg(1)
        if n_bts == 4:
            self.setup_3710a_waveform("1", "B", "1960MHZ", init_pwr, "LTE",
                                      "10M_B1_CID3")
        self.log.info("Wait for {} seconds to settle".format(
            self._ANRITSU_SETTLING_TIME))
        time.sleep(self._ANRITSU_SETTLING_TIME)
        cell_list = ad.droid.telephonyGetAllCellInfo()
        self.log.info("Received Cell Info List: {}".format(cell_list))
        init_rsrp_list = []
        for pcid in pcid_list:
            for cell in cell_list:
                if cell['pcid'] == pcid:
                    init_rsrp_list.append(cell['rsrp'])
                    break
        self.log.info("init_rsrp_list = {}".format(init_rsrp_list))
        error_seq = []
        for power in power_seq:
            self.log.info("power = {}".format(power))
            for i in range(2):
                bts[i].output_level = init_pwr + power[i]
            if n_bts >= 3:
                self.mg3710a.set_arb_level_aorb("A", init_pwr + power[2], "1")
            if n_bts == 4:
                self.mg3710a.set_arb_level_aorb("B", init_pwr + power[2], "1")
            time.sleep(interval)
            cell_list = ad.droid.telephonyGetAllCellInfo()
            delta = []
            error = []
            for pcid, init_rsrp, pwr in zip(pcid_list, init_rsrp_list, power):
                found = False
                for cell in cell_list:
                    if cell['pcid'] == pcid:
                        found = True
                        self.log.info("pcid {}, rsrp = {}".format(pcid, cell[
                            'rsrp']))
                        delta.append(cell['rsrp'] - init_rsrp)
                        error.append(cell['rsrp'] - init_rsrp - pwr)
                if not found:
                    self.log.info("pcid {} not found!".format(pcid))
                    delta.append(-99)
                    error.append(-99)
            self.log.info("delta = {}".format(delta))
            self.log.info("error = {}".format(error))
            error_seq.append(error)
        self.log.info("error_seq = {}".format(error_seq))
        for error in error_seq:
            for err in error:
                if err != -99 and abs(err) > err_margin:
                    self.log.error(
                        "Test failed! Measured power error is greater than margin."
                    )
                    return False
        return True

    """ Tests Begin """

    @test_tracker_info(uuid="17a42861-abb5-480b-9139-89219fa304b2")
    @TelephonyBaseTest.tel_test_wrap
    def test_2lte_intra_freq_ncell_away_close(self):
        """ Test phone moving away from Neighbor Intra Freq cell then
        close back while serving cell stays the same power

        Setup a two LTE cell configuration on MD8475A
        Make Sure Phone is in LTE mode
        Verify the reported RSSI follows the DL power change in MD8475A

        Returns:
            True if pass; False if fail
        """
        pcid = [0, 1]
        init_pwr = -30  # initial DL power for all cells
        power_seq = [
            [0, -1],  # power change sequence reference to init_pwr
            [0, -2],
            [0, -3],
            [0, -4],
            [0, -3],
            [0, -2],
            [0, -1],
            [0, 0],
            [0, 1],
            [0, 3],
            [0, 4],
            [0, 5],
            [0, 6],
            [0, 7],
            [0, 8],
            [0, 9],
            [0, 10],
            [0, 9],
            [0, 8],
            [0, 7],
            [0, 6],
            [0, 5],
            [0, 4],
            [0, 3],
            [0, 2],
            [0, 1],
            [0, 0]
        ]

        return self.lte_intra_freq_ncell(self.ad, pcid, init_pwr, power_seq)

    @test_tracker_info(uuid="117f404b-fb78-474a-86ba-209e6a54c9a8")
    @TelephonyBaseTest.tel_test_wrap
    def test_2lte_intra_freq_scell_away_close(self):
        """ Test phone moving away from serving cell then close back while
        neighbor Intra Freq cell stays the same power

        Setup a two LTE cell configuration on MD8475A
        Make Sure Phone is in LTE mode
        Verify the reported RSSI follows the DL power change in MD8475A

        Returns:
            True if pass; False if fail
        """
        pcid = [0, 1]
        init_pwr = -30  # initial DL power for all cells
        power_seq = [
            [-1, 0],  # power change sequence reference to init_pwr
            [-2, 0],
            [-3, 0],
            [-4, 0],
            [-5, 0],
            [-6, 0],
            [-7, 0],
            [-8, 0],
            [-9, 0],
            [-10, 0],
            [-9, 0],
            [-8, 0],
            [-7, 0],
            [-6, 0],
            [-5, 0],
            [-4, 0],
            [-3, 0],
            [-2, 0],
            [-1, 0],
            [0, 0],
            [1, 0],
            [2, 0],
            [3, 0],
            [4, 0],
            [3, 0],
            [2, 0],
            [1, 0],
            [0, 0]
        ]

        return self.lte_intra_freq_ncell(self.ad, pcid, init_pwr, power_seq)

    @test_tracker_info(uuid="d1eec95f-40e9-4099-a669-9a88e56049ca")
    @TelephonyBaseTest.tel_test_wrap
    def test_2lte_intra_freq_ncell_away_close_2(self):
        """ Test phone moving away from serving cell and close to neighbor
        Intra Freq cell, then back and forth

        Setup a two LTE cell configuration on MD8475A
        Make Sure Phone is in LTE mode
        Verify the reported RSSI follows the DL power change in MD8475A

        Returns:
            True if pass; False if fail
        """
        pcid = [0, 1]
        init_pwr = -30  # initial DL power for all cells
        power_seq = [
            [-1, 1],  # power change sequence reference to init_pwr
            [-2, 2],
            [-3, 3],
            [-4, 4],
            [-5, 5],
            [-4, 4],
            [-3, 3],
            [-2, 2],
            [-1, 1],
            [-0, 0],
            [1, -1],
            [2, -2],
            [1, -1],
            [0, 0]
        ]

        return self.lte_intra_freq_ncell(self.ad, pcid, init_pwr, power_seq)

    @test_tracker_info(uuid="c642a85b-4970-429c-81c4-f635392879be")
    @TelephonyBaseTest.tel_test_wrap
    def test_2lte_intra_freq_2cell_synced(self):
        """ Test phone moving away and back to both serving cell and neighbor
        Intra Freq cell

        Setup a two LTE cell configuration on MD8475A
        Make Sure Phone is in LTE mode
        Verify the reported RSSI follows the DL power change in MD8475A and MG3710A

        Returns:
            True if pass; False if fail
        """
        pcid = [0, 1]
        init_pwr = -30  # initial DL power for all cells
        power_seq = [
            [-1, -1],  # power change sequence reference to init_pwr
            [-3, -3],
            [-5, -5],
            [-7, -7],
            [-5, -5],
            [-3, -3],
            [-1, -1],
            [1, 1],
            [3, 3],
            [5, 5],
            [7, 7],
            [3, 3],
            [0, 0]
        ]

        return self.lte_intra_freq_ncell(self.ad, pcid, init_pwr, power_seq)

    @test_tracker_info(uuid="9144fab6-c7e1-4de2-a01d-7a15c117ec70")
    @TelephonyBaseTest.tel_test_wrap
    def test_3lte_intra_freq_scell_reversed(self):
        """ Test phone moving away and back between 2 neighbor cells while maintain
        same rssi with serving cell

        Setup a two LTE cell configuration on MD8475A
        Make Sure Phone is in LTE mode
        Verify the reported RSSI follows the DL power change in MD8475A and MG3710A

        Returns:
            True if pass; False if fail
        """
        pcid = [0, 1, 2]
        init_pwr = -30  # initial DL power for all cells
        power_seq = [
            [0, 1, -1],  # power change sequence reference to init_pwr
            [0, 2, -2],
            [0, 3, -3],
            [0, 4, -4],
            [0, 3, -3],
            [0, 2, -2],
            [0, 1, -1],
            [0, 0, 0],
            [0, -1, 1],
            [0, -2, 2],
            [0, -3, 3],
            [0, -4, 4],
            [0, -3, 3],
            [0, -2, 2],
            [0, -1, 1],
            [0, 0, 0]
        ]

        return self.lte_intra_freq_ncell(self.ad, pcid, init_pwr, power_seq)

    @test_tracker_info(uuid="7bfbea72-e6fa-45ae-bf7e-b9b42063abe7")
    @TelephonyBaseTest.tel_test_wrap
    def test_3lte_intra_freq_3cell_synced(self):
        """ Test phone moving away and back to both serving cell and neighbor
        Intra Freq cell

        Setup a two LTE cell configuration on MD8475A
        Make Sure Phone is in LTE mode
        Verify the reported RSSI follows the DL power change in MD8475A

        Returns:
            True if pass; False if fail
        """
        pcid = [0, 1, 2]
        init_pwr = -30  # initial DL power for all cells
        power_seq = [
            [-1, -1, -1],  # power change sequence reference to init_pwr
            [-3, -3, -3],
            [-5, -5, -5],
            [-7, -7, -7],
            [-5, -5, -5],
            [-3, -3, -3],
            [-1, -1, -1],
            [1, 1, 1],
            [3, 3, 3],
            [5, 5, 5],
            [7, 7, 7],
            [3, 3, 3],
            [0, 0, 0]
        ]

        return self.lte_intra_freq_ncell(self.ad, pcid, init_pwr, power_seq)

    @test_tracker_info(uuid="b4577ae1-6435-4a15-9449-e02013dfb032")
    @TelephonyBaseTest.tel_test_wrap
    def test_ncells_intra_lte_0_cells(self):
        """ Test Number of neighbor cells reported by Phone when no neighbor
        cells are present (Phone camped on LTE)

        Setup a single LTE cell configuration on MD8475A
        Make Sure Phone is in LTE mode
        Verify the number of neighbor cells reported by Phone

        Returns:
            True if pass; False if fail
        """
        serving_cell_cid = 11
        serving_cell_pcid = 11
        serving_cell_dlpower = -20
        expected_cell_info_list = [
            # Serving Cell
            {
                self.CID: serving_cell_cid,
                self.PCID: serving_cell_pcid,
                self.REPORT_RATE: 1,
                self.IS_REGISTERED: True,
                self.RAT: 'lte',
                self.TARGET_RSSI: self._LTE_RSSI_OFFSET + serving_cell_dlpower,
                self.MAX_ERROR_RSSI: 3,
                self.MAX_ERROR_AVERAGE_RSSI: 3
            }
        ]
        expected_cell_info_stats = {}
        for sample in expected_cell_info_list:
            expected_cell_info_stats[self._unique_cell_id(sample)] = sample

        [bts1] = set_system_model_lte(self.md8475a, self.user_params,
                                      self.ad.sim_card)
        self._setup_lte_serving_cell(bts1, serving_cell_dlpower,
                                     serving_cell_cid, serving_cell_pcid)
        set_usim_parameters(self.md8475a, self.ad.sim_card)
        self.md8475a.start_simulation()

        if not ensure_network_rat(
                self.log,
                self.ad,
                NETWORK_MODE_LTE_GSM_WCDMA,
                RAT_FAMILY_LTE,
                toggle_apm_after_setting=True):
            self.log.error("Failed to set rat family {}, preferred network:{}".
                           format(RAT_FAMILY_LTE, NETWORK_MODE_LTE_GSM_WCDMA))
            return False
        self.md8475a.wait_for_registration_state()
        time.sleep(self._SETTLING_TIME)
        return self._verify_cell_info(self.ad, expected_cell_info_stats)

    @test_tracker_info(uuid="fe2cc07b-9676-41ab-b7ff-112d3ef84980")
    @TelephonyBaseTest.tel_test_wrap
    def test_ncells_intra_lte_1_cells(self):
        """ Test Number of neighbor cells reported by Phone when one neighbor
        cell is present (Phone camped on LTE)

        Setup a two LTE cell configuration on MD8475A
        Make Sure Phone is in LTE mode
        Verify the number of neighbor cells reported by Phone

        Returns:
            True if pass; False if fail
        """
        serving_cell_cid = 11
        serving_cell_pcid = 11
        neigh_cell_cid = 22
        neigh_cell_pcid = 22
        serving_cell_dlpower = -20
        neigh_cell_dlpower = -24

        expected_cell_info_list = [
            # Serving Cell
            {
                self.CID: serving_cell_cid,
                self.PCID: serving_cell_pcid,
                self.REPORT_RATE: 1,
                self.IS_REGISTERED: True,
                self.RAT: 'lte',
                self.TARGET_RSSI: self._LTE_RSSI_OFFSET + serving_cell_dlpower,
                self.MAX_ERROR_RSSI: 3,
                self.MAX_ERROR_AVERAGE_RSSI: 3
            },
            # Neighbor Cells
            {
                self.PCID: neigh_cell_pcid,
                self.REPORT_RATE: 0.1,
                self.IS_REGISTERED: False,
                self.RAT: 'lte',
                self.TARGET_RSSI: self._LTE_RSSI_OFFSET + neigh_cell_dlpower,
                self.MAX_ERROR_RSSI: 4,
                self.MAX_ERROR_AVERAGE_RSSI: 3
            }
        ]
        expected_cell_info_stats = {}
        for sample in expected_cell_info_list:
            expected_cell_info_stats[self._unique_cell_id(sample)] = sample

        [bts1, bts2] = set_system_model_lte_lte(self.md8475a, self.user_params,
                                                self.ad.sim_card)

        self._setup_lte_serving_cell(bts1, serving_cell_dlpower,
                                     serving_cell_cid, serving_cell_pcid)
        self._setup_lte_neighbhor_cell_md8475a(bts2, LTE_BAND_2,
                                               neigh_cell_dlpower,
                                               neigh_cell_cid, neigh_cell_pcid)
        set_usim_parameters(self.md8475a, self.ad.sim_card)
        self.md8475a.send_command("IMSSTARTVN 1")
        self.md8475a.start_simulation()
        bts2.service_state = BtsServiceState.SERVICE_STATE_OUT

        if not ensure_network_rat(
                self.log,
                self.ad,
                NETWORK_MODE_LTE_GSM_WCDMA,
                RAT_FAMILY_LTE,
                toggle_apm_after_setting=True):
            self.log.error("Failed to set rat family {}, preferred network:{}".
                           format(RAT_FAMILY_LTE, NETWORK_MODE_LTE_GSM_WCDMA))
            return False
        self.md8475a.wait_for_registration_state()
        bts2.service_state = BtsServiceState.SERVICE_STATE_IN
        self.log.info("Wait for {} seconds to settle".format(
            self._SETTLING_TIME))
        time.sleep(self._SETTLING_TIME)
        return self._verify_cell_info(self.ad, expected_cell_info_stats)

    @test_tracker_info(uuid="8abc7903-4ea7-407a-946b-455d7f767c3e")
    @TelephonyBaseTest.tel_test_wrap
    def test_ncells_intra_lte_2_cells(self):
        """ Test Number of neighbor cells reported by Phone when two neighbor
        cells are present (Phone camped on LTE)

        Setup a two LTE cell configuration on MD8475A
        Setup one waveform on MG3710A
        Make Sure Phone is in LTE mode
        Verify the number of neighbor cells reported by Phone

        Returns:
            True if pass; False if fail
        """
        serving_cell_cid = 11
        serving_cell_pcid = 11
        neigh_cell_1_cid = 22
        neigh_cell_1_pcid = 22
        neigh_cell_2_cid = 1
        neigh_cell_2_pcid = 1
        serving_cell_dlpower = -20
        neigh_cell_1_dlpower = -24
        neigh_cell_2_dlpower = -23

        expected_cell_info_list = [
            # Serving Cell
            {
                self.CID: serving_cell_cid,
                self.PCID: serving_cell_pcid,
                self.REPORT_RATE: 1,
                self.IS_REGISTERED: True,
                self.RAT: 'lte',
                self.TARGET_RSSI: self._LTE_RSSI_OFFSET + serving_cell_dlpower,
                self.MAX_ERROR_RSSI: 3,
                self.MAX_ERROR_AVERAGE_RSSI: 3
            },
            # Neighbor Cells
            {
                self.PCID: neigh_cell_1_pcid,
                self.REPORT_RATE: 0.1,
                self.IS_REGISTERED: False,
                self.RAT: 'lte',
                self.TARGET_RSSI: self._LTE_RSSI_OFFSET + neigh_cell_1_dlpower,
                self.MAX_ERROR_RSSI: 4,
                self.MAX_ERROR_AVERAGE_RSSI: 3
            },
            {
                self.PCID: neigh_cell_2_pcid,
                self.REPORT_RATE: 0.1,
                self.IS_REGISTERED: False,
                self.RAT: 'lte',
                self.TARGET_RSSI: self._LTE_RSSI_OFFSET + neigh_cell_2_dlpower,
                self.MAX_ERROR_RSSI: 4,
                self.MAX_ERROR_AVERAGE_RSSI: 3
            }
        ]
        expected_cell_info_stats = {}
        for sample in expected_cell_info_list:
            expected_cell_info_stats[self._unique_cell_id(sample)] = sample

        [bts1, bts2] = set_system_model_lte_lte(self.md8475a, self.user_params,
                                                self.ad.sim_card)

        self._setup_lte_serving_cell(bts1, serving_cell_dlpower,
                                     serving_cell_cid, serving_cell_pcid)
        self._setup_lte_neighbhor_cell_md8475a(
            bts2, LTE_BAND_2, neigh_cell_1_dlpower, neigh_cell_1_cid,
            neigh_cell_1_pcid)
        set_usim_parameters(self.md8475a, self.ad.sim_card)
        self.md8475a.start_simulation()
        if not ensure_network_rat(
                self.log,
                self.ad,
                NETWORK_MODE_LTE_GSM_WCDMA,
                RAT_FAMILY_LTE,
                toggle_apm_after_setting=True):
            self.log.error("Failed to set rat family {}, preferred network:{}".
                           format(RAT_FAMILY_LTE, NETWORK_MODE_LTE_GSM_WCDMA))
            return False
        self.md8475a.wait_for_registration_state()

        self.setup_3710a_waveform("1", "A", "1960MHZ", neigh_cell_2_dlpower,
                                  "LTE", "10M_B1_CID1")
        self.turn_on_3710a_sg(1)
        time.sleep(self._SETTLING_TIME)
        return self._verify_cell_info(self.ad, expected_cell_info_stats)

    @test_tracker_info(uuid="623b3d16-bc48-4353-abc3-054ca6351a97")
    @TelephonyBaseTest.tel_test_wrap
    def test_ncells_intra_lte_3_cells(self):
        """ Test Number of neighbor cells reported by Phone when three neighbor
        cells are present (Phone camped on LTE)

        Setup two LTE cell configuration on MD8475A
        Setup two waveform on MG3710A
        Make Sure Phone is in LTE mode
        Verify the number of neighbor cells reported by Phone

        Returns:
            True if pass; False if fail
        """
        serving_cell_cid = 11
        serving_cell_pcid = 11
        neigh_cell_1_cid = 1
        neigh_cell_1_pcid = 1
        neigh_cell_2_cid = 2
        neigh_cell_2_pcid = 2
        neigh_cell_3_cid = 3
        neigh_cell_3_pcid = 3
        serving_cell_dlpower = -20
        neigh_cell_1_dlpower = -24
        neigh_cell_2_dlpower = -22
        neigh_cell_3_dlpower = -23

        expected_cell_info_list = [
            # Serving Cell
            {
                self.CID: serving_cell_cid,
                self.PCID: serving_cell_pcid,
                self.REPORT_RATE: 1,
                self.IS_REGISTERED: True,
                self.RAT: 'lte',
                self.TARGET_RSSI: self._LTE_RSSI_OFFSET + serving_cell_dlpower,
                self.MAX_ERROR_RSSI: 3,
                self.MAX_ERROR_AVERAGE_RSSI: 3
            },
            # Neighbor Cells
            {
                self.PCID: neigh_cell_1_pcid,
                self.REPORT_RATE: 0.1,
                self.IS_REGISTERED: False,
                self.RAT: 'lte',
                self.TARGET_RSSI: self._LTE_RSSI_OFFSET + neigh_cell_1_dlpower,
                self.MAX_ERROR_RSSI: 4,
                self.MAX_ERROR_AVERAGE_RSSI: 3
            },
            {
                self.PCID: neigh_cell_2_pcid,
                self.REPORT_RATE: 0.1,
                self.IS_REGISTERED: False,
                self.RAT: 'lte',
                self.TARGET_RSSI: self._LTE_RSSI_OFFSET + neigh_cell_2_dlpower,
                self.MAX_ERROR_RSSI: 4,
                self.MAX_ERROR_AVERAGE_RSSI: 3
            },
            {
                self.PCID: neigh_cell_3_pcid,
                self.REPORT_RATE: 0.1,
                self.IS_REGISTERED: False,
                self.RAT: 'lte',
                self.TARGET_RSSI: self._LTE_RSSI_OFFSET + neigh_cell_3_dlpower,
                self.MAX_ERROR_RSSI: 4,
                self.MAX_ERROR_AVERAGE_RSSI: 3
            }
        ]
        expected_cell_info_stats = {}
        for sample in expected_cell_info_list:
            expected_cell_info_stats[self._unique_cell_id(sample)] = sample
        [bts1, bts2] = set_system_model_lte_lte(self.md8475a, self.user_params,
                                                self.ad.sim_card)
        self._setup_lte_serving_cell(bts1, serving_cell_dlpower,
                                     serving_cell_cid, serving_cell_pcid)

        self._setup_lte_neighbhor_cell_md8475a(
            bts2, LTE_BAND_2, neigh_cell_1_dlpower, neigh_cell_1_cid,
            neigh_cell_1_pcid)
        set_usim_parameters(self.md8475a, self.ad.sim_card)
        self.md8475a.start_simulation()
        if not ensure_network_rat(
                self.log,
                self.ad,
                NETWORK_MODE_LTE_GSM_WCDMA,
                RAT_FAMILY_LTE,
                toggle_apm_after_setting=True):
            self.log.error("Failed to set rat family {}, preferred network:{}".
                           format(RAT_FAMILY_LTE, NETWORK_MODE_LTE_GSM_WCDMA))
            return False
        self.md8475a.wait_for_registration_state()

        self.setup_3710a_waveform("1", "A", "1960MHZ", neigh_cell_2_dlpower,
                                  "LTE", "10M_B1_CID2")

        self.setup_3710a_waveform("1", "B", "1960MHZ", neigh_cell_3_dlpower,
                                  "LTE", "10M_B1_CID3")
        self.turn_on_3710a_sg(1)
        time.sleep(self._SETTLING_TIME)
        return self._verify_cell_info(self.ad, expected_cell_info_stats)

    @test_tracker_info(uuid="3e094e3d-e7b7-447a-9a7a-8060c5b17e88")
    @TelephonyBaseTest.tel_test_wrap
    def test_ncells_intra_lte_4_cells(self):
        """ Test Number of neighbor cells reported by Phone when four neighbor
        cells are present (Phone camped on LTE)

        Setup two LTE cell configuration on MD8475A
        Setup three waveform on MG3710A
        Make Sure Phone is in LTE mode
        Verify the number of neighbor cells reported by Phone

        Returns:
            True if pass; False if fail
        """
        serving_cell_cid = 11
        serving_cell_pcid = 11
        neigh_cell_1_cid = 1
        neigh_cell_1_pcid = 1
        neigh_cell_2_cid = 2
        neigh_cell_2_pcid = 2
        neigh_cell_3_cid = 3
        neigh_cell_3_pcid = 3
        neigh_cell_4_cid = 5
        neigh_cell_4_pcid = 5
        serving_cell_dlpower = -20
        neigh_cell_1_dlpower = -24
        neigh_cell_2_dlpower = -22
        neigh_cell_3_dlpower = -24
        neigh_cell_4_dlpower = -22

        expected_cell_info_list = [
            # Serving Cell
            {
                self.CID: serving_cell_cid,
                self.PCID: serving_cell_pcid,
                self.REPORT_RATE: 1,
                self.IS_REGISTERED: True,
                self.RAT: 'lte',
                self.TARGET_RSSI: self._LTE_RSSI_OFFSET + serving_cell_dlpower,
                self.MAX_ERROR_RSSI: 3,
                self.MAX_ERROR_AVERAGE_RSSI: 3
            },
            # Neighbor Cells
            {
                self.PCID: neigh_cell_1_pcid,
                self.REPORT_RATE: 0.1,
                self.IS_REGISTERED: False,
                self.RAT: 'lte',
                self.TARGET_RSSI: self._LTE_RSSI_OFFSET + neigh_cell_1_dlpower,
                self.MAX_ERROR_RSSI: 4,
                self.MAX_ERROR_AVERAGE_RSSI: 3
            },
            {
                self.PCID: neigh_cell_2_pcid,
                self.REPORT_RATE: 0.1,
                self.IS_REGISTERED: False,
                self.RAT: 'lte',
                self.TARGET_RSSI: self._LTE_RSSI_OFFSET + neigh_cell_2_dlpower,
                self.MAX_ERROR_RSSI: 4,
                self.MAX_ERROR_AVERAGE_RSSI: 3
            },
            {
                self.PCID: neigh_cell_3_pcid,
                self.REPORT_RATE: 0.1,
                self.IS_REGISTERED: False,
                self.RAT: 'lte',
                self.TARGET_RSSI: self._LTE_RSSI_OFFSET + neigh_cell_3_dlpower,
                self.MAX_ERROR_RSSI: 4,
                self.MAX_ERROR_AVERAGE_RSSI: 3
            },
            {
                self.PCID: neigh_cell_4_pcid,
                self.REPORT_RATE: 0.1,
                self.IS_REGISTERED: False,
                self.RAT: 'lte',
                self.TARGET_RSSI: self._LTE_RSSI_OFFSET + neigh_cell_4_dlpower,
                self.MAX_ERROR_RSSI: 4,
                self.MAX_ERROR_AVERAGE_RSSI: 3
            }
        ]
        expected_cell_info_stats = {}
        for sample in expected_cell_info_list:
            expected_cell_info_stats[self._unique_cell_id(sample)] = sample

        [bts1, bts2] = set_system_model_lte_lte(self.md8475a, self.user_params,
                                                self.ad.sim_card)
        self._setup_lte_serving_cell(bts1, serving_cell_dlpower,
                                     serving_cell_cid, serving_cell_pcid)

        self._setup_lte_neighbhor_cell_md8475a(
            bts2, LTE_BAND_2, neigh_cell_1_dlpower, neigh_cell_1_cid,
            neigh_cell_1_pcid)
        set_usim_parameters(self.md8475a, self.ad.sim_card)
        self.md8475a.start_simulation()

        if not ensure_network_rat(
                self.log,
                self.ad,
                NETWORK_MODE_LTE_GSM_WCDMA,
                RAT_FAMILY_LTE,
                toggle_apm_after_setting=True):
            self.log.error("Failed to set rat family {}, preferred network:{}".
                           format(RAT_FAMILY_LTE, NETWORK_MODE_LTE_GSM_WCDMA))
            return False
        self.md8475a.wait_for_registration_state()

        self.setup_3710a_waveform("1", "A", "1960MHZ", neigh_cell_2_dlpower,
                                  "LTE", "10M_B1_CID2")

        self.setup_3710a_waveform("1", "B", "1960MHZ", neigh_cell_3_dlpower,
                                  "LTE", "10M_B1_CID3")

        self.setup_3710a_waveform("2", "A", "1960MHZ", neigh_cell_4_dlpower,
                                  "LTE", "10M_B1_CID5")
        self.turn_on_3710a_sg(1)
        self.turn_on_3710a_sg(2)
        time.sleep(self._SETTLING_TIME)

        return self._verify_cell_info(self.ad, expected_cell_info_stats)

    @test_tracker_info(uuid="7e9a9c30-9284-4440-b85e-f94b83e0373f")
    @TelephonyBaseTest.tel_test_wrap
    def test_neighbor_cell_reporting_lte_intrafreq_0_tmo(self):
        """ Test Number of neighbor cells reported by Phone when no neighbor
        cells are present (Phone camped on LTE)

        Setup a single LTE cell configuration on MD8475A
        Make Sure Phone is in LTE mode
        Verify the number of neighbor cells reported by Phone

        Returns:
            True if pass; False if fail
        """
        serving_cell = lte_band4_ch2000_fr2115_pcid1_cell
        serving_cell['power'] = -20

        expected_cell_info_list = [
            # Serving Cell
            {
                self.CID: serving_cell[self.CID],
                self.PCID: serving_cell[self.PCID],
                self.REPORT_RATE: 1,
                self.IS_REGISTERED: True,
                self.RAT: 'lte',
                self.TARGET_RSSI:
                self._LTE_RSSI_OFFSET + serving_cell['power'],
                self.MAX_ERROR_RSSI: 3,
                self.MAX_ERROR_AVERAGE_RSSI: 3
            }
        ]

        expected_cell_info_stats = {}
        for sample in expected_cell_info_list:
            expected_cell_info_stats[self._unique_cell_id(sample)] = sample

        [bts1] = set_system_model_lte(self.md8475a, self.user_params,
                                      self.ad.sim_card)
        self._setup_lte_cell_md8475a(bts1, serving_cell, serving_cell['power'])
        set_usim_parameters(self.md8475a, self.ad.sim_card)
        self.md8475a.start_simulation()

        if not ensure_network_rat(
                self.log,
                self.ad,
                NETWORK_MODE_LTE_GSM_WCDMA,
                RAT_FAMILY_LTE,
                toggle_apm_after_setting=True):
            self.log.error("Failed to set rat family {}, preferred network:{}".
                           format(RAT_FAMILY_LTE, NETWORK_MODE_LTE_GSM_WCDMA))
            return False
        self.md8475a.wait_for_registration_state()
        time.sleep(self._SETTLING_TIME)
        return self._verify_cell_info(self.ad, expected_cell_info_stats)

    @test_tracker_info(uuid="13bd7000-5a45-43f5-9e54-001e0aa09262")
    @TelephonyBaseTest.tel_test_wrap
    def test_neighbor_cell_reporting_lte_intrafreq_1_tmo(self):
        """ Test Number of neighbor cells reported by Phone when one neighbor
        cell is present (Phone camped on LTE)

        Setup a two LTE cell configuration on MD8475A
        Make Sure Phone is in LTE mode
        Verify the number of neighbor cells reported by Phone

        Returns:
            True if pass; False if fail
        """
        serving_cell = lte_band4_ch2000_fr2115_pcid1_cell
        neighbor_cell = lte_band4_ch2000_fr2115_pcid2_cell
        serving_cell['power'] = -20
        neighbor_cell['power'] = -24
        expected_cell_info_list = [
            # Serving Cell
            {
                self.CID: serving_cell[self.CID],
                self.PCID: serving_cell[self.PCID],
                self.REPORT_RATE: 1,
                self.IS_REGISTERED: True,
                self.RAT: 'lte',
                self.TARGET_RSSI:
                self._LTE_RSSI_OFFSET + serving_cell['power'],
                self.MAX_ERROR_RSSI: 3,
                self.MAX_ERROR_AVERAGE_RSSI: 3
            },
            # Neighbor Cells
            {
                self.PCID: neighbor_cell[self.PCID],
                self.REPORT_RATE: 0.1,
                self.IS_REGISTERED: False,
                self.RAT: 'lte',
                self.TARGET_RSSI:
                self._LTE_RSSI_OFFSET + neighbor_cell['power'],
                self.MAX_ERROR_RSSI: 4,
                self.MAX_ERROR_AVERAGE_RSSI: 3
            }
        ]
        expected_cell_info_stats = {}
        for sample in expected_cell_info_list:
            expected_cell_info_stats[self._unique_cell_id(sample)] = sample

        self.md8475a.load_cell_paramfile(self._CELL_PARAM_FILE)
        [bts1, bts2] = set_system_model_lte_lte(self.md8475a, self.user_params,
                                                self.ad.sim_card)
        self._setup_lte_cell_md8475a(bts1, serving_cell, serving_cell['power'])
        self._setup_lte_cell_md8475a(bts2, neighbor_cell,
                                     neighbor_cell['power'])
        bts1.neighbor_cell_mode = "USERDATA"
        bts1.set_neighbor_cell_type("LTE", 1, "CELLNAME")
        bts1.set_neighbor_cell_name("LTE", 1, "LTE_4_C2000_F2115_PCID2")
        set_usim_parameters(self.md8475a, self.ad.sim_card)
        self.md8475a.start_simulation()
        # To make sure phone camps on BTS1
        bts2.service_state = BtsServiceState.SERVICE_STATE_OUT

        if not ensure_network_rat(
                self.log,
                self.ad,
                NETWORK_MODE_LTE_GSM_WCDMA,
                RAT_FAMILY_LTE,
                toggle_apm_after_setting=True):
            self.log.error("Failed to set rat family {}, preferred network:{}".
                           format(RAT_FAMILY_LTE, NETWORK_MODE_LTE_GSM_WCDMA))
            return False
        self.md8475a.wait_for_registration_state()
        time.sleep(self._ANRITSU_SETTLING_TIME)
        bts2.service_state = BtsServiceState.SERVICE_STATE_IN
        time.sleep(self._SETTLING_TIME)
        return self._verify_cell_info(self.ad, expected_cell_info_stats)

    @test_tracker_info(uuid="5dca3a16-73a0-448a-a35d-22ebd253a570")
    @TelephonyBaseTest.tel_test_wrap
    def test_neighbor_cell_reporting_lte_intrafreq_2_tmo(self):
        """ Test Number of neighbor cells reported by Phone when two neighbor
        cells are present (Phone camped on LTE)

        Setup one LTE cell configuration on MD8475A
        Setup two waveform on MG3710A
        Make Sure Phone is in LTE mode
        Verify the number of neighbor cells reported by Phone

        Returns:
            True if pass; False if fail
        """
        serving_cell = lte_band4_ch2000_fr2115_pcid1_cell
        neighbor_cell_1 = lte_band4_ch2000_fr2115_pcid2_cell
        neighbor_cell_2 = lte_band4_ch2000_fr2115_pcid3_cell
        serving_cell['power'] = -20
        neighbor_cell_1['power'] = -24
        neighbor_cell_2['power'] = -23

        expected_cell_info_list = [
            # Serving Cell
            {
                self.CID: serving_cell[self.CID],
                self.PCID: serving_cell[self.PCID],
                self.REPORT_RATE: 1,
                self.IS_REGISTERED: True,
                self.RAT: 'lte',
                self.TARGET_RSSI:
                self._LTE_RSSI_OFFSET + serving_cell['power'],
                self.MAX_ERROR_RSSI: 3,
                self.MAX_ERROR_AVERAGE_RSSI: 3
            },
            # Neighbor Cells
            {
                self.PCID: neighbor_cell_1[self.PCID],
                self.REPORT_RATE: 0.1,
                self.IS_REGISTERED: False,
                self.RAT: 'lte',
                self.TARGET_RSSI:
                self._LTE_RSSI_OFFSET + neighbor_cell_1['power'],
                self.MAX_ERROR_RSSI: 4,
                self.MAX_ERROR_AVERAGE_RSSI: 3
            },
            {
                self.PCID: neighbor_cell_2[self.PCID],
                self.REPORT_RATE: 0.1,
                self.IS_REGISTERED: False,
                self.RAT: 'lte',
                self.TARGET_RSSI:
                self._LTE_RSSI_OFFSET + neighbor_cell_2['power'],
                self.MAX_ERROR_RSSI: 4,
                self.MAX_ERROR_AVERAGE_RSSI: 3
            }
        ]
        expected_cell_info_stats = {}
        for sample in expected_cell_info_list:
            expected_cell_info_stats[self._unique_cell_id(sample)] = sample

        self.md8475a.load_cell_paramfile(self._CELL_PARAM_FILE)
        [bts1, bts2] = set_system_model_lte_lte(self.md8475a, self.user_params,
                                                self.ad.sim_card)
        self._setup_lte_cell_md8475a(bts1, serving_cell, serving_cell['power'])
        self._setup_lte_cell_md8475a(bts2, neighbor_cell_1,
                                     neighbor_cell_1['power'])
        bts1.neighbor_cell_mode = "USERDATA"
        bts1.set_neighbor_cell_type("LTE", 1, "CELLNAME")
        bts1.set_neighbor_cell_name("LTE", 1, "LTE_4_C2000_F2115_PCID2")
        bts1.set_neighbor_cell_type("LTE", 2, "CELLNAME")
        bts1.set_neighbor_cell_name("LTE", 2, "LTE_4_C2000_F2115_PCID3")
        set_usim_parameters(self.md8475a, self.ad.sim_card)
        self.md8475a.start_simulation()
        # To make sure phone camps on BTS1
        bts2.service_state = BtsServiceState.SERVICE_STATE_OUT

        if not ensure_network_rat(
                self.log,
                self.ad,
                NETWORK_MODE_LTE_GSM_WCDMA,
                RAT_FAMILY_LTE,
                toggle_apm_after_setting=True):
            self.log.error("Failed to set rat family {}, preferred network:{}".
                           format(RAT_FAMILY_LTE, NETWORK_MODE_LTE_GSM_WCDMA))
            return False
        self.md8475a.wait_for_registration_state()
        time.sleep(self._ANRITSU_SETTLING_TIME)
        bts2.service_state = BtsServiceState.SERVICE_STATE_IN
        self.setup_3710a_waveform("1", "A", "2115MHz",
                                  neighbor_cell_2['power'], "LTE",
                                  "lte_4_ch2000_pcid3")
        self.turn_on_3710a_sg(1)
        time.sleep(self._SETTLING_TIME)
        return self._verify_cell_info(self.ad, expected_cell_info_stats)

    @test_tracker_info(uuid="860152de-8aa0-422e-b5b0-28bf244076f4")
    @TelephonyBaseTest.tel_test_wrap
    def test_neighbor_cell_reporting_lte_intrafreq_3_tmo(self):
        """ Test Number of neighbor cells reported by Phone when three neighbor
        cells are present (Phone camped on LTE)

        Setup a one LTE cell configuration on MD8475A
        Setup three waveforms on MG3710A
        Make Sure Phone is in LTE mode
        Verify the number of neighbor cells reported by Phone

        Returns:
            True if pass; False if fail
        """
        serving_cell = lte_band4_ch2000_fr2115_pcid1_cell
        neighbor_cell_1 = lte_band4_ch2000_fr2115_pcid2_cell
        neighbor_cell_2 = lte_band4_ch2000_fr2115_pcid3_cell
        neighbor_cell_3 = lte_band4_ch2000_fr2115_pcid4_cell
        serving_cell['power'] = -20
        neighbor_cell_1['power'] = -24
        neighbor_cell_2['power'] = -23
        neighbor_cell_3['power'] = -22

        expected_cell_info_list = [
            # Serving Cell
            {
                self.CID: serving_cell[self.CID],
                self.PCID: serving_cell[self.PCID],
                self.REPORT_RATE: 1,
                self.IS_REGISTERED: True,
                self.RAT: 'lte',
                self.TARGET_RSSI:
                self._LTE_RSSI_OFFSET + serving_cell['power'],
                self.MAX_ERROR_RSSI: 3,
                self.MAX_ERROR_AVERAGE_RSSI: 3
            },
            # Neighbor Cells
            {
                self.PCID: neighbor_cell_1[self.PCID],
                self.REPORT_RATE: 0.1,
                self.IS_REGISTERED: False,
                self.RAT: 'lte',
                self.TARGET_RSSI:
                self._LTE_RSSI_OFFSET + neighbor_cell_1['power'],
                self.MAX_ERROR_RSSI: 4,
                self.MAX_ERROR_AVERAGE_RSSI: 3
            },
            {
                self.PCID: neighbor_cell_2[self.PCID],
                self.REPORT_RATE: 0.1,
                self.IS_REGISTERED: False,
                self.RAT: 'lte',
                self.TARGET_RSSI:
                self._LTE_RSSI_OFFSET + neighbor_cell_2['power'],
                self.MAX_ERROR_RSSI: 4,
                self.MAX_ERROR_AVERAGE_RSSI: 3
            },
            {
                self.PCID: neighbor_cell_3[self.PCID],
                self.REPORT_RATE: 0.1,
                self.IS_REGISTERED: False,
                self.RAT: 'lte',
                self.TARGET_RSSI:
                self._LTE_RSSI_OFFSET + neighbor_cell_3['power'],
                self.MAX_ERROR_RSSI: 4,
                self.MAX_ERROR_AVERAGE_RSSI: 3
            }
        ]
        expected_cell_info_stats = {}
        for sample in expected_cell_info_list:
            expected_cell_info_stats[self._unique_cell_id(sample)] = sample

        self.md8475a.load_cell_paramfile(self._CELL_PARAM_FILE)
        [bts1, bts2] = set_system_model_lte_lte(self.md8475a, self.user_params,
                                                self.ad.sim_card)
        self._setup_lte_cell_md8475a(bts1, serving_cell, serving_cell['power'])
        self._setup_lte_cell_md8475a(bts2, neighbor_cell_1,
                                     neighbor_cell_1['power'])
        bts1.neighbor_cell_mode = "USERDATA"
        bts1.set_neighbor_cell_type("LTE", 1, "CELLNAME")
        bts1.set_neighbor_cell_name("LTE", 1, "LTE_4_C2000_F2115_PCID2")
        bts1.set_neighbor_cell_type("LTE", 2, "CELLNAME")
        bts1.set_neighbor_cell_name("LTE", 2, "LTE_4_C2000_F2115_PCID3")
        bts1.set_neighbor_cell_type("LTE", 3, "CELLNAME")
        bts1.set_neighbor_cell_name("LTE", 3, "LTE_4_C2000_F2115_PCID4")
        set_usim_parameters(self.md8475a, self.ad.sim_card)
        self.md8475a.start_simulation()
        # To make sure phone camps on BTS1
        bts2.service_state = BtsServiceState.SERVICE_STATE_OUT

        if not ensure_network_rat(
                self.log,
                self.ad,
                NETWORK_MODE_LTE_GSM_WCDMA,
                RAT_FAMILY_LTE,
                toggle_apm_after_setting=True):
            self.log.error("Failed to set rat family {}, preferred network:{}".
                           format(RAT_FAMILY_LTE, NETWORK_MODE_LTE_GSM_WCDMA))
            return False
        self.md8475a.wait_for_registration_state()
        time.sleep(self._ANRITSU_SETTLING_TIME)
        bts2.service_state = BtsServiceState.SERVICE_STATE_IN
        self.setup_3710a_waveform("1", "A", "2115MHz",
                                  neighbor_cell_2['power'], "LTE",
                                  "lte_4_ch2000_pcid3")

        self.setup_3710a_waveform("1", "B", "2115MHz",
                                  neighbor_cell_3['power'], "LTE",
                                  "lte_4_ch2000_pcid4")
        self.turn_on_3710a_sg(1)
        time.sleep(self._SETTLING_TIME)
        return self._verify_cell_info(self.ad, expected_cell_info_stats)

    @test_tracker_info(uuid="8c5b63ba-1322-47b6-adce-5224cbc0995a")
    @TelephonyBaseTest.tel_test_wrap
    def test_neighbor_cell_reporting_lte_interfreq_1_tmo(self):
        """ Test Number of neighbor cells reported by Phone when two neighbor
        cells(inter frequency) are present (Phone camped on LTE)

        Setup a a LTE cell configuration on MD8475A
        Setup two LTE waveforms(inter frequency) on MG3710A
        Make Sure Phone is in LTE mode
        Verify the number of neighbor cells reported by Phone

        Returns:
            True if pass; False if fail
        """
        serving_cell = lte_band4_ch2000_fr2115_pcid1_cell
        neighbor_cell_1 = lte_band4_ch2050_fr2120_pcid7_cell
        serving_cell['power'] = -20
        neighbor_cell_1['power'] = -23
        expected_cell_info_list = [
            # Serving Cell
            {
                self.CID: serving_cell[self.CID],
                self.PCID: serving_cell[self.PCID],
                self.REPORT_RATE: 1,
                self.IS_REGISTERED: True,
                self.RAT: 'lte',
                self.TARGET_RSSI:
                self._LTE_RSSI_OFFSET + serving_cell['power'],
                self.MAX_ERROR_RSSI: 3,
                self.MAX_ERROR_AVERAGE_RSSI: 3
            },
            # Neighbor Cells
            {
                self.PCID: neighbor_cell_1[self.PCID],
                self.REPORT_RATE: 0.1,
                self.IS_REGISTERED: False,
                self.RAT: 'lte',
                self.TARGET_RSSI:
                self._LTE_RSSI_OFFSET + neighbor_cell_1['power'],
                self.MAX_ERROR_RSSI: 4,
                self.MAX_ERROR_AVERAGE_RSSI: 3
            }
        ]
        expected_cell_info_stats = {}
        for sample in expected_cell_info_list:
            expected_cell_info_stats[self._unique_cell_id(sample)] = sample

        self.md8475a.load_cell_paramfile(self._CELL_PARAM_FILE)
        [bts1, bts2] = set_system_model_lte_lte(self.md8475a, self.user_params,
                                                self.ad.sim_card)
        self._setup_lte_cell_md8475a(bts1, serving_cell, serving_cell['power'])
        self._setup_lte_cell_md8475a(bts2, neighbor_cell_1,
                                     neighbor_cell_1['power'])
        bts1.neighbor_cell_mode = "USERDATA"
        bts1.set_neighbor_cell_type("LTE", 1, "CELLNAME")
        bts1.set_neighbor_cell_name("LTE", 1, "LTE_4_C2050_F2120_PCID7")
        set_usim_parameters(self.md8475a, self.ad.sim_card)
        self.md8475a.start_simulation()
        # To make sure phone camps on BTS1
        bts2.service_state = BtsServiceState.SERVICE_STATE_OUT

        self.ad.droid.telephonyToggleDataConnection(False)
        if not ensure_network_rat(
                self.log,
                self.ad,
                NETWORK_MODE_LTE_GSM_WCDMA,
                RAT_FAMILY_LTE,
                toggle_apm_after_setting=True):
            self.log.error("Failed to set rat family {}, preferred network:{}".
                           format(RAT_FAMILY_LTE, NETWORK_MODE_LTE_GSM_WCDMA))
            return False
        self.md8475a.wait_for_registration_state()
        time.sleep(self._ANRITSU_SETTLING_TIME)
        bts2.service_state = BtsServiceState.SERVICE_STATE_IN
        time.sleep(self._ANRITSU_SETTLING_TIME)
        self.md8475a.set_packet_preservation()
        time.sleep(self._SETTLING_TIME)
        return self._verify_cell_info(self.ad, expected_cell_info_stats)

    @test_tracker_info(uuid="97853501-a328-4706-bb3f-c5e708b1ccb8")
    @TelephonyBaseTest.tel_test_wrap
    def test_neighbor_cell_reporting_lte_interfreq_2_tmo(self):
        """ Test Number of neighbor cells reported by Phone when two neighbor
        cells(inter frequency) are present (Phone camped on LTE)

        Setup a a LTE cell configuration on MD8475A
        Setup two LTE waveforms(inter frequency) on MG3710A
        Make Sure Phone is in LTE mode
        Verify the number of neighbor cells reported by Phone

        Returns:
            True if pass; False if fail
        """
        serving_cell = lte_band4_ch2000_fr2115_pcid1_cell
        neighbor_cell_1 = lte_band4_ch2050_fr2120_pcid7_cell
        neighbor_cell_2 = lte_band4_ch2250_fr2140_pcid8_cell
        serving_cell['power'] = -20
        neighbor_cell_1['power'] = -23
        neighbor_cell_2['power'] = -22

        expected_cell_info_list = [
            # Serving Cell
            {
                self.CID: serving_cell[self.CID],
                self.PCID: serving_cell[self.PCID],
                self.REPORT_RATE: 1,
                self.IS_REGISTERED: True,
                self.RAT: 'lte',
                self.TARGET_RSSI:
                self._LTE_RSSI_OFFSET + serving_cell['power'],
                self.MAX_ERROR_RSSI: 3,
                self.MAX_ERROR_AVERAGE_RSSI: 3
            },
            # Neighbor Cells
            {
                self.PCID: neighbor_cell_1[self.PCID],
                self.REPORT_RATE: 0.1,
                self.IS_REGISTERED: False,
                self.RAT: 'lte',
                self.TARGET_RSSI:
                self._LTE_RSSI_OFFSET + neighbor_cell_1['power'],
                self.MAX_ERROR_RSSI: 4,
                self.MAX_ERROR_AVERAGE_RSSI: 3
            },
            {
                self.PCID: neighbor_cell_2[self.PCID],
                self.REPORT_RATE: 0.1,
                self.IS_REGISTERED: False,
                self.RAT: 'lte',
                self.TARGET_RSSI:
                self._LTE_RSSI_OFFSET + neighbor_cell_2['power'],
                self.MAX_ERROR_RSSI: 4,
                self.MAX_ERROR_AVERAGE_RSSI: 3
            }
        ]
        expected_cell_info_stats = {}
        for sample in expected_cell_info_list:
            expected_cell_info_stats[self._unique_cell_id(sample)] = sample

        self.md8475a.load_cell_paramfile(self._CELL_PARAM_FILE)
        [bts1, bts2] = set_system_model_lte_lte(self.md8475a, self.user_params,
                                                self.ad.sim_card)
        self._setup_lte_cell_md8475a(bts1, serving_cell, serving_cell['power'])
        self._setup_lte_cell_md8475a(bts2, neighbor_cell_1,
                                     neighbor_cell_1['power'])
        bts1.neighbor_cell_mode = "USERDATA"
        bts1.set_neighbor_cell_type("LTE", 1, "CELLNAME")
        bts1.set_neighbor_cell_name("LTE", 1, "LTE_4_C2050_F2120_PCID7")
        bts1.set_neighbor_cell_type("LTE", 2, "CELLNAME")
        bts1.set_neighbor_cell_name("LTE", 2, "LTE_4_C2250_F2140_PCID8")
        set_usim_parameters(self.md8475a, self.ad.sim_card)
        self.md8475a.start_simulation()
        # To make sure phone camps on BTS1
        bts2.service_state = BtsServiceState.SERVICE_STATE_OUT

        self.ad.droid.telephonyToggleDataConnection(False)
        if not ensure_network_rat(
                self.log,
                self.ad,
                NETWORK_MODE_LTE_GSM_WCDMA,
                RAT_FAMILY_LTE,
                toggle_apm_after_setting=True):
            self.log.error("Failed to set rat family {}, preferred network:{}".
                           format(RAT_FAMILY_LTE, NETWORK_MODE_LTE_GSM_WCDMA))
            return False
        self.md8475a.wait_for_registration_state()
        time.sleep(self._ANRITSU_SETTLING_TIME)
        bts2.service_state = BtsServiceState.SERVICE_STATE_IN
        time.sleep(self._ANRITSU_SETTLING_TIME)
        self.setup_3710a_waveform("1", "A", "2140MHz",
                                  neighbor_cell_2['power'], "LTE",
                                  "lte_4_ch2250_pcid8")

        self.turn_on_3710a_sg(1)
        self.md8475a.set_packet_preservation()
        time.sleep(self._SETTLING_TIME)
        return self._verify_cell_info(self.ad, expected_cell_info_stats)

    @test_tracker_info(uuid="74bd528c-e1c5-476d-9ee0-ebfc7bbc5de1")
    @TelephonyBaseTest.tel_test_wrap
    def test_neighbor_cell_reporting_lte_interband_2_tmo(self):
        """ Test Number of neighbor cells reported by Phone when two neighbor
        cells(inter band) are present (Phone camped on LTE)

        Setup one LTE cell configuration on MD8475A
        Setup two LTE waveforms((inter band)) on MG3710A
        Make Sure Phone is in LTE mode
        Verify the number of neighbor cells reported by Phone

        Returns:
            True if pass; False if fail
        """
        serving_cell = lte_band4_ch2000_fr2115_pcid1_cell
        neighbor_cell_1 = lte_band2_ch900_fr1960_pcid9_cell
        neighbor_cell_2 = lte_band12_ch5095_fr737_pcid10_cell
        serving_cell['power'] = -20
        neighbor_cell_1['power'] = -24
        neighbor_cell_2['power'] = -22

        expected_cell_info_list = [
            # Serving Cell
            {
                self.CID: serving_cell[self.CID],
                self.PCID: serving_cell[self.PCID],
                self.REPORT_RATE: 1,
                self.IS_REGISTERED: True,
                self.RAT: 'lte',
                self.TARGET_RSSI:
                self._LTE_RSSI_OFFSET + serving_cell['power'],
                self.MAX_ERROR_RSSI: 3,
                self.MAX_ERROR_AVERAGE_RSSI: 3
            },
            # Neighbor Cells
            {
                self.PCID: neighbor_cell_1[self.PCID],
                self.REPORT_RATE: 0.1,
                self.IS_REGISTERED: False,
                self.RAT: 'lte',
                self.TARGET_RSSI:
                self._LTE_RSSI_OFFSET + neighbor_cell_1['power'],
                self.MAX_ERROR_RSSI: 4,
                self.MAX_ERROR_AVERAGE_RSSI: 3
            },
            {
                self.PCID: neighbor_cell_2[self.PCID],
                self.REPORT_RATE: 0.1,
                self.IS_REGISTERED: False,
                self.RAT: 'lte',
                self.TARGET_RSSI:
                self._LTE_RSSI_OFFSET + neighbor_cell_2['power'],
                self.MAX_ERROR_RSSI: 4,
                self.MAX_ERROR_AVERAGE_RSSI: 3
            }
        ]
        expected_cell_info_stats = {}
        for sample in expected_cell_info_list:
            expected_cell_info_stats[self._unique_cell_id(sample)] = sample

        self.md8475a.load_cell_paramfile(self._CELL_PARAM_FILE)
        [bts1, bts2] = set_system_model_lte_lte(self.md8475a, self.user_params,
                                                self.ad.sim_card)
        self._setup_lte_cell_md8475a(bts1, serving_cell, serving_cell['power'])
        self._setup_lte_cell_md8475a(bts2, neighbor_cell_1,
                                     neighbor_cell_1['power'])
        bts1.neighbor_cell_mode = "USERDATA"
        bts1.set_neighbor_cell_type("LTE", 1, "CELLNAME")
        bts1.set_neighbor_cell_name("LTE", 1, "LTE_2_C900_F1960_PCID9")
        bts1.set_neighbor_cell_type("LTE", 2, "CELLNAME")
        bts1.set_neighbor_cell_name("LTE", 2, "LTE_12_C5095_F737_PCID10")
        set_usim_parameters(self.md8475a, self.ad.sim_card)
        self.md8475a.start_simulation()
        # To make sure phone camps on BTS1
        bts2.service_state = BtsServiceState.SERVICE_STATE_OUT

        self.ad.droid.telephonyToggleDataConnection(False)
        if not ensure_network_rat(
                self.log,
                self.ad,
                NETWORK_MODE_LTE_GSM_WCDMA,
                RAT_FAMILY_LTE,
                toggle_apm_after_setting=True):
            self.log.error("Failed to set rat family {}, preferred network:{}".
                           format(RAT_FAMILY_LTE, NETWORK_MODE_LTE_GSM_WCDMA))
            return False
        self.md8475a.wait_for_registration_state()
        time.sleep(self._ANRITSU_SETTLING_TIME)
        bts2.service_state = BtsServiceState.SERVICE_STATE_IN
        self.setup_3710a_waveform("1", "A", "737.5MHz",
                                  neighbor_cell_2['power'], "LTE",
                                  "lte_12_ch5095_pcid10")
        self.turn_on_3710a_sg(1)
        time.sleep(self._ANRITSU_SETTLING_TIME)
        self.md8475a.set_packet_preservation()
        time.sleep(self._SETTLING_TIME)
        return self._verify_cell_info(self.ad, expected_cell_info_stats)

    @test_tracker_info(uuid="6289e3e4-9316-4b82-bd0b-dde53f26da0d")
    @TelephonyBaseTest.tel_test_wrap
    def test_neighbor_cell_reporting_lte_interrat_1_tmo(self):
        """ Test Number of neighbor cells reported by Phone when two neighbor
        cells(inter RAT) are present (Phone camped on LTE)

        Setup one LTE and one WCDMA cell configuration on MD8475A
        Setup one GSM waveform on MG3710A
        Make Sure Phone is in LTE mode
        Verify the number of neighbor cells reported by Phone

        Returns:
            True if pass; False if fail
        """
        serving_cell = lte_band4_ch2000_fr2115_pcid1_cell
        neighbor_cell_1 = wcdma_band1_ch10700_fr2140_cid31_cell
        serving_cell['power'] = -20
        neighbor_cell_1['power'] = -24

        expected_cell_info_list = [
            # Serving Cell
            {
                self.CID: serving_cell[self.CID],
                self.PCID: serving_cell[self.PCID],
                self.REPORT_RATE: 1,
                self.IS_REGISTERED: True,
                self.RAT: 'lte',
                self.TARGET_RSSI:
                self._LTE_RSSI_OFFSET + serving_cell['power'],
                self.MAX_ERROR_RSSI: 3,
                self.MAX_ERROR_AVERAGE_RSSI: 3
            },
            # Neighbor Cells
            {
                self.PSC: neighbor_cell_1[self.PSC],
                self.REPORT_RATE: 0.1,
                self.IS_REGISTERED: False,
                self.RAT: 'wcdma',
                self.TARGET_RSSI:
                self._WCDMA_RSSI_OFFSET + neighbor_cell_1['power'],
                self.MAX_ERROR_RSSI: 4,
                self.MAX_ERROR_AVERAGE_RSSI: 3
            }
        ]
        expected_cell_info_stats = {}
        for sample in expected_cell_info_list:
            expected_cell_info_stats[self._unique_cell_id(sample)] = sample

        self.md8475a.load_cell_paramfile(self._CELL_PARAM_FILE)
        [bts1, bts2] = set_system_model_lte_wcdma(
            self.md8475a, self.user_params, self.ad.sim_card)
        self._setup_lte_cell_md8475a(bts1, serving_cell, serving_cell['power'])
        self._setup_wcdma_cell_md8475a(bts2, neighbor_cell_1,
                                       neighbor_cell_1['power'])
        bts1.neighbor_cell_mode = "USERDATA"
        bts1.set_neighbor_cell_type("WCDMA", 1, "CELLNAME")
        bts1.set_neighbor_cell_name("WCDMA", 1, "WCDM_1_C10700_F2140_CID31")
        set_usim_parameters(self.md8475a, self.ad.sim_card)
        self.md8475a.start_simulation()
        # To make sure phone camps on BTS1
        bts2.service_state = BtsServiceState.SERVICE_STATE_OUT

        self.ad.droid.telephonyToggleDataConnection(False)
        if not ensure_network_rat(
                self.log,
                self.ad,
                NETWORK_MODE_LTE_GSM_WCDMA,
                RAT_FAMILY_LTE,
                toggle_apm_after_setting=True):
            self.log.error("Failed to set rat family {}, preferred network:{}".
                           format(RAT_FAMILY_LTE, NETWORK_MODE_LTE_GSM_WCDMA))
            return False
        self.md8475a.wait_for_registration_state()
        time.sleep(self._ANRITSU_SETTLING_TIME)
        self.md8475a.set_packet_preservation()
        time.sleep(self._SETTLING_TIME)
        return self._verify_cell_info(self.ad, expected_cell_info_stats)

    @test_tracker_info(uuid="9be4e4a8-f79a-4283-9a85-371a9bddfa5d")
    @TelephonyBaseTest.tel_test_wrap
    def test_neighbor_cell_reporting_lte_interrat_2_tmo(self):
        """ Test Number of neighbor cells reported by Phone when two neighbor
        cells(inter RAT) are present (Phone camped on LTE)

        Setup one LTE and one WCDMA cell configuration on MD8475A
        Setup one GSM waveform on MG3710A
        Make Sure Phone is in LTE mode
        Verify the number of neighbor cells reported by Phone

        Returns:
            True if pass; False if fail
        """
        serving_cell = lte_band4_ch2000_fr2115_pcid1_cell
        neighbor_cell_1 = wcdma_band1_ch10700_fr2140_cid31_cell
        neighbor_cell_2 = gsm_band1900_ch512_fr1930_cid51_cell
        serving_cell['power'] = -20
        neighbor_cell_1['power'] = -24
        neighbor_cell_2['power'] = -22
        expected_cell_info_list = [
            # Serving Cell
            {
                self.CID: serving_cell[self.CID],
                self.PCID: serving_cell[self.PCID],
                self.REPORT_RATE: 1,
                self.IS_REGISTERED: True,
                self.RAT: 'lte',
                self.TARGET_RSSI:
                self._LTE_RSSI_OFFSET + serving_cell['power'],
                self.MAX_ERROR_RSSI: 3,
                self.MAX_ERROR_AVERAGE_RSSI: 3
            },
            # Neighbor Cells
            {
                self.PSC: neighbor_cell_1[self.PSC],
                self.REPORT_RATE: 0.1,
                self.IS_REGISTERED: False,
                self.RAT: 'wcdma',
                self.TARGET_RSSI:
                self._WCDMA_RSSI_OFFSET + neighbor_cell_1['power'],
                self.MAX_ERROR_RSSI: 4,
                self.MAX_ERROR_AVERAGE_RSSI: 3
            },
            {
                self.CID: neighbor_cell_2[self.CID],
                self.REPORT_RATE: 0.1,
                self.IS_REGISTERED: False,
                self.RAT: 'gsm',
                self.TARGET_RSSI:
                self._GSM_RSSI_OFFSET + neighbor_cell_2['power'],
                self.MAX_ERROR_RSSI: 4,
                self.MAX_ERROR_AVERAGE_RSSI: 3
            }
        ]
        expected_cell_info_stats = {}
        for sample in expected_cell_info_list:
            expected_cell_info_stats[self._unique_cell_id(sample)] = sample

        self.md8475a.load_cell_paramfile(self._CELL_PARAM_FILE)
        [bts1, bts2] = set_system_model_lte_wcdma(
            self.md8475a, self.user_params, self.ad.sim_card)
        self._setup_lte_cell_md8475a(bts1, serving_cell, serving_cell['power'])
        self._setup_wcdma_cell_md8475a(bts2, neighbor_cell_1,
                                       neighbor_cell_1['power'])
        bts1.neighbor_cell_mode = "USERDATA"
        bts1.set_neighbor_cell_type("WCDMA", 1, "CELLNAME")
        bts1.set_neighbor_cell_name("WCDMA", 1, "WCDM_1_C10700_F2140_CID31")
        bts1.set_neighbor_cell_type("GSM", 1, "CELLNAME")
        bts1.set_neighbor_cell_name("GSM", 1, "GSM_1900_C512_F1930_CID51")
        set_usim_parameters(self.md8475a, self.ad.sim_card)
        self.md8475a.start_simulation()
        # To make sure phone camps on BTS1
        bts2.service_state = BtsServiceState.SERVICE_STATE_OUT

        self.ad.droid.telephonyToggleDataConnection(False)
        if not ensure_network_rat(
                self.log,
                self.ad,
                NETWORK_MODE_LTE_GSM_WCDMA,
                RAT_FAMILY_LTE,
                toggle_apm_after_setting=True):
            self.log.error("Failed to set rat family {}, preferred network:{}".
                           format(RAT_FAMILY_LTE, NETWORK_MODE_LTE_GSM_WCDMA))
            return False
        self.md8475a.wait_for_registration_state()
        time.sleep(self._ANRITSU_SETTLING_TIME)
        bts2.service_state = BtsServiceState.SERVICE_STATE_IN
        self.setup_3710a_waveform("1", "A", "1930.2MHz",
                                  neighbor_cell_2['power'], "GSM",
                                  "gsm_lac51_cid51")
        self.turn_on_3710a_sg(1)
        time.sleep(self._ANRITSU_SETTLING_TIME)
        self.md8475a.set_packet_preservation()
        time.sleep(self._SETTLING_TIME)
        return self._verify_cell_info(self.ad, expected_cell_info_stats)

    @test_tracker_info(uuid="14db7a3d-b18b-4b87-9d84-fb0c00d3971e")
    @TelephonyBaseTest.tel_test_wrap
    def test_neighbor_cell_reporting_wcdma_intrafreq_0_tmo(self):
        """ Test Number of neighbor cells reported by Phone when no neighbor
        cells are present (Phone camped on WCDMA)

        Setup a single WCDMA cell configuration on MD8475A
        Make Sure Phone camped on WCDMA
        Verify the number of neighbor cells reported by Phone

        Returns:
            True if pass; False if fail
        """
        serving_cell = wcdma_band1_ch10700_fr2140_cid31_cell
        serving_cell['power'] = -20
        expected_cell_info_list = [
            # Serving Cell
            {
                self.CID: serving_cell[self.CID],
                self.PSC: serving_cell[self.PSC],
                self.REPORT_RATE: 1,
                self.IS_REGISTERED: True,
                self.RAT: 'wcdma',
                self.TARGET_RSSI:
                self._WCDMA_RSSI_OFFSET + serving_cell['power'],
                self.MAX_ERROR_RSSI: 3,
                self.MAX_ERROR_AVERAGE_RSSI: 3
            }
        ]
        expected_cell_info_stats = {}
        for sample in expected_cell_info_list:
            expected_cell_info_stats[self._unique_cell_id(sample)] = sample

        [bts1] = set_system_model_wcdma(self.md8475a, self.user_params,
                                        self.ad.sim_card)
        self._setup_wcdma_cell_md8475a(bts1, serving_cell,
                                       serving_cell['power'])
        set_usim_parameters(self.md8475a, self.ad.sim_card)
        self.md8475a.start_simulation()

        if not ensure_network_rat(
                self.log,
                self.ad,
                NETWORK_MODE_GSM_UMTS,
                RAT_FAMILY_UMTS,
                toggle_apm_after_setting=True):
            self.log.error("Failed to set rat family {}, preferred network:{}".
                           format(RAT_FAMILY_UMTS, NETWORK_MODE_GSM_UMTS))
            return False
        self.md8475a.wait_for_registration_state()
        time.sleep(self._SETTLING_TIME)
        return self._verify_cell_info(self.ad, expected_cell_info_stats)

    @test_tracker_info(uuid="1a227d1e-9991-4646-b51a-8156f24485da")
    @TelephonyBaseTest.tel_test_wrap
    def test_neighbor_cell_reporting_wcdma_intrafreq_1_tmo(self):
        """ Test Number of neighbor cells reported by Phone when one neighbor
        cells is present (Phone camped on WCDMA)

        Setup two WCDMA cell configuration on MD8475A
        Make Sure Phone camped on WCDMA
        Verify the number of neighbor cells reported by Phone

        Returns:
            True if pass; False if fail
        """
        serving_cell = wcdma_band1_ch10700_fr2140_cid31_cell
        neighbor_cell = wcdma_band1_ch10700_fr2140_cid34_cell
        serving_cell['power'] = -20
        neighbor_cell['power'] = -24
        expected_cell_info_list = [
            # Serving Cell
            {
                self.CID: serving_cell[self.CID],
                self.PSC: serving_cell[self.PSC],
                self.REPORT_RATE: 1,
                self.IS_REGISTERED: True,
                self.RAT: 'wcdma',
                self.TARGET_RSSI:
                self._WCDMA_RSSI_OFFSET + serving_cell['power'],
                self.MAX_ERROR_RSSI: 3,
                self.MAX_ERROR_AVERAGE_RSSI: 3
            },
            # Neighbor Cells
            {
                self.PSC: neighbor_cell[self.PSC],
                self.REPORT_RATE: 0.1,
                self.IS_REGISTERED: False,
                self.RAT: 'wcdma',
                self.TARGET_RSSI:
                self._WCDMA_RSSI_OFFSET + neighbor_cell['power'],
                self.MAX_ERROR_RSSI: 4,
                self.MAX_ERROR_AVERAGE_RSSI: 3
            }
        ]
        expected_cell_info_stats = {}
        for sample in expected_cell_info_list:
            expected_cell_info_stats[self._unique_cell_id(sample)] = sample

        self.md8475a.load_cell_paramfile(self._CELL_PARAM_FILE)
        [bts1, bts2] = set_system_model_wcdma_wcdma(
            self.md8475a, self.user_params, self.ad.sim_card)
        self._setup_wcdma_cell_md8475a(bts1, serving_cell,
                                       serving_cell['power'])
        self._setup_wcdma_cell_md8475a(bts2, neighbor_cell,
                                       neighbor_cell['power'])
        bts1.neighbor_cell_mode = "USERDATA"
        bts1.set_neighbor_cell_type("WCDMA", 1, "CELLNAME")
        bts1.set_neighbor_cell_name("WCDMA", 1, "WCDM_1_C10700_F2140_CID34")
        set_usim_parameters(self.md8475a, self.ad.sim_card)
        self.md8475a.start_simulation()
        # To make sure phone camps on BTS1
        bts2.service_state = BtsServiceState.SERVICE_STATE_OUT

        if not ensure_network_rat(
                self.log,
                self.ad,
                NETWORK_MODE_GSM_UMTS,
                RAT_FAMILY_UMTS,
                toggle_apm_after_setting=True):
            self.log.error("Failed to set rat family {}, preferred network:{}".
                           format(RAT_FAMILY_UMTS, NETWORK_MODE_GSM_UMTS))
            return False
        self.md8475a.wait_for_registration_state()
        time.sleep(self._ANRITSU_SETTLING_TIME)
        bts2.service_state = BtsServiceState.SERVICE_STATE_IN
        time.sleep(self._SETTLING_TIME)
        return self._verify_cell_info(self.ad, expected_cell_info_stats)

    @test_tracker_info(uuid="170689a0-0db1-4a14-8b87-5a1b6c9b8581")
    @TelephonyBaseTest.tel_test_wrap
    def test_neighbor_cell_reporting_wcdma_intrafreq_2_tmo(self):
        """ Test Number of neighbor cells reported by Phone when two neighbor
        cells are present (Phone camped on WCDMA)

        Setup two WCDMA cell configuration on MD8475A
        Setup one WCDMA waveform on MG3710A
        Make Sure Phone camped on WCDMA
        Verify the number of neighbor cells reported by Phone

        Returns:
            True if pass; False if fail
        """
        serving_cell = wcdma_band1_ch10700_fr2140_cid31_cell
        neighbor_cell_1 = wcdma_band1_ch10700_fr2140_cid32_cell
        neighbor_cell_2 = wcdma_band1_ch10700_fr2140_cid33_cell
        serving_cell['power'] = -20
        neighbor_cell_1['power'] = -24
        neighbor_cell_2['power'] = -22
        expected_cell_info_list = [
            # Serving Cell
            {
                self.CID: serving_cell[self.CID],
                self.PSC: serving_cell[self.PSC],
                self.REPORT_RATE: 1,
                self.IS_REGISTERED: True,
                self.RAT: 'wcdma',
                self.TARGET_RSSI:
                self._WCDMA_RSSI_OFFSET + serving_cell['power'],
                self.MAX_ERROR_RSSI: 3,
                self.MAX_ERROR_AVERAGE_RSSI: 3
            },
            # Neighbor Cells
            {
                self.PSC: neighbor_cell_1[self.PSC],
                self.REPORT_RATE: 0.1,
                self.IS_REGISTERED: False,
                self.RAT: 'wcdma',
                self.TARGET_RSSI:
                self._WCDMA_RSSI_OFFSET + neighbor_cell_1['power'],
                self.MAX_ERROR_RSSI: 4,
                self.MAX_ERROR_AVERAGE_RSSI: 3
            },
            {
                self.PSC: neighbor_cell_2[self.PSC],
                self.REPORT_RATE: 0.1,
                self.IS_REGISTERED: False,
                self.RAT: 'wcdma',
                self.TARGET_RSSI:
                self._WCDMA_RSSI_OFFSET + neighbor_cell_2['power'],
                self.MAX_ERROR_RSSI: 4,
                self.MAX_ERROR_AVERAGE_RSSI: 3
            }
        ]
        expected_cell_info_stats = {}
        for sample in expected_cell_info_list:
            expected_cell_info_stats[self._unique_cell_id(sample)] = sample

        self.md8475a.load_cell_paramfile(self._CELL_PARAM_FILE)
        [bts1, bts2] = set_system_model_wcdma_wcdma(
            self.md8475a, self.user_params, self.ad.sim_card)
        self._setup_wcdma_cell_md8475a(bts1, serving_cell,
                                       serving_cell['power'])
        self._setup_wcdma_cell_md8475a(bts2, neighbor_cell_1,
                                       neighbor_cell_1['power'])
        bts1.neighbor_cell_mode = "USERDATA"
        bts1.set_neighbor_cell_type("WCDMA", 1, "CELLNAME")
        bts1.set_neighbor_cell_name("WCDMA", 1, "WCDM_1_C10700_F2140_CID32")
        bts1.set_neighbor_cell_type("WCDMA", 2, "CELLNAME")
        bts1.set_neighbor_cell_name("WCDMA", 2, "WCDM_1_C10700_F2140_CID33")
        set_usim_parameters(self.md8475a, self.ad.sim_card)
        self.md8475a.start_simulation()
        # To make sure phone camps on BTS1
        bts2.service_state = BtsServiceState.SERVICE_STATE_OUT

        if not ensure_network_rat(
                self.log,
                self.ad,
                NETWORK_MODE_GSM_UMTS,
                RAT_FAMILY_UMTS,
                toggle_apm_after_setting=True):
            self.log.error("Failed to set rat family {}, preferred network:{}".
                           format(RAT_FAMILY_UMTS, NETWORK_MODE_GSM_UMTS))
            return False
        self.md8475a.wait_for_registration_state()
        time.sleep(self._ANRITSU_SETTLING_TIME)
        bts2.service_state = BtsServiceState.SERVICE_STATE_IN
        self.setup_3710a_waveform("1", "A", "2140MHz",
                                  neighbor_cell_2['power'], "WCDMA",
                                  "wcdma_1_psc33_cid33")
        self.turn_on_3710a_sg(1)
        time.sleep(self._SETTLING_TIME)
        return self._verify_cell_info(self.ad, expected_cell_info_stats)

    @test_tracker_info(uuid="3ec77512-4d5b-40c9-b733-cf358f999e15")
    @TelephonyBaseTest.tel_test_wrap
    def test_neighbor_cell_reporting_wcdma_intrafreq_3_tmo(self):
        """ Test Number of neighbor cells reported by Phone when three neighbor
        cells are present (Phone camped on WCDMA)

        Setup two WCDMA cell configuration on MD8475A
        Setup two WCDMA waveform on MG3710A
        Make Sure Phone camped on WCDMA
        Verify the number of neighbor cells reported by Phone

        Returns:
            True if pass; False if fail
        """
        serving_cell = wcdma_band1_ch10700_fr2140_cid31_cell
        neighbor_cell_1 = wcdma_band1_ch10700_fr2140_cid32_cell
        neighbor_cell_2 = wcdma_band1_ch10700_fr2140_cid33_cell
        neighbor_cell_3 = wcdma_band1_ch10700_fr2140_cid34_cell
        serving_cell['power'] = -20
        neighbor_cell_1['power'] = -24
        neighbor_cell_2['power'] = -23
        neighbor_cell_3['power'] = -22

        expected_cell_info_list = [
            # Serving Cell
            {
                self.CID: serving_cell[self.CID],
                self.PSC: serving_cell[self.PSC],
                self.REPORT_RATE: 1,
                self.IS_REGISTERED: True,
                self.RAT: 'wcdma',
                self.TARGET_RSSI:
                self._WCDMA_RSSI_OFFSET + serving_cell['power'],
                self.MAX_ERROR_RSSI: 3,
                self.MAX_ERROR_AVERAGE_RSSI: 3
            },
            # Neighbor Cells
            {
                self.PSC: neighbor_cell_1[self.PSC],
                self.REPORT_RATE: 0.1,
                self.IS_REGISTERED: False,
                self.RAT: 'wcdma',
                self.TARGET_RSSI:
                self._WCDMA_RSSI_OFFSET + neighbor_cell_1['power'],
                self.MAX_ERROR_RSSI: 4,
                self.MAX_ERROR_AVERAGE_RSSI: 3
            },
            {
                self.PSC: neighbor_cell_2[self.PSC],
                self.REPORT_RATE: 0.1,
                self.IS_REGISTERED: False,
                self.RAT: 'wcdma',
                self.TARGET_RSSI:
                self._WCDMA_RSSI_OFFSET + neighbor_cell_2['power'],
                self.MAX_ERROR_RSSI: 4,
                self.MAX_ERROR_AVERAGE_RSSI: 3
            },
            {
                self.PSC: neighbor_cell_3[self.PSC],
                self.REPORT_RATE: 0.1,
                self.IS_REGISTERED: False,
                self.RAT: 'wcdma',
                self.TARGET_RSSI:
                self._WCDMA_RSSI_OFFSET + neighbor_cell_3['power'],
                self.MAX_ERROR_RSSI: 4,
                self.MAX_ERROR_AVERAGE_RSSI: 3
            }
        ]
        expected_cell_info_stats = {}
        for sample in expected_cell_info_list:
            expected_cell_info_stats[self._unique_cell_id(sample)] = sample

        self.md8475a.load_cell_paramfile(self._CELL_PARAM_FILE)
        [bts1, bts2] = set_system_model_wcdma_wcdma(
            self.md8475a, self.user_params, self.ad.sim_card)
        self._setup_wcdma_cell_md8475a(bts1, serving_cell,
                                       serving_cell['power'])
        self._setup_wcdma_cell_md8475a(bts2, neighbor_cell_1,
                                       neighbor_cell_1['power'])
        bts1.neighbor_cell_mode = "USERDATA"
        bts1.set_neighbor_cell_type("WCDMA", 1, "CELLNAME")
        bts1.set_neighbor_cell_name("WCDMA", 1, "WCDM_1_C10700_F2140_CID32")
        bts1.set_neighbor_cell_type("WCDMA", 2, "CELLNAME")
        bts1.set_neighbor_cell_name("WCDMA", 2, "WCDM_1_C10700_F2140_CID33")
        bts1.set_neighbor_cell_type("WCDMA", 3, "CELLNAME")
        bts1.set_neighbor_cell_name("WCDMA", 3, "WCDM_1_C10700_F2140_CID34")
        set_usim_parameters(self.md8475a, self.ad.sim_card)
        self.md8475a.start_simulation()
        # To make sure phone camps on BTS1
        bts2.service_state = BtsServiceState.SERVICE_STATE_OUT

        if not ensure_network_rat(
                self.log,
                self.ad,
                NETWORK_MODE_GSM_UMTS,
                RAT_FAMILY_UMTS,
                toggle_apm_after_setting=True):
            self.log.error("Failed to set rat family {}, preferred network:{}".
                           format(RAT_FAMILY_UMTS, NETWORK_MODE_GSM_UMTS))
            return False
        self.md8475a.wait_for_registration_state()
        time.sleep(self._ANRITSU_SETTLING_TIME)
        bts2.service_state = BtsServiceState.SERVICE_STATE_IN
        self.setup_3710a_waveform("1", "A", "2140MHz",
                                  neighbor_cell_2['power'], "WCDMA",
                                  "wcdma_1_psc33_cid33")

        self.setup_3710a_waveform("2", "A", "2140MHz",
                                  neighbor_cell_3['power'], "WCDMA",
                                  "wcdma_1_psc34_cid34")
        self.turn_on_3710a_sg(1)
        self.turn_on_3710a_sg(2)
        time.sleep(self._SETTLING_TIME)
        return self._verify_cell_info(self.ad, expected_cell_info_stats)

    @test_tracker_info(uuid="6f39e4a5-81da-4f47-8022-f22d82ff6f31")
    @TelephonyBaseTest.tel_test_wrap
    def test_neighbor_cell_reporting_wcdma_interfreq_1_tmo(self):
        """ Test Number of neighbor cells reported by Phone when two neighbor
        cells(inter frequency) are present (Phone camped on WCDMA)

        Setup a two WCDMA cell configuration on MD8475A
        Setup one WCDMA waveform on MG3710A
        Make Sure Phone camped on WCDMA
        Verify the number of neighbor cells reported by Phonene

        Returns:
            True if pass; False if fail
        """
        serving_cell = wcdma_band1_ch10700_fr2140_cid31_cell
        neighbor_cell_1 = wcdma_band1_ch10800_fr2160_cid37_cell
        serving_cell['power'] = -20
        neighbor_cell_1['power'] = -24

        expected_cell_info_list = [
            # Serving Cell
            {
                self.CID: serving_cell[self.CID],
                self.PSC: serving_cell[self.PSC],
                self.REPORT_RATE: 1,
                self.IS_REGISTERED: True,
                self.RAT: 'wcdma',
                self.TARGET_RSSI:
                self._WCDMA_RSSI_OFFSET + serving_cell['power'],
                self.MAX_ERROR_RSSI: 3,
                self.MAX_ERROR_AVERAGE_RSSI: 3
            },
            # Neighbor Cells
            {
                self.PSC: neighbor_cell_1[self.PSC],
                self.REPORT_RATE: 0.1,
                self.IS_REGISTERED: False,
                self.RAT: 'wcdma',
                self.TARGET_RSSI:
                self._WCDMA_RSSI_OFFSET + neighbor_cell_1['power'],
                self.MAX_ERROR_RSSI: 4,
                self.MAX_ERROR_AVERAGE_RSSI: 3
            }
        ]
        expected_cell_info_stats = {}
        for sample in expected_cell_info_list:
            expected_cell_info_stats[self._unique_cell_id(sample)] = sample

        self.md8475a.load_cell_paramfile(self._CELL_PARAM_FILE)
        [bts1, bts2] = set_system_model_wcdma_wcdma(
            self.md8475a, self.user_params, self.ad.sim_card)
        self._setup_wcdma_cell_md8475a(bts1, serving_cell,
                                       serving_cell['power'])
        self._setup_wcdma_cell_md8475a(bts2, neighbor_cell_1,
                                       neighbor_cell_1['power'])
        bts1.neighbor_cell_mode = "USERDATA"
        bts1.set_neighbor_cell_type("WCDMA", 1, "CELLNAME")
        bts1.set_neighbor_cell_name("WCDMA", 1, "WCDM_1_C10800_F2160_CID37")
        set_usim_parameters(self.md8475a, self.ad.sim_card)
        self.md8475a.start_simulation()
        #To make sure phone camps on BTS1
        bts2.service_state = BtsServiceState.SERVICE_STATE_OUT

        self.ad.droid.telephonyToggleDataConnection(False)
        if not ensure_network_rat(
                self.log,
                self.ad,
                NETWORK_MODE_GSM_UMTS,
                RAT_FAMILY_UMTS,
                toggle_apm_after_setting=True):
            self.log.error("Failed to set rat family {}, preferred network:{}".
                           format(RAT_FAMILY_UMTS, NETWORK_MODE_GSM_UMTS))
            return False
        self.md8475a.wait_for_registration_state()
        time.sleep(self._ANRITSU_SETTLING_TIME)
        bts2.service_state = BtsServiceState.SERVICE_STATE_IN
        time.sleep(self._SETTLING_TIME)
        return self._verify_cell_info(self.ad, expected_cell_info_stats)

    @test_tracker_info(uuid="992d9ffb-2538-447b-b7e8-f40061063686")
    @TelephonyBaseTest.tel_test_wrap
    def test_neighbor_cell_reporting_wcdma_interfreq_2_tmo(self):
        """ Test Number of neighbor cells reported by Phone when two neighbor
        cells(inter frequency) are present (Phone camped on WCDMA)

        Setup a two WCDMA cell configuration on MD8475A
        Setup one WCDMA waveform on MG3710A
        Make Sure Phone camped on WCDMA
        Verify the number of neighbor cells reported by Phonene

        Returns:
            True if pass; False if fail
        """
        serving_cell = wcdma_band1_ch10700_fr2140_cid31_cell
        neighbor_cell_1 = wcdma_band1_ch10575_fr2115_cid36_cell
        neighbor_cell_2 = wcdma_band1_ch10800_fr2160_cid37_cell
        serving_cell['power'] = -20
        neighbor_cell_1['power'] = -24
        neighbor_cell_2['power'] = -23
        expected_cell_info_list = [
            # Serving Cell
            {
                self.CID: serving_cell[self.CID],
                self.PSC: serving_cell[self.PSC],
                self.REPORT_RATE: 1,
                self.IS_REGISTERED: True,
                self.RAT: 'wcdma',
                self.TARGET_RSSI:
                self._WCDMA_RSSI_OFFSET + serving_cell['power'],
                self.MAX_ERROR_RSSI: 3,
                self.MAX_ERROR_AVERAGE_RSSI: 3
            },
            # Neighbor Cells
            {
                self.PSC: neighbor_cell_1[self.PSC],
                self.REPORT_RATE: 0.1,
                self.IS_REGISTERED: False,
                self.RAT: 'wcdma',
                self.TARGET_RSSI:
                self._WCDMA_RSSI_OFFSET + neighbor_cell_1['power'],
                self.MAX_ERROR_RSSI: 4,
                self.MAX_ERROR_AVERAGE_RSSI: 3
            },
            {
                self.PSC: neighbor_cell_2[self.PSC],
                self.REPORT_RATE: 0.1,
                self.IS_REGISTERED: False,
                self.RAT: 'wcdma',
                self.TARGET_RSSI:
                self._WCDMA_RSSI_OFFSET + neighbor_cell_2['power'],
                self.MAX_ERROR_RSSI: 4,
                self.MAX_ERROR_AVERAGE_RSSI: 3
            }
        ]
        expected_cell_info_stats = {}
        for sample in expected_cell_info_list:
            expected_cell_info_stats[self._unique_cell_id(sample)] = sample

        self.md8475a.load_cell_paramfile(self._CELL_PARAM_FILE)
        [bts1, bts2] = set_system_model_wcdma_wcdma(
            self.md8475a, self.user_params, self.ad.sim_card)
        self._setup_wcdma_cell_md8475a(bts1, serving_cell,
                                       serving_cell['power'])
        self._setup_wcdma_cell_md8475a(bts2, neighbor_cell_1,
                                       neighbor_cell_1['power'])
        bts1.neighbor_cell_mode = "USERDATA"
        bts1.set_neighbor_cell_type("WCDMA", 1, "CELLNAME")
        bts1.set_neighbor_cell_name("WCDMA", 1, "WCDM_1_C10575_F2115_CID36")
        bts1.set_neighbor_cell_type("WCDMA", 2, "CELLNAME")
        bts1.set_neighbor_cell_name("WCDMA", 2, "WCDM_1_C10800_F2160_CID37")
        set_usim_parameters(self.md8475a, self.ad.sim_card)
        self.md8475a.start_simulation()
        # To make sure phone camps on BTS1
        bts2.service_state = BtsServiceState.SERVICE_STATE_OUT

        self.ad.droid.telephonyToggleDataConnection(False)
        if not ensure_network_rat(
                self.log,
                self.ad,
                NETWORK_MODE_GSM_UMTS,
                RAT_FAMILY_UMTS,
                toggle_apm_after_setting=True):
            self.log.error("Failed to set rat family {}, preferred network:{}".
                           format(RAT_FAMILY_UMTS, NETWORK_MODE_GSM_UMTS))
            return False
        self.md8475a.wait_for_registration_state()
        time.sleep(self._ANRITSU_SETTLING_TIME)
        bts2.service_state = BtsServiceState.SERVICE_STATE_IN
        self.setup_3710a_waveform("1", "A", "2160MHz",
                                  neighbor_cell_2['power'], "WCDMA",
                                  "wcdma_1_psc37_cid37")
        self.turn_on_3710a_sg(1)
        time.sleep(self._SETTLING_TIME)
        return self._verify_cell_info(self.ad, expected_cell_info_stats)

    @test_tracker_info(uuid="60cb8c15-3cb3-4ead-9e59-a8aee819e9ef")
    @TelephonyBaseTest.tel_test_wrap
    def test_neighbor_cell_reporting_wcdma_interband_2_tmo(self):
        """ Test Number of neighbor cells reported by Phone when two neighbor
        cells(inter band) are present (Phone camped on WCDMA)

        Setup a two WCDMA cell configuration on MD8475A
        Setup one WCDMA waveform on MG3710A
        Make Sure Phone camped on WCDMA
        Verify the number of neighbor cells reported by Phone

        Returns:
            True if pass; False if fail
        """
        serving_cell = wcdma_band1_ch10700_fr2140_cid31_cell
        neighbor_cell_1 = wcdma_band2_ch9800_fr1960_cid38_cell
        neighbor_cell_2 = wcdma_band2_ch9900_fr1980_cid39_cell
        serving_cell['power'] = -20
        neighbor_cell_1['power'] = -23
        neighbor_cell_2['power'] = -22

        expected_cell_info_list = [
            # Serving Cell
            {
                self.CID: serving_cell[self.CID],
                self.PSC: serving_cell[self.PSC],
                self.REPORT_RATE: 1,
                self.IS_REGISTERED: True,
                self.RAT: 'wcdma',
                self.TARGET_RSSI:
                self._WCDMA_RSSI_OFFSET + serving_cell['power'],
                self.MAX_ERROR_RSSI: 3,
                self.MAX_ERROR_AVERAGE_RSSI: 3
            },
            # Neighbor Cells
            {
                self.PSC: neighbor_cell_1[self.PSC],
                self.REPORT_RATE: 0.1,
                self.IS_REGISTERED: False,
                self.RAT: 'wcdma',
                self.TARGET_RSSI:
                self._WCDMA_RSSI_OFFSET + neighbor_cell_1['power'],
                self.MAX_ERROR_RSSI: 4,
                self.MAX_ERROR_AVERAGE_RSSI: 3
            },
            {
                self.PSC: neighbor_cell_2[self.PSC],
                self.REPORT_RATE: 0.1,
                self.IS_REGISTERED: False,
                self.RAT: 'wcdma',
                self.TARGET_RSSI:
                self._WCDMA_RSSI_OFFSET + neighbor_cell_2['power'],
                self.MAX_ERROR_RSSI: 4,
                self.MAX_ERROR_AVERAGE_RSSI: 3
            }
        ]
        expected_cell_info_stats = {}
        for sample in expected_cell_info_list:
            expected_cell_info_stats[self._unique_cell_id(sample)] = sample

        self.md8475a.load_cell_paramfile(self._CELL_PARAM_FILE)
        [bts1, bts2] = set_system_model_wcdma_wcdma(
            self.md8475a, self.user_params, self.ad.sim_card)
        self._setup_wcdma_cell_md8475a(bts1, serving_cell,
                                       serving_cell['power'])
        self._setup_wcdma_cell_md8475a(bts2, neighbor_cell_1,
                                       neighbor_cell_1['power'])
        bts1.neighbor_cell_mode = "USERDATA"
        bts1.set_neighbor_cell_type("WCDMA", 1, "CELLNAME")
        bts1.set_neighbor_cell_name("WCDMA", 1, "WCDM_2_C9800_F1960_CID38")
        bts1.set_neighbor_cell_type("WCDMA", 2, "CELLNAME")
        bts1.set_neighbor_cell_name("WCDMA", 2, "WCDM_2_C9900_F1980_CID39")
        set_usim_parameters(self.md8475a, self.ad.sim_card)
        self.md8475a.start_simulation()
        # To make sure phone camps on BTS1
        bts2.service_state = BtsServiceState.SERVICE_STATE_OUT

        self.ad.droid.telephonyToggleDataConnection(False)
        if not ensure_network_rat(
                self.log,
                self.ad,
                NETWORK_MODE_GSM_UMTS,
                RAT_FAMILY_UMTS,
                toggle_apm_after_setting=True):
            self.log.error("Failed to set rat family {}, preferred network:{}".
                           format(RAT_FAMILY_UMTS, NETWORK_MODE_GSM_UMTS))
            return False
        self.md8475a.wait_for_registration_state()
        time.sleep(self._ANRITSU_SETTLING_TIME)
        bts2.service_state = BtsServiceState.SERVICE_STATE_IN
        self.setup_3710a_waveform("1", "A", "1980MHz",
                                  neighbor_cell_2['power'], "WCDMA",
                                  "wcdma_2_psc39_cid39")
        self.turn_on_3710a_sg(1)
        time.sleep(self._SETTLING_TIME)
        return self._verify_cell_info(self.ad, expected_cell_info_stats)

    @test_tracker_info(uuid="daa29f27-f67b-47ee-9a30-1c9572eedf2f")
    @TelephonyBaseTest.tel_test_wrap
    def test_neighbor_cell_reporting_wcdma_interrat_1_tmo(self):
        """ Test Number of neighbor cells reported by Phone when two neighbor
        cells are present (Phone camped on WCDMA)

        Setup a two WCDMA cell configuration on MD8475A
        Setup one WCDMA waveform on MG3710A
        Make Sure Phone camped on WCDMA
        Verify the number of neighbor cells reported by Phone

        Returns:
            True if pass; False if fail
        """
        serving_cell = wcdma_band1_ch10700_fr2140_cid31_cell
        neighbor_cell_1 = gsm_band1900_ch512_fr1930_cid51_cell
        neighbor_cell_2 = lte_band4_ch2000_fr2115_pcid1_cell
        serving_cell['power'] = -20
        neighbor_cell_1['power'] = -23
        neighbor_cell_2['power'] = -22

        expected_cell_info_list = [
            # Serving Cell
            {
                self.CID: serving_cell[self.CID],
                self.PSC: serving_cell[self.PSC],
                self.REPORT_RATE: 1,
                self.IS_REGISTERED: True,
                self.RAT: 'wcdma',
                self.TARGET_RSSI:
                self._WCDMA_RSSI_OFFSET + serving_cell['power'],
                self.MAX_ERROR_RSSI: 3,
                self.MAX_ERROR_AVERAGE_RSSI: 3
            },
            # Neighbor Cells
            {
                self.CID: neighbor_cell_1[self.CID],
                self.REPORT_RATE: 0.1,
                self.IS_REGISTERED: False,
                self.RAT: 'gsm',
                self.TARGET_RSSI:
                self._GSM_RSSI_OFFSET + neighbor_cell_1['power'],
                self.MAX_ERROR_RSSI: 4,
                self.MAX_ERROR_AVERAGE_RSSI: 3
            },
            {
                self.PCID: neighbor_cell_2[self.PCID],
                self.REPORT_RATE: 0.1,
                self.IS_REGISTERED: False,
                self.RAT: 'lte',
                self.TARGET_RSSI:
                self._LTE_RSSI_OFFSET + neighbor_cell_2['power'],
                self.MAX_ERROR_RSSI: 4,
                self.MAX_ERROR_AVERAGE_RSSI: 3
            }
        ]
        expected_cell_info_stats = {}
        for sample in expected_cell_info_list:
            expected_cell_info_stats[self._unique_cell_id(sample)] = sample

        self.md8475a.load_cell_paramfile(self._CELL_PARAM_FILE)
        [bts1, bts2] = set_system_model_lte_wcdma(
            self.md8475a, self.user_params, self.ad.sim_card)
        self._setup_wcdma_cell_md8475a(bts2, serving_cell,
                                       serving_cell['power'])
        self._setup_lte_cell_md8475a(bts1, neighbor_cell_2,
                                     neighbor_cell_2['power'])
        bts2.neighbor_cell_mode = "USERDATA"
        bts2.set_neighbor_cell_type("LTE", 1, "CELLNAME")
        bts2.set_neighbor_cell_name("LTE", 1, "LTE_4_C2000_F2115_PCID1")
        set_usim_parameters(self.md8475a, self.ad.sim_card)
        self.md8475a.start_simulation()
        # To make sure phone camps on BTS1
        bts1.service_state = BtsServiceState.SERVICE_STATE_OUT

        self.ad.droid.telephonyToggleDataConnection(False)
        if not ensure_network_rat(
                self.log,
                self.ad,
                NETWORK_MODE_GSM_UMTS,
                RAT_FAMILY_UMTS,
                toggle_apm_after_setting=True):
            self.log.error("Failed to set rat family {}, preferred network:{}".
                           format(RAT_FAMILY_UMTS, NETWORK_MODE_GSM_UMTS))
            return False
        self.md8475a.wait_for_registration_state()
        time.sleep(self._ANRITSU_SETTLING_TIME)
        bts1.service_state = BtsServiceState.SERVICE_STATE_IN
        time.sleep(self._SETTLING_TIME)
        return self._verify_cell_info(self.ad, expected_cell_info_stats)

    @test_tracker_info(uuid="08e5d666-fae6-48a3-b03b-de7b7b3f5982")
    @TelephonyBaseTest.tel_test_wrap
    def test_neighbor_cell_reporting_wcdma_interrat_2_tmo(self):
        """ Test Number of neighbor cells reported by Phone when two neighbor
        cells are present (Phone camped on WCDMA)

        Setup a two WCDMA cell configuration on MD8475A
        Setup one WCDMA waveform on MG3710A
        Make Sure Phone camped on WCDMA
        Verify the number of neighbor cells reported by Phone

        Returns:
            True if pass; False if fail
        """
        serving_cell = wcdma_band1_ch10700_fr2140_cid31_cell
        neighbor_cell_1 = gsm_band1900_ch512_fr1930_cid51_cell
        neighbor_cell_2 = lte_band4_ch2000_fr2115_pcid1_cell
        serving_cell['power'] = -20
        neighbor_cell_1['power'] = -23
        neighbor_cell_2['power'] = -22

        expected_cell_info_list = [
            # Serving Cell
            {
                self.CID: serving_cell[self.CID],
                self.PSC: serving_cell[self.PSC],
                self.REPORT_RATE: 1,
                self.IS_REGISTERED: True,
                self.RAT: 'wcdma',
                self.TARGET_RSSI:
                self._WCDMA_RSSI_OFFSET + serving_cell['power'],
                self.MAX_ERROR_RSSI: 3,
                self.MAX_ERROR_AVERAGE_RSSI: 3
            },
            # Neighbor Cells
            {
                self.CID: neighbor_cell_1[self.CID],
                self.REPORT_RATE: 0.1,
                self.IS_REGISTERED: False,
                self.RAT: 'gsm',
                self.TARGET_RSSI:
                self._GSM_RSSI_OFFSET + neighbor_cell_1['power'],
                self.MAX_ERROR_RSSI: 4,
                self.MAX_ERROR_AVERAGE_RSSI: 3
            },
            {
                self.PCID: neighbor_cell_2[self.PCID],
                self.REPORT_RATE: 0.1,
                self.IS_REGISTERED: False,
                self.RAT: 'lte',
                self.TARGET_RSSI:
                self._LTE_RSSI_OFFSET + neighbor_cell_2['power'],
                self.MAX_ERROR_RSSI: 4,
                self.MAX_ERROR_AVERAGE_RSSI: 3
            }
        ]
        expected_cell_info_stats = {}
        for sample in expected_cell_info_list:
            expected_cell_info_stats[self._unique_cell_id(sample)] = sample

        self.md8475a.load_cell_paramfile(self._CELL_PARAM_FILE)
        [bts1, bts2] = set_system_model_wcdma_gsm(
            self.md8475a, self.user_params, self.ad.sim_card)
        self._setup_wcdma_cell_md8475a(bts1, serving_cell,
                                       serving_cell['power'])
        self._setup_gsm_cell_md8475a(bts2, neighbor_cell_1,
                                     neighbor_cell_1['power'])
        bts1.neighbor_cell_mode = "USERDATA"
        bts1.set_neighbor_cell_type("GSM", 1, "CELLNAME")
        bts1.set_neighbor_cell_name("GSM", 1, "GSM_1900_C512_F1930_CID51")
        bts1.set_neighbor_cell_type("LTE", 1, "CELLNAME")
        bts1.set_neighbor_cell_name("LTE", 1, "LTE_4_C2000_F2115_PCID1")
        set_usim_parameters(self.md8475a, self.ad.sim_card)
        self.md8475a.start_simulation()
        # To make sure phone camps on BTS1
        bts2.service_state = BtsServiceState.SERVICE_STATE_OUT

        self.ad.droid.telephonyToggleDataConnection(False)
        if not ensure_network_rat(
                self.log,
                self.ad,
                NETWORK_MODE_GSM_UMTS,
                RAT_FAMILY_UMTS,
                toggle_apm_after_setting=True):
            self.log.error("Failed to set rat family {}, preferred network:{}".
                           format(RAT_FAMILY_UMTS, NETWORK_MODE_GSM_UMTS))
            return False
        self.md8475a.wait_for_registration_state()
        time.sleep(self._ANRITSU_SETTLING_TIME)
        bts2.service_state = BtsServiceState.SERVICE_STATE_IN
        self.setup_3710a_waveform("1", "A", "2115MHz",
                                  neighbor_cell_2['power'], "LTE",
                                  "lte_4_ch2000_pcid1")
        self.turn_on_3710a_sg(1)
        time.sleep(self._SETTLING_TIME)
        return self._verify_cell_info(self.ad, expected_cell_info_stats)

    @test_tracker_info(uuid="bebbe764-4c8c-4aaf-81b9-c61509a9695e")
    @TelephonyBaseTest.tel_test_wrap
    def test_neighbor_cell_reporting_gsm_intrafreq_0_tmo(self):
        """ Test Number of neighbor cells reported by Phone when no neighbor
        cells are present (Phone camped on GSM)

        Setup one GSM cell configuration on MD8475A
        Make Sure Phone camped on GSM
        Verify the number of neighbor cells reported by Phone

        Returns:
            True if pass; False if fail
        """
        serving_cell = gsm_band1900_ch512_fr1930_cid51_cell
        serving_cell['power'] = -30

        expected_cell_info_list = [
            # Serving Cell
            {
                self.CID: serving_cell[self.CID],
                self.REPORT_RATE: 1,
                self.IS_REGISTERED: True,
                self.RAT: 'gsm',
                self.TARGET_RSSI:
                self._GSM_RSSI_OFFSET + serving_cell['power'],
                self.MAX_ERROR_RSSI: 3,
                self.MAX_ERROR_AVERAGE_RSSI: 3
            }
        ]
        expected_cell_info_stats = {}
        for sample in expected_cell_info_list:
            expected_cell_info_stats[self._unique_cell_id(sample)] = sample

        [bts1] = set_system_model_gsm(self.md8475a, self.user_params,
                                      self.ad.sim_card)
        self._setup_gsm_cell_md8475a(bts1, serving_cell, serving_cell['power'])
        set_usim_parameters(self.md8475a, self.ad.sim_card)
        self.md8475a.start_simulation()
        if not ensure_network_rat(
                self.log,
                self.ad,
                NETWORK_MODE_GSM_ONLY,
                RAT_FAMILY_GSM,
                toggle_apm_after_setting=True):
            self.log.error("Failed to set rat family {}, preferred network:{}".
                           format(RAT_FAMILY_GSM, NETWORK_MODE_GSM_ONLY))
            return False
        self.md8475a.wait_for_registration_state()
        time.sleep(self._SETTLING_TIME)
        return self._verify_cell_info(self.ad, expected_cell_info_stats)

    @test_tracker_info(uuid="861dd399-d6f6-4e9f-9e8d-0718966ea45a")
    @TelephonyBaseTest.tel_test_wrap
    def test_neighbor_cell_reporting_gsm_intrafreq_1_tmo(self):
        """ Test Number of neighbor cells reported by Phone when one neighbor
        cell is present (Phone camped on GSM)

        Setup one GSM cell configuration on MD8475A
        Make Sure Phone camped on GSM
        Verify the number of neighbor cells reported by Phone

        Returns:
            True if pass; False if fail
        """
        serving_cell = gsm_band1900_ch512_fr1930_cid51_cell
        neighbor_cell = gsm_band1900_ch512_fr1930_cid52_cell
        serving_cell['power'] = -20
        neighbor_cell['power'] = -22

        expected_cell_info_list = [
            # Serving Cell
            {
                self.CID: serving_cell[self.CID],
                self.REPORT_RATE: 1,
                self.IS_REGISTERED: True,
                self.RAT: 'gsm',
                self.TARGET_RSSI:
                self._GSM_RSSI_OFFSET + serving_cell['power'],
                self.MAX_ERROR_RSSI: 3,
                self.MAX_ERROR_AVERAGE_RSSI: 3
            },
            # Neighbor Cells
            {
                self.CID: neighbor_cell_1[self.CID],
                self.REPORT_RATE: 0.1,
                self.IS_REGISTERED: False,
                self.RAT: 'gsm',
                self.TARGET_RSSI:
                self._GSM_RSSI_OFFSET + neighbor_cell_1['power'],
                self.MAX_ERROR_RSSI: 4,
                self.MAX_ERROR_AVERAGE_RSSI: 3
            }
        ]
        expected_cell_info_stats = {}
        for sample in expected_cell_info_list:
            expected_cell_info_stats[self._unique_cell_id(sample)] = sample

        self.md8475a.load_cell_paramfile(self._CELL_PARAM_FILE)
        [bts1] = set_system_model_gsm(self.md8475a, self.user_params,
                                      self.ad.sim_card)
        self._setup_gsm_cell_md8475a(bts1, serving_cell, serving_cell['power'])
        bts1.neighbor_cell_mode = "USERDATA"
        bts1.set_neighbor_cell_type("GSM", 1, "CELLNAME")
        bts1.set_neighbor_cell_name("GSM", 1, "GSM_1900_C512_F1930_CID52")
        set_usim_parameters(self.md8475a, self.ad.sim_card)
        self.md8475a.start_simulation()

        if not ensure_network_rat(
                self.log,
                self.ad,
                NETWORK_MODE_GSM_ONLY,
                RAT_FAMILY_GSM,
                toggle_apm_after_setting=True):
            self.log.error("Failed to set rat family {}, preferred network:{}".
                           format(RAT_FAMILY_GSM, NETWORK_MODE_GSM_ONLY))
            return False
        self.md8475a.wait_for_registration_state()
        time.sleep(self._ANRITSU_SETTLING_TIME)
        self.setup_3710a_waveform("1", "A", "1930.2MHz",
                                  neighbor_cell['power'], "GSM",
                                  "gsm_lac52_cid52")

        self.turn_on_3710a_sg(1)
        time.sleep(self._SETTLING_TIME)
        return self._verify_cell_info(self.ad, expected_cell_info_stats)

    @test_tracker_info(uuid="58627a33-45bd-436d-85b2-1ca711f56794")
    @TelephonyBaseTest.tel_test_wrap
    def test_neighbor_cell_reporting_gsm_intrafreq_2_tmo(self):
        """ Test Number of neighbor cells reported by Phone when two neighbor
        cells are present (Phone camped on GSM)

        Setup one GSM cell configuration on MD8475A
        Setup two GSM waveforms on MG3710A
        Make Sure Phone camped on GSM
        Verify the number of neighbor cells reported by Phone

        Returns:
            True if pass; False if fail
        """
        serving_cell = gsm_band1900_ch512_fr1930_cid51_cell
        neighbor_cell_1 = gsm_band1900_ch512_fr1930_cid52_cell
        neighbor_cell_2 = gsm_band1900_ch512_fr1930_cid53_cell
        serving_cell['power'] = -20
        neighbor_cell_1['power'] = -24
        neighbor_cell_2['power'] = -22

        expected_cell_info_list = [
            # Serving Cell
            {
                self.CID: serving_cell[self.CID],
                self.REPORT_RATE: 1,
                self.IS_REGISTERED: True,
                self.RAT: 'gsm',
                self.TARGET_RSSI:
                self._GSM_RSSI_OFFSET + serving_cell['power'],
                self.MAX_ERROR_RSSI: 3,
                self.MAX_ERROR_AVERAGE_RSSI: 3
            },
            # Neighbor Cells
            {
                self.CID: neighbor_cell_1[self.CID],
                self.REPORT_RATE: 0.1,
                self.IS_REGISTERED: False,
                self.RAT: 'gsm',
                self.TARGET_RSSI:
                self._GSM_RSSI_OFFSET + neighbor_cell_1['power'],
                self.MAX_ERROR_RSSI: 4,
                self.MAX_ERROR_AVERAGE_RSSI: 3
            },
            {
                self.CID: neighbor_cell_2[self.CID],
                self.REPORT_RATE: 0.1,
                self.IS_REGISTERED: False,
                self.RAT: 'gsm',
                self.TARGET_RSSI:
                self._GSM_RSSI_OFFSET + neighbor_cell_2['power'],
                self.MAX_ERROR_RSSI: 4,
                self.MAX_ERROR_AVERAGE_RSSI: 3
            }
        ]
        expected_cell_info_stats = {}
        for sample in expected_cell_info_list:
            expected_cell_info_stats[self._unique_cell_id(sample)] = sample

        self.md8475a.load_cell_paramfile(self._CELL_PARAM_FILE)
        [bts1] = set_system_model_gsm(self.md8475a, self.user_params,
                                      self.ad.sim_card)
        self._setup_gsm_cell_md8475a(bts1, serving_cell, serving_cell['power'])
        bts1.neighbor_cell_mode = "USERDATA"
        bts1.set_neighbor_cell_type("GSM", 1, "CELLNAME")
        bts1.set_neighbor_cell_name("GSM", 1, "GSM_1900_C512_F1930_CID52")
        bts1.set_neighbor_cell_type("GSM", 2, "CELLNAME")
        bts1.set_neighbor_cell_name("GSM", 2, "GSM_1900_C512_F1930_CID53")
        set_usim_parameters(self.md8475a, self.ad.sim_card)
        self.md8475a.start_simulation()

        if not ensure_network_rat(
                self.log,
                self.ad,
                NETWORK_MODE_GSM_ONLY,
                RAT_FAMILY_GSM,
                toggle_apm_after_setting=True):
            self.log.error("Failed to set rat family {}, preferred network:{}".
                           format(RAT_FAMILY_GSM, NETWORK_MODE_GSM_ONLY))
            return False
        self.md8475a.wait_for_registration_state()
        self.setup_3710a_waveform("1", "A", "1930.2MHz",
                                  neighbor_cell_1['power'], "GSM",
                                  "gsm_lac52_cid52")

        self.setup_3710a_waveform("2", "A", "1930.2MHz",
                                  neighbor_cell_2['power'], "GSM",
                                  "gsm_lac53_cid53")
        self.turn_on_3710a_sg(1)
        self.turn_on_3710a_sg(2)
        time.sleep(self._SETTLING_TIME)
        return self._verify_cell_info(self.ad, expected_cell_info_stats)

    @test_tracker_info(uuid="3ff3439a-2e45-470a-a2d6-c63e37379f19")
    @TelephonyBaseTest.tel_test_wrap
    def test_neighbor_cell_reporting_gsm_intrafreq_3_tmo(self):
        """ Test Number of neighbor cells reported by Phone when three neighbor
        cells are present (Phone camped on GSM)

        Setup one GSM cell configuration on MD8475A
        Setup three GSM waveforms on MG3710A
        Make Sure Phone camped on GSM
        Verify the number of neighbor cells reported by Phone

        Returns:
            True if pass; False if fail
        """
        serving_cell = gsm_band1900_ch512_fr1930_cid51_cell
        neighbor_cell_1 = gsm_band1900_ch512_fr1930_cid52_cell
        neighbor_cell_2 = gsm_band1900_ch512_fr1930_cid53_cell
        neighbor_cell_3 = gsm_band1900_ch512_fr1930_cid54_cell
        serving_cell['power'] = -20
        neighbor_cell_1['power'] = -24
        neighbor_cell_2['power'] = -22
        neighbor_cell_3['power'] = -24

        expected_cell_info_list = [
            # Serving Cell
            {
                self.CID: serving_cell[self.CID],
                self.REPORT_RATE: 1,
                self.IS_REGISTERED: True,
                self.RAT: 'gsm',
                self.TARGET_RSSI:
                self._GSM_RSSI_OFFSET + serving_cell['power'],
                self.MAX_ERROR_RSSI: 3,
                self.MAX_ERROR_AVERAGE_RSSI: 3
            },
            # Neighbor Cells
            {
                self.CID: neighbor_cell_1[self.CID],
                self.REPORT_RATE: 0.1,
                self.IS_REGISTERED: False,
                self.RAT: 'gsm',
                self.TARGET_RSSI:
                self._GSM_RSSI_OFFSET + neighbor_cell_1['power'],
                self.MAX_ERROR_RSSI: 4,
                self.MAX_ERROR_AVERAGE_RSSI: 3
            },
            {
                self.CID: neighbor_cell_2[self.CID],
                self.REPORT_RATE: 0.1,
                self.IS_REGISTERED: False,
                self.RAT: 'gsm',
                self.TARGET_RSSI:
                self._GSM_RSSI_OFFSET + neighbor_cell_2['power'],
                self.MAX_ERROR_RSSI: 4,
                self.MAX_ERROR_AVERAGE_RSSI: 3
            },
            {
                self.CID: neighbor_cell_3[self.CID],
                self.REPORT_RATE: 0.1,
                self.IS_REGISTERED: False,
                self.RAT: 'gsm',
                self.TARGET_RSSI:
                self._GSM_RSSI_OFFSET + neighbor_cell_3['power'],
                self.MAX_ERROR_RSSI: 4,
                self.MAX_ERROR_AVERAGE_RSSI: 3
            }
        ]
        expected_cell_info_stats = {}
        for sample in expected_cell_info_list:
            expected_cell_info_stats[self._unique_cell_id(sample)] = sample

        self.md8475a.load_cell_paramfile(self._CELL_PARAM_FILE)
        [bts1] = set_system_model_gsm(self.md8475a, self.user_params,
                                      self.ad.sim_card)
        self._setup_gsm_cell_md8475a(bts1, serving_cell, serving_cell['power'])
        bts1.neighbor_cell_mode = "USERDATA"
        bts1.set_neighbor_cell_type("GSM", 1, "CELLNAME")
        bts1.set_neighbor_cell_name("GSM", 1, "GSM_1900_C512_F1930_CID52")
        bts1.set_neighbor_cell_type("GSM", 2, "CELLNAME")
        bts1.set_neighbor_cell_name("GSM", 2, "GSM_1900_C512_F1930_CID53")
        bts1.set_neighbor_cell_type("GSM", 3, "CELLNAME")
        bts1.set_neighbor_cell_name("GSM", 3, "GSM_1900_C512_F1930_CID53")
        set_usim_parameters(self.md8475a, self.ad.sim_card)
        self.md8475a.start_simulation()

        if not ensure_network_rat(
                self.log,
                self.ad,
                NETWORK_MODE_GSM_ONLY,
                RAT_FAMILY_GSM,
                toggle_apm_after_setting=True):
            self.log.error("Failed to set rat family {}, preferred network:{}".
                           format(RAT_FAMILY_GSM, NETWORK_MODE_GSM_ONLY))
            return False
        self.md8475a.wait_for_registration_state()
        self.setup_3710a_waveform("1", "A", "1930.2MHz",
                                  neighbor_cell_1['power'], "GSM",
                                  "gsm_lac52_cid52")

        self.setup_3710a_waveform("2", "A", "1930.2MHz",
                                  neighbor_cell_2['power'], "GSM",
                                  "gsm_lac53_cid53")

        self.setup_3710a_waveform("2", "B", "1930.2MHz",
                                  neighbor_cell_3['power'], "GSM",
                                  "gsm_lac54_cid54")
        self.turn_on_3710a_sg(1)
        self.turn_on_3710a_sg(2)
        time.sleep(self._SETTLING_TIME)
        return self._verify_cell_info(self.ad, expected_cell_info_stats)

    @test_tracker_info(uuid="0cac1370-144e-40a4-b6bc-66691926f898")
    @TelephonyBaseTest.tel_test_wrap
    def test_neighbor_cell_reporting_gsm_interfreq_1_tmo(self):
        """ Test Number of neighbor cells reported by Phone when one neighbor
        cells(inter frequency) is present (Phone camped on GSM)

        Setup one GSM cell configuration on MD8475A
        Setup two GSM waveforms on MG3710A
        Make Sure Phone camped on GSM
        Verify the number of neighbor cells reported by Phone

        Returns:
            True if pass; False if fail
        """
        serving_cell = gsm_band1900_ch512_fr1930_cid51_cell
        neighbor_cell_1 = gsm_band1900_ch640_fr1955_cid56_cell
        serving_cell['power'] = -20
        neighbor_cell_1['power'] = -24

        expected_cell_info_list = [
            # Serving Cell
            {
                self.CID: serving_cell[self.CID],
                self.REPORT_RATE: 1,
                self.IS_REGISTERED: True,
                self.RAT: 'gsm',
                self.TARGET_RSSI:
                self._GSM_RSSI_OFFSET + serving_cell['power'],
                self.MAX_ERROR_RSSI: 3,
                self.MAX_ERROR_AVERAGE_RSSI: 3
            },
            # Neighbor Cells
            {
                self.CID: neighbor_cell_1[self.CID],
                self.REPORT_RATE: 0.1,
                self.IS_REGISTERED: False,
                self.RAT: 'gsm',
                self.TARGET_RSSI:
                self._GSM_RSSI_OFFSET + neighbor_cell_1['power'],
                self.MAX_ERROR_RSSI: 4,
                self.MAX_ERROR_AVERAGE_RSSI: 3
            }
        ]
        expected_cell_info_stats = {}
        for sample in expected_cell_info_list:
            expected_cell_info_stats[self._unique_cell_id(sample)] = sample

        self.md8475a.load_cell_paramfile(self._CELL_PARAM_FILE)
        [bts1] = set_system_model_gsm(self.md8475a, self.user_params,
                                      self.ad.sim_card)
        self._setup_gsm_cell_md8475a(bts1, serving_cell, serving_cell['power'])
        bts1.neighbor_cell_mode = "USERDATA"
        bts1.set_neighbor_cell_type("GSM", 1, "CELLNAME")
        bts1.set_neighbor_cell_name("GSM", 1, "GSM_1900_C640_F1955_CID56")
        set_usim_parameters(self.md8475a, self.ad.sim_card)
        self.md8475a.start_simulation()

        self.ad.droid.telephonyToggleDataConnection(False)
        if not ensure_network_rat(
                self.log,
                self.ad,
                NETWORK_MODE_GSM_ONLY,
                RAT_FAMILY_GSM,
                toggle_apm_after_setting=True):
            self.log.error("Failed to set rat family {}, preferred network:{}".
                           format(RAT_FAMILY_GSM, NETWORK_MODE_GSM_ONLY))
            return False
        self.md8475a.wait_for_registration_state()
        self.setup_3710a_waveform("1", "A", "1955.8MHz",
                                  neighbor_cell_1['power'], "GSM",
                                  "gsm_lac56_cid56")

        self.turn_on_3710a_sg(1)
        time.sleep(self._SETTLING_TIME)
        return self._verify_cell_info(self.ad, expected_cell_info_stats)

    @test_tracker_info(uuid="5f0367dd-08b5-4871-a784-51a0f76e229b")
    @TelephonyBaseTest.tel_test_wrap
    def test_neighbor_cell_reporting_gsm_interfreq_2_tmo(self):
        """ Test Number of neighbor cells reported by Phone when two neighbor
        cells(inter frequency) are present (Phone camped on GSM)

        Setup one GSM cell configuration on MD8475A
        Setup two GSM waveforms on MG3710A
        Make Sure Phone camped on GSM
        Verify the number of neighbor cells reported by Phone

        Returns:
            True if pass; False if fail
        """
        serving_cell = gsm_band1900_ch512_fr1930_cid51_cell
        neighbor_cell_1 = gsm_band1900_ch640_fr1955_cid56_cell
        neighbor_cell_2 = gsm_band1900_ch750_fr1977_cid57_cell
        serving_cell['power'] = -20
        neighbor_cell_1['power'] = -24
        neighbor_cell_2['power'] = -22

        expected_cell_info_list = [
            # Serving Cell
            {
                self.CID: serving_cell[self.CID],
                self.REPORT_RATE: 1,
                self.IS_REGISTERED: True,
                self.RAT: 'gsm',
                self.TARGET_RSSI:
                self._GSM_RSSI_OFFSET + serving_cell['power'],
                self.MAX_ERROR_RSSI: 3,
                self.MAX_ERROR_AVERAGE_RSSI: 3
            },
            # Neighbor Cells
            {
                self.CID: neighbor_cell_1[self.CID],
                self.REPORT_RATE: 0.1,
                self.IS_REGISTERED: False,
                self.RAT: 'gsm',
                self.TARGET_RSSI:
                self._GSM_RSSI_OFFSET + neighbor_cell_1['power'],
                self.MAX_ERROR_RSSI: 4,
                self.MAX_ERROR_AVERAGE_RSSI: 3
            },
            {
                self.CID: neighbor_cell_2[self.CID],
                self.REPORT_RATE: 0.1,
                self.IS_REGISTERED: False,
                self.RAT: 'gsm',
                self.TARGET_RSSI:
                self._GSM_RSSI_OFFSET + neighbor_cell_2['power'],
                self.MAX_ERROR_RSSI: 4,
                self.MAX_ERROR_AVERAGE_RSSI: 3
            }
        ]
        expected_cell_info_stats = {}
        for sample in expected_cell_info_list:
            expected_cell_info_stats[self._unique_cell_id(sample)] = sample

        self.md8475a.load_cell_paramfile(self._CELL_PARAM_FILE)
        [bts1] = set_system_model_gsm(self.md8475a, self.user_params,
                                      self.ad.sim_card)
        self._setup_gsm_cell_md8475a(bts1, serving_cell, serving_cell['power'])
        bts1.neighbor_cell_mode = "USERDATA"
        bts1.set_neighbor_cell_type("GSM", 1, "CELLNAME")
        bts1.set_neighbor_cell_name("GSM", 1, "GSM_1900_C640_F1955_CID56")
        bts1.set_neighbor_cell_type("GSM", 2, "CELLNAME")
        bts1.set_neighbor_cell_name("GSM", 2, "GSM_1900_C750_F1977_CID57")
        set_usim_parameters(self.md8475a, self.ad.sim_card)
        self.md8475a.start_simulation()

        self.ad.droid.telephonyToggleDataConnection(False)
        if not ensure_network_rat(
                self.log,
                self.ad,
                NETWORK_MODE_GSM_ONLY,
                RAT_FAMILY_GSM,
                toggle_apm_after_setting=True):
            self.log.error("Failed to set rat family {}, preferred network:{}".
                           format(RAT_FAMILY_GSM, NETWORK_MODE_GSM_ONLY))
            return False
        self.md8475a.wait_for_registration_state()
        self.setup_3710a_waveform("1", "A", "1955.8MHz",
                                  neighbor_cell_1['power'], "GSM",
                                  "gsm_lac56_cid56")

        self.setup_3710a_waveform("2", "A", "1977.8MHz",
                                  neighbor_cell_2['power'], "GSM",
                                  "gsm_lac57_cid57")
        self.turn_on_3710a_sg(1)
        self.turn_on_3710a_sg(2)
        time.sleep(self._SETTLING_TIME)
        return self._verify_cell_info(self.ad, expected_cell_info_stats)

    @test_tracker_info(uuid="b195153f-f6a0-4ec4-bb53-29c30ec0a034")
    @TelephonyBaseTest.tel_test_wrap
    def test_neighbor_cell_reporting_gsm_interband_2_tmo(self):
        """ Test Number of neighbor cells reported by Phone when two neighbor
        cells(inter band) are present (Phone camped on GSM)

        Setup one GSM cell configuration on MD8475A
        Setup two GSM waveforms on MG3710A
        Make Sure Phone camped on GSM
        Verify the number of neighbor cells reported by Phone

        Returns:
            True if pass; False if fail
        """
        serving_cell = gsm_band1900_ch512_fr1930_cid51_cell
        neighbor_cell_1 = gsm_band850_ch128_fr869_cid58_cell
        neighbor_cell_2 = gsm_band850_ch251_fr893_cid59_cell
        serving_cell['power'] = -20
        neighbor_cell_1['power'] = -24
        neighbor_cell_2['power'] = -22

        expected_cell_info_list = [
            # Serving Cell
            {
                self.CID: serving_cell[self.CID],
                self.REPORT_RATE: 1,
                self.IS_REGISTERED: True,
                self.RAT: 'gsm',
                self.TARGET_RSSI:
                self._GSM_RSSI_OFFSET + serving_cell['power'],
                self.MAX_ERROR_RSSI: 3,
                self.MAX_ERROR_AVERAGE_RSSI: 3
            },
            # Neighbor Cells
            {
                self.CID: neighbor_cell_1[self.CID],
                self.REPORT_RATE: 0.1,
                self.IS_REGISTERED: False,
                self.RAT: 'gsm',
                self.TARGET_RSSI:
                self._GSM_RSSI_OFFSET + neighbor_cell_1['power'],
                self.MAX_ERROR_RSSI: 4,
                self.MAX_ERROR_AVERAGE_RSSI: 3
            },
            {
                self.CID: neighbor_cell_2[self.CID],
                self.REPORT_RATE: 0.1,
                self.IS_REGISTERED: False,
                self.RAT: 'gsm',
                self.TARGET_RSSI:
                self._GSM_RSSI_OFFSET + neighbor_cell_2['power'],
                self.MAX_ERROR_RSSI: 4,
                self.MAX_ERROR_AVERAGE_RSSI: 3
            }
        ]
        expected_cell_info_stats = {}
        for sample in expected_cell_info_list:
            expected_cell_info_stats[self._unique_cell_id(sample)] = sample

        self.md8475a.load_cell_paramfile(self._CELL_PARAM_FILE)
        [bts1] = set_system_model_gsm(self.md8475a, self.user_params,
                                      self.ad.sim_card)
        self._setup_gsm_cell_md8475a(bts1, serving_cell, serving_cell['power'])
        bts1.neighbor_cell_mode = "USERDATA"
        bts1.set_neighbor_cell_type("GSM", 1, "CELLNAME")
        bts1.set_neighbor_cell_name("GSM", 1, "GSM_850_C128_F869_CID58")
        bts1.set_neighbor_cell_type("GSM", 2, "CELLNAME")
        bts1.set_neighbor_cell_name("GSM", 2, "GSM_850_C251_F893_CID59")
        set_usim_parameters(self.md8475a, self.ad.sim_card)
        self.md8475a.start_simulation()

        self.ad.droid.telephonyToggleDataConnection(False)
        if not ensure_network_rat(
                self.log,
                self.ad,
                NETWORK_MODE_GSM_ONLY,
                RAT_FAMILY_GSM,
                toggle_apm_after_setting=True):
            self.log.error("Failed to set rat family {}, preferred network:{}".
                           format(RAT_FAMILY_GSM, NETWORK_MODE_GSM_ONLY))
            return False
        self.md8475a.wait_for_registration_state()
        self.setup_3710a_waveform("1", "A", "869MHz", neighbor_cell_1['power'],
                                  "GSM", "gsm_lac58_cid58")

        self.setup_3710a_waveform("2", "A", "893MHz", neighbor_cell_2['power'],
                                  "GSM", "gsm_lac59_cid59")
        self.turn_on_3710a_sg(1)
        self.turn_on_3710a_sg(2)
        time.sleep(self._SETTLING_TIME)
        return self._verify_cell_info(self.ad, expected_cell_info_stats)

    @test_tracker_info(uuid="209f62c1-7950-447c-9101-abe930da20ba")
    @TelephonyBaseTest.tel_test_wrap
    def test_neighbor_cell_reporting_gsm_interrat_2_tmo(self):
        """ Test Number of neighbor cells reported by Phone when no neighbor
        cells(inter RAT) are present (Phone camped on GSM)

        Setup one GSM cell configuration on MD8475A
        Setup one LTE and one GSM waveforms on MG3710A
        Make Sure Phone camped on GSM
        Verify the number of neighbor cells reported by Phone

        Returns:
            True if pass; False if fail
        """
        serving_cell = gsm_band1900_ch512_fr1930_cid51_cell
        neighbor_cell_1 = lte_band4_ch2000_fr2115_pcid1_cell
        neighbor_cell_2 = wcdma_band1_ch10700_fr2140_cid31_cell
        serving_cell['power'] = -20
        neighbor_cell_1['power'] = -24
        neighbor_cell_2['power'] = -22

        expected_cell_info_list = [
            # Serving Cell
            {
                self.CID: serving_cell[self.CID],
                self.REPORT_RATE: 1,
                self.IS_REGISTERED: True,
                self.RAT: 'gsm',
                self.TARGET_RSSI:
                self._GSM_RSSI_OFFSET + serving_cell['power'],
                self.MAX_ERROR_RSSI: 3,
                self.MAX_ERROR_AVERAGE_RSSI: 3
            },
            # Neighbor Cells
            {
                self.PCID: neighbor_cell_1[self.PCID],
                self.REPORT_RATE: 0.1,
                self.IS_REGISTERED: False,
                self.RAT: 'lte',
                self.TARGET_RSSI:
                self._LTE_RSSI_OFFSET + neighbor_cell_1['power'],
                self.MAX_ERROR_RSSI: 4,
                self.MAX_ERROR_AVERAGE_RSSI: 3
            },
            {
                self.PSC: neighbor_cell_2[self.PSC],
                self.REPORT_RATE: 0.1,
                self.IS_REGISTERED: False,
                self.RAT: 'wcdma',
                self.TARGET_RSSI:
                self._WCDMA_RSSI_OFFSET + neighbor_cell_2['power'],
                self.MAX_ERROR_RSSI: 4,
                self.MAX_ERROR_AVERAGE_RSSI: 3
            }
        ]
        expected_cell_info_stats = {}
        for sample in expected_cell_info_list:
            expected_cell_info_stats[self._unique_cell_id(sample)] = sample

        self.md8475a.load_cell_paramfile(self._CELL_PARAM_FILE)
        [bts1] = set_system_model_gsm(self.md8475a, self.user_params,
                                      self.ad.sim_card)
        self._setup_gsm_cell_md8475a(bts1, serving_cell, serving_cell['power'])
        bts1.neighbor_cell_mode = "USERDATA"
        bts1.set_neighbor_cell_type("LTE", 1, "CELLNAME")
        bts1.set_neighbor_cell_name("LTE", 1, "LTE_4_C2000_F2115_PCID1")
        bts1.set_neighbor_cell_type("WCDMA", 2, "CELLNAME")
        bts1.set_neighbor_cell_name("WCDMA", 2, "WCDM_1_C10700_F2140_CID31")
        set_usim_parameters(self.md8475a, self.ad.sim_card)
        self.md8475a.start_simulation()
        self.ad.droid.telephonyToggleDataConnection(False)
        if not ensure_network_rat(
                self.log,
                self.ad,
                NETWORK_MODE_GSM_ONLY,
                RAT_FAMILY_GSM,
                toggle_apm_after_setting=True):
            self.log.error("Failed to set rat family {}, preferred network:{}".
                           format(RAT_FAMILY_GSM, NETWORK_MODE_GSM_ONLY))
            return False
        self.md8475a.wait_for_registration_state()
        self.setup_3710a_waveform("1", "A", "2115MHz",
                                  neighbor_cell_1['power'], "LTE",
                                  "lte_1_ch2000_pcid1")

        self.setup_3710a_waveform("2", "A", "2140MHz",
                                  neighbor_cell_2['power'], "WCDMA",
                                  "wcdma_1_psc31_cid31")
        self.turn_on_3710a_sg(1)
        self.turn_on_3710a_sg(2)
        time.sleep(self._SETTLING_TIME)
        return self._verify_cell_info(self.ad, expected_cell_info_stats)

    """ Tests End """
