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

import time
from queue import Empty
from datetime import datetime

from acts.controllers.anritsu_lib._anritsu_utils import AnritsuUtils
from acts.controllers.anritsu_lib.md8475a import BtsNumber
from acts.controllers.anritsu_lib.md8475a import BtsNwNameEnable
from acts.controllers.anritsu_lib.md8475a import BtsServiceState
from acts.controllers.anritsu_lib.md8475a import BtsTechnology
from acts.controllers.anritsu_lib.md8475a import CsfbType
from acts.controllers.anritsu_lib.md8475a import ImsCscfCall
from acts.controllers.anritsu_lib.md8475a import ImsCscfStatus
from acts.controllers.anritsu_lib.md8475a import MD8475A
from acts.controllers.anritsu_lib.md8475a import ReturnToEUTRAN
from acts.controllers.anritsu_lib.md8475a import VirtualPhoneStatus
from acts.controllers.anritsu_lib.md8475a import TestProcedure
from acts.controllers.anritsu_lib.md8475a import TestPowerControl
from acts.controllers.anritsu_lib.md8475a import TestMeasurement
from acts.controllers.anritsu_lib.md8475a import Switch
from acts.controllers.anritsu_lib.md8475a import BtsPacketRate
from acts.test_utils.tel.tel_defines import CALL_TEARDOWN_PHONE
from acts.test_utils.tel.tel_defines import CALL_TEARDOWN_REMOTE
from acts.test_utils.tel.tel_defines import MAX_WAIT_TIME_CALL_DROP
from acts.test_utils.tel.tel_defines import RAT_1XRTT
from acts.test_utils.tel.tel_defines import WAIT_TIME_IN_CALL
from acts.test_utils.tel.tel_defines import WAIT_TIME_IN_CALL_FOR_IMS
from acts.test_utils.tel.tel_defines import EventCmasReceived
from acts.test_utils.tel.tel_defines import EventEtwsReceived
from acts.test_utils.tel.tel_defines import EventSmsDeliverSuccess
from acts.test_utils.tel.tel_defines import EventSmsSentSuccess
from acts.test_utils.tel.tel_defines import EventSmsReceived
from acts.test_utils.tel.tel_test_utils import ensure_phone_idle
from acts.test_utils.tel.tel_test_utils import hangup_call
from acts.test_utils.tel.tel_test_utils import initiate_call
from acts.test_utils.tel.tel_test_utils import wait_and_answer_call
from acts.test_utils.tel.tel_test_utils import wait_for_droid_not_in_call

# Timers
# Time to wait after registration before sending a command to Anritsu
# to ensure the phone has sufficient time to reconfigure based on new
# network in Anritsu
WAIT_TIME_ANRITSU_REG_AND_OPER = 10
# Time to wait after registration to ensure the phone
# has sufficient time to reconfigure based on new network in Anritsu
WAIT_TIME_ANRITSU_REG_AND_CALL = 10
# Max time to wait for Anritsu's virtual phone state change
MAX_WAIT_TIME_VIRTUAL_PHONE_STATE = 45
# Time to wait for Anritsu's IMS CSCF state change
MAX_WAIT_TIME_IMS_CSCF_STATE = 30
# Time to wait for before aSRVCC
WAIT_TIME_IN_ALERT = 5

# SIM card names
P0250Ax = "P0250Ax"
VzW12349 = "VzW12349"
P0135Ax = "P0135Ax"

# Test PLMN information
TEST_PLMN_LTE_NAME = "MD8475A_LTE"
TEST_PLMN_WCDMA_NAME = "MD8475A_WCDMA"
TEST_PLMN_GSM_NAME = "MD8475A_GSM"
TEST_PLMN_1X_NAME = "MD8475A_1X"
TEST_PLMN_1_MCC = "001"
TEST_PLMN_1_MNC = "01"
DEFAULT_MCC = "310"
DEFAULT_MNC = "260"
DEFAULT_RAC = 1
DEFAULT_LAC = 1
VzW_MCC = "311"
VzW_MNC = "480"
TMO_MCC = "310"
TMO_MNC = "260"

# IP address information for internet sharing
#GATEWAY_IPV4_ADDR = "192.168.137.1"
#UE_IPV4_ADDR_1 = "192.168.137.2"
#UE_IPV4_ADDR_2 = "192.168.137.3"
#UE_IPV4_ADDR_3 = "192.168.137.4"
#DNS_IPV4_ADDR = "192.168.137.1"
#CSCF_IPV4_ADDR = "192.168.137.1"

# Default IP address in Smart Studio, work for Internet Sharing with and
# without WLAN ePDG server. Remember to add 192.168.1.2 to Ethernet 0
# on MD8475A after turn on Windows' Internet Coonection Sharing
GATEWAY_IPV4_ADDR = "192.168.1.2"
UE_IPV4_ADDR_1 = "192.168.1.1"
UE_IPV4_ADDR_2 = "192.168.1.11"
UE_IPV4_ADDR_3 = "192.168.1.21"
UE_IPV6_ADDR_1 = "2001:0:0:1::1"
UE_IPV6_ADDR_2 = "2001:0:0:2::1"
UE_IPV6_ADDR_3 = "2001:0:0:3::1"
DNS_IPV4_ADDR = "192.168.1.12"
CSCF_IPV4_ADDR = "192.168.1.2"
CSCF_IPV6_ADDR = "2001:0:0:1::2"
CSCF_IPV6_ADDR_2 = "2001:0:0:2::2"
CSCF_IPV6_ADDR_3 = "2001:0:0:3::2"

# GSM BAND constants
GSM_BAND_GSM450 = "GSM450"
GSM_BAND_GSM480 = "GSM480"
GSM_BAND_GSM850 = "GSM850"
GSM_BAND_PGSM900 = "P-GSM900"
GSM_BAND_EGSM900 = "E-GSM900"
GSM_BAND_RGSM900 = "R-GSM900"
GSM_BAND_DCS1800 = "DCS1800"
GSM_BAND_PCS1900 = "PCS1900"

LTE_BAND_2 = 2
LTE_BAND_4 = 4
LTE_BAND_12 = 12
WCDMA_BAND_1 = 1
WCDMA_BAND_2 = 2

# Default Cell Parameters
DEFAULT_OUTPUT_LEVEL = -20
DEFAULT_1X_OUTPUT_LEVEL = -35
DEFAULT_INPUT_LEVEL = 0
DEFAULT_LTE_BAND = [2, 4]
DEFAULT_WCDMA_BAND = 1
DEFAULT_WCDMA_PACKET_RATE = BtsPacketRate.WCDMA_DLHSAUTO_REL7_ULHSAUTO
DEFAULT_GSM_BAND = GSM_BAND_GSM850
DEFAULT_CDMA1X_BAND = 0
DEFAULT_CDMA1X_CH = 356
DEFAULT_CDMA1X_SID = 0
DEFAULT_CDMA1X_NID = 65535
DEFAULT_EVDO_BAND = 0
DEFAULT_EVDO_CH = 356
DEFAULT_EVDO_SECTOR_ID = "00000000,00000000,00000000,00000000"
VzW_CDMA1x_BAND = 1
VzW_CDMA1x_CH = 150
VzW_CDMA1X_SID = 26
VzW_CDMA1X_NID = 65535
VzW_EVDO_BAND = 0
VzW_EVDO_CH = 384
VzW_EVDO_SECTOR_ID = "12345678,00000000,00000000,00000000"
DEFAULT_T_MODE = "TM1"
DEFAULT_DL_ANTENNA = 1

# CMAS Message IDs
CMAS_MESSAGE_PRESIDENTIAL_ALERT = hex(0x1112)
CMAS_MESSAGE_EXTREME_IMMEDIATE_OBSERVED = hex(0x1113)
CMAS_MESSAGE_EXTREME_IMMEDIATE_LIKELY = hex(0x1114)
CMAS_MESSAGE_EXTREME_EXPECTED_OBSERVED = hex(0x1115)
CMAS_MESSAGE_EXTREME_EXPECTED_LIKELY = hex(0x1116)
CMAS_MESSAGE_SEVERE_IMMEDIATE_OBSERVED = hex(0x1117)
CMAS_MESSAGE_SEVERE_IMMEDIATE_LIKELY = hex(0x1118)
CMAS_MESSAGE_SEVERE_EXPECTED_OBSERVED = hex(0x1119)
CMAS_MESSAGE_SEVERE_EXPECTED_LIKELY = hex(0x111A)
CMAS_MESSAGE_CHILD_ABDUCTION_EMERGENCY = hex(0x111B)
CMAS_MESSAGE_MONTHLY_TEST = hex(0x111C)
CMAS_MESSAGE_CMAS_EXECERCISE = hex(0x111D)

# ETWS Message IDs
ETWS_WARNING_EARTHQUAKE = hex(0x1100)
ETWS_WARNING_TSUNAMI = hex(0x1101)
ETWS_WARNING_EARTHQUAKETSUNAMI = hex(0x1102)
ETWS_WARNING_TEST_MESSAGE = hex(0x1103)
ETWS_WARNING_OTHER_EMERGENCY = hex(0x1104)

# C2K CMAS Message Constants
CMAS_C2K_CATEGORY_PRESIDENTIAL = "Presidential"
CMAS_C2K_CATEGORY_EXTREME = "Extreme"
CMAS_C2K_CATEGORY_SEVERE = "Severe"
CMAS_C2K_CATEGORY_AMBER = "AMBER"
CMAS_C2K_CATEGORY_CMASTEST = "CMASTest"

CMAS_C2K_PRIORITY_NORMAL = "Normal"
CMAS_C2K_PRIORITY_INTERACTIVE = "Interactive"
CMAS_C2K_PRIORITY_URGENT = "Urgent"
CMAS_C2K_PRIORITY_EMERGENCY = "Emergency"

CMAS_C2K_RESPONSETYPE_SHELTER = "Shelter"
CMAS_C2K_RESPONSETYPE_EVACUATE = "Evacuate"
CMAS_C2K_RESPONSETYPE_PREPARE = "Prepare"
CMAS_C2K_RESPONSETYPE_EXECUTE = "Execute"
CMAS_C2K_RESPONSETYPE_MONITOR = "Monitor"
CMAS_C2K_RESPONSETYPE_AVOID = "Avoid"
CMAS_C2K_RESPONSETYPE_ASSESS = "Assess"
CMAS_C2K_RESPONSETYPE_NONE = "None"

CMAS_C2K_SEVERITY_EXTREME = "Extreme"
CMAS_C2K_SEVERITY_SEVERE = "Severe"

CMAS_C2K_URGENCY_IMMEDIATE = "Immediate"
CMAS_C2K_URGENCY_EXPECTED = "Expected"

CMAS_C2K_CERTIANTY_OBSERVED = "Observed"
CMAS_C2K_CERTIANTY_LIKELY = "Likely"

#PDN Numbers
PDN_NO_1 = 1
PDN_NO_2 = 2
PDN_NO_3 = 3

# IMS Services parameters
DEFAULT_VNID = 1
NDP_NIC_NAME = '"Intel(R) 82577LM Gigabit Network Connection"'
CSCF_Monitoring_UA_URI = '"sip:+11234567890@test.3gpp.com"'
TMO_CSCF_Monitoring_UA_URI = '"sip:001010123456789@msg.lab.t-mobile.com"'
CSCF_Virtual_UA_URI = '"sip:+11234567891@test.3gpp.com"'
TMO_CSCF_Virtual_UA_URI = '"sip:0123456789@ims.mnc01.mcc001.3gppnetwork.org"'
CSCF_HOSTNAME = '"ims.mnc01.mcc001.3gppnetwork.org"'
TMO_USERLIST_NAME = "310260123456789@msg.lab.t-mobile.com"
VZW_USERLIST_NAME = "001010123456789@test.3gpp.com"

#Cell Numbers
CELL_1 = 1
CELL_2 = 2

# default ims virtual network id for Anritsu ims call test.
DEFAULT_IMS_VIRTUAL_NETWORK_ID = 1


def cb_serial_number():
    """ CMAS/ETWS serial number generator """
    i = 0x3000
    while True:
        yield i
        i += 1


def set_usim_parameters(anritsu_handle, sim_card):
    """ set USIM parameters in MD8475A simulationn parameter

    Args:
        anritsu_handle: anritusu device object.
        sim_card : "P0250Ax" or "12349"

    Returns:
        None
    """
    if sim_card == P0250Ax:
        anritsu_handle.usim_key = "000102030405060708090A0B0C0D0E0F"
    elif sim_card == P0135Ax:
        anritsu_handle.usim_key = "00112233445566778899AABBCCDDEEFF"
    elif sim_card == VzW12349:
        anritsu_handle.usim_key = "465B5CE8B199B49FAA5F0A2EE238A6BC"
        anritsu_handle.send_command("IMSI 311480012345678")
        anritsu_handle.send_command("SECURITY3G MILENAGE")
        anritsu_handle.send_command(
            "MILENAGEOP 5F1D289C5D354D0A140C2548F5F3E3BA")


def save_anritsu_log_files(anritsu_handle, test_name, user_params):
    """ saves the anritsu smart studio log files
        The logs should be saved in Anritsu system. Need to provide
        log folder path in Anritsu system

    Args:
        anritsu_handle: anritusu device object.
        test_name: test case name
        user_params : user supplied parameters list

    Returns:
        None
    """
    md8475a_log_folder = user_params["anritsu_log_file_path"]
    file_name = getfilenamewithtimestamp(test_name)
    seq_logfile = "{}\\{}_seq.csv".format(md8475a_log_folder, file_name)
    msg_logfile = "{}\\{}_msg.csv".format(md8475a_log_folder, file_name)
    trace_logfile = "{}\\{}_trace.lgex".format(md8475a_log_folder, file_name)
    anritsu_handle.save_sequence_log(seq_logfile)
    anritsu_handle.save_message_log(msg_logfile)
    anritsu_handle.save_trace_log(trace_logfile, "BINARY", 1, 0, 0)
    anritsu_handle.clear_sequence_log()
    anritsu_handle.clear_message_log()


def getfilenamewithtimestamp(test_name):
    """ Gets the test name appended with current time

    Args:
        test_name : test case name

    Returns:
        string of test name appended with current time
    """
    time_stamp = datetime.now().strftime("%m-%d-%Y_%H-%M-%S")
    return "{}_{}".format(test_name, time_stamp)


def _init_lte_bts(bts, user_params, cell_no, sim_card):
    """ initializes the LTE BTS
        All BTS parameters should be set here

    Args:
        bts: BTS object.
        user_params: pointer to user supplied parameters
        cell_no: specify the cell number this BTS is configured
        Anritsu supports two cells. so cell_1 or cell_2

    Returns:
        None
    """
    
    bts.nw_fullname_enable = BtsNwNameEnable.NAME_ENABLE
    bts.nw_fullname = TEST_PLMN_LTE_NAME
    bts.mcc = get_lte_mcc(user_params, cell_no, sim_card)
    bts.mnc = get_lte_mnc(user_params, cell_no, sim_card)
    bts.band = get_lte_band(user_params, cell_no)
    bts.transmode = get_transmission_mode(user_params, cell_no)
    bts.dl_antenna = get_dl_antenna(user_params, cell_no)
    bts.output_level = DEFAULT_OUTPUT_LEVEL
    bts.input_level = DEFAULT_INPUT_LEVEL


def _init_wcdma_bts(bts, user_params, cell_no, sim_card):
    """ initializes the WCDMA BTS
        All BTS parameters should be set here

    Args:
        bts: BTS object.
        user_params: pointer to user supplied parameters
        cell_no: specify the cell number this BTS is configured
        Anritsu supports two cells. so cell_1 or cell_2

    Returns:
        None
    """
    bts.nw_fullname_enable = BtsNwNameEnable.NAME_ENABLE
    bts.nw_fullname = TEST_PLMN_WCDMA_NAME
    bts.mcc = get_wcdma_mcc(user_params, cell_no, sim_card)
    bts.mnc = get_wcdma_mnc(user_params, cell_no, sim_card)
    bts.band = get_wcdma_band(user_params, cell_no)
    bts.rac = get_wcdma_rac(user_params, cell_no)
    bts.lac = get_wcdma_lac(user_params, cell_no)
    bts.output_level = DEFAULT_OUTPUT_LEVEL
    bts.input_level = DEFAULT_INPUT_LEVEL
    bts.packet_rate = DEFAULT_WCDMA_PACKET_RATE


def _init_gsm_bts(bts, user_params, cell_no, sim_card):
    """ initializes the GSM BTS
        All BTS parameters should be set here

    Args:
        bts: BTS object.
        user_params: pointer to user supplied parameters
        cell_no: specify the cell number this BTS is configured
        Anritsu supports two cells. so cell_1 or cell_2

    Returns:
        None
    """
    bts.nw_fullname_enable = BtsNwNameEnable.NAME_ENABLE
    bts.nw_fullname = TEST_PLMN_GSM_NAME
    bts.mcc = get_gsm_mcc(user_params, cell_no, sim_card)
    bts.mnc = get_gsm_mnc(user_params, cell_no, sim_card)
    bts.band = get_gsm_band(user_params, cell_no)
    bts.rac = get_gsm_rac(user_params, cell_no)
    bts.lac = get_gsm_lac(user_params, cell_no)
    bts.output_level = DEFAULT_OUTPUT_LEVEL
    bts.input_level = DEFAULT_INPUT_LEVEL


def _init_1x_bts(bts, user_params, cell_no, sim_card):
    """ initializes the 1X BTS
        All BTS parameters should be set here

    Args:
        bts: BTS object.
        user_params: pointer to user supplied parameters
        cell_no: specify the cell number this BTS is configured
        Anritsu supports two cells. so cell_1 or cell_2

    Returns:
        None
    """
    bts.sector1_mcc = get_1x_mcc(user_params, cell_no, sim_card)
    bts.band = get_1x_band(user_params, cell_no, sim_card)
    bts.dl_channel = get_1x_channel(user_params, cell_no, sim_card)
    bts.sector1_sid = get_1x_sid(user_params, cell_no, sim_card)
    bts.sector1_nid = get_1x_nid(user_params, cell_no, sim_card)
    bts.output_level = DEFAULT_1X_OUTPUT_LEVEL


def _init_evdo_bts(bts, user_params, cell_no, sim_card):
    """ initializes the EVDO BTS
        All BTS parameters should be set here

    Args:
        bts: BTS object.
        user_params: pointer to user supplied parameters
        cell_no: specify the cell number this BTS is configured
        Anritsu supports two cells. so cell_1 or cell_2

    Returns:
        None
    """
    bts.band = get_evdo_band(user_params, cell_no, sim_card)
    bts.dl_channel = get_evdo_channel(user_params, cell_no, sim_card)
    bts.evdo_sid = get_evdo_sid(user_params, cell_no, sim_card)
    bts.output_level = DEFAULT_1X_OUTPUT_LEVEL


def _init_PDN(anritsu_handle,
              pdn,
              ipv4,
              ipv6,
              ims_binding,
              vnid_number=DEFAULT_VNID):
    """ initializes the PDN parameters
        All PDN parameters should be set here

    Args:
        anritsu_handle: anritusu device object.
        pdn: pdn object
        ip_address : UE IP address
        ims_binding: to bind with IMS VNID(1) or not

    Returns:
        None
    """
    # Setting IP address for internet connection sharing
    anritsu_handle.gateway_ipv4addr = GATEWAY_IPV4_ADDR
    pdn.ue_address_ipv4 = ipv4
    pdn.ue_address_ipv6 = ipv6
    if ims_binding:
        pdn.pdn_ims = Switch.ENABLE
        pdn.pdn_vnid = vnid_number
    else:
        pdn.primary_dns_address_ipv4 = DNS_IPV4_ADDR
        pdn.secondary_dns_address_ipv4 = DNS_IPV4_ADDR
        pdn.cscf_address_ipv4 = CSCF_IPV4_ADDR


def _init_IMS(anritsu_handle,
              vnid,
              sim_card=None,
              ipv6_address=CSCF_IPV6_ADDR,
              ip_type="IPV4V6",
              auth=False):
    """ initializes the IMS VNID parameters
        All IMS parameters should be set here

    Args:
        anritsu_handle: anritusu device object.
        vnid: IMS Services object

    Returns:
        None
    """
    # vnid.sync = Switch.ENABLE # supported in 6.40a release
    vnid.cscf_address_ipv4 = CSCF_IPV4_ADDR
    vnid.cscf_address_ipv6 = ipv6_address
    vnid.imscscf_iptype = ip_type
    vnid.dns = Switch.DISABLE
    vnid.ndp_nic = NDP_NIC_NAME
    vnid.ndp_prefix = ipv6_address
    if sim_card == P0135Ax:
        vnid.cscf_monitoring_ua = TMO_CSCF_Monitoring_UA_URI
        vnid.cscf_virtual_ua = TMO_CSCF_Virtual_UA_URI
        vnid.cscf_host_name = CSCF_HOSTNAME
        vnid.cscf_ims_authentication = "DISABLE"
        if auth:
            vnid.cscf_ims_authentication = "ENABLE"
            vnid.tmo_cscf_userslist_add = TMO_USERLIST_NAME
    elif sim_card == VzW12349:
        vnid.cscf_monitoring_ua = CSCF_Monitoring_UA_URI
        vnid.cscf_virtual_ua = CSCF_Virtual_UA_URI
        vnid.cscf_ims_authentication = "DISABLE"
        if auth:
            vnid.cscf_ims_authentication = "ENABLE"
            vnid.vzw_cscf_userslist_add = VZW_USERLIST_NAME
    else:
        vnid.cscf_monitoring_ua = CSCF_Monitoring_UA_URI
    vnid.psap = Switch.ENABLE
    vnid.psap_auto_answer = Switch.ENABLE


def set_system_model_lte_lte(anritsu_handle, user_params, sim_card):
    """ Configures Anritsu system for LTE and LTE simulation

    Args:
        anritsu_handle: anritusu device object.
        user_params: pointer to user supplied parameters

    Returns:
        Lte and Wcdma BTS objects
    """
    anritsu_handle.set_simulation_model(BtsTechnology.LTE, BtsTechnology.LTE)
    # setting BTS parameters
    lte1_bts = anritsu_handle.get_BTS(BtsNumber.BTS1)
    lte2_bts = anritsu_handle.get_BTS(BtsNumber.BTS2)
    _init_lte_bts(lte1_bts, user_params, CELL_1, sim_card)
    _init_lte_bts(lte2_bts, user_params, CELL_2, sim_card)
    pdn1 = anritsu_handle.get_PDN(PDN_NO_1)
    pdn2 = anritsu_handle.get_PDN(PDN_NO_2)
    pdn3 = anritsu_handle.get_PDN(PDN_NO_3)
    # Initialize PDN IP address for internet connection sharing
    _init_PDN(anritsu_handle, pdn1, UE_IPV4_ADDR_1, UE_IPV6_ADDR_1, True)
    _init_PDN(anritsu_handle, pdn2, UE_IPV4_ADDR_2, UE_IPV6_ADDR_2, False)
    _init_PDN(anritsu_handle, pdn3, UE_IPV4_ADDR_3, UE_IPV6_ADDR_3, True)
    vnid1 = anritsu_handle.get_IMS(DEFAULT_VNID)
    if sim_card == P0135Ax:
        vnid2 = anritsu_handle.get_IMS(2)
        vnid3 = anritsu_handle.get_IMS(3)
        _init_IMS(
            anritsu_handle,
            vnid1,
            sim_card,
            ipv6_address=CSCF_IPV6_ADDR,
            auth=True)
        _init_IMS(
            anritsu_handle,
            vnid2,
            sim_card,
            ipv6_address=CSCF_IPV6_ADDR_2,
            ip_type="IPV6")
        _init_IMS(
            anritsu_handle,
            vnid3,
            sim_card,
            ipv6_address=CSCF_IPV6_ADDR_3,
            ip_type="IPV6")
    elif sim_card == VzW12349:
        _init_IMS(anritsu_handle, vnid1, sim_card, auth=True)
    else:
        _init_IMS(anritsu_handle, vnid1, sim_card)
    return [lte1_bts, lte2_bts]


def set_system_model_wcdma_wcdma(anritsu_handle, user_params, sim_card):
    """ Configures Anritsu system for WCDMA and WCDMA simulation

    Args:
        anritsu_handle: anritusu device object.
        user_params: pointer to user supplied parameters

    Returns:
        Lte and Wcdma BTS objects
    """
    anritsu_handle.set_simulation_model(BtsTechnology.WCDMA,
                                        BtsTechnology.WCDMA)
    # setting BTS parameters
    wcdma1_bts = anritsu_handle.get_BTS(BtsNumber.BTS1)
    wcdma2_bts = anritsu_handle.get_BTS(BtsNumber.BTS2)
    _init_wcdma_bts(wcdma1_bts, user_params, CELL_1, sim_card)
    _init_wcdma_bts(wcdma2_bts, user_params, CELL_2, sim_card)
    pdn1 = anritsu_handle.get_PDN(PDN_NO_1)
    # Initialize PDN IP address for internet connection sharing
    _init_PDN(anritsu_handle, pdn1, UE_IPV4_ADDR_1, UE_IPV6_ADDR_1, False)
    return [wcdma1_bts, wcdma2_bts]


def set_system_model_lte_wcdma(anritsu_handle, user_params, sim_card):
    """ Configures Anritsu system for LTE and WCDMA simulation

    Args:
        anritsu_handle: anritusu device object.
        user_params: pointer to user supplied parameters

    Returns:
        Lte and Wcdma BTS objects
    """
    anritsu_handle.set_simulation_model(BtsTechnology.LTE, BtsTechnology.WCDMA)
    # setting BTS parameters
    lte_bts = anritsu_handle.get_BTS(BtsNumber.BTS1)
    wcdma_bts = anritsu_handle.get_BTS(BtsNumber.BTS2)
    _init_lte_bts(lte_bts, user_params, CELL_1, sim_card)
    _init_wcdma_bts(wcdma_bts, user_params, CELL_2, sim_card)
    pdn1 = anritsu_handle.get_PDN(PDN_NO_1)
    pdn2 = anritsu_handle.get_PDN(PDN_NO_2)
    pdn3 = anritsu_handle.get_PDN(PDN_NO_3)
    # Initialize PDN IP address for internet connection sharing
    _init_PDN(anritsu_handle, pdn1, UE_IPV4_ADDR_1, UE_IPV6_ADDR_1, True)
    _init_PDN(anritsu_handle, pdn2, UE_IPV4_ADDR_2, UE_IPV6_ADDR_2, False)
    _init_PDN(anritsu_handle, pdn3, UE_IPV4_ADDR_3, UE_IPV6_ADDR_3, True)
    vnid1 = anritsu_handle.get_IMS(DEFAULT_VNID)
    if sim_card == P0135Ax:
        vnid2 = anritsu_handle.get_IMS(2)
        vnid3 = anritsu_handle.get_IMS(3)
        _init_IMS(
            anritsu_handle,
            vnid1,
            sim_card,
            ipv6_address=CSCF_IPV6_ADDR,
            auth=True)
        _init_IMS(
            anritsu_handle,
            vnid2,
            sim_card,
            ipv6_address=CSCF_IPV6_ADDR_2,
            ip_type="IPV6")
        _init_IMS(
            anritsu_handle,
            vnid3,
            sim_card,
            ipv6_address=CSCF_IPV6_ADDR_3,
            ip_type="IPV6")
    elif sim_card == VzW12349:
        _init_IMS(anritsu_handle, vnid1, sim_card, auth=True)
    else:
        _init_IMS(anritsu_handle, vnid1, sim_card)
    return [lte_bts, wcdma_bts]


def set_system_model_lte_gsm(anritsu_handle, user_params, sim_card):
    """ Configures Anritsu system for LTE and GSM simulation

    Args:
        anritsu_handle: anritusu device object.
        user_params: pointer to user supplied parameters

    Returns:
        Lte and Wcdma BTS objects
    """
    anritsu_handle.set_simulation_model(BtsTechnology.LTE, BtsTechnology.GSM)
    # setting BTS parameters
    lte_bts = anritsu_handle.get_BTS(BtsNumber.BTS1)
    gsm_bts = anritsu_handle.get_BTS(BtsNumber.BTS2)
    _init_lte_bts(lte_bts, user_params, CELL_1, sim_card)
    _init_gsm_bts(gsm_bts, user_params, CELL_2, sim_card)
    pdn1 = anritsu_handle.get_PDN(PDN_NO_1)
    pdn2 = anritsu_handle.get_PDN(PDN_NO_2)
    pdn3 = anritsu_handle.get_PDN(PDN_NO_3)
    # Initialize PDN IP address for internet connection sharing
    _init_PDN(anritsu_handle, pdn1, UE_IPV4_ADDR_1, UE_IPV6_ADDR_1, True)
    _init_PDN(anritsu_handle, pdn2, UE_IPV4_ADDR_2, UE_IPV6_ADDR_2, False)
    _init_PDN(anritsu_handle, pdn3, UE_IPV4_ADDR_3, UE_IPV6_ADDR_3, True)
    vnid1 = anritsu_handle.get_IMS(DEFAULT_VNID)
    if sim_card == P0135Ax:
        vnid2 = anritsu_handle.get_IMS(2)
        vnid3 = anritsu_handle.get_IMS(3)
        _init_IMS(
            anritsu_handle,
            vnid1,
            sim_card,
            ipv6_address=CSCF_IPV6_ADDR,
            auth=True)
        _init_IMS(
            anritsu_handle,
            vnid2,
            sim_card,
            ipv6_address=CSCF_IPV6_ADDR_2,
            ip_type="IPV6")
        _init_IMS(
            anritsu_handle,
            vnid3,
            sim_card,
            ipv6_address=CSCF_IPV6_ADDR_3,
            ip_type="IPV6")
    elif sim_card == VzW12349:
        _init_IMS(anritsu_handle, vnid1, sim_card, auth=True)
    else:
        _init_IMS(anritsu_handle, vnid1, sim_card)
    return [lte_bts, gsm_bts]


def set_system_model_lte_1x(anritsu_handle, user_params, sim_card):
    """ Configures Anritsu system for LTE and 1x simulation

    Args:
        anritsu_handle: anritusu device object.
        user_params: pointer to user supplied parameters

    Returns:
        Lte and 1x BTS objects
    """
    anritsu_handle.set_simulation_model(BtsTechnology.LTE,
                                        BtsTechnology.CDMA1X)
    # setting BTS parameters
    lte_bts = anritsu_handle.get_BTS(BtsNumber.BTS1)
    cdma1x_bts = anritsu_handle.get_BTS(BtsNumber.BTS2)
    _init_lte_bts(lte_bts, user_params, CELL_1, sim_card)
    _init_1x_bts(cdma1x_bts, user_params, CELL_2, sim_card)
    pdn1 = anritsu_handle.get_PDN(PDN_NO_1)
    pdn2 = anritsu_handle.get_PDN(PDN_NO_2)
    pdn3 = anritsu_handle.get_PDN(PDN_NO_3)
    # Initialize PDN IP address for internet connection sharing
    _init_PDN(anritsu_handle, pdn1, UE_IPV4_ADDR_1, UE_IPV6_ADDR_1, True)
    _init_PDN(anritsu_handle, pdn2, UE_IPV4_ADDR_2, UE_IPV6_ADDR_2, False)
    _init_PDN(anritsu_handle, pdn3, UE_IPV4_ADDR_3, UE_IPV6_ADDR_3, True)
    vnid1 = anritsu_handle.get_IMS(DEFAULT_VNID)
    if sim_card == P0135Ax:
        vnid2 = anritsu_handle.get_IMS(2)
        vnid3 = anritsu_handle.get_IMS(3)
        _init_IMS(
            anritsu_handle,
            vnid1,
            sim_card,
            ipv6_address=CSCF_IPV6_ADDR,
            auth=True)
        _init_IMS(
            anritsu_handle,
            vnid2,
            sim_card,
            ipv6_address=CSCF_IPV6_ADDR_2,
            ip_type="IPV6")
        _init_IMS(
            anritsu_handle,
            vnid3,
            sim_card,
            ipv6_address=CSCF_IPV6_ADDR_3,
            ip_type="IPV6")
    elif sim_card == VzW12349:
        _init_IMS(anritsu_handle, vnid1, sim_card, auth=True)
    else:
        _init_IMS(anritsu_handle, vnid1, sim_card)
    return [lte_bts, cdma1x_bts]


def set_system_model_lte_evdo(anritsu_handle, user_params, sim_card):
    """ Configures Anritsu system for LTE and EVDO simulation

    Args:
        anritsu_handle: anritusu device object.
        user_params: pointer to user supplied parameters

    Returns:
        Lte and 1x BTS objects
    """
    anritsu_handle.set_simulation_model(BtsTechnology.LTE, BtsTechnology.EVDO)
    # setting BTS parameters
    lte_bts = anritsu_handle.get_BTS(BtsNumber.BTS1)
    evdo_bts = anritsu_handle.get_BTS(BtsNumber.BTS2)
    _init_lte_bts(lte_bts, user_params, CELL_1, sim_card)
    _init_evdo_bts(evdo_bts, user_params, CELL_2, sim_card)
    pdn1 = anritsu_handle.get_PDN(PDN_NO_1)
    pdn2 = anritsu_handle.get_PDN(PDN_NO_2)
    pdn3 = anritsu_handle.get_PDN(PDN_NO_3)
    # Initialize PDN IP address for internet connection sharing
    _init_PDN(anritsu_handle, pdn1, UE_IPV4_ADDR_1, UE_IPV6_ADDR_1, True)
    _init_PDN(anritsu_handle, pdn2, UE_IPV4_ADDR_2, UE_IPV6_ADDR_2, False)
    _init_PDN(anritsu_handle, pdn3, UE_IPV4_ADDR_3, UE_IPV6_ADDR_3, True)
    vnid1 = anritsu_handle.get_IMS(DEFAULT_VNID)
    if sim_card == P0135Ax:
        vnid2 = anritsu_handle.get_IMS(2)
        vnid3 = anritsu_handle.get_IMS(3)
        _init_IMS(
            anritsu_handle,
            vnid1,
            sim_card,
            ipv6_address=CSCF_IPV6_ADDR,
            auth=True)
        _init_IMS(
            anritsu_handle,
            vnid2,
            sim_card,
            ipv6_address=CSCF_IPV6_ADDR_2,
            ip_type="IPV6")
        _init_IMS(
            anritsu_handle,
            vnid3,
            sim_card,
            ipv6_address=CSCF_IPV6_ADDR_3,
            ip_type="IPV6")
    elif sim_card == VzW12349:
        _init_IMS(anritsu_handle, vnid1, sim_card, auth=True)
    else:
        _init_IMS(anritsu_handle, vnid1, sim_card)
    return [lte_bts, evdo_bts]


def set_system_model_wcdma_gsm(anritsu_handle, user_params, sim_card):
    """ Configures Anritsu system for WCDMA and GSM simulation

    Args:
        anritsu_handle: anritusu device object.
        user_params: pointer to user supplied parameters

    Returns:
        Wcdma and Gsm BTS objects
    """
    anritsu_handle.set_simulation_model(BtsTechnology.WCDMA, BtsTechnology.GSM)
    # setting BTS parameters
    wcdma_bts = anritsu_handle.get_BTS(BtsNumber.BTS1)
    gsm_bts = anritsu_handle.get_BTS(BtsNumber.BTS2)
    _init_wcdma_bts(wcdma_bts, user_params, CELL_1, sim_card)
    _init_gsm_bts(gsm_bts, user_params, CELL_2, sim_card)
    pdn1 = anritsu_handle.get_PDN(PDN_NO_1)
    # Initialize PDN IP address for internet connection sharing
    _init_PDN(anritsu_handle, pdn1, UE_IPV4_ADDR_1, UE_IPV6_ADDR_1, False)
    return [wcdma_bts, gsm_bts]


def set_system_model_gsm_gsm(anritsu_handle, user_params, sim_card):
    """ Configures Anritsu system for GSM and GSM simulation

    Args:
        anritsu_handle: anritusu device object.
        user_params: pointer to user supplied parameters

    Returns:
        Wcdma and Gsm BTS objects
    """
    anritsu_handle.set_simulation_model(BtsTechnology.GSM, BtsTechnology.GSM)
    # setting BTS parameters
    gsm1_bts = anritsu_handle.get_BTS(BtsNumber.BTS1)
    gsm2_bts = anritsu_handle.get_BTS(BtsNumber.BTS2)
    _init_gsm_bts(gsm1_bts, user_params, CELL_1, sim_card)
    _init_gsm_bts(gsm2_bts, user_params, CELL_2, sim_card)
    pdn1 = anritsu_handle.get_PDN(PDN_NO_1)
    # Initialize PDN IP address for internet connection sharing
    _init_PDN(anritsu_handle, pdn1, UE_IPV4_ADDR_1, UE_IPV6_ADDR_1, False)
    return [gsm1_bts, gsm2_bts]

def load_system_model_from_config_files(anritsu_handle, user_params, sim_card):
    # TODO: this function should go. it is only here while testing.
    
    """ Configures Anritsu system for LTE simulation

    Args:
        anritsu_handle: anritusu device object.
        user_params: pointer to user supplied parameters

    Returns:
        Lte BTS object
    """
    
    anritsu_handle.load_simulation_paramfile('C:\\Users\MD8475A\Documents\DAN_configs\Anritsu_SIM_Bo.wnssp')
    anritsu_handle.load_cell_paramfile('C:\\Users\MD8475A\Documents\\DAN_configs\\Anritsu_SIM_cell_config3.wnscp')
    
    lte_bts = anritsu_handle.get_BTS(BtsNumber.BTS1)

    return [lte_bts]

def load_system_model_from_config_files_ca(anritsu_handle, user_params, sim_card):
    # TODO: this function should go. it is only here while testing.
    
    """ Configures Anritsu system for LTE simulation

    Args:
        anritsu_handle: anritusu device object.
        user_params: pointer to user supplied parameters

    Returns:
        Lte BTS object
    """
    
    anritsu_handle.load_simulation_paramfile('C:\\Users\MD8475A\Documents\DAN_configs\Anritsu_SIM_Bo_ca.wnssp')
    anritsu_handle.load_cell_paramfile('C:\\Users\MD8475A\Documents\\DAN_configs\\Anritsu_SIM_cell_config3_ca.wnscp')
    
    lte_bts1 = anritsu_handle.get_BTS(BtsNumber.BTS1)
    lte_bts2 = anritsu_handle.get_BTS(BtsNumber.BTS2)

    return [lte_bts1, lte_bts2]

def set_system_model_lte(anritsu_handle, user_params, sim_card):
    """ Configures Anritsu system for LTE simulation

    Args:
        anritsu_handle: anritusu device object.
        user_params: pointer to user supplied parameters

    Returns:
        Lte BTS object
    """
    anritsu_handle.set_simulation_model(BtsTechnology.LTE)
    # setting BTS parameters
    lte_bts = anritsu_handle.get_BTS(BtsNumber.BTS1)
    _init_lte_bts(lte_bts, user_params, CELL_1, sim_card)
    pdn1 = anritsu_handle.get_PDN(PDN_NO_1)
    pdn2 = anritsu_handle.get_PDN(PDN_NO_2)
    pdn3 = anritsu_handle.get_PDN(PDN_NO_3)
    # Initialize PDN IP address for internet connection sharing
    _init_PDN(anritsu_handle, pdn1, UE_IPV4_ADDR_1, UE_IPV6_ADDR_1, True)
    _init_PDN(anritsu_handle, pdn2, UE_IPV4_ADDR_2, UE_IPV6_ADDR_2, False)
    _init_PDN(anritsu_handle, pdn3, UE_IPV4_ADDR_3, UE_IPV6_ADDR_3, True)
    vnid1 = anritsu_handle.get_IMS(DEFAULT_VNID)
    if sim_card == P0135Ax:
        vnid2 = anritsu_handle.get_IMS(2)
        vnid3 = anritsu_handle.get_IMS(3)
        _init_IMS(
            anritsu_handle,
            vnid1,
            sim_card,
            ipv6_address=CSCF_IPV6_ADDR,
            auth=True)
        _init_IMS(
            anritsu_handle,
            vnid2,
            sim_card,
            ipv6_address=CSCF_IPV6_ADDR_2,
            ip_type="IPV6")
        _init_IMS(
            anritsu_handle,
            vnid3,
            sim_card,
            ipv6_address=CSCF_IPV6_ADDR_3,
            ip_type="IPV6")
    elif sim_card == VzW12349:
        _init_IMS(anritsu_handle, vnid1, sim_card, auth=True)
    else:
        _init_IMS(anritsu_handle, vnid1, sim_card)
    return [lte_bts]


def set_system_model_wcdma(anritsu_handle, user_params, sim_card):
    """ Configures Anritsu system for WCDMA simulation

    Args:
        anritsu_handle: anritusu device object.
        user_params: pointer to user supplied parameters

    Returns:
        Wcdma BTS object
    """
    anritsu_handle.set_simulation_model(BtsTechnology.WCDMA)
    # setting BTS parameters
    wcdma_bts = anritsu_handle.get_BTS(BtsNumber.BTS1)
    _init_wcdma_bts(wcdma_bts, user_params, CELL_1, sim_card)
    pdn1 = anritsu_handle.get_PDN(PDN_NO_1)
    # Initialize PDN IP address for internet connection sharing
    _init_PDN(anritsu_handle, pdn1, UE_IPV4_ADDR_1, UE_IPV6_ADDR_1, False)
    return [wcdma_bts]


def set_system_model_gsm(anritsu_handle, user_params, sim_card):
    """ Configures Anritsu system for GSM simulation

    Args:
        anritsu_handle: anritusu device object.
        user_params: pointer to user supplied parameters

    Returns:
        Gsm BTS object
    """
    anritsu_handle.set_simulation_model(BtsTechnology.GSM)
    # setting BTS parameters
    gsm_bts = anritsu_handle.get_BTS(BtsNumber.BTS1)
    _init_gsm_bts(gsm_bts, user_params, CELL_1, sim_card)
    pdn1 = anritsu_handle.get_PDN(PDN_NO_1)
    # Initialize PDN IP address for internet connection sharing
    _init_PDN(anritsu_handle, pdn1, UE_IPV4_ADDR_1, UE_IPV6_ADDR_1, False)
    return [gsm_bts]


def set_system_model_1x(anritsu_handle, user_params, sim_card):
    """ Configures Anritsu system for CDMA 1X simulation

    Args:
        anritsu_handle: anritusu device object.
        user_params: pointer to user supplied parameters

    Returns:
        Cdma 1x BTS object
    """
    PDN_ONE = 1
    anritsu_handle.set_simulation_model(BtsTechnology.CDMA1X)
    # setting BTS parameters
    cdma1x_bts = anritsu_handle.get_BTS(BtsNumber.BTS1)
    _init_1x_bts(cdma1x_bts, user_params, CELL_1, sim_card)
    pdn1 = anritsu_handle.get_PDN(PDN_ONE)
    # Initialize PDN IP address for internet connection sharing
    _init_PDN(anritsu_handle, pdn1, UE_IPV4_ADDR_1, UE_IPV6_ADDR_1, False)
    return [cdma1x_bts]


def set_system_model_1x_evdo(anritsu_handle, user_params, sim_card):
    """ Configures Anritsu system for CDMA 1X simulation

    Args:
        anritsu_handle: anritusu device object.
        user_params: pointer to user supplied parameters

    Returns:
        Cdma 1x BTS object
    """
    PDN_ONE = 1
    anritsu_handle.set_simulation_model(BtsTechnology.CDMA1X,
                                        BtsTechnology.EVDO)
    # setting BTS parameters
    cdma1x_bts = anritsu_handle.get_BTS(BtsNumber.BTS1)
    evdo_bts = anritsu_handle.get_BTS(BtsNumber.BTS2)
    _init_1x_bts(cdma1x_bts, user_params, CELL_1, sim_card)
    _init_evdo_bts(evdo_bts, user_params, CELL_2, sim_card)
    pdn1 = anritsu_handle.get_PDN(PDN_ONE)
    # Initialize PDN IP address for internet connection sharing
    _init_PDN(anritsu_handle, pdn1, UE_IPV4_ADDR_1, UE_IPV6_ADDR_1, False)
    return [cdma1x_bts]


def wait_for_bts_state(log, btsnumber, state, timeout=30):
    """ Waits for BTS to be in the specified state ("IN" or "OUT")

    Args:
        btsnumber: BTS number.
        state: expected state

    Returns:
        True for success False for failure
    """
    #  state value are "IN" and "OUT"
    status = False
    sleep_interval = 1
    wait_time = timeout

    if state is "IN":
        service_state = BtsServiceState.SERVICE_STATE_IN
    elif state is "OUT":
        service_state = BtsServiceState.SERVICE_STATE_OUT
    else:
        log.info("wrong state value")
        return status

    if btsnumber.service_state is service_state:
        log.info("BTS state is already in {}".format(state))
        return True

    # set to desired service state
    btsnumber.service_state = service_state

    while wait_time > 0:
        if service_state == btsnumber.service_state:
            status = True
            break
        time.sleep(sleep_interval)
        wait_time = wait_time - sleep_interval

    if not status:
        log.info("Timeout: Expected BTS state is not received.")
    return status


class _CallSequenceException(Exception):
    pass


def call_mo_setup_teardown(
        log,
        ad,
        anritsu_handle,
        callee_number,
        teardown_side=CALL_TEARDOWN_PHONE,
        is_emergency=False,
        wait_time_in_call=WAIT_TIME_IN_CALL,
        is_ims_call=False,
        ims_virtual_network_id=DEFAULT_IMS_VIRTUAL_NETWORK_ID):
    """ Makes a MO call and tear down the call

    Args:
        ad: Android device object.
        anritsu_handle: Anritsu object.
        callee_number: Number to be called.
        teardown_side: the side to end the call (Phone or remote).
        is_emergency: is the call an emergency call.
        wait_time_in_call: Time to wait when phone in call.
        is_ims_call: is the call expected to be ims call.
        ims_virtual_network_id: ims virtual network id.

    Returns:
        True for success False for failure
    """

    log.info("Making Call to " + callee_number)
    virtual_phone_handle = anritsu_handle.get_VirtualPhone()

    try:
        # for an IMS call we either check CSCF or *nothing* (no virtual phone).
        if is_ims_call:
            # we only need pre-call registration in a non-emergency case
            if not is_emergency:
                if not wait_for_ims_cscf_status(log, anritsu_handle,
                                                ims_virtual_network_id,
                                                ImsCscfStatus.SIPIDLE.value):
                    raise _CallSequenceException(
                        "Phone IMS status is not idle.")
        else:
            if not wait_for_virtualphone_state(log, virtual_phone_handle,
                                               VirtualPhoneStatus.STATUS_IDLE):
                raise _CallSequenceException("Virtual Phone not idle.")

        if not initiate_call(log, ad, callee_number, is_emergency):
            raise _CallSequenceException("Initiate call failed.")

        if is_ims_call:
            if not wait_for_ims_cscf_status(log, anritsu_handle,
                                            ims_virtual_network_id,
                                            ImsCscfStatus.CALLING.value):
                raise _CallSequenceException(
                    "Phone IMS status is not calling.")
            if not wait_for_ims_cscf_status(log, anritsu_handle,
                                            ims_virtual_network_id,
                                            ImsCscfStatus.CONNECTED.value):
                raise _CallSequenceException(
                    "Phone IMS status is not connected.")
        else:
            # check Virtual phone answered the call
            if not wait_for_virtualphone_state(
                    log, virtual_phone_handle,
                    VirtualPhoneStatus.STATUS_VOICECALL_INPROGRESS):
                raise _CallSequenceException("Virtual Phone not in call.")

        time.sleep(wait_time_in_call)

        if not ad.droid.telecomIsInCall():
            raise _CallSequenceException("Call ended before delay_in_call.")

        if teardown_side is CALL_TEARDOWN_REMOTE:
            log.info("Disconnecting the call from Remote")
            if is_ims_call:
                anritsu_handle.ims_cscf_call_action(ims_virtual_network_id,
                                                    ImsCscfCall.END.value)
            else:
                virtual_phone_handle.set_voice_on_hook()
            if not wait_for_droid_not_in_call(log, ad,
                                              MAX_WAIT_TIME_CALL_DROP):
                raise _CallSequenceException("DUT call not drop.")
        else:
            log.info("Disconnecting the call from DUT")
            if not hangup_call(log, ad):
                raise _CallSequenceException(
                    "Error in Hanging-Up Call on DUT.")

        if is_ims_call:
            if not wait_for_ims_cscf_status(log, anritsu_handle,
                                            ims_virtual_network_id,
                                            ImsCscfStatus.SIPIDLE.value):
                raise _CallSequenceException("Phone IMS status is not idle.")
        else:
            if not wait_for_virtualphone_state(log, virtual_phone_handle,
                                               VirtualPhoneStatus.STATUS_IDLE):
                raise _CallSequenceException(
                    "Virtual Phone not idle after hangup.")
        return True

    except _CallSequenceException as e:
        log.error(e)
        return False
    finally:
        try:
            if ad.droid.telecomIsInCall():
                ad.droid.telecomEndCall()
        except Exception as e:
            log.error(str(e))


def handover_tc(log,
                anritsu_handle,
                wait_time=0,
                s_bts=BtsNumber.BTS1,
                t_bts=BtsNumber.BTS2,
                timeout=60):
    """ Setup and perform a handover test case in MD8475A

    Args:
        anritsu_handle: Anritsu object.
        s_bts: Serving (originating) BTS
        t_bts: Target (destination) BTS
        wait_time: time to wait before handover

    Returns:
        True for success False for failure
    """
    log.info("Starting HO test case procedure")
    log.info("Serving BTS = {}, Target BTS = {}".format(s_bts, t_bts))
    time.sleep(wait_time)
    ho_tc = anritsu_handle.get_AnritsuTestCases()
    ho_tc.procedure = TestProcedure.PROCEDURE_HO
    ho_tc.bts_direction = (s_bts, t_bts)
    ho_tc.power_control = TestPowerControl.POWER_CONTROL_DISABLE
    ho_tc.measurement_LTE = TestMeasurement.MEASUREMENT_DISABLE
    anritsu_handle.start_testcase()
    status = anritsu_handle.get_testcase_status()
    timer = 0
    while status == "0":
        time.sleep(1)
        status = anritsu_handle.get_testcase_status()
        timer += 1
        if timer > timeout:
            return "Handover Test Case time out in {} sec!".format(timeout)
    return status


def make_ims_call(log,
                  ad,
                  anritsu_handle,
                  callee_number,
                  is_emergency=False,
                  check_ims_reg=True,
                  check_ims_calling=True,
                  mo=True,
                  ims_virtual_network_id=DEFAULT_IMS_VIRTUAL_NETWORK_ID):
    """ Makes a MO call after IMS registred

    Args:
        ad: Android device object.
        anritsu_handle: Anritsu object.
        callee_number: Number to be called.
        check_ims_reg: check if Anritsu cscf server state is "SIPIDLE".
        check_ims_calling: check if Anritsu cscf server state is "CALLING".
        mo: Mobile originated call
        ims_virtual_network_id: ims virtual network id.

    Returns:
        True for success False for failure
    """

    try:
        # confirm ims registration
        if check_ims_reg:
            if not wait_for_ims_cscf_status(log, anritsu_handle,
                                            ims_virtual_network_id,
                                            ImsCscfStatus.SIPIDLE.value):
                raise _CallSequenceException("IMS/CSCF status is not idle.")
        if mo:  # make MO call
            log.info("Making Call to " + callee_number)
            if not initiate_call(log, ad, callee_number, is_emergency):
                raise _CallSequenceException("Initiate call failed.")
            if not wait_for_ims_cscf_status(log, anritsu_handle,
                                            ims_virtual_network_id,
                                            ImsCscfStatus.CALLING.value):
                raise _CallSequenceException(
                    "Phone IMS status is not calling.")
        else:  # make MT call
            log.info("Making IMS Call to UE from MD8475A...")
            anritsu_handle.ims_cscf_call_action(ims_virtual_network_id,
                                                ImsCscfCall.MAKE.value)
            if not wait_for_ims_cscf_status(log, anritsu_handle,
                                            ims_virtual_network_id,
                                            ImsCscfStatus.RINGING.value):
                raise _CallSequenceException(
                    "Phone IMS status is not ringing.")
            # answer the call on the UE
            if not wait_and_answer_call(log, ad):
                raise _CallSequenceException("UE Answer call Fail")

        if not wait_for_ims_cscf_status(log, anritsu_handle,
                                        ims_virtual_network_id,
                                        ImsCscfStatus.CONNECTED.value):
            raise _CallSequenceException(
                "MD8475A IMS status is not connected.")
        return True

    except _CallSequenceException as e:
        log.error(e)
        return False


def tear_down_call(log,
                   ad,
                   anritsu_handle,
                   ims_virtual_network_id=DEFAULT_IMS_VIRTUAL_NETWORK_ID):
    """ Check and End a VoLTE call

    Args:
        ad: Android device object.
        anritsu_handle: Anritsu object.
        ims_virtual_network_id: ims virtual network id.

    Returns:
        True for success False for failure
    """
    try:
        # end the call from phone
        log.info("Disconnecting the call from DUT")
        if not hangup_call(log, ad):
            raise _CallSequenceException("Error in Hanging-Up Call on DUT.")
        # confirm if CSCF status is back to idle
        if not wait_for_ims_cscf_status(log, anritsu_handle,
                                        ims_virtual_network_id,
                                        ImsCscfStatus.SIPIDLE.value):
            raise _CallSequenceException("IMS/CSCF status is not idle.")
        return True

    except _CallSequenceException as e:
        log.error(e)
        return False
    finally:
        try:
            if ad.droid.telecomIsInCall():
                ad.droid.telecomEndCall()
        except Exception as e:
            log.error(str(e))


# This procedure is for VoLTE mobility test cases
def ims_call_ho(log,
                ad,
                anritsu_handle,
                callee_number,
                is_emergency=False,
                check_ims_reg=True,
                check_ims_calling=True,
                mo=True,
                wait_time_in_volte=WAIT_TIME_IN_CALL_FOR_IMS,
                ims_virtual_network_id=DEFAULT_IMS_VIRTUAL_NETWORK_ID):
    """ Makes a MO call after IMS registred, then handover

    Args:
        ad: Android device object.
        anritsu_handle: Anritsu object.
        callee_number: Number to be called.
        check_ims_reg: check if Anritsu cscf server state is "SIPIDLE".
        check_ims_calling: check if Anritsu cscf server state is "CALLING".
        mo: Mobile originated call
        wait_time_in_volte: Time for phone in VoLTE call, not used for SRLTE
        ims_virtual_network_id: ims virtual network id.

    Returns:
        True for success False for failure
    """

    try:
        # confirm ims registration
        if check_ims_reg:
            if not wait_for_ims_cscf_status(log, anritsu_handle,
                                            ims_virtual_network_id,
                                            ImsCscfStatus.SIPIDLE.value):
                raise _CallSequenceException("IMS/CSCF status is not idle.")
        if mo:  # make MO call
            log.info("Making Call to " + callee_number)
            if not initiate_call(log, ad, callee_number, is_emergency):
                raise _CallSequenceException("Initiate call failed.")
            if not wait_for_ims_cscf_status(log, anritsu_handle,
                                            ims_virtual_network_id,
                                            ImsCscfStatus.CALLING.value):
                raise _CallSequenceException(
                    "Phone IMS status is not calling.")
        else:  # make MT call
            log.info("Making IMS Call to UE from MD8475A...")
            anritsu_handle.ims_cscf_call_action(ims_virtual_network_id,
                                                ImsCscfCall.MAKE.value)
            if not wait_for_ims_cscf_status(log, anritsu_handle,
                                            ims_virtual_network_id,
                                            ImsCscfStatus.RINGING.value):
                raise _CallSequenceException(
                    "Phone IMS status is not ringing.")
            # answer the call on the UE
            if not wait_and_answer_call(log, ad):
                raise _CallSequenceException("UE Answer call Fail")

        if not wait_for_ims_cscf_status(log, anritsu_handle,
                                        ims_virtual_network_id,
                                        ImsCscfStatus.CONNECTED.value):
            raise _CallSequenceException("Phone IMS status is not connected.")
        log.info(
            "Wait for {} seconds before handover".format(wait_time_in_volte))
        time.sleep(wait_time_in_volte)

        # Once VoLTE call is connected, then Handover
        log.info("Starting handover procedure...")
        result = handover_tc(anritsu_handle, BtsNumber.BTS1, BtsNumber.BTS2)
        log.info("Handover procedure ends with result code {}".format(result))
        log.info(
            "Wait for {} seconds after handover".format(wait_time_in_volte))
        time.sleep(wait_time_in_volte)

        # check if the phone stay in call
        if not ad.droid.telecomIsInCall():
            raise _CallSequenceException("Call ended before delay_in_call.")
        # end the call from phone
        log.info("Disconnecting the call from DUT")
        if not hangup_call(log, ad):
            raise _CallSequenceException("Error in Hanging-Up Call on DUT.")
        # confirm if CSCF status is back to idle
        if not wait_for_ims_cscf_status(log, anritsu_handle,
                                        ims_virtual_network_id,
                                        ImsCscfStatus.SIPIDLE.value):
            raise _CallSequenceException("IMS/CSCF status is not idle.")

        return True

    except _CallSequenceException as e:
        log.error(e)
        return False
    finally:
        try:
            if ad.droid.telecomIsInCall():
                ad.droid.telecomEndCall()
        except Exception as e:
            log.error(str(e))


# This procedure is for SRLTE CSFB and SRVCC test cases
def ims_call_cs_teardown(
        log,
        ad,
        anritsu_handle,
        callee_number,
        teardown_side=CALL_TEARDOWN_PHONE,
        is_emergency=False,
        check_ims_reg=True,
        check_ims_calling=True,
        srvcc=None,
        mo=True,
        wait_time_in_volte=WAIT_TIME_IN_CALL_FOR_IMS,
        wait_time_in_cs=WAIT_TIME_IN_CALL,
        wait_time_in_alert=WAIT_TIME_IN_ALERT,
        ims_virtual_network_id=DEFAULT_IMS_VIRTUAL_NETWORK_ID):
    """ Makes a MO call after IMS registred, transit to CS, tear down the call

    Args:
        ad: Android device object.
        anritsu_handle: Anritsu object.
        callee_number: Number to be called.
        teardown_side: the side to end the call (Phone or remote).
        is_emergency: to make emergency call on the phone.
        check_ims_reg: check if Anritsu cscf server state is "SIPIDLE".
        check_ims_calling: check if Anritsu cscf server state is "CALLING".
        srvcc: is the test case a SRVCC call.
        mo: Mobile originated call
        wait_time_in_volte: Time for phone in VoLTE call, not used for SRLTE
        wait_time_in_cs: Time for phone in CS call.
        ims_virtual_network_id: ims virtual network id.

    Returns:
        True for success False for failure
    """

    virtual_phone_handle = anritsu_handle.get_VirtualPhone()

    try:
        # confirm ims registration
        if check_ims_reg:
            if not wait_for_ims_cscf_status(log, anritsu_handle,
                                            ims_virtual_network_id,
                                            ImsCscfStatus.SIPIDLE.value):
                raise _CallSequenceException("IMS/CSCF status is not idle.")
        # confirm virtual phone in idle
        if not wait_for_virtualphone_state(log, virtual_phone_handle,
                                           VirtualPhoneStatus.STATUS_IDLE):
            raise _CallSequenceException("Virtual Phone not idle.")
        if mo:  # make MO call
            log.info("Making Call to " + callee_number)
            if not initiate_call(log, ad, callee_number, is_emergency):
                raise _CallSequenceException("Initiate call failed.")
        else:  # make MT call
            log.info("Making IMS Call to UE from MD8475A...")
            anritsu_handle.ims_cscf_call_action(ims_virtual_network_id,
                                                ImsCscfCall.MAKE.value)
        # if check ims calling is required
        if check_ims_calling:
            if mo:
                if not wait_for_ims_cscf_status(log, anritsu_handle,
                                                ims_virtual_network_id,
                                                ImsCscfStatus.CALLING.value):
                    raise _CallSequenceException(
                        "Phone IMS status is not calling.")
            else:
                if not wait_for_ims_cscf_status(log, anritsu_handle,
                                                ims_virtual_network_id,
                                                ImsCscfStatus.RINGING.value):
                    raise _CallSequenceException(
                        "Phone IMS status is not ringing.")

            # if SRVCC, check if VoLTE call is connected, then Handover
            if srvcc != None:
                if srvcc == "InCall":
                    if not wait_for_ims_cscf_status(
                            log, anritsu_handle, ims_virtual_network_id,
                            ImsCscfStatus.CONNECTED.value):
                        raise _CallSequenceException(
                            "Phone IMS status is not connected.")
                    # stay in call for "wait_time_in_volte" seconds
                    time.sleep(wait_time_in_volte)
                elif srvcc == "Alert":
                    # ring for WAIT_TIME_IN_ALERT seconds
                    time.sleep(WAIT_TIME_IN_ALERT)
                # SRVCC by handover test case procedure
                srvcc_tc = anritsu_handle.get_AnritsuTestCases()
                srvcc_tc.procedure = TestProcedure.PROCEDURE_HO
                srvcc_tc.bts_direction = (BtsNumber.BTS1, BtsNumber.BTS2)
                srvcc_tc.power_control = TestPowerControl.POWER_CONTROL_DISABLE
                srvcc_tc.measurement_LTE = TestMeasurement.MEASUREMENT_DISABLE
                anritsu_handle.start_testcase()
                time.sleep(5)
        if not mo:
            # answer the call on the UE
            if not wait_and_answer_call(log, ad):
                raise _CallSequenceException("UE Answer call Fail")
        # check if Virtual phone in the call
        if not wait_for_virtualphone_state(
                log, virtual_phone_handle,
                VirtualPhoneStatus.STATUS_VOICECALL_INPROGRESS):
            raise _CallSequenceException("Virtual Phone not in call.")
        # stay in call for "wait_time_in_cs" seconds
        time.sleep(wait_time_in_cs)
        # check if the phone stay in call
        if not ad.droid.telecomIsInCall():
            raise _CallSequenceException("Call ended before delay_in_call.")
        # end the call
        if teardown_side is CALL_TEARDOWN_REMOTE:
            log.info("Disconnecting the call from Remote")
            virtual_phone_handle.set_voice_on_hook()
            if not wait_for_droid_not_in_call(log, ad,
                                              MAX_WAIT_TIME_CALL_DROP):
                raise _CallSequenceException("DUT call not drop.")
        else:
            log.info("Disconnecting the call from DUT")
            if not hangup_call(log, ad):
                raise _CallSequenceException(
                    "Error in Hanging-Up Call on DUT.")
        # confirm if virtual phone status is back to idle
        if not wait_for_virtualphone_state(log, virtual_phone_handle,
                                           VirtualPhoneStatus.STATUS_IDLE):
            raise _CallSequenceException(
                "Virtual Phone not idle after hangup.")
        return True

    except _CallSequenceException as e:
        log.error(e)
        return False
    finally:
        try:
            if ad.droid.telecomIsInCall():
                ad.droid.telecomEndCall()
        except Exception as e:
            log.error(str(e))


def call_mt_setup_teardown(log,
                           ad,
                           virtual_phone_handle,
                           caller_number=None,
                           teardown_side=CALL_TEARDOWN_PHONE,
                           rat=""):
    """ Makes a call from Anritsu Virtual phone to device and tear down the call

    Args:
        ad: Android device object.
        virtual_phone_handle: Anritus virtual phone handle
        caller_number =  Caller number
        teardown_side = specifiy the side to end the call (Phone or remote)

    Returns:
        True for success False for failure
    """
    log.info("Receive MT Call - Making a call to the phone from remote")
    try:
        if not wait_for_virtualphone_state(log, virtual_phone_handle,
                                           VirtualPhoneStatus.STATUS_IDLE):
            raise Exception("Virtual Phone is not in a state to start call")
        if caller_number is not None:
            if rat == RAT_1XRTT:
                virtual_phone_handle.id_c2k = caller_number
            else:
                virtual_phone_handle.id = caller_number
        virtual_phone_handle.set_voice_off_hook()

        if not wait_and_answer_call(log, ad, caller_number):
            raise Exception("Answer call Fail")

        time.sleep(WAIT_TIME_IN_CALL)

        if not ad.droid.telecomIsInCall():
            raise Exception("Call ended before delay_in_call.")
    except Exception:
        return False

    if ad.droid.telecomIsInCall():
        if teardown_side is CALL_TEARDOWN_REMOTE:
            log.info("Disconnecting the call from Remote")
            virtual_phone_handle.set_voice_on_hook()
        else:
            log.info("Disconnecting the call from Phone")
            ad.droid.telecomEndCall()

    wait_for_virtualphone_state(log, virtual_phone_handle,
                                VirtualPhoneStatus.STATUS_IDLE)
    ensure_phone_idle(log, ad)

    return True


def wait_for_sms_deliver_success(log, ad, time_to_wait=60):
    sms_deliver_event = EventSmsDeliverSuccess
    sleep_interval = 2
    status = False
    event = None

    try:
        event = ad.ed.pop_event(sms_deliver_event, time_to_wait)
        status = True
    except Empty:
        log.info("Timeout: Expected event is not received.")
    return status


def wait_for_sms_sent_success(log, ad, time_to_wait=60):
    sms_sent_event = EventSmsSentSuccess
    sleep_interval = 2
    status = False
    event = None

    try:
        event = ad.ed.pop_event(sms_sent_event, time_to_wait)
        log.info(event)
        status = True
    except Empty:
        log.info("Timeout: Expected event is not received.")
    return status


def wait_for_incoming_sms(log, ad, time_to_wait=60):
    sms_received_event = EventSmsReceived
    sleep_interval = 2
    status = False
    event = None

    try:
        event = ad.ed.pop_event(sms_received_event, time_to_wait)
        log.info(event)
        status = True
    except Empty:
        log.info("Timeout: Expected event is not received.")
    return status, event


def verify_anritsu_received_sms(log, vp_handle, receiver_number, message, rat):
    if rat == RAT_1XRTT:
        receive_sms = vp_handle.receiveSms_c2k()
    else:
        receive_sms = vp_handle.receiveSms()

    if receive_sms == "NONE":
        return False
    split = receive_sms.split('&')
    text = ""
    if rat == RAT_1XRTT:
        # TODO: b/26296388 There is some problem when retrieving message with é
        # from Anritsu.
        return True
    for i in range(len(split)):
        if split[i].startswith('Text='):
            text = split[i][5:]
            text = AnritsuUtils.gsm_decode(text)
            break
    # TODO: b/26296388 Verify Phone number
    if text != message:
        log.error("Wrong message received")
        return False
    return True


def sms_mo_send(log, ad, vp_handle, receiver_number, message, rat=""):
    try:
        if not wait_for_virtualphone_state(log, vp_handle,
                                           VirtualPhoneStatus.STATUS_IDLE):
            raise Exception("Virtual Phone is not in a state to receive SMS")
        log.info("Sending SMS to " + receiver_number)
        ad.droid.smsSendTextMessage(receiver_number, message, False)
        log.info("Waiting for SMS sent event")
        test_status = wait_for_sms_sent_success(log, ad)
        if not test_status:
            raise Exception("Failed to send SMS")
        if not verify_anritsu_received_sms(log, vp_handle, receiver_number,
                                           message, rat):
            raise Exception("Anritsu didn't receive message")
    except Exception as e:
        log.error("Exception :" + str(e))
        return False
    return True


def sms_mt_receive_verify(log, ad, vp_handle, sender_number, message, rat=""):
    ad.droid.smsStartTrackingIncomingMessage()
    try:
        if not wait_for_virtualphone_state(log, vp_handle,
                                           VirtualPhoneStatus.STATUS_IDLE):
            raise Exception("Virtual Phone is not in a state to receive SMS")
        log.info("Waiting for Incoming SMS from " + sender_number)
        if rat == RAT_1XRTT:
            vp_handle.sendSms_c2k(sender_number, message)
        else:
            vp_handle.sendSms(sender_number, message)
        test_status, event = wait_for_incoming_sms(log, ad)
        if not test_status:
            raise Exception("Failed to receive SMS")
        log.info("Incoming SMS: Sender " + event['data']['Sender'])
        log.info("Incoming SMS: Message " + event['data']['Text'])
        if event['data']['Sender'] != sender_number:
            raise Exception("Wrong sender Number")
        if event['data']['Text'] != message:
            raise Exception("Wrong message")
    except Exception as e:
        log.error("exception: " + str(e))
        return False
    finally:
        ad.droid.smsStopTrackingIncomingMessage()
    return True


def wait_for_ims_cscf_status(log,
                             anritsu_handle,
                             virtual_network_id,
                             status,
                             timeout=MAX_WAIT_TIME_IMS_CSCF_STATE):
    """ Wait for IMS CSCF to be in expected state.

    Args:
        log: log object
        anritsu_handle: anritsu object
        virtual_network_id: virtual network id to be monitored
        status: expected status
        timeout: wait time
    """
    sleep_interval = 1
    wait_time = timeout
    while wait_time > 0:
        if status == anritsu_handle.get_ims_cscf_status(virtual_network_id):
            return True
        time.sleep(sleep_interval)
        wait_time = wait_time - sleep_interval
    return False


def wait_for_virtualphone_state(log,
                                vp_handle,
                                state,
                                timeout=MAX_WAIT_TIME_VIRTUAL_PHONE_STATE):
    """ Waits for Anritsu Virtual phone to be in expected state

    Args:
        ad: Android device object.
        vp_handle: Anritus virtual phone handle
        state =  expected state

    Returns:
        True for success False for failure
    """
    status = False
    sleep_interval = 1
    wait_time = timeout
    while wait_time > 0:
        if vp_handle.status == state:
            log.info(vp_handle.status)
            status = True
            break
        time.sleep(sleep_interval)
        wait_time = wait_time - sleep_interval

    if not status:
        log.info("Timeout: Expected state is not received.")
    return status


# There is a difference between CMAS/ETWS message formation in LTE/WCDMA and CDMA 1X
# LTE and CDMA : 3GPP
# CDMA 1X: 3GPP2
# hence different functions
def cmas_receive_verify_message_lte_wcdma(
        log, ad, anritsu_handle, serial_number, message_id, warning_message):
    """ Makes Anritsu to send a CMAS message and phone and verifies phone
        receives the message on LTE/WCDMA

    Args:
        ad: Android device object.
        anritsu_handle: Anritus device object
        serial_number =  serial number of CMAS message
        message_id =  CMAS message ID
        warning_message =  CMAS warning message

    Returns:
        True for success False for failure
    """
    status = False
    event = None
    ad.droid.smsStartTrackingGsmEmergencyCBMessage()
    anritsu_handle.send_cmas_lte_wcdma(
        hex(serial_number), message_id, warning_message)
    try:
        log.info("Waiting for CMAS Message")
        event = ad.ed.pop_event(EventCmasReceived, 60)
        status = True
        log.info(event)
        if warning_message != event['data']['message']:
            log.info("Wrong warning messgae received")
            status = False
        if message_id != hex(event['data']['serviceCategory']):
            log.info("Wrong warning messgae received")
            status = False
    except Empty:
        log.info("Timeout: Expected event is not received.")

    ad.droid.smsStopTrackingGsmEmergencyCBMessage()
    return status


def cmas_receive_verify_message_cdma1x(
        log,
        ad,
        anritsu_handle,
        message_id,
        service_category,
        alert_text,
        response_type=CMAS_C2K_RESPONSETYPE_SHELTER,
        severity=CMAS_C2K_SEVERITY_EXTREME,
        urgency=CMAS_C2K_URGENCY_IMMEDIATE,
        certainty=CMAS_C2K_CERTIANTY_OBSERVED):
    """ Makes Anritsu to send a CMAS message and phone and verifies phone
        receives the message on CDMA 1X

    Args:
        ad: Android device object.
        anritsu_handle: Anritus device object
        serial_number =  serial number of CMAS message
        message_id =  CMAS message ID
        warning_message =  CMAS warning message

    Returns:
        True for success False for failure
    """
    status = False
    event = None
    ad.droid.smsStartTrackingCdmaEmergencyCBMessage()
    anritsu_handle.send_cmas_etws_cdma1x(message_id, service_category,
                                         alert_text, response_type, severity,
                                         urgency, certainty)
    try:
        log.info("Waiting for CMAS Message")
        event = ad.ed.pop_event(EventCmasReceived, 60)
        status = True
        log.info(event)
        if alert_text != event['data']['message']:
            log.info("Wrong alert messgae received")
            status = False

        if event['data']['cmasResponseType'].lower() != response_type.lower():
            log.info("Wrong response type received")
            status = False

        if event['data']['cmasUrgency'].lower() != urgency.lower():
            log.info("Wrong cmasUrgency received")
            status = False

        if event['data']['cmasSeverity'].lower() != severity.lower():
            Log.info("Wrong cmasSeverity received")
            status = False
    except Empty:
        log.info("Timeout: Expected event is not received.")

    ad.droid.smsStopTrackingCdmaEmergencyCBMessage()
    return status


def etws_receive_verify_message_lte_wcdma(
        log, ad, anritsu_handle, serial_number, message_id, warning_message):
    """ Makes Anritsu to send a ETWS message and phone and verifies phone
        receives the message on LTE/WCDMA

    Args:
        ad: Android device object.
        anritsu_handle: Anritus device object
        serial_number =  serial number of ETWS message
        message_id =  ETWS message ID
        warning_message =  ETWS warning message

    Returns:
        True for success False for failure
    """
    status = False
    event = None
    if message_id == ETWS_WARNING_EARTHQUAKE:
        warning_type = "Earthquake"
    elif message_id == ETWS_WARNING_EARTHQUAKETSUNAMI:
        warning_type = "EarthquakeandTsunami"
    elif message_id == ETWS_WARNING_TSUNAMI:
        warning_type = "Tsunami"
    elif message_id == ETWS_WARNING_TEST_MESSAGE:
        warning_type = "test"
    elif message_id == ETWS_WARNING_OTHER_EMERGENCY:
        warning_type = "other"
    ad.droid.smsStartTrackingGsmEmergencyCBMessage()
    anritsu_handle.send_etws_lte_wcdma(
        hex(serial_number), message_id, warning_type, warning_message, "ON",
        "ON")
    try:
        log.info("Waiting for ETWS Message")
        event = ad.ed.pop_event(EventEtwsReceived, 60)
        status = True
        log.info(event)
        # TODO: b/26296388 Event data verification
    except Empty:
        log.info("Timeout: Expected event is not received.")

    ad.droid.smsStopTrackingGsmEmergencyCBMessage()
    return status


def etws_receive_verify_message_cdma1x(log, ad, anritsu_handle, serial_number,
                                       message_id, warning_message):
    """ Makes Anritsu to send a ETWS message and phone and verifies phone
        receives the message on CDMA1X

    Args:
        ad: Android device object.
        anritsu_handle: Anritus device object
        serial_number =  serial number of ETWS message
        message_id =  ETWS message ID
        warning_message =  ETWS warning message

    Returns:
        True for success False for failure
    """
    status = False
    event = None
    # TODO: b/26296388 need to add logic to check etws.
    return status


def read_ue_identity(log, ad, anritsu_handle, identity_type):
    """ Get the UE identity IMSI, IMEI, IMEISV

    Args:
        ad: Android device object.
        anritsu_handle: Anritus device object
        identity_type: Identity type(IMSI/IMEI/IMEISV)

    Returns:
        Requested Identity value
    """
    return anritsu_handle.get_ue_identity(identity_type)


def get_transmission_mode(user_params, cell_no):
    """ Returns the TRANSMODE to be used from the user specified parameters
        or default value

    Args:
        user_params: pointer to user supplied parameters
        cell_no: specify the cell number this BTS is configured
        Anritsu supports two cells. so cell_1 or cell_2

    Returns:
        TM to be used
    """
    key = "cell{}_transmission_mode".format(cell_no)
    transmission_mode = user_params.get(key, DEFAULT_T_MODE)
    return transmission_mode


def get_dl_antenna(user_params, cell_no):
    """ Returns the DL ANTENNA to be used from the user specified parameters
        or default value

    Args:
        user_params: pointer to user supplied parameters
        cell_no: specify the cell number this BTS is configured
        Anritsu supports two cells. so cell_1 or cell_2

    Returns:
        number of DL ANTENNAS to be used
    """
    key = "cell{}_dl_antenna".format(cell_no)
    dl_antenna = user_params.get(key, DEFAULT_DL_ANTENNA)
    return dl_antenna


def get_lte_band(user_params, cell_no):
    """ Returns the LTE BAND to be used from the user specified parameters
        or default value

    Args:
        user_params: pointer to user supplied parameters
        cell_no: specify the cell number this BTS is configured
        Anritsu supports two cells. so cell_1 or cell_2

    Returns:
        LTE BAND to be used
    """
    key = "cell{}_lte_band".format(cell_no)
    band = DEFAULT_LTE_BAND[cell_no - 1]
    return user_params.get(key, band)


def get_wcdma_band(user_params, cell_no):
    """ Returns the WCDMA BAND to be used from the user specified parameters
        or default value

    Args:
        user_params: pointer to user supplied parameters
        cell_no: specify the cell number this BTS is configured
        Anritsu supports two cells. so cell_1 or cell_2

    Returns:
        WCDMA BAND to be used
    """
    key = "cell{}_wcdma_band".format(cell_no)
    wcdma_band = user_params.get(key, DEFAULT_WCDMA_BAND)
    return wcdma_band


def get_gsm_band(user_params, cell_no):
    """ Returns the GSM BAND to be used from the user specified parameters
        or default value

    Args:
        user_params: pointer to user supplied parameters
        cell_no: specify the cell number this BTS is configured
        Anritsu supports two cells. so cell_1 or cell_2

    Returns:
        GSM BAND to be used
    """
    key = "cell{}_gsm_band".format(cell_no)
    gsm_band = user_params.get(key, DEFAULT_GSM_BAND)
    return gsm_band


def get_1x_band(user_params, cell_no, sim_card):
    """ Returns the 1X BAND to be used from the user specified parameters
        or default value

    Args:
        user_params: pointer to user supplied parameters
        cell_no: specify the cell number this BTS is configured
        Anritsu supports two cells. so cell_1 or cell_2

    Returns:
        1X BAND to be used
    """
    key = "cell{}_1x_band".format(cell_no)
    band = VzW_CDMA1x_BAND if sim_card == VzW12349 else DEFAULT_CDMA1X_BAND
    return user_params.get(key, band)


def get_evdo_band(user_params, cell_no, sim_card):
    """ Returns the EVDO BAND to be used from the user specified parameters
        or default value

    Args:
        user_params: pointer to user supplied parameters
        cell_no: specify the cell number this BTS is configured
        Anritsu supports two cells. so cell_1 or cell_2

    Returns:
        EVDO BAND to be used
    """
    key = "cell{}_evdo_band".format(cell_no)
    band = VzW_EVDO_BAND if sim_card == VzW12349 else DEFAULT_EVDO_BAND
    return user_params.get(key, band)


def get_wcdma_rac(user_params, cell_no):
    """ Returns the WCDMA RAC to be used from the user specified parameters
        or default value

    Args:
        user_params: pointer to user supplied parameters
        cell_no: specify the cell number this BTS is configured
        Anritsu supports two cells. so cell_1 or cell_2

    Returns:
        WCDMA RAC to be used
    """
    key = "cell{}_wcdma_rac".format(cell_no)
    try:
        wcdma_rac = user_params[key]
    except KeyError:
        wcdma_rac = DEFAULT_RAC
    return wcdma_rac


def get_gsm_rac(user_params, cell_no):
    """ Returns the GSM RAC to be used from the user specified parameters
        or default value

    Args:
        user_params: pointer to user supplied parameters
        cell_no: specify the cell number this BTS is configured
        Anritsu supports two cells. so cell_1 or cell_2

    Returns:
        GSM RAC to be used
    """
    key = "cell{}_gsm_rac".format(cell_no)
    try:
        gsm_rac = user_params[key]
    except KeyError:
        gsm_rac = DEFAULT_RAC
    return gsm_rac


def get_wcdma_lac(user_params, cell_no):
    """ Returns the WCDMA LAC to be used from the user specified parameters
        or default value

    Args:
        user_params: pointer to user supplied parameters
        cell_no: specify the cell number this BTS is configured
        Anritsu supports two cells. so cell_1 or cell_2

    Returns:
        WCDMA LAC to be used
    """
    key = "cell{}_wcdma_lac".format(cell_no)
    try:
        wcdma_lac = user_params[key]
    except KeyError:
        wcdma_lac = DEFAULT_LAC
    return wcdma_lac


def get_gsm_lac(user_params, cell_no):
    """ Returns the GSM LAC to be used from the user specified parameters
        or default value

    Args:
        user_params: pointer to user supplied parameters
        cell_no: specify the cell number this BTS is configured
        Anritsu supports two cells. so cell_1 or cell_2

    Returns:
        GSM LAC to be used
    """
    key = "cell{}_gsm_lac".format(cell_no)
    try:
        gsm_lac = user_params[key]
    except KeyError:
        gsm_lac = DEFAULT_LAC
    return gsm_lac


def get_lte_mcc(user_params, cell_no, sim_card):
    """ Returns the LTE MCC to be used from the user specified parameters
        or default value

    Args:
        user_params: pointer to user supplied parameters
        cell_no: specify the cell number this BTS is configured
        Anritsu supports two cells. so cell_1 or cell_2

    Returns:
        LTE MCC to be used
    """

    key = "cell{}_lte_mcc".format(cell_no)
    mcc = VzW_MCC if sim_card == VzW12349 else DEFAULT_MCC
    return user_params.get(key, mcc)


def get_lte_mnc(user_params, cell_no, sim_card):
    """ Returns the LTE MNC to be used from the user specified parameters
        or default value

    Args:
        user_params: pointer to user supplied parameters
        cell_no: specify the cell number this BTS is configured
        Anritsu supports two cells. so cell_1 or cell_2

    Returns:
        LTE MNC to be used
    """
    key = "cell{}_lte_mnc".format(cell_no)
    mnc = VzW_MNC if sim_card == VzW12349 else DEFAULT_MNC
    return user_params.get(key, mnc)


def get_wcdma_mcc(user_params, cell_no, sim_card):
    """ Returns the WCDMA MCC to be used from the user specified parameters
        or default value

    Args:
        user_params: pointer to user supplied parameters
        cell_no: specify the cell number this BTS is configured
        Anritsu supports two cells. so cell_1 or cell_2

    Returns:
        WCDMA MCC to be used
    """
    key = "cell{}_wcdma_mcc".format(cell_no)
    mcc = VzW_MCC if sim_card == VzW12349 else DEFAULT_MCC
    return user_params.get(key, mcc)


def get_wcdma_mnc(user_params, cell_no, sim_card):
    """ Returns the WCDMA MNC to be used from the user specified parameters
        or default value

    Args:
        user_params: pointer to user supplied parameters
        cell_no: specify the cell number this BTS is configured
        Anritsu supports two cells. so cell_1 or cell_2

    Returns:
        WCDMA MNC to be used
    """
    key = "cell{}_wcdma_mnc".format(cell_no)
    mnc = VzW_MNC if sim_card == VzW12349 else DEFAULT_MNC
    return user_params.get(key, mnc)


def get_gsm_mcc(user_params, cell_no, sim_card):
    """ Returns the GSM MCC to be used from the user specified parameters
        or default value

    Args:
        user_params: pointer to user supplied parameters
        cell_no: specify the cell number this BTS is configured
        Anritsu supports two cells. so cell_1 or cell_2

    Returns:
        GSM MCC to be used
    """
    key = "cell{}_gsm_mcc".format(cell_no)
    mcc = VzW_MCC if sim_card == VzW12349 else DEFAULT_MCC
    return user_params.get(key, mcc)


def get_gsm_mnc(user_params, cell_no, sim_card):
    """ Returns the GSM MNC to be used from the user specified parameters
        or default value

    Args:
        user_params: pointer to user supplied parameters
        cell_no: specify the cell number this BTS is configured
        Anritsu supports two cells. so cell_1 or cell_2

    Returns:
        GSM MNC to be used
    """
    key = "cell{}_gsm_mnc".format(cell_no)
    mnc = VzW_MNC if sim_card == VzW12349 else DEFAULT_MNC
    return user_params.get(key, mnc)


def get_1x_mcc(user_params, cell_no, sim_card):
    """ Returns the 1X MCC to be used from the user specified parameters
        or default value

    Args:
        user_params: pointer to user supplied parameters
        cell_no: specify the cell number this BTS is configured
        Anritsu supports two cells. so cell_1 or cell_2

    Returns:
        1X MCC to be used
    """
    key = "cell{}_1x_mcc".format(cell_no)
    mcc = VzW_MCC if sim_card == VzW12349 else DEFAULT_MCC
    return user_params.get(key, mcc)


def get_1x_channel(user_params, cell_no, sim_card):
    """ Returns the 1X Channel to be used from the user specified parameters
        or default value

    Args:
        user_params: pointer to user supplied parameters
        cell_no: specify the cell number this BTS is configured
        Anritsu supports two cells. so cell_1 or cell_2

    Returns:
        1X Channel to be used
    """
    key = "cell{}_1x_channel".format(cell_no)
    ch = VzW_CDMA1x_CH if sim_card == VzW12349 else DEFAULT_CDMA1X_CH
    return user_params.get(key, ch)


def get_1x_sid(user_params, cell_no, sim_card):
    """ Returns the 1X SID to be used from the user specified parameters
        or default value

    Args:
        user_params: pointer to user supplied parameters
        cell_no: specify the cell number this BTS is configured
        Anritsu supports two cells. so cell_1 or cell_2

    Returns:
        1X SID to be used
    """
    key = "cell{}_1x_sid".format(cell_no)
    sid = VzW_CDMA1X_SID if sim_card == VzW12349 else DEFAULT_CDMA1X_SID
    return user_params.get(key, sid)


def get_1x_nid(user_params, cell_no, sim_card):
    """ Returns the 1X NID to be used from the user specified parameters
        or default value

    Args:
        user_params: pointer to user supplied parameters
        cell_no: specify the cell number this BTS is configured
        Anritsu supports two cells. so cell_1 or cell_2

    Returns:
        1X NID to be used
    """
    key = "cell{}_1x_nid".format(cell_no)
    nid = VzW_CDMA1X_NID if sim_card == VzW12349 else DEFAULT_CDMA1X_NID
    return user_params.get(key, nid)


def get_evdo_channel(user_params, cell_no, sim_card):
    """ Returns the EVDO Channel to be used from the user specified parameters
        or default value

    Args:
        user_params: pointer to user supplied parameters
        cell_no: specify the cell number this BTS is configured
        Anritsu supports two cells. so cell_1 or cell_2

    Returns:
        EVDO Channel to be used
    """
    key = "cell{}_evdo_channel".format(cell_no)
    ch = VzW_EVDO_CH if sim_card == VzW12349 else DEFAULT_EVDO_CH
    return user_params.get(key, ch)


def get_evdo_sid(user_params, cell_no, sim_card):
    """ Returns the EVDO SID to be used from the user specified parameters
        or default value

    Args:
        user_params: pointer to user supplied parameters
        cell_no: specify the cell number this BTS is configured
        Anritsu supports two cells. so cell_1 or cell_2

    Returns:
        EVDO SID to be used
    """
    key = "cell{}_evdo_sid".format(cell_no)
    return user_params.get(key, DEFAULT_EVDO_SECTOR_ID)
    sid = VzW_EVDO_SECTOR_ID if sim_card == VzW12349 else DEFAULT_EVDO_SECTOR_ID
    return user_params.get(key, sid)


def get_csfb_type(user_params):
    """ Returns the CSFB Type to be used from the user specified parameters
        or default value

    Args:
        user_params: pointer to user supplied parameters
        cell_no: specify the cell number this BTS is configured
        Anritsu supports two cells. so cell_1 or cell_2

    Returns:
        CSFB Type to be used
    """
    try:
        csfb_type = user_params["csfb_type"]
    except KeyError:
        csfb_type = CsfbType.CSFB_TYPE_REDIRECTION
    return csfb_type


def set_post_sim_params(anritsu_handle, user_params, sim_card):
    if sim_card == P0135Ax:
        anritsu_handle.send_command("PDNCHECKAPN 1,ims")
        anritsu_handle.send_command("PDNCHECKAPN 2,fast.t-mobile.com")
        anritsu_handle.send_command("PDNIMS 1,ENABLE")
        anritsu_handle.send_command("PDNVNID 1,1")
        anritsu_handle.send_command("PDNIMS 2,ENABLE")
        anritsu_handle.send_command("PDNVNID 2,2")
        anritsu_handle.send_command("PDNIMS 3,ENABLE")
        anritsu_handle.send_command("PDNVNID 3,1")
    if sim_card == VzW12349:
        anritsu_handle.send_command("PDNCHECKAPN 1,IMS")
        anritsu_handle.send_command("PDNCHECKAPN 2,VZWINTERNET")
        anritsu_handle.send_command("PDNIMS 1,ENABLE")
        anritsu_handle.send_command("PDNVNID 1,1")
        anritsu_handle.send_command("PDNIMS 3,ENABLE")
        anritsu_handle.send_command("PDNVNID 3,1")
