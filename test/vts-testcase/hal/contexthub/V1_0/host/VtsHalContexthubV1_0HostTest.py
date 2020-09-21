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
import threading
import time

from vts.runners.host import asserts
from vts.runners.host import test_runner
from vts.testcases.template.hal_hidl_host_test import hal_hidl_host_test


class ContextHubCallback:
    def __init__(self, hub_id):
        self.hub_id = hub_id
        self.event = threading.Event()

    def wait_on_callback(timeout=None):
        """Wait on the next callback in this object to be invoked.

        Args:
            timeout: (fractional) seconds to wait before timing out, or None

        Returns:
            True when a callback was received, False if a timeout occurred
        """
        return self.event.wait(timeout)

    def prepare():
        # TODO: cleaner method of doing this, so that we clear --> call HAL
        # method --> wait for CB, all wrapped into one
        self.event.clear()

    def ignore_client_msg(self, msg):
        logging.debug("Ignoring client message from hubId %s: %s", self.hub_id, msg)
        self.event.set()

    def ignore_txn_result(self, txnId, result):
        logging.debug("Ignoring transaction result from hubId %s: %s", self.hub_id, msg)
        self.event.set()

    def ignore_hub_event(self, evt):
        logging.debug("Ignoring hub event from hubId %s: %s", self.hub_id, evt)
        self.event.set()

    def ignore_apps_info(self, appInfo):
        logging.debug("Ignoring app info data from hubId %s: %s", self.hub_id, appInfo)
        self.event.set()


class ContexthubHidlTest(hal_hidl_host_test.HalHidlHostTest):
    """A set of test cases for the context hub HIDL HAL"""
    TEST_HAL_SERVICES = {
        "android.hardware.contexthub@1.0::IContexthub",
    }

    def setUpClass(self):
        """Creates a mirror and turns on the framework-layer CONTEXTHUB service."""
        super(ContexthubHidlTest, self).setUpClass()
        self.dut.hal.InitHidlHal(
            target_type="contexthub",
            target_basepaths=self.dut.libPaths,
            target_version=1.0,
            target_package="android.hardware.contexthub",
            target_component_name="IContexthub",
            bits=int(self.abi_bitness))

        self.types = self.dut.hal.contexthub.GetHidlTypeInterface("types")
        logging.info("types: %s", self.types)

    def testContexthubBasic(self):
        logging.info("About to call gethubs!!!")
        hubs = self.dut.hal.contexthub.getHubs()
        logging.info("Got result: %s", hubs)
        #
        #hub_id = 0 # TODO: should get this from hub list
        #
        #cb = ContextHubCallback(hub_id)
        #client_callback = self.dut.hal.contexthub.GetHidlCallbackInterface(
        #    "IContexthubCallback",
        #    handleClientMsg=cb.ignore_client_msg,
        #    handleTxnResult=cb.ignore_txn_result,
        #    handleHubEvent=cb.ignore_hub_event,
        #    handleAppsInfo=cb.ignore_apps_info)
        #
        #logging.info("About to call registerCallback")
        #result = self.dut.hal.contexthub.registerCallback(hub_id,
        #                                                  client_callback)
        #logging.info("registerCallback returned result: %s", result)
        #
        #logging.info("About to call queryApps")
        #result = self.dut.hal.contexthub.queryApps(hub_id)
        #logging.info("queryApps returned result: %s", result)
        #
        #got_callback = cb.wait_on_callback(5)
        #logging.info("Wait on callback returned %s", got_callback)

if __name__ == "__main__":
    test_runner.main()
