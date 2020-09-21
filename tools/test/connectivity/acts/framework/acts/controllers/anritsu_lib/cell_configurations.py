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
"""
Sanity tests for voice tests in telephony
"""
from acts.controllers.anritsu_lib.md8475a import BtsBandwidth
from acts.test_utils.tel.anritsu_utils import GSM_BAND_PCS1900
from acts.test_utils.tel.anritsu_utils import GSM_BAND_GSM850
from acts.test_utils.tel.anritsu_utils import LTE_BAND_2
from acts.test_utils.tel.anritsu_utils import LTE_BAND_4
from acts.test_utils.tel.anritsu_utils import LTE_BAND_12
from acts.test_utils.tel.anritsu_utils import WCDMA_BAND_1
from acts.test_utils.tel.anritsu_utils import WCDMA_BAND_2

# Different Cell configurations
# TMO bands
lte_band4_ch2000_fr2115_pcid1_cell = {
    'band': LTE_BAND_4,
    'bandwidth': BtsBandwidth.LTE_BANDWIDTH_10MHz,
    'mcc': '001',
    'mnc': '01',
    'tac': 11,
    'cid': 1,
    'pcid': 1,
    'channel': 2000
}

lte_band4_ch2000_fr2115_pcid2_cell = {
    'band': LTE_BAND_4,
    'bandwidth': BtsBandwidth.LTE_BANDWIDTH_10MHz,
    'mcc': '001',
    'mnc': '01',
    'tac': 12,
    'cid': 2,
    'pcid': 2,
    'channel': 2000
}

lte_band4_ch2000_fr2115_pcid3_cell = {
    'band': LTE_BAND_4,
    'bandwidth': BtsBandwidth.LTE_BANDWIDTH_10MHz,
    'mcc': '001',
    'mnc': '01',
    'tac': 13,
    'cid': 3,
    'pcid': 3,
    'channel': 2000
}

lte_band4_ch2000_fr2115_pcid4_cell = {
    'band': LTE_BAND_4,
    'bandwidth': BtsBandwidth.LTE_BANDWIDTH_10MHz,
    'mcc': '001',
    'mnc': '01',
    'tac': 14,
    'cid': 4,
    'pcid': 4,
    'channel': 2000
}

lte_band4_ch2000_fr2115_pcid5_cell = {
    'band': LTE_BAND_4,
    'bandwidth': BtsBandwidth.LTE_BANDWIDTH_10MHz,
    'mcc': '001',
    'mnc': '01',
    'tac': 15,
    'cid': 5,
    'pcid': 5,
    'channel': 2000
}

lte_band4_ch2000_fr2115_pcid6_cell = {
    'band': LTE_BAND_4,
    'bandwidth': BtsBandwidth.LTE_BANDWIDTH_10MHz,
    'mcc': '001',
    'mnc': '01',
    'tac': 16,
    'cid': 6,
    'pcid': 6,
    'channel': 2000
}

lte_band4_ch2050_fr2120_pcid7_cell = {
    'band': LTE_BAND_4,
    'bandwidth': BtsBandwidth.LTE_BANDWIDTH_10MHz,
    'mcc': '001',
    'mnc': '01',
    'tac': 17,
    'cid': 7,
    'pcid': 7,
    'channel': 2050
}

lte_band4_ch2250_fr2140_pcid8_cell = {
    'band': LTE_BAND_4,
    'bandwidth': BtsBandwidth.LTE_BANDWIDTH_10MHz,
    'mcc': '001',
    'mnc': '01',
    'tac': 18,
    'cid': 8,
    'pcid': 8,
    'channel': 2250
}

lte_band2_ch900_fr1960_pcid9_cell = {
    'band': LTE_BAND_2,
    'bandwidth': BtsBandwidth.LTE_BANDWIDTH_10MHz,
    'mcc': '001',
    'mnc': '01',
    'tac': 19,
    'cid': 9,
    'pcid': 9,
    'channel': 900
}

lte_band12_ch5095_fr737_pcid10_cell = {
    'band': LTE_BAND_12,
    'bandwidth': BtsBandwidth.LTE_BANDWIDTH_10MHz,
    'mcc': '001',
    'mnc': '01',
    'tac': 20,
    'cid': 10,
    'pcid': 10,
    'channel': 5095
}

wcdma_band1_ch10700_fr2140_cid31_cell = {
    'band': WCDMA_BAND_1,
    'mcc': '001',
    'mnc': '01',
    'lac': 31,
    'rac': 31,
    'cid': 31,
    'channel': 10700,
    'psc': 31
}

wcdma_band1_ch10700_fr2140_cid32_cell = {
    'band': WCDMA_BAND_1,
    'mcc': '001',
    'mnc': '01',
    'lac': 32,
    'rac': 32,
    'cid': 32,
    'channel': 10700,
    'psc': 32
}

wcdma_band1_ch10700_fr2140_cid33_cell = {
    'band': WCDMA_BAND_1,
    'mcc': '001',
    'mnc': '01',
    'lac': 33,
    'rac': 33,
    'cid': 33,
    'channel': 10700,
    'psc': 33
}

wcdma_band1_ch10700_fr2140_cid34_cell = {
    'band': WCDMA_BAND_1,
    'mcc': '001',
    'mnc': '01',
    'lac': 34,
    'rac': 34,
    'cid': 34,
    'channel': 10700,
    'psc': 34
}

wcdma_band1_ch10700_fr2140_cid35_cell = {
    'band': WCDMA_BAND_1,
    'mcc': '001',
    'mnc': '01',
    'lac': 35,
    'rac': 35,
    'cid': 35,
    'channel': 10700,
    'psc': 35
}

wcdma_band1_ch10575_fr2115_cid36_cell = {
    'band': WCDMA_BAND_1,
    'mcc': '001',
    'mnc': '01',
    'lac': 36,
    'rac': 36,
    'cid': 36,
    'channel': 10575,
    'psc': 36
}

wcdma_band1_ch10800_fr2160_cid37_cell = {
    'band': WCDMA_BAND_1,
    'mcc': '001',
    'mnc': '01',
    'lac': 37,
    'rac': 37,
    'cid': 37,
    'channel': 10800,
    'psc': 37
}

wcdma_band2_ch9800_fr1960_cid38_cell = {
    'band': WCDMA_BAND_2,
    'mcc': '001',
    'mnc': '01',
    'lac': 38,
    'rac': 38,
    'cid': 38,
    'channel': 9800,
    'psc': 38
}

wcdma_band2_ch9900_fr1980_cid39_cell = {
    'band': WCDMA_BAND_2,
    'mcc': '001',
    'mnc': '01',
    'lac': 39,
    'rac': 39,
    'cid': 39,
    'channel': 9900,
    'psc': 39
}

gsm_band1900_ch512_fr1930_cid51_cell = {
    'band': GSM_BAND_PCS1900,
    'mcc': '001',
    'mnc': '01',
    'lac': 51,
    'rac': 51,
    'cid': 51,
    'channel': 512,
}

gsm_band1900_ch512_fr1930_cid52_cell = {
    'band': GSM_BAND_PCS1900,
    'mcc': '001',
    'mnc': '01',
    'lac': 52,
    'rac': 52,
    'cid': 52,
    'channel': 512,
}

gsm_band1900_ch512_fr1930_cid53_cell = {
    'band': GSM_BAND_PCS1900,
    'mcc': '001',
    'mnc': '01',
    'lac': 53,
    'rac': 53,
    'cid': 53,
    'channel': 512,
}

gsm_band1900_ch512_fr1930_cid54_cell = {
    'band': GSM_BAND_PCS1900,
    'mcc': '001',
    'mnc': '01',
    'lac': 54,
    'rac': 54,
    'cid': 54,
    'channel': 512,
}

gsm_band1900_ch512_fr1930_cid55_cell = {
    'band': GSM_BAND_PCS1900,
    'mcc': '001',
    'mnc': '01',
    'lac': 55,
    'rac': 55,
    'cid': 55,
    'channel': 512,
}

gsm_band1900_ch640_fr1955_cid56_cell = {
    'band': GSM_BAND_PCS1900,
    'mcc': '001',
    'mnc': '01',
    'lac': 56,
    'rac': 56,
    'cid': 56,
    'channel': 640,
}

gsm_band1900_ch750_fr1977_cid57_cell = {
    'band': GSM_BAND_PCS1900,
    'mcc': '001',
    'mnc': '01',
    'lac': 57,
    'rac': 57,
    'cid': 57,
    'channel': 750,
}

gsm_band850_ch128_fr869_cid58_cell = {
    'band': GSM_BAND_GSM850,
    'mcc': '001',
    'mnc': '01',
    'lac': 58,
    'rac': 58,
    'cid': 58,
    'channel': 128,
}

gsm_band850_ch251_fr893_cid59_cell = {
    'band': GSM_BAND_GSM850,
    'mcc': '001',
    'mnc': '01',
    'lac': 59,
    'rac': 59,
    'cid': 59,
    'channel': 251,
}
