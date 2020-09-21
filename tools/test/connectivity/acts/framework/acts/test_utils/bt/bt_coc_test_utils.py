#/usr/bin/env python3.4
#
# Copyright 2018 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

import logging
import time
from acts import utils

from acts.test_utils.bt.bt_constants import bt_default_timeout
from acts.test_utils.bt.bt_constants import default_bluetooth_socket_timeout_ms
from acts.test_utils.bt.bt_constants import default_le_connection_interval_ms
from acts.test_utils.bt.bt_constants import default_le_data_length
from acts.test_utils.bt.bt_constants import gatt_phy
from acts.test_utils.bt.bt_constants import gatt_transport
from acts.test_utils.bt.bt_constants import l2cap_coc_header_size
from acts.test_utils.bt.bt_constants import le_connection_event_time_step_ms
from acts.test_utils.bt.bt_constants import le_connection_interval_time_step_ms
from acts.test_utils.bt.bt_constants import le_default_supervision_timeout
from acts.test_utils.bt.bt_test_utils import get_mac_address_of_generic_advertisement
from acts.test_utils.bt.bt_gatt_utils import setup_gatt_connection
from acts.test_utils.bt.bt_gatt_utils import disconnect_gatt_connection

log = logging


class BtCoCTestUtilsError(Exception):
    pass


def do_multi_connection_throughput(client_ad, list_server_ad,
                                   list_client_conn_id, num_iterations,
                                   number_buffers, buffer_size):
    """Throughput measurements from one client to one-or-many servers.

    Args:
        client_ad: the Android device to perform the write.
        list_server_ad: the list of Android server devices connected to this client.
        list_client_conn_id: list of client connection IDs
        num_iterations: the number of test repetitions.
        number_buffers: the total number of data buffers to transmit per test.
        buffer_size: the number of bytes per L2CAP data buffer.

    Returns:
        Throughput in terms of bytes per second, 0 if test failed.
    """

    total_num_bytes = 0
    start_write_time = time.perf_counter()
    client_ad.log.info(
        "do_multi_connection_throughput: Before write. Start Time={:f}, "
        "num_iterations={}, number_buffers={}, buffer_size={}, "
        "number_buffers*buffer_size={}, num_servers={}".format(
            start_write_time, num_iterations, number_buffers, buffer_size,
            number_buffers * buffer_size, len(list_server_ad)))

    if (len(list_server_ad) != len(list_client_conn_id)):
        client_ad.log.error("do_multi_connection_throughput: invalid "
                            "parameters. Num of list_server_ad({}) != "
                            "list_client_conn({})".format(
                                len(list_server_ad), len(list_client_conn_id)))
        return 0

    try:
        for _, client_conn_id in enumerate(list_client_conn_id):
            client_ad.log.info("do_multi_connection_throughput: "
                               "client_conn_id={}".format(client_conn_id))
            # Plumb the tx data queue with the first set of data buffers.
            client_ad.droid.bluetoothConnectionThroughputSend(
                number_buffers, buffer_size, client_conn_id)
    except Exception as err:
        client_ad.log.error("Failed to write data: {}".format(err))
        return 0

    # Each Loop iteration will write and read one set of buffers.
    for _ in range(0, (num_iterations - 1)):
        try:
            for _, client_conn_id in enumerate(list_client_conn_id):
                client_ad.droid.bluetoothConnectionThroughputSend(
                    number_buffers, buffer_size, client_conn_id)
        except Exception as err:
            client_ad.log.error("Failed to write data: {}".format(err))
            return 0

        for _, server_ad in enumerate(list_server_ad):
            try:
                server_ad.droid.bluetoothConnectionThroughputRead(
                    number_buffers, buffer_size)
                total_num_bytes += number_buffers * buffer_size
            except Exception as err:
                server_ad.log.error("Failed to read data: {}".format(err))
                return 0

    for _, server_ad in enumerate(list_server_ad):
        try:
            server_ad.droid.bluetoothConnectionThroughputRead(
                number_buffers, buffer_size)
            total_num_bytes += number_buffers * buffer_size
        except Exception as err:
            server_ad.log.error("Failed to read data: {}".format(err))
            return 0

    end_read_time = time.perf_counter()

    test_time = (end_read_time - start_write_time)
    if (test_time == 0):
        client_ad.log.error("Buffer transmits cannot take zero time")
        return 0
    data_rate = (1.000 * total_num_bytes) / test_time
    log.info(
        "Calculated using total write and read times: total_num_bytes={}, "
        "test_time={}, data rate={:08.0f} bytes/sec, {:08.0f} bits/sec".format(
            total_num_bytes, test_time, data_rate, (data_rate * 8)))
    return data_rate


def orchestrate_coc_connection(
        client_ad,
        server_ad,
        is_ble,
        secured_conn=False,
        le_connection_interval=0,
        le_tx_data_length=default_le_data_length,
        accept_timeout_ms=default_bluetooth_socket_timeout_ms,
        le_min_ce_len=0,
        le_max_ce_len=0):
    """Sets up the CoC connection between two Android devices.

    Args:
        client_ad: the Android device performing the connection.
        server_ad: the Android device accepting the connection.
        is_ble: using LE transport.
        secured_conn: using secured connection
        le_connection_interval: LE Connection interval. 0 means use default.
        le_tx_data_length: LE Data Length used by BT Controller to transmit.
        accept_timeout_ms: timeout while waiting for incoming connection.
    Returns:
        True if connection was successful or false if unsuccessful,
        client connection ID,
        and server connection ID
    """
    server_ad.droid.bluetoothStartPairingHelper()
    client_ad.droid.bluetoothStartPairingHelper()

    adv_callback = None
    mac_address = None
    if is_ble:
        try:
            # This will start advertising and scanning. Will fail if it could
            # not find the advertisements from server_ad
            client_ad.log.info(
                "Orchestrate_coc_connection: Start BLE advertisement and"
                "scanning. Secured Connection={}".format(secured_conn))
            mac_address, adv_callback, scan_callback = (
                get_mac_address_of_generic_advertisement(client_ad, server_ad))
        except BtTestUtilsError as err:
            raise BtCoCTestUtilsError(
                "Orchestrate_coc_connection: Error in getting mac address: {}".
                format(err))
    else:
        mac_address = server_ad.droid.bluetoothGetLocalAddress()
        adv_callback = None

    # Adjust the Connection Interval (if necessary)
    bluetooth_gatt_1 = -1
    gatt_callback_1 = -1
    gatt_connected = False
    if is_ble and (le_connection_interval != 0 or le_min_ce_len != 0 or le_max_ce_len != 0):
        client_ad.log.info(
            "Adjusting connection interval={}, le_min_ce_len={}, le_max_ce_len={}"
            .format(le_connection_interval, le_min_ce_len, le_max_ce_len))
        try:
            bluetooth_gatt_1, gatt_callback_1 = setup_gatt_connection(
                client_ad,
                mac_address,
                False,
                transport=gatt_transport['le'],
                opportunistic=False)
            client_ad.droid.bleStopBleScan(scan_callback)
        except GattTestUtilsError as err:
            client_ad.log.error(err)
            if (adv_callback != None):
                server_ad.droid.bleStopBleAdvertising(adv_callback)
            return False, None, None
        client_ad.log.info("setup_gatt_connection returns success")
        if (le_connection_interval != 0):
            minInterval = le_connection_interval / le_connection_interval_time_step_ms
            maxInterval = le_connection_interval / le_connection_interval_time_step_ms
        else:
            minInterval = default_le_connection_interval_ms / le_connection_interval_time_step_ms
            maxInterval = default_le_connection_interval_ms / le_connection_interval_time_step_ms
        if (le_min_ce_len != 0):
            le_min_ce_len = le_min_ce_len / le_connection_event_time_step_ms
        if (le_max_ce_len != 0):
            le_max_ce_len = le_max_ce_len / le_connection_event_time_step_ms

        return_status = client_ad.droid.gattClientRequestLeConnectionParameters(
            bluetooth_gatt_1, minInterval, maxInterval, 0,
            le_default_supervision_timeout, le_min_ce_len, le_max_ce_len)
        if not return_status:
            client_ad.log.error(
                "gattClientRequestLeConnectionParameters returns failure")
            if (adv_callback != None):
                server_ad.droid.bleStopBleAdvertising(adv_callback)
            return False, None, None
        client_ad.log.info(
            "gattClientRequestLeConnectionParameters returns success. Interval={}"
            .format(minInterval))
        gatt_connected = True
        # For now, we will only test with 1 Mbit Phy.
        # TODO: Add explicit tests with 2 MBit Phy.
        client_ad.droid.gattClientSetPreferredPhy(
            bluetooth_gatt_1, gatt_phy['1m'], gatt_phy['1m'], 0)

    server_ad.droid.bluetoothSocketConnBeginAcceptThreadPsm(
        accept_timeout_ms, is_ble, secured_conn)

    psm_value = server_ad.droid.bluetoothSocketConnGetPsm()
    client_ad.log.info("Assigned PSM value={}".format(psm_value))

    client_ad.droid.bluetoothSocketConnBeginConnectThreadPsm(
        mac_address, is_ble, psm_value, secured_conn)

    if (le_tx_data_length != default_le_data_length) and is_ble:
        client_ad.log.info("orchestrate_coc_connection: call "
                           "bluetoothSocketRequestMaximumTxDataLength")
        client_ad.droid.bluetoothSocketRequestMaximumTxDataLength()

    end_time = time.time() + bt_default_timeout
    test_result = False
    while time.time() < end_time:
        if len(server_ad.droid.bluetoothSocketConnActiveConnections()) > 0:
            server_ad.log.info("CoC Server Connection Active")
            if len(client_ad.droid.bluetoothSocketConnActiveConnections()) > 0:
                client_ad.log.info("CoC Client Connection Active")
                test_result = True
                break
        time.sleep(1)

    if (adv_callback != None):
        server_ad.droid.bleStopBleAdvertising(adv_callback)

    if not test_result:
        client_ad.log.error("Failed to establish an CoC connection")
        return False, None, None

    if len(client_ad.droid.bluetoothSocketConnActiveConnections()) > 0:
        server_ad.log.info(
            "CoC client_ad Connection Active, num=%d",
            len(client_ad.droid.bluetoothSocketConnActiveConnections()))
    else:
        server_ad.log.info("Error CoC client_ad Connection Inactive")
        client_ad.log.info("Error CoC client_ad Connection Inactive")

    # Wait for the client to be ready
    client_conn_id = None
    while (client_conn_id == None):
        client_conn_id = client_ad.droid.bluetoothGetLastConnId()
        if (client_conn_id != None):
            break
        time.sleep(1)

    # Wait for the server to be ready
    server_conn_id = None
    while (server_conn_id == None):
        server_conn_id = server_ad.droid.bluetoothGetLastConnId()
        if (server_conn_id != None):
            break
        time.sleep(1)

    client_ad.log.info(
        "orchestrate_coc_connection: client conn id={}, server conn id={}".
        format(client_conn_id, server_conn_id))

    if gatt_connected:
        disconnect_gatt_connection(client_ad, bluetooth_gatt_1,
                                   gatt_callback_1)
        client_ad.droid.gattClientClose(bluetooth_gatt_1)

    return True, client_conn_id, server_conn_id
