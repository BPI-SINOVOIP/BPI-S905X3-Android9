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

from future import standard_library
standard_library.install_aliases()

import concurrent.futures
import json
import logging
import re
import os
import urllib.parse
import time

from acts import utils
from queue import Empty
from acts.asserts import abort_all
from acts.controllers.adb import AdbError
from acts.controllers.android_device import DEFAULT_QXDM_LOG_PATH
from acts.controllers.android_device import SL4A_APK_NAME
from acts.controllers.sl4a_lib.event_dispatcher import EventDispatcher
from acts.test_utils.tel.tel_defines import AOSP_PREFIX
from acts.test_utils.tel.tel_defines import CARD_POWER_DOWN
from acts.test_utils.tel.tel_defines import CARD_POWER_UP
from acts.test_utils.tel.tel_defines import CARRIER_UNKNOWN
from acts.test_utils.tel.tel_defines import COUNTRY_CODE_LIST
from acts.test_utils.tel.tel_defines import DATA_STATE_CONNECTED
from acts.test_utils.tel.tel_defines import DATA_STATE_DISCONNECTED
from acts.test_utils.tel.tel_defines import DATA_ROAMING_ENABLE
from acts.test_utils.tel.tel_defines import DATA_ROAMING_DISABLE
from acts.test_utils.tel.tel_defines import GEN_4G
from acts.test_utils.tel.tel_defines import GEN_UNKNOWN
from acts.test_utils.tel.tel_defines import INCALL_UI_DISPLAY_BACKGROUND
from acts.test_utils.tel.tel_defines import INCALL_UI_DISPLAY_FOREGROUND
from acts.test_utils.tel.tel_defines import INVALID_SIM_SLOT_INDEX
from acts.test_utils.tel.tel_defines import INVALID_SUB_ID
from acts.test_utils.tel.tel_defines import MAX_SAVED_VOICE_MAIL
from acts.test_utils.tel.tel_defines import MAX_SCREEN_ON_TIME
from acts.test_utils.tel.tel_defines import \
    MAX_WAIT_TIME_ACCEPT_CALL_TO_OFFHOOK_EVENT
from acts.test_utils.tel.tel_defines import MAX_WAIT_TIME_AIRPLANEMODE_EVENT
from acts.test_utils.tel.tel_defines import MAX_WAIT_TIME_CALL_DROP
from acts.test_utils.tel.tel_defines import MAX_WAIT_TIME_CALL_INITIATION
from acts.test_utils.tel.tel_defines import MAX_WAIT_TIME_CALLEE_RINGING
from acts.test_utils.tel.tel_defines import MAX_WAIT_TIME_CONNECTION_STATE_UPDATE
from acts.test_utils.tel.tel_defines import MAX_WAIT_TIME_DATA_SUB_CHANGE
from acts.test_utils.tel.tel_defines import MAX_WAIT_TIME_CALL_IDLE_EVENT
from acts.test_utils.tel.tel_defines import MAX_WAIT_TIME_NW_SELECTION
from acts.test_utils.tel.tel_defines import MAX_WAIT_TIME_SMS_RECEIVE
from acts.test_utils.tel.tel_defines import MAX_WAIT_TIME_SMS_SENT_SUCCESS
from acts.test_utils.tel.tel_defines import MAX_WAIT_TIME_TELECOM_RINGING
from acts.test_utils.tel.tel_defines import MAX_WAIT_TIME_VOICE_MAIL_COUNT
from acts.test_utils.tel.tel_defines import MAX_WAIT_TIME_WFC_DISABLED
from acts.test_utils.tel.tel_defines import MAX_WAIT_TIME_WFC_ENABLED
from acts.test_utils.tel.tel_defines import NETWORK_MODE_LTE_ONLY
from acts.test_utils.tel.tel_defines import NETWORK_CONNECTION_TYPE_CELL
from acts.test_utils.tel.tel_defines import NETWORK_CONNECTION_TYPE_WIFI
from acts.test_utils.tel.tel_defines import NETWORK_SERVICE_DATA
from acts.test_utils.tel.tel_defines import NETWORK_SERVICE_VOICE
from acts.test_utils.tel.tel_defines import PHONE_NUMBER_STRING_FORMAT_7_DIGIT
from acts.test_utils.tel.tel_defines import PHONE_NUMBER_STRING_FORMAT_10_DIGIT
from acts.test_utils.tel.tel_defines import PHONE_NUMBER_STRING_FORMAT_11_DIGIT
from acts.test_utils.tel.tel_defines import PHONE_NUMBER_STRING_FORMAT_12_DIGIT
from acts.test_utils.tel.tel_defines import RAT_FAMILY_GSM
from acts.test_utils.tel.tel_defines import RAT_FAMILY_LTE
from acts.test_utils.tel.tel_defines import RAT_FAMILY_WLAN
from acts.test_utils.tel.tel_defines import RAT_FAMILY_WCDMA
from acts.test_utils.tel.tel_defines import RAT_1XRTT
from acts.test_utils.tel.tel_defines import RAT_UNKNOWN
from acts.test_utils.tel.tel_defines import SERVICE_STATE_EMERGENCY_ONLY
from acts.test_utils.tel.tel_defines import SERVICE_STATE_IN_SERVICE
from acts.test_utils.tel.tel_defines import SERVICE_STATE_MAPPING
from acts.test_utils.tel.tel_defines import SERVICE_STATE_OUT_OF_SERVICE
from acts.test_utils.tel.tel_defines import SERVICE_STATE_POWER_OFF
from acts.test_utils.tel.tel_defines import SIM_STATE_LOADED
from acts.test_utils.tel.tel_defines import SIM_STATE_NOT_READY
from acts.test_utils.tel.tel_defines import SIM_STATE_PIN_REQUIRED
from acts.test_utils.tel.tel_defines import SIM_STATE_READY
from acts.test_utils.tel.tel_defines import SIM_STATE_UNKNOWN
from acts.test_utils.tel.tel_defines import TELEPHONY_STATE_IDLE
from acts.test_utils.tel.tel_defines import TELEPHONY_STATE_OFFHOOK
from acts.test_utils.tel.tel_defines import TELEPHONY_STATE_RINGING
from acts.test_utils.tel.tel_defines import VOICEMAIL_DELETE_DIGIT
from acts.test_utils.tel.tel_defines import WAIT_TIME_1XRTT_VOICE_ATTACH
from acts.test_utils.tel.tel_defines import WAIT_TIME_ANDROID_STATE_SETTLING
from acts.test_utils.tel.tel_defines import WAIT_TIME_BETWEEN_STATE_CHECK
from acts.test_utils.tel.tel_defines import MAX_WAIT_TIME_FOR_STATE_CHANGE
from acts.test_utils.tel.tel_defines import WAIT_TIME_CHANGE_DATA_SUB_ID
from acts.test_utils.tel.tel_defines import WAIT_TIME_IN_CALL
from acts.test_utils.tel.tel_defines import WAIT_TIME_LEAVE_VOICE_MAIL
from acts.test_utils.tel.tel_defines import WAIT_TIME_REJECT_CALL
from acts.test_utils.tel.tel_defines import WAIT_TIME_VOICE_MAIL_SERVER_RESPONSE
from acts.test_utils.tel.tel_defines import WFC_MODE_DISABLED
from acts.test_utils.tel.tel_defines import TYPE_MOBILE
from acts.test_utils.tel.tel_defines import TYPE_WIFI
from acts.test_utils.tel.tel_defines import EventCallStateChanged
from acts.test_utils.tel.tel_defines import EventConnectivityChanged
from acts.test_utils.tel.tel_defines import EventDataConnectionStateChanged
from acts.test_utils.tel.tel_defines import EventDataSmsReceived
from acts.test_utils.tel.tel_defines import EventMessageWaitingIndicatorChanged
from acts.test_utils.tel.tel_defines import EventServiceStateChanged
from acts.test_utils.tel.tel_defines import EventMmsSentSuccess
from acts.test_utils.tel.tel_defines import EventMmsDownloaded
from acts.test_utils.tel.tel_defines import EventSmsReceived
from acts.test_utils.tel.tel_defines import EventSmsSentSuccess
from acts.test_utils.tel.tel_defines import CallStateContainer
from acts.test_utils.tel.tel_defines import DataConnectionStateContainer
from acts.test_utils.tel.tel_defines import MessageWaitingIndicatorContainer
from acts.test_utils.tel.tel_defines import NetworkCallbackContainer
from acts.test_utils.tel.tel_defines import ServiceStateContainer
from acts.test_utils.tel.tel_lookup_tables import \
    connection_type_from_type_string
from acts.test_utils.tel.tel_lookup_tables import is_valid_rat
from acts.test_utils.tel.tel_lookup_tables import get_allowable_network_preference
from acts.test_utils.tel.tel_lookup_tables import \
    get_voice_mail_count_check_function
from acts.test_utils.tel.tel_lookup_tables import get_voice_mail_check_number
from acts.test_utils.tel.tel_lookup_tables import get_voice_mail_delete_digit
from acts.test_utils.tel.tel_lookup_tables import \
    network_preference_for_generation
from acts.test_utils.tel.tel_lookup_tables import operator_name_from_plmn_id
from acts.test_utils.tel.tel_lookup_tables import \
    rat_families_for_network_preference
from acts.test_utils.tel.tel_lookup_tables import rat_family_for_generation
from acts.test_utils.tel.tel_lookup_tables import rat_family_from_rat
from acts.test_utils.tel.tel_lookup_tables import rat_generation_from_rat
from acts.test_utils.tel.tel_subscription_utils import \
    get_default_data_sub_id
from acts.test_utils.tel.tel_subscription_utils import \
    get_outgoing_message_sub_id
from acts.test_utils.tel.tel_subscription_utils import \
    get_outgoing_voice_sub_id
from acts.test_utils.tel.tel_subscription_utils import \
    get_incoming_voice_sub_id
from acts.test_utils.tel.tel_subscription_utils import \
    get_incoming_message_sub_id
from acts.test_utils.wifi import wifi_test_utils
from acts.test_utils.wifi import wifi_constants
from acts.utils import adb_shell_ping
from acts.utils import load_config
from acts.utils import create_dir
from acts.utils import start_standing_subprocess
from acts.utils import stop_standing_subprocess
from acts.logger import epoch_to_log_line_timestamp
from acts.logger import normalize_log_line_timestamp
from acts.utils import get_current_epoch_time
from acts.utils import exe_cmd

WIFI_SSID_KEY = wifi_test_utils.WifiEnums.SSID_KEY
WIFI_PWD_KEY = wifi_test_utils.WifiEnums.PWD_KEY
WIFI_CONFIG_APBAND_2G = wifi_test_utils.WifiEnums.WIFI_CONFIG_APBAND_2G
WIFI_CONFIG_APBAND_5G = wifi_test_utils.WifiEnums.WIFI_CONFIG_APBAND_5G
WIFI_CONFIG_APBAND_AUTO = wifi_test_utils.WifiEnums.WIFI_CONFIG_APBAND_AUTO
log = logging
STORY_LINE = "+19523521350"


class _CallSequenceException(Exception):
    pass


class TelTestUtilsError(Exception):
    pass


def abort_all_tests(log, msg):
    log.error("Aborting all ongoing tests due to: %s.", msg)
    abort_all(msg)


def get_phone_number_by_adb(ad):
    return phone_number_formatter(
        ad.adb.shell("service call iphonesubinfo 13"))


def get_iccid_by_adb(ad):
    return ad.adb.shell("service call iphonesubinfo 11")


def get_operator_by_adb(ad):
    return ad.adb.getprop("gsm.sim.operator.alpha")


def get_plmn_by_adb(ad):
    return ad.adb.getprop("gsm.sim.operator.numeric")


def get_sub_id_by_adb(ad):
    return ad.adb.shell("service call iphonesubinfo 5")


def setup_droid_properties_by_adb(log, ad, sim_filename=None):

    # Check to see if droid already has this property
    if hasattr(ad, 'cfg'):
        return

    sim_data = None
    if sim_filename:
        try:
            sim_data = load_config(sim_filename)
        except Exception:
            log.warning("Failed to load %s!", sim_filename)

    sub_id = get_sub_id_by_adb(ad)
    iccid = get_iccid_by_adb(ad)
    ad.log.info("iccid = %s", iccid)
    if sim_data.get(iccid) and sim_data[iccid].get("phone_num"):
        phone_number = phone_number_formatter(sim_data[iccid]["phone_num"])
    else:
        phone_number = get_phone_number_by_adb(ad)
        if not phone_number and hasattr(ad, phone_number):
            phone_number = ad.phone_number
    if not phone_number:
        ad.log.error("Failed to find valid phone number for %s", iccid)
        abort_all_tests("Failed to find valid phone number for %s" % ad.serial)
    sim_record = {
        'phone_num': phone_number,
        'iccid': get_iccid_by_adb(ad),
        'sim_operator_name': get_operator_by_adb(ad),
        'operator': operator_name_from_plmn_id(get_plmn_by_adb(ad))
    }
    device_props = {'subscription': {sub_id: sim_record}}
    ad.log.info("subId %s SIM record: %s", sub_id, sim_record)
    setattr(ad, 'cfg', device_props)


def setup_droid_properties(log, ad, sim_filename=None):

    # Check to see if droid already has this property
    if hasattr(ad, 'cfg'):
        return

    if ad.skip_sl4a:
        return setup_droid_properties_by_adb(
            log, ad, sim_filename=sim_filename)

    refresh_droid_config(log, ad)
    device_props = {}
    device_props['subscription'] = {}

    sim_data = {}
    if sim_filename:
        try:
            sim_data = load_config(sim_filename)
        except Exception:
            log.warning("Failed to load %s!", sim_filename)
    if not ad.cfg["subscription"]:
        abort_all_tests(ad.log, "No Valid SIMs found in device")
    result = True
    active_sub_id = get_outgoing_voice_sub_id(ad)
    for sub_id, sub_info in ad.cfg["subscription"].items():
        sub_info["operator"] = get_operator_name(log, ad, sub_id)
        iccid = sub_info["iccid"]
        if not iccid:
            ad.log.warn("Unable to find ICC-ID for SIM")
            continue
        if sub_info.get("phone_num"):
            if getattr(ad, "phone_number", None) and check_phone_number_match(
                    sub_info["phone_num"], ad.phone_number):
                sub_info["phone_num"] = ad.phone_number
            elif iccid and iccid in sim_data and sim_data[iccid].get(
                    "phone_num"):
                if check_phone_number_match(sim_data[iccid]["phone_num"],
                                            sub_info["phone_num"]):
                    sub_info["phone_num"] = sim_data[iccid]["phone_num"]
                else:
                    ad.log.warning(
                        "phone_num %s in sim card data file for iccid %s"
                        "  do not match phone_num %s in droid subscription",
                        sim_data[iccid]["phone_num"], iccid,
                        sub_info["phone_num"])
        elif iccid and iccid in sim_data and sim_data[iccid].get("phone_num"):
            sub_info["phone_num"] = sim_data[iccid]["phone_num"]
        elif sub_id == active_sub_id:
            phone_number = get_phone_number_by_secret_code(
                ad, sub_info["sim_operator_name"])
            if phone_number:
                sub_info["phone_num"] = phone_number
            elif getattr(ad, "phone_num", None):
                sub_info["phone_num"] = ad.phone_number
        if (not sub_info.get("phone_num")) and sub_id == active_sub_id:
            ad.log.info("sub_id %s sub_info = %s", sub_id, sub_info)
            ad.log.error(
                "Unable to retrieve phone number for sub %s with iccid"
                " %s from device or testbed config or sim card file %s",
                sub_id, iccid, sim_filename)
            result = False
        if not hasattr(
                ad, 'roaming'
        ) and sub_info["sim_plmn"] != sub_info["network_plmn"] and (
                sub_info["sim_operator_name"].strip() not in
                sub_info["network_operator_name"].strip()):
            ad.log.info("roaming is not enabled, enable it")
            setattr(ad, 'roaming', True)
        ad.log.info("SubId %s info: %s", sub_id, sorted(sub_info.items()))
    data_roaming = getattr(ad, 'roaming', False)
    if get_cell_data_roaming_state_by_adb(ad) != data_roaming:
        set_cell_data_roaming_state_by_adb(ad, data_roaming)
    if not result:
        abort_all_tests(ad.log, "Failed to find valid phone number")

    ad.log.debug("cfg = %s", ad.cfg)


def refresh_droid_config(log, ad):
    """ Update Android Device cfg records for each sub_id.

    Args:
        log: log object
        ad: android device object

    Returns:
        None
    """
    if hasattr(ad, 'cfg'):
        cfg = ad.cfg.copy()
    else:
        cfg = {"subscription": {}}
    droid = ad.droid
    sub_info_list = droid.subscriptionGetAllSubInfoList()
    for sub_info in sub_info_list:
        sub_id = sub_info["subscriptionId"]
        sim_slot = sub_info["simSlotIndex"]
        if sim_slot != INVALID_SIM_SLOT_INDEX:
            sim_record = {}
            if sub_info.get("iccId"):
                sim_record["iccid"] = sub_info["iccId"]
            else:
                sim_record[
                    "iccid"] = droid.telephonyGetSimSerialNumberForSubscription(
                        sub_id)
            sim_record["sim_slot"] = sim_slot
            try:
                sim_record[
                    "phone_type"] = droid.telephonyGetPhoneTypeForSubscription(
                        sub_id)
            except:
                sim_record["phone_type"] = droid.telephonyGetPhoneType()
            if sub_info.get("mcc"):
                sim_record["mcc"] = sub_info["mcc"]
            if sub_info.get("mnc"):
                sim_record["mnc"] = sub_info["mnc"]
            sim_record[
                "sim_plmn"] = droid.telephonyGetSimOperatorForSubscription(
                    sub_id)
            sim_record["display_name"] = sub_info["displayName"]
            sim_record[
                "sim_operator_name"] = droid.telephonyGetSimOperatorNameForSubscription(
                    sub_id)
            sim_record[
                "network_plmn"] = droid.telephonyGetNetworkOperatorForSubscription(
                    sub_id)
            sim_record[
                "network_operator_name"] = droid.telephonyGetNetworkOperatorNameForSubscription(
                    sub_id)
            sim_record[
                "network_type"] = droid.telephonyGetNetworkTypeForSubscription(
                    sub_id)
            sim_record[
                "sim_country"] = droid.telephonyGetSimCountryIsoForSubscription(
                    sub_id)
            if sub_info.get("number"):
                sim_record["phone_num"] = sub_info["number"]
            else:
                sim_record["phone_num"] = phone_number_formatter(
                    droid.telephonyGetLine1NumberForSubscription(sub_id))
            sim_record[
                "phone_tag"] = droid.telephonyGetLine1AlphaTagForSubscription(
                    sub_id)
            if (not sim_record["phone_num"]
                ) and cfg["subscription"].get(sub_id):
                sim_record["phone_num"] = cfg["subscription"][sub_id][
                    "phone_num"]
            cfg['subscription'][sub_id] = sim_record
            ad.log.debug("SubId %s SIM record: %s", sub_id, sim_record)
    setattr(ad, 'cfg', cfg)


def get_phone_number_by_secret_code(ad, operator):
    if "T-Mobile" in operator:
        ad.droid.telecomDialNumber("#686#")
        ad.send_keycode("ENTER")
        for _ in range(12):
            output = ad.search_logcat("mobile number")
            if output:
                result = re.findall(r"mobile number is (\S+)",
                                    output[-1]["log_message"])
                ad.send_keycode("BACK")
                return result[0]
            else:
                time.sleep(5)
    return ""


def get_slot_index_from_subid(log, ad, sub_id):
    try:
        info = ad.droid.subscriptionGetSubInfoForSubscriber(sub_id)
        return info['simSlotIndex']
    except KeyError:
        return INVALID_SIM_SLOT_INDEX


def get_num_active_sims(log, ad):
    """ Get the number of active SIM cards by counting slots

    Args:
        ad: android_device object.

    Returns:
        result: The number of loaded (physical) SIM cards
    """
    # using a dictionary as a cheap way to prevent double counting
    # in the situation where multiple subscriptions are on the same SIM.
    # yes, this is a corner corner case.
    valid_sims = {}
    subInfo = ad.droid.subscriptionGetAllSubInfoList()
    for info in subInfo:
        ssidx = info['simSlotIndex']
        if ssidx == INVALID_SIM_SLOT_INDEX:
            continue
        valid_sims[ssidx] = True
    return len(valid_sims.keys())


def toggle_airplane_mode_by_adb(log, ad, new_state=None):
    """ Toggle the state of airplane mode.

    Args:
        log: log handler.
        ad: android_device object.
        new_state: Airplane mode state to set to.
            If None, opposite of the current state.
        strict_checking: Whether to turn on strict checking that checks all features.

    Returns:
        result: True if operation succeed. False if error happens.
    """
    cur_state = bool(int(ad.adb.shell("settings get global airplane_mode_on")))
    if new_state == cur_state:
        ad.log.info("Airplane mode already in %s", new_state)
        return True
    elif new_state is None:
        new_state = not cur_state

    ad.adb.shell("settings put global airplane_mode_on %s" % int(new_state))
    ad.adb.shell("am broadcast -a android.intent.action.AIRPLANE_MODE")
    return True


def toggle_airplane_mode(log, ad, new_state=None, strict_checking=True):
    """ Toggle the state of airplane mode.

    Args:
        log: log handler.
        ad: android_device object.
        new_state: Airplane mode state to set to.
            If None, opposite of the current state.
        strict_checking: Whether to turn on strict checking that checks all features.

    Returns:
        result: True if operation succeed. False if error happens.
    """
    if ad.skip_sl4a:
        return toggle_airplane_mode_by_adb(log, ad, new_state)
    else:
        return toggle_airplane_mode_msim(
            log, ad, new_state, strict_checking=strict_checking)


def get_telephony_signal_strength(ad):
    signal_strength = ad.droid.telephonyGetSignalStrength()
    #{'evdoEcio': -1, 'asuLevel': 28, 'lteSignalStrength': 14, 'gsmLevel': 0,
    # 'cdmaAsuLevel': 99, 'evdoDbm': -120, 'gsmDbm': -1, 'cdmaEcio': -160,
    # 'level': 2, 'lteLevel': 2, 'cdmaDbm': -120, 'dbm': -112, 'cdmaLevel': 0,
    # 'lteAsuLevel': 28, 'gsmAsuLevel': 99, 'gsmBitErrorRate': 0,
    # 'lteDbm': -112, 'gsmSignalStrength': 99}
    if not signal_strength: signal_strength = {}
    out = ad.adb.shell("dumpsys telephony.registry | grep -i signalstrength")
    signals = re.findall(r"(-*\d+)", out)
    for i, val in enumerate(
        ("gsmSignalStrength", "gsmBitErrorRate", "cdmaDbm", "cdmaEcio",
         "evdoDbm", "evdoEcio", "evdoSnr", "lteSignalStrength", "lteRsrp",
         "lteRsrq", "lteRssnr", "lteCqi", "lteRsrpBoost")):
        signal_strength[val] = signal_strength.get(val, int(signals[i]))
    ad.log.info("Telephony Signal strength = %s", signal_strength)
    return signal_strength


def get_wifi_signal_strength(ad):
    signal_strength = ad.droid.wifiGetConnectionInfo()['rssi']
    ad.log.info("WiFi Signal Strength is %s" % signal_strength)
    return signal_strength


def is_expected_event(event_to_check, events_list):
    """ check whether event is present in the event list

    Args:
        event_to_check: event to be checked.
        events_list: list of events
    Returns:
        result: True if event present in the list. False if not.
    """
    for event in events_list:
        if event in event_to_check['name']:
            return True
    return False


def is_sim_ready(log, ad, sim_slot_id=None):
    """ check whether SIM is ready.

    Args:
        ad: android_device object.
        sim_slot_id: check the SIM status for sim_slot_id
            This is optional. If this is None, check default SIM.

    Returns:
        result: True if all SIMs are ready. False if not.
    """
    if sim_slot_id is None:
        status = ad.droid.telephonyGetSimState()
    else:
        status = ad.droid.telephonyGetSimStateForSlotId(sim_slot_id)
    if status != SIM_STATE_READY:
        log.info("Sim not ready")
        return False
    return True


def is_sim_ready_by_adb(log, ad):
    state = ad.adb.getprop("gsm.sim.state")
    return state == SIM_STATE_READY or state == SIM_STATE_LOADED


def wait_for_sim_ready_by_adb(log, ad, wait_time=90):
    return _wait_for_droid_in_state(log, ad, wait_time, is_sim_ready_by_adb)


def get_service_state_by_adb(log, ad):
    output = ad.adb.shell("dumpsys telephony.registry | grep mServiceState")
    if "mVoiceRegState" in output:
        result = re.search(r"mVoiceRegState=(\S+)\((\S+)\)", output)
        if result:
            ad.log.info("mVoiceRegState is %s %s", result.group(1),
                        result.group(2))
            return result.group(2)
    else:
        result = re.search(r"mServiceState=(\S+)", output)
        if result:
            ad.log.info("mServiceState=%s %s", result.group(1),
                        SERVICE_STATE_MAPPING[result.group(1)])
            return SERVICE_STATE_MAPPING[result.group(1)]


def _is_expecting_event(event_recv_list):
    """ check for more event is expected in event list

    Args:
        event_recv_list: list of events
    Returns:
        result: True if more events are expected. False if not.
    """
    for state in event_recv_list:
        if state is False:
            return True
    return False


def _set_event_list(event_recv_list, sub_id_list, sub_id, value):
    """ set received event in expected event list

    Args:
        event_recv_list: list of received events
        sub_id_list: subscription ID list
        sub_id: subscription id of current event
        value: True or False
    Returns:
        None.
    """
    for i in range(len(sub_id_list)):
        if sub_id_list[i] == sub_id:
            event_recv_list[i] = value


def _wait_for_bluetooth_in_state(log, ad, state, max_wait):
    # FIXME: These event names should be defined in a common location
    _BLUETOOTH_STATE_ON_EVENT = 'BluetoothStateChangedOn'
    _BLUETOOTH_STATE_OFF_EVENT = 'BluetoothStateChangedOff'
    ad.ed.clear_events(_BLUETOOTH_STATE_ON_EVENT)
    ad.ed.clear_events(_BLUETOOTH_STATE_OFF_EVENT)

    ad.droid.bluetoothStartListeningForAdapterStateChange()
    try:
        bt_state = ad.droid.bluetoothCheckState()
        if bt_state == state:
            return True
        if max_wait <= 0:
            ad.log.error("Time out: bluetooth state still %s, expecting %s",
                         bt_state, state)
            return False

        event = {
            False: _BLUETOOTH_STATE_OFF_EVENT,
            True: _BLUETOOTH_STATE_ON_EVENT
        }[state]
        ad.ed.pop_event(event, max_wait)
        return True
    except Empty:
        ad.log.error("Time out: bluetooth state still in %s, expecting %s",
                     bt_state, state)
        return False
    finally:
        ad.droid.bluetoothStopListeningForAdapterStateChange()


# TODO: replace this with an event-based function
def _wait_for_wifi_in_state(log, ad, state, max_wait):
    return _wait_for_droid_in_state(log, ad, max_wait,
        lambda log, ad, state: \
                (True if ad.droid.wifiCheckState() == state else False),
                state)


def toggle_airplane_mode_msim(log, ad, new_state=None, strict_checking=True):
    """ Toggle the state of airplane mode.

    Args:
        log: log handler.
        ad: android_device object.
        new_state: Airplane mode state to set to.
            If None, opposite of the current state.
        strict_checking: Whether to turn on strict checking that checks all features.

    Returns:
        result: True if operation succeed. False if error happens.
    """

    ad.ed.clear_all_events()
    sub_id_list = []

    active_sub_info = ad.droid.subscriptionGetAllSubInfoList()
    for info in active_sub_info:
        sub_id_list.append(info['subscriptionId'])

    cur_state = ad.droid.connectivityCheckAirplaneMode()
    if cur_state == new_state:
        ad.log.info("Airplane mode already in %s", new_state)
        return True
    elif new_state is None:
        ad.log.info("APM Current State %s New state %s", cur_state, new_state)

    if new_state is None:
        new_state = not cur_state

    service_state_list = []
    if new_state:
        service_state_list.append(SERVICE_STATE_POWER_OFF)
        ad.log.info("Turn on airplane mode")

    else:
        # If either one of these 3 events show up, it should be OK.
        # Normal SIM, phone in service
        service_state_list.append(SERVICE_STATE_IN_SERVICE)
        # NO SIM, or Dead SIM, or no Roaming coverage.
        service_state_list.append(SERVICE_STATE_OUT_OF_SERVICE)
        service_state_list.append(SERVICE_STATE_EMERGENCY_ONLY)
        ad.log.info("Turn off airplane mode")

    for sub_id in sub_id_list:
        ad.droid.telephonyStartTrackingServiceStateChangeForSubscription(
            sub_id)

    timeout_time = time.time() + MAX_WAIT_TIME_AIRPLANEMODE_EVENT
    ad.droid.connectivityToggleAirplaneMode(new_state)

    event = None

    try:
        try:
            event = ad.ed.wait_for_event(
                EventServiceStateChanged,
                is_event_match_for_list,
                timeout=MAX_WAIT_TIME_AIRPLANEMODE_EVENT,
                field=ServiceStateContainer.SERVICE_STATE,
                value_list=service_state_list)
        except Empty:
            pass
        if event is None:
            ad.log.error("Did not get expected service state %s",
                         service_state_list)
            return False
        else:
            ad.log.info("Received event: %s", event)
    finally:
        for sub_id in sub_id_list:
            ad.droid.telephonyStopTrackingServiceStateChangeForSubscription(
                sub_id)

    # APM on (new_state=True) will turn off bluetooth but may not turn it on
    try:
        if new_state and not _wait_for_bluetooth_in_state(
                log, ad, False, timeout_time - time.time()):
            ad.log.error(
                "Failed waiting for bluetooth during airplane mode toggle")
            if strict_checking: return False
    except Exception as e:
        ad.log.error("Failed to check bluetooth state due to %s", e)
        if strict_checking:
            raise

    # APM on (new_state=True) will turn off wifi but may not turn it on
    if new_state and not _wait_for_wifi_in_state(log, ad, False,
                                                 timeout_time - time.time()):
        ad.log.error("Failed waiting for wifi during airplane mode toggle on")
        if strict_checking: return False

    if ad.droid.connectivityCheckAirplaneMode() != new_state:
        ad.log.error("Set airplane mode to %s failed", new_state)
        return False
    return True


def wait_and_answer_call(log,
                         ad,
                         incoming_number=None,
                         incall_ui_display=INCALL_UI_DISPLAY_FOREGROUND,
                         caller=None):
    """Wait for an incoming call on default voice subscription and
       accepts the call.

    Args:
        ad: android device object.
        incoming_number: Expected incoming number.
            Optional. Default is None
        incall_ui_display: after answer the call, bring in-call UI to foreground or
            background. Optional, default value is INCALL_UI_DISPLAY_FOREGROUND.
            if = INCALL_UI_DISPLAY_FOREGROUND, bring in-call UI to foreground.
            if = INCALL_UI_DISPLAY_BACKGROUND, bring in-call UI to background.
            else, do nothing.

    Returns:
        True: if incoming call is received and answered successfully.
        False: for errors
        """
    return wait_and_answer_call_for_subscription(
        log,
        ad,
        get_incoming_voice_sub_id(ad),
        incoming_number,
        incall_ui_display=incall_ui_display,
        caller=caller)


def wait_for_ringing_event(log, ad, wait_time):
    log.warning("***DEPRECATED*** wait_for_ringing_event()")
    return _wait_for_ringing_event(log, ad, wait_time)


def _wait_for_ringing_event(log, ad, wait_time):
    """Wait for ringing event.

    Args:
        log: log object.
        ad: android device object.
        wait_time: max time to wait for ringing event.

    Returns:
        event_ringing if received ringing event.
        otherwise return None.
    """
    event_ringing = None

    try:
        event_ringing = ad.ed.wait_for_event(
            EventCallStateChanged,
            is_event_match,
            timeout=wait_time,
            field=CallStateContainer.CALL_STATE,
            value=TELEPHONY_STATE_RINGING)
        ad.log.info("Receive ringing event")
    except Empty:
        ad.log.info("No Ringing Event")
    finally:
        return event_ringing


def wait_for_ringing_call(log, ad, incoming_number=None):
    """Wait for an incoming call on default voice subscription and
       accepts the call.

    Args:
        log: log object.
        ad: android device object.
        incoming_number: Expected incoming number.
            Optional. Default is None

    Returns:
        True: if incoming call is received and answered successfully.
        False: for errors
        """
    return wait_for_ringing_call_for_subscription(
        log, ad, get_incoming_voice_sub_id(ad), incoming_number)


def wait_for_ringing_call_for_subscription(
        log,
        ad,
        sub_id,
        incoming_number=None,
        caller=None,
        event_tracking_started=False,
        timeout=MAX_WAIT_TIME_CALLEE_RINGING,
        retries=1):
    """Wait for an incoming call on specified subscription.

    Args:
        log: log object.
        ad: android device object.
        sub_id: subscription ID
        incoming_number: Expected incoming number. Default is None
        event_tracking_started: True if event tracking already state outside
        timeout: time to wait for ring

    Returns:
        True: if incoming call is received and answered successfully.
        False: for errors
    """
    if not event_tracking_started:
        ad.ed.clear_events(EventCallStateChanged)
        ad.droid.telephonyStartTrackingCallStateForSubscription(sub_id)
    event_ringing = None
    for i in range(retries):
        event_ringing = _wait_for_ringing_event(log, ad, timeout)
        if event_ringing:
            ad.log.info("callee received ring event")
            break
        if ad.droid.telephonyGetCallStateForSubscription(
                sub_id
        ) == TELEPHONY_STATE_RINGING or ad.droid.telecomIsRinging():
            ad.log.info("callee in ringing state")
            break
        if i == retries - 1:
            ad.log.info(
                "callee didn't receive ring event or got into ringing state")
            return False
    if not event_tracking_started:
        ad.droid.telephonyStopTrackingCallStateChangeForSubscription(sub_id)
    if caller and not caller.droid.telecomIsInCall():
        caller.log.error("Caller not in call state")
        raise _CallSequenceException("Caller not in call state")
    if not incoming_number:
        return True

    if event_ringing and not check_phone_number_match(
            event_ringing['data'][CallStateContainer.INCOMING_NUMBER],
            incoming_number):
        ad.log.error(
            "Incoming Number not match. Expected number:%s, actual number:%s",
            incoming_number,
            event_ringing['data'][CallStateContainer.INCOMING_NUMBER])
        return False
    return True


def wait_for_call_offhook_event(
        log,
        ad,
        sub_id,
        event_tracking_started=False,
        timeout=MAX_WAIT_TIME_ACCEPT_CALL_TO_OFFHOOK_EVENT):
    """Wait for an incoming call on specified subscription.

    Args:
        log: log object.
        ad: android device object.
        event_tracking_started: True if event tracking already state outside
        timeout: time to wait for event

    Returns:
        True: if call offhook event is received.
        False: if call offhook event is not received.
    """
    if not event_tracking_started:
        ad.ed.clear_events(EventCallStateChanged)
        ad.droid.telephonyStartTrackingCallStateForSubscription(sub_id)
    try:
        ad.ed.wait_for_event(
            EventCallStateChanged,
            is_event_match,
            timeout=timeout,
            field=CallStateContainer.CALL_STATE,
            value=TELEPHONY_STATE_OFFHOOK)
    except Empty:
        ad.log.info("No event for call state change to OFFHOOK")
        return False
    finally:
        if not event_tracking_started:
            ad.droid.telephonyStopTrackingCallStateChangeForSubscription(
                sub_id)
    return True


def wait_and_answer_call_for_subscription(
        log,
        ad,
        sub_id,
        incoming_number=None,
        incall_ui_display=INCALL_UI_DISPLAY_FOREGROUND,
        timeout=MAX_WAIT_TIME_CALLEE_RINGING,
        caller=None):
    """Wait for an incoming call on specified subscription and
       accepts the call.

    Args:
        log: log object.
        ad: android device object.
        sub_id: subscription ID
        incoming_number: Expected incoming number.
            Optional. Default is None
        incall_ui_display: after answer the call, bring in-call UI to foreground or
            background. Optional, default value is INCALL_UI_DISPLAY_FOREGROUND.
            if = INCALL_UI_DISPLAY_FOREGROUND, bring in-call UI to foreground.
            if = INCALL_UI_DISPLAY_BACKGROUND, bring in-call UI to background.
            else, do nothing.

    Returns:
        True: if incoming call is received and answered successfully.
        False: for errors
    """
    ad.ed.clear_events(EventCallStateChanged)
    ad.droid.telephonyStartTrackingCallStateForSubscription(sub_id)
    try:
        if not _wait_for_droid_in_state(
                log,
                ad,
                timeout,
                wait_for_ringing_call_for_subscription,
                sub_id,
                incoming_number=None,
                caller=caller,
                event_tracking_started=True,
                timeout=WAIT_TIME_BETWEEN_STATE_CHECK):
            ad.log.info("Could not answer a call: phone never rang.")
            return False
        time.sleep(WAIT_TIME_BETWEEN_STATE_CHECK)
        ad.log.info("Accept the ring call")
        ad.droid.telecomAcceptRingingCall()

        if ad.droid.telecomIsInCall() or wait_for_call_offhook_event(
                log, ad, sub_id, event_tracking_started=True,
                timeout=timeout) or ad.droid.telecomIsInCall():
            ad.log.info("Call answered successfully.")
            return True
        else:
            ad.log.error("Could not answer the call.")
            return False
    except Exception as e:
        log.error(e)
        return False
    finally:
        ad.droid.telephonyStopTrackingCallStateChangeForSubscription(sub_id)
        if incall_ui_display == INCALL_UI_DISPLAY_FOREGROUND:
            ad.droid.telecomShowInCallScreen()
        elif incall_ui_display == INCALL_UI_DISPLAY_BACKGROUND:
            ad.droid.showHomeScreen()


def wait_and_reject_call(log,
                         ad,
                         incoming_number=None,
                         delay_reject=WAIT_TIME_REJECT_CALL,
                         reject=True):
    """Wait for an incoming call on default voice subscription and
       reject the call.

    Args:
        log: log object.
        ad: android device object.
        incoming_number: Expected incoming number.
            Optional. Default is None
        delay_reject: time to wait before rejecting the call
            Optional. Default is WAIT_TIME_REJECT_CALL

    Returns:
        True: if incoming call is received and reject successfully.
        False: for errors
    """
    return wait_and_reject_call_for_subscription(log, ad,
                                                 get_incoming_voice_sub_id(ad),
                                                 incoming_number, delay_reject,
                                                 reject)


def wait_and_reject_call_for_subscription(log,
                                          ad,
                                          sub_id,
                                          incoming_number=None,
                                          delay_reject=WAIT_TIME_REJECT_CALL,
                                          reject=True):
    """Wait for an incoming call on specific subscription and
       reject the call.

    Args:
        log: log object.
        ad: android device object.
        sub_id: subscription ID
        incoming_number: Expected incoming number.
            Optional. Default is None
        delay_reject: time to wait before rejecting the call
            Optional. Default is WAIT_TIME_REJECT_CALL

    Returns:
        True: if incoming call is received and reject successfully.
        False: for errors
    """

    if not wait_for_ringing_call_for_subscription(log, ad, sub_id,
                                                  incoming_number):
        ad.log.error("Could not reject a call: phone never rang.")
        return False

    ad.ed.clear_events(EventCallStateChanged)
    ad.droid.telephonyStartTrackingCallStateForSubscription(sub_id)
    if reject is True:
        # Delay between ringing and reject.
        time.sleep(delay_reject)
        is_find = False
        # Loop the call list and find the matched one to disconnect.
        for call in ad.droid.telecomCallGetCallIds():
            if check_phone_number_match(
                    get_number_from_tel_uri(get_call_uri(ad, call)),
                    incoming_number):
                ad.droid.telecomCallDisconnect(call)
                ad.log.info("Callee reject the call")
                is_find = True
        if is_find is False:
            ad.log.error("Callee did not find matching call to reject.")
            return False
    else:
        # don't reject on callee. Just ignore the incoming call.
        ad.log.info("Callee received incoming call. Ignore it.")
    try:
        ad.ed.wait_for_event(
            EventCallStateChanged,
            is_event_match_for_list,
            timeout=MAX_WAIT_TIME_CALL_IDLE_EVENT,
            field=CallStateContainer.CALL_STATE,
            value_list=[TELEPHONY_STATE_IDLE, TELEPHONY_STATE_OFFHOOK])
    except Empty:
        ad.log.error("No onCallStateChangedIdle event received.")
        return False
    finally:
        ad.droid.telephonyStopTrackingCallStateChangeForSubscription(sub_id)
    return True


def hangup_call(log, ad):
    """Hang up ongoing active call.

    Args:
        log: log object.
        ad: android device object.

    Returns:
        True: if all calls are cleared
        False: for errors
    """
    # short circuit in case no calls are active
    if not ad.droid.telecomIsInCall():
        return True
    ad.ed.clear_events(EventCallStateChanged)
    ad.droid.telephonyStartTrackingCallState()
    ad.log.info("Hangup call.")
    ad.droid.telecomEndCall()

    try:
        ad.ed.wait_for_event(
            EventCallStateChanged,
            is_event_match,
            timeout=MAX_WAIT_TIME_CALL_IDLE_EVENT,
            field=CallStateContainer.CALL_STATE,
            value=TELEPHONY_STATE_IDLE)
    except Empty:
        if ad.droid.telecomIsInCall():
            log.error("Hangup call failed.")
            return False
    finally:
        ad.droid.telephonyStopTrackingCallStateChange()
    return not ad.droid.telecomIsInCall()


def disconnect_call_by_id(log, ad, call_id):
    """Disconnect call by call id.
    """
    ad.droid.telecomCallDisconnect(call_id)
    return True


def _phone_number_remove_prefix(number):
    """Remove the country code and other prefix from the input phone number.
    Currently only handle phone number with the following formats:
        (US phone number format)
        +1abcxxxyyyy
        1abcxxxyyyy
        abcxxxyyyy
        abc xxx yyyy
        abc.xxx.yyyy
        abc-xxx-yyyy
        (EEUK phone number format)
        +44abcxxxyyyy
        0abcxxxyyyy

    Args:
        number: input phone number

    Returns:
        Phone number without country code or prefix
    """
    if number is None:
        return None, None
    for country_code in COUNTRY_CODE_LIST:
        if number.startswith(country_code):
            return number[len(country_code):], country_code
    if number[0] == "1" or number[0] == "0":
        return number[1:], None
    return number, None


def check_phone_number_match(number1, number2):
    """Check whether two input phone numbers match or not.

    Compare the two input phone numbers.
    If they match, return True; otherwise, return False.
    Currently only handle phone number with the following formats:
        (US phone number format)
        +1abcxxxyyyy
        1abcxxxyyyy
        abcxxxyyyy
        abc xxx yyyy
        abc.xxx.yyyy
        abc-xxx-yyyy
        (EEUK phone number format)
        +44abcxxxyyyy
        0abcxxxyyyy

        There are some scenarios we can not verify, one example is:
            number1 = +15555555555, number2 = 5555555555
            (number2 have no country code)

    Args:
        number1: 1st phone number to be compared.
        number2: 2nd phone number to be compared.

    Returns:
        True if two phone numbers match. Otherwise False.
    """
    number1 = phone_number_formatter(number1)
    number2 = phone_number_formatter(number2)
    # Handle extra country code attachment when matching phone number
    if number1[-7:] in number2 or number2[-7:] in number1:
        return True
    else:
        logging.info("phone number1 %s and number2 %s does not match" %
                     (number1, number2))
        return False


def initiate_call(log,
                  ad,
                  callee_number,
                  emergency=False,
                  timeout=MAX_WAIT_TIME_CALL_INITIATION,
                  checking_interval=5,
                  wait_time_betwn_call_initcheck=0):
    """Make phone call from caller to callee.

    Args:
        ad_caller: Caller android device object.
        callee_number: Callee phone number.
        emergency : specify the call is emergency.
            Optional. Default value is False.

    Returns:
        result: if phone call is placed successfully.
    """
    ad.ed.clear_events(EventCallStateChanged)
    sub_id = get_outgoing_voice_sub_id(ad)
    ad.droid.telephonyStartTrackingCallStateForSubscription(sub_id)

    try:
        # Make a Call
        ad.log.info("Make a phone call to %s", callee_number)
        if emergency:
            ad.droid.telecomCallEmergencyNumber(callee_number)
        else:
            ad.droid.telecomCallNumber(callee_number)

        # Sleep time for stress test b/64915613
        time.sleep(wait_time_betwn_call_initcheck)

        # Verify OFFHOOK event
        checking_retries = int(timeout / checking_interval)
        for i in range(checking_retries):
            if (ad.droid.telecomIsInCall() and
                    ad.droid.telephonyGetCallState() == TELEPHONY_STATE_OFFHOOK
                    and ad.droid.telecomGetCallState() ==
                    TELEPHONY_STATE_OFFHOOK) or wait_for_call_offhook_event(
                        log, ad, sub_id, True, checking_interval):
                return True
        ad.log.info(
            "Make call to %s fail. telecomIsInCall:%s, Telecom State:%s,"
            " Telephony State:%s", callee_number, ad.droid.telecomIsInCall(),
            ad.droid.telephonyGetCallState(), ad.droid.telecomGetCallState())
        reasons = ad.search_logcat(
            "qcril_qmi_voice_map_qmi_to_ril_last_call_failure_cause")
        if reasons:
            ad.log.info(reasons[-1]["log_message"])
        return False
    finally:
        ad.droid.telephonyStopTrackingCallStateChangeForSubscription(sub_id)


def dial_phone_number(ad, callee_number):
    for number in str(callee_number):
        if number == "#":
            ad.send_keycode("POUND")
        elif number == "*":
            ad.send_keycode("STAR")
        elif number in ["1", "2", "3", "4", "5", "6", "7", "8", "9", "0"]:
            ad.send_keycode("%s" % number)


def get_call_state_by_adb(ad):
    return ad.adb.shell("dumpsys telephony.registry | grep mCallState")


def check_call_state_connected_by_adb(ad):
    return "2" in get_call_state_by_adb(ad)


def check_call_state_idle_by_adb(ad):
    return "0" in get_call_state_by_adb(ad)


def check_call_state_ring_by_adb(ad):
    return "1" in get_call_state_by_adb(ad)


def get_incoming_call_number_by_adb(ad):
    output = ad.adb.shell(
        "dumpsys telephony.registry | grep mCallIncomingNumber")
    return re.search(r"mCallIncomingNumber=(.*)", output).group(1)


def emergency_dialer_call_by_keyevent(ad, callee_number):
    for i in range(3):
        if "EmergencyDialer" in ad.get_my_current_focus_window():
            ad.log.info("EmergencyDialer is the current focus window")
            break
        elif i <= 2:
            ad.adb.shell("am start -a com.android.phone.EmergencyDialer.DIAL")
            time.sleep(1)
        else:
            ad.log.error("Unable to bring up EmergencyDialer")
            return False
    ad.log.info("Make a phone call to %s", callee_number)
    dial_phone_number(ad, callee_number)
    ad.send_keycode("CALL")


def initiate_emergency_dialer_call_by_adb(
        log,
        ad,
        callee_number,
        timeout=MAX_WAIT_TIME_CALL_INITIATION,
        checking_interval=5):
    """Make emergency call by EmergencyDialer.

    Args:
        ad: Caller android device object.
        callee_number: Callee phone number.
        emergency : specify the call is emergency.
        Optional. Default value is False.

    Returns:
        result: if phone call is placed successfully.
    """
    try:
        # Make a Call
        ad.wakeup_screen()
        ad.send_keycode("MENU")
        ad.log.info("Call %s", callee_number)
        ad.adb.shell("am start -a com.android.phone.EmergencyDialer.DIAL")
        ad.adb.shell(
            "am start -a android.intent.action.CALL_EMERGENCY -d tel:%s" %
            callee_number)
        ad.log.info("Check call state")
        # Verify Call State
        elapsed_time = 0
        while elapsed_time < timeout:
            time.sleep(checking_interval)
            elapsed_time += checking_interval
            if check_call_state_connected_by_adb(ad):
                ad.log.info("Call to %s is connected", callee_number)
                return True
            if check_call_state_idle_by_adb(ad):
                ad.log.info("Call to %s failed", callee_number)
                return False
        ad.log.info("Make call to %s failed", callee_number)
        return False
    except Exception as e:
        ad.log.error("initiate emergency call failed with error %s", e)


def hangup_call_by_adb(ad):
    """Make emergency call by EmergencyDialer.

    Args:
        ad: Caller android device object.
        callee_number: Callee phone number.
    """
    ad.log.info("End call by adb")
    ad.send_keycode("ENDCALL")


def dumpsys_telecom_call_info(ad):
    """ Get call information by dumpsys telecom. """
    output = ad.adb.shell("dumpsys telecom")
    calls = re.findall("Call TC@\d+: {(.*?)}", output, re.DOTALL)
    calls_info = []
    for call in calls:
        call_info = {}
        for attr in ("startTime", "endTime", "direction", "isInterrupted",
                     "callTechnologies", "callTerminationsReason",
                     "connectionService", "isVedeoCall", "callProperties"):
            match = re.search(r"%s: (.*)" % attr, call)
            if match:
                call_info[attr] = match.group(1)
        call_info["inCallServices"] = re.findall(r"name: (.*)", call)
        calls_info.append(call_info)
    ad.log.debug("calls_info = %s", calls_info)
    return calls_info


def call_reject(log, ad_caller, ad_callee, reject=True):
    """Caller call Callee, then reject on callee.


    """
    subid_caller = ad_caller.droid.subscriptionGetDefaultVoiceSubId()
    subid_callee = ad_callee.incoming_voice_sub_id
    ad_caller.log.info("Sub-ID Caller %s, Sub-ID Callee %s", subid_caller,
                       subid_callee)
    return call_reject_for_subscription(log, ad_caller, ad_callee,
                                        subid_caller, subid_callee, reject)


def call_reject_for_subscription(log,
                                 ad_caller,
                                 ad_callee,
                                 subid_caller,
                                 subid_callee,
                                 reject=True):
    """
    """

    caller_number = ad_caller.cfg['subscription'][subid_caller]['phone_num']
    callee_number = ad_callee.cfg['subscription'][subid_callee]['phone_num']

    ad_caller.log.info("Call from %s to %s", caller_number, callee_number)
    try:
        if not initiate_call(log, ad_caller, callee_number):
            raise _CallSequenceException("Initiate call failed.")

        if not wait_and_reject_call_for_subscription(
                log, ad_callee, subid_callee, caller_number,
                WAIT_TIME_REJECT_CALL, reject):
            raise _CallSequenceException("Reject call fail.")
        # Check if incoming call is cleared on callee or not.
        if ad_callee.droid.telephonyGetCallStateForSubscription(
                subid_callee) == TELEPHONY_STATE_RINGING:
            raise _CallSequenceException("Incoming call is not cleared.")
        # Hangup on caller
        hangup_call(log, ad_caller)
    except _CallSequenceException as e:
        log.error(e)
        return False
    return True


def call_reject_leave_message(log,
                              ad_caller,
                              ad_callee,
                              verify_caller_func=None,
                              wait_time_in_call=WAIT_TIME_LEAVE_VOICE_MAIL):
    """On default voice subscription, Call from caller to callee,
    reject on callee, caller leave a voice mail.

    1. Caller call Callee.
    2. Callee reject incoming call.
    3. Caller leave a voice mail.
    4. Verify callee received the voice mail notification.

    Args:
        ad_caller: caller android device object.
        ad_callee: callee android device object.
        verify_caller_func: function to verify caller is in correct state while in-call.
            This is optional, default is None.
        wait_time_in_call: time to wait when leaving a voice mail.
            This is optional, default is WAIT_TIME_LEAVE_VOICE_MAIL

    Returns:
        True: if voice message is received on callee successfully.
        False: for errors
    """
    subid_caller = get_outgoing_voice_sub_id(ad_caller)
    subid_callee = get_incoming_voice_sub_id(ad_callee)
    return call_reject_leave_message_for_subscription(
        log, ad_caller, ad_callee, subid_caller, subid_callee,
        verify_caller_func, wait_time_in_call)


def call_reject_leave_message_for_subscription(
        log,
        ad_caller,
        ad_callee,
        subid_caller,
        subid_callee,
        verify_caller_func=None,
        wait_time_in_call=WAIT_TIME_LEAVE_VOICE_MAIL):
    """On specific voice subscription, Call from caller to callee,
    reject on callee, caller leave a voice mail.

    1. Caller call Callee.
    2. Callee reject incoming call.
    3. Caller leave a voice mail.
    4. Verify callee received the voice mail notification.

    Args:
        ad_caller: caller android device object.
        ad_callee: callee android device object.
        subid_caller: caller's subscription id.
        subid_callee: callee's subscription id.
        verify_caller_func: function to verify caller is in correct state while in-call.
            This is optional, default is None.
        wait_time_in_call: time to wait when leaving a voice mail.
            This is optional, default is WAIT_TIME_LEAVE_VOICE_MAIL

    Returns:
        True: if voice message is received on callee successfully.
        False: for errors
    """

    # Currently this test utility only works for TMO and ATT and SPT.
    # It does not work for VZW (see b/21559800)
    # "with VVM TelephonyManager APIs won't work for vm"

    caller_number = ad_caller.cfg['subscription'][subid_caller]['phone_num']
    callee_number = ad_callee.cfg['subscription'][subid_callee]['phone_num']

    ad_caller.log.info("Call from %s to %s", caller_number, callee_number)

    try:
        voice_mail_count_before = ad_callee.droid.telephonyGetVoiceMailCountForSubscription(
            subid_callee)
        ad_callee.log.info("voice mail count is %s", voice_mail_count_before)
        # -1 means there are unread voice mail, but the count is unknown
        # 0 means either this API not working (VZW) or no unread voice mail.
        if voice_mail_count_before != 0:
            log.warning("--Pending new Voice Mail, please clear on phone.--")

        if not initiate_call(log, ad_caller, callee_number):
            raise _CallSequenceException("Initiate call failed.")

        if not wait_and_reject_call_for_subscription(
                log, ad_callee, subid_callee, incoming_number=caller_number):
            raise _CallSequenceException("Reject call fail.")

        ad_callee.droid.telephonyStartTrackingVoiceMailStateChangeForSubscription(
            subid_callee)

        # ensure that all internal states are updated in telecom
        time.sleep(WAIT_TIME_ANDROID_STATE_SETTLING)
        ad_callee.ed.ad.ed.clear_events(EventCallStateChanged)

        if verify_caller_func and not verify_caller_func(log, ad_caller):
            raise _CallSequenceException("Caller not in correct state!")

        # TODO: b/26293512 Need to play some sound to leave message.
        # Otherwise carrier voice mail server may drop this voice mail.
        time.sleep(wait_time_in_call)

        if not verify_caller_func:
            caller_state_result = ad_caller.droid.telecomIsInCall()
        else:
            caller_state_result = verify_caller_func(log, ad_caller)
        if not caller_state_result:
            raise _CallSequenceException(
                "Caller %s not in correct state after %s seconds" %
                (ad_caller.serial, wait_time_in_call))

        if not hangup_call(log, ad_caller):
            raise _CallSequenceException("Error in Hanging-Up Call")

        log.info("Wait for voice mail indicator on callee.")
        try:
            event = ad_callee.ed.wait_for_event(
                EventMessageWaitingIndicatorChanged,
                _is_on_message_waiting_event_true)
            log.info(event)
        except Empty:
            ad_callee.log.warning("No expected event %s",
                                  EventMessageWaitingIndicatorChanged)
            raise _CallSequenceException("No expected event {}.".format(
                EventMessageWaitingIndicatorChanged))
        voice_mail_count_after = ad_callee.droid.telephonyGetVoiceMailCountForSubscription(
            subid_callee)
        ad_callee.log.info(
            "telephonyGetVoiceMailCount output - before: %s, after: %s",
            voice_mail_count_before, voice_mail_count_after)

        # voice_mail_count_after should:
        # either equals to (voice_mail_count_before + 1) [For ATT and SPT]
        # or equals to -1 [For TMO]
        # -1 means there are unread voice mail, but the count is unknown
        if not check_voice_mail_count(log, ad_callee, voice_mail_count_before,
                                      voice_mail_count_after):
            log.error("before and after voice mail count is not incorrect.")
            return False

    except _CallSequenceException as e:
        log.error(e)
        return False
    finally:
        ad_callee.droid.telephonyStopTrackingVoiceMailStateChangeForSubscription(
            subid_callee)
    return True


def call_voicemail_erase_all_pending_voicemail(log, ad):
    """Script for phone to erase all pending voice mail.
    This script only works for TMO and ATT and SPT currently.
    This script only works if phone have already set up voice mail options,
    and phone should disable password protection for voice mail.

    1. If phone don't have pending voice message, return True.
    2. Dial voice mail number.
        For TMO, the number is '123'
        For ATT, the number is phone's number
        For SPT, the number is phone's number
    3. Wait for voice mail connection setup.
    4. Wait for voice mail play pending voice message.
    5. Send DTMF to delete one message.
        The digit is '7'.
    6. Repeat steps 4 and 5 until voice mail server drop this call.
        (No pending message)
    6. Check telephonyGetVoiceMailCount result. it should be 0.

    Args:
        log: log object
        ad: android device object
    Returns:
        False if error happens. True is succeed.
    """
    log.info("Erase all pending voice mail.")
    count = ad.droid.telephonyGetVoiceMailCount()
    if count == 0:
        ad.log.info("No Pending voice mail.")
        return True
    if count == -1:
        ad.log.info("There is pending voice mail, but the count is unknown")
        count = MAX_SAVED_VOICE_MAIL
    else:
        ad.log.info("There are %s voicemails", count)

    voice_mail_number = get_voice_mail_number(log, ad)
    delete_digit = get_voice_mail_delete_digit(get_operator_name(log, ad))
    if not initiate_call(log, ad, voice_mail_number):
        log.error("Initiate call to voice mail failed.")
        return False
    time.sleep(WAIT_TIME_VOICE_MAIL_SERVER_RESPONSE)
    callId = ad.droid.telecomCallGetCallIds()[0]
    time.sleep(WAIT_TIME_VOICE_MAIL_SERVER_RESPONSE)
    while (is_phone_in_call(log, ad) and (count > 0)):
        ad.log.info("Press %s to delete voice mail.", delete_digit)
        ad.droid.telecomCallPlayDtmfTone(callId, delete_digit)
        ad.droid.telecomCallStopDtmfTone(callId)
        time.sleep(WAIT_TIME_VOICE_MAIL_SERVER_RESPONSE)
        count -= 1
    if is_phone_in_call(log, ad):
        hangup_call(log, ad)

    # wait for telephonyGetVoiceMailCount to update correct result
    remaining_time = MAX_WAIT_TIME_VOICE_MAIL_COUNT
    while ((remaining_time > 0)
           and (ad.droid.telephonyGetVoiceMailCount() != 0)):
        time.sleep(1)
        remaining_time -= 1
    current_voice_mail_count = ad.droid.telephonyGetVoiceMailCount()
    ad.log.info("telephonyGetVoiceMailCount: %s", current_voice_mail_count)
    return (current_voice_mail_count == 0)


def _is_on_message_waiting_event_true(event):
    """Private function to return if the received EventMessageWaitingIndicatorChanged
    event MessageWaitingIndicatorContainer.IS_MESSAGE_WAITING field is True.
    """
    return event['data'][MessageWaitingIndicatorContainer.IS_MESSAGE_WAITING]


def call_setup_teardown(log,
                        ad_caller,
                        ad_callee,
                        ad_hangup=None,
                        verify_caller_func=None,
                        verify_callee_func=None,
                        wait_time_in_call=WAIT_TIME_IN_CALL,
                        incall_ui_display=INCALL_UI_DISPLAY_FOREGROUND,
                        extra_sleep=0):
    """ Call process, including make a phone call from caller,
    accept from callee, and hang up. The call is on default voice subscription

    In call process, call from <droid_caller> to <droid_callee>,
    accept the call, (optional)then hang up from <droid_hangup>.

    Args:
        ad_caller: Caller Android Device Object.
        ad_callee: Callee Android Device Object.
        ad_hangup: Android Device Object end the phone call.
            Optional. Default value is None, and phone call will continue.
        verify_call_mode_caller: func_ptr to verify caller in correct mode
            Optional. Default is None
        verify_call_mode_caller: func_ptr to verify caller in correct mode
            Optional. Default is None
        incall_ui_display: after answer the call, bring in-call UI to foreground or
            background. Optional, default value is INCALL_UI_DISPLAY_FOREGROUND.
            if = INCALL_UI_DISPLAY_FOREGROUND, bring in-call UI to foreground.
            if = INCALL_UI_DISPLAY_BACKGROUND, bring in-call UI to background.
            else, do nothing.
        extra_sleep: for stress test only - b/64915613

    Returns:
        True if call process without any error.
        False if error happened.

    """
    subid_caller = get_outgoing_voice_sub_id(ad_caller)
    subid_callee = get_incoming_voice_sub_id(ad_callee)
    return call_setup_teardown_for_subscription(
        log, ad_caller, ad_callee, subid_caller, subid_callee, ad_hangup,
        verify_caller_func, verify_callee_func, wait_time_in_call,
        incall_ui_display, extra_sleep)


def call_setup_teardown_for_subscription(
        log,
        ad_caller,
        ad_callee,
        subid_caller,
        subid_callee,
        ad_hangup=None,
        verify_caller_func=None,
        verify_callee_func=None,
        wait_time_in_call=WAIT_TIME_IN_CALL,
        incall_ui_display=INCALL_UI_DISPLAY_FOREGROUND,
        extra_sleep=0):
    """ Call process, including make a phone call from caller,
    accept from callee, and hang up. The call is on specified subscription

    In call process, call from <droid_caller> to <droid_callee>,
    accept the call, (optional)then hang up from <droid_hangup>.

    Args:
        ad_caller: Caller Android Device Object.
        ad_callee: Callee Android Device Object.
        subid_caller: Caller subscription ID
        subid_callee: Callee subscription ID
        ad_hangup: Android Device Object end the phone call.
            Optional. Default value is None, and phone call will continue.
        verify_call_mode_caller: func_ptr to verify caller in correct mode
            Optional. Default is None
        verify_call_mode_caller: func_ptr to verify caller in correct mode
            Optional. Default is None
        incall_ui_display: after answer the call, bring in-call UI to foreground or
            background. Optional, default value is INCALL_UI_DISPLAY_FOREGROUND.
            if = INCALL_UI_DISPLAY_FOREGROUND, bring in-call UI to foreground.
            if = INCALL_UI_DISPLAY_BACKGROUND, bring in-call UI to background.
            else, do nothing.
        extra_sleep: for stress test only - b/64915613

    Returns:
        True if call process without any error.
        False if error happened.

    """
    CHECK_INTERVAL = 5
    begin_time = get_current_epoch_time()

    caller_number = ad_caller.cfg['subscription'][subid_caller]['phone_num']
    callee_number = ad_callee.cfg['subscription'][subid_callee]['phone_num']

    if not verify_caller_func:
        verify_caller_func = is_phone_in_call
    if not verify_callee_func:
        verify_callee_func = is_phone_in_call
    result = True
    msg = "Call from %s to %s" % (caller_number, callee_number)
    if ad_hangup:
        msg = "%s for duration of %s seconds" % (msg, wait_time_in_call)
    ad_caller.log.info(msg)

    for ad in (ad_caller, ad_callee):
        call_ids = ad.droid.telecomCallGetCallIds()
        setattr(ad, "call_ids", call_ids)
        ad.log.info("Before making call, existing phone calls %s", call_ids)
    try:
        if not initiate_call(
                log,
                ad_caller,
                callee_number,
                wait_time_betwn_call_initcheck=extra_sleep):
            ad_caller.log.error("Initiate call failed.")
            return False
        else:
            ad_caller.log.info("Caller initate call successfully")
        if not wait_and_answer_call_for_subscription(
                log,
                ad_callee,
                subid_callee,
                incoming_number=caller_number,
                caller=ad_caller,
                incall_ui_display=incall_ui_display):
            ad_callee.log.error("Answer call fail.")
            return False
        else:
            ad_callee.log.info("Callee answered the call successfully")

        for ad in (ad_caller, ad_callee):
            call_ids = ad.droid.telecomCallGetCallIds()
            new_call_id = list(set(call_ids) - set(ad.call_ids))[0]
            if not wait_for_in_call_active(ad, call_id=new_call_id):
                result = False
            if not ad.droid.telecomCallGetAudioState():
                ad.log.error("Audio is not in call state")
                result = False

        elapsed_time = 0
        while (elapsed_time < wait_time_in_call):
            CHECK_INTERVAL = min(CHECK_INTERVAL,
                                 wait_time_in_call - elapsed_time)
            time.sleep(CHECK_INTERVAL)
            elapsed_time += CHECK_INTERVAL
            time_message = "at <%s>/<%s> second." % (elapsed_time,
                                                     wait_time_in_call)
            for ad, call_func in [(ad_caller, verify_caller_func),
                                  (ad_callee, verify_callee_func)]:
                if not call_func(log, ad):
                    ad.log.error("NOT in correct %s state at %s",
                                 call_func.__name__, time_message)
                    result = False
                else:
                    ad.log.info("In correct %s state at %s",
                                call_func.__name__, time_message)
                if not ad.droid.telecomCallGetAudioState():
                    ad.log.error("Audio is not in call state at %s",
                                 time_message)
                    result = False
                if not result:
                    break
        if ad_hangup and not hangup_call(log, ad_hangup):
            ad_hangup.log.info("Failed to hang up the call")
            result = False
        return result
    finally:
        if not result:
            for ad in [ad_caller, ad_callee]:
                reasons = ad.search_logcat(
                    "qcril_qmi_voice_map_qmi_to_ril_last_call_failure_cause",
                    begin_time)
                if reasons:
                    ad.log.info(reasons[-1]["log_message"])
                try:
                    if ad.droid.telecomIsInCall():
                        ad.droid.telecomEndCall()
                except Exception as e:
                    log.error(str(e))


def phone_number_formatter(input_string, formatter=None):
    """Get expected format of input phone number string.

    Args:
        input_string: (string) input phone number.
            The input could be 10/11/12 digital, with or without " "/"-"/"."
        formatter: (int) expected format, this could be 7/10/11/12
            if formatter is 7: output string would be 7 digital number.
            if formatter is 10: output string would be 10 digital (standard) number.
            if formatter is 11: output string would be "1" + 10 digital number.
            if formatter is 12: output string would be "+1" + 10 digital number.

    Returns:
        If no error happen, return phone number in expected format.
        Else, return None.
    """
    if not input_string:
        return ""
    # make sure input_string is 10 digital
    # Remove white spaces, dashes, dots
    input_string = input_string.replace(" ", "").replace("-", "").replace(
        ".", "").lstrip("0")
    if not formatter:
        return input_string
    # Remove "1"  or "+1"from front
    if (len(input_string) == PHONE_NUMBER_STRING_FORMAT_11_DIGIT
            and input_string[0] == "1"):
        input_string = input_string[1:]
    elif (len(input_string) == PHONE_NUMBER_STRING_FORMAT_12_DIGIT
          and input_string[0:2] == "+1"):
        input_string = input_string[2:]
    elif (len(input_string) == PHONE_NUMBER_STRING_FORMAT_7_DIGIT
          and formatter == PHONE_NUMBER_STRING_FORMAT_7_DIGIT):
        return input_string
    elif len(input_string) != PHONE_NUMBER_STRING_FORMAT_10_DIGIT:
        return None
    # change input_string according to format
    if formatter == PHONE_NUMBER_STRING_FORMAT_12_DIGIT:
        input_string = "+1" + input_string
    elif formatter == PHONE_NUMBER_STRING_FORMAT_11_DIGIT:
        input_string = "1" + input_string
    elif formatter == PHONE_NUMBER_STRING_FORMAT_10_DIGIT:
        input_string = input_string
    elif formatter == PHONE_NUMBER_STRING_FORMAT_7_DIGIT:
        input_string = input_string[3:]
    else:
        return None
    return input_string


def get_internet_connection_type(log, ad):
    """Get current active connection type name.

    Args:
        log: Log object.
        ad: Android Device Object.
    Returns:
        current active connection type name.
    """
    if not ad.droid.connectivityNetworkIsConnected():
        return 'none'
    return connection_type_from_type_string(
        ad.droid.connectivityNetworkGetActiveConnectionTypeName())


def verify_http_connection(log,
                           ad,
                           url="www.google.com",
                           retry=5,
                           retry_interval=15,
                           expected_state=True):
    """Make ping request and return status.

    Args:
        log: log object
        ad: Android Device Object.
        url: Optional. The ping request will be made to this URL.
            Default Value is "http://www.google.com/".

    """
    for i in range(0, retry + 1):
        # b/18899134 httpPing will hang
        #try:
        #    http_response = ad.droid.httpPing(url)
        #except:
        #    http_response = None
        # If httpPing failed, it may return {} (if phone just turn off APM) or
        # None (regular fail)
        state = ad.droid.pingHost(url)
        ad.log.info("Connection to %s is %s", url, state)
        if expected_state == state:
            ad.log.info("Verify Internet connection state is %s succeeded",
                        str(expected_state))
            return True
        if i < retry:
            ad.log.info(
                "Verify Internet connection state=%s failed. Try again",
                str(expected_state))
            time.sleep(retry_interval)
    ad.log.info("Verify Internet state=%s failed after %s second",
                expected_state, i * retry_interval)
    return False


def _generate_file_directory_and_file_name(url, out_path):
    file_name = url.split("/")[-1]
    if not out_path:
        file_directory = "/sdcard/Download/"
    elif not out_path.endswith("/"):
        file_directory, file_name = os.path.split(out_path)
    else:
        file_directory = out_path
    return file_directory, file_name


def _check_file_existance(ad, file_path, expected_file_size=None):
    """Check file existance by file_path. If expected_file_size
       is provided, then also check if the file meet the file size requirement.
    """
    out = None
    try:
        out = ad.adb.shell('stat -c "%%s" %s' % file_path)
    except AdbError:
        pass
    # Handle some old version adb returns error message "No such" into std_out
    if out and "No such" not in out:
        if expected_file_size:
            file_size = int(out)
            if file_size >= expected_file_size:
                ad.log.info("File %s of size %s exists", file_path, file_size)
                return True
            else:
                ad.log.info("File %s is of size %s, does not meet expected %s",
                            file_path, file_size, expected_file_size)
                return False
        else:
            ad.log.info("File %s exists", file_path)
            return True
    else:
        ad.log.info("File %s does not exist.", file_path)
        return False


def check_curl_availability(ad):
    if not hasattr(ad, "curl_capable"):
        try:
            out = ad.adb.shell("/data/curl --version")
            if not out or "not found" in out:
                setattr(ad, "curl_capable", False)
                ad.log.info("curl is unavailable, use chrome to download file")
            else:
                setattr(ad, "curl_capable", True)
        except Exception:
            setattr(ad, "curl_capable", False)
            ad.log.info("curl is unavailable, use chrome to download file")
    return ad.curl_capable


def active_file_download_task(log, ad, file_name="5MB", method="chrome"):
    # files available for download on the same website:
    # 1GB.zip, 512MB.zip, 200MB.zip, 50MB.zip, 20MB.zip, 10MB.zip, 5MB.zip
    # download file by adb command, as phone call will use sl4a
    file_map_dict = {
        '5MB': 5000000,
        '10MB': 10000000,
        '20MB': 20000000,
        '50MB': 50000000,
        '100MB': 100000000,
        '200MB': 200000000,
        '512MB': 512000000
    }
    file_size = file_map_dict.get(file_name)
    if not file_size:
        log.warning("file_name %s for download is not available", file_name)
        return False
    timeout = min(max(file_size / 100000, 600), 3600)
    output_path = "/sdcard/Download/" + file_name + ".zip"
    url = "http://ipv4.download.thinkbroadband.com/" + file_name + ".zip"
    if method == "sl4a":
        return (http_file_download_by_sl4a, (ad, url, output_path, file_size,
                                             True, timeout))
    if method == "curl" and check_curl_availability(ad):
        url = "http://146.148.91.8/download/" + file_name + ".zip"
        return (http_file_download_by_curl, (ad, url, output_path, file_size,
                                             True, timeout))
    elif method == "sl4a":
        return (http_file_download_by_sl4a, (ad, url, output_path, file_size,
                                             True, timeout))
    else:
        return (http_file_download_by_chrome, (ad, url, file_size, True,
                                               timeout))


def active_file_download_test(log, ad, file_name="5MB", method="chrome"):
    task = active_file_download_task(log, ad, file_name, method=method)
    return task[0](*task[1])


def verify_internet_connection(log, ad, retries=1):
    """Verify internet connection by ping test.

    Args:
        log: log object
        ad: Android Device Object.

    """
    for i in range(retries):
        ad.log.info("Verify internet connection - attempt %d", i + 1)
        dest_to_ping = ["www.google.com", "www.amazon.com", "54.230.144.105"]
        for dest in dest_to_ping:
            result = adb_shell_ping(
                ad, count=5, timeout=60, loss_tolerance=40, dest_ip=dest)
            if result:
                return True
    return False


def iperf_test_by_adb(log,
                      ad,
                      iperf_server,
                      port_num=None,
                      reverse=False,
                      timeout=180,
                      limit_rate=None,
                      omit=10,
                      ipv6=False,
                      rate_dict=None,
                      blocking=True,
                      log_file_path=None):
    """Iperf test by adb.

    Args:
        log: log object
        ad: Android Device Object.
        iperf_Server: The iperf host url".
        port_num: TCP/UDP server port
        timeout: timeout for file download to complete.
        limit_rate: iperf bandwidth option. None by default
        omit: the omit option provided in iperf command.
    """
    iperf_option = "-t %s -O %s -J" % (timeout, omit)
    if limit_rate: iperf_option += " -b %s" % limit_rate
    if port_num: iperf_option += " -p %s" % port_num
    if ipv6: iperf_option += " -6"
    if reverse: iperf_option += " -R"
    try:
        if log_file_path:
            ad.adb.shell("rm %s" % log_file_path, ignore_status=True)
        ad.log.info("Running adb iperf test with server %s", iperf_server)
        if not blocking:
            ad.run_iperf_client_nb(
                iperf_server,
                iperf_option,
                timeout=timeout + 60,
                log_file_path=log_file_path)
            return True
        result, data = ad.run_iperf_client(
            iperf_server, iperf_option, timeout=timeout + 60)
        ad.log.info("Iperf test result with server %s is %s", iperf_server,
                    result)
        if result:
            data_json = json.loads(''.join(data))
            tx_rate = data_json['end']['sum_sent']['bits_per_second']
            rx_rate = data_json['end']['sum_received']['bits_per_second']
            ad.log.info(
                'iPerf3 upload speed is %sbps, download speed is %sbps',
                tx_rate, rx_rate)
            if rate_dict is not None:
                rate_dict["Uplink"] = tx_rate
                rate_dict["Downlink"] = rx_rate
        return result
    except Exception as e:
        ad.log.warning("Fail to run iperf test with exception %s", e)
        return False


def http_file_download_by_curl(ad,
                               url,
                               out_path=None,
                               expected_file_size=None,
                               remove_file_after_check=True,
                               timeout=3600,
                               limit_rate=None,
                               retry=3):
    """Download http file by adb curl.

    Args:
        ad: Android Device Object.
        url: The url that file to be downloaded from".
        out_path: Optional. Where to download file to.
                  out_path is /sdcard/Download/ by default.
        expected_file_size: Optional. Provided if checking the download file meet
                            expected file size in unit of byte.
        remove_file_after_check: Whether to remove the downloaded file after
                                 check.
        timeout: timeout for file download to complete.
        limit_rate: download rate in bps. None, if do not apply rate limit.
        retry: the retry request times provided in curl command.
    """
    file_directory, file_name = _generate_file_directory_and_file_name(
        url, out_path)
    file_path = os.path.join(file_directory, file_name)
    curl_cmd = "/data/curl"
    if limit_rate:
        curl_cmd += " --limit-rate %s" % limit_rate
    if retry:
        curl_cmd += " --retry %s" % retry
    curl_cmd += " --url %s > %s" % (url, file_path)
    accounting_apk = "com.android.server.telecom"  #"com.quicinc.cne.CNEService"
    result = True
    try:
        data_accounting = {
            "mobile_rx_bytes":
            ad.droid.getMobileRxBytes(),
            "subscriber_mobile_data_usage":
            get_mobile_data_usage(ad, None, None),
            "curl_mobile_data_usage":
            get_mobile_data_usage(ad, None, accounting_apk)
        }
        ad.log.info("Before downloading: %s", data_accounting)
        ad.log.info("Download %s to %s by adb shell command %s", url,
                    file_path, curl_cmd)
        ad.adb.shell(curl_cmd, timeout=timeout)
        if _check_file_existance(ad, file_path, expected_file_size):
            ad.log.info("%s is downloaded to %s successfully", url, file_path)
            new_data_accounting = {
                "mobile_rx_bytes":
                ad.droid.getMobileRxBytes(),
                "subscriber_mobile_data_usage":
                get_mobile_data_usage(ad, None, None),
                "curl_mobile_data_usage":
                get_mobile_data_usage(ad, None, accounting_apk)
            }
            ad.log.info("After downloading: %s", new_data_accounting)
            accounting_diff = {
                key: value - data_accounting[key]
                for key, value in new_data_accounting.items()
            }
            ad.log.info("Data accounting difference: %s", accounting_diff)
            if getattr(ad, "on_mobile_data", False):
                for key, value in accounting_diff.items():
                    if value < expected_file_size:
                        ad.log.warning("%s diff is %s less than %s", key,
                                       value, expected_file_size)
                        ad.data_accounting["%s_failure" % key] += 1
            else:
                for key, value in accounting_diff.items():
                    if value >= expected_file_size:
                        ad.log.error("%s diff is %s. File download is "
                                     "consuming mobile data", key, value)
                        result = False
            ad.log.info("data_accounting_failure: %s", dict(
                ad.data_accounting))
            return result
        else:
            ad.log.warning("Fail to download %s", url)
            return False
    except Exception as e:
        ad.log.warning("Download %s failed with exception %s", url, e)
        return False
    finally:
        if remove_file_after_check:
            ad.log.info("Remove the downloaded file %s", file_path)
            ad.adb.shell("rm %s" % file_path, ignore_status=True)


def open_url_by_adb(ad, url):
    ad.adb.shell('am start -a android.intent.action.VIEW -d "%s"' % url)


def http_file_download_by_chrome(ad,
                                 url,
                                 expected_file_size=None,
                                 remove_file_after_check=True,
                                 timeout=3600):
    """Download http file by chrome.

    Args:
        ad: Android Device Object.
        url: The url that file to be downloaded from".
        expected_file_size: Optional. Provided if checking the download file meet
                            expected file size in unit of byte.
        remove_file_after_check: Whether to remove the downloaded file after
                                 check.
        timeout: timeout for file download to complete.
    """
    chrome_apk = "com.android.chrome"
    file_directory, file_name = _generate_file_directory_and_file_name(
        url, "/sdcard/Download/")
    file_path = os.path.join(file_directory, file_name)
    # Remove pre-existing file
    ad.force_stop_apk(chrome_apk)
    file_to_be_delete = os.path.join(file_directory, "*%s*" % file_name)
    ad.adb.shell("rm -f %s" % file_to_be_delete)
    ad.adb.shell("rm -rf /sdcard/Download/.*")
    ad.adb.shell("rm -f /sdcard/Download/.*")
    data_accounting = {
        "total_rx_bytes": ad.droid.getTotalRxBytes(),
        "mobile_rx_bytes": ad.droid.getMobileRxBytes(),
        "subscriber_mobile_data_usage": get_mobile_data_usage(ad, None, None),
        "chrome_mobile_data_usage": get_mobile_data_usage(
            ad, None, chrome_apk)
    }
    ad.log.info("Before downloading: %s", data_accounting)
    ad.ensure_screen_on()
    ad.log.info("Download %s with timeout %s", url, timeout)
    open_url_by_adb(ad, url)
    elapse_time = 0
    result = True
    while elapse_time < timeout:
        time.sleep(30)
        if _check_file_existance(ad, file_path, expected_file_size):
            ad.log.info("%s is downloaded successfully", url)
            if remove_file_after_check:
                ad.log.info("Remove the downloaded file %s", file_path)
                ad.adb.shell("rm -f %s" % file_to_be_delete)
                ad.adb.shell("rm -rf /sdcard/Download/.*")
                ad.adb.shell("rm -f /sdcard/Download/.*")
            #time.sleep(30)
            new_data_accounting = {
                "mobile_rx_bytes":
                ad.droid.getMobileRxBytes(),
                "subscriber_mobile_data_usage":
                get_mobile_data_usage(ad, None, None),
                "chrome_mobile_data_usage":
                get_mobile_data_usage(ad, None, chrome_apk)
            }
            ad.log.info("After downloading: %s", new_data_accounting)
            accounting_diff = {
                key: value - data_accounting[key]
                for key, value in new_data_accounting.items()
            }
            ad.log.info("Data accounting difference: %s", accounting_diff)
            if getattr(ad, "on_mobile_data", False):
                for key, value in accounting_diff.items():
                    if value < expected_file_size:
                        ad.log.warning("%s diff is %s less than %s", key,
                                       value, expected_file_size)
                        ad.data_accounting["%s_failure" % key] += 1
            else:
                for key, value in accounting_diff.items():
                    if value >= expected_file_size:
                        ad.log.error("%s diff is %s. File download is "
                                     "consuming mobile data", key, value)
                        result = False
            return result
        elif _check_file_existance(ad, "%s.crdownload" % file_path):
            ad.log.info("Chrome is downloading %s", url)
        elif elapse_time < 60:
            # download not started, retry download wit chrome again
            open_url_by_adb(ad, url)
        else:
            ad.log.error("Unable to download file from %s", url)
            break
        elapse_time += 30
    ad.log.warning("Fail to download file from %s", url)
    ad.force_stop_apk("com.android.chrome")
    ad.adb.shell("rm -f %s" % file_to_be_delete)
    ad.adb.shell("rm -rf /sdcard/Download/.*")
    ad.adb.shell("rm -f /sdcard/Download/.*")
    return False


def http_file_download_by_sl4a(ad,
                               url,
                               out_path=None,
                               expected_file_size=None,
                               remove_file_after_check=True,
                               timeout=300):
    """Download http file by sl4a RPC call.

    Args:
        ad: Android Device Object.
        url: The url that file to be downloaded from".
        out_path: Optional. Where to download file to.
                  out_path is /sdcard/Download/ by default.
        expected_file_size: Optional. Provided if checking the download file meet
                            expected file size in unit of byte.
        remove_file_after_check: Whether to remove the downloaded file after
                                 check.
        timeout: timeout for file download to complete.
    """
    file_folder, file_name = _generate_file_directory_and_file_name(
        url, out_path)
    file_path = os.path.join(file_folder, file_name)
    ad.adb.shell("rm -f %s" % file_path)
    accounting_apk = SL4A_APK_NAME
    result = True
    try:
        if not getattr(ad, "downloading_droid", None):
            ad.downloading_droid, ad.downloading_ed = ad.get_droid()
            ad.downloading_ed.start()
        else:
            try:
                if not ad.downloading_droid.is_live:
                    ad.downloading_droid, ad.downloading_ed = ad.get_droid()
                    ad.downloading_ed.start()
            except Exception as e:
                ad.log.info(e)
                ad.downloading_droid, ad.downloading_ed = ad.get_droid()
                ad.downloading_ed.start()
        data_accounting = {
            "mobile_rx_bytes":
            ad.droid.getMobileRxBytes(),
            "subscriber_mobile_data_usage":
            get_mobile_data_usage(ad, None, None),
            "sl4a_mobile_data_usage":
            get_mobile_data_usage(ad, None, accounting_apk)
        }
        ad.log.info("Before downloading: %s", data_accounting)
        ad.log.info("Download file from %s to %s by sl4a RPC call", url,
                    file_path)
        try:
            ad.downloading_droid.httpDownloadFile(
                url, file_path, timeout=timeout)
        except Exception as e:
            ad.log.warning("SL4A file download error: %s", e)
            return False
        if _check_file_existance(ad, file_path, expected_file_size):
            ad.log.info("%s is downloaded successfully", url)
            new_data_accounting = {
                "mobile_rx_bytes":
                ad.droid.getMobileRxBytes(),
                "subscriber_mobile_data_usage":
                get_mobile_data_usage(ad, None, None),
                "sl4a_mobile_data_usage":
                get_mobile_data_usage(ad, None, accounting_apk)
            }
            ad.log.info("After downloading: %s", new_data_accounting)
            accounting_diff = {
                key: value - data_accounting[key]
                for key, value in new_data_accounting.items()
            }
            ad.log.info("Data accounting difference: %s", accounting_diff)
            if getattr(ad, "on_mobile_data", False):
                for key, value in accounting_diff.items():
                    if value < expected_file_size:
                        ad.log.warning("%s diff is %s less than %s", key,
                                       value, expected_file_size)
                        ad.data_accounting["%s_failure"] += 1
            else:
                for key, value in accounting_diff.items():
                    if value >= expected_file_size:
                        ad.log.error("%s diff is %s. File download is "
                                     "consuming mobile data", key, value)
                        result = False
            return result
        else:
            ad.log.warning("Fail to download %s", url)
            return False
    except Exception as e:
        ad.log.error("Download %s failed with exception %s", url, e)
        raise
    finally:
        if remove_file_after_check:
            ad.log.info("Remove the downloaded file %s", file_path)
            ad.adb.shell("rm %s" % file_path, ignore_status=True)


def get_mobile_data_usage(ad, subscriber_id=None, apk=None):
    if not subscriber_id:
        subscriber_id = ad.droid.telephonyGetSubscriberId()
    if not getattr(ad, "data_metering_begin_time", None) or not getattr(
            ad, "data_metering_end_time", None):
        current_time = int(time.time() * 1000)
        setattr(ad, "data_metering_begin_time",
                current_time - 24 * 60 * 60 * 1000)
        setattr(ad, "data_metering_end_time",
                current_time + 30 * 24 * 60 * 60 * 1000)
    begin_time = ad.data_metering_begin_time
    end_time = ad.data_metering_end_time
    if apk:
        uid = ad.get_apk_uid(apk)
        try:
            usage = ad.droid.connectivityQueryDetailsForUid(
                TYPE_MOBILE, subscriber_id, begin_time, end_time, uid)
        except Exception:
            usage = ad.droid.connectivityQueryDetailsForUid(
                subscriber_id, begin_time, end_time, uid)
        ad.log.debug("The mobile data usage for apk %s is %s", apk, usage)
    else:
        try:
            usage = ad.droid.connectivityQuerySummaryForDevice(
                TYPE_MOBILE, subscriber_id, begin_time, end_time)
        except Exception:
            usage = ad.droid.connectivityQuerySummaryForDevice(
                subscriber_id, begin_time, end_time)
        ad.log.debug("The mobile data usage for subscriber is %s", usage)
    return usage


def set_mobile_data_usage_limit(ad, limit, subscriber_id=None):
    if not subscriber_id:
        subscriber_id = ad.droid.telephonyGetSubscriberId()
    ad.log.info("Set subscriber mobile data usage limit to %s", limit)
    ad.droid.connectivitySetDataUsageLimit(subscriber_id, str(limit))


def remove_mobile_data_usage_limit(ad, subscriber_id=None):
    if not subscriber_id:
        subscriber_id = ad.droid.telephonyGetSubscriberId()
    ad.log.info("Remove subscriber mobile data usage limit")
    ad.droid.connectivitySetDataUsageLimit(subscriber_id, "-1")


def trigger_modem_crash(ad, timeout=120):
    cmd = "echo restart > /sys/kernel/debug/msm_subsys/modem"
    ad.log.info("Triggering Modem Crash from kernel using adb command %s", cmd)
    ad.adb.shell(cmd)
    time.sleep(timeout)
    return True


def trigger_modem_crash_by_modem(ad, timeout=120):
    begin_time = get_current_epoch_time()
    ad.adb.shell(
        "setprop persist.vendor.sys.modem.diag.mdlog false", ignore_status=True)
    # Legacy pixels use persist.sys.modem.diag.mdlog.
    ad.adb.shell(
        "setprop persist.sys.modem.diag.mdlog false", ignore_status=True)
    stop_qxdm_logger(ad)
    cmd = ('am instrument -w -e request "4b 25 03 00" '
           '"com.google.mdstest/com.google.mdstest.instrument.'
           'ModemCommandInstrumentation"')
    ad.log.info("Crash modem by %s", cmd)
    ad.adb.shell(cmd, ignore_status=True)
    time.sleep(timeout)  # sleep time for sl4a stability
    reasons = ad.search_logcat("modem subsystem failure reason", begin_time)
    if reasons:
        ad.log.info("Modem crash is triggered successfully")
        ad.log.info(reasons[-1]["log_message"])
        return True
    else:
        ad.log.info("Modem crash is not triggered successfully")
        return False


def _connection_state_change(_event, target_state, connection_type):
    if connection_type:
        if 'TypeName' not in _event['data']:
            return False
        connection_type_string_in_event = _event['data']['TypeName']
        cur_type = connection_type_from_type_string(
            connection_type_string_in_event)
        if cur_type != connection_type:
            log.info(
                "_connection_state_change expect: %s, received: %s <type %s>",
                connection_type, connection_type_string_in_event, cur_type)
            return False

    if 'isConnected' in _event['data'] and _event['data']['isConnected'] == target_state:
        return True
    return False


def wait_for_cell_data_connection(
        log, ad, state, timeout_value=MAX_WAIT_TIME_CONNECTION_STATE_UPDATE):
    """Wait for data connection status to be expected value for default
       data subscription.

    Wait for the data connection status to be DATA_STATE_CONNECTED
        or DATA_STATE_DISCONNECTED.

    Args:
        log: Log object.
        ad: Android Device Object.
        state: Expected status: True or False.
            If True, it will wait for status to be DATA_STATE_CONNECTED.
            If False, it will wait for status ti be DATA_STATE_DISCONNECTED.
        timeout_value: wait for cell data timeout value.
            This is optional, default value is MAX_WAIT_TIME_CONNECTION_STATE_UPDATE

    Returns:
        True if success.
        False if failed.
    """
    sub_id = get_default_data_sub_id(ad)
    return wait_for_cell_data_connection_for_subscription(
        log, ad, sub_id, state, timeout_value)


def _is_data_connection_state_match(log, ad, expected_data_connection_state):
    return (expected_data_connection_state ==
            ad.droid.telephonyGetDataConnectionState())


def _is_network_connected_state_match(log, ad,
                                      expected_network_connected_state):
    return (expected_network_connected_state ==
            ad.droid.connectivityNetworkIsConnected())


def wait_for_cell_data_connection_for_subscription(
        log,
        ad,
        sub_id,
        state,
        timeout_value=MAX_WAIT_TIME_CONNECTION_STATE_UPDATE):
    """Wait for data connection status to be expected value for specified
       subscrption id.

    Wait for the data connection status to be DATA_STATE_CONNECTED
        or DATA_STATE_DISCONNECTED.

    Args:
        log: Log object.
        ad: Android Device Object.
        sub_id: subscription Id
        state: Expected status: True or False.
            If True, it will wait for status to be DATA_STATE_CONNECTED.
            If False, it will wait for status ti be DATA_STATE_DISCONNECTED.
        timeout_value: wait for cell data timeout value.
            This is optional, default value is MAX_WAIT_TIME_CONNECTION_STATE_UPDATE

    Returns:
        True if success.
        False if failed.
    """
    state_str = {
        True: DATA_STATE_CONNECTED,
        False: DATA_STATE_DISCONNECTED
    }[state]

    data_state = ad.droid.telephonyGetDataConnectionState()
    if not state and ad.droid.telephonyGetDataConnectionState() == state_str:
        return True
    ad.ed.clear_events(EventDataConnectionStateChanged)
    ad.droid.telephonyStartTrackingDataConnectionStateChangeForSubscription(
        sub_id)
    ad.droid.connectivityStartTrackingConnectivityStateChange()
    try:
        # TODO: b/26293147 There is no framework API to get data connection
        # state by sub id
        data_state = ad.droid.telephonyGetDataConnectionState()
        if data_state == state_str:
            return _wait_for_nw_data_connection(
                log, ad, state, NETWORK_CONNECTION_TYPE_CELL, timeout_value)

        try:
            event = ad.ed.wait_for_event(
                EventDataConnectionStateChanged,
                is_event_match,
                timeout=timeout_value,
                field=DataConnectionStateContainer.DATA_CONNECTION_STATE,
                value=state_str)
        except Empty:
            ad.log.info("No expected event EventDataConnectionStateChanged %s",
                        state_str)

        # TODO: Wait for <MAX_WAIT_TIME_CONNECTION_STATE_UPDATE> seconds for
        # data connection state.
        # Otherwise, the network state will not be correct.
        # The bug is tracked here: b/20921915

        # Previously we use _is_data_connection_state_match,
        # but telephonyGetDataConnectionState sometimes return wrong value.
        # The bug is tracked here: b/22612607
        # So we use _is_network_connected_state_match.

        if _wait_for_droid_in_state(log, ad, timeout_value,
                                    _is_network_connected_state_match, state):
            return _wait_for_nw_data_connection(
                log, ad, state, NETWORK_CONNECTION_TYPE_CELL, timeout_value)
        else:
            return False

    finally:
        ad.droid.telephonyStopTrackingDataConnectionStateChangeForSubscription(
            sub_id)


def wait_for_wifi_data_connection(
        log, ad, state, timeout_value=MAX_WAIT_TIME_CONNECTION_STATE_UPDATE):
    """Wait for data connection status to be expected value and connection is by WiFi.

    Args:
        log: Log object.
        ad: Android Device Object.
        state: Expected status: True or False.
            If True, it will wait for status to be DATA_STATE_CONNECTED.
            If False, it will wait for status ti be DATA_STATE_DISCONNECTED.
        timeout_value: wait for network data timeout value.
            This is optional, default value is MAX_WAIT_TIME_NW_SELECTION

    Returns:
        True if success.
        False if failed.
    """
    ad.log.info("wait_for_wifi_data_connection")
    return _wait_for_nw_data_connection(
        log, ad, state, NETWORK_CONNECTION_TYPE_WIFI, timeout_value)


def wait_for_data_connection(
        log, ad, state, timeout_value=MAX_WAIT_TIME_CONNECTION_STATE_UPDATE):
    """Wait for data connection status to be expected value.

    Wait for the data connection status to be DATA_STATE_CONNECTED
        or DATA_STATE_DISCONNECTED.

    Args:
        log: Log object.
        ad: Android Device Object.
        state: Expected status: True or False.
            If True, it will wait for status to be DATA_STATE_CONNECTED.
            If False, it will wait for status ti be DATA_STATE_DISCONNECTED.
        timeout_value: wait for network data timeout value.
            This is optional, default value is MAX_WAIT_TIME_CONNECTION_STATE_UPDATE

    Returns:
        True if success.
        False if failed.
    """
    return _wait_for_nw_data_connection(log, ad, state, None, timeout_value)


def _wait_for_nw_data_connection(
        log,
        ad,
        is_connected,
        connection_type=None,
        timeout_value=MAX_WAIT_TIME_CONNECTION_STATE_UPDATE):
    """Wait for data connection status to be expected value.

    Wait for the data connection status to be DATA_STATE_CONNECTED
        or DATA_STATE_DISCONNECTED.

    Args:
        log: Log object.
        ad: Android Device Object.
        is_connected: Expected connection status: True or False.
            If True, it will wait for status to be DATA_STATE_CONNECTED.
            If False, it will wait for status ti be DATA_STATE_DISCONNECTED.
        connection_type: expected connection type.
            This is optional, if it is None, then any connection type will return True.
        timeout_value: wait for network data timeout value.
            This is optional, default value is MAX_WAIT_TIME_CONNECTION_STATE_UPDATE

    Returns:
        True if success.
        False if failed.
    """
    ad.ed.clear_events(EventConnectivityChanged)
    ad.droid.connectivityStartTrackingConnectivityStateChange()
    try:
        cur_data_connection_state = ad.droid.connectivityNetworkIsConnected()
        if is_connected == cur_data_connection_state:
            current_type = get_internet_connection_type(log, ad)
            ad.log.info("current data connection type: %s", current_type)
            if not connection_type:
                return True
            else:
                if not is_connected and current_type != connection_type:
                    ad.log.info("data connection not on %s!", connection_type)
                    return True
                elif is_connected and current_type == connection_type:
                    ad.log.info("data connection on %s as expected",
                                connection_type)
                    return True
        else:
            ad.log.info("current data connection state: %s target: %s",
                        cur_data_connection_state, is_connected)

        try:
            event = ad.ed.wait_for_event(
                EventConnectivityChanged, _connection_state_change,
                timeout_value, is_connected, connection_type)
            ad.log.info("received event: %s", event)
        except Empty:
            pass

        log.info(
            "_wait_for_nw_data_connection: check connection after wait event.")
        # TODO: Wait for <MAX_WAIT_TIME_CONNECTION_STATE_UPDATE> seconds for
        # data connection state.
        # Otherwise, the network state will not be correct.
        # The bug is tracked here: b/20921915
        if _wait_for_droid_in_state(log, ad, timeout_value,
                                    _is_network_connected_state_match,
                                    is_connected):
            current_type = get_internet_connection_type(log, ad)
            ad.log.info("current data connection type: %s", current_type)
            if not connection_type:
                return True
            else:
                if not is_connected and current_type != connection_type:
                    ad.log.info("data connection not on %s", connection_type)
                    return True
                elif is_connected and current_type == connection_type:
                    ad.log.info("after event wait, data connection on %s",
                                connection_type)
                    return True
                else:
                    return False
        else:
            return False
    except Exception as e:
        ad.log.error("Exception error %s", str(e))
        return False
    finally:
        ad.droid.connectivityStopTrackingConnectivityStateChange()


def get_cell_data_roaming_state_by_adb(ad):
    """Get Cell Data Roaming state. True for enabled, False for disabled"""
    adb_str = {"1": True, "0": False}
    out = ad.adb.shell("settings get global data_roaming")
    return adb_str[out]


def get_cell_data_roaming_state_by_adb(ad):
    """Get Cell Data Roaming state. True for enabled, False for disabled"""
    state_mapping = {"1": True, "0": False}
    return state_mapping[ad.adb.shell("settings get global data_roaming")]


def set_cell_data_roaming_state_by_adb(ad, state):
    """Set Cell Data Roaming state."""
    state_mapping = {True: "1", False: "0"}
    ad.log.info("Set data roaming to %s", state)
    ad.adb.shell("settings put global data_roaming %s" % state_mapping[state])


def toggle_cell_data_roaming(ad, state):
    """Enable cell data roaming for default data subscription.

    Wait for the data roaming status to be DATA_STATE_CONNECTED
        or DATA_STATE_DISCONNECTED.

    Args:
        log: Log object.
        ad: Android Device Object.
        state: True or False for enable or disable cell data roaming.

    Returns:
        True if success.
        False if failed.
    """
    state_int = {True: DATA_ROAMING_ENABLE, False: DATA_ROAMING_DISABLE}[state]
    action_str = {True: "Enable", False: "Disable"}[state]
    if ad.droid.connectivityCheckDataRoamingMode() == state:
        ad.log.info("Data roaming is already in state %s", state)
        return True
    if not ad.droid.connectivitySetDataRoaming(state_int):
        ad.error.info("Fail to config data roaming into state %s", state)
        return False
    if ad.droid.connectivityCheckDataRoamingMode() == state:
        ad.log.info("Data roaming is configured into state %s", state)
        return True
    else:
        ad.log.error("Data roaming is not configured into state %s", state)
        return False


def verify_incall_state(log, ads, expected_status):
    """Verify phones in incall state or not.

    Verify if all phones in the array <ads> are in <expected_status>.

    Args:
        log: Log object.
        ads: Array of Android Device Object. All droid in this array will be tested.
        expected_status: If True, verify all Phones in incall state.
            If False, verify all Phones not in incall state.

    """
    result = True
    for ad in ads:
        if ad.droid.telecomIsInCall() is not expected_status:
            ad.log.error("InCall status:%s, expected:%s",
                         ad.droid.telecomIsInCall(), expected_status)
            result = False
    return result


def verify_active_call_number(log, ad, expected_number):
    """Verify the number of current active call.

    Verify if the number of current active call in <ad> is
        equal to <expected_number>.

    Args:
        ad: Android Device Object.
        expected_number: Expected active call number.
    """
    calls = ad.droid.telecomCallGetCallIds()
    if calls is None:
        actual_number = 0
    else:
        actual_number = len(calls)
    if actual_number != expected_number:
        ad.log.error("Active Call number is %s, expecting", actual_number,
                     expected_number)
        return False
    return True


def num_active_calls(log, ad):
    """Get the count of current active calls.

    Args:
        log: Log object.
        ad: Android Device Object.

    Returns:
        Count of current active calls.
    """
    calls = ad.droid.telecomCallGetCallIds()
    return len(calls) if calls else 0


def toggle_volte(log, ad, new_state=None):
    """Toggle enable/disable VoLTE for default voice subscription.

    Args:
        ad: Android device object.
        new_state: VoLTE mode state to set to.
            True for enable, False for disable.
            If None, opposite of the current state.

    Raises:
        TelTestUtilsError if platform does not support VoLTE.
    """
    return toggle_volte_for_subscription(
        log, ad, get_outgoing_voice_sub_id(ad), new_state)


def toggle_volte_for_subscription(log, ad, sub_id, new_state=None):
    """Toggle enable/disable VoLTE for specified voice subscription.

    Args:
        ad: Android device object.
        sub_id: subscription ID
        new_state: VoLTE mode state to set to.
            True for enable, False for disable.
            If None, opposite of the current state.

    Raises:
        TelTestUtilsError if platform does not support VoLTE.
    """
    # TODO: b/26293960 No framework API available to set IMS by SubId.
    if not ad.droid.imsIsEnhanced4gLteModeSettingEnabledByPlatform():
        ad.log.info("VoLTE not supported by platform.")
        raise TelTestUtilsError(
            "VoLTE not supported by platform %s." % ad.serial)
    current_state = ad.droid.imsIsEnhanced4gLteModeSettingEnabledByUser()
    if new_state is None:
        new_state = not current_state
    if new_state != current_state:
        ad.droid.imsSetEnhanced4gMode(new_state)
    return True


def set_wfc_mode(log, ad, wfc_mode):
    """Set WFC enable/disable and mode.

    Args:
        log: Log object
        ad: Android device object.
        wfc_mode: WFC mode to set to.
            Valid mode includes: WFC_MODE_WIFI_ONLY, WFC_MODE_CELLULAR_PREFERRED,
            WFC_MODE_WIFI_PREFERRED, WFC_MODE_DISABLED.

    Returns:
        True if success. False if ad does not support WFC or error happened.
    """
    try:
        ad.log.info("Set wfc mode to %s", wfc_mode)
        if not ad.droid.imsIsWfcEnabledByPlatform():
            if wfc_mode == WFC_MODE_DISABLED:
                return True
            else:
                ad.log.error("WFC not supported by platform.")
                return False
        ad.droid.imsSetWfcMode(wfc_mode)
        mode = ad.droid.imsGetWfcMode()
        if mode != wfc_mode:
            ad.log.error("WFC mode is %s, not in %s", mode, wfc_mode)
            return False
    except Exception as e:
        log.error(e)
        return False
    return True


def toggle_video_calling(log, ad, new_state=None):
    """Toggle enable/disable Video calling for default voice subscription.

    Args:
        ad: Android device object.
        new_state: Video mode state to set to.
            True for enable, False for disable.
            If None, opposite of the current state.

    Raises:
        TelTestUtilsError if platform does not support Video calling.
    """
    if not ad.droid.imsIsVtEnabledByPlatform():
        if new_state is not False:
            raise TelTestUtilsError("VT not supported by platform.")
        # if the user sets VT false and it's unavailable we just let it go
        return False

    current_state = ad.droid.imsIsVtEnabledByUser()
    if new_state is None:
        new_state = not current_state
    if new_state != current_state:
        ad.droid.imsSetVtSetting(new_state)
    return True


def _wait_for_droid_in_state(log, ad, max_time, state_check_func, *args,
                             **kwargs):
    while max_time >= 0:
        if state_check_func(log, ad, *args, **kwargs):
            return True

        time.sleep(WAIT_TIME_BETWEEN_STATE_CHECK)
        max_time -= WAIT_TIME_BETWEEN_STATE_CHECK

    return False


def _wait_for_droid_in_state_for_subscription(
        log, ad, sub_id, max_time, state_check_func, *args, **kwargs):
    while max_time >= 0:
        if state_check_func(log, ad, sub_id, *args, **kwargs):
            return True

        time.sleep(WAIT_TIME_BETWEEN_STATE_CHECK)
        max_time -= WAIT_TIME_BETWEEN_STATE_CHECK

    return False


def _wait_for_droids_in_state(log, ads, max_time, state_check_func, *args,
                              **kwargs):
    while max_time > 0:
        success = True
        for ad in ads:
            if not state_check_func(log, ad, *args, **kwargs):
                success = False
                break
        if success:
            return True

        time.sleep(WAIT_TIME_BETWEEN_STATE_CHECK)
        max_time -= WAIT_TIME_BETWEEN_STATE_CHECK

    return False


def is_phone_in_call(log, ad):
    """Return True if phone in call.

    Args:
        log: log object.
        ad:  android device.
    """
    try:
        return ad.droid.telecomIsInCall()
    except:
        return "mCallState=2" in ad.adb.shell(
            "dumpsys telephony.registry | grep mCallState")


def is_phone_not_in_call(log, ad):
    """Return True if phone not in call.

    Args:
        log: log object.
        ad:  android device.
    """
    in_call = ad.droid.telecomIsInCall()
    call_state = ad.droid.telephonyGetCallState()
    if in_call:
        ad.log.info("Device is In Call")
    if call_state != TELEPHONY_STATE_IDLE:
        ad.log.info("Call_state is %s, not %s", call_state,
                    TELEPHONY_STATE_IDLE)
    return ((not in_call) and (call_state == TELEPHONY_STATE_IDLE))


def wait_for_droid_in_call(log, ad, max_time):
    """Wait for android to be in call state.

    Args:
        log: log object.
        ad:  android device.
        max_time: maximal wait time.

    Returns:
        If phone become in call state within max_time, return True.
        Return False if timeout.
    """
    return _wait_for_droid_in_state(log, ad, max_time, is_phone_in_call)


def is_phone_in_call_active(ad, call_id=None):
    """Return True if phone in active call.

    Args:
        log: log object.
        ad:  android device.
        call_id: the call id
    """
    if not call_id:
        call_id = ad.droid.telecomCallGetCallIds()[0]
    call_state = ad.droid.telecomCallGetCallState(call_id)
    ad.log.info("%s state is %s", call_id, call_state)
    return call_state == "ACTIVE"


def wait_for_in_call_active(ad, timeout=5, interval=1, call_id=None):
    """Wait for call reach active state.

    Args:
        log: log object.
        ad:  android device.
        call_id: the call id
    """
    if not call_id:
        call_id = ad.droid.telecomCallGetCallIds()[0]
    args = [ad, call_id]
    if not wait_for_state(is_phone_in_call_active, True, timeout, interval,
                          *args):
        ad.log.error("Call did not reach ACTIVE state")
        return False
    else:
        return True


def wait_for_telecom_ringing(log, ad, max_time=MAX_WAIT_TIME_TELECOM_RINGING):
    """Wait for android to be in telecom ringing state.

    Args:
        log: log object.
        ad:  android device.
        max_time: maximal wait time. This is optional.
            Default Value is MAX_WAIT_TIME_TELECOM_RINGING.

    Returns:
        If phone become in telecom ringing state within max_time, return True.
        Return False if timeout.
    """
    return _wait_for_droid_in_state(
        log, ad, max_time, lambda log, ad: ad.droid.telecomIsRinging())


def wait_for_droid_not_in_call(log, ad, max_time=MAX_WAIT_TIME_CALL_DROP):
    """Wait for android to be not in call state.

    Args:
        log: log object.
        ad:  android device.
        max_time: maximal wait time.

    Returns:
        If phone become not in call state within max_time, return True.
        Return False if timeout.
    """
    return _wait_for_droid_in_state(log, ad, max_time, is_phone_not_in_call)


def _is_attached(log, ad, voice_or_data):
    return _is_attached_for_subscription(
        log, ad, ad.droid.subscriptionGetDefaultSubId(), voice_or_data)


def _is_attached_for_subscription(log, ad, sub_id, voice_or_data):
    rat = get_network_rat_for_subscription(log, ad, sub_id, voice_or_data)
    ad.log.info("Sub_id %s network RAT is %s for %s", sub_id, rat,
                voice_or_data)
    return rat != RAT_UNKNOWN


def is_voice_attached(log, ad):
    return _is_attached_for_subscription(
        log, ad, ad.droid.subscriptionGetDefaultSubId(), NETWORK_SERVICE_VOICE)


def wait_for_voice_attach(log, ad, max_time):
    """Wait for android device to attach on voice.

    Args:
        log: log object.
        ad:  android device.
        max_time: maximal wait time.

    Returns:
        Return True if device attach voice within max_time.
        Return False if timeout.
    """
    return _wait_for_droid_in_state(log, ad, max_time, _is_attached,
                                    NETWORK_SERVICE_VOICE)


def wait_for_voice_attach_for_subscription(log, ad, sub_id, max_time):
    """Wait for android device to attach on voice in subscription id.

    Args:
        log: log object.
        ad:  android device.
        sub_id: subscription id.
        max_time: maximal wait time.

    Returns:
        Return True if device attach voice within max_time.
        Return False if timeout.
    """
    if not _wait_for_droid_in_state_for_subscription(
            log, ad, sub_id, max_time, _is_attached_for_subscription,
            NETWORK_SERVICE_VOICE):
        return False

    # TODO: b/26295983 if pone attach to 1xrtt from unknown, phone may not
    # receive incoming call immediately.
    if ad.droid.telephonyGetCurrentVoiceNetworkType() == RAT_1XRTT:
        time.sleep(WAIT_TIME_1XRTT_VOICE_ATTACH)
    return True


def wait_for_data_attach(log, ad, max_time):
    """Wait for android device to attach on data.

    Args:
        log: log object.
        ad:  android device.
        max_time: maximal wait time.

    Returns:
        Return True if device attach data within max_time.
        Return False if timeout.
    """
    return _wait_for_droid_in_state(log, ad, max_time, _is_attached,
                                    NETWORK_SERVICE_DATA)


def wait_for_data_attach_for_subscription(log, ad, sub_id, max_time):
    """Wait for android device to attach on data in subscription id.

    Args:
        log: log object.
        ad:  android device.
        sub_id: subscription id.
        max_time: maximal wait time.

    Returns:
        Return True if device attach data within max_time.
        Return False if timeout.
    """
    return _wait_for_droid_in_state_for_subscription(
        log, ad, sub_id, max_time, _is_attached_for_subscription,
        NETWORK_SERVICE_DATA)


def is_ims_registered(log, ad):
    """Return True if IMS registered.

    Args:
        log: log object.
        ad: android device.

    Returns:
        Return True if IMS registered.
        Return False if IMS not registered.
    """
    return ad.droid.telephonyIsImsRegistered()


def wait_for_ims_registered(log, ad, max_time):
    """Wait for android device to register on ims.

    Args:
        log: log object.
        ad:  android device.
        max_time: maximal wait time.

    Returns:
        Return True if device register ims successfully within max_time.
        Return False if timeout.
    """
    return _wait_for_droid_in_state(log, ad, max_time, is_ims_registered)


def is_volte_enabled(log, ad):
    """Return True if VoLTE feature bit is True.

    Args:
        log: log object.
        ad: android device.

    Returns:
        Return True if VoLTE feature bit is True and IMS registered.
        Return False if VoLTE feature bit is False or IMS not registered.
    """
    result = True
    if not ad.droid.telephonyIsVolteAvailable():
        ad.log.info("IsVolteCallingAvailble is False")
        result = False
    else:
        ad.log.info("IsVolteCallingAvailble is True")
    if not is_ims_registered(log, ad):
        ad.log.info("IMS is not registered.")
        result = False
    else:
        ad.log.info("IMS is registered")
    return result


def is_video_enabled(log, ad):
    """Return True if Video Calling feature bit is True.

    Args:
        log: log object.
        ad: android device.

    Returns:
        Return True if Video Calling feature bit is True and IMS registered.
        Return False if Video Calling feature bit is False or IMS not registered.
    """
    video_status = ad.droid.telephonyIsVideoCallingAvailable()
    if video_status is True and is_ims_registered(log, ad) is False:
        log.error("Error! Video Call is Available, but IMS is not registered.")
        return False
    return video_status


def wait_for_volte_enabled(log, ad, max_time):
    """Wait for android device to report VoLTE enabled bit true.

    Args:
        log: log object.
        ad:  android device.
        max_time: maximal wait time.

    Returns:
        Return True if device report VoLTE enabled bit true within max_time.
        Return False if timeout.
    """
    return _wait_for_droid_in_state(log, ad, max_time, is_volte_enabled)


def wait_for_video_enabled(log, ad, max_time):
    """Wait for android device to report Video Telephony enabled bit true.

    Args:
        log: log object.
        ad:  android device.
        max_time: maximal wait time.

    Returns:
        Return True if device report Video Telephony enabled bit true within max_time.
        Return False if timeout.
    """
    return _wait_for_droid_in_state(log, ad, max_time, is_video_enabled)


def is_wfc_enabled(log, ad):
    """Return True if WiFi Calling feature bit is True.

    Args:
        log: log object.
        ad: android device.

    Returns:
        Return True if WiFi Calling feature bit is True and IMS registered.
        Return False if WiFi Calling feature bit is False or IMS not registered.
    """
    if not ad.droid.telephonyIsWifiCallingAvailable():
        ad.log.info("IsWifiCallingAvailble is False")
        return False
    else:
        ad.log.info("IsWifiCallingAvailble is True")
        if not is_ims_registered(log, ad):
            ad.log.info(
                "WiFi Calling is Available, but IMS is not registered.")
            return False
        return True


def wait_for_wfc_enabled(log, ad, max_time=MAX_WAIT_TIME_WFC_ENABLED):
    """Wait for android device to report WiFi Calling enabled bit true.

    Args:
        log: log object.
        ad:  android device.
        max_time: maximal wait time.
            Default value is MAX_WAIT_TIME_WFC_ENABLED.

    Returns:
        Return True if device report WiFi Calling enabled bit true within max_time.
        Return False if timeout.
    """
    return _wait_for_droid_in_state(log, ad, max_time, is_wfc_enabled)


def wait_for_wfc_disabled(log, ad, max_time=MAX_WAIT_TIME_WFC_DISABLED):
    """Wait for android device to report WiFi Calling enabled bit false.

    Args:
        log: log object.
        ad:  android device.
        max_time: maximal wait time.
            Default value is MAX_WAIT_TIME_WFC_DISABLED.

    Returns:
        Return True if device report WiFi Calling enabled bit false within max_time.
        Return False if timeout.
    """
    return _wait_for_droid_in_state(
        log, ad, max_time, lambda log, ad: not is_wfc_enabled(log, ad))


def get_phone_number(log, ad):
    """Get phone number for default subscription

    Args:
        log: log object.
        ad: Android device object.

    Returns:
        Phone number.
    """
    return get_phone_number_for_subscription(log, ad,
                                             get_outgoing_voice_sub_id(ad))


def get_phone_number_for_subscription(log, ad, subid):
    """Get phone number for subscription

    Args:
        log: log object.
        ad: Android device object.
        subid: subscription id.

    Returns:
        Phone number.
    """
    number = None
    try:
        number = ad.cfg['subscription'][subid]['phone_num']
    except KeyError:
        number = ad.droid.telephonyGetLine1NumberForSubscription(subid)
    return number


def set_phone_number(log, ad, phone_num):
    """Set phone number for default subscription

    Args:
        log: log object.
        ad: Android device object.
        phone_num: phone number string.

    Returns:
        True if success.
    """
    return set_phone_number_for_subscription(log, ad,
                                             get_outgoing_voice_sub_id(ad),
                                             phone_num)


def set_phone_number_for_subscription(log, ad, subid, phone_num):
    """Set phone number for subscription

    Args:
        log: log object.
        ad: Android device object.
        subid: subscription id.
        phone_num: phone number string.

    Returns:
        True if success.
    """
    try:
        ad.cfg['subscription'][subid]['phone_num'] = phone_num
    except Exception:
        return False
    return True


def get_operator_name(log, ad, subId=None):
    """Get operator name (e.g. vzw, tmo) of droid.

    Args:
        ad: Android device object.
        sub_id: subscription ID
            Optional, default is None

    Returns:
        Operator name.
    """
    try:
        if subId is not None:
            result = operator_name_from_plmn_id(
                ad.droid.telephonyGetNetworkOperatorForSubscription(subId))
        else:
            result = operator_name_from_plmn_id(
                ad.droid.telephonyGetNetworkOperator())
    except KeyError:
        result = CARRIER_UNKNOWN
    return result


def get_model_name(ad):
    """Get android device model name

    Args:
        ad: Android device object

    Returns:
        model name string
    """
    # TODO: Create translate table.
    model = ad.model
    if (model.startswith(AOSP_PREFIX)):
        model = model[len(AOSP_PREFIX):]
    return model


def is_sms_match(event, phonenumber_tx, text):
    """Return True if 'text' equals to event['data']['Text']
        and phone number match.

    Args:
        event: Event object to verify.
        phonenumber_tx: phone number for sender.
        text: text string to verify.

    Returns:
        Return True if 'text' equals to event['data']['Text']
            and phone number match.
    """
    return (check_phone_number_match(event['data']['Sender'], phonenumber_tx)
            and event['data']['Text'] == text)


def is_sms_partial_match(event, phonenumber_tx, text):
    """Return True if 'text' starts with event['data']['Text']
        and phone number match.

    Args:
        event: Event object to verify.
        phonenumber_tx: phone number for sender.
        text: text string to verify.

    Returns:
        Return True if 'text' starts with event['data']['Text']
            and phone number match.
    """
    return (check_phone_number_match(event['data']['Sender'], phonenumber_tx)
            and text.startswith(event['data']['Text']))


def sms_send_receive_verify(log,
                            ad_tx,
                            ad_rx,
                            array_message,
                            max_wait_time=MAX_WAIT_TIME_SMS_RECEIVE):
    """Send SMS, receive SMS, and verify content and sender's number.

        Send (several) SMS from droid_tx to droid_rx.
        Verify SMS is sent, delivered and received.
        Verify received content and sender's number are correct.

    Args:
        log: Log object.
        ad_tx: Sender's Android Device Object
        ad_rx: Receiver's Android Device Object
        array_message: the array of message to send/receive
    """
    subid_tx = get_outgoing_message_sub_id(ad_tx)
    subid_rx = get_incoming_message_sub_id(ad_rx)
    return sms_send_receive_verify_for_subscription(
        log, ad_tx, ad_rx, subid_tx, subid_rx, array_message, max_wait_time)


def wait_for_matching_sms(log,
                          ad_rx,
                          phonenumber_tx,
                          text,
                          allow_multi_part_long_sms=True,
                          begin_time=None,
                          max_wait_time=MAX_WAIT_TIME_SMS_RECEIVE):
    """Wait for matching incoming SMS.

    Args:
        log: Log object.
        ad_rx: Receiver's Android Device Object
        phonenumber_tx: Sender's phone number.
        text: SMS content string.
        allow_multi_part_long_sms: is long SMS allowed to be received as
            multiple short SMS. This is optional, default value is True.

    Returns:
        True if matching incoming SMS is received.
    """
    if not allow_multi_part_long_sms:
        try:
            ad_rx.messaging_ed.wait_for_event(EventSmsReceived, is_sms_match,
                                              max_wait_time, phonenumber_tx,
                                              text)
            return True
        except Empty:
            ad_rx.log.error("No matched SMS received event.")
            if begin_time:
                if sms_mms_receive_logcat_check(ad_rx, "sms", begin_time):
                    ad_rx.log.info("Receivd SMS message is seen in logcat")
            return False
    else:
        try:
            received_sms = ''
            while (text != ''):
                event = ad_rx.messaging_ed.wait_for_event(
                    EventSmsReceived, is_sms_partial_match, max_wait_time,
                    phonenumber_tx, text)
                text = text[len(event['data']['Text']):]
                received_sms += event['data']['Text']
            return True
        except Empty:
            ad_rx.log.error("No matched SMS received event.")
            if begin_time:
                if sms_mms_receive_logcat_check(ad_rx, "sms", begin_time):
                    ad_rx.log.info("Receivd SMS message is seen in logcat")
            if received_sms != '':
                ad_rx.log.error("Only received partial matched SMS: %s",
                                received_sms)
            return False


def is_mms_match(event, phonenumber_tx, text):
    """Return True if 'text' equals to event['data']['Text']
        and phone number match.

    Args:
        event: Event object to verify.
        phonenumber_tx: phone number for sender.
        text: text string to verify.

    Returns:
        Return True if 'text' equals to event['data']['Text']
            and phone number match.
    """
    #TODO:  add mms matching after mms message parser is added in sl4a. b/34276948
    return True


def wait_for_matching_mms(log,
                          ad_rx,
                          phonenumber_tx,
                          text,
                          begin_time=None,
                          max_wait_time=MAX_WAIT_TIME_SMS_RECEIVE):
    """Wait for matching incoming SMS.

    Args:
        log: Log object.
        ad_rx: Receiver's Android Device Object
        phonenumber_tx: Sender's phone number.
        text: SMS content string.
        allow_multi_part_long_sms: is long SMS allowed to be received as
            multiple short SMS. This is optional, default value is True.

    Returns:
        True if matching incoming SMS is received.
    """
    try:
        #TODO: add mms matching after mms message parser is added in sl4a. b/34276948
        ad_rx.messaging_ed.wait_for_event(EventMmsDownloaded, is_mms_match,
                                          max_wait_time, phonenumber_tx, text)
        return True
    except Empty:
        ad_rx.log.warning("No matched MMS downloaded event.")
        if begin_time:
            if sms_mms_receive_logcat_check(ad_rx, "mms", begin_time):
                return True
        return False


def sms_send_receive_verify_for_subscription(
        log,
        ad_tx,
        ad_rx,
        subid_tx,
        subid_rx,
        array_message,
        max_wait_time=MAX_WAIT_TIME_SMS_RECEIVE):
    """Send SMS, receive SMS, and verify content and sender's number.

        Send (several) SMS from droid_tx to droid_rx.
        Verify SMS is sent, delivered and received.
        Verify received content and sender's number are correct.

    Args:
        log: Log object.
        ad_tx: Sender's Android Device Object..
        ad_rx: Receiver's Android Device Object.
        subid_tx: Sender's subsciption ID to be used for SMS
        subid_rx: Receiver's subsciption ID to be used for SMS
        array_message: the array of message to send/receive
    """
    phonenumber_tx = ad_tx.cfg['subscription'][subid_tx]['phone_num']
    phonenumber_rx = ad_rx.cfg['subscription'][subid_rx]['phone_num']

    for ad in (ad_tx, ad_rx):
        if not getattr(ad, "messaging_droid", None):
            ad.messaging_droid, ad.messaging_ed = ad.get_droid()
            ad.messaging_ed.start()
        else:
            try:
                if not ad.messaging_droid.is_live:
                    ad.messaging_droid, ad.messaging_ed = ad.get_droid()
                    ad.messaging_ed.start()
            except Exception as e:
                ad.log.info(e)
                ad.messaging_droid, ad.messaging_ed = ad.get_droid()
                ad.messaging_ed.start()

    for text in array_message:
        # set begin_time 300ms before current time to system time discrepency
        begin_time = get_current_epoch_time() - 300
        length = len(text)
        ad_tx.log.info("Sending SMS from %s to %s, len: %s, content: %s.",
                       phonenumber_tx, phonenumber_rx, length, text)
        try:
            ad_rx.messaging_ed.clear_events(EventSmsReceived)
            ad_tx.messaging_ed.clear_events(EventSmsSentSuccess)
            ad_rx.messaging_droid.smsStartTrackingIncomingSmsMessage()
            time.sleep(1)  #sleep 100ms after starting event tracking
            ad_tx.messaging_droid.smsSendTextMessage(phonenumber_rx, text,
                                                     False)
            try:
                ad_tx.messaging_ed.pop_event(EventSmsSentSuccess,
                                             max_wait_time)
            except Empty:
                ad_tx.log.error("No sent_success event for SMS of length %s.",
                                length)
                # check log message as a work around for the missing sl4a
                # event dispatcher event
                if not sms_mms_send_logcat_check(ad_tx, "sms", begin_time):
                    return False

            if not wait_for_matching_sms(
                    log,
                    ad_rx,
                    phonenumber_tx,
                    text,
                    allow_multi_part_long_sms=True,
                    begin_time=begin_time):
                ad_rx.log.error("No matching received SMS of length %s.",
                                length)
                return False
        except Exception as e:
            log.error("Exception error %s", e)
            raise
        finally:
            ad_rx.messaging_droid.smsStopTrackingIncomingSmsMessage()
    return True


def mms_send_receive_verify(log,
                            ad_tx,
                            ad_rx,
                            array_message,
                            max_wait_time=MAX_WAIT_TIME_SMS_RECEIVE):
    """Send MMS, receive MMS, and verify content and sender's number.

        Send (several) MMS from droid_tx to droid_rx.
        Verify MMS is sent, delivered and received.
        Verify received content and sender's number are correct.

    Args:
        log: Log object.
        ad_tx: Sender's Android Device Object
        ad_rx: Receiver's Android Device Object
        array_message: the array of message to send/receive
    """
    return mms_send_receive_verify_for_subscription(
        log, ad_tx, ad_rx, get_outgoing_message_sub_id(ad_tx),
        get_incoming_message_sub_id(ad_rx), array_message, max_wait_time)


def sms_mms_send_logcat_check(ad, type, begin_time):
    type = type.upper()
    log_results = ad.search_logcat(
        "%s Message sent successfully" % type, begin_time=begin_time)
    if log_results:
        ad.log.info("Found %s sent succeessful log message: %s", type,
                    log_results[-1]["log_message"])
        return True
    else:
        log_results = ad.search_logcat(
            "ProcessSentMessageAction: Done sending %s message" % type,
            begin_time=begin_time)
        if log_results:
            for log_result in log_results:
                if "status is SUCCEEDED" in log_result["log_message"]:
                    ad.log.info(
                        "Found BugleDataModel %s send succeed log message: %s",
                        type, log_result["log_message"])
                    return True
    return False


def sms_mms_receive_logcat_check(ad, type, begin_time):
    type = type.upper()
    log_results = ad.search_logcat(
        "New %s Received" % type, begin_time=begin_time) or \
        ad.search_logcat("New %s Downloaded" % type, begin_time=begin_time)
    if log_results:
        ad.log.info("Found SL4A %s received log message: %s", type,
                    log_results[-1]["log_message"])
        return True
    else:
        log_results = ad.search_logcat(
            "Received %s message" % type, begin_time=begin_time)
        if log_results:
            ad.log.info("Found %s received log message: %s", type,
                        log_results[-1]["log_message"])
            return True
    return False


#TODO: add mms matching after mms message parser is added in sl4a. b/34276948
def mms_send_receive_verify_for_subscription(
        log,
        ad_tx,
        ad_rx,
        subid_tx,
        subid_rx,
        array_payload,
        max_wait_time=MAX_WAIT_TIME_SMS_RECEIVE):
    """Send MMS, receive MMS, and verify content and sender's number.

        Send (several) MMS from droid_tx to droid_rx.
        Verify MMS is sent, delivered and received.
        Verify received content and sender's number are correct.

    Args:
        log: Log object.
        ad_tx: Sender's Android Device Object..
        ad_rx: Receiver's Android Device Object.
        subid_tx: Sender's subsciption ID to be used for SMS
        subid_rx: Receiver's subsciption ID to be used for SMS
        array_message: the array of message to send/receive
    """

    phonenumber_tx = ad_tx.cfg['subscription'][subid_tx]['phone_num']
    phonenumber_rx = ad_rx.cfg['subscription'][subid_rx]['phone_num']

    for ad in (ad_rx, ad_tx):
        if "Permissive" not in ad.adb.shell("su root getenforce"):
            ad.adb.shell("su root setenforce 0")
        if not getattr(ad, "messaging_droid", None):
            ad.messaging_droid, ad.messaging_ed = ad.get_droid()
            ad.messaging_ed.start()

    for subject, message, filename in array_payload:
        begin_time = get_current_epoch_time()
        ad_tx.messaging_ed.clear_events(EventMmsSentSuccess)
        ad_rx.messaging_ed.clear_events(EventMmsDownloaded)
        ad_rx.messaging_droid.smsStartTrackingIncomingMmsMessage()
        ad_tx.log.info(
            "Sending MMS from %s to %s, subject: %s, message: %s, file: %s.",
            phonenumber_tx, phonenumber_rx, subject, message, filename)
        try:
            ad_tx.messaging_droid.smsSendMultimediaMessage(
                phonenumber_rx, subject, message, phonenumber_tx, filename)
            try:
                ad_tx.messaging_ed.pop_event(EventMmsSentSuccess,
                                             max_wait_time)
            except Empty:
                ad_tx.log.warning("No sent_success event.")
                # check log message as a work around for the missing sl4a
                # event dispatcher event
                if not sms_mms_send_logcat_check(ad_tx, "mms", begin_time):
                    return False

            if not wait_for_matching_mms(
                    log, ad_rx, phonenumber_tx, message,
                    begin_time=begin_time):
                return False
        except Exception as e:
            log.error("Exception error %s", e)
            raise
        finally:
            ad_rx.droid.smsStopTrackingIncomingMmsMessage()
    return True


def mms_receive_verify_after_call_hangup(
        log, ad_tx, ad_rx, array_message,
        max_wait_time=MAX_WAIT_TIME_SMS_RECEIVE):
    """Verify the suspanded MMS during call will send out after call release.

        Hangup call from droid_tx to droid_rx.
        Verify MMS is sent, delivered and received.
        Verify received content and sender's number are correct.

    Args:
        log: Log object.
        ad_tx: Sender's Android Device Object
        ad_rx: Receiver's Android Device Object
        array_message: the array of message to send/receive
    """
    return mms_receive_verify_after_call_hangup_for_subscription(
        log, ad_tx, ad_rx, get_outgoing_message_sub_id(ad_tx),
        get_incoming_message_sub_id(ad_rx), array_message, max_wait_time)


#TODO: add mms matching after mms message parser is added in sl4a. b/34276948
def mms_receive_verify_after_call_hangup_for_subscription(
        log,
        ad_tx,
        ad_rx,
        subid_tx,
        subid_rx,
        array_payload,
        max_wait_time=MAX_WAIT_TIME_SMS_RECEIVE):
    """Verify the suspanded MMS during call will send out after call release.

        Hangup call from droid_tx to droid_rx.
        Verify MMS is sent, delivered and received.
        Verify received content and sender's number are correct.

    Args:
        log: Log object.
        ad_tx: Sender's Android Device Object..
        ad_rx: Receiver's Android Device Object.
        subid_tx: Sender's subsciption ID to be used for SMS
        subid_rx: Receiver's subsciption ID to be used for SMS
        array_message: the array of message to send/receive
    """

    phonenumber_tx = ad_tx.cfg['subscription'][subid_tx]['phone_num']
    phonenumber_rx = ad_rx.cfg['subscription'][subid_rx]['phone_num']
    for ad in (ad_tx, ad_rx):
        if not getattr(ad, "messaging_droid", None):
            ad.messaging_droid, ad.messaging_ed = ad.get_droid()
            ad.messaging_ed.start()
    for subject, message, filename in array_payload:
        begin_time = get_current_epoch_time()
        ad_rx.log.info(
            "Waiting MMS from %s to %s, subject: %s, message: %s, file: %s.",
            phonenumber_tx, phonenumber_rx, subject, message, filename)
        ad_rx.messaging_droid.smsStartTrackingIncomingMmsMessage()
        time.sleep(5)
        try:
            hangup_call(log, ad_tx)
            hangup_call(log, ad_rx)
            try:
                ad_tx.messaging_ed.pop_event(EventMmsSentSuccess,
                                             max_wait_time)
            except Empty:
                log.warning("No sent_success event.")
                if not sms_mms_send_logcat_check(ad_tx, "mms", begin_time):
                    return False
            if not wait_for_matching_mms(
                    log, ad_rx, phonenumber_tx, message,
                    begin_time=begin_time):
                return False
        finally:
            ad_rx.droid.smsStopTrackingIncomingMmsMessage()
    return True


def ensure_network_rat(log,
                       ad,
                       network_preference,
                       rat_family,
                       voice_or_data=None,
                       max_wait_time=MAX_WAIT_TIME_NW_SELECTION,
                       toggle_apm_after_setting=False):
    """Ensure ad's current network is in expected rat_family.
    """
    return ensure_network_rat_for_subscription(
        log, ad, ad.droid.subscriptionGetDefaultSubId(), network_preference,
        rat_family, voice_or_data, max_wait_time, toggle_apm_after_setting)


def ensure_network_rat_for_subscription(
        log,
        ad,
        sub_id,
        network_preference,
        rat_family,
        voice_or_data=None,
        max_wait_time=MAX_WAIT_TIME_NW_SELECTION,
        toggle_apm_after_setting=False):
    """Ensure ad's current network is in expected rat_family.
    """
    if not ad.droid.telephonySetPreferredNetworkTypesForSubscription(
            network_preference, sub_id):
        ad.log.error("Set sub_id %s Preferred Networks Type %s failed.",
                     sub_id, network_preference)
        return False
    if is_droid_in_rat_family_for_subscription(log, ad, sub_id, rat_family,
                                               voice_or_data):
        ad.log.info("Sub_id %s in RAT %s for %s", sub_id, rat_family,
                    voice_or_data)
        return True

    if toggle_apm_after_setting:
        toggle_airplane_mode(log, ad, new_state=True, strict_checking=False)
        time.sleep(WAIT_TIME_ANDROID_STATE_SETTLING)
        toggle_airplane_mode(log, ad, new_state=None, strict_checking=False)

    result = wait_for_network_rat_for_subscription(
        log, ad, sub_id, rat_family, max_wait_time, voice_or_data)

    log.info(
        "End of ensure_network_rat_for_subscription for %s. "
        "Setting to %s, Expecting %s %s. Current: voice: %s(family: %s), "
        "data: %s(family: %s)", ad.serial, network_preference, rat_family,
        voice_or_data,
        ad.droid.telephonyGetCurrentVoiceNetworkTypeForSubscription(sub_id),
        rat_family_from_rat(
            ad.droid.telephonyGetCurrentVoiceNetworkTypeForSubscription(
                sub_id)),
        ad.droid.telephonyGetCurrentDataNetworkTypeForSubscription(sub_id),
        rat_family_from_rat(
            ad.droid.telephonyGetCurrentDataNetworkTypeForSubscription(
                sub_id)))
    return result


def ensure_network_preference(log,
                              ad,
                              network_preference,
                              voice_or_data=None,
                              max_wait_time=MAX_WAIT_TIME_NW_SELECTION,
                              toggle_apm_after_setting=False):
    """Ensure that current rat is within the device's preferred network rats.
    """
    return ensure_network_preference_for_subscription(
        log, ad, ad.droid.subscriptionGetDefaultSubId(), network_preference,
        voice_or_data, max_wait_time, toggle_apm_after_setting)


def ensure_network_preference_for_subscription(
        log,
        ad,
        sub_id,
        network_preference,
        voice_or_data=None,
        max_wait_time=MAX_WAIT_TIME_NW_SELECTION,
        toggle_apm_after_setting=False):
    """Ensure ad's network preference is <network_preference> for sub_id.
    """
    rat_family_list = rat_families_for_network_preference(network_preference)
    if not ad.droid.telephonySetPreferredNetworkTypesForSubscription(
            network_preference, sub_id):
        log.error("Set Preferred Networks failed.")
        return False
    if is_droid_in_rat_family_list_for_subscription(
            log, ad, sub_id, rat_family_list, voice_or_data):
        return True

    if toggle_apm_after_setting:
        toggle_airplane_mode(log, ad, new_state=True, strict_checking=False)
        time.sleep(WAIT_TIME_ANDROID_STATE_SETTLING)
        toggle_airplane_mode(log, ad, new_state=False, strict_checking=False)

    result = wait_for_preferred_network_for_subscription(
        log, ad, sub_id, network_preference, max_wait_time, voice_or_data)

    ad.log.info(
        "End of ensure_network_preference_for_subscription. "
        "Setting to %s, Expecting %s %s. Current: voice: %s(family: %s), "
        "data: %s(family: %s)", network_preference, rat_family_list,
        voice_or_data,
        ad.droid.telephonyGetCurrentVoiceNetworkTypeForSubscription(sub_id),
        rat_family_from_rat(
            ad.droid.telephonyGetCurrentVoiceNetworkTypeForSubscription(
                sub_id)),
        ad.droid.telephonyGetCurrentDataNetworkTypeForSubscription(sub_id),
        rat_family_from_rat(
            ad.droid.telephonyGetCurrentDataNetworkTypeForSubscription(
                sub_id)))
    return result


def ensure_network_generation(log,
                              ad,
                              generation,
                              max_wait_time=MAX_WAIT_TIME_NW_SELECTION,
                              voice_or_data=None,
                              toggle_apm_after_setting=False):
    """Ensure ad's network is <network generation> for default subscription ID.

    Set preferred network generation to <generation>.
    Toggle ON/OFF airplane mode if necessary.
    Wait for ad in expected network type.
    """
    return ensure_network_generation_for_subscription(
        log, ad, ad.droid.subscriptionGetDefaultSubId(), generation,
        max_wait_time, voice_or_data, toggle_apm_after_setting)


def ensure_network_generation_for_subscription(
        log,
        ad,
        sub_id,
        generation,
        max_wait_time=MAX_WAIT_TIME_NW_SELECTION,
        voice_or_data=None,
        toggle_apm_after_setting=False):
    """Ensure ad's network is <network generation> for specified subscription ID.

    Set preferred network generation to <generation>.
    Toggle ON/OFF airplane mode if necessary.
    Wait for ad in expected network type.
    """
    ad.log.info(
        "RAT network type voice: %s, data: %s",
        ad.droid.telephonyGetCurrentVoiceNetworkTypeForSubscription(sub_id),
        ad.droid.telephonyGetCurrentDataNetworkTypeForSubscription(sub_id))

    try:
        ad.log.info("Finding the network preference for generation %s for "
                    "operator %s phone type %s", generation,
                    ad.cfg["subscription"][sub_id]["operator"],
                    ad.cfg["subscription"][sub_id]["phone_type"])
        network_preference = network_preference_for_generation(
            generation, ad.cfg["subscription"][sub_id]["operator"],
            ad.cfg["subscription"][sub_id]["phone_type"])
        ad.log.info("Network preference for %s is %s", generation,
                    network_preference)
        rat_family = rat_family_for_generation(
            generation, ad.cfg["subscription"][sub_id]["operator"],
            ad.cfg["subscription"][sub_id]["phone_type"])
    except KeyError as e:
        ad.log.error("Failed to find a rat_family entry for generation %s"
                     " for subscriber %s with error %s", generation,
                     ad.cfg["subscription"][sub_id], e)
        return False

    current_network_preference = \
            ad.droid.telephonyGetPreferredNetworkTypesForSubscription(
                sub_id)
    for _ in range(3):
        if current_network_preference == network_preference:
            break
        if not ad.droid.telephonySetPreferredNetworkTypesForSubscription(
                network_preference, sub_id):
            ad.log.info(
                "Network preference is %s. Set Preferred Networks to %s failed.",
                current_network_preference, network_preference)
            reasons = ad.search_logcat(
                "REQUEST_SET_PREFERRED_NETWORK_TYPE error")
            if reasons:
                reason_log = reasons[-1]["log_message"]
                ad.log.info(reason_log)
                if "DEVICE_IN_USE" in reason_log:
                    time.sleep(5)
                else:
                    ad.log.error("Failed to set Preferred Networks to %s",
                                 network_preference)
                    return False
            else:
                ad.log.error("Failed to set Preferred Networks to %s",
                             network_preference)
                return False

    if is_droid_in_network_generation_for_subscription(
            log, ad, sub_id, generation, voice_or_data):
        return True

    if toggle_apm_after_setting:
        toggle_airplane_mode(log, ad, new_state=True, strict_checking=False)
        time.sleep(WAIT_TIME_ANDROID_STATE_SETTLING)
        toggle_airplane_mode(log, ad, new_state=False, strict_checking=False)

    result = wait_for_network_generation_for_subscription(
        log, ad, sub_id, generation, max_wait_time, voice_or_data)

    ad.log.info(
        "Ensure network %s %s %s. With network preference %s, "
        "current: voice: %s(family: %s), data: %s(family: %s)", generation,
        voice_or_data, result, network_preference,
        ad.droid.telephonyGetCurrentVoiceNetworkTypeForSubscription(sub_id),
        rat_generation_from_rat(
            ad.droid.telephonyGetCurrentVoiceNetworkTypeForSubscription(
                sub_id)),
        ad.droid.telephonyGetCurrentDataNetworkTypeForSubscription(sub_id),
        rat_generation_from_rat(
            ad.droid.telephonyGetCurrentDataNetworkTypeForSubscription(
                sub_id)))
    if not result:
        ad.log.info("singal strength = %s", get_telephony_signal_strength(ad))
    return result


def wait_for_network_rat(log,
                         ad,
                         rat_family,
                         max_wait_time=MAX_WAIT_TIME_NW_SELECTION,
                         voice_or_data=None):
    return wait_for_network_rat_for_subscription(
        log, ad, ad.droid.subscriptionGetDefaultSubId(), rat_family,
        max_wait_time, voice_or_data)


def wait_for_network_rat_for_subscription(
        log,
        ad,
        sub_id,
        rat_family,
        max_wait_time=MAX_WAIT_TIME_NW_SELECTION,
        voice_or_data=None):
    return _wait_for_droid_in_state_for_subscription(
        log, ad, sub_id, max_wait_time,
        is_droid_in_rat_family_for_subscription, rat_family, voice_or_data)


def wait_for_not_network_rat(log,
                             ad,
                             rat_family,
                             max_wait_time=MAX_WAIT_TIME_NW_SELECTION,
                             voice_or_data=None):
    return wait_for_not_network_rat_for_subscription(
        log, ad, ad.droid.subscriptionGetDefaultSubId(), rat_family,
        max_wait_time, voice_or_data)


def wait_for_not_network_rat_for_subscription(
        log,
        ad,
        sub_id,
        rat_family,
        max_wait_time=MAX_WAIT_TIME_NW_SELECTION,
        voice_or_data=None):
    return _wait_for_droid_in_state_for_subscription(
        log, ad, sub_id, max_wait_time,
        lambda log, ad, sub_id, *args, **kwargs: not is_droid_in_rat_family_for_subscription(log, ad, sub_id, rat_family, voice_or_data)
    )


def wait_for_preferred_network(log,
                               ad,
                               network_preference,
                               max_wait_time=MAX_WAIT_TIME_NW_SELECTION,
                               voice_or_data=None):
    return wait_for_preferred_network_for_subscription(
        log, ad, ad.droid.subscriptionGetDefaultSubId(), network_preference,
        max_wait_time, voice_or_data)


def wait_for_preferred_network_for_subscription(
        log,
        ad,
        sub_id,
        network_preference,
        max_wait_time=MAX_WAIT_TIME_NW_SELECTION,
        voice_or_data=None):
    rat_family_list = rat_families_for_network_preference(network_preference)
    return _wait_for_droid_in_state_for_subscription(
        log, ad, sub_id, max_wait_time,
        is_droid_in_rat_family_list_for_subscription, rat_family_list,
        voice_or_data)


def wait_for_network_generation(log,
                                ad,
                                generation,
                                max_wait_time=MAX_WAIT_TIME_NW_SELECTION,
                                voice_or_data=None):
    return wait_for_network_generation_for_subscription(
        log, ad, ad.droid.subscriptionGetDefaultSubId(), generation,
        max_wait_time, voice_or_data)


def wait_for_network_generation_for_subscription(
        log,
        ad,
        sub_id,
        generation,
        max_wait_time=MAX_WAIT_TIME_NW_SELECTION,
        voice_or_data=None):
    return _wait_for_droid_in_state_for_subscription(
        log, ad, sub_id, max_wait_time,
        is_droid_in_network_generation_for_subscription, generation,
        voice_or_data)


def is_droid_in_rat_family(log, ad, rat_family, voice_or_data=None):
    return is_droid_in_rat_family_for_subscription(
        log, ad, ad.droid.subscriptionGetDefaultSubId(), rat_family,
        voice_or_data)


def is_droid_in_rat_family_for_subscription(log,
                                            ad,
                                            sub_id,
                                            rat_family,
                                            voice_or_data=None):
    return is_droid_in_rat_family_list_for_subscription(
        log, ad, sub_id, [rat_family], voice_or_data)


def is_droid_in_rat_familiy_list(log, ad, rat_family_list, voice_or_data=None):
    return is_droid_in_rat_family_list_for_subscription(
        log, ad, ad.droid.subscriptionGetDefaultSubId(), rat_family_list,
        voice_or_data)


def is_droid_in_rat_family_list_for_subscription(log,
                                                 ad,
                                                 sub_id,
                                                 rat_family_list,
                                                 voice_or_data=None):
    service_list = [NETWORK_SERVICE_DATA, NETWORK_SERVICE_VOICE]
    if voice_or_data:
        service_list = [voice_or_data]

    for service in service_list:
        nw_rat = get_network_rat_for_subscription(log, ad, sub_id, service)
        if nw_rat == RAT_UNKNOWN or not is_valid_rat(nw_rat):
            continue
        if rat_family_from_rat(nw_rat) in rat_family_list:
            return True
    return False


def is_droid_in_network_generation(log, ad, nw_gen, voice_or_data):
    """Checks if a droid in expected network generation ("2g", "3g" or "4g").

    Args:
        log: log object.
        ad: android device.
        nw_gen: expected generation "4g", "3g", "2g".
        voice_or_data: check voice network generation or data network generation
            This parameter is optional. If voice_or_data is None, then if
            either voice or data in expected generation, function will return True.

    Returns:
        True if droid in expected network generation. Otherwise False.
    """
    return is_droid_in_network_generation_for_subscription(
        log, ad, ad.droid.subscriptionGetDefaultSubId(), nw_gen, voice_or_data)


def is_droid_in_network_generation_for_subscription(log, ad, sub_id, nw_gen,
                                                    voice_or_data):
    """Checks if a droid in expected network generation ("2g", "3g" or "4g").

    Args:
        log: log object.
        ad: android device.
        nw_gen: expected generation "4g", "3g", "2g".
        voice_or_data: check voice network generation or data network generation
            This parameter is optional. If voice_or_data is None, then if
            either voice or data in expected generation, function will return True.

    Returns:
        True if droid in expected network generation. Otherwise False.
    """
    service_list = [NETWORK_SERVICE_DATA, NETWORK_SERVICE_VOICE]

    if voice_or_data:
        service_list = [voice_or_data]

    for service in service_list:
        nw_rat = get_network_rat_for_subscription(log, ad, sub_id, service)
        ad.log.info("%s network rat is %s", service, nw_rat)
        if nw_rat == RAT_UNKNOWN or not is_valid_rat(nw_rat):
            continue

        if rat_generation_from_rat(nw_rat) == nw_gen:
            ad.log.info("%s network rat %s is expected %s", service, nw_rat,
                        nw_gen)
            return True
        else:
            ad.log.info("%s network rat %s is %s, does not meet expected %s",
                        service, nw_rat, rat_generation_from_rat(nw_rat),
                        nw_gen)
            return False

    return False


def get_network_rat(log, ad, voice_or_data):
    """Get current network type (Voice network type, or data network type)
       for default subscription id

    Args:
        ad: Android Device Object
        voice_or_data: Input parameter indicating to get voice network type or
            data network type.

    Returns:
        Current voice/data network type.
    """
    return get_network_rat_for_subscription(
        log, ad, ad.droid.subscriptionGetDefaultSubId(), voice_or_data)


def get_network_rat_for_subscription(log, ad, sub_id, voice_or_data):
    """Get current network type (Voice network type, or data network type)
       for specified subscription id

    Args:
        ad: Android Device Object
        sub_id: subscription ID
        voice_or_data: Input parameter indicating to get voice network type or
            data network type.

    Returns:
        Current voice/data network type.
    """
    if voice_or_data == NETWORK_SERVICE_VOICE:
        ret_val = ad.droid.telephonyGetCurrentVoiceNetworkTypeForSubscription(
            sub_id)
    elif voice_or_data == NETWORK_SERVICE_DATA:
        ret_val = ad.droid.telephonyGetCurrentDataNetworkTypeForSubscription(
            sub_id)
    else:
        ret_val = ad.droid.telephonyGetNetworkTypeForSubscription(sub_id)

    if ret_val is None:
        log.error("get_network_rat(): Unexpected null return value")
        return RAT_UNKNOWN
    else:
        return ret_val


def get_network_gen(log, ad, voice_or_data):
    """Get current network generation string (Voice network type, or data network type)

    Args:
        ad: Android Device Object
        voice_or_data: Input parameter indicating to get voice network generation
            or data network generation.

    Returns:
        Current voice/data network generation.
    """
    return get_network_gen_for_subscription(
        log, ad, ad.droid.subscriptionGetDefaultSubId(), voice_or_data)


def get_network_gen_for_subscription(log, ad, sub_id, voice_or_data):
    """Get current network generation string (Voice network type, or data network type)

    Args:
        ad: Android Device Object
        voice_or_data: Input parameter indicating to get voice network generation
            or data network generation.

    Returns:
        Current voice/data network generation.
    """
    try:
        return rat_generation_from_rat(
            get_network_rat_for_subscription(log, ad, sub_id, voice_or_data))
    except KeyError as e:
        ad.log.error("KeyError %s", e)
        return GEN_UNKNOWN


def check_voice_mail_count(log, ad, voice_mail_count_before,
                           voice_mail_count_after):
    """function to check if voice mail count is correct after leaving a new voice message.
    """
    return get_voice_mail_count_check_function(get_operator_name(log, ad))(
        voice_mail_count_before, voice_mail_count_after)


def get_voice_mail_number(log, ad):
    """function to get the voice mail number
    """
    voice_mail_number = get_voice_mail_check_number(get_operator_name(log, ad))
    if voice_mail_number is None:
        return get_phone_number(log, ad)
    return voice_mail_number


def ensure_phones_idle(log, ads, max_time=MAX_WAIT_TIME_CALL_DROP):
    """Ensure ads idle (not in call).
    """
    result = True
    for ad in ads:
        if not ensure_phone_idle(log, ad, max_time=max_time):
            result = False
    return result


def ensure_phone_idle(log, ad, max_time=MAX_WAIT_TIME_CALL_DROP):
    """Ensure ad idle (not in call).
    """
    if ad.droid.telecomIsInCall():
        ad.droid.telecomEndCall()
    if not wait_for_droid_not_in_call(log, ad, max_time=max_time):
        ad.log.error("Failed to end call")
        return False
    return True


def ensure_phone_subscription(log, ad):
    """Ensure Phone Subscription.
    """
    #check for sim and service
    duration = 0
    while duration < MAX_WAIT_TIME_NW_SELECTION:
        subInfo = ad.droid.subscriptionGetAllSubInfoList()
        if subInfo and len(subInfo) >= 1:
            ad.log.debug("Find valid subcription %s", subInfo)
            break
        else:
            ad.log.info("Did not find a valid subscription")
            time.sleep(5)
            duration += 5
    else:
        ad.log.error("Unable to find A valid subscription!")
        return False
    if ad.droid.subscriptionGetDefaultDataSubId() <= INVALID_SUB_ID and (
            ad.droid.subscriptionGetDefaultVoiceSubId() <= INVALID_SUB_ID):
        ad.log.error("No Valid Voice or Data Sub ID")
        return False
    voice_sub_id = ad.droid.subscriptionGetDefaultVoiceSubId()
    data_sub_id = ad.droid.subscriptionGetDefaultVoiceSubId()
    if not wait_for_voice_attach_for_subscription(
            log, ad, voice_sub_id, MAX_WAIT_TIME_NW_SELECTION -
            duration) and not wait_for_data_attach_for_subscription(
                log, ad, data_sub_id, MAX_WAIT_TIME_NW_SELECTION - duration):
        ad.log.error("Did Not Attach For Voice or Data Services")
        return False
    return True


def ensure_phone_default_state(log, ad, check_subscription=True):
    """Ensure ad in default state.
    Phone not in call.
    Phone have no stored WiFi network and WiFi disconnected.
    Phone not in airplane mode.
    """
    result = True
    if not toggle_airplane_mode(log, ad, False, False):
        ad.log.error("Fail to turn off airplane mode")
        result = False
    try:
        set_wifi_to_default(log, ad)
        if ad.droid.telecomIsInCall():
            ad.droid.telecomEndCall()
            if not wait_for_droid_not_in_call(log, ad):
                ad.log.error("Failed to end call")
        ad.droid.telephonyFactoryReset()
        ad.droid.imsFactoryReset()
        data_roaming = getattr(ad, 'roaming', False)
        if get_cell_data_roaming_state_by_adb(ad) != data_roaming:
            set_cell_data_roaming_state_by_adb(ad, data_roaming)
        remove_mobile_data_usage_limit(ad)
        if not wait_for_not_network_rat(
                log, ad, RAT_FAMILY_WLAN, voice_or_data=NETWORK_SERVICE_DATA):
            ad.log.error("%s still in %s", NETWORK_SERVICE_DATA,
                         RAT_FAMILY_WLAN)
            result = False

        if check_subscription and not ensure_phone_subscription(log, ad):
            ad.log.error("Unable to find a valid subscription!")
            result = False
    except Exception as e:
        ad.log.error("%s failure, toggle APM instead", e)
        toggle_airplane_mode_by_adb(log, ad, True)
        toggle_airplane_mode_by_adb(log, ad, False)
        ad.send_keycode("ENDCALL")
        ad.adb.shell("settings put global wfc_ims_enabled 0")
        ad.adb.shell("settings put global mobile_data 1")

    return result


def ensure_phones_default_state(log, ads, check_subscription=True):
    """Ensure ads in default state.
    Phone not in call.
    Phone have no stored WiFi network and WiFi disconnected.
    Phone not in airplane mode.

    Returns:
        True if all steps of restoring default state succeed.
        False if any of the steps to restore default state fails.
    """
    tasks = []
    for ad in ads:
        tasks.append((ensure_phone_default_state, (log, ad,
                                                   check_subscription)))
    if not multithread_func(log, tasks):
        log.error("Ensure_phones_default_state Fail.")
        return False
    return True


def check_is_wifi_connected(log, ad, wifi_ssid):
    """Check if ad is connected to wifi wifi_ssid.

    Args:
        log: Log object.
        ad: Android device object.
        wifi_ssid: WiFi network SSID.

    Returns:
        True if wifi is connected to wifi_ssid
        False if wifi is not connected to wifi_ssid
    """
    wifi_info = ad.droid.wifiGetConnectionInfo()
    if wifi_info["supplicant_state"] == "completed" and wifi_info["SSID"] == wifi_ssid:
        ad.log.info("Wifi is connected to %s", wifi_ssid)
        ad.on_mobile_data = False
        return True
    else:
        ad.log.info("Wifi is not connected to %s", wifi_ssid)
        ad.log.debug("Wifi connection_info=%s", wifi_info)
        ad.on_mobile_data = True
        return False


def ensure_wifi_connected(log, ad, wifi_ssid, wifi_pwd=None, retries=3):
    """Ensure ad connected to wifi on network wifi_ssid.

    Args:
        log: Log object.
        ad: Android device object.
        wifi_ssid: WiFi network SSID.
        wifi_pwd: optional secure network password.
        retries: the number of retries.

    Returns:
        True if wifi is connected to wifi_ssid
        False if wifi is not connected to wifi_ssid
    """
    network = {WIFI_SSID_KEY: wifi_ssid}
    if wifi_pwd:
        network[WIFI_PWD_KEY] = wifi_pwd
    for i in range(retries):
        if not ad.droid.wifiCheckState():
            ad.log.info("Wifi state is down. Turn on Wifi")
            ad.droid.wifiToggleState(True)
        if check_is_wifi_connected(log, ad, wifi_ssid):
            ad.log.info("Wifi is connected to %s", wifi_ssid)
            return True
        else:
            ad.log.info("Connecting to wifi %s", wifi_ssid)
            try:
                ad.droid.wifiConnectByConfig(network)
            except Exception:
                ad.log.info("Connecting to wifi by wifiConnect instead")
                ad.droid.wifiConnect(network)
            time.sleep(20)
            if check_is_wifi_connected(log, ad, wifi_ssid):
                ad.log.info("Connected to Wifi %s", wifi_ssid)
                return True
    ad.log.info("Fail to connected to wifi %s", wifi_ssid)
    return False


def forget_all_wifi_networks(log, ad):
    """Forget all stored wifi network information

    Args:
        log: log object
        ad: AndroidDevice object

    Returns:
        boolean success (True) or failure (False)
    """
    if not ad.droid.wifiGetConfiguredNetworks():
        ad.on_mobile_data = True
        return True
    try:
        old_state = ad.droid.wifiCheckState()
        wifi_test_utils.reset_wifi(ad)
        wifi_toggle_state(log, ad, old_state)
    except Exception as e:
        log.error("forget_all_wifi_networks with exception: %s", e)
        return False
    ad.on_mobile_data = True
    return True


def wifi_reset(log, ad, disable_wifi=True):
    """Forget all stored wifi networks and (optionally) disable WiFi

    Args:
        log: log object
        ad: AndroidDevice object
        disable_wifi: boolean to disable wifi, defaults to True
    Returns:
        boolean success (True) or failure (False)
    """
    if not forget_all_wifi_networks(log, ad):
        ad.log.error("Unable to forget all networks")
        return False
    if not wifi_toggle_state(log, ad, not disable_wifi):
        ad.log.error("Failed to toggle WiFi state to %s!", not disable_wifi)
        return False
    return True


def set_wifi_to_default(log, ad):
    """Set wifi to default state (Wifi disabled and no configured network)

    Args:
        log: log object
        ad: AndroidDevice object

    Returns:
        boolean success (True) or failure (False)
    """
    ad.droid.wifiFactoryReset()
    ad.droid.wifiToggleState(False)
    ad.on_mobile_data = True


def wifi_toggle_state(log, ad, state, retries=3):
    """Toggle the WiFi State

    Args:
        log: log object
        ad: AndroidDevice object
        state: True, False, or None

    Returns:
        boolean success (True) or failure (False)
    """
    for i in range(retries):
        if wifi_test_utils.wifi_toggle_state(ad, state, assert_on_fail=False):
            ad.on_mobile_data = not state
            return True
        time.sleep(WAIT_TIME_BETWEEN_STATE_CHECK)
    return False


def start_wifi_tethering(log, ad, ssid, password, ap_band=None):
    """Start a Tethering Session

    Args:
        log: log object
        ad: AndroidDevice object
        ssid: the name of the WiFi network
        password: optional password, used for secure networks.
        ap_band=DEPRECATED specification of 2G or 5G tethering
    Returns:
        boolean success (True) or failure (False)
    """
    return wifi_test_utils._assert_on_fail_handler(
        wifi_test_utils.start_wifi_tethering,
        False,
        ad,
        ssid,
        password,
        band=ap_band)


def stop_wifi_tethering(log, ad):
    """Stop a Tethering Session

    Args:
        log: log object
        ad: AndroidDevice object
    Returns:
        boolean success (True) or failure (False)
    """
    return wifi_test_utils._assert_on_fail_handler(
        wifi_test_utils.stop_wifi_tethering, False, ad)


def reset_preferred_network_type_to_allowable_range(log, ad):
    """If preferred network type is not in allowable range, reset to GEN_4G
    preferred network type.

    Args:
        log: log object
        ad: android device object

    Returns:
        None
    """
    for sub_id, sub_info in ad.cfg["subscription"].items():
        current_preference = \
            ad.droid.telephonyGetPreferredNetworkTypesForSubscription(sub_id)
        ad.log.debug("sub_id network preference is %s", current_preference)
        try:
            if current_preference not in get_allowable_network_preference(
                    sub_info["operator"], sub_info["phone_type"]):
                network_preference = network_preference_for_generation(
                    GEN_4G, sub_info["operator"], sub_info["phone_type"])
                ad.droid.telephonySetPreferredNetworkTypesForSubscription(
                    network_preference, sub_id)
        except KeyError:
            pass


def task_wrapper(task):
    """Task wrapper for multithread_func

    Args:
        task[0]: function to be wrapped.
        task[1]: function args.

    Returns:
        Return value of wrapped function call.
    """
    func = task[0]
    params = task[1]
    return func(*params)


def run_multithread_func_async(log, task):
    """Starts a multi-threaded function asynchronously.

    Args:
        log: log object.
        task: a task to be executed in parallel.

    Returns:
        Future object representing the execution of the task.
    """
    executor = concurrent.futures.ThreadPoolExecutor(max_workers=1)
    try:
        future_object = executor.submit(task_wrapper, task)
    except Exception as e:
        log.error("Exception error %s", e)
        raise
    return future_object


def run_multithread_func(log, tasks):
    """Run multi-thread functions and return results.

    Args:
        log: log object.
        tasks: a list of tasks to be executed in parallel.

    Returns:
        results for tasks.
    """
    MAX_NUMBER_OF_WORKERS = 10
    number_of_workers = min(MAX_NUMBER_OF_WORKERS, len(tasks))
    executor = concurrent.futures.ThreadPoolExecutor(
        max_workers=number_of_workers)
    if not log: log = logging
    try:
        results = list(executor.map(task_wrapper, tasks))
    except Exception as e:
        log.error("Exception error %s", e)
        raise
    executor.shutdown()
    if log:
        log.info("multithread_func %s result: %s",
                 [task[0].__name__ for task in tasks], results)
    return results


def multithread_func(log, tasks):
    """Multi-thread function wrapper.

    Args:
        log: log object.
        tasks: tasks to be executed in parallel.

    Returns:
        True if all tasks return True.
        False if any task return False.
    """
    results = run_multithread_func(log, tasks)
    for r in results:
        if not r:
            return False
    return True


def multithread_func_and_check_results(log, tasks, expected_results):
    """Multi-thread function wrapper.

    Args:
        log: log object.
        tasks: tasks to be executed in parallel.
        expected_results: check if the results from tasks match expected_results.

    Returns:
        True if expected_results are met.
        False if expected_results are not met.
    """
    return_value = True
    results = run_multithread_func(log, tasks)
    log.info("multithread_func result: %s, expecting %s", results,
             expected_results)
    for task, result, expected_result in zip(tasks, results, expected_results):
        if result != expected_result:
            logging.info("Result for task %s is %s, expecting %s", task[0],
                         result, expected_result)
            return_value = False
    return return_value


def set_phone_screen_on(log, ad, screen_on_time=MAX_SCREEN_ON_TIME):
    """Set phone screen on time.

    Args:
        log: Log object.
        ad: Android device object.
        screen_on_time: screen on time.
            This is optional, default value is MAX_SCREEN_ON_TIME.
    Returns:
        True if set successfully.
    """
    ad.droid.setScreenTimeout(screen_on_time)
    return screen_on_time == ad.droid.getScreenTimeout()


def set_phone_silent_mode(log, ad, silent_mode=True):
    """Set phone silent mode.

    Args:
        log: Log object.
        ad: Android device object.
        silent_mode: set phone silent or not.
            This is optional, default value is True (silent mode on).
    Returns:
        True if set successfully.
    """
    ad.droid.toggleRingerSilentMode(silent_mode)
    ad.droid.setMediaVolume(0)
    ad.droid.setVoiceCallVolume(0)
    ad.droid.setAlarmVolume(0)
    out = ad.adb.shell("settings list system | grep volume")
    for attr in re.findall(r"(volume_.*)=\d+", out):
        ad.adb.shell("settings put system %s 0" % attr)
    return silent_mode == ad.droid.checkRingerSilentMode()


def set_preferred_network_mode_pref(log, ad, sub_id, network_preference):
    """Set Preferred Network Mode for Sub_id
    Args:
        log: Log object.
        ad: Android device object.
        sub_id: Subscription ID.
        network_preference: Network Mode Type
    """
    ad.log.info("Setting ModePref to %s for Sub %s", network_preference,
                sub_id)
    if not ad.droid.telephonySetPreferredNetworkTypesForSubscription(
            network_preference, sub_id):
        ad.log.error("Set sub_id %s PreferredNetworkType %s failed", sub_id,
                     network_preference)
        return False
    return True


def set_preferred_subid_for_sms(log, ad, sub_id):
    """set subscription id for SMS

    Args:
        log: Log object.
        ad: Android device object.
        sub_id :Subscription ID.

    """
    ad.log.info("Setting subscription %s as preferred SMS SIM", sub_id)
    ad.droid.subscriptionSetDefaultSmsSubId(sub_id)
    # Wait to make sure settings take effect
    time.sleep(WAIT_TIME_ANDROID_STATE_SETTLING)
    return sub_id == ad.droid.subscriptionGetDefaultSmsSubId()


def set_preferred_subid_for_data(log, ad, sub_id):
    """set subscription id for data

    Args:
        log: Log object.
        ad: Android device object.
        sub_id :Subscription ID.

    """
    ad.log.info("Setting subscription %s as preferred Data SIM", sub_id)
    ad.droid.subscriptionSetDefaultDataSubId(sub_id)
    time.sleep(WAIT_TIME_ANDROID_STATE_SETTLING)
    # Wait to make sure settings take effect
    # Data SIM change takes around 1 min
    # Check whether data has changed to selected sim
    if not wait_for_data_connection(log, ad, True,
                                    MAX_WAIT_TIME_DATA_SUB_CHANGE):
        log.error("Data Connection failed - Not able to switch Data SIM")
        return False
    return True


def set_preferred_subid_for_voice(log, ad, sub_id):
    """set subscription id for voice

    Args:
        log: Log object.
        ad: Android device object.
        sub_id :Subscription ID.

    """
    ad.log.info("Setting subscription %s as Voice SIM", sub_id)
    ad.droid.subscriptionSetDefaultVoiceSubId(sub_id)
    ad.droid.telecomSetUserSelectedOutgoingPhoneAccountBySubId(sub_id)
    # Wait to make sure settings take effect
    time.sleep(WAIT_TIME_ANDROID_STATE_SETTLING)
    return True


def set_call_state_listen_level(log, ad, value, sub_id):
    """Set call state listen level for subscription id.

    Args:
        log: Log object.
        ad: Android device object.
        value: True or False
        sub_id :Subscription ID.

    Returns:
        True or False
    """
    if sub_id == INVALID_SUB_ID:
        log.error("Invalid Subscription ID")
        return False
    ad.droid.telephonyAdjustPreciseCallStateListenLevelForSubscription(
        "Foreground", value, sub_id)
    ad.droid.telephonyAdjustPreciseCallStateListenLevelForSubscription(
        "Ringing", value, sub_id)
    ad.droid.telephonyAdjustPreciseCallStateListenLevelForSubscription(
        "Background", value, sub_id)
    return True


def setup_sim(log, ad, sub_id, voice=False, sms=False, data=False):
    """set subscription id for voice, sms and data

    Args:
        log: Log object.
        ad: Android device object.
        sub_id :Subscription ID.
        voice: True if to set subscription as default voice subscription
        sms: True if to set subscription as default sms subscription
        data: True if to set subscription as default data subscription

    """
    if sub_id == INVALID_SUB_ID:
        log.error("Invalid Subscription ID")
        return False
    else:
        if voice:
            if not set_preferred_subid_for_voice(log, ad, sub_id):
                return False
        if sms:
            if not set_preferred_subid_for_sms(log, ad, sub_id):
                return False
        if data:
            if not set_preferred_subid_for_data(log, ad, sub_id):
                return False
    return True


def is_event_match(event, field, value):
    """Return if <field> in "event" match <value> or not.

    Args:
        event: event to test. This event need to have <field>.
        field: field to match.
        value: value to match.

    Returns:
        True if <field> in "event" match <value>.
        False otherwise.
    """
    return is_event_match_for_list(event, field, [value])


def is_event_match_for_list(event, field, value_list):
    """Return if <field> in "event" match any one of the value
        in "value_list" or not.

    Args:
        event: event to test. This event need to have <field>.
        field: field to match.
        value_list: a list of value to match.

    Returns:
        True if <field> in "event" match one of the value in "value_list".
        False otherwise.
    """
    try:
        value_in_event = event['data'][field]
    except KeyError:
        return False
    for value in value_list:
        if value_in_event == value:
            return True
    return False


def is_network_call_back_event_match(event, network_callback_id,
                                     network_callback_event):
    try:
        return (
            (network_callback_id == event['data'][NetworkCallbackContainer.ID])
            and (network_callback_event == event['data']
                 [NetworkCallbackContainer.NETWORK_CALLBACK_EVENT]))
    except KeyError:
        return False


def is_build_id(log, ad, build_id):
    """Return if ad's build id is the same as input parameter build_id.

    Args:
        log: log object.
        ad: android device object.
        build_id: android build id.

    Returns:
        True if ad's build id is the same as input parameter build_id.
        False otherwise.
    """
    actual_bid = ad.droid.getBuildID()

    ad.log.info("BUILD DISPLAY: %s", ad.droid.getBuildDisplay())
    #In case we want to log more stuff/more granularity...
    #log.info("{} BUILD ID:{} ".format(ad.serial, ad.droid.getBuildID()))
    #log.info("{} BUILD FINGERPRINT: {} "
    # .format(ad.serial), ad.droid.getBuildFingerprint())
    #log.info("{} BUILD TYPE: {} "
    # .format(ad.serial), ad.droid.getBuildType())
    #log.info("{} BUILD NUMBER: {} "
    # .format(ad.serial), ad.droid.getBuildNumber())
    if actual_bid.upper() != build_id.upper():
        ad.log.error("%s: Incorrect Build ID", ad.model)
        return False
    return True


def is_uri_equivalent(uri1, uri2):
    """Check whether two input uris match or not.

    Compare Uris.
        If Uris are tel URI, it will only take the digit part
        and compare as phone number.
        Else, it will just do string compare.

    Args:
        uri1: 1st uri to be compared.
        uri2: 2nd uri to be compared.

    Returns:
        True if two uris match. Otherwise False.
    """

    #If either is None/empty we return false
    if not uri1 or not uri2:
        return False

    try:
        if uri1.startswith('tel:') and uri2.startswith('tel:'):
            uri1_number = get_number_from_tel_uri(uri1)
            uri2_number = get_number_from_tel_uri(uri2)
            return check_phone_number_match(uri1_number, uri2_number)
        else:
            return uri1 == uri2
    except AttributeError as e:
        return False


def get_call_uri(ad, call_id):
    """Get call's uri field.

    Get Uri for call_id in ad.

    Args:
        ad: android device object.
        call_id: the call id to get Uri from.

    Returns:
        call's Uri if call is active and have uri field. None otherwise.
    """
    try:
        call_detail = ad.droid.telecomCallGetDetails(call_id)
        return call_detail["Handle"]["Uri"]
    except:
        return None


def get_number_from_tel_uri(uri):
    """Get Uri number from tel uri

    Args:
        uri: input uri

    Returns:
        If input uri is tel uri, return the number part.
        else return None.
    """
    if uri.startswith('tel:'):
        uri_number = ''.join(
            i for i in urllib.parse.unquote(uri) if i.isdigit())
        return uri_number
    else:
        return None


def find_qxdm_log_mask(ad, mask="default.cfg"):
    """Find QXDM logger mask."""
    if "/" not in mask:
        # Call nexuslogger to generate log mask
        start_nexuslogger(ad)
        # Find the log mask path
        for path in (DEFAULT_QXDM_LOG_PATH, "/data/diag_logs",
                     "/vendor/etc/mdlog/"):
            out = ad.adb.shell(
                "find %s -type f -iname %s" % (path, mask), ignore_status=True)
            if out and "No such" not in out and "Permission denied" not in out:
                if path.startswith("/vendor/"):
                    ad.qxdm_log_path = DEFAULT_QXDM_LOG_PATH
                else:
                    ad.qxdm_log_path = path
                return out.split("\n")[0]
        if mask in ad.adb.shell("ls /vendor/etc/mdlog/"):
            ad.qxdm_log_path = DEFAULT_QXDM_LOG_PATH
            return "%s/%s" % ("/vendor/etc/mdlog/", mask)
    else:
        out = ad.adb.shell("ls %s" % mask, ignore_status=True)
        if out and "No such" not in out:
            ad.qxdm_log_path = "/data/vendor/radio/diag_logs"
            return mask
    ad.log.warning("Could NOT find QXDM logger mask path for %s", mask)


def set_qxdm_logger_command(ad, mask=None):
    """Set QXDM logger always on.

    Args:
        ad: android device object.

    """
    ## Neet to check if log mask will be generated without starting nexus logger
    masks = []
    mask_path = None
    if mask:
        masks = [mask]
    masks.extend(["QC_Default.cfg", "default.cfg"])
    for mask in masks:
        mask_path = find_qxdm_log_mask(ad, mask)
        if mask_path: break
    if not mask_path:
        ad.log.error("Cannot find QXDM mask %s", mask)
        ad.qxdm_logger_command = None
        return False
    else:
        ad.log.info("Use QXDM log mask %s", mask_path)
        ad.log.debug("qxdm_log_path = %s", ad.qxdm_log_path)
        output_path = os.path.join(ad.qxdm_log_path, "logs")
        ad.qxdm_logger_command = ("diag_mdlog -f %s -o %s -s 50 -c" %
                                  (mask_path, output_path))
        conf_path = os.path.join(ad.qxdm_log_path, "diag.conf")
        # Enable qxdm always on so that after device reboot, qxdm will be
        # turned on automatically
        ad.adb.shell('echo "%s" > %s' % (ad.qxdm_logger_command, conf_path))
        ad.adb.shell(
            "setprop persist.vendor.sys.modem.diag.mdlog true", ignore_status=True)
        # Legacy pixels use persist.sys.modem.diag.mdlog.
        ad.adb.shell(
            "setprop persist.sys.modem.diag.mdlog true", ignore_status=True)
        return True


def stop_qxdm_logger(ad):
    """Stop QXDM logger."""
    for cmd in ("diag_mdlog -k", "killall diag_mdlog"):
        output = ad.adb.shell("ps -ef | grep mdlog") or ""
        if "diag_mdlog" not in output:
            break
        ad.log.debug("Kill the existing qxdm process")
        ad.adb.shell(cmd, ignore_status=True)
        time.sleep(5)


def start_qxdm_logger(ad, begin_time=None):
    """Start QXDM logger."""
    if not getattr(ad, "qxdm_log", True): return
    # Delete existing QXDM logs 5 minutes earlier than the begin_time
    if getattr(ad, "qxdm_log_path", None):
        seconds = None
        if begin_time:
            current_time = get_current_epoch_time()
            seconds = int((current_time - begin_time) / 1000.0) + 10 * 60
        elif len(ad.get_file_names(ad.qxdm_log_path)) > 50:
            seconds = 900
        if seconds:
            ad.adb.shell(
                "find %s -type f -iname *.qmdl -not -mtime -%ss -delete" %
                (ad.qxdm_log_path, seconds))
    if getattr(ad, "qxdm_logger_command", None):
        output = ad.adb.shell("ps -ef | grep mdlog") or ""
        if ad.qxdm_logger_command not in output:
            ad.log.debug("QXDM logging command %s is not running",
                         ad.qxdm_logger_command)
            if "diag_mdlog" in output:
                # Kill the existing diag_mdlog process
                # Only one diag_mdlog process can be run
                stop_qxdm_logger(ad)
            ad.log.info("Start QXDM logger")
            ad.adb.shell_nb(ad.qxdm_logger_command)
        elif not ad.get_file_names(ad.qxdm_log_path, 60):
            ad.log.debug("Existing diag_mdlog is not generating logs")
            stop_qxdm_logger(ad)
            ad.adb.shell_nb(ad.qxdm_logger_command)
        return True


def start_qxdm_loggers(log, ads, begin_time=None):
    tasks = [(start_qxdm_logger, [ad, begin_time]) for ad in ads
             if getattr(ad, "qxdm_log", True)]
    if tasks: run_multithread_func(log, tasks)


def stop_qxdm_loggers(log, ads):
    tasks = [(stop_qxdm_logger, [ad]) for ad in ads]
    run_multithread_func(log, tasks)


def start_nexuslogger(ad):
    """Start Nexus/Pixel Logger Apk."""
    qxdm_logger_apk = None
    for apk, activity in (("com.android.nexuslogger", ".MainActivity"),
                          ("com.android.pixellogger",
                           ".ui.main.MainActivity")):
        if ad.is_apk_installed(apk):
            qxdm_logger_apk = apk
            break
    if not qxdm_logger_apk: return
    if ad.is_apk_running(qxdm_logger_apk):
        if "granted=true" in ad.adb.shell(
                "dumpsys package %s | grep WRITE_EXTERN" % qxdm_logger_apk):
            return True
        else:
            ad.log.info("Kill %s" % qxdm_logger_apk)
            ad.force_stop_apk(qxdm_logger_apk)
            time.sleep(5)
    for perm in ("READ", "WRITE"):
        ad.adb.shell("pm grant %s android.permission.%s_EXTERNAL_STORAGE" %
                     (qxdm_logger_apk, perm))
    time.sleep(2)
    for i in range(3):
        ad.log.info("Start %s Attempt %d" % (qxdm_logger_apk, i + 1))
        ad.adb.shell("am start -n %s/%s" % (qxdm_logger_apk, activity))
        time.sleep(5)
        if ad.is_apk_running(qxdm_logger_apk):
            ad.send_keycode("HOME")
            return True
    return False


def check_qxdm_logger_mask(ad, mask_file="QC_Default.cfg"):
    """Check if QXDM logger always on is set.

    Args:
        ad: android device object.

    """
    output = ad.adb.shell(
        "ls /data/vendor/radio/diag_logs/", ignore_status=True)
    if not output or "No such" in output:
        return True
    if mask_file not in ad.adb.shell(
            "cat /data/vendor/radio/diag_logs/diag.conf", ignore_status=True):
        return False
    return True


def start_adb_tcpdump(ad, test_name, mask="ims"):
    """Start tcpdump on any iface

    Args:
        ad: android device object.
        test_name: tcpdump file name will have this

    """
    ad.log.debug("Ensuring no tcpdump is running in background")
    try:
        ad.adb.shell("killall -9 tcpdump")
    except AdbError:
        ad.log.warn("Killing existing tcpdump processes failed")
    out = ad.adb.shell("ls -l /sdcard/tcpdump/")
    if "No such file" in out or not out:
        ad.adb.shell("mkdir /sdcard/tcpdump")
    else:
        ad.adb.shell("rm -rf /sdcard/tcpdump/*", ignore_status=True)

    begin_time = epoch_to_log_line_timestamp(get_current_epoch_time())
    begin_time = normalize_log_line_timestamp(begin_time)

    file_name = "/sdcard/tcpdump/tcpdump_%s_%s_%s.pcap" % (ad.serial,
                                                           test_name,
                                                           begin_time)
    ad.log.info("tcpdump file is %s", file_name)
    if mask == "all":
        cmd = "adb -s {} shell tcpdump -i any -s0 -w {}" . \
                  format(ad.serial, file_name)
    else:
        cmd = "adb -s {} shell tcpdump -i any -s0 -n -p udp port 500 or \
              udp port 4500 -w {}".format(ad.serial, file_name)
    ad.log.debug("%s" % cmd)
    return start_standing_subprocess(cmd, 5)


def stop_adb_tcpdump(ad, proc=None, pull_tcpdump=False, test_name=""):
    """Stops tcpdump on any iface
       Pulls the tcpdump file in the tcpdump dir

    Args:
        ad: android device object.
        tcpdump_pid: need to know which pid to stop
        tcpdump_file: filename needed to pull out

    """
    ad.log.info("Stopping and pulling tcpdump if any")
    try:
        if proc is not None:
            stop_standing_subprocess(proc)
    except Exception as e:
        ad.log.warning(e)
    if pull_tcpdump:
        log_path = os.path.join(ad.log_path, test_name,
                                "TCPDUMP_%s" % ad.serial)
        utils.create_dir(log_path)
        ad.adb.pull("/sdcard/tcpdump/. %s" % log_path)
    ad.adb.shell("rm -rf /sdcard/tcpdump/*", ignore_status=True)
    return True


def fastboot_wipe(ad, skip_setup_wizard=True):
    """Wipe the device in fastboot mode.

    Pull sl4a apk from device. Terminate all sl4a sessions,
    Reboot the device to bootloader, wipe the device by fastboot.
    Reboot the device. wait for device to complete booting
    Re-intall and start an sl4a session.
    """
    status = True
    # Pull sl4a apk from device
    out = ad.adb.shell("pm path %s" % SL4A_APK_NAME)
    result = re.search(r"package:(.*)", out)
    if not result:
        ad.log.error("Couldn't find sl4a apk")
    else:
        sl4a_apk = result.group(1)
        ad.log.info("Get sl4a apk from %s", sl4a_apk)
        ad.pull_files([sl4a_apk], "/tmp/")
    ad.stop_services()
    ad.log.info("Reboot to bootloader")
    ad.adb.reboot_bootloader(ignore_status=True)
    ad.log.info("Wipe in fastboot")
    try:
        ad.fastboot._w()
    except Exception as e:
        ad.log.error(e)
        status = False
    time.sleep(30)  #sleep time after fastboot wipe
    for _ in range(2):
        try:
            ad.log.info("Reboot in fastboot")
            ad.fastboot.reboot()
            ad.wait_for_boot_completion()
            break
        except Exception as e:
            ad.log.error("Exception error %s", e)
    ad.root_adb()
    if result:
        # Try to reinstall for three times as the device might not be
        # ready to apk install shortly after boot complete.
        for _ in range(3):
            if ad.is_sl4a_installed():
                break
            ad.log.info("Re-install sl4a")
            ad.adb.install("-r /tmp/base.apk", ignore_status=True)
            time.sleep(10)
    try:
        ad.start_adb_logcat()
    except:
        ad.log.exception("Failed to start adb logcat!")
    if skip_setup_wizard:
        ad.exit_setup_wizard()
    if ad.skip_sl4a: return status
    bring_up_sl4a(ad)

    return status


def bring_up_sl4a(ad, attemps=3):
    for i in range(attemps):
        try:
            droid, ed = ad.get_droid()
            ed.start()
            ad.log.info("Broght up new sl4a session")
        except Exception as e:
            if i < attemps - 1:
                ad.log.info(e)
                time.sleep(10)
            else:
                ad.log.error(e)
                raise


def reboot_device(ad):
    ad.reboot()
    ad.ensure_screen_on()
    unlock_sim(ad)


def unlocking_device(ad, device_password=None):
    """First unlock device attempt, required after reboot"""
    ad.unlock_screen(device_password)
    time.sleep(2)
    ad.adb.wait_for_device(timeout=180)
    if not ad.is_waiting_for_unlock_pin():
        return True
    else:
        ad.unlock_screen(device_password)
        time.sleep(2)
        ad.adb.wait_for_device(timeout=180)
        if ad.wait_for_window_ready():
            return True
    ad.log.error("Unable to unlock to user window")
    return False


def refresh_sl4a_session(ad):
    try:
        ad.droid.logI("Checking SL4A connection")
        ad.log.info("Existing sl4a session is active")
    except:
        ad.terminate_all_sessions()
        ad.ensure_screen_on()
        ad.log.info("Open new sl4a connection")
        bring_up_sl4a(ad)


def reset_device_password(ad, device_password=None):
    # Enable or Disable Device Password per test bed config
    unlock_sim(ad)
    screen_lock = ad.is_screen_lock_enabled()
    if device_password:
        try:
            refresh_sl4a_session(ad)
            ad.droid.setDevicePassword(device_password)
        except Exception as e:
            ad.log.warning("setDevicePassword failed with %s", e)
            try:
                ad.droid.setDevicePassword(device_password, "1111")
            except Exception as e:
                ad.log.warning(
                    "setDevicePassword providing previous password error: %s",
                    e)
        time.sleep(2)
        if screen_lock:
            # existing password changed
            return
        else:
            # enable device password and log in for the first time
            ad.log.info("Enable device password")
            ad.adb.wait_for_device(timeout=180)
    else:
        if not screen_lock:
            # no existing password, do not set password
            return
        else:
            # password is enabled on the device
            # need to disable the password and log in on the first time
            # with unlocking with a swipe
            ad.log.info("Disable device password")
            ad.unlock_screen(password="1111")
            refresh_sl4a_session(ad)
            ad.ensure_screen_on()
            try:
                ad.droid.disableDevicePassword()
            except Exception as e:
                ad.log.warning("disableDevicePassword failed with %s", e)
                fastboot_wipe(ad)
            time.sleep(2)
            ad.adb.wait_for_device(timeout=180)
    refresh_sl4a_session(ad)
    if not ad.is_adb_logcat_on:
        ad.start_adb_logcat()


def get_sim_state(ad):
    try:
        state = ad.droid.telephonyGetSimState()
    except:
        state = ad.adb.getprop("gsm.sim.state")
    return state


def is_sim_locked(ad):
    return get_sim_state(ad) == SIM_STATE_PIN_REQUIRED


def unlock_sim(ad):
    #The puk and pin can be provided in testbed config file.
    #"AndroidDevice": [{"serial": "84B5T15A29018214",
    #                   "adb_logcat_param": "-b all",
    #                   "puk": "12345678",
    #                   "puk_pin": "1234"}]
    if not is_sim_locked(ad):
        return True
    else:
        ad.is_sim_locked = True
    puk_pin = getattr(ad, "puk_pin", "1111")
    try:
        if not hasattr(ad, 'puk'):
            ad.log.info("Enter SIM pin code")
            ad.droid.telephonySupplyPin(puk_pin)
        else:
            ad.log.info("Enter PUK code and pin")
            ad.droid.telephonySupplyPuk(ad.puk, puk_pin)
    except:
        # if sl4a is not available, use adb command
        ad.unlock_screen(puk_pin)
        if is_sim_locked(ad):
            ad.unlock_screen(puk_pin)
    time.sleep(30)
    return not is_sim_locked(ad)


def send_dialer_secret_code(ad, secret_code):
    """Send dialer secret code.

    ad: android device controller
    secret_code: the secret code to be sent to dialer. the string between
                 code prefix *#*# and code postfix #*#*. *#*#<xxx>#*#*
    """
    action = 'android.provider.Telephony.SECRET_CODE'
    uri = 'android_secret_code://%s' % secret_code
    intent = ad.droid.makeIntent(
        action,
        uri,
        None,  # type
        None,  # extras
        None,  # categories,
        None,  # packagename,
        None,  # classname,
        0x01000000)  # flags
    ad.log.info('Issuing dialer secret dialer code: %s', secret_code)
    ad.droid.sendBroadcastIntent(intent)


def system_file_push(ad, src_file_path, dst_file_path):
    """Push system file on a device.

    Push system file need to change some system setting and remount.
    """
    cmd = "%s %s" % (src_file_path, dst_file_path)
    out = ad.adb.push(cmd, timeout=300, ignore_status=True)
    skip_sl4a = True if "sl4a.apk" in src_file_path else False
    if "Read-only file system" in out:
        ad.log.info("Change read-only file system")
        ad.adb.disable_verity()
        ad.reboot(skip_sl4a)
        ad.adb.remount()
        out = ad.adb.push(cmd, timeout=300, ignore_status=True)
        if "Read-only file system" in out:
            ad.reboot(skip_sl4a)
            out = ad.adb.push(cmd, timeout=300, ignore_status=True)
            if "error" in out:
                ad.log.error("%s failed with %s", cmd, out)
                return False
            else:
                ad.log.info("push %s succeed")
                if skip_sl4a: ad.reboot(skip_sl4a)
                return True
        else:
            return True
    elif "error" in out:
        return False
    else:
        return True


def flash_radio(ad, file_path, skip_setup_wizard=True):
    """Flash radio image."""
    ad.stop_services()
    ad.log.info("Reboot to bootloader")
    ad.adb.reboot_bootloader(ignore_status=True)
    ad.log.info("Flash radio in fastboot")
    try:
        ad.fastboot.flash("radio %s" % file_path, timeout=300)
    except Exception as e:
        ad.log.error(e)
    for _ in range(2):
        try:
            ad.log.info("Reboot in fastboot")
            ad.fastboot.reboot()
            ad.wait_for_boot_completion()
            break
        except Exception as e:
            ad.log.error("Exception error %s", e)
    ad.root_adb()
    if not ad.ensure_screen_on():
        ad.log.error("User window cannot come up")
    ad.start_services(ad.skip_sl4a, skip_setup_wizard=skip_setup_wizard)
    unlock_sim(ad)


def set_preferred_apn_by_adb(ad, pref_apn_name):
    """Select Pref APN
       Set Preferred APN on UI using content query/insert
       It needs apn name as arg, and it will match with plmn id
    """
    try:
        plmn_id = get_plmn_by_adb(ad)
        out = ad.adb.shell("content query --uri content://telephony/carriers "
                           "--where \"apn='%s' and numeric='%s'\"" %
                           (pref_apn_name, plmn_id))
        if "No result found" in out:
            ad.log.warning("Cannot find APN %s on device", pref_apn_name)
            return False
        else:
            apn_id = re.search(r'_id=(\d+)', out).group(1)
            ad.log.info("APN ID is %s", apn_id)
            ad.adb.shell("content insert --uri content:"
                         "//telephony/carriers/preferapn --bind apn_id:i:%s" %
                         (apn_id))
            out = ad.adb.shell("content query --uri "
                               "content://telephony/carriers/preferapn")
            if "No result found" in out:
                ad.log.error("Failed to set prefer APN %s", pref_apn_name)
                return False
            elif apn_id == re.search(r'_id=(\d+)', out).group(1):
                ad.log.info("Preferred APN set to %s", pref_apn_name)
                return True
    except Exception as e:
        ad.log.error("Exception while setting pref apn %s", e)
        return True


def check_apm_mode_on_by_serial(ad, serial_id):
    try:
        apm_check_cmd = "|".join(("adb -s %s shell dumpsys wifi" % serial_id,
                                  "grep -i airplanemodeon", "cut -f2 -d ' '"))
        output = exe_cmd(apm_check_cmd)
        if output.decode("utf-8").split("\n")[0] == "true":
            return True
        else:
            return False
    except Exception as e:
        ad.log.warning("Exception during check apm mode on %s", e)
        return True


def set_apm_mode_on_by_serial(ad, serial_id):
    try:
        cmd1 = "adb -s %s shell settings put global airplane_mode_on 1" % serial_id
        cmd2 = "adb -s %s shell am broadcast -a android.intent.action.AIRPLANE_MODE" % serial_id
        exe_cmd(cmd1)
        exe_cmd(cmd2)
    except Exception as e:
        ad.log.warning("Exception during set apm mode on %s", e)
        return True


def print_radio_info(ad, extra_msg=""):
    for prop in ("gsm.version.baseband", "persist.radio.ver_info",
                 "persist.radio.cnv.ver_info"):
        output = ad.adb.getprop(prop)
        ad.log.info("%s%s = %s", extra_msg, prop, output)


def wait_for_state(state_check_func,
                   state,
                   max_wait_time=MAX_WAIT_TIME_FOR_STATE_CHANGE,
                   checking_interval=WAIT_TIME_BETWEEN_STATE_CHECK,
                   *args,
                   **kwargs):
    while max_wait_time >= 0:
        if state_check_func(*args, **kwargs) == state:
            return True
        time.sleep(checking_interval)
        max_wait_time -= checking_interval
    return False


def power_off_sim(ad, sim_slot_id=None):
    try:
        if sim_slot_id is None:
            ad.droid.telephonySetSimPowerState(CARD_POWER_DOWN)
            verify_func = ad.droid.telephonyGetSimState
            verify_args = []
        else:
            ad.droid.telephonySetSimStateForSlotId(sim_slot_id,
                                                   CARD_POWER_DOWN)
            verify_func = ad.droid.telephonyGetSimStateForSlotId
            verify_args = [sim_slot_id]
    except Exception as e:
        ad.log.error(e)
        return False
    if wait_for_state(verify_func, SIM_STATE_UNKNOWN,
                      MAX_WAIT_TIME_FOR_STATE_CHANGE,
                      WAIT_TIME_BETWEEN_STATE_CHECK, *verify_args):
        ad.log.info("SIM slot is powered off, SIM state is UNKNOWN")
        return True
    else:
        ad.log.info("SIM state = %s", verify_func(*verify_args))
        ad.log.warning("Fail to power off SIM slot")
        return False


def power_on_sim(ad, sim_slot_id=None):
    try:
        if sim_slot_id is None:
            ad.droid.telephonySetSimPowerState(CARD_POWER_UP)
            verify_func = ad.droid.telephonyGetSimState
            verify_args = []
        else:
            ad.droid.telephonySetSimStateForSlotId(sim_slot_id, CARD_POWER_UP)
            verify_func = ad.droid.telephonyGetSimStateForSlotId
            verify_args = [sim_slot_id]
    except Exception as e:
        ad.log.error(e)
        return False
    if wait_for_state(verify_func, SIM_STATE_READY,
                      MAX_WAIT_TIME_FOR_STATE_CHANGE,
                      WAIT_TIME_BETWEEN_STATE_CHECK, *verify_args):
        ad.log.info("SIM slot is powered on, SIM state is READY")
        return True
    elif verify_func(*verify_args) == SIM_STATE_PIN_REQUIRED:
        ad.log.info("SIM is pin locked")
        return True
    else:
        ad.log.error("Fail to power on SIM slot")
        return False


def log_screen_shot(ad, test_name):
    file_name = "/sdcard/Pictures/screencap_%s.png" % (
        utils.get_current_epoch_time())
    screen_shot_path = os.path.join(ad.log_path, test_name,
                                    "Screenshot_%s" % ad.serial)
    utils.create_dir(screen_shot_path)
    ad.adb.shell("screencap -p %s" % file_name)
    ad.adb.pull("%s %s" % (file_name, screen_shot_path))
