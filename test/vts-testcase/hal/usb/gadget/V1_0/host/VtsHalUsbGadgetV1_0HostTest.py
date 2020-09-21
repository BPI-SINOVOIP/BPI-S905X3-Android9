#!/usr/bin/env python
#
# Copyright (C) 2018 The Android Open Source Project
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
#

import logging
import time
import usb1

from vts.runners.host import asserts
from vts.runners.host import base_test
from vts.runners.host import const
from vts.runners.host import test_runner
from vts.testcases.template.hal_hidl_host_test import hal_hidl_host_test
from vts.utils.python.controllers import adb


class UsbGadgetHidlTest(hal_hidl_host_test.HalHidlHostTest):
    """A host-side test for USB Gadget HAL.

    This test requires Android framework to run.
    """

    TEST_HAL_SERVICES = {"android.hardware.usb.gadget@1.0::IUsbGadget"}

    def setUpClass(self):
        """Creates an adb session and reads sysprop values."""
        super(UsbGadgetHidlTest, self).setUpClass()

        self.adb = self.dut.adb
        try:
            self.adb.root()
            self.adb.wait_for_device()
        except adb.AdbError as e:
            logging.exception(e)
        self.serialno = self.adb.shell("\"getprop ro.serialno\"")

    def checkProtocol(self, usb_class, usb_sub_class, usb_protocol):
        """Queries the host USB bus to see if the interface is present.

        Args:
            usb_class: usbClass id of the interface.
            usb_sub_class: usbSubClass id of the interface.
            usb_protocol: usbProtocol id of the interface.

        Returns:
            True if the usb interface was present. False otherwise.
        """
        with usb1.USBContext() as context:
            for device in context.getDeviceIterator(skip_on_error=True):
                logging.info("ID %04x:%04x ", device.getVendorID(),
                             device.getProductID())
                for config in device.iterConfigurations():
                    logging.info("config: %d", config.getConfigurationValue())
                    interfaces_list = iter(config)
                    for interface in interfaces_list:
                        altsettings_list = iter(interface)
                        for altsetting in altsettings_list:
                            logging.info("interfaceNum:%d altSetting:%d "
                                         "class:%d subclass:%d protocol:%d",
                                         altsetting.getNumber(),
                                         altsetting.getAlternateSetting(),
                                         altsetting.getClass(),
                                         altsetting.getSubClass(),
                                         altsetting.getProtocol())
                            if altsetting.getClass() == usb_class and \
                                altsetting.getSubClass() == usb_sub_class and \
                                altsetting.getProtocol() == usb_protocol:
                                return True
        return False

    def testAdb(self):
        """Check for ADB"""
        asserts.assertTrue(self.checkProtocol(255, 66, 1), "ADB not present")

    def testMtp(self):
        """Check for MTP.

        Enables mtp and checks the host to see if mtp interface is present.
        MTP: https://en.wikipedia.org/wiki/Media_Transfer_Protocol.
        """
        self.adb.shell("svc usb setFunctions mtp true")
        time.sleep(3)
        asserts.assertTrue(self.checkProtocol(6, 1, 1), "MTP not present")

    def testPtp(self):
        """Check for PTP.

        Enables ptp and checks the host to see if ptp interface is present.
        PTP: https://en.wikipedia.org/wiki/Picture_Transfer_Protocol.
        """
        self.adb.shell("svc usb setFunctions ptp true")
        time.sleep(3)
        asserts.assertTrue(self.checkProtocol(6, 1, 1), "PTP not present")

    def testMIDI(self):
        """Check for MIDI.

        Enables midi and checks the host to see if midi interface is present.
        MIDI: https://en.wikipedia.org/wiki/MIDI.
        """
        self.adb.shell("svc usb setFunctions midi true")
        time.sleep(3)
        asserts.assertTrue(self.checkProtocol(1, 3, 0), "MIDI not present")

    def testRndis(self):
        """Check for RNDIS.

        Enables rndis and checks the host to see if rndis interface is present.
        RNDIS: https://en.wikipedia.org/wiki/RNDIS.
        """
        self.adb.shell("svc usb setFunctions rndis true")
        time.sleep(3)
        asserts.assertTrue(self.checkProtocol(10, 0, 0), "RNDIS not present")


if __name__ == "__main__":
    test_runner.main()
