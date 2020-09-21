# /usr/bin/env python3.4
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
"""Compare_contacts accepts 2 vcf files, extracts full name, email, and
telephone numbers from each and reports how many unique cards it finds across
the two files.
"""

from mmap import ACCESS_READ
from mmap import mmap
import logging
import re
import random
import string
import time
from acts.utils import exe_cmd
import queue

# CallLog types
INCOMMING_CALL_TYPE = "1"
OUTGOING_CALL_TYPE = "2"
MISSED_CALL_TYPE = "3"

# Callback strings.
CONTACTS_CHANGED_CALLBACK = "ContactsChanged"
CALL_LOG_CHANGED = "CallLogChanged"
CONTACTS_ERASED_CALLBACK = "ContactsErased"

# URI for contacts database on Nexus.
CONTACTS_URI = "content://com.android.contacts/data/phones"

# Path for temporary file storage on device.
STORAGE_PATH = "/storage/emulated/0/Download/"

PBAP_SYNC_TIME = 30

log = logging


def parse_contacts(file_name):
    """Read vcf file and generate a list of contacts.

    Contacts full name, prefered email, and all phone numbers are extracted.
    """

    vcard_regex = re.compile(b"^BEGIN:VCARD((\n*?.*?)*?)END:VCARD",
                             re.MULTILINE)
    fullname_regex = re.compile(b"^FN:(.*)", re.MULTILINE)
    email_regex = re.compile(b"^EMAIL;PREF:(.*)", re.MULTILINE)
    tel_regex = re.compile(b"^TEL;(.*):(.*)", re.MULTILINE)

    with open(file_name, "r") as contacts_file:
        contacts = []
        contacts_map = mmap(
            contacts_file.fileno(), length=0, access=ACCESS_READ)
        new_contact = None

        # Find all VCARDs in the input file, then extract the first full name,
        # first email address, and all phone numbers from it.  If there is at
        # least a full name add it to the contact list.
        for current_vcard in vcard_regex.findall(contacts_map):
            new_contact = VCard()

            fullname = fullname_regex.search(current_vcard[0])
            if fullname is not None:
                new_contact.name = fullname.group(1)

            email = email_regex.search(current_vcard[0])
            if email is not None:
                new_contact.email = email.group(1)

            for phone_number in tel_regex.findall(current_vcard[0]):
                new_contact.add_phone_number(
                    PhoneNumber(phone_number[0], phone_number[1]))

            contacts.append(new_contact)

        return contacts


def phone_number_count(destination_path, file_name):
    """Counts number of phone numbers in a VCF.
    """
    tel_regex = re.compile(b"^TEL;(.*):(.*)", re.MULTILINE)
    with open("{}{}".format(destination_path, file_name),
              "r") as contacts_file:
        contacts_map = mmap(
            contacts_file.fileno(), length=0, access=ACCESS_READ)
        numbers = tel_regex.findall(contacts_map)
        return len(numbers)


def count_contacts_with_differences(destination_path,
                                    pce_contacts_vcf_file_name,
                                    pse_contacts_vcf_file_name):
    """Compare two contact files and report the number of differences.

    Difference count is returned, and the differences are logged, this is order
    independent.
    """

    pce_contacts = parse_contacts("{}{}".format(destination_path,
                                                pce_contacts_vcf_file_name))
    pse_contacts = parse_contacts("{}{}".format(destination_path,
                                                pse_contacts_vcf_file_name))

    differences = set(pce_contacts).symmetric_difference(set(pse_contacts))
    if not differences:
        log.info("All {} contacts in the phonebooks match".format(
            str(len(pce_contacts))))
    else:
        log.info("{} contacts match, but ".format(
            str(len(set(pce_contacts).intersection(set(pse_contacts))))))
        log.info("the following {} entries don't match:".format(
            str(len(differences))))
        for current_vcard in differences:
            log.info(current_vcard)
    return len(differences)


class PhoneNumber(object):
    """Simple class for maintaining a phone number entry and type with only the
    digits.
    """

    def __init__(self, phone_type, phone_number):
        self.phone_type = phone_type
        # remove non digits from phone_number
        self.phone_number = re.sub(r"\D", "", str(phone_number))

    def __eq__(self, other):
        return (self.phone_type == other.phone_type and
                self.phone_number == other.phone_number)

    def __hash__(self):
        return hash(self.phone_type) ^ hash(self.phone_number)


class VCard(object):
    """Contains name, email, and phone numbers.
    """

    def __init__(self):
        self.name = None
        self.first_name = None
        self.last_name = None
        self.email = None
        self.phone_numbers = []
        self.photo = None

    def __lt__(self, other):
        return self.name < other.name

    def __hash__(self):
        result = hash(self.name) ^ hash(self.email) ^ hash(self.photo == None)
        for number in self.phone_numbers:
            result ^= hash(number)
        return result

    def __eq__(self, other):
        return hash(self) == hash(other)

    def __ne__(self, other):
        return not self.__eq__(other)

    def __str__(self):
        vcard_strings = ["BEGIN:VCARD\n", "VERSION:2.1\n"]

        if self.first_name or self.last_name:
            vcard_strings.append("N:{};{};;;\nFN:{} {}\n".format(
                self.last_name, self.first_name, self.first_name,
                self.last_name))
        elif self.name:
            vcard_strings.append("FN:{}\n".format(self.name))

        if self.phone_numbers:
            for phone in self.phone_numbers:
                vcard_strings.append("TEL;{}:{}\n".format(
                    str(phone.phone_type), phone.phone_number))

        if self.email:
            vcard_strings.append("EMAIL;PREF:{}\n".format(self.email))

        vcard_strings.append("END:VCARD\n")
        return "".join(vcard_strings)

    def add_phone_number(self, phone_number):
        if phone_number not in self.phone_numbers:
            self.phone_numbers.append(phone_number)


def generate_random_phone_number():
    """Generate a random phone number/type
    """
    return PhoneNumber("CELL",
                       "+{0:010d}".format(random.randint(0, 9999999999)))


def generate_random_string(length=8,
                           charset="{}{}{}".format(string.digits,
                                                   string.ascii_letters,
                                                   string.punctuation)):
    """Generate a random string of specified length from the characterset
    """
    # Remove ; since that would make 2 words.
    charset = charset.replace(";", "")
    name = []
    for i in range(length):
        name.append(random.choice(charset))
    return "".join(name)


def generate_contact_list(destination_path,
                          file_name,
                          contact_count,
                          phone_number_count=1):
    """Generate a simple VCF file for count contacts with basic content.

    An example with count = 1 and local_number = 2]

    BEGIN:VCARD
    VERSION:2.1
    N:Person;1;;;
    FN:1 Person
    TEL;CELL:+1-555-555-1234
    TEL;CELL:+1-555-555-4321
    EMAIL;PREF:person1@gmail.com
    END:VCARD
    """
    vcards = []
    for i in range(contact_count):
        current_contact = VCard()
        current_contact.first_name = generate_random_string(
            random.randint(1, 19))
        current_contact.last_name = generate_random_string(
            random.randint(1, 19))
        current_contact.email = "{}{}@{}.{}".format(
            current_contact.last_name, current_contact.first_name,
            generate_random_string(random.randint(1, 19)),
            generate_random_string(random.randint(1, 4)))
        for number in range(phone_number_count):
            current_contact.add_phone_number(generate_random_phone_number())
        vcards.append(current_contact)
    create_new_contacts_vcf_from_vcards(destination_path, file_name, vcards)


def create_new_contacts_vcf_from_vcards(destination_path, vcf_file_name,
                                        vcards):
    """Create a new file with filename
    """
    contact_file = open("{}{}".format(destination_path, vcf_file_name), "w+")
    for card in vcards:
        contact_file.write(str(card))
    contact_file.close()


def get_contact_count(device):
    """Returns the number of name:phone number pairs.
    """
    contact_list = device.droid.contactsQueryContent(
        CONTACTS_URI, ["display_name", "data1"], "", [], "display_name")
    return len(contact_list)


def import_device_contacts_from_vcf(device, destination_path, vcf_file, timeout=10):
    """Uploads and import vcf file to device.
    """
    number_count = phone_number_count(destination_path, vcf_file)
    device.log.info("Trying to add {} phone numbers.".format(number_count))
    local_phonebook_path = "{}{}".format(destination_path, vcf_file)
    phone_phonebook_path = "{}{}".format(STORAGE_PATH, vcf_file)
    device.adb.push("{} {}".format(local_phonebook_path, phone_phonebook_path))
    device.droid.importVcf("file://{}{}".format(STORAGE_PATH, vcf_file))
    start_time = time.time()
    while time.time() < start_time + timeout:
        #TODO: use unattended way to bypass contact import module instead of keyevent
        if "ImportVCardActivity" in device.get_my_current_focus_window():
            # keyevent to allow contacts import from vcf file
            for key in ["DPAD_RIGHT", "DPAD_RIGHT", "ENTER"]:
                device.adb.shell("input keyevent KEYCODE_{}".format(key))
            break
        time.sleep(1)
    if wait_for_phone_number_update_complete(device, number_count):
        return number_count
    else:
        return 0


def export_device_contacts_to_vcf(device, destination_path, vcf_file):
    """Export and download vcf file from device.
    """
    path_on_phone = "{}{}".format(STORAGE_PATH, vcf_file)
    device.droid.exportVcf("{}".format(path_on_phone))
    # Download and then remove file from device
    device.adb.pull("{} {}".format(path_on_phone, destination_path))
    return True


def delete_vcf_files(device):
    """Deletes all files with .vcf extension
    """
    files = device.adb.shell("ls {}".format(STORAGE_PATH))
    for file_name in files.split():
        if ".vcf" in file_name:
            device.adb.shell("rm -f {}{}".format(STORAGE_PATH, file_name))


def erase_contacts(device):
    """Erase all contacts out of devices contact database.
    """
    device.log.info("Erasing contacts.")
    if get_contact_count(device) > 0:
        device.droid.contactsEraseAll()
        try:
            device.ed.pop_event(CONTACTS_ERASED_CALLBACK, PBAP_SYNC_TIME)
        except queue.Empty:
            log.error("Phone book not empty.")
            return False
    return True


def wait_for_phone_number_update_complete(device, expected_count):
    """Check phone_number count on device and wait for updates until it has the
    expected number of phone numbers in its contact database.
    """
    update_completed = True
    try:
        while (expected_count != get_contact_count(device) and
               device.ed.pop_event(CONTACTS_CHANGED_CALLBACK, PBAP_SYNC_TIME)):
            pass
    except queue.Empty:
        log.error("Contacts failed to update.")
        update_completed = False
    device.log.info("Found {} out of the expected {} contacts.".format(
        get_contact_count(device), expected_count))
    return update_completed


def wait_for_call_log_update_complete(device, expected_count):
    """Check call log count on device and wait for updates until it has the
    expected number of calls in its call log database.
    """
    update_completed = True
    try:
        while (expected_count != device.droid.callLogGetCount() and
               device.ed.pop_event(CALL_LOG_CHANGED, PBAP_SYNC_TIME)):
            pass
    except queue.Empty:
        log.error("Call Log failed to update.")
        update_completed = False
    device.log.info("Found {} out of the expected {} call logs.".format(
        device.droid.callLogGetCount(), expected_count))
    return


def add_call_log(device, call_log_type, phone_number, call_time):
    """Add call number and time to specified log.
    """
    new_call_log = {}
    new_call_log["type"] = str(call_log_type)
    new_call_log["number"] = phone_number
    new_call_log["time"] = str(call_time)
    device.droid.callLogsPut(new_call_log)


def get_and_compare_call_logs(pse, pce, call_log_type):
    """Gather and compare call logs from PSE and PCE for the specified type.
    """
    pse_call_log = pse.droid.callLogsGet(call_log_type)
    pce_call_log = pce.droid.callLogsGet(call_log_type)
    return compare_call_logs(pse_call_log, pce_call_log)


def normalize_phonenumber(phone_number):
    """Remove all non-digits from phone_number
    """
    return re.sub(r"\D", "", phone_number)


def compare_call_logs(pse_call_log, pce_call_log):
    """Gather and compare call logs from PSE and PCE for the specified type.
    """
    call_logs_match = True
    if len(pse_call_log) == len(pce_call_log):
        for i in range(len(pse_call_log)):
            # Compare the phone number
            if normalize_phonenumber(pse_call_log[i][
                                         "number"]) != normalize_phonenumber(pce_call_log[i][
                                                                                 "number"]):
                log.warning("Call Log numbers differ")
                call_logs_match = False

            # Compare which log it was taken from (Incomming, Outgoing, Missed
            if pse_call_log[i]["type"] != pce_call_log[i]["type"]:
                log.warning("Call Log types differ")
                call_logs_match = False

            # Compare time to truncated second.
            if int(pse_call_log[i]["date"]) // 1000 != int(pce_call_log[i][
                                                               "date"]) // 1000:
                log.warning("Call log times don't match, check timezone.")
                call_logs_match = False

    else:
        log.warning("Call Log lengths differ {}:{}".format(
            len(pse_call_log), len(pce_call_log)))
        call_logs_match = False

    if not call_logs_match:
        log.info("PSE Call Log:")
        log.info(pse_call_log)
        log.info("PCE Call Log:")
        log.info(pce_call_log)

    return call_logs_match
