# /usr/bin/env python3.4
#
# Copyright (C) 2018 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License"); you may not
# use this file except in compliance with the License. You may obtain a copy of
# the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
# License for the specific language governing permissions and limitations under
# the License

import json
import logging
import math
import os
import re
import subprocess
import time
import xlsxwriter

from acts.controllers.ap_lib import hostapd_config
from acts.controllers.ap_lib import hostapd_constants
from acts.controllers.ap_lib import hostapd_security
from acts.test_utils.bt.bt_constants import \
    bluetooth_profile_connection_state_changed
from acts.test_utils.bt.bt_constants import bt_default_timeout
from acts.test_utils.bt.bt_constants import bt_profile_constants
from acts.test_utils.bt.bt_constants import bt_profile_states
from acts.test_utils.bt.bt_constants import bt_scan_mode_types
from acts.test_utils.bt.bt_gatt_utils import GattTestUtilsError
from acts.test_utils.bt.bt_gatt_utils import orchestrate_gatt_connection
from acts.test_utils.bt.bt_test_utils import disable_bluetooth
from acts.test_utils.bt.bt_test_utils import enable_bluetooth
from acts.test_utils.bt.bt_test_utils import is_a2dp_src_device_connected
from acts.test_utils.bt.bt_test_utils import is_a2dp_snk_device_connected
from acts.test_utils.bt.bt_test_utils import is_hfp_client_device_connected
from acts.test_utils.bt.bt_test_utils import is_map_mce_device_connected
from acts.test_utils.bt.bt_test_utils import is_map_mse_device_connected
from acts.test_utils.car.car_telecom_utils import wait_for_active
from acts.test_utils.car.car_telecom_utils import wait_for_dialing
from acts.test_utils.car.car_telecom_utils import wait_for_not_in_call
from acts.test_utils.car.car_telecom_utils import wait_for_ringing
from acts.test_utils.tel.tel_test_utils import get_phone_number
from acts.test_utils.tel.tel_test_utils import hangup_call
from acts.test_utils.tel.tel_test_utils import initiate_call
from acts.test_utils.tel.tel_test_utils import run_multithread_func
from acts.test_utils.tel.tel_test_utils import setup_droid_properties
from acts.test_utils.tel.tel_test_utils import wait_and_answer_call
from acts.test_utils.wifi.wifi_test_utils import reset_wifi
from acts.test_utils.wifi.wifi_test_utils import wifi_connect
from acts.test_utils.wifi.wifi_test_utils import wifi_test_device_init
from acts.test_utils.wifi.wifi_test_utils import wifi_toggle_state
from acts.utils import exe_cmd, create_dir

THROUGHPUT_THRESHOLD = 100
AP_START_TIME = 10
DISCOVERY_TIME = 10
BLUETOOTH_WAIT_TIME = 2


def a2dp_dumpsys_parser(file_path):
    """Convenience function to parse a2dp dumpsys logs.

    Args:
        file_path: Path of dumpsys logs.

    Returns:
        True if parsing is successful, False otherwise.
    """
    a2dp_dumpsys_info = []
    with open(file_path) as dumpsys_file:
        for line in dumpsys_file:
            if "A2DP State:" in line:
                a2dp_dumpsys_info.append(line)
            elif "Counts (max dropped)" not in line and len(
                    a2dp_dumpsys_info) > 0:
                a2dp_dumpsys_info.append(line)
            elif "Counts (max dropped)" in line:
                a2dp_dumpsys_info = ''.join(a2dp_dumpsys_info)
                logging.info(a2dp_dumpsys_info)
                return True
    logging.error("failed to get A2DP state")
    return False


def connect_ble(pri_ad, sec_ad):
    """Connect BLE device from DUT.

    Args:
        pri_ad: An android device object.
        sec_ad: An android device object.

    Returns:
        True if successful, otherwise False.
    """
    adv_instances = []
    gatt_server_list = []
    bluetooth_gatt_list = []
    pri_ad.droid.bluetoothEnableBLE()
    gatt_server_cb = sec_ad.droid.gattServerCreateGattServerCallback()
    gatt_server = sec_ad.droid.gattServerOpenGattServer(gatt_server_cb)
    gatt_server_list.append(gatt_server)
    try:
        bluetooth_gatt, gatt_callback, adv_callback = (
            orchestrate_gatt_connection(pri_ad, sec_ad))
        bluetooth_gatt_list.append(bluetooth_gatt)
    except GattTestUtilsError as err:
        logging.error(err)
        return False
    adv_instances.append(adv_callback)
    connected_devices = sec_ad.droid.gattServerGetConnectedDevices(gatt_server)
    logging.debug("Connected device = {}".format(connected_devices))
    return True


def collect_bluetooth_manager_dumpsys_logs(pri_ad):
    """Collect "adb shell dumpsys bluetooth_manager" logs.

    Args:
        pri_ad : An android device.

    Returns:
        True if dumpsys is successful, False otherwise.
    """
    out_file = "{}_{}".format(pri_ad.serial, "bluetooth_dumpsys.txt")
    dumpsys_path = ''.join((pri_ad.log_path, "/BluetoothDumpsys"))
    create_dir(dumpsys_path)
    cmd = ''.join("adb -s {} shell dumpsys bluetooth_manager > {}/{}".format(
        pri_ad.serial, dumpsys_path, out_file))
    exe_cmd(cmd)
    file_path = "{}/{}".format(dumpsys_path, out_file)
    if not a2dp_dumpsys_parser(file_path):
        logging.error("Could not parse dumpsys logs")
        return False
    return True


def configure_and_start_ap(ap, network):
    """Configure hostapd parameters and starts access point.

    Args:
        ap: An access point object.
        network: A dictionary with wifi network details.
    """
    hostapd_sec = hostapd_security.Security(
        security_mode=network["security"], password=network["password"])

    config = hostapd_config.HostapdConfig(
        n_capabilities=[hostapd_constants.N_CAPABILITY_HT40_MINUS],
        mode=hostapd_constants.MODE_11N_PURE,
        channel=network["channel"],
        ssid=network["SSID"],
        security=hostapd_sec)
    ap.start_ap(config)
    time.sleep(AP_START_TIME)


def connect_dev_to_headset(pri_droid, dev_to_connect, profiles_set):
    supported_profiles = bt_profile_constants.values()
    for profile in profiles_set:
        if profile not in supported_profiles:
            pri_droid.log.info("Profile {} is not supported list {}".format(
                profile, supported_profiles))
            return False

    paired = False
    for paired_device in pri_droid.droid.bluetoothGetBondedDevices():
        if paired_device['address'] == \
                dev_to_connect:
            paired = True
            break

    if not paired:
        pri_droid.log.info("{} not paired to {}".format(
            pri_droid.droid.getBuildSerial(), dev_to_connect))
        return False

    pri_droid.droid.bluetoothConnectBonded(dev_to_connect)

    end_time = time.time() + 10
    profile_connected = set()
    sec_addr = dev_to_connect
    logging.info("Profiles to be connected {}".format(profiles_set))

    while (time.time() < end_time and
               not profile_connected.issuperset(profiles_set)):
        if (bt_profile_constants['headset_client'] not in profile_connected and
                    bt_profile_constants['headset_client'] in profiles_set):
            if is_hfp_client_device_connected(pri_droid, sec_addr):
                profile_connected.add(bt_profile_constants['headset_client'])
        if (bt_profile_constants['headset'] not in profile_connected and
                    bt_profile_constants['headset'] in profiles_set):
            profile_connected.add(bt_profile_constants['headset'])
        if (bt_profile_constants['a2dp'] not in profile_connected and
                    bt_profile_constants['a2dp'] in profiles_set):
            if is_a2dp_src_device_connected(pri_droid, sec_addr):
                profile_connected.add(bt_profile_constants['a2dp'])
        if (bt_profile_constants['a2dp_sink'] not in profile_connected and
                    bt_profile_constants['a2dp_sink'] in profiles_set):
            if is_a2dp_snk_device_connected(pri_droid, sec_addr):
                profile_connected.add(bt_profile_constants['a2dp_sink'])
        if (bt_profile_constants['map_mce'] not in profile_connected and
                    bt_profile_constants['map_mce'] in profiles_set):
            if is_map_mce_device_connected(pri_droid, sec_addr):
                profile_connected.add(bt_profile_constants['map_mce'])
        if (bt_profile_constants['map'] not in profile_connected and
                    bt_profile_constants['map'] in profiles_set):
            if is_map_mse_device_connected(pri_droid, sec_addr):
                profile_connected.add(bt_profile_constants['map'])
        time.sleep(0.1)

    while not profile_connected.issuperset(profiles_set):
        try:
            time.sleep(10)
            profile_event = pri_droid.ed.pop_event(
                bluetooth_profile_connection_state_changed,
                bt_default_timeout + 10)
            logging.info("Got event {}".format(profile_event))
        except Exception:
            logging.error("Did not get {} profiles left {}".format(
                bluetooth_profile_connection_state_changed, profile_connected))
            return False
        profile = profile_event['data']['profile']
        state = profile_event['data']['state']
        device_addr = profile_event['data']['addr']
        if state == bt_profile_states['connected'] and \
                        device_addr == dev_to_connect:
            profile_connected.add(profile)
        logging.info("Profiles connected until now {}".format(
            profile_connected))
    return True


def device_discoverable(pri_ad, sec_ad):
    """Verifies whether the device is discoverable or not.

    Args:
        pri_ad: An primary android device object.
        sec_ad: An secondary android device object.

    Returns:
        True if the device found, False otherwise.
    """
    pri_ad.droid.bluetoothMakeDiscoverable()
    scan_mode = pri_ad.droid.bluetoothGetScanMode()
    if scan_mode == bt_scan_mode_types['connectable_discoverable']:
        logging.info("Primary device scan mode is "
                     "SCAN_MODE_CONNECTABLE_DISCOVERABLE.")
    else:
        logging.info("Primary device scan mode is not "
                     "SCAN_MODE_CONNECTABLE_DISCOVERABLE.")
        return False
    if sec_ad.droid.bluetoothStartDiscovery():
        time.sleep(DISCOVERY_TIME)
        droid_name = pri_ad.droid.bluetoothGetLocalName()
        get_discovered_devices = sec_ad.droid.bluetoothGetDiscoveredDevices()
        find_flag = False

        if get_discovered_devices:
            for device in get_discovered_devices:
                if 'name' in device and device['name'] == droid_name:
                    logging.info("Primary device is in the discovery "
                                 "list of secondary device.")
                    find_flag = True
                    break
        else:
            logging.info("Secondary device get all the discovered devices "
                         "list is empty")
            return False
    else:
        logging.info("Secondary device start discovery process error.")
        return False
    if not find_flag:
        return False
    return True


def disconnect_headset_from_dev(pri_ad, sec_ad, profiles_list):
    """
    Disconnect primary from secondary on a specific set of profiles
    Args:
        pri_ad - Primary android_device initiating disconnection
        sec_ad - Secondary android droid (sl4a interface to keep the
          method signature the same connect_pri_to_sec above)
        profiles_list - List of profiles we want to disconnect from

    Returns:
        True on Success
        False on Failure
    """
    supported_profiles = bt_profile_constants.values()
    for profile in profiles_list:
        if profile not in supported_profiles:
            pri_ad.log.info("Profile {} is not in supported list {}".format(
                profile, supported_profiles))
            return False

    pri_ad.log.info(pri_ad.droid.bluetoothGetBondedDevices())

    try:
        pri_ad.droid.bluetoothDisconnectConnectedProfile(sec_ad, profiles_list)
    except Exception as err:
        pri_ad.log.error(
            "Exception while trying to disconnect profile(s) {}: {}".format(
                profiles_list, err))
        return False

    profile_disconnected = set()
    pri_ad.log.info("Disconnecting from profiles: {}".format(profiles_list))

    while not profile_disconnected.issuperset(profiles_list):
        try:
            profile_event = pri_ad.ed.pop_event(
                bluetooth_profile_connection_state_changed, bt_default_timeout)
            pri_ad.log.info("Got event {}".format(profile_event))
        except Exception:
            pri_ad.log.error("Did not disconnect from Profiles")
            return False

        profile = profile_event['data']['profile']
        state = profile_event['data']['state']
        device_addr = profile_event['data']['addr']

        if state == bt_profile_states['disconnected'] and \
                        device_addr == sec_ad:
            profile_disconnected.add(profile)
        pri_ad.log.info("Profiles disconnected so far {}".format(
            profile_disconnected))

    return True


def initiate_disconnect_from_hf(audio_receiver, pri_ad, sec_ad, duration):
    """Initiates call and disconnect call on primary device.
    Steps:
    1. Initiate call from HF.
    2. Wait for dialing state at DUT and wait for ringing at secondary device.
    3. Accepts call from secondary device.
    4. Wait for call active state at primary and secondary device.
    5. Sleeps until given duration.
    6. Disconnect call from primary device.
    7. Wait for call is not present state.

    Args:
        pri_ad: An android device to disconnect call.
        sec_ad: An android device accepting call.
        duration: Duration of call in seconds.

    Returns:
        True if successful, False otherwise.
    """
    audio_receiver.initiate_call_from_hf()
    time.sleep(2)
    flag = True
    flag &= wait_for_dialing(logging, pri_ad)
    flag &= wait_for_ringing(logging, sec_ad)
    if not flag:
        logging.error("Outgoing call did not get established")
        return False

    if not wait_and_answer_call(logging, sec_ad):
        logging.error("Failed to answer call in second device.")
        return False
    if not wait_for_active(logging, pri_ad):
        logging.error("AG not in Active state.")
        return False
    if not wait_for_active(logging, sec_ad):
        logging.error("RE not in Active state.")
        return False
    time.sleep(duration)
    if not hangup_call(logging, pri_ad):
        logging.error("Failed to hangup call.")
        return False
    flag = True
    flag &= wait_for_not_in_call(logging, pri_ad)
    flag &= wait_for_not_in_call(logging, sec_ad)
    return flag


def initiate_disconnect_call_dut(pri_ad, sec_ad, duration, callee_number):
    """Initiates call and disconnect call on primary device.
    Steps:
    1. Initiate call from DUT.
    2. Wait for dialing state at DUT and wait for ringing at secondary device.
    3. Accepts call from secondary device.
    4. Wait for call active state at primary and secondary device.
    5. Sleeps until given duration.
    6. Disconnect call from primary device.
    7. Wait for call is not present state.

    Args:
        pri_ad: An android device to disconnect call.
        sec_ad: An android device accepting call.
        duration: Duration of call in seconds.
        callee_number: Secondary device's phone number.

    Returns:
        True if successful, False otherwise.
    """
    if not initiate_call(logging, pri_ad, callee_number):
        logging.error("Failed to initiate call")
        return False
    time.sleep(2)

    flag = True
    flag &= wait_for_dialing(logging, pri_ad)
    flag &= wait_for_ringing(logging, sec_ad)
    if not flag:
        logging.error("Outgoing call did not get established")
        return False

    if not wait_and_answer_call(logging, sec_ad):
        logging.error("Failed to answer call in second device.")
        return False
    # Wait for AG, RE to go into an Active state.
    if not wait_for_active(logging, pri_ad):
        logging.error("AG not in Active state.")
        return False
    if not wait_for_active(logging, sec_ad):
        logging.error("RE not in Active state.")
        return False
    time.sleep(duration)
    if not hangup_call(logging, pri_ad):
        logging.error("Failed to hangup call.")
        return False
    flag = True
    flag &= wait_for_not_in_call(logging, pri_ad)
    flag &= wait_for_not_in_call(logging, sec_ad)

    return flag


def iperf_result(result, stream):
    """Accepts the iperf result in json format and parse the output to
    get throughput value.

    Args:
        result: contains the logs of iperf in json format.
        stream: string to indicate uplink/downlink traffic.

    Returns:
        tx_rate: Data sent from client.
        rx_rate: Data received from client.
    """
    try:
        with open(result, 'r') as iperf_data:
            time.sleep(1)
            json_data = json.load(iperf_data)
    except ValueError:
        with open(result, 'r') as iperf_data:
            # Possibly a result from interrupted iperf run, skip first line
            # and try again.
            time.sleep(1)
            lines = iperf_data.readlines()[1:]
            json_data = json.loads(''.join(lines))

    if "error" in json_data:
        logging.error(json_data["error"])
        return False

    protocol = json_data["start"]["test_start"]["protocol"]
    if protocol == "UDP":
        if "intervals" in json_data.keys():
            interval = [
                interval["sum"]["bits_per_second"] / 1e6
                for interval in json_data["intervals"]
            ]
            tx_rate = math.fsum(interval) / len(interval)
        else:
            logging.error("Unable to retrive client results")
            return False
        if "server_output_json" in json_data.keys():
            interval = [
                interval["sum"]["bits_per_second"] / 1e6
                for interval in json_data["server_output_json"]["intervals"]
            ]
            rx_rate = math.fsum(interval) / len(interval)
        else:
            logging.info("unable to retrive server results")
            return False
        if not stream == "ul":
            return tx_rate, rx_rate
        return rx_rate, tx_rate

    elif protocol == "TCP":
        tx_rate = json_data['end']['sum_sent']['bits_per_second']
        rx_rate = json_data['end']['sum_received']['bits_per_second']
        return tx_rate / 1e6, rx_rate / 1e6
    else:
        return False


def is_a2dp_connected(pri_ad, headset_mac_address):
    """Convenience Function to see if the 2 devices are connected on A2DP.

    Args:
        pri_ad : An android device.
        headset_mac_address : Mac address of headset.

    Returns:
        True:If A2DP connection exists, False otherwise.
    """
    devices = pri_ad.droid.bluetoothA2dpGetConnectedDevices()
    for device in devices:
        logging.debug("A2dp Connected device {}".format(device["name"]))
        if device["address"] == headset_mac_address:
            return True
    return False


def media_stream_check(pri_ad, duration, headset_mac_address):
    """Checks whether A2DP connecion is active or not for given duration of
    time.

    Args:
        pri_ad : An android device.
        duration : No of seconds to check if a2dp streaming is alive.
        headset_mac_address : Headset mac address.

    Returns:
        True: If A2dp connection is active for entire duration.
        False: If A2dp connection is not active.
    """
    while time.time() < duration:
        if not is_a2dp_connected(pri_ad, headset_mac_address):
            logging.error("A2dp connection not active at {}".format(
                pri_ad.droid.getBuildSerial()))
            return False
        time.sleep(1)
    return True


def multithread_func(log, tasks):
    """Multi-thread function wrapper.

    Args:
        log: log object.
        tasks: tasks to be executed in parallel.

    Returns:
       List of results of tasks
    """
    results = run_multithread_func(log, tasks)
    for res in results:
        if not res:
            return False
    return True


def music_play_and_check(pri_ad, headset_mac_address, music_to_play, duration):
    """Starts playing media and checks if media plays for n seconds.

    Steps:
    1. Starts media player on android device.
    2. Checks if music streaming is ongoing for n seconds.
    3. Stops media player.
    4. Collect dumpsys logs.

    Args:
        pri_ad: An android device.
        headset_mac_address: Mac address of third party headset.
        music_to_play: Indicates the music file to play.
        duration: Time in secs to indicate music time to play.

    Returns:
        True if successful, False otherwise.
    """
    if not start_media_play(pri_ad, music_to_play):
        logging.error("Start media play failed.")
        return False
    stream_time = time.time() + duration
    if not media_stream_check(pri_ad, stream_time, headset_mac_address):
        logging.error("A2DP Connection check failed.")
        pri_ad.droid.mediaPlayStop()
        return False
    pri_ad.droid.mediaPlayStop()
    if not collect_bluetooth_manager_dumpsys_logs(pri_ad):
        return False
    return True


def music_play_and_check_via_app(pri_ad, headset_mac_address):
    """Starts google music player and check for A2DP connection.

    Steps:
    1. Starts Google music player on android device.
    2. Checks for A2DP connection.

    Args:
        pri_ad: An android device.
        headset_mac_address: Mac address of third party headset.

    Returns:
        True if successful, False otherwise.
    """
    pri_ad.adb.shell("am start com.google.android.music")
    time.sleep(3)
    pri_ad.adb.shell("input keyevent 85")

    if not is_a2dp_connected(pri_ad, headset_mac_address):
        logging.error("A2dp connection not active at {}".format(
            pri_ad.droid.getBuildSerial()))
        return False
    return True


def get_phone_ip(ad):
    """Get the WiFi IP address of the phone.

    Args:
        ad: the android device under test

    Returns:
        Ip address of the phone for WiFi, as a string
    """
    return ad.droid.connectivityGetIPv4Addresses('wlan0')[0]


def pair_dev_to_headset(pri_ad, dev_to_pair):

    bonded_devices = pri_ad.droid.bluetoothGetBondedDevices()
    for d in bonded_devices:
        if d['address'] == dev_to_pair:
            pri_ad.log.info("Successfully bonded to device".format(
                dev_to_pair))
            return True
    pri_ad.droid.bluetoothStartDiscovery()
    time.sleep(10)
    pri_ad.droid.bluetoothCancelDiscovery()
    logging.info("disovered devices = {}".format(
        pri_ad.droid.bluetoothGetDiscoveredDevices()))
    for device in pri_ad.droid.bluetoothGetDiscoveredDevices():
        if device['address'] == dev_to_pair:

            result = pri_ad.droid.bluetoothDiscoverAndBond(dev_to_pair)
            pri_ad.log.info(result)
            end_time = time.time() + bt_default_timeout
            pri_ad.log.info("Verifying devices are bonded")
            time.sleep(5)
            while time.time() < end_time:
                bonded_devices = pri_ad.droid.bluetoothGetBondedDevices()
                bonded = False
                for d in bonded_devices:
                    if d['address'] == dev_to_pair:
                        pri_ad.log.info("Successfully bonded to device".format(
                            dev_to_pair))
                        return True
    pri_ad.log.info("Failed to bond devices.")
    return False

def pair_and_connect_headset(pri_ad, headset_mac_address, profile_to_connect):
    """Pair and connect android device with third party headset.

    Args:
        pri_ad: An android device.
        headset_mac_address: Mac address of third party headset.
        profile_to_connect: Profile to be connected with headset.

    Returns:
        True if pair and connect to headset successful, False otherwise.
    """
    if not pair_dev_to_headset(pri_ad, headset_mac_address):
        logging.error("Could not pair to headset.")
        return False
    # Wait until pairing gets over.
    time.sleep(2)
    if not connect_dev_to_headset(pri_ad, headset_mac_address,
                                  profile_to_connect):
        logging.error("Could not connect to headset.")
        return False
    return True


def perform_classic_discovery(pri_ad):
    """Convenience function to start and stop device discovery.

    Args:
        pri_ad: An android device.

    Returns:
        True start and stop discovery is successful, False otherwise.
    """
    if not pri_ad.droid.bluetoothStartDiscovery():
        logging.info("Failed to start inquiry")
        return False
    time.sleep(DISCOVERY_TIME)
    if not pri_ad.droid.bluetoothCancelDiscovery():
        logging.info("Failed to cancel inquiry")
        return False
    logging.info("Discovered device list {}".format(
        pri_ad.droid.bluetoothGetDiscoveredDevices()))
    return True


def connect_wlan_profile(pri_ad, network):
    """Disconnect and Connect to AP.

    Args:
        pri_ad: An android device.
        network: Network to which AP to be connected.

    Returns:
        True if successful, False otherwise.
    """
    reset_wifi(pri_ad)
    wifi_toggle_state(pri_ad, False)
    wifi_test_device_init(pri_ad)
    wifi_connect(pri_ad, network)
    if not wifi_connection_check(pri_ad, network["SSID"]):
        logging.error("Wifi connection does not exist.")
        return False
    return True


def toggle_bluetooth(pri_ad, iterations):
    """Toggles bluetooth on/off for N iterations.

    Args:
        pri_ad: An android device object.
        iterations: Number of iterations to run.

    Returns:
        True if successful, False otherwise.
    """
    for i in range(iterations):
        logging.info("Bluetooth Turn on/off iteration : {}".format(i))
        if not enable_bluetooth(pri_ad.droid, pri_ad.ed):
            logging.error("Failed to enable bluetooth")
            return False
        time.sleep(BLUETOOTH_WAIT_TIME)
        if not disable_bluetooth(pri_ad.droid):
            logging.error("Failed to turn off bluetooth")
            return False
        time.sleep(BLUETOOTH_WAIT_TIME)
    return True


def toggle_screen_state(pri_ad, iterations):
    """Toggles the screen state to on or off..

    Args:
        pri_ad: Android device.
        iterations: Number of times screen on/off should be performed.

    Returns:
        True if successful, False otherwise.
    """
    for i in range(iterations):
        if not pri_ad.ensure_screen_on():
            logging.error("User window cannot come up")
            return False
        if not pri_ad.go_to_sleep():
            logging.info("Screen off")
    return True


def setup_tel_config(pri_ad, sec_ad, sim_conf_file):
    """Sets tel properties for primary device and secondary devices

    Args:
        pri_ad: An android device object.
        sec_ad: An android device object.
        sim_conf_file: Sim card map.

    Returns:
        pri_ad_num: Phone number of primary device.
        sec_ad_num: Phone number of secondary device.
    """
    setup_droid_properties(logging, pri_ad, sim_conf_file)
    pri_ad_num = get_phone_number(logging, pri_ad)
    setup_droid_properties(logging, sec_ad, sim_conf_file)
    sec_ad_num = get_phone_number(logging, sec_ad)
    return pri_ad_num, sec_ad_num


def start_fping(pri_ad, duration):
    """Starts fping to ping for DUT's ip address.

    Steps:
    1. Run fping command to check DUT's IP is alive or not.

    Args:
        pri_ad: An android device object.
        duration: Duration of fping in seconds.

    Returns:
        True if successful, False otherwise.
    """
    out_file_name = "{}".format("fping.txt")
    full_out_path = os.path.join(pri_ad.log_path, out_file_name)
    cmd = "fping {} -D -c {}".format(get_phone_ip(pri_ad), duration)
    cmd = cmd.split()
    with open(full_out_path, "w") as f:
        subprocess.call(cmd, stdout=f)
    f = open(full_out_path, "r")
    for lines in f:
        l = re.split("[:/=]", lines)
        if l[len(l) - 1] != "0%":
            logging.error("Packet drop observed")
            return False
    return True


def start_media_play(pri_ad, music_file_to_play):
    """Starts media player on device.

    Args:
        pri_ad : An android device.
        music_file_to_play : An audio file to play.

    Returns:
        True:If media player start music, False otherwise.
    """
    if not pri_ad.droid.mediaPlayOpen("file:///sdcard/Music/{}"
                                      .format(music_file_to_play)):
        logging.error("Failed to play music")
        return False

    pri_ad.droid.mediaPlaySetLooping(True)
    logging.info("Music is now playing on device {}".format(pri_ad.serial))
    return True


def throughput_pass_fail_check(throughput):
    """Method to check if the throughput is above the defined
    threshold.

    Args:
        throughput: Throughput value of test run.

    Returns:
        Pass if throughput is above threshold.
        Fail if throughput is below threshold and Iperf Failed.
        Empty if Iperf value is not present.
    """
    iperf_result_list = []
    if len(throughput) != 0:
        for i in range(len(throughput)):
            if throughput[i] != 'Iperf Failed':
                if float(throughput[i].split('Mb/s')[0]) > \
                        THROUGHPUT_THRESHOLD:
                    iperf_result_list.append("PASS")
                else:
                    iperf_result_list.append("FAIL")
            elif throughput[i] == 'Iperf Failed':
                iperf_result_list.append("FAIL")
        return "FAIL" if "FAIL" or "Iperf Failed" in iperf_result_list \
            else "PASS"
    else:
        return " "


def wifi_connection_check(pri_ad, ssid):
    """Function to check existence of wifi connection.

    Args:
        pri_ad : An android device.
        ssid : wifi ssid to check.

    Returns:
        True if wifi connection exists, False otherwise.
    """
    wifi_info = pri_ad.droid.wifiGetConnectionInfo()
    if (wifi_info["SSID"] == ssid and
            wifi_info["supplicant_state"] == "completed"):
        return True
    logging.error("Wifi Connection check failed : {}".format(wifi_info))
    return False


def xlsheet(pri_ad, test_result, attenuation_range=range(0, 1, 1)):
    """Parses the output and writes in spreadsheet.

    Args:
        pri_ad: An android device.
        test_result: final output of testrun.
        attenuation_range: list of attenuation values.
    """
    r = 0
    c = 0
    i = 0
    test_result = json.loads(test_result)
    file_name = '/'.join((pri_ad.log_path,
                          test_result["Results"][0]["Test Class"] + ".xlsx"))
    workbook = xlsxwriter.Workbook(file_name)
    worksheet = workbook.add_worksheet()
    wb_format = workbook.add_format()
    wb_format.set_text_wrap()
    worksheet.set_column('A:A', 50)
    worksheet.set_column('B:B', 10)
    wb_format = workbook.add_format({'bold': True})
    worksheet.write(r, c, "Test_case_name", wb_format)
    worksheet.write(r, c + 1, "Result", wb_format)
    if len(attenuation_range) > 1:
        for idx in attenuation_range:
            c += 1
            worksheet.write(r, c + 1, str(idx) + "db", wb_format)
    else:
        worksheet.write(r, c + 2, "Iperf_throughput", wb_format)
        c += 1

    worksheet.write(r, (c + 2), "Throughput_Result", wb_format)
    result = [(i["Test Name"], i["Result"], (i["Extras"]),
               throughput_pass_fail_check(i["Extras"]))
              for i in test_result["Results"]]

    for row, line in enumerate(result):
        for col, cell in enumerate(line):
            if isinstance(cell, list):
                for i in range(len(cell)):
                    worksheet.write(row + 1, col, cell[i])
                    col += 1
            else:
                worksheet.write(row + 1, col + i, cell)
