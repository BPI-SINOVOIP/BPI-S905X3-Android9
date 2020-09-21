#!/usr/bin/env python3.4
#
#   Copyright 2016 Google, Inc.
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

import logging
import time
import pprint

from enum import IntEnum
from queue import Empty

from acts import asserts
from acts import signals
from acts import utils
from acts.controllers import attenuator
from acts.test_utils.wifi import wifi_constants
from acts.test_utils.tel import tel_defines

# Default timeout used for reboot, toggle WiFi and Airplane mode,
# for the system to settle down after the operation.
DEFAULT_TIMEOUT = 10
# Number of seconds to wait for events that are supposed to happen quickly.
# Like onSuccess for start background scan and confirmation on wifi state
# change.
SHORT_TIMEOUT = 30
ROAMING_TIMEOUT = 30

# Speed of light in m/s.
SPEED_OF_LIGHT = 299792458

DEFAULT_PING_ADDR = "https://www.google.com/robots.txt"

roaming_attn = {
        "AP1_on_AP2_off": [
            0,
            0,
            95,
            95
        ],
        "AP1_off_AP2_on": [
            95,
            95,
            0,
            0
        ],
        "default": [
            0,
            0,
            0,
            0
        ]
    }

class WifiEnums():

    SSID_KEY = "SSID"
    NETID_KEY = "network_id"
    BSSID_KEY = "BSSID"
    PWD_KEY = "password"
    frequency_key = "frequency"
    APBAND_KEY = "apBand"
    HIDDEN_KEY = "hiddenSSID"

    WIFI_CONFIG_APBAND_2G = 0
    WIFI_CONFIG_APBAND_5G = 1
    WIFI_CONFIG_APBAND_AUTO = -1

    WIFI_WPS_INFO_PBC = 0
    WIFI_WPS_INFO_DISPLAY = 1
    WIFI_WPS_INFO_KEYPAD = 2
    WIFI_WPS_INFO_LABEL = 3
    WIFI_WPS_INFO_INVALID = 4

    class CountryCode():
        CHINA = "CN"
        JAPAN = "JP"
        UK = "GB"
        US = "US"
        UNKNOWN = "UNKNOWN"

    # Start of Macros for EAP
    # EAP types
    class Eap(IntEnum):
        NONE = -1
        PEAP = 0
        TLS = 1
        TTLS = 2
        PWD = 3
        SIM = 4
        AKA = 5
        AKA_PRIME = 6
        UNAUTH_TLS = 7

    # EAP Phase2 types
    class EapPhase2(IntEnum):
        NONE = 0
        PAP = 1
        MSCHAP = 2
        MSCHAPV2 = 3
        GTC = 4

    class Enterprise:
        # Enterprise Config Macros
        EMPTY_VALUE = "NULL"
        EAP = "eap"
        PHASE2 = "phase2"
        IDENTITY = "identity"
        ANON_IDENTITY = "anonymous_identity"
        PASSWORD = "password"
        SUBJECT_MATCH = "subject_match"
        ALTSUBJECT_MATCH = "altsubject_match"
        DOM_SUFFIX_MATCH = "domain_suffix_match"
        CLIENT_CERT = "client_cert"
        CA_CERT = "ca_cert"
        ENGINE = "engine"
        ENGINE_ID = "engine_id"
        PRIVATE_KEY_ID = "key_id"
        REALM = "realm"
        PLMN = "plmn"
        FQDN = "FQDN"
        FRIENDLY_NAME = "providerFriendlyName"
        ROAMING_IDS = "roamingConsortiumIds"
    # End of Macros for EAP

    # Macros for wifi p2p.
    WIFI_P2P_SERVICE_TYPE_ALL = 0
    WIFI_P2P_SERVICE_TYPE_BONJOUR = 1
    WIFI_P2P_SERVICE_TYPE_UPNP = 2
    WIFI_P2P_SERVICE_TYPE_VENDOR_SPECIFIC = 255

    class ScanResult:
        CHANNEL_WIDTH_20MHZ = 0
        CHANNEL_WIDTH_40MHZ = 1
        CHANNEL_WIDTH_80MHZ = 2
        CHANNEL_WIDTH_160MHZ = 3
        CHANNEL_WIDTH_80MHZ_PLUS_MHZ = 4

    # Macros for wifi rtt.
    class RttType(IntEnum):
        TYPE_ONE_SIDED = 1
        TYPE_TWO_SIDED = 2

    class RttPeerType(IntEnum):
        PEER_TYPE_AP = 1
        PEER_TYPE_STA = 2  # Requires NAN.
        PEER_P2P_GO = 3
        PEER_P2P_CLIENT = 4
        PEER_NAN = 5

    class RttPreamble(IntEnum):
        PREAMBLE_LEGACY = 0x01
        PREAMBLE_HT = 0x02
        PREAMBLE_VHT = 0x04

    class RttBW(IntEnum):
        BW_5_SUPPORT = 0x01
        BW_10_SUPPORT = 0x02
        BW_20_SUPPORT = 0x04
        BW_40_SUPPORT = 0x08
        BW_80_SUPPORT = 0x10
        BW_160_SUPPORT = 0x20

    class Rtt(IntEnum):
        STATUS_SUCCESS = 0
        STATUS_FAILURE = 1
        STATUS_FAIL_NO_RSP = 2
        STATUS_FAIL_REJECTED = 3
        STATUS_FAIL_NOT_SCHEDULED_YET = 4
        STATUS_FAIL_TM_TIMEOUT = 5
        STATUS_FAIL_AP_ON_DIFF_CHANNEL = 6
        STATUS_FAIL_NO_CAPABILITY = 7
        STATUS_ABORTED = 8
        STATUS_FAIL_INVALID_TS = 9
        STATUS_FAIL_PROTOCOL = 10
        STATUS_FAIL_SCHEDULE = 11
        STATUS_FAIL_BUSY_TRY_LATER = 12
        STATUS_INVALID_REQ = 13
        STATUS_NO_WIFI = 14
        STATUS_FAIL_FTM_PARAM_OVERRIDE = 15

        REASON_UNSPECIFIED = -1
        REASON_NOT_AVAILABLE = -2
        REASON_INVALID_LISTENER = -3
        REASON_INVALID_REQUEST = -4

    class RttParam:
        device_type = "deviceType"
        request_type = "requestType"
        BSSID = "bssid"
        channel_width = "channelWidth"
        frequency = "frequency"
        center_freq0 = "centerFreq0"
        center_freq1 = "centerFreq1"
        number_burst = "numberBurst"
        interval = "interval"
        num_samples_per_burst = "numSamplesPerBurst"
        num_retries_per_measurement_frame = "numRetriesPerMeasurementFrame"
        num_retries_per_FTMR = "numRetriesPerFTMR"
        lci_request = "LCIRequest"
        lcr_request = "LCRRequest"
        burst_timeout = "burstTimeout"
        preamble = "preamble"
        bandwidth = "bandwidth"
        margin = "margin"

    RTT_MARGIN_OF_ERROR = {
        RttBW.BW_80_SUPPORT: 2,
        RttBW.BW_40_SUPPORT: 5,
        RttBW.BW_20_SUPPORT: 5
    }

    # Macros as specified in the WifiScanner code.
    WIFI_BAND_UNSPECIFIED = 0  # not specified
    WIFI_BAND_24_GHZ = 1  # 2.4 GHz band
    WIFI_BAND_5_GHZ = 2  # 5 GHz band without DFS channels
    WIFI_BAND_5_GHZ_DFS_ONLY = 4  # 5 GHz band with DFS channels
    WIFI_BAND_5_GHZ_WITH_DFS = 6  # 5 GHz band with DFS channels
    WIFI_BAND_BOTH = 3  # both bands without DFS channels
    WIFI_BAND_BOTH_WITH_DFS = 7  # both bands with DFS channels

    REPORT_EVENT_AFTER_BUFFER_FULL = 0
    REPORT_EVENT_AFTER_EACH_SCAN = 1
    REPORT_EVENT_FULL_SCAN_RESULT = 2

    SCAN_TYPE_LOW_LATENCY = 0
    SCAN_TYPE_LOW_POWER = 1
    SCAN_TYPE_HIGH_ACCURACY = 2

    # US Wifi frequencies
    ALL_2G_FREQUENCIES = [2412, 2417, 2422, 2427, 2432, 2437, 2442, 2447, 2452,
                          2457, 2462]
    DFS_5G_FREQUENCIES = [5260, 5280, 5300, 5320, 5500, 5520, 5540, 5560, 5580,
                          5600, 5620, 5640, 5660, 5680, 5700, 5720]
    NONE_DFS_5G_FREQUENCIES = [5180, 5200, 5220, 5240, 5745, 5765, 5785, 5805,
                               5825]
    ALL_5G_FREQUENCIES = DFS_5G_FREQUENCIES + NONE_DFS_5G_FREQUENCIES

    band_to_frequencies = {
        WIFI_BAND_24_GHZ: ALL_2G_FREQUENCIES,
        WIFI_BAND_5_GHZ: NONE_DFS_5G_FREQUENCIES,
        WIFI_BAND_5_GHZ_DFS_ONLY: DFS_5G_FREQUENCIES,
        WIFI_BAND_5_GHZ_WITH_DFS: ALL_5G_FREQUENCIES,
        WIFI_BAND_BOTH: ALL_2G_FREQUENCIES + NONE_DFS_5G_FREQUENCIES,
        WIFI_BAND_BOTH_WITH_DFS: ALL_5G_FREQUENCIES + ALL_2G_FREQUENCIES
    }

    # All Wifi frequencies to channels lookup.
    freq_to_channel = {
        2412: 1,
        2417: 2,
        2422: 3,
        2427: 4,
        2432: 5,
        2437: 6,
        2442: 7,
        2447: 8,
        2452: 9,
        2457: 10,
        2462: 11,
        2467: 12,
        2472: 13,
        2484: 14,
        4915: 183,
        4920: 184,
        4925: 185,
        4935: 187,
        4940: 188,
        4945: 189,
        4960: 192,
        4980: 196,
        5035: 7,
        5040: 8,
        5045: 9,
        5055: 11,
        5060: 12,
        5080: 16,
        5170: 34,
        5180: 36,
        5190: 38,
        5200: 40,
        5210: 42,
        5220: 44,
        5230: 46,
        5240: 48,
        5260: 52,
        5280: 56,
        5300: 60,
        5320: 64,
        5500: 100,
        5520: 104,
        5540: 108,
        5560: 112,
        5580: 116,
        5600: 120,
        5620: 124,
        5640: 128,
        5660: 132,
        5680: 136,
        5700: 140,
        5745: 149,
        5765: 153,
        5785: 157,
        5805: 161,
        5825: 165,
    }

    # All Wifi channels to frequencies lookup.
    channel_2G_to_freq = {
        1: 2412,
        2: 2417,
        3: 2422,
        4: 2427,
        5: 2432,
        6: 2437,
        7: 2442,
        8: 2447,
        9: 2452,
        10: 2457,
        11: 2462,
        12: 2467,
        13: 2472,
        14: 2484
    }

    channel_5G_to_freq = {
        183: 4915,
        184: 4920,
        185: 4925,
        187: 4935,
        188: 4940,
        189: 4945,
        192: 4960,
        196: 4980,
        7: 5035,
        8: 5040,
        9: 5045,
        11: 5055,
        12: 5060,
        16: 5080,
        34: 5170,
        36: 5180,
        38: 5190,
        40: 5200,
        42: 5210,
        44: 5220,
        46: 5230,
        48: 5240,
        52: 5260,
        56: 5280,
        60: 5300,
        64: 5320,
        100: 5500,
        104: 5520,
        108: 5540,
        112: 5560,
        116: 5580,
        120: 5600,
        124: 5620,
        128: 5640,
        132: 5660,
        136: 5680,
        140: 5700,
        149: 5745,
        153: 5765,
        157: 5785,
        161: 5805,
        165: 5825
    }


class WifiChannelBase:
    ALL_2G_FREQUENCIES = []
    DFS_5G_FREQUENCIES = []
    NONE_DFS_5G_FREQUENCIES = []
    ALL_5G_FREQUENCIES = DFS_5G_FREQUENCIES + NONE_DFS_5G_FREQUENCIES
    MIX_CHANNEL_SCAN = []

    def band_to_freq(self, band):
        _band_to_frequencies = {
            WifiEnums.WIFI_BAND_24_GHZ: self.ALL_2G_FREQUENCIES,
            WifiEnums.WIFI_BAND_5_GHZ: self.NONE_DFS_5G_FREQUENCIES,
            WifiEnums.WIFI_BAND_5_GHZ_DFS_ONLY: self.DFS_5G_FREQUENCIES,
            WifiEnums.WIFI_BAND_5_GHZ_WITH_DFS: self.ALL_5G_FREQUENCIES,
            WifiEnums.WIFI_BAND_BOTH:
            self.ALL_2G_FREQUENCIES + self.NONE_DFS_5G_FREQUENCIES,
            WifiEnums.WIFI_BAND_BOTH_WITH_DFS:
            self.ALL_5G_FREQUENCIES + self.ALL_2G_FREQUENCIES
        }
        return _band_to_frequencies[band]


class WifiChannelUS(WifiChannelBase):
    # US Wifi frequencies
    ALL_2G_FREQUENCIES = [2412, 2417, 2422, 2427, 2432, 2437, 2442, 2447, 2452,
                          2457, 2462]
    NONE_DFS_5G_FREQUENCIES = [5180, 5200, 5220, 5240, 5745, 5765, 5785, 5805,
                               5825]
    MIX_CHANNEL_SCAN = [2412, 2437, 2462, 5180, 5200, 5280, 5260, 5300, 5500,
                        5320, 5520, 5560, 5700, 5745, 5805]

    def __init__(self, model=None):
        self.DFS_5G_FREQUENCIES = [5260, 5280, 5300, 5320, 5500, 5520,
                                   5540, 5560, 5580, 5600, 5620, 5640,
                                   5660, 5680, 5700, 5720]
        self.ALL_5G_FREQUENCIES = self.DFS_5G_FREQUENCIES + self.NONE_DFS_5G_FREQUENCIES

class WifiReferenceNetworks:
    """ Class to parse and return networks of different band and
        auth type from reference_networks
    """
    def __init__(self, obj):
        self.reference_networks = obj
        self.WIFI_2G = "2g"
        self.WIFI_5G = "5g"

        self.secure_networks_2g = []
        self.secure_networks_5g = []
        self.open_networks_2g = []
        self.open_networks_5g = []
        self._parse_networks()

    def _parse_networks(self):
        for network in self.reference_networks:
            for key in network:
                if key == self.WIFI_2G:
                    if "password" in network[key]:
                        self.secure_networks_2g.append(network[key])
                    else:
                        self.open_networks_2g.append(network[key])
                else:
                    if "password" in network[key]:
                        self.secure_networks_5g.append(network[key])
                    else:
                        self.open_networks_5g.append(network[key])

    def return_2g_secure_networks(self):
        return self.secure_networks_2g

    def return_5g_secure_networks(self):
        return self.secure_networks_5g

    def return_2g_open_networks(self):
        return self.open_networks_2g

    def return_5g_open_networks(self):
        return self.open_networks_5g

    def return_secure_networks(self):
        return self.secure_networks_2g + self.secure_networks_5g

    def return_open_networks(self):
        return self.open_networks_2g + self.open_networks_5g


def _assert_on_fail_handler(func, assert_on_fail, *args, **kwargs):
    """Wrapper function that handles the bahevior of assert_on_fail.

    When assert_on_fail is True, let all test signals through, which can
    terminate test cases directly. When assert_on_fail is False, the wrapper
    raises no test signals and reports operation status by returning True or
    False.

    Args:
        func: The function to wrap. This function reports operation status by
              raising test signals.
        assert_on_fail: A boolean that specifies if the output of the wrapper
                        is test signal based or return value based.
        args: Positional args for func.
        kwargs: Name args for func.

    Returns:
        If assert_on_fail is True, returns True/False to signal operation
        status, otherwise return nothing.
    """
    try:
        func(*args, **kwargs)
        if not assert_on_fail:
            return True
    except signals.TestSignal:
        if assert_on_fail:
            raise
        return False


def assert_network_in_list(target, network_list):
    """Makes sure a specified target Wi-Fi network exists in a list of Wi-Fi
    networks.

    Args:
        target: A dict representing a Wi-Fi network.
                E.g. {WifiEnums.SSID_KEY: "SomeNetwork"}
        network_list: A list of dicts, each representing a Wi-Fi network.
    """
    match_results = match_networks(target, network_list)
    asserts.assert_true(
        match_results, "Target network %s, does not exist in network list %s" %
        (target, network_list))


def match_networks(target_params, networks):
    """Finds the WiFi networks that match a given set of parameters in a list
    of WiFi networks.

    To be considered a match, the network should contain every key-value pair
    of target_params

    Args:
        target_params: A dict with 1 or more key-value pairs representing a Wi-Fi network.
                       E.g { 'SSID': 'wh_ap1_5g', 'BSSID': '30:b5:c2:33:e4:47' }
        networks: A list of dict objects representing WiFi networks.

    Returns:
        The networks that match the target parameters.
    """
    results = []
    asserts.assert_true(target_params,
                        "Expected networks object 'target_params' is empty")
    for n in networks:
        add_network = 1
        for k, v in target_params.items():
            if k not in n:
                add_network = 0
                break
            if n[k] != v:
                add_network = 0
                break
        if add_network:
            results.append(n)
    return results

def wait_for_wifi_state(ad, state, assert_on_fail=True):
    """Waits for the device to transition to the specified wifi state

    Args:
        ad: An AndroidDevice object.
        state: Wifi state to wait for.
        assert_on_fail: If True, error checks in this function will raise test
                        failure signals.

    Returns:
        If assert_on_fail is False, function returns True if the device transitions
        to the specified state, False otherwise. If assert_on_fail is True, no return value.
    """
    return _assert_on_fail_handler(
        _wait_for_wifi_state, assert_on_fail, ad, state=state)


def _wait_for_wifi_state(ad, state):
    """Toggles the state of wifi.

    TestFailure signals are raised when something goes wrong.

    Args:
        ad: An AndroidDevice object.
        state: Wifi state to wait for.
    """
    if state == ad.droid.wifiCheckState():
        # Check if the state is already achieved, so we don't wait for the
        # state change event by mistake.
        return
    ad.droid.wifiStartTrackingStateChange()
    fail_msg = "Device did not transition to Wi-Fi state to %s on %s." % (state,
                                                           ad.serial)
    try:
        ad.ed.wait_for_event(wifi_constants.WIFI_STATE_CHANGED,
                             lambda x: x["data"]["enabled"] == state,
                             SHORT_TIMEOUT)
    except Empty:
        asserts.assert_equal(state, ad.droid.wifiCheckState(), fail_msg)
    finally:
        ad.droid.wifiStopTrackingStateChange()

def wifi_toggle_state(ad, new_state=None, assert_on_fail=True):
    """Toggles the state of wifi.

    Args:
        ad: An AndroidDevice object.
        new_state: Wifi state to set to. If None, opposite of the current state.
        assert_on_fail: If True, error checks in this function will raise test
                        failure signals.

    Returns:
        If assert_on_fail is False, function returns True if the toggle was
        successful, False otherwise. If assert_on_fail is True, no return value.
    """
    return _assert_on_fail_handler(
        _wifi_toggle_state, assert_on_fail, ad, new_state=new_state)


def _wifi_toggle_state(ad, new_state=None):
    """Toggles the state of wifi.

    TestFailure signals are raised when something goes wrong.

    Args:
        ad: An AndroidDevice object.
        new_state: The state to set Wi-Fi to. If None, opposite of the current
                   state will be set.
    """
    if new_state is None:
        new_state = not ad.droid.wifiCheckState()
    elif new_state == ad.droid.wifiCheckState():
        # Check if the new_state is already achieved, so we don't wait for the
        # state change event by mistake.
        return
    ad.droid.wifiStartTrackingStateChange()
    ad.log.info("Setting Wi-Fi state to %s.", new_state)
    ad.ed.clear_all_events()
    # Setting wifi state.
    ad.droid.wifiToggleState(new_state)
    fail_msg = "Failed to set Wi-Fi state to %s on %s." % (new_state,
                                                           ad.serial)
    try:
        ad.ed.wait_for_event(wifi_constants.WIFI_STATE_CHANGED,
                             lambda x: x["data"]["enabled"] == new_state,
                             SHORT_TIMEOUT)
    except Empty:
        asserts.assert_equal(new_state, ad.droid.wifiCheckState(), fail_msg)
    finally:
        ad.droid.wifiStopTrackingStateChange()


def reset_wifi(ad):
    """Clears all saved Wi-Fi networks on a device.

    This will turn Wi-Fi on.

    Args:
        ad: An AndroidDevice object.

    """
    networks = ad.droid.wifiGetConfiguredNetworks()
    if not networks:
        return
    for n in networks:
        ad.droid.wifiForgetNetwork(n['networkId'])
        try:
            event = ad.ed.pop_event(wifi_constants.WIFI_FORGET_NW_SUCCESS,
                                    SHORT_TIMEOUT)
        except Empty:
            logging.warning("Could not confirm the removal of network %s.", n)
    # Check again to see if there's any network left.
    asserts.assert_true(
        not ad.droid.wifiGetConfiguredNetworks(),
        "Failed to remove these configured Wi-Fi networks: %s" % networks)


def toggle_airplane_mode_on_and_off(ad):
    """Turn ON and OFF Airplane mode.

    ad: An AndroidDevice object.
    Returns: Assert if turning on/off Airplane mode fails.

    """
    ad.log.debug("Toggling Airplane mode ON.")
    asserts.assert_true(
        utils.force_airplane_mode(ad, True),
        "Can not turn on airplane mode on: %s" % ad.serial)
    time.sleep(DEFAULT_TIMEOUT)
    ad.log.debug("Toggling Airplane mode OFF.")
    asserts.assert_true(
        utils.force_airplane_mode(ad, False),
        "Can not turn on airplane mode on: %s" % ad.serial)
    time.sleep(DEFAULT_TIMEOUT)


def toggle_wifi_off_and_on(ad):
    """Turn OFF and ON WiFi.

    ad: An AndroidDevice object.
    Returns: Assert if turning off/on WiFi fails.

    """
    ad.log.debug("Toggling wifi OFF.")
    wifi_toggle_state(ad, False)
    time.sleep(DEFAULT_TIMEOUT)
    ad.log.debug("Toggling wifi ON.")
    wifi_toggle_state(ad, True)
    time.sleep(DEFAULT_TIMEOUT)


def wifi_forget_network(ad, net_ssid):
    """Remove configured Wifi network on an android device.

    Args:
        ad: android_device object for forget network.
        net_ssid: ssid of network to be forget

    """
    networks = ad.droid.wifiGetConfiguredNetworks()
    if not networks:
        return
    for n in networks:
        if net_ssid in n[WifiEnums.SSID_KEY]:
            ad.droid.wifiForgetNetwork(n['networkId'])
            try:
                event = ad.ed.pop_event(wifi_constants.WIFI_FORGET_NW_SUCCESS,
                                        SHORT_TIMEOUT)
            except Empty:
                asserts.fail("Failed to remove network %s." % n)


def wifi_test_device_init(ad):
    """Initializes an android device for wifi testing.

    0. Make sure SL4A connection is established on the android device.
    1. Disable location service's WiFi scan.
    2. Turn WiFi on.
    3. Clear all saved networks.
    4. Set country code to US.
    5. Enable WiFi verbose logging.
    6. Sync device time with computer time.
    7. Turn off cellular data.
    8. Turn off ambient display.
    """
    utils.require_sl4a((ad, ))
    ad.droid.wifiScannerToggleAlwaysAvailable(False)
    msg = "Failed to turn off location service's scan."
    asserts.assert_true(not ad.droid.wifiScannerIsAlwaysAvailable(), msg)
    wifi_toggle_state(ad, True)
    reset_wifi(ad)
    ad.droid.wifiEnableVerboseLogging(1)
    msg = "Failed to enable WiFi verbose logging."
    asserts.assert_equal(ad.droid.wifiGetVerboseLoggingLevel(), 1, msg)
    # We don't verify the following settings since they are not critical.
    # Set wpa_supplicant log level to EXCESSIVE.
    output = ad.adb.shell("wpa_cli -i wlan0 -p -g@android:wpa_wlan0 IFNAME="
                          "wlan0 log_level EXCESSIVE")
    ad.log.info("wpa_supplicant log change status: %s", output)
    utils.sync_device_time(ad)
    ad.droid.telephonyToggleDataConnection(False)
    # TODO(angli): need to verify the country code was actually set. No generic
    # way to check right now.
    ad.adb.shell("halutil -country %s" % WifiEnums.CountryCode.US)
    utils.set_ambient_display(ad, False)


def start_wifi_connection_scan(ad):
    """Starts a wifi connection scan and wait for results to become available.

    Args:
        ad: An AndroidDevice object.
    """
    ad.ed.clear_all_events()
    ad.droid.wifiStartScan()
    try:
        ad.ed.pop_event("WifiManagerScanResultsAvailable", 60)
    except Empty:
        asserts.fail("Wi-Fi results did not become available within 60s.")


def start_wifi_connection_scan_and_return_status(ad):
    """
    Starts a wifi connection scan and wait for results to become available
    or a scan failure to be reported.

    Args:
        ad: An AndroidDevice object.
    Returns:
        True: if scan succeeded & results are available
        False: if scan failed
    """
    ad.ed.clear_all_events()
    ad.droid.wifiStartScan()
    try:
        events = ad.ed.pop_events(
            "WifiManagerScan(ResultsAvailable|Failure)", 60)
    except Empty:
        asserts.fail(
            "Wi-Fi scan results/failure did not become available within 60s.")
    # If there are multiple matches, we check for atleast one success.
    for event in events:
        if event["name"] == "WifiManagerScanResultsAvailable":
            return True
        elif event["name"] == "WifiManagerScanFailure":
            ad.log.debug("Scan failure received")
    return False


def start_wifi_connection_scan_and_check_for_network(ad, network_ssid,
                                                     max_tries=3):
    """
    Start connectivity scans & checks if the |network_ssid| is seen in
    scan results. The method performs a max of |max_tries| connectivity scans
    to find the network.

    Args:
        ad: An AndroidDevice object.
        network_ssid: SSID of the network we are looking for.
        max_tries: Number of scans to try.
    Returns:
        True: if network_ssid is found in scan results.
        False: if network_ssid is not found in scan results.
    """
    for num_tries in range(max_tries):
        if start_wifi_connection_scan_and_return_status(ad):
            scan_results = ad.droid.wifiGetScanResults()
            match_results = match_networks(
                {WifiEnums.SSID_KEY: network_ssid}, scan_results)
            if len(match_results) > 0:
                return True
    return False


def start_wifi_connection_scan_and_ensure_network_found(ad, network_ssid,
                                                        max_tries=3):
    """
    Start connectivity scans & ensure the |network_ssid| is seen in
    scan results. The method performs a max of |max_tries| connectivity scans
    to find the network.
    This method asserts on failure!

    Args:
        ad: An AndroidDevice object.
        network_ssid: SSID of the network we are looking for.
        max_tries: Number of scans to try.
    """
    ad.log.info("Starting scans to ensure %s is present", network_ssid)
    assert_msg = "Failed to find " + network_ssid + " in scan results" \
        " after " + str(max_tries) + " tries"
    asserts.assert_true(start_wifi_connection_scan_and_check_for_network(
        ad, network_ssid, max_tries), assert_msg)


def start_wifi_connection_scan_and_ensure_network_not_found(ad, network_ssid,
                                                            max_tries=3):
    """
    Start connectivity scans & ensure the |network_ssid| is not seen in
    scan results. The method performs a max of |max_tries| connectivity scans
    to find the network.
    This method asserts on failure!

    Args:
        ad: An AndroidDevice object.
        network_ssid: SSID of the network we are looking for.
        max_tries: Number of scans to try.
    """
    ad.log.info("Starting scans to ensure %s is not present", network_ssid)
    assert_msg = "Found " + network_ssid + " in scan results" \
        " after " + str(max_tries) + " tries"
    asserts.assert_false(start_wifi_connection_scan_and_check_for_network(
        ad, network_ssid, max_tries), assert_msg)


def start_wifi_background_scan(ad, scan_setting):
    """Starts wifi background scan.

    Args:
        ad: android_device object to initiate connection on.
        scan_setting: A dict representing the settings of the scan.

    Returns:
        If scan was started successfully, event data of success event is returned.
    """
    idx = ad.droid.wifiScannerStartBackgroundScan(scan_setting)
    event = ad.ed.pop_event("WifiScannerScan{}onSuccess".format(idx),
                            SHORT_TIMEOUT)
    return event['data']


def start_wifi_tethering(ad, ssid, password, band=None, hidden=None):
    """Starts wifi tethering on an android_device.

    Args:
        ad: android_device to start wifi tethering on.
        ssid: The SSID the soft AP should broadcast.
        password: The password the soft AP should use.
        band: The band the soft AP should be set on. It should be either
            WifiEnums.WIFI_CONFIG_APBAND_2G or WifiEnums.WIFI_CONFIG_APBAND_5G.
        hidden: boolean to indicate if the AP needs to be hidden or not.

    Returns:
        No return value. Error checks in this function will raise test failure signals
    """
    config = {WifiEnums.SSID_KEY: ssid}
    if password:
        config[WifiEnums.PWD_KEY] = password
    if band:
        config[WifiEnums.APBAND_KEY] = band
    if hidden:
      config[WifiEnums.HIDDEN_KEY] = hidden
    asserts.assert_true(
        ad.droid.wifiSetWifiApConfiguration(config),
        "Failed to update WifiAp Configuration")
    ad.droid.wifiStartTrackingTetherStateChange()
    ad.droid.connectivityStartTethering(tel_defines.TETHERING_WIFI, False)
    try:
        ad.ed.pop_event("ConnectivityManagerOnTetheringStarted")
        ad.ed.wait_for_event("TetherStateChanged",
                             lambda x: x["data"]["ACTIVE_TETHER"], 30)
        ad.log.debug("Tethering started successfully.")
    except Empty:
        msg = "Failed to receive confirmation of wifi tethering starting"
        asserts.fail(msg)
    finally:
        ad.droid.wifiStopTrackingTetherStateChange()


def stop_wifi_tethering(ad):
    """Stops wifi tethering on an android_device.

    Args:
        ad: android_device to stop wifi tethering on.
    """
    ad.droid.wifiStartTrackingTetherStateChange()
    ad.droid.connectivityStopTethering(tel_defines.TETHERING_WIFI)
    try:
        ad.ed.pop_event("WifiManagerApDisabled", 30)
        ad.ed.wait_for_event("TetherStateChanged",
                             lambda x: not x["data"]["ACTIVE_TETHER"], 30)
    except Empty:
        msg = "Failed to receive confirmation of wifi tethering stopping"
        asserts.fail(msg)
    finally:
        ad.droid.wifiStopTrackingTetherStateChange()


def toggle_wifi_and_wait_for_reconnection(ad,
                                          network,
                                          num_of_tries=1,
                                          assert_on_fail=True):
    """Toggle wifi state and then wait for Android device to reconnect to
    the provided wifi network.

    This expects the device to be already connected to the provided network.

    Logic steps are
     1. Ensure that we're connected to the network.
     2. Turn wifi off.
     3. Wait for 10 seconds.
     4. Turn wifi on.
     5. Wait for the "connected" event, then confirm the connected ssid is the
        one requested.

    Args:
        ad: android_device object to initiate connection on.
        network: A dictionary representing the network to await connection. The
                 dictionary must have the key "SSID".
        num_of_tries: An integer that is the number of times to try before
                      delaring failure. Default is 1.
        assert_on_fail: If True, error checks in this function will raise test
                        failure signals.

    Returns:
        If assert_on_fail is False, function returns True if the toggle was
        successful, False otherwise. If assert_on_fail is True, no return value.
    """
    return _assert_on_fail_handler(
        _toggle_wifi_and_wait_for_reconnection,
        assert_on_fail,
        ad,
        network,
        num_of_tries=num_of_tries)


def _toggle_wifi_and_wait_for_reconnection(ad, network, num_of_tries=1):
    """Toggle wifi state and then wait for Android device to reconnect to
    the provided wifi network.

    This expects the device to be already connected to the provided network.

    Logic steps are
     1. Ensure that we're connected to the network.
     2. Turn wifi off.
     3. Wait for 10 seconds.
     4. Turn wifi on.
     5. Wait for the "connected" event, then confirm the connected ssid is the
        one requested.

    This will directly fail a test if anything goes wrong.

    Args:
        ad: android_device object to initiate connection on.
        network: A dictionary representing the network to await connection. The
                 dictionary must have the key "SSID".
        num_of_tries: An integer that is the number of times to try before
                      delaring failure. Default is 1.
    """
    expected_ssid = network[WifiEnums.SSID_KEY]
    # First ensure that we're already connected to the provided network.
    verify_con = {WifiEnums.SSID_KEY: expected_ssid}
    verify_wifi_connection_info(ad, verify_con)
    # Now toggle wifi state and wait for the connection event.
    wifi_toggle_state(ad, False)
    time.sleep(10)
    wifi_toggle_state(ad, True)
    ad.droid.wifiStartTrackingStateChange()
    try:
        connect_result = None
        for i in range(num_of_tries):
            try:
                connect_result = ad.ed.pop_event(wifi_constants.WIFI_CONNECTED,
                                                 30)
                break
            except Empty:
                pass
        asserts.assert_true(connect_result,
                            "Failed to connect to Wi-Fi network %s on %s" %
                            (network, ad.serial))
        logging.debug("Connection result on %s: %s.", ad.serial,
                      connect_result)
        actual_ssid = connect_result['data'][WifiEnums.SSID_KEY]
        asserts.assert_equal(actual_ssid, expected_ssid,
                             "Connected to the wrong network on %s."
                             "Expected %s, but got %s." %
                             (ad.serial, expected_ssid, actual_ssid))
        logging.info("Connected to Wi-Fi network %s on %s", actual_ssid,
                     ad.serial)
    finally:
        ad.droid.wifiStopTrackingStateChange()


def wait_for_connect(ad, ssid=None, id=None, tries=1):
    """Wait for a connect event on queue and pop when available.

    Args:
        ad: An Android device object.
        ssid: SSID of the network to connect to.
        id: Network Id of the network to connect to.
        tries: An integer that is the number of times to try before failing.

    Returns:
        A dict with details of the connection data, which looks like this:
        {
         'time': 1485460337798,
         'name': 'WifiNetworkConnected',
         'data': {
                  'rssi': -27,
                  'is_24ghz': True,
                  'mac_address': '02:00:00:00:00:00',
                  'network_id': 1,
                  'BSSID': '30:b5:c2:33:d3:fc',
                  'ip_address': 117483712,
                  'link_speed': 54,
                  'supplicant_state': 'completed',
                  'hidden_ssid': False,
                  'SSID': 'wh_ap1_2g',
                  'is_5ghz': False}
        }

    """
    conn_result = None

    # If ssid and network id is None, just wait for any connect event.
    if id is None and ssid is None:
        for i in range(tries):
            try:
                conn_result = ad.ed.pop_event(wifi_constants.WIFI_CONNECTED, 30)
                break
            except Empty:
                pass
    else:
    # If ssid or network id is specified, wait for specific connect event.
        for i in range(tries):
            try:
                conn_result = ad.ed.pop_event(wifi_constants.WIFI_CONNECTED, 30)
                if id and conn_result['data'][WifiEnums.NETID_KEY] == id:
                    break
                elif ssid and conn_result['data'][WifiEnums.SSID_KEY] == ssid:
                    break
            except Empty:
                pass

    return conn_result


def wait_for_disconnect(ad):
    """Wait for a Disconnect event from the supplicant.

    Args:
        ad: Android device object.

    """
    try:
        ad.droid.wifiStartTrackingStateChange()
        event = ad.ed.pop_event("WifiNetworkDisconnected", 10)
        ad.droid.wifiStopTrackingStateChange()
    except Empty:
        raise signals.TestFailure("Device did not disconnect from the network")


def connect_to_wifi_network(ad, network, assert_on_fail=True,
        check_connectivity=True):
    """Connection logic for open and psk wifi networks.

    Args:
        ad: AndroidDevice to use for connection
        network: network info of the network to connect to
        assert_on_fail: If true, errors from wifi_connect will raise
                        test failure signals.
    """
    start_wifi_connection_scan_and_ensure_network_found(
        ad, network[WifiEnums.SSID_KEY])
    wifi_connect(ad,
                 network,
                 num_of_tries=3,
                 assert_on_fail=assert_on_fail,
                 check_connectivity=check_connectivity)


def connect_to_wifi_network_with_id(ad, network_id, network_ssid):
    """Connect to the given network using network id and verify SSID.

    Args:
        network_id: int Network Id of the network.
        network_ssid: string SSID of the network.

    Returns: True if connect using network id was successful;
             False otherwise.

    """
    start_wifi_connection_scan_and_ensure_network_found(ad, network_ssid)
    wifi_connect_by_id(ad, network_id)
    connect_data = ad.droid.wifiGetConnectionInfo()
    connect_ssid = connect_data[WifiEnums.SSID_KEY]
    ad.log.debug("Expected SSID = %s Connected SSID = %s" %
                   (network_ssid, connect_ssid))
    if connect_ssid != network_ssid:
        return False
    return True


def wifi_connect(ad, network, num_of_tries=1, assert_on_fail=True,
        check_connectivity=True):
    """Connect an Android device to a wifi network.

    Initiate connection to a wifi network, wait for the "connected" event, then
    confirm the connected ssid is the one requested.

    This will directly fail a test if anything goes wrong.

    Args:
        ad: android_device object to initiate connection on.
        network: A dictionary representing the network to connect to. The
                 dictionary must have the key "SSID".
        num_of_tries: An integer that is the number of times to try before
                      delaring failure. Default is 1.
        assert_on_fail: If True, error checks in this function will raise test
                        failure signals.

    Returns:
        Returns a value only if assert_on_fail is false.
        Returns True if the connection was successful, False otherwise.
    """
    return _assert_on_fail_handler(
        _wifi_connect, assert_on_fail, ad, network, num_of_tries=num_of_tries,
          check_connectivity=check_connectivity)


def _wifi_connect(ad, network, num_of_tries=1, check_connectivity=True):
    """Connect an Android device to a wifi network.

    Initiate connection to a wifi network, wait for the "connected" event, then
    confirm the connected ssid is the one requested.

    This will directly fail a test if anything goes wrong.

    Args:
        ad: android_device object to initiate connection on.
        network: A dictionary representing the network to connect to. The
                 dictionary must have the key "SSID".
        num_of_tries: An integer that is the number of times to try before
                      delaring failure. Default is 1.
    """
    asserts.assert_true(WifiEnums.SSID_KEY in network,
                        "Key '%s' must be present in network definition." %
                        WifiEnums.SSID_KEY)
    ad.droid.wifiStartTrackingStateChange()
    expected_ssid = network[WifiEnums.SSID_KEY]
    ad.droid.wifiConnectByConfig(network)
    ad.log.info("Starting connection process to %s", expected_ssid)
    try:
        event = ad.ed.pop_event(wifi_constants.CONNECT_BY_CONFIG_SUCCESS, 30)
        connect_result = wait_for_connect(ad, ssid=expected_ssid, tries=num_of_tries)
        asserts.assert_true(connect_result,
                            "Failed to connect to Wi-Fi network %s on %s" %
                            (network, ad.serial))
        ad.log.debug("Wi-Fi connection result: %s.", connect_result)
        actual_ssid = connect_result['data'][WifiEnums.SSID_KEY]
        asserts.assert_equal(actual_ssid, expected_ssid,
                             "Connected to the wrong network on %s." %
                             ad.serial)
        ad.log.info("Connected to Wi-Fi network %s.", actual_ssid)

        # Wait for data connection to stabilize.
        time.sleep(5)

        if check_connectivity:
            internet = validate_connection(ad, DEFAULT_PING_ADDR)
            if not internet:
                raise signals.TestFailure("Failed to connect to internet on %s" %
                                          expected_ssid)
    except Empty:
        asserts.fail("Failed to start connection process to %s on %s" %
                     (network, ad.serial))
    except Exception as error:
        ad.log.error("Failed to connect to %s with error %s", expected_ssid,
                     error)
        raise signals.TestFailure("Failed to connect to %s network" % network)

    finally:
        ad.droid.wifiStopTrackingStateChange()


def wifi_connect_by_id(ad, network_id, num_of_tries=3, assert_on_fail=True):
    """Connect an Android device to a wifi network using network Id.

    Start connection to the wifi network, with the given network Id, wait for
    the "connected" event, then verify the connected network is the one requested.

    This will directly fail a test if anything goes wrong.

    Args:
        ad: android_device object to initiate connection on.
        network_id: Integer specifying the network id of the network.
        num_of_tries: An integer that is the number of times to try before
                      delaring failure. Default is 1.
        assert_on_fail: If True, error checks in this function will raise test
                        failure signals.

    Returns:
        Returns a value only if assert_on_fail is false.
        Returns True if the connection was successful, False otherwise.
    """
    _assert_on_fail_handler(_wifi_connect_by_id, assert_on_fail, ad,
                            network_id, num_of_tries)


def _wifi_connect_by_id(ad, network_id, num_of_tries=1):
    """Connect an Android device to a wifi network using it's network id.

    Start connection to the wifi network, with the given network id, wait for
    the "connected" event, then verify the connected network is the one requested.

    Args:
        ad: android_device object to initiate connection on.
        network_id: Integer specifying the network id of the network.
        num_of_tries: An integer that is the number of times to try before
                      delaring failure. Default is 1.
    """
    ad.droid.wifiStartTrackingStateChange()
    # Clear all previous events.
    ad.ed.clear_all_events()
    ad.droid.wifiConnectByNetworkId(network_id)
    ad.log.info("Starting connection to network with id %d", network_id)
    try:
        event = ad.ed.pop_event(wifi_constants.CONNECT_BY_NETID_SUCCESS, 60)
        connect_result = wait_for_connect(ad, id=network_id, tries=num_of_tries)
        asserts.assert_true(connect_result,
                            "Failed to connect to Wi-Fi network using network id")
        ad.log.debug("Wi-Fi connection result: %s", connect_result)
        actual_id = connect_result['data'][WifiEnums.NETID_KEY]
        asserts.assert_equal(actual_id, network_id,
                             "Connected to the wrong network on %s."
                             "Expected network id = %d, but got %d." %
                             (ad.serial, network_id, actual_id))
        expected_ssid = connect_result['data'][WifiEnums.SSID_KEY]
        ad.log.info("Connected to Wi-Fi network %s with %d network id.",
                     expected_ssid, network_id)

        # Wait for data connection to stabilize.
        time.sleep(5)

        internet = validate_connection(ad, DEFAULT_PING_ADDR)
        if not internet:
            raise signals.TestFailure("Failed to connect to internet on %s" %
                                      expected_ssid)
    except Empty:
        asserts.fail("Failed to connect to network with id %d on %s" %
                    (network_id, ad.serial))
    except Exception as error:
        ad.log.error("Failed to connect to network with id %d with error %s",
                      network_id, error)
        raise signals.TestFailure("Failed to connect to network with network"
                                  " id %d" % network_id)
    finally:
        ad.droid.wifiStopTrackingStateChange()


def wifi_passpoint_connect(ad, passpoint_network, num_of_tries=1,
                           assert_on_fail=True):
    """Connect an Android device to a wifi network.

    Initiate connection to a wifi network, wait for the "connected" event, then
    confirm the connected ssid is the one requested.

    This will directly fail a test if anything goes wrong.

    Args:
        ad: android_device object to initiate connection on.
        passpoint_network: SSID of the Passpoint network to connect to.
        num_of_tries: An integer that is the number of times to try before
                      delaring failure. Default is 1.
        assert_on_fail: If True, error checks in this function will raise test
                        failure signals.

    Returns:
        If assert_on_fail is False, function returns network id, if the connect was
        successful, False otherwise. If assert_on_fail is True, no return value.
    """
    _assert_on_fail_handler(_wifi_passpoint_connect, assert_on_fail, ad,
                            passpoint_network, num_of_tries = num_of_tries)


def _wifi_passpoint_connect(ad, passpoint_network, num_of_tries=1):
    """Connect an Android device to a wifi network.

    Initiate connection to a wifi network, wait for the "connected" event, then
    confirm the connected ssid is the one requested.

    This will directly fail a test if anything goes wrong.

    Args:
        ad: android_device object to initiate connection on.
        passpoint_network: SSID of the Passpoint network to connect to.
        num_of_tries: An integer that is the number of times to try before
                      delaring failure. Default is 1.
    """
    ad.droid.wifiStartTrackingStateChange()
    expected_ssid = passpoint_network
    ad.log.info("Starting connection process to passpoint %s", expected_ssid)

    try:
        connect_result = wait_for_connect(ad, expected_ssid, num_of_tries)
        asserts.assert_true(connect_result,
                            "Failed to connect to WiFi passpoint network %s on"
                            " %s" % (expected_ssid, ad.serial))
        ad.log.info("Wi-Fi connection result: %s.", connect_result)
        actual_ssid = connect_result['data'][WifiEnums.SSID_KEY]
        asserts.assert_equal(actual_ssid, expected_ssid,
                             "Connected to the wrong network on %s." % ad.serial)
        ad.log.info("Connected to Wi-Fi passpoint network %s.", actual_ssid)

        # Wait for data connection to stabilize.
        time.sleep(5)

        internet = validate_connection(ad, DEFAULT_PING_ADDR)
        if not internet:
            raise signals.TestFailure("Failed to connect to internet on %s" %
                                      expected_ssid)
    except Exception as error:
        ad.log.error("Failed to connect to passpoint network %s with error %s",
                      expected_ssid, error)
        raise signals.TestFailure("Failed to connect to %s passpoint network" %
                                   expected_ssid)

    finally:
        ad.droid.wifiStopTrackingStateChange()


def delete_passpoint(ad, fqdn):
    """Delete a required Passpoint configuration."""
    try:
        ad.droid.removePasspointConfig(fqdn)
        return True
    except Exception as error:
        ad.log.error("Failed to remove passpoint configuration with FQDN=%s "
                     "and error=%s" , fqdn, error)
        return False


def start_wifi_single_scan(ad, scan_setting):
    """Starts wifi single shot scan.

    Args:
        ad: android_device object to initiate connection on.
        scan_setting: A dict representing the settings of the scan.

    Returns:
        If scan was started successfully, event data of success event is returned.
    """
    idx = ad.droid.wifiScannerStartScan(scan_setting)
    event = ad.ed.pop_event("WifiScannerScan%sonSuccess" % idx, SHORT_TIMEOUT)
    ad.log.debug("Got event %s", event)
    return event['data']


def track_connection(ad, network_ssid, check_connection_count):
    """Track wifi connection to network changes for given number of counts

    Args:
        ad: android_device object for forget network.
        network_ssid: network ssid to which connection would be tracked
        check_connection_count: Integer for maximum number network connection
                                check.
    Returns:
        True if connection to given network happen, else return False.
    """
    ad.droid.wifiStartTrackingStateChange()
    while check_connection_count > 0:
        connect_network = ad.ed.pop_event("WifiNetworkConnected", 120)
        ad.log.info("Connected to network %s", connect_network)
        if (WifiEnums.SSID_KEY in connect_network['data'] and
                connect_network['data'][WifiEnums.SSID_KEY] == network_ssid):
            return True
        check_connection_count -= 1
    ad.droid.wifiStopTrackingStateChange()
    return False


def get_scan_time_and_channels(wifi_chs, scan_setting, stime_channel):
    """Calculate the scan time required based on the band or channels in scan
    setting

    Args:
        wifi_chs: Object of channels supported
        scan_setting: scan setting used for start scan
        stime_channel: scan time per channel

    Returns:
        scan_time: time required for completing a scan
        scan_channels: channel used for scanning
    """
    scan_time = 0
    scan_channels = []
    if "band" in scan_setting and "channels" not in scan_setting:
        scan_channels = wifi_chs.band_to_freq(scan_setting["band"])
    elif "channels" in scan_setting and "band" not in scan_setting:
        scan_channels = scan_setting["channels"]
    scan_time = len(scan_channels) * stime_channel
    for channel in scan_channels:
        if channel in WifiEnums.DFS_5G_FREQUENCIES:
            scan_time += 132  #passive scan time on DFS
    return scan_time, scan_channels


def start_wifi_track_bssid(ad, track_setting):
    """Start tracking Bssid for the given settings.

    Args:
      ad: android_device object.
      track_setting: Setting for which the bssid tracking should be started

    Returns:
      If tracking started successfully, event data of success event is returned.
    """
    idx = ad.droid.wifiScannerStartTrackingBssids(
        track_setting["bssidInfos"], track_setting["apLostThreshold"])
    event = ad.ed.pop_event("WifiScannerBssid{}onSuccess".format(idx),
                            SHORT_TIMEOUT)
    return event['data']


def convert_pem_key_to_pkcs8(in_file, out_file):
    """Converts the key file generated by us to the format required by
    Android using openssl.

    The input file must have the extension "pem". The output file must
    have the extension "der".

    Args:
        in_file: The original key file.
        out_file: The full path to the converted key file, including
        filename.
    """
    asserts.assert_true(in_file.endswith(".pem"), "Input file has to be .pem.")
    asserts.assert_true(
        out_file.endswith(".der"), "Output file has to be .der.")
    cmd = ("openssl pkcs8 -inform PEM -in {} -outform DER -out {} -nocrypt"
           " -topk8").format(in_file, out_file)
    utils.exe_cmd(cmd)


def validate_connection(ad, ping_addr=DEFAULT_PING_ADDR):
    """Validate internet connection by pinging the address provided.

    Args:
        ad: android_device object.
        ping_addr: address on internet for pinging.

    Returns:
        ping output if successful, NULL otherwise.
    """
    ping = ad.droid.httpPing(ping_addr)
    ad.log.info("Http ping result: %s.", ping)
    return ping


#TODO(angli): This can only verify if an actual value is exactly the same.
# Would be nice to be able to verify an actual value is one of serveral.
def verify_wifi_connection_info(ad, expected_con):
    """Verifies that the information of the currently connected wifi network is
    as expected.

    Args:
        expected_con: A dict representing expected key-value pairs for wifi
            connection. e.g. {"SSID": "test_wifi"}
    """
    current_con = ad.droid.wifiGetConnectionInfo()
    case_insensitive = ["BSSID", "supplicant_state"]
    ad.log.debug("Current connection: %s", current_con)
    for k, expected_v in expected_con.items():
        # Do not verify authentication related fields.
        if k == "password":
            continue
        msg = "Field %s does not exist in wifi connection info %s." % (
            k, current_con)
        if k not in current_con:
            raise signals.TestFailure(msg)
        actual_v = current_con[k]
        if k in case_insensitive:
            actual_v = actual_v.lower()
            expected_v = expected_v.lower()
        msg = "Expected %s to be %s, actual %s is %s." % (k, expected_v, k,
                                                          actual_v)
        if actual_v != expected_v:
            raise signals.TestFailure(msg)


def expand_enterprise_config_by_phase2(config):
    """Take an enterprise config and generate a list of configs, each with
    a different phase2 auth type.

    Args:
        config: A dict representing enterprise config.

    Returns
        A list of enterprise configs.
    """
    results = []
    phase2_types = WifiEnums.EapPhase2
    if config[WifiEnums.Enterprise.EAP] == WifiEnums.Eap.PEAP:
        # Skip unsupported phase2 types for PEAP.
        phase2_types = [WifiEnums.EapPhase2.GTC, WifiEnums.EapPhase2.MSCHAPV2]
    for phase2_type in phase2_types:
        # Skip a special case for passpoint TTLS.
        if (WifiEnums.Enterprise.FQDN in config and
                phase2_type == WifiEnums.EapPhase2.GTC):
            continue
        c = dict(config)
        c[WifiEnums.Enterprise.PHASE2] = phase2_type.value
        results.append(c)
    return results


def generate_eap_test_name(config, ad=None):
    """ Generates a test case name based on an EAP configuration.

    Args:
        config: A dict representing an EAP credential.
        ad object: Redundant but required as the same param is passed
                   to test_func in run_generated_tests

    Returns:
        A string representing the name of a generated EAP test case.
    """
    eap = WifiEnums.Eap
    eap_phase2 = WifiEnums.EapPhase2
    Ent = WifiEnums.Enterprise
    name = "test_connect-"
    eap_name = ""
    for e in eap:
        if e.value == config[Ent.EAP]:
            eap_name = e.name
            break
    if "peap0" in config[WifiEnums.SSID_KEY].lower():
        eap_name = "PEAP0"
    if "peap1" in config[WifiEnums.SSID_KEY].lower():
        eap_name = "PEAP1"
    name += eap_name
    if Ent.PHASE2 in config:
        for e in eap_phase2:
            if e.value == config[Ent.PHASE2]:
                name += "-{}".format(e.name)
                break
    return name


def group_attenuators(attenuators):
    """Groups a list of attenuators into attenuator groups for backward
    compatibility reasons.

    Most legacy Wi-Fi setups have two attenuators each connected to a separate
    AP. The new Wi-Fi setup has four attenuators, each connected to one channel
    on an AP, so two of them are connected to one AP.

    To make the existing scripts work in the new setup, when the script needs
    to attenuate one AP, it needs to set attenuation on both attenuators
    connected to the same AP.

    This function groups attenuators properly so the scripts work in both
    legacy and new Wi-Fi setups.

    Args:
        attenuators: A list of attenuator objects, either two or four in length.

    Raises:
        signals.TestFailure is raised if the attenuator list does not have two
        or four objects.
    """
    attn0 = attenuator.AttenuatorGroup("AP0")
    attn1 = attenuator.AttenuatorGroup("AP1")
    # Legacy testbed setup has two attenuation channels.
    num_of_attns = len(attenuators)
    if num_of_attns == 2:
        attn0.add(attenuators[0])
        attn1.add(attenuators[1])
    elif num_of_attns == 4:
        attn0.add(attenuators[0])
        attn0.add(attenuators[1])
        attn1.add(attenuators[2])
        attn1.add(attenuators[3])
    else:
        asserts.fail(("Either two or four attenuators are required for this "
                      "test, but found %s") % num_of_attns)
    return [attn0, attn1]

def set_attns(attenuator, attn_val_name):
    """Sets attenuation values on attenuators used in this test.

    Args:
        attenuator: The attenuator object.
        attn_val_name: Name of the attenuation value pair to use.
    """
    logging.info("Set attenuation values to %s", roaming_attn[attn_val_name])
    try:
        attenuator[0].set_atten(roaming_attn[attn_val_name][0])
        attenuator[1].set_atten(roaming_attn[attn_val_name][1])
        attenuator[2].set_atten(roaming_attn[attn_val_name][2])
        attenuator[3].set_atten(roaming_attn[attn_val_name][3])
    except:
        logging.exception("Failed to set attenuation values %s.",
                       attn_val_name)
        raise


def trigger_roaming_and_validate(dut, attenuator, attn_val_name, expected_con):
    """Sets attenuators to trigger roaming and validate the DUT connected
    to the BSSID expected.

    Args:
        attenuator: The attenuator object.
        attn_val_name: Name of the attenuation value pair to use.
        expected_con: The network information of the expected network.
    """
    expected_con = {
        WifiEnums.SSID_KEY: expected_con[WifiEnums.SSID_KEY],
        WifiEnums.BSSID_KEY: expected_con["bssid"],
    }
    set_attns(attenuator, attn_val_name)
    logging.info("Wait %ss for roaming to finish.", ROAMING_TIMEOUT)
    time.sleep(ROAMING_TIMEOUT)
    try:
        # Wakeup device and verify connection.
        dut.droid.wakeLockAcquireBright()
        dut.droid.wakeUpNow()
        cur_con = dut.droid.wifiGetConnectionInfo()
        verify_wifi_connection_info(dut, expected_con)
        expected_bssid = expected_con[WifiEnums.BSSID_KEY]
        logging.info("Roamed to %s successfully", expected_bssid)
        if not validate_connection(dut):
            raise signals.TestFailure("Fail to connect to internet on %s" %
                                      expected_ssid)
    finally:
        dut.droid.wifiLockRelease()
        dut.droid.goToSleepNow()


def create_softap_config():
    """Create a softap config with random ssid and password."""
    ap_ssid = "softap_" + utils.rand_ascii_str(8)
    ap_password = utils.rand_ascii_str(8)
    logging.info("softap setup: %s %s", ap_ssid, ap_password)
    config = {
        WifiEnums.SSID_KEY: ap_ssid,
        WifiEnums.PWD_KEY: ap_password,
    }
    return config
