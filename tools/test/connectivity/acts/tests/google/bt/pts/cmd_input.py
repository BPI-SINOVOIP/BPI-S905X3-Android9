#/usr/bin/env python3.4
#
# Copyright (C) 2016 The Android Open Source Project
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
# the License.
"""
Python script for wrappers to various libraries.
"""
from acts.test_utils.bt.bt_constants import bt_scan_mode_types
from acts.test_utils.bt.bt_constants import gatt_server_responses
import acts.test_utils.bt.gatt_test_database as gatt_test_database
from acts.test_utils.bt.bt_carkit_lib import E2eBtCarkitLib

import threading
import time
import cmd
"""Various Global Strings"""
CMD_LOG = "CMD {} result: {}"
FAILURE = "CMD {} threw exception: {}"


class CmdInput(cmd.Cmd):
    android_devices = []
    """Simple command processor for Bluetooth PTS Testing"""

    def connect_hsp_helper(self, ad):
        """A helper function for making HSP connections"""
        end_time = time.time() + 20
        connected_hsp_devices = len(ad.droid.bluetoothHspGetConnectedDevices())
        while connected_hsp_devices != 1 and time.time() < end_time:
            try:
                ad.droid.bluetoothHspConnect(self.mac_addr)
                time.sleep(3)
                if len(ad.droid.bluetoothHspGetConnectedDevices() == 1):
                    break
            except Exception:
                self.log.debug("Failed to connect hsp trying again...")
            try:
                ad.droid.bluetoothConnectBonded(self.mac_addr)
            except Exception:
                self.log.info("Failed to connect to bonded device...")
            connected_hsp_devices = len(
                ad.droid.bluetoothHspGetConnectedDevices())
        if connected_hsp_devices != 1:
            self.log.error("Failed to reconnect to HSP service...")
            return False
        self.log.info("Connected to HSP service...")
        return True

    def setup_vars(self, android_devices, mac_addr, log):
        self.pri_dut = android_devices[0]
        if len(android_devices) > 1:
            self.sec_dut = android_devices[1]
        if len(android_devices) > 2:
            self.ter_dut = android_devices[2]
        if len(android_devices) > 3:
            self.qua_dut = android_devices[3]
        self.mac_addr = mac_addr
        self.log = log
        self.pri_dut.bta.set_target_mac_addr(mac_addr)
        self.pri_dut.gattc.set_target_mac_addr(mac_addr)
        self.pri_dut.rfcomm.set_target_mac_addr(mac_addr)
        self.bt_carkit_lib = E2eBtCarkitLib(self.log, mac_addr)

    def emptyline(self):
        pass

    def do_EOF(self, line):
        "End Script"
        return True

    def do_reset_mac_address(self, line):
        """Reset target mac address for libraries based on the current connected device"""
        try:
            device = self.pri_dut.droid.bluetoothGetConnectedDevices()[0]
            #self.setup_vars(self.android_devices, device['address'], self.log)
            self.log.info("New device is {}".format(device))
        except Exception as err:
            self.log.info("Failed to setup new vars with {}".format(err))

    """Begin GATT Client wrappers"""

    def do_gattc_socket_conn_begin_connect_thread_psm(self, line):
        cmd = ""
        try:
            self.pri_dut.gattc.socket_conn_begin_connect_thread_psm(line)
        except Exception as err:
            self.log.info(FAILURE.format(cmd, err))

    def do_gattc_socket_conn_begin_accept_thread_psm(self, line):
        cmd = ""
        try:
            self.pri_dut.gattc.socket_conn_begin_accept_thread_psm(line)
        except Exception as err:
            self.log.info(FAILURE.format(cmd, err))

    def do_gattc_request_le_connection_parameters(self, line):
        cmd = ""
        try:
            self.pri_dut.gattc.request_le_connection_parameters()
        except Exception as err:
            self.log.info(FAILURE.format(cmd, err))

    def do_gattc_connect_over_le(self, line):
        """Perform GATT connection over LE"""
        cmd = "Gatt connect over LE"
        try:
            autoconnect = False
            if line:
                autoconnect = bool(line)
            self.pri_dut.gattc.connect_over_le(autoconnect)
        except Exception as err:
            self.log.info(FAILURE.format(cmd, err))

    def do_gattc_connect_over_bredr(self, line):
        """Perform GATT connection over BREDR"""
        cmd = "Gatt connect over BR/EDR"
        try:
            self.pri_dut.gattc.connect_over_bredr()
        except Exception as err:
            self.log.info(FAILURE.format(cmd, err))

    def do_gattc_disconnect(self, line):
        """Perform GATT disconnect"""
        cmd = "Gatt Disconnect"
        try:
            self.pri_dut.gattc.disconnect()
        except Exception as err:
            self.log.info(FAILURE.format(cmd, err))

    def do_gattc_read_char_by_uuid(self, line):
        """GATT client read Characteristic by UUID."""
        cmd = "GATT client read Characteristic by UUID."
        try:
            self.pri_dut.gattc.read_char_by_uuid(line)
        except Exception as err:
            self.log.info(FAILURE.format(cmd, err))

    def do_gattc_request_mtu(self, line):
        """Request MTU Change of input value"""
        cmd = "Request MTU Value"
        try:
            self.pri_dut.gattc.request_mtu(line)
        except Exception as err:
            self.log.info(FAILURE.format(cmd, err))

    def do_gattc_list_all_uuids(self, line):
        """From the GATT Client, discover services and list all services,
        chars and descriptors
        """
        cmd = "Discovery Services and list all UUIDS"
        try:
            self.pri_dut.gattc.list_all_uuids()
        except Exception as err:
            self.log.info(FAILURE.format(cmd, err))

    def do_gattc_discover_services(self, line):
        """GATT Client discover services of GATT Server"""
        cmd = "Discovery Services of GATT Server"
        try:
            self.pri_dut.gattc.discover_services()
        except Exception as err:
            self.log.info(FAILURE.format(cmd, err))

    def do_gattc_refresh(self, line):
        """Perform Gatt Client Refresh"""
        cmd = "GATT Client Refresh"
        try:
            self.pri_dut.gattc.refresh()
        except Exception as err:
            self.log.info(FAILURE.format(cmd, err))

    def do_gattc_read_char_by_instance_id(self, line):
        """From the GATT Client, discover services and list all services,
        chars and descriptors
        """
        cmd = "GATT Client Read By Instance ID"
        try:
            self.pri_dut.gattc.read_char_by_instance_id(line)
        except Exception as err:
            self.log.info(FAILURE.format(cmd, err))

    def do_gattc_write_char_by_instance_id(self, line):
        """GATT Client Write to Characteristic by instance ID"""
        cmd = "GATT Client write to Characteristic by instance ID"
        try:
            self.pri_dut.gattc.write_char_by_instance_id(line)
        except Exception as err:
            self.log.info(FAILURE.format(cmd, err))

    def do_gattc_write_char_by_instance_id_value(self, line):
        """GATT Client Write to Characteristic by instance ID"""
        cmd = "GATT Client write to Characteristic by instance ID"
        try:
            self.pri_dut.gattc.write_char_by_instance_id_value(line)
        except Exception as err:
            self.log.info(FAILURE.format(cmd, err))

    def do_gattc_mod_write_char_by_instance_id(self, line):
        """GATT Client Write to Char that doesn't have write permission"""
        cmd = "GATT Client Write to Char that doesn't have write permission"
        try:
            self.pri_dut.gattc.mod_write_char_by_instance_id(line)
        except Exception as err:
            self.log.info(FAILURE.format(cmd, err))

    def do_gattc_write_invalid_char_by_instance_id(self, line):
        """GATT Client Write to Char that doesn't exists"""
        try:
            self.pri_dut.gattc.write_invalid_char_by_instance_id(line)
        except Exception as err:
            self.log.info(FAILURE.format(cmd, err))

    def do_gattc_mod_read_char_by_instance_id(self, line):
        """GATT Client Read Char that doesn't have write permission"""
        cmd = "GATT Client Read Char that doesn't have write permission"
        try:
            self.pri_dut.gattc.mod_read_char_by_instance_id(line)
        except Exception as err:
            self.log.info(FAILURE.format(cmd, err))

    def do_gattc_read_invalid_char_by_instance_id(self, line):
        """GATT Client Read Char that doesn't exists"""
        cmd = "GATT Client Read Char that doesn't exists"
        try:
            self.pri_dut.gattc.read_invalid_char_by_instance_id(line)
        except Exception as err:
            self.log.info(FAILURE.format(cmd, err))

    def do_gattc_mod_write_desc_by_instance_id(self, line):
        """GATT Client Write to Desc that doesn't have write permission"""
        cmd = "GATT Client Write to Desc that doesn't have write permission"
        try:
            self.pri_dut.gattc.mod_write_desc_by_instance_id(line)
        except Exception as err:
            self.log.info(FAILURE.format(cmd, err))

    def do_gattc_write_invalid_desc_by_instance_id(self, line):
        """GATT Client Write to Desc that doesn't exists"""
        cmd = "GATT Client Write to Desc that doesn't exists"
        try:
            self.pri_dut.gattc.write_invalid_desc_by_instance_id(line)
        except Exception as err:
            self.log.info(FAILURE.format(cmd, err))

    def do_gattc_mod_read_desc_by_instance_id(self, line):
        """GATT Client Read Desc that doesn't have write permission"""
        cmd = "GATT Client Read Desc that doesn't have write permission"
        try:
            self.pri_dut.gattc.mod_read_desc_by_instance_id(line)
        except Exception as err:
            self.log.info(FAILURE.format(cmd, err))

    def do_gattc_read_invalid_desc_by_instance_id(self, line):
        """GATT Client Read Desc that doesn't exists"""
        cmd = "GATT Client Read Desc that doesn't exists"
        try:
            self.pri_dut.gattc.read_invalid_desc_by_instance_id(line)
        except Exception as err:
            self.log.info(FAILURE.format(cmd, err))

    def do_gattc_mod_read_char_by_uuid_and_instance_id(self, line):
        """GATT Client Read Char that doesn't have write permission"""
        cmd = "GATT Client Read Char that doesn't have write permission"
        try:
            self.pri_dut.gattc.mod_read_char_by_uuid_and_instance_id(line)
        except Exception as err:
            self.log.info(FAILURE.format(cmd, err))

    def do_gattc_read_invalid_char_by_uuid(self, line):
        """GATT Client Read Char that doesn't exist"""
        cmd = "GATT Client Read Char that doesn't exist"
        try:
            self.pri_dut.gattc.read_invalid_char_by_uuid(line)
        except Exception as err:
            self.log.info(FAILURE.format(cmd, err))

    def do_gattc_write_desc_by_instance_id(self, line):
        """GATT Client Write to Descriptor by instance ID"""
        cmd = "GATT Client Write to Descriptor by instance ID"
        try:
            self.pri_dut.gattc.write_desc_by_instance_id(line)
        except Exception as err:
            self.log.info(FAILURE.format(cmd, err))

    def do_gattc_enable_notification_desc_by_instance_id(self, line):
        """GATT Client Enable Notification on Descriptor by instance ID"""
        cmd = "GATT Client Enable Notification on Descriptor by instance ID"
        try:
            self.pri_dut.gattc.enable_notification_desc_by_instance_id(line)
        except Exception as err:
            self.log.info(FAILURE.format(cmd, err))

    def do_gattc_enable_indication_desc_by_instance_id(self, line):
        """GATT Client Enable Indication on Descriptor by instance ID"""
        cmd = "GATT Client Enable Indication on Descriptor by instance ID"
        try:
            self.pri_dut.gattc.enable_indication_desc_by_instance_id(line)
        except Exception as err:
            self.log.info(FAILURE.format(cmd, err))

    def do_gattc_char_enable_all_notifications(self, line):
        """GATT Client enable all notifications"""
        cmd = "GATT Client enable all notifications"
        try:
            self.pri_dut.gattc.char_enable_all_notifications()
        except Exception as err:
            self.log.info(FAILURE.format(cmd, err))

    def do_gattc_read_char_by_invalid_instance_id(self, line):
        """GATT Client read char by non-existant instance id"""
        cmd = "GATT Client read char by non-existant instance id"
        try:
            self.pri_dut.gattc.read_char_by_invalid_instance_id(line)
        except Exception as err:
            self.log.info(FAILURE.format(cmd, err))

    def do_gattc_begin_reliable_write(self, line):
        """Begin a reliable write on the Bluetooth Gatt Client"""
        cmd = "GATT Client Begin Reliable Write"
        try:
            self.pri_dut.gattc.begin_reliable_write()
        except Exception as err:
            self.log.info(FAILURE.format(cmd, err))

    def do_gattc_abort_reliable_write(self, line):
        """Abort a reliable write on the Bluetooth Gatt Client"""
        cmd = "GATT Client Abort Reliable Write"
        try:
            self.pri_dut.gattc.abort_reliable_write()
        except Exception as err:
            self.log.info(FAILURE.format(cmd, err))

    def do_gattc_execute_reliable_write(self, line):
        """Execute a reliable write on the Bluetooth Gatt Client"""
        cmd = "GATT Client Execute Reliable Write"
        try:
            self.pri_dut.gattc.execute_reliable_write()
        except Exception as err:
            self.log.info(FAILURE.format(cmd, err))

    def do_gattc_read_all_char(self, line):
        """GATT Client read all Characteristic values"""
        cmd = "GATT Client read all Characteristic values"
        try:
            self.pri_dut.gattc.read_all_char()
        except Exception as err:
            self.log.info(FAILURE.format(cmd, err))

    def do_gattc_write_all_char(self, line):
        """Write to every Characteristic on the GATT server"""
        cmd = "GATT Client Write All Characteristics"
        try:
            self.pri_dut.gattc.write_all_char(line)
        except Exception as err:
            self.log.info(FAILURE.format(cmd, err))

    def do_gattc_write_all_desc(self, line):
        """ Write to every Descriptor on the GATT server """
        cmd = "GATT Client Write All Descriptors"
        try:
            self.pri_dut.gattc.write_all_desc(line)
        except Exception as err:
            self.log.info(FAILURE.format(cmd, err))

    def do_gattc_write_desc_notification_by_instance_id(self, line):
        """Write 0x00 or 0x02 to notification descriptor [instance_id]"""
        cmd = "Write 0x00 0x02 to notification descriptor"
        try:
            self.pri_dut.gattc.write_desc_notification_by_instance_id(line)
        except Exception as err:
            self.log.info(FAILURE.format(cmd, err))

    def do_gattc_discover_service_by_uuid(self, line):
        """Discover service by uuid"""
        cmd = "Discover service by uuid"
        try:
            self.pri_dut.gattc.discover_service_by_uuid(line)
        except Exception as err:
            self.log.info(FAILURE.format(cmd, err))

    def do_gattc_read_all_desc(self, line):
        """Read all Descriptor values"""
        cmd = "Read all Descriptor values"
        try:
            self.pri_dut.gattc.read_all_desc()
        except Exception as err:
            self.log.info(FAILURE.format(cmd, err))

    """End GATT Client wrappers"""
    """Begin GATT Server wrappers"""

    def do_gatts_close_bluetooth_gatt_servers(self, line):
        """Close Bluetooth Gatt Servers"""
        cmd = "Close Bluetooth Gatt Servers"
        try:
            self.pri_dut.gatts.close_bluetooth_gatt_servers()
        except Exception as err:
            self.log.info(FAILURE.format(cmd, err))

    def complete_gatts_setup_database(self, text, line, begidx, endidx):
        if not text:
            completions = list(
                gatt_test_database.GATT_SERVER_DB_MAPPING.keys())[:]
        else:
            completions = [
                s for s in gatt_test_database.GATT_SERVER_DB_MAPPING.keys()
                if s.startswith(text)
            ]
        return completions

    def complete_gatts_send_response(self, text, line, begidx, endidx):
        """GATT Server database name completion"""
        if not text:
            completions = list(gatt_server_responses.keys())[:]
        else:
            completions = [
                s for s in gatt_server_responses.keys() if s.startswith(text)
            ]
        return completions

    def complete_gatts_send_continuous_response(self, text, line, begidx,
                                                endidx):
        """GATT Server database name completion"""
        if not text:
            completions = list(gatt_server_responses.keys())[:]
        else:
            completions = [
                s for s in gatt_server_responses.keys() if s.startswith(text)
            ]
        return completions

    def complete_gatts_send_continuous_response_data(self, text, line, begidx,
                                                     endidx):
        """GATT Server database name completion"""
        if not text:
            completions = list(gatt_server_responses.keys())[:]
        else:
            completions = [
                s for s in gatt_server_responses.keys() if s.startswith(text)
            ]
        return completions

    def do_gatts_list_all_uuids(self, line):
        """From the GATT Client, discover services and list all services,
        chars and descriptors
        """
        cmd = "Discovery Services and list all UUIDS"
        try:
            self.pri_dut.gatts.list_all_uuids()
        except Exception as err:
            self.log.info(FAILURE.format(cmd, err))

    def do_gatts_send_response(self, line):
        """Send a response to the GATT Client"""
        cmd = "GATT server send response"
        try:
            self.pri_dut.gatts.send_response(line)
        except Exception as err:
            self.log.info(FAILURE.format(cmd, err))

    def do_gatts_notify_characteristic_changed(self, line):
        """Notify char changed by instance id [instance_id] [true|false]"""
        cmd = "Notify characteristic changed by instance id"
        try:
            info = line.split()
            instance_id = info[0]
            confirm_str = info[1]
            confirm = False
            if confirm_str.lower() == 'true':
                confirm = True
            self.pri_dut.gatts.notify_characteristic_changed(
                instance_id, confirm)
        except Exception as err:
            self.log.info(FAILURE.format(cmd, err))

    def do_gatts_setup_database(self, line):
        cmd = "Setup GATT Server database: {}".format(line)
        try:
            self.pri_dut.gatts.setup_gatts_db(
                gatt_test_database.GATT_SERVER_DB_MAPPING.get(line))
        except Exception as err:
            self.log.info(FAILURE.format(cmd, err))

    def do_gatts_characteristic_set_value_by_instance_id(self, line):
        """Set Characteristic value by instance id"""
        cmd = "Change value of a characteristic by instance id"
        try:
            info = line.split()
            instance_id = info[0]
            size = int(info[1])
            value = []
            for i in range(size):
                value.append(i % 256)
            self.pri_dut.gatts.characteristic_set_value_by_instance_id(
                instance_id, value)
        except Exception as err:
            self.log.info(FAILURE.format(cmd, err))

    def do_notify_characteristic_changed(self, line):
        """Notify characteristic changed [instance_id] [confirm]"""
        cmd = "Notify characteristic changed"
        try:
            info = line.split()
            instance_id = info[0]
            confirm = bool(info[1])
            self.pri_dut.gatts.notify_characteristic_changed(
                instance_id, confirm)
        except Exception as err:
            self.log.info(FAILURE.format(cmd, err))

    def do_gatts_open(self, line):
        """Open a GATT Server instance"""
        cmd = "Open an empty GATT Server"
        try:
            self.pri_dut.gatts.open()
        except Exception as err:
            self.log.info(FAILURE.format(cmd, err))

    def do_gatts_clear_services(self, line):
        """Clear BluetoothGattServices from BluetoothGattServer"""
        cmd = "Clear BluetoothGattServices from BluetoothGattServer"
        try:
            self.pri_dut.gatts.gatt_server_clear_services()
        except Exception as err:
            self.log.info(FAILURE.format(cmd, err))

    def do_gatts_send_continuous_response(self, line):
        """Send continous response with random data"""
        cmd = "Send continous response with random data"
        try:
            self.pri_dut.gatts.send_continuous_response(line)
        except Exception as err:
            self.log.info(FAILURE.format(cmd, err))

    def do_gatts_send_continuous_response_data(self, line):
        """Send continous response including requested data"""
        cmd = "Send continous response including requested data"
        try:
            self.pri_dut.gatts.send_continuous_response_data(line)
        except Exception as err:
            self.log.info(FAILURE.format(cmd, err))

    """End GATT Server wrappers"""
    """Begin Ble wrappers"""

    def complete_ble_adv_data_include_local_name(self, text, line, begidx,
                                                 endidx):
        options = ['true', 'false']
        if not text:
            completions = list(options)[:]
        else:
            completions = [s for s in options if s.startswith(text)]
        return completions

    def complete_ble_adv_data_include_tx_power_level(self, text, line, begidx,
                                                     endidx):
        options = ['true', 'false']
        if not text:
            completions = list(options)[:]
        else:
            completions = [s for s in options if s.startswith(text)]
        return completions

    def complete_ble_stop_advertisement(self, text, line, begidx, endidx):
        str_adv_list = list(map(str, self.pri_dut.ble.ADVERTISEMENT_LIST))
        if not text:
            completions = str_adv_list[:]
        else:
            completions = [s for s in str_adv_list if s.startswith(text)]
        return completions

    def do_ble_start_generic_connectable_advertisement(self, line):
        """Start a connectable LE advertisement"""
        cmd = "Start a connectable LE advertisement"
        try:
            self.pri_dut.ble.start_generic_connectable_advertisement(line)
        except Exception as err:
            self.log.info(FAILURE.format(cmd, err))

    def _start_max_advertisements(self, ad):
        self.pri_dut.ble.start_max_advertisements(ad)

    def do_ble_start_generic_connectable_beacon_swarm(self, line):
        """Start a connectable LE advertisement"""
        cmd = "Start as many advertisements as possible on all devices"
        try:
            threads = []
            for ad in self.android_devices:
                thread = threading.Thread(
                    target=self._start_max_advertisements, args=([ad]))
                threads.append(thread)
                thread.start()
            for t in threads:
                t.join()
        except Exception as err:
            self.log.info(FAILURE.format(cmd, err))

    def do_ble_start_connectable_advertisement_set(self, line):
        """Start a connectable advertisement set"""
        try:
            self.pri_dut.ble.start_connectable_advertisement_set(line)
        except Exception as err:
            self.log.error("Failed to start advertisement: {}".format(err))

    def do_ble_stop_all_advertisement_set(self, line):
        """Stop all advertisement sets"""
        try:
            self.pri_dut.ble.stop_all_advertisement_set(line)
        except Exception as err:
            self.log.error("Failed to stop advertisement: {}".format(err))

    def do_ble_adv_add_service_uuid_list(self, line):
        """Add service UUID to the LE advertisement inputs:
         [uuid1 uuid2 ... uuidN]"""
        cmd = "Add a valid service UUID to the advertisement."
        try:
            self.pri_dut.ble.adv_add_service_uuid_list(line)
        except Exception as err:
            self.log.info(FAILURE.format(cmd, err))

    def do_ble_adv_data_include_local_name(self, line):
        """Include local name in the advertisement. inputs: [true|false]"""
        cmd = "Include local name in the advertisement."
        try:
            self.pri_dut.ble.adv_data_include_local_name(line)
        except Exception as err:
            self.log.info(FAILURE.format(cmd, err))

    def do_ble_adv_data_include_tx_power_level(self, line):
        """Include tx power level in the advertisement. inputs: [true|false]"""
        cmd = "Include local name in the advertisement."
        try:
            self.pri_dut.ble.adv_data_include_tx_power_level(line)
        except Exception as err:
            self.log.info(FAILURE.format(cmd, err))

    def do_ble_adv_data_add_manufacturer_data(self, line):
        """Include manufacturer id and data to the advertisment:
        [id data1 data2 ... dataN]"""
        cmd = "Include manufacturer id and data to the advertisment."
        try:
            self.pri_dut.ble.adv_data_add_manufacturer_data(line)
        except Exception as err:
            self.log.info(FAILURE.format(cmd, err))

    def do_ble_start_generic_nonconnectable_advertisement(self, line):
        """Start a nonconnectable LE advertisement"""
        cmd = "Start a nonconnectable LE advertisement"
        try:
            self.pri_dut.ble.start_generic_nonconnectable_advertisement(line)
        except Exception as err:
            self.log.info(FAILURE.format(cmd, err))

    def do_ble_list_active_advertisement_ids(self, line):
        """List all active BLE advertisements"""
        self.log.info("IDs: {}".format(self.pri_dut.ble.advertisement_list))

    def do_ble_stop_all_advertisements(self, line):
        """Stop all LE advertisements"""
        cmd = "Stop all LE advertisements"
        try:
            self.pri_dut.ble.stop_all_advertisements(line)
        except Exception as err:
            self.log.info(FAILURE.format(cmd, err))

    def do_ble_stop_advertisement(self, line):
        """Stop an LE advertisement"""
        cmd = "Stop a connectable LE advertisement"
        try:
            self.pri_dut.ble.ble_stop_advertisement(line)
        except Exception as err:
            self.log.info(FAILURE.format(cmd, err))

    """End Ble wrappers"""
    """Begin Bta wrappers"""

    def complete_bta_start_pairing_helper(self, text, line, begidx, endidx):
        options = ['true', 'false']
        if not text:
            completions = list(options)[:]
        else:
            completions = [s for s in options if s.startswith(text)]
        return completions

    def complete_bta_set_scan_mode(self, text, line, begidx, endidx):
        completions = list(bt_scan_mode_types.keys())[:]
        if not text:
            completions = completions[:]
        else:
            completions = [s for s in completions if s.startswith(text)]
        return completions

    def do_bta_set_scan_mode(self, line):
        """Set the Scan mode of the Bluetooth Adapter"""
        cmd = "Set the Scan mode of the Bluetooth Adapter"
        try:
            self.pri_dut.bta.set_scan_mode(line)
        except Exception as err:
            self.log.info(FAILURE.format(cmd, err))

    def do_bta_set_device_name(self, line):
        """Set Bluetooth Adapter Name"""
        cmd = "Set Bluetooth Adapter Name"
        try:
            self.pri_dut.bta.set_device_name(line)
        except Exception as err:
            self.log.info(FAILURE.format(cmd, err))

    def do_bta_enable(self, line):
        """Enable Bluetooth Adapter"""
        cmd = "Enable Bluetooth Adapter"
        try:
            self.pri_dut.bta.enable()
        except Exception as err:
            self.log.info(FAILURE.format(cmd, err))

    def do_bta_disable(self, line):
        """Disable Bluetooth Adapter"""
        cmd = "Disable Bluetooth Adapter"
        try:
            self.pri_dut.bta.disable()
        except Exception as err:
            self.log.info(FAILURE.format(cmd, err))

    def do_bta_init_bond(self, line):
        """Initiate bond to PTS device"""
        cmd = "Initiate Bond"
        try:
            self.pri_dut.bta.init_bond()
        except Exception as err:
            self.log.info(FAILURE.format(cmd, err))

    def do_bta_start_discovery(self, line):
        """Start BR/EDR Discovery"""
        cmd = "Start BR/EDR Discovery"
        try:
            self.pri_dut.bta.start_discovery()
        except Exception as err:
            self.log.info(FAILURE.format(cmd, err))

    def do_bta_stop_discovery(self, line):
        """Stop BR/EDR Discovery"""
        cmd = "Stop BR/EDR Discovery"
        try:
            self.pri_dut.bta.stop_discovery()
        except Exception as err:
            self.log.info(FAILURE.format(cmd, err))

    def do_bta_get_discovered_devices(self, line):
        """Get Discovered Br/EDR Devices"""
        cmd = "Get Discovered Br/EDR Devices\n"
        try:
            self.pri_dut.bta.get_discovered_devices()
        except Exception as err:
            self.log.info(FAILURE.format(cmd, err))

    def do_bta_bond(self, line):
        """Bond to PTS device"""
        cmd = "Bond to the PTS dongle directly"
        try:
            self.pri_dut.bta.bond()
        except Exception as err:
            self.log.info(FAILURE.format(cmd, err))

    def do_bta_disconnect(self, line):
        """BTA disconnect"""
        cmd = "BTA disconnect"
        try:
            self.pri_dut.bta.disconnect()
        except Exception as err:
            self.log.info(FAILURE.format(cmd, err))

    def do_bta_unbond(self, line):
        """Unbond from PTS device"""
        cmd = "Unbond from the PTS dongle"
        try:
            self.pri_dut.bta.unbond()
        except Exception as err:
            self.log.info(FAILURE.format(cmd, err))

    def do_bta_start_pairing_helper(self, line):
        """Start or stop Bluetooth Pairing Helper"""
        cmd = "Start or stop BT Pairing helper"
        try:
            self.pri_dut.bta.start_pairing_helper(line)
        except Exception as err:
            self.log.info(FAILURE.format(cmd, err))

    def do_bta_push_pairing_pin(self, line):
        """Push pairing pin to the Android Device"""
        cmd = "Push the pin to the Android Device"
        try:
            self.pri_dut.bta.push_pairing_pin(line)
        except Exception as err:
            self.log.info(FAILURE.format(cmd, err))

    def do_bta_get_pairing_pin(self, line):
        """Get pairing PIN"""
        cmd = "Get Pin Info"
        try:
            self.pri_dut.bta.get_pairing_pin()
        except Exception as err:
            self.log.info(FAILURE.format(cmd, err))

    def do_bta_fetch_uuids_with_sdp(self, line):
        """BTA fetch UUIDS with SDP"""
        cmd = "Fetch UUIDS with SDP"
        try:
            self.pri_dut.bta.fetch_uuids_with_sdp()
        except Exception as err:
            self.log.info(FAILURE.format(cmd, err))

    def do_bta_connect_profiles(self, line):
        """Connect available profiles"""
        cmd = "Connect all profiles possible"
        try:
            self.pri_dut.bta.connect_profiles()
        except Exception as err:
            self.log.info(FAILURE.format(cmd, err))

    def do_bta_tts_speak(self, line):
        cmd = "Open audio channel by speaking characters"
        try:
            self.pri_dut.bta.tts_speak()
        except Exception as err:
            self.log.info(FAILURE.format(cmd, err))

    """End Bta wrappers"""
    """Begin Rfcomm wrappers"""

    def do_rfcomm_connect(self, line):
        """Perform an RFCOMM connect"""
        try:
            self.pri_dut.rfcomm.connect(line)
        except Exception as err:
            self.log.info(FAILURE.format(cmd, err))

    def do_rfcomm_open_rfcomm_socket(self, line):
        """Open rfcomm socket"""
        cmd = "Open RFCOMM socket"
        try:
            self.pri_dut.rfcomm.open_rfcomm_socket()
        except Exception as err:
            self.log.info(FAILURE.format(cmd, err))

    def do_rfcomm_open_l2cap_socket(self, line):
        """Open L2CAP socket"""
        cmd = "Open L2CAP socket"
        try:
            self.pri_dut.rfcomm.open_l2cap_socket()
        except Exception as err:
            self.log.info(FAILURE.format(cmd, err))

    def do_rfcomm_write(self, line):
        cmd = "Write String data over an RFCOMM connection"
        try:
            self.pri_dut.rfcomm.write(line)
        except Exception as err:
            self.log.info(FAILURE.format(cmd, err))

    def do_rfcomm_write_binary(self, line):
        cmd = "Write String data over an RFCOMM connection"
        try:
            self.pri_dut.rfcomm.write_binary(line)
        except Exception as err:
            self.log.info(FAILURE.format(cmd, err))

    def do_rfcomm_end_connect(self, line):
        cmd = "End RFCOMM connection"
        try:
            self.pri_dut.rfcomm.end_connect()
        except Exception as err:
            self.log.info(FAILURE.format(cmd, err))

    def do_rfcomm_accept(self, line):
        cmd = "Accept RFCOMM connection"
        try:
            self.pri_dut.rfcomm.accept(line)
        except Exception as err:
            self.log.info(FAILURE.format(cmd, err))

    def do_rfcomm_stop(self, line):
        cmd = "STOP RFCOMM Connection"
        try:
            self.pri_dut.rfcomm.stop()
        except Exception as err:
            self.log.info(FAILURE.format(cmd, err))

    def do_rfcomm_open_l2cap_socket(self, line):
        """Open L2CAP socket"""
        cmd = "Open L2CAP socket"
        try:
            self.pri_dut.rfcomm.open_l2cap_socket()
        except Exception as err:
            self.log.info(FAILURE.format(cmd, err))

    """End Rfcomm wrappers"""
    """Begin Config wrappers"""

    def do_config_reset(self, line):
        """Reset Bluetooth Config file"""
        cmd = "Reset Bluetooth Config file"
        try:
            self.pri_dut.config.reset()
        except Exception as err:
            self.log.info(FAILURE.format(cmd, err))

    def do_config_set_nonbond(self, line):
        """Set NonBondable Mode"""
        cmd = "Set NonBondable Mode"
        try:
            self.pri_dut.config.set_nonbond()
        except Exception as err:
            self.log.info(FAILURE.format(cmd, err))

    def do_config_set_disable_mitm(self, line):
        """Set Disable MITM"""
        cmd = "Set Disable MITM"
        try:
            self.pri_dut.config.set_disable_mitm()
        except Exception as err:
            self.log.info(FAILURE.format(cmd, err))

    """End Config wrappers"""
    """Begin HFP/HSP wrapper"""

    def do_bta_hfp_start_voice_recognition(self, line):
        self.pri_dut.droid.bluetoothHspStartVoiceRecognition(self.mac_addr)

    def do_bta_hfp_stop_voice_recognition(self, line):
        self.pri_dut.droid.bluetoothHspStopVoiceRecognition(self.mac_addr)

    def do_test_clcc_response(self, line):
        # Experimental
        clcc_index = 1
        clcc_direction = 1
        clcc_status = 4
        clcc_mode = 0
        clcc_mpty = False
        clcc_number = "18888888888"
        clcc_type = 0
        self.pri_dut.droid.bluetoothHspClccResponse(
            clcc_index, clcc_direction, clcc_status, clcc_mode, clcc_mpty,
            clcc_number, clcc_type)

    def do_bta_hsp_force_sco_audio_on(self, line):
        """HFP/HSP Force SCO Audio ON"""
        cmd = "HFP/HSP Force SCO Audio ON"
        try:
            if not self.pri_dut.droid.bluetoothHspForceScoAudio(True):
                self.log.info(
                    FAILURE.format(cmd,
                                   "bluetoothHspForceScoAudio returned false"))
        except Exception as err:
            self.log.info(FAILURE.format(cmd, err))

    def do_bta_hsp_force_sco_audio_off(self, line):
        """HFP/HSP Force SCO Audio OFF"""
        cmd = "HFP/HSP Force SCO Audio OFF"
        try:
            if not self.pri_dut.droid.bluetoothHspForceScoAudio(False):
                self.log.info(
                    FAILURE.format(cmd,
                                   "bluetoothHspForceScoAudio returned false"))
        except Exception as err:
            self.log.info(FAILURE.format(cmd, err))

    def do_bta_hsp_connect_audio(self, line):
        """HFP/HSP connect audio"""
        cmd = "HFP/HSP connect audio"
        try:
            if not self.pri_dut.droid.bluetoothHspConnectAudio(self.mac_addr):
                self.log.info(
                    FAILURE.format(
                        cmd, "bluetoothHspConnectAudio returned false for " +
                        self.mac_addr))
        except Exception as err:
            self.log.info(FAILURE.format(cmd, err))

    def do_bta_hsp_disconnect_audio(self, line):
        """HFP/HSP disconnect audio"""
        cmd = "HFP/HSP disconnect audio"
        try:
            if not self.pri_dut.droid.bluetoothHspDisconnectAudio(
                    self.mac_addr):
                self.log.info(
                    FAILURE.format(
                        cmd,
                        "bluetoothHspDisconnectAudio returned false for " +
                        self.mac_addr))
        except Exception as err:
            self.log.info(FAILURE.format(cmd, err))

    def do_bta_hsp_connect_slc(self, line):
        """HFP/HSP connect SLC with additional tries and help"""
        cmd = "Connect to hsp with some help"
        try:
            if not self.connect_hsp_helper(self.pri_dut):
                self.log.error("Failed to connect to HSP")
        except Exception as err:
            self.log.info(FAILURE.format(cmd, err))

    def do_bta_hsp_disconnect_slc(self, line):
        """HFP/HSP disconnect SLC"""
        cmd = "HFP/HSP disconnect SLC"
        try:
            if not self.pri_dut.droid.bluetoothHspDisconnect(self.mac_addr):
                self.log.info(
                    FAILURE.format(
                        cmd, "bluetoothHspDisconnect returned false for " +
                        self.mac_addr))
        except Exception as err:
            self.log.info(FAILURE.format(cmd, err))

    """End HFP/HSP wrapper"""
    """Begin HID wrappers"""

    def do_hid_get_report(self, line):
        """Get HID Report"""
        cmd = "Get HID Report"
        try:
            self.pri_dut.droid.bluetoothHidGetReport(self.mac_addr, 1, 1, 1024)
        except Exception as err:
            self.log.info(FAILURE.format(cmd, err))

    def do_hid_set_report(self, line):
        """Get HID Report"""
        cmd = "Get HID Report"
        try:
            self.pri_dut.droid.bluetoothHidSetReport(self.mac_addr, 1, "Test")
        except Exception as err:
            self.log.info(FAILURE.format(cmd, err))

    def do_hid_virtual_unplug(self, line):
        """Get HID Report"""
        cmd = "Get HID Report"
        try:
            self.pri_dut.droid.bluetoothHidVirtualUnplug(self.mac_addr)
        except Exception as err:
            self.log.info(FAILURE.format(cmd, err))

    def do_hid_send_report(self, line):
        """Get HID Report"""
        cmd = "Get HID Report"
        try:
            self.pri_dut.droid.bluetoothHidSendData(device_id, "42")
        except Exception as err:
            self.log.info(FAILURE.format(cmd, err))

    """End HID wrappers"""
    """Begin carkit test wrappers"""

    def do_test_suite_generic_bt_tests(self, line):
        """Run generic Bluetooth connection test suite"""
        generic_bt_tests = [
            tuple((self.bt_carkit_lib.disconnect_reconnect_multiple_iterations,
                   [self.pri_dut])),
            tuple((self.bt_carkit_lib.disconnect_a2dp_only_then_reconnect,
                   [self.pri_dut])),
            tuple((self.bt_carkit_lib.disconnect_hsp_only_then_reconnect,
                   [self.pri_dut])),
            tuple((
                self.bt_carkit_lib.disconnect_both_hsp_and_a2dp_then_reconnect,
                [self.pri_dut])),
        ]
        try:
            for func, param in generic_bt_tests:
                try:
                    func(param)
                except Exception:
                    self.log.info("Test {} failed.".format(func))
        except Exception as err:
            self.log.info(FAILURE.format(cmd, err))

    def do_e2e_connect_hsp_helper(self, line):
        """Connect to HSP/HFP with additional tries and help"""
        cmd = "Connect to hsp with some help"
        try:
            self.bt_carkit_lib.connect_hsp_helper(self.pri_dut)
        except Exception as err:
            self.log.info(FAILURE.format(cmd, err))

    def do_e2e_hfp_setup_multi_incomming_calls(self, line):
        """Setup two incomming calls"""
        cmd = "Setup multiple incomming calls"
        try:
            self.bt_carkit_lib.setup_multi_call(self.sec_dut, self.ter_dut,
                                                self.pri_dut)
        except Exception as err:
            self.log.info(FAILURE.format(cmd, err))

    def do_e2e_disconnect_reconnect_multiple_iterations(self, line):
        """Quick disconnect/reconnect stress test"""
        try:
            self.bt_carkit_lib.disconnect_reconnect_multiple_iterations(
                self.pri_dut)
        except Exception as err:
            self.log.info(FAILURE.format(cmd, err))

    def do_e2e_disconnect_a2dp_only_then_reconnect(self, line):
        """Test disconnect-reconnect a2dp only scenario from phone."""
        cmd = "Test disconnect-reconnect a2dp only scenario from phone."
        try:
            self.bt_carkit_lib.disconnect_a2dp_only_then_reconnect(
                self.pri_dut)
        except Exception as err:
            self.log.info(FAILURE.format(cmd, err))

    def do_e2e_disconnect_hsp_only_then_reconnect(self, line):
        """Test disconnect-reconnect hsp only scenario from phone."""
        cmd = "Test disconnect-reconnect hsp only scenario from phone."
        try:
            self.bt_carkit_lib.disconnect_hsp_only_then_reconnect(self.pri_dut)
        except Exception as err:
            self.log.info(FAILURE.format(cmd, err))

    def do_e2e_disconnect_both_hsp_and_a2dp_then_reconnect(self, line):
        """Test disconnect-reconnect hsp and a2dp scenario from phone."""
        cmd = "Test disconnect-reconnect hsp and a2dp scenario from phone."
        try:
            self.bt_carkit_lib.disconnect_both_hsp_and_a2dp_then_reconnect(
                self.pri_dut)
        except Exception as err:
            self.log.info(FAILURE.format(cmd, err))

    def do_e2e_outgoing_call_private_number(self, line):
        """Test outgoing call scenario from phone to private number"""
        cmd = "Test outgoing call scenario from phone to private number"
        try:
            self.bt_carkit_lib.outgoing_call_private_number(
                self.pri_dut, self.ter_dut)
        except Exception as err:
            self.log.info(FAILURE.format(cmd, err))

    def do_e2e_outgoing_call_a2dp_play_before_and_after(self, line):
        """Test outgoing call scenario while playing music. Music should resume
        after call."""
        cmd = "Test outgoing call scenario while playing music. Music should " \
              "resume after call."
        try:
            self.bt_carkit_lib.outgoing_call_a2dp_play_before_and_after(
                self.pri_dut, self.sec_dut)
        except Exception as err:
            self.log.info(FAILURE.format(cmd, err))

    def do_e2e_outgoing_call_unknown_contact(self, line):
        """Test outgoing call scenario from phone to unknow contact"""
        cmd = "Test outgoing call scenario from phone to unknow contact"
        try:
            self.bt_carkit_lib.outgoing_call_unknown_contact(
                self.pri_dut, self.ter_dut)
        except Exception as err:
            self.log.info(FAILURE.format(cmd, err))

    def do_e2e_incomming_call_private_number(self, line):
        """Test incomming call scenario to phone from unknown contact"""
        cmd = "Test incomming call scenario to phone from unknown contact"
        try:
            self.bt_carkit_lib.incomming_call_private_number(
                self.pri_dut, self.ter_dut)
        except Exception as err:
            self.log.info(FAILURE.format(cmd, err))

    def do_e2e_outgoing_call_multiple_iterations(self, line):
        """Test outgoing call quickly 3 times"""
        cmd = "Test outgoing call quickly 3 times"
        try:
            self.bt_carkit_lib.outgoing_call_multiple_iterations(
                self.pri_dut, self.sec_dut)
        except Exception as err:
            self.log.info(FAILURE.format(cmd, err))

    def do_e2e_outgoing_call_hsp_disabled_then_enabled_during_call(self, line):
        """Test outgoing call hsp disabled then enable during call."""
        cmd = "Test outgoing call hsp disabled then enable during call."
        try:
            self.bt_carkit_lib.outgoing_call_hsp_disabled_then_enabled_during_call(
                self.pri_dut, self.sec_dut)
        except Exception as err:
            self.log.info(FAILURE.format(cmd, err))

    def do_e2e_call_audio_routes(self, line):
        """Test various audio routes scenario from phone."""
        cmd = "Test various audio routes scenario from phone."
        try:
            self.bt_carkit_lib.call_audio_routes(self.pri_dut, self.sec_dut)
        except Exception as err:
            self.log.info(FAILURE.format(cmd, err))

    def do_e2e_sms_receive_different_sizes(self, line):
        """Test recieve sms of different sizes."""
        cmd = "Test recieve sms of different sizes."
        try:
            self.bt_carkit_lib.sms_receive_different_sizes(
                self.pri_dut, self.sec_dut)
        except Exception as err:
            self.log.info(FAILURE.format(cmd, err))

    def do_e2e_sms_receive_multiple(self, line):
        """Test recieve sms of different sizes."""
        cmd = "Test recieve sms of different sizes."
        try:
            self.bt_carkit_lib.sms_receive_multiple(self.pri_dut, self.sec_dut)
        except Exception as err:
            self.log.info(FAILURE.format(cmd, err))

    def do_e2e_sms_send_outgoing_texts(self, line):
        """Test send sms of different sizes."""
        cmd = "Test send sms of different sizes."
        try:
            self.bt_carkit_lib.sms_send_outgoing_texts(self.pri_dut,
                                                       self.sec_dut)
        except Exception as err:
            self.log.info(FAILURE.format(cmd, err))

    def do_e2e_sms_during_incomming_call(self, line):
        """Test incomming call scenario to phone from unknown contact"""
        cmd = "Test incomming call scenario to phone from unknown contact"
        try:
            self.bt_carkit_lib.sms_during_incomming_call(
                self.pri_dut, self.sec_dut)
        except Exception as err:
            self.log.info(FAILURE.format(cmd, err))

    def do_e2e_multi_incomming_call(self, line):
        """Test 2 incomming calls scenario to phone."""
        cmd = "Test 2 incomming calls scenario to phone."
        try:
            self.bt_carkit_lib.multi_incomming_call(self.pri_dut, self.sec_dut,
                                                    self.ter_dut)
        except Exception as err:
            self.log.info(FAILURE.format(cmd, err))

    def do_e2e_multi_call_audio_routing(self, line):
        """Test 2 incomming calls scenario to phone, then test audio routing."""
        cmd = "Test 2 incomming calls scenario to phone, then test audio" \
            "routing."
        try:
            self.bt_carkit_lib.multi_call_audio_routing(
                self.pri_dut, self.sec_dut, self.ter_dut)
        except Exception as err:
            self.log.info(FAILURE.format(cmd, err))

    def do_e2e_multi_call_swap_multiple_times(self, line):
        """Test 2 incomming calls scenario to phone, then swap the calls
        multiple times"""
        cmd = "Test 2 incomming calls scenario to phone, then swap the calls" \
            "multiple times"
        try:
            self.bt_carkit_lib.multi_call_swap_multiple_times(
                self.pri_dut, self.sec_dut, self.ter_dut)
        except Exception as err:
            self.log.info(FAILURE.format(cmd, err))

    def do_e2e_multi_call_join_conference_call(self, line):
        """Test 2 incomming calls scenario to phone then join the calls."""
        cmd = "Test 2 incomming calls scenario to phone then join the calls."
        try:
            self.bt_carkit_lib.multi_call_join_conference_call(
                self.pri_dut, self.sec_dut, self.ter_dut)
        except Exception as err:
            self.log.info(FAILURE.format(cmd, err))

    def do_e2e_multi_call_join_conference_call_hangup_conf_call(self, line):
        """Test 2 incomming calls scenario to phone then join the calls,
        then terminate the call from the primary dut."""
        cmd = "Test 2 incomming calls scenario to phone then join the calls, " \
            "then terminate the call from the primary dut."
        try:
            self.bt_carkit_lib.multi_call_join_conference_call_hangup_conf_call(
                self.pri_dut, self.sec_dut, self.ter_dut)
        except Exception as err:
            self.log.info(FAILURE.format(cmd, err))

    def do_e2e_outgoing_multi_call_join_conference_call(self, line):
        """Test 2 outgoing calls scenario from phone then join the calls."""
        cmd = "Test 2 outgoing calls scenario from phone then join the calls."
        try:
            self.bt_carkit_lib.outgoing_multi_call_join_conference_call(
                self.pri_dut, self.sec_dut, self.ter_dut)
        except Exception as err:
            self.log.info(FAILURE.format(cmd, err))

    def do_e2e_multi_call_join_conference_call_audio_routes(self, line):
        """Test 2 incomming calls scenario to phone then join the calls,
        then test different audio routes."""
        cmd = "Test 2 incomming calls scenario to phone then join the calls, " \
            "then test different audio routes."
        try:
            self.bt_carkit_lib.multi_call_join_conference_call_audio_routes(
                self.pri_dut, self.sec_dut, self.ter_dut)
        except Exception as err:
            self.log.info(FAILURE.format(cmd, err))

    def do_e2e_avrcp_play_pause(self, line):
        """Test avrcp play/pause commands multiple times from phone"""
        cmd = "Test avrcp play/pause commands multiple times from phone"
        try:
            self.bt_carkit_lib.avrcp_play_pause(self.pri_dut)
        except Exception as err:
            self.log.info(FAILURE.format(cmd, err))

    def do_e2e_avrcp_next_previous_song(self, line):
        """Test AVRCP go to the next song then the previous song."""
        cmd = "Test AVRCP go to the next song then the previous song."
        try:
            self.bt_carkit_lib.avrcp_next_previous_song(self.pri_dut)
        except Exception as err:
            self.log.info(FAILURE.format(cmd, err))

    def do_e2e_avrcp_next_previous(self, line):
        """Test AVRCP go to the next song then the press previous after a few
        seconds."""
        cmd = "Test AVRCP go to the next song then the press previous after " \
            "a few seconds."
        try:
            self.bt_carkit_lib.avrcp_next_previous(self.pri_dut)
        except Exception as err:
            self.log.info(FAILURE.format(cmd, err))

    def do_e2e_avrcp_next_repetative(self, line):
        """Test AVRCP go to the next 10 times"""
        cmd = "Test AVRCP go to the next 10 times"
        try:
            self.bt_carkit_lib.avrcp_next_repetative(self.pri_dut)
        except Exception as err:
            self.log.info(FAILURE.format(cmd, err))

    def _process_question(self, question, expected_response):
        while True:
            try:
                result = input(question.format(level)).lower()
            except Exception as err:
                print(err)

    def do_e2e_cycle_battery_level(self, line):
        """Cycle battery level through different values and verify result on carkit"""
        cmd = "Test that verifies battery level indicator changes with the " \
            "phone. Phone current level."
        try:
            self.bt_carkit_lib.cycle_absolute_volume_control(self.pri_dut)
        except Exception as err:
            self.log.info(FAILURE.format(cmd, err))

    def do_e2e_cycle_absolute_volume_control(self, line):
        """Cycle media volume level through different values and verify result on carkit"""
        cmd = "Test aboslute volume on carkit by changed volume levels from phone."
        try:
            self.bt_carkit_lib.cycle_absolute_volume_control(self.pri_dut)
        except Exception as err:
            self.log.info(FAILURE.format(cmd, err))

    def do_e2e_test_voice_recognition_from_phone(self, line):
        """Test Voice Recognition from phone."""
        cmd = "Test voice recognition from phone."
        try:
            self.bt_carkit_lib.test_voice_recognition_from_phone(self.pri_dut)
        except Exception as err:
            self.log.info(FAILURE.format(cmd, err))

    def do_e2e_test_audio_and_voice_recognition_from_phone(self, line):
        """Test Voice Recognition from phone and confirm music audio continues."""
        cmd = "Test Voice Recognition from phone and confirm music audio continues."
        try:
            self.bt_carkit_lib.test_audio_and_voice_recognition_from_phone(
                self.pri_dut)
        except Exception as err:
            self.log.info(FAILURE.format(cmd, err))

    """End carkit test wrappers"""
    """Begin adb shell test wrappers"""

    def do_set_battery_level(self, line):
        """Set battery level based on input"""
        cmd = "Set battery level based on input"
        try:
            self.pri_dut.shell.set_battery_level(int(line))
        except Exception as err:
            self.log.info(FAILURE.format(cmd, err))

    """End adb shell test wrappers"""
