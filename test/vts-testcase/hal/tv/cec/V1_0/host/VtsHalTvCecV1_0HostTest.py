#!/usr/bin/env python
#
# Copyright (C) 2016 The Android Open Source Project
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

from vts.runners.host import asserts
from vts.runners.host import test_runner
from vts.testcases.template.hal_hidl_host_test import hal_hidl_host_test

TVCEC_V1_0_HAL = "android.hardware.tv.cec@1.0::IHdmiCec"

class TvCecHidlTest(hal_hidl_host_test.HalHidlHostTest):
    """Host testcase class for the TV HDMI_CEC HIDL HAL."""

    TEST_HAL_SERVICES = {TVCEC_V1_0_HAL}
    def setUpClass(self):
        """Creates a mirror and init tv hdmi cec hal service."""
        super(TvCecHidlTest, self).setUpClass()

        self.dut.hal.InitHidlHal(
            target_type="tv_cec",
            target_basepaths=self.dut.libPaths,
            target_version=1.0,
            target_package="android.hardware.tv.cec",
            target_component_name="IHdmiCec",
            hw_binder_service_name=self.getHalServiceName(TVCEC_V1_0_HAL),
            bits=int(self.abi_bitness))

        time.sleep(1) # Wait for hal to be ready

        self.vtypes = self.dut.hal.tv_cec.GetHidlTypeInterface("types")
        logging.info("tv_cec types: %s", self.vtypes)

    def testClearAndAddLogicalAddress(self):
        """A simple test case which sets logical address and clears it."""
        self.dut.hal.tv_cec.clearLogicalAddress()
        result = self.dut.hal.tv_cec.addLogicalAddress(
                self.vtypes.CecLogicalAddress.PLAYBACK_3)
        asserts.assertEqual(self.vtypes.Result.SUCCESS, result)
        logging.info("addLogicalAddress result: %s", result)

    def testGetPhysicalAddress(self):
        """A simple test case which queries the physical address."""
        status, paddr = self.dut.hal.tv_cec.getPhysicalAddress()
        asserts.assertEqual(self.vtypes.Result.SUCCESS, status)
        logging.info("getPhysicalAddress status: %s, paddr: %s", status, paddr)

    def testSendRandomMessage(self):
        """A test case which sends a random message."""
        cec_message = {
            "initiator": self.vtypes.CecLogicalAddress.TV,
            "destination": self.vtypes.CecLogicalAddress.PLAYBACK_1,
            "body": [1, 2, 3]
        }
        message = self.vtypes.Py2Pb("CecMessage", cec_message)
        logging.info("message: %s", message)
        result = self.dut.hal.tv_cec.sendMessage(message)
        logging.info("sendMessage result: %s", result)

    def testGetCecVersion1(self):
        """A simple test case which queries the cec version."""
        version = self.dut.hal.tv_cec.getCecVersion()
        logging.info("getCecVersion version: %s", version)

    def testGetVendorId(self):
        """A simple test case which queries vendor id."""
        vendor_id = self.dut.hal.tv_cec.getVendorId()
        asserts.assertEqual(0, 0xff000000 & vendor_id)
        logging.info("getVendorId vendor_id: %s", vendor_id)

    def testGetPortInfo(self):
        """A simple test case which queries port information."""
        port_infos = self.dut.hal.tv_cec.getPortInfo()
        logging.info("getPortInfo port_infos: %s", port_infos)

    def testSetOption(self):
        """A simple test case which changes CEC options."""
        self.dut.hal.tv_cec.setOption(self.vtypes.OptionKey.WAKEUP, True)
        self.dut.hal.tv_cec.setOption(self.vtypes.OptionKey.ENABLE_CEC, True)
        self.dut.hal.tv_cec.setOption(
                self.vtypes.OptionKey.SYSTEM_CEC_CONTROL, True)

    def testSetLanguage(self):
        """A simple test case which updates language information."""
        self.dut.hal.tv_cec.setLanguage("eng")

    def testEnableAudioReturnChannel(self):
        """Checks whether Audio Return Channel can be enabled."""
        port_infos = self.dut.hal.tv_cec.getPortInfo()
        for port_info in port_infos:
            if "portId" in port_info and port_info.get("arcSupported"):
                self.dut.hal.tv_cec.enableAudioReturnChannel(
                        port_info["portId"], True)

    def testIsConnected(self):
        """A simple test case which queries the connected status."""
        status = self.dut.hal.tv_cec.isConnected()
        logging.info("isConnected status: %s", status)

if __name__ == "__main__":
    test_runner.main()
