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
"""Test script to test PBAP contact download between two devices which can run SL4A.
"""

import os
import time

from acts.test_decorators import test_tracker_info
from acts.test_utils.bt.BluetoothBaseTest import BluetoothBaseTest
from acts.test_utils.bt.bt_test_utils import setup_multiple_devices_for_bt_test
from acts.base_test import BaseTestClass
from acts.test_utils.bt import bt_contacts_utils
from acts.test_utils.bt import bt_test_utils
from acts.test_utils.car import car_bt_utils
from acts.utils import exe_cmd
import acts.test_utils.bt.BtEnum as BtEnum

# Offset call logs by 1 minute
CALL_LOG_TIME_OFFSET_IN_MSEC = 60000
PSE_CONTACTS_FILE = "psecontacts.vcf"
PCE_CONTACTS_FILE = "pcecontacts.vcf"
MERGED_CONTACTS_FILE = "psecombined.vcf"
STANDART_CONTACT_COUNT = 100


class BtCarPbapTest(BluetoothBaseTest):
    contacts_destination_path = ""

    def __init__(self, controllers):
        BaseTestClass.__init__(self, controllers)
        self.pce = self.android_devices[0]
        self.pse = self.android_devices[1]
        self.pse2 = self.android_devices[2]
        self.contacts_destination_path = self.log_path + "/"

    def setup_class(self):
        if not super(BtCarPbapTest, self).setup_class():
            return False
        permissions_list = [
            "android.permission.READ_CONTACTS",
            "android.permission.WRITE_CONTACTS",
            "android.permission.READ_EXTERNAL_STORAGE"
        ]
        for device in self.android_devices:
            for permission in permissions_list:
                device.adb.shell(
                    "pm grant com.google.android.contacts {}".format(permission))

        # Pair the devices.
        # This call may block until some specified timeout in bt_test_utils.py.
        # Grace time inbetween stack state changes

        setup_multiple_devices_for_bt_test(self.android_devices)
        if not bt_test_utils.pair_pri_to_sec(self.pce, self.pse):
            self.log.error("Failed to pair.")
            return False
        time.sleep(3)
        if not bt_test_utils.pair_pri_to_sec(self.pce, self.pse2):
            self.log.error("Failed to pair.")
            return False

        # Disable the HFP and A2DP profiles. This will ensure only PBAP
        # gets connected. Also, this will eliminate the auto-connect loop.
        car_bt_utils.set_car_profile_priorities_off(self.pce, self.pse)
        car_bt_utils.set_car_profile_priorities_off(self.pce, self.pse2)

        # Enable PBAP on PSE & PCE.

        self.pse.droid.bluetoothChangeProfileAccessPermission(
            self.pce.droid.bluetoothGetLocalAddress(),
            BtEnum.BluetoothProfile.PBAP_SERVER.value,
            BtEnum.BluetoothAccessLevel.ACCESS_ALLOWED.value)

        self.pse2.droid.bluetoothChangeProfileAccessPermission(
            self.pce.droid.bluetoothGetLocalAddress(),
            BtEnum.BluetoothProfile.PBAP_SERVER.value,
            BtEnum.BluetoothAccessLevel.ACCESS_ALLOWED.value)

        bt_test_utils.set_profile_priority(
            self.pce, self.pse, [BtEnum.BluetoothProfile.PBAP_CLIENT],
            BtEnum.BluetoothPriorityLevel.PRIORITY_ON)
        bt_test_utils.set_profile_priority(
            self.pce, self.pse2, [BtEnum.BluetoothProfile.PBAP_CLIENT],
            BtEnum.BluetoothPriorityLevel.PRIORITY_ON)

        return True

    def setup_test(self):
        if not super(BtCarPbapTest, self).setup_test():
            return False
        self.pse.droid.callLogsEraseAll()
        return self.erase_all_contacts()

    def teardown_test(self):
        if not super(BtCarPbapTest, self).teardown_test():
            return False
        for device in self.android_devices:
            bt_contacts_utils.delete_vcf_files(device)

        self.pce.droid.bluetoothPbapClientDisconnect(
            self.pse.droid.bluetoothGetLocalAddress())
        return self.erase_all_contacts()

    def erase_all_contacts(self):
        try:
            return all(bt_contacts_utils.erase_contacts(device) for device in self.android_devices)
        finally:
            # Allow all content providers to synchronize.
            time.sleep(1)

    def verify_contacts_match(self):
        bt_contacts_utils.export_device_contacts_to_vcf(
            self.pce, self.contacts_destination_path, PCE_CONTACTS_FILE)
        return bt_contacts_utils.count_contacts_with_differences(
            self.contacts_destination_path, PCE_CONTACTS_FILE,
            PSE_CONTACTS_FILE) == 0

    def connect_and_verify(self, count):
        bt_test_utils.connect_pri_to_sec(
            self.pce, self.pse,
            set([BtEnum.BluetoothProfile.PBAP_CLIENT.value]))
        bt_contacts_utils.wait_for_phone_number_update_complete(self.pce,
                                                                count)
        contacts_added = self.verify_contacts_match()
        self.pce.droid.bluetoothPbapClientDisconnect(
            self.pse.droid.bluetoothGetLocalAddress())
        contacts_removed = bt_contacts_utils.wait_for_phone_number_update_complete(
            self.pce, 0)
        return contacts_added and contacts_removed

    @test_tracker_info(uuid='7dcdecfc-42d1-4f41-b66e-823c8f161356')
    @BluetoothBaseTest.bt_test_wrap
    def test_pbap_connect_and_disconnect(self):
        """Test Connectivity

        Test connecting with the server enabled and disabled

        Precondition:
        1. Devices are paired.

        Steps:
        1. Disable permission on PSE to prevent PCE from connecting
        2. Attempt to connect PCE to PSE
        3. Verify connection failed
        4. Enable permission on PSE to allow PCE to connect
        5. Attempt to connect PCE to PSE
        6. Verify connection succeeded

        Returns:
            Pass if True
            Fail if False
        """
        self.pse.droid.bluetoothChangeProfileAccessPermission(
            self.pce.droid.bluetoothGetLocalAddress(),
            BtEnum.BluetoothProfile.PBAP_SERVER.value,
            BtEnum.BluetoothAccessLevel.ACCESS_DENIED.value)
        if bt_test_utils.connect_pri_to_sec(
                self.pce, self.pse,
                set([BtEnum.BluetoothProfile.PBAP_CLIENT.value])):
            self.log.error("Client connected and shouldn't be.")
            return False

        self.pce.droid.bluetoothPbapClientDisconnect(
            self.pse.droid.bluetoothGetLocalAddress())

        self.pse.droid.bluetoothChangeProfileAccessPermission(
            self.pce.droid.bluetoothGetLocalAddress(),
            BtEnum.BluetoothProfile.PBAP_SERVER.value,
            BtEnum.BluetoothAccessLevel.ACCESS_ALLOWED.value)

        if not bt_test_utils.connect_pri_to_sec(
                self.pce, self.pse,
                set([BtEnum.BluetoothProfile.PBAP_CLIENT.value])):
            self.log.error("No client connected and should be.")
            return False

        return True

    @test_tracker_info(uuid='1733efb9-71af-4956-bd3a-0d3167d94d0c')
    @BluetoothBaseTest.bt_test_wrap
    def test_contact_download(self):
        """Test Contact Download

        Test download of contacts from a clean state.

        Precondition:
        1. Devices are paired.

        Steps:
        1. Erase contacts from PSE and PCE.
        2. Add a predefined list of contacts to PSE.
        3. Connect PCE to PSE to perform transfer.
        4. Compare transfered contacts.
        5. Disconnect.
        6. Verify PCE cleaned up contact list.

        Returns:
            Pass if True
            Fail if False
        """
        bt_contacts_utils.generate_contact_list(self.contacts_destination_path,
                                                PSE_CONTACTS_FILE, 100)
        phone_numbers_added = bt_contacts_utils.import_device_contacts_from_vcf(
            self.pse, self.contacts_destination_path, PSE_CONTACTS_FILE)
        bt_test_utils.connect_pri_to_sec(
            self.pce, self.pse,
            set([BtEnum.BluetoothProfile.PBAP_CLIENT.value]))
        bt_contacts_utils.wait_for_phone_number_update_complete(
            self.pce, phone_numbers_added)
        if not self.verify_contacts_match():
            return False
        return bt_contacts_utils.erase_contacts(self.pce)

    @test_tracker_info(uuid='99dc6ac6-b7cf-45ce-927b-8c4ebf8ab664')
    @BluetoothBaseTest.bt_test_wrap
    def test_modify_phonebook(self):
        """Test Modify Phonebook

        Test changing contacts and reconnecting PBAP.

        Precondition:
        1. Devices are paired.

        Steps:
        1. Add a predefined list of contacts to PSE.
        2. Connect PCE to PSE to perform transfer.
        3. Verify that contacts match.
        4. Change some contacts on the PSE.
        5. Reconnect PCE to PSE to perform transfer.
        6. Verify that new contacts match.

        Returns:
            Pass if True
            Fail if False
        """
        bt_contacts_utils.generate_contact_list(self.contacts_destination_path,
                                                PSE_CONTACTS_FILE, 100)
        phone_numbers_added = bt_contacts_utils.import_device_contacts_from_vcf(
            self.pse, self.contacts_destination_path, PSE_CONTACTS_FILE)
        if not self.connect_and_verify(phone_numbers_added):
            return False

        bt_contacts_utils.erase_contacts(self.pse)
        bt_contacts_utils.generate_contact_list(self.contacts_destination_path,
                                                PSE_CONTACTS_FILE, 110, 2)
        phone_numbers_added = bt_contacts_utils.import_device_contacts_from_vcf(
            self.pse, self.contacts_destination_path, PSE_CONTACTS_FILE)
        return self.connect_and_verify(phone_numbers_added)

    @test_tracker_info(uuid='bbe31bf5-51e8-4175-b266-1c7750e44f5b')
    @BluetoothBaseTest.bt_test_wrap
    def test_special_contacts(self):
        """Test Special Contacts

        Test numerous special cases of contacts that could cause errors.

        Precondition:
        1. Devices are paired.

        Steps:
        1. Add a predefined list of contacts to PSE that includes special cases:
        2. Connect PCE to PSE to perform transfer.
        3. Verify that contacts match.

        Returns:
            Pass if True
            Fail if False
        """

        vcards = []

        # Generate a contact with no email address
        current_contact = bt_contacts_utils.VCard()
        current_contact.first_name = "Mr."
        current_contact.last_name = "Smiley"
        current_contact.add_phone_number(
            bt_contacts_utils.generate_random_phone_number())
        vcards.append(current_contact)

        # Generate a 2nd contact with the same name but different phone number
        current_contact = bt_contacts_utils.VCard()
        current_contact.first_name = "Mr."
        current_contact.last_name = "Smiley"
        current_contact.add_phone_number(
            bt_contacts_utils.generate_random_phone_number())
        vcards.append(current_contact)

        # Generate a contact with no name
        current_contact = bt_contacts_utils.VCard()
        current_contact.email = "{}@gmail.com".format(
            bt_contacts_utils.generate_random_string())
        current_contact.add_phone_number(
            bt_contacts_utils.generate_random_phone_number())
        vcards.append(current_contact)

        # Generate a contact with random characters in its name
        current_contact = bt_contacts_utils.VCard()
        current_contact.first_name = bt_contacts_utils.generate_random_string()
        current_contact.last_name = bt_contacts_utils.generate_random_string()
        current_contact.add_phone_number(
            bt_contacts_utils.generate_random_phone_number())
        vcards.append(current_contact)

        # Generate a contact with only a phone number
        current_contact = bt_contacts_utils.VCard()
        current_contact.add_phone_number(
            bt_contacts_utils.generate_random_phone_number())
        vcards.append(current_contact)

        # Generate a 2nd contact with only a phone number
        current_contact = bt_contacts_utils.VCard()
        current_contact.add_phone_number(
            bt_contacts_utils.generate_random_phone_number())
        vcards.append(current_contact)

        bt_contacts_utils.create_new_contacts_vcf_from_vcards(
            self.contacts_destination_path, PSE_CONTACTS_FILE, vcards)

        phone_numbers_added = bt_contacts_utils.import_device_contacts_from_vcf(
            self.pse, self.contacts_destination_path, PSE_CONTACTS_FILE)
        return self.connect_and_verify(phone_numbers_added)

    @test_tracker_info(uuid='2aa2bd00-86cc-4f39-a06a-90b17ea5b320')
    @BluetoothBaseTest.bt_test_wrap
    def test_call_log(self):
        """Test Call Log

        Test that Call Logs are transfered

        Precondition:
        1. Devices are paired.

        Steps:
        1. Add a predefined list of calls to the PSE call log.
        2. Connect PCE to PSE to allow call log transfer
        3. Verify the Missed, Incoming, and Outgoing Call History

        Returns:
            Pass if True
            Fail if False
        """
        bt_contacts_utils.add_call_log(
            self.pse, bt_contacts_utils.INCOMMING_CALL_TYPE,
            bt_contacts_utils.generate_random_phone_number().phone_number,
            int(time.time() * 1000))
        bt_contacts_utils.add_call_log(
            self.pse, bt_contacts_utils.INCOMMING_CALL_TYPE,
            bt_contacts_utils.generate_random_phone_number().phone_number,
            int(time.time()) * 1000 - 4 * CALL_LOG_TIME_OFFSET_IN_MSEC)
        bt_contacts_utils.add_call_log(
            self.pse, bt_contacts_utils.OUTGOING_CALL_TYPE,
            bt_contacts_utils.generate_random_phone_number().phone_number,
            int(time.time()) * 1000 - CALL_LOG_TIME_OFFSET_IN_MSEC)
        bt_contacts_utils.add_call_log(
            self.pse, bt_contacts_utils.MISSED_CALL_TYPE,
            bt_contacts_utils.generate_random_phone_number().phone_number,
            int(time.time()) * 1000 - 2 * CALL_LOG_TIME_OFFSET_IN_MSEC)
        bt_contacts_utils.add_call_log(
            self.pse, bt_contacts_utils.MISSED_CALL_TYPE,
            bt_contacts_utils.generate_random_phone_number().phone_number,
            int(time.time()) * 1000 - 2 * CALL_LOG_TIME_OFFSET_IN_MSEC)

        self.pce.droid.bluetoothPbapClientDisconnect(
            self.pse.droid.bluetoothGetLocalAddress())
        self.pce.droid.bluetoothPbapClientDisconnect(
            self.pse2.droid.bluetoothGetLocalAddress())

        bt_test_utils.connect_pri_to_sec(
            self.pce, self.pse,
            set([BtEnum.BluetoothProfile.PBAP_CLIENT.value]))
        pse_call_log_count = self.pse.droid.callLogGetCount()
        self.log.info("Waiting for {} call logs to be transfered".format(
            pse_call_log_count))
        bt_contacts_utils.wait_for_call_log_update_complete(self.pce,
                                                            pse_call_log_count)

        if not bt_contacts_utils.get_and_compare_call_logs(
                self.pse, self.pce, bt_contacts_utils.INCOMMING_CALL_TYPE):
            return False
        if not bt_contacts_utils.get_and_compare_call_logs(
                self.pse, self.pce, bt_contacts_utils.OUTGOING_CALL_TYPE):
            return False
        if not bt_contacts_utils.get_and_compare_call_logs(
                self.pse, self.pce, bt_contacts_utils.MISSED_CALL_TYPE):
            return False

        return True

    @test_tracker_info(uuid='bb018bf4-5a61-478d-acce-eef88050e489')
    @BluetoothBaseTest.bt_test_wrap
    def test_multiple_phones(self):
        """Test Multiple Phones

        Test that connects two phones and confirms contacts are transfered
        and merged while still being associated with their original phone.

        Precondition:
        1. Devices are paired.

        Steps:
        1. Add a unique list of contacts to PSE on each phone.
        2. Connect PCE to PSE 1 to perform transfer.
        3. Verify contacts match.
        4. Connect PCE to PSE 2 to perform transfer.
        5. Verify that the PCE has a union set of contacts from
           PSE 1 and PSE 2.
        6. Disconnect PCE from PSE 1 to clean up contacts.
        7. Verify that only PSE 2 contacts remain on PCE and they match.
        8. Disconnect PCE from PSE 2 to clean up contacts.

        Returns:
           Pass if True
           Fail if False
        """
        PSE1_CONTACTS_FILE = "{}{}".format(PSE_CONTACTS_FILE, "1")
        PSE2_CONTACTS_FILE = "{}{}".format(PSE_CONTACTS_FILE, "2")

        bt_contacts_utils.generate_contact_list(self.contacts_destination_path,
                                                PSE1_CONTACTS_FILE, 100)
        bt_contacts_utils.import_device_contacts_from_vcf(self.pse,
            self.contacts_destination_path,
            PSE1_CONTACTS_FILE)
        bt_contacts_utils.generate_contact_list(self.contacts_destination_path,
                                                PSE2_CONTACTS_FILE, 100)
        bt_contacts_utils.import_device_contacts_from_vcf(self.pse2,
            self.contacts_destination_path,
            PSE2_CONTACTS_FILE)

        self.pce.droid.bluetoothPbapClientDisconnect(
            self.pse.droid.bluetoothGetLocalAddress())
        self.pce.droid.bluetoothPbapClientDisconnect(
            self.pse2.droid.bluetoothGetLocalAddress())

        bt_test_utils.connect_pri_to_sec(
            self.pce, self.pse,
            set([BtEnum.BluetoothProfile.PBAP_CLIENT.value]))
        bt_contacts_utils.wait_for_phone_number_update_complete(self.pce, 100)
        bt_contacts_utils.export_device_contacts_to_vcf(
            self.pce, self.contacts_destination_path, PCE_CONTACTS_FILE)
        pse1_matches = bt_contacts_utils.count_contacts_with_differences(
            self.contacts_destination_path, PCE_CONTACTS_FILE,
            PSE1_CONTACTS_FILE) == 0

        bt_test_utils.connect_pri_to_sec(
            self.pce, self.pse2,
            set([BtEnum.BluetoothProfile.PBAP_CLIENT.value]))
        bt_contacts_utils.wait_for_phone_number_update_complete(self.pce, 200)
        bt_contacts_utils.export_device_contacts_to_vcf(
            self.pce, self.contacts_destination_path, PCE_CONTACTS_FILE)

        merged_file = open('{}{}'.format(self.contacts_destination_path,
                                         MERGED_CONTACTS_FILE), 'w')
        for contacts_file in [PSE1_CONTACTS_FILE, PSE2_CONTACTS_FILE]:
            infile = open(self.contacts_destination_path + contacts_file)
            merged_file.write(infile.read())

        self.log.info("Checking combined phonebook.")
        pse1andpse2_matches = bt_contacts_utils.count_contacts_with_differences(
            self.contacts_destination_path, PCE_CONTACTS_FILE,
            MERGED_CONTACTS_FILE) == 0

        self.pce.droid.bluetoothPbapClientDisconnect(
            self.pse.droid.bluetoothGetLocalAddress())
        bt_contacts_utils.wait_for_phone_number_update_complete(self.pce, 100)

        self.log.info("Checking phonebook after disconnecting first device.")
        bt_contacts_utils.export_device_contacts_to_vcf(
            self.pce, self.contacts_destination_path, PCE_CONTACTS_FILE)
        pse2_matches = bt_contacts_utils.count_contacts_with_differences(
            self.contacts_destination_path, PCE_CONTACTS_FILE,
            PSE2_CONTACTS_FILE) == 0

        bt_contacts_utils.erase_contacts(self.pse)
        bt_contacts_utils.erase_contacts(self.pse2)
        return pse1_matches and pse2_matches and pse1andpse2_matches