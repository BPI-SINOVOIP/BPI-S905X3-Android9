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
from vts.runners.host import base_test
from vts.runners.host import test_runner
from vts.utils.python.controllers import sl4a_client


class SampleSl4aTest(base_test.BaseTestClass):
    """An example showing making SL4A calls in VTS."""

    BATTERY = "battery"

    def setUpClass(self):
        self.dut = self.android_devices[0]

    def testToast(self):
        """A sample test controlling Android device with sl4a.

        Shows a toast message on the device screen.
        """
        logging.info("A toast message should show up on the devce's screen.")
        try:
            for index in range(2):
                self.dut.sl4a.makeToast("Hello World! %s" % index)
                time.sleep(1)
        except sl4a_client.ProtocolError as e:
            asserts.fail("Protocol error in an SL4A operation")
        except sl4a_client.ApiError as e:
            asserts.fail("API error during an SL4A API call")

    def testReadLoud(self):
        """Reads a text."""
        self.dut.sl4a.ttsSpeak("hello Treble VTS")

    def testEvent(self):
        """Tests event dispatching."""
        self.dut.sl4a.batteryStartMonitoring()
        self.dut.sl4a.eventRegisterForBroadcast(self.BATTERY)

        msg = "battery %s" % self.dut.sl4a.eventWaitFor(self.BATTERY, 10000)
        self.dut.sl4a.eventPoll()
        self.dut.sl4a.makeToast(msg)
        self.dut.sl4a.ttsSpeak(msg)

if __name__ == "__main__":
    test_runner.main()
