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

import time

from acts.utils import rand_ascii_str
from acts.test_utils.tel.tel_subscription_utils import \
    get_subid_from_slot_index
from acts.test_utils.tel.tel_subscription_utils import set_subid_for_data
from acts.test_utils.tel.tel_defines import MAX_WAIT_TIME_NW_SELECTION
from acts.test_utils.tel.tel_defines import NETWORK_SERVICE_DATA
from acts.test_utils.tel.tel_defines import WAIT_TIME_ANDROID_STATE_SETTLING
from acts.test_utils.tel.tel_subscription_utils import get_default_data_sub_id
from acts.test_utils.tel.tel_test_utils import start_wifi_tethering
from acts.test_utils.tel.tel_test_utils import stop_wifi_tethering
from acts.test_utils.tel.tel_test_utils import ensure_network_generation_for_subscription
from acts.test_utils.tel.tel_test_utils import ensure_phones_idle
from acts.test_utils.tel.tel_test_utils import ensure_wifi_connected
from acts.test_utils.tel.tel_test_utils import get_network_rat_for_subscription
from acts.test_utils.tel.tel_test_utils import is_droid_in_network_generation_for_subscription
from acts.test_utils.tel.tel_test_utils import rat_generation_from_rat
from acts.test_utils.tel.tel_test_utils import set_wifi_to_default
from acts.test_utils.tel.tel_test_utils import toggle_airplane_mode
from acts.test_utils.tel.tel_test_utils import verify_internet_connection
from acts.test_utils.tel.tel_test_utils import wait_for_cell_data_connection
from acts.test_utils.tel.tel_test_utils import wait_for_wifi_data_connection
from acts.test_utils.tel.tel_test_utils import wait_for_data_attach_for_subscription
from acts.test_utils.tel.tel_test_utils import wifi_toggle_state
from acts.test_utils.tel.tel_test_utils import WIFI_CONFIG_APBAND_2G
from acts.test_utils.tel.tel_test_utils import WIFI_CONFIG_APBAND_5G


def wifi_tethering_cleanup(log, provider, client_list):
    """Clean up steps for WiFi Tethering.

    Make sure provider turn off tethering.
    Make sure clients reset WiFi and turn on cellular data.

    Args:
        log: log object.
        provider: android object provide WiFi tethering.
        client_list: a list of clients using tethered WiFi.

    Returns:
        True if no error happened. False otherwise.
    """
    for client in client_list:
        client.droid.telephonyToggleDataConnection(True)
        set_wifi_to_default(log, client)
    if not stop_wifi_tethering(log, provider):
        provider.log.error("Provider stop WiFi tethering failed.")
        return False
    if provider.droid.wifiIsApEnabled():
        provider.log.error("Provider WiFi tethering is still enabled.")
        return False
    return True


def wifi_tethering_setup_teardown(log,
                                  provider,
                                  client_list,
                                  ap_band=WIFI_CONFIG_APBAND_2G,
                                  check_interval=30,
                                  check_iteration=4,
                                  do_cleanup=True,
                                  ssid=None,
                                  password=None):
    """Test WiFi Tethering.

    Turn off WiFi on clients.
    Turn off data and reset WiFi on clients.
    Verify no Internet access on clients.
    Turn on WiFi tethering on provider.
    Clients connect to provider's WiFI.
    Verify Internet on provider and clients.
    Tear down WiFi tethering setup and clean up.

    Args:
        log: log object.
        provider: android object provide WiFi tethering.
        client_list: a list of clients using tethered WiFi.
        ap_band: setup WiFi tethering on 2G or 5G.
            This is optional, default value is WIFI_CONFIG_APBAND_2G
        check_interval: delay time between each around of Internet connection check.
            This is optional, default value is 30 (seconds).
        check_iteration: check Internet connection for how many times in total.
            This is optional, default value is 4 (4 times).
        do_cleanup: after WiFi tethering test, do clean up to tear down tethering
            setup or not. This is optional, default value is True.
        ssid: use this string as WiFi SSID to setup tethered WiFi network.
            This is optional. Default value is None.
            If it's None, a random string will be generated.
        password: use this string as WiFi password to setup tethered WiFi network.
            This is optional. Default value is None.
            If it's None, a random string will be generated.

    Returns:
        True if no error happened. False otherwise.
    """
    log.info("--->Start wifi_tethering_setup_teardown<---")
    log.info("Provider: {}".format(provider.serial))
    if not provider.droid.connectivityIsTetheringSupported():
        provider.log.error(
            "Provider does not support tethering. Stop tethering test.")
        return False

    if ssid is None:
        ssid = rand_ascii_str(10)
    if password is None:
        password = rand_ascii_str(8)

    # No password
    if password == "":
        password = None

    try:
        for client in client_list:
            log.info("Client: {}".format(client.serial))
            wifi_toggle_state(log, client, False)
            client.droid.telephonyToggleDataConnection(False)
        log.info("WiFI Tethering: Verify client have no Internet access.")
        for client in client_list:
            if verify_internet_connection(log, client):
                client.log.error("Turn off Data on client fail")
                return False

        provider.log.info(
            "Provider turn on WiFi tethering. SSID: %s, password: %s", ssid,
            password)

        if not start_wifi_tethering(log, provider, ssid, password, ap_band):
            provider.log.error("Provider start WiFi tethering failed.")
            return False
        time.sleep(WAIT_TIME_ANDROID_STATE_SETTLING)

        provider.log.info("Provider check Internet connection.")
        if not verify_internet_connection(log, provider):
            return False
        for client in client_list:
            client.log.info(
                "Client connect to WiFi and verify AP band correct.")
            if not ensure_wifi_connected(log, client, ssid, password):
                client.log.error("Client connect to WiFi failed.")
                return False

            wifi_info = client.droid.wifiGetConnectionInfo()
            if ap_band == WIFI_CONFIG_APBAND_5G:
                if wifi_info["is_24ghz"]:
                    client.log.error("Expected 5g network. WiFi Info: %s",
                                     wifi_info)
                    return False
            else:
                if wifi_info["is_5ghz"]:
                    client.log.error("Expected 2g network. WiFi Info: %s",
                                     wifi_info)
                    return False

            client.log.info("Client check Internet connection.")
            if (not wait_for_wifi_data_connection(log, client, True) or
                    not verify_internet_connection(log, client)):
                client.log.error("No WiFi Data on client")
                return False

        if not tethering_check_internet_connection(
                log, provider, client_list, check_interval, check_iteration):
            return False

    finally:
        if (do_cleanup and
            (not wifi_tethering_cleanup(log, provider, client_list))):
            return False
    return True


def tethering_check_internet_connection(log, provider, client_list,
                                        check_interval, check_iteration):
    """During tethering test, check client(s) and provider Internet connection.

    Do the following for <check_iteration> times:
        Delay <check_interval> seconds.
        Check Tethering provider's Internet connection.
        Check each client's Internet connection.

    Args:
        log: log object.
        provider: android object provide WiFi tethering.
        client_list: a list of clients using tethered WiFi.
        check_interval: delay time between each around of Internet connection check.
        check_iteration: check Internet connection for how many times in total.

    Returns:
        True if no error happened. False otherwise.
    """
    for i in range(1, check_iteration + 1):
        result = True
        time.sleep(check_interval)
        provider.log.info(
            "Provider check Internet connection after %s seconds.",
            check_interval * i)
        if not verify_internet_connection(log, provider):
            result = False
            continue
        for client in client_list:
            client.log.info(
                "Client check Internet connection after %s seconds.",
                check_interval * i)
            if not verify_internet_connection(log, client):
                result = False
                break
        if result: return result
    return False


def wifi_cell_switching(log, ad, wifi_network_ssid, wifi_network_pass, nw_gen):
    """Test data connection network switching when phone on <nw_gen>.

    Ensure phone is on <nw_gen>
    Ensure WiFi can connect to live network,
    Airplane mode is off, data connection is on, WiFi is on.
    Turn off WiFi, verify data is on cell and browse to google.com is OK.
    Turn on WiFi, verify data is on WiFi and browse to google.com is OK.
    Turn off WiFi, verify data is on cell and browse to google.com is OK.

    Args:
        log: log object.
        ad: android object.
        wifi_network_ssid: ssid for live wifi network.
        wifi_network_pass: password for live wifi network.
        nw_gen: network generation the phone should be camped on.

    Returns:
        True if pass.
    """
    try:

        if not ensure_network_generation_for_subscription(
                log, ad,
                get_default_data_sub_id(ad), nw_gen,
                MAX_WAIT_TIME_NW_SELECTION, NETWORK_SERVICE_DATA):
            ad.log.error("Device failed to register in %s", nw_gen)
            return False

        # Ensure WiFi can connect to live network
        ad.log.info("Make sure phone can connect to live network by WIFI")
        if not ensure_wifi_connected(log, ad, wifi_network_ssid,
                                     wifi_network_pass):
            ad.log.error("WiFi connect fail.")
            return False
        log.info("Phone connected to WIFI.")

        log.info("Step1 Airplane Off, WiFi On, Data On.")
        toggle_airplane_mode(log, ad, False)
        wifi_toggle_state(log, ad, True)
        ad.droid.telephonyToggleDataConnection(True)
        if (not wait_for_wifi_data_connection(log, ad, True) or
                not verify_internet_connection(log, ad)):
            ad.log.error("Data is not on WiFi")
            return False

        log.info("Step2 WiFi is Off, Data is on Cell.")
        wifi_toggle_state(log, ad, False)
        if (not wait_for_cell_data_connection(log, ad, True) or
                not verify_internet_connection(log, ad)):
            ad.log.error("Data did not return to cell")
            return False

        log.info("Step3 WiFi is On, Data is on WiFi.")
        wifi_toggle_state(log, ad, True)
        if (not wait_for_wifi_data_connection(log, ad, True) or
                not verify_internet_connection(log, ad)):
            ad.log.error("Data did not return to WiFi")
            return False

        log.info("Step4 WiFi is Off, Data is on Cell.")
        wifi_toggle_state(log, ad, False)
        if (not wait_for_cell_data_connection(log, ad, True) or
                not verify_internet_connection(log, ad)):
            ad.log.error("Data did not return to cell")
            return False
        return True

    finally:
        wifi_toggle_state(log, ad, False)


def airplane_mode_test(log, ad, retries=3):
    """ Test airplane mode basic on Phone and Live SIM.

    Ensure phone attach, data on, WiFi off and verify Internet.
    Turn on airplane mode to make sure detach.
    Turn off airplane mode to make sure attach.
    Verify Internet connection.

    Args:
        log: log object.
        ad: android object.

    Returns:
        True if pass; False if fail.
    """
    if not ensure_phones_idle(log, [ad]):
        log.error("Failed to return phones to idle.")
        return False

    try:
        ad.droid.telephonyToggleDataConnection(True)
        wifi_toggle_state(log, ad, False)

        log.info("Step1: ensure attach")
        if not toggle_airplane_mode(log, ad, False):
            ad.log.error("Failed initial attach")
            return False
        for i in range(retries):
            if verify_internet_connection(log, ad):
                ad.log.info("Data available on cell.")
                break
            elif i == retries - 1:
                ad.log.error("Data not available on cell.")
                return False
            else:
                ad.log.warning("Attempt %d Data not available on cell" %
                               (i + 1))

        log.info("Step2: enable airplane mode and ensure detach")
        if not toggle_airplane_mode(log, ad, True):
            ad.log.error("Failed to enable Airplane Mode")
            return False
        if not wait_for_cell_data_connection(log, ad, False):
            ad.log.error("Failed to disable cell data connection")
            return False
        if verify_internet_connection(log, ad):
            ad.log.error("Data available in airplane mode.")
            return False

        log.info("Step3: disable airplane mode and ensure attach")
        if not toggle_airplane_mode(log, ad, False):
            ad.log.error("Failed to disable Airplane Mode")
            return False

        if not wait_for_cell_data_connection(log, ad, True):
            ad.log.error("Failed to enable cell data connection")
            return False

        time.sleep(WAIT_TIME_ANDROID_STATE_SETTLING)

        log.info("Step4 verify internet")
        for i in range(retries):
            if verify_internet_connection(log, ad):
                ad.log.info("Data available on cell.")
                break
            elif i == retries - 1:
                ad.log.error("Data not available on cell.")
                return False
            else:
                ad.log.warning("Attempt %d Data not available on cell" %
                               (i + 1))
        return True
    finally:
        toggle_airplane_mode(log, ad, False)


def data_connectivity_single_bearer(log, ad, nw_gen):
    """Test data connection: single-bearer (no voice).

    Turn off airplane mode, enable Cellular Data.
    Ensure phone data generation is expected.
    Verify Internet.
    Disable Cellular Data, verify Internet is inaccessible.
    Enable Cellular Data, verify Internet.

    Args:
        log: log object.
        ad: android object.
        nw_gen: network generation the phone should on.

    Returns:
        True if success.
        False if failed.
    """
    ensure_phones_idle(log, [ad])
    wait_time = MAX_WAIT_TIME_NW_SELECTION
    if getattr(ad, 'roaming', False):
        wait_time = 2 * wait_time
    if not ensure_network_generation_for_subscription(
            log, ad,
            get_default_data_sub_id(ad), nw_gen, MAX_WAIT_TIME_NW_SELECTION,
            NETWORK_SERVICE_DATA):
        ad.log.error("Device failed to connect to %s in %s seconds.", nw_gen,
                     wait_time)
        return False

    try:
        log.info("Step1 Airplane Off, Data On.")
        toggle_airplane_mode(log, ad, False)
        ad.droid.telephonyToggleDataConnection(True)
        if not wait_for_cell_data_connection(log, ad, True):
            ad.log.error("Failed to enable data connection.")
            return False

        log.info("Step2 Verify internet")
        if not verify_internet_connection(log, ad):
            ad.log.error("Data not available on cell.")
            return False

        log.info("Step3 Turn off data and verify not connected.")
        ad.droid.telephonyToggleDataConnection(False)
        if not wait_for_cell_data_connection(log, ad, False):
            ad.log.error("Step3 Failed to disable data connection.")
            return False

        if verify_internet_connection(log, ad):
            ad.log.error("Step3 Data still available when disabled.")
            return False

        log.info("Step4 Re-enable data.")
        ad.droid.telephonyToggleDataConnection(True)
        if not wait_for_cell_data_connection(log, ad, True):
            ad.log.error("Step4 failed to re-enable data.")
            return False
        if not verify_internet_connection(log, ad):
            ad.log.error("Data not available on cell.")
            return False

        if not is_droid_in_network_generation_for_subscription(
                log, ad,
                get_default_data_sub_id(ad), nw_gen, NETWORK_SERVICE_DATA):
            ad.log.error("Failed: droid is no longer on correct network")
            ad.log.info(
                "Expected:%s, Current:%s", nw_gen,
                rat_generation_from_rat(
                    get_network_rat_for_subscription(
                        log, ad,
                        get_default_data_sub_id(ad), NETWORK_SERVICE_DATA)))
            return False
        return True
    finally:
        ad.droid.telephonyToggleDataConnection(True)


def change_data_sim_and_verify_data(log, ad, sim_slot):
    """Change Data SIM and verify Data attach and Internet access

    Args:
        log: log object.
        ad: android device object.
        sim_slot: SIM slot index.

    Returns:
        Data SIM changed successfully, data attached and Internet access is OK.
    """
    sub_id = get_subid_from_slot_index(log, ad, sim_slot)
    ad.log.info("Change Data to subId: %s, SIM slot: %s", sub_id, sim_slot)
    set_subid_for_data(ad, sub_id)
    if not wait_for_data_attach_for_subscription(log, ad, sub_id,
                                                 MAX_WAIT_TIME_NW_SELECTION):
        ad.log.error("Failed to attach data on subId:%s", sub_id)
        return False
    if not verify_internet_connection(log, ad):
        ad.log.error("No Internet access after changing Data SIM.")
        return False
    return True
