#!/usr/bin/env python
#
# Copyright (C) 2017 The Android Open Source Project
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

from vts.runners.host import test_runner
from vts.testcases.template.cts_test import cts_test


class VtsHalCoverageMeasurement(cts_test.CtsTest):
    """A code coverage measurement test for CTS.

    This class uses an apk which is packaged as part of VTS. It is used to measure
    the code coverage of hal implementation accessed by the apk.
    """

    def setUpClass(self):
        super(VtsHalCoverageMeasurement, self).setUpClass()
        self.dut.start()  # start the framework.

        # Initialization for coverage measurement.
        if self.coverage.enabled and self.coverage.global_coverage:
            self.coverage.InitializeDeviceCoverage(self.dut)

    def tearDownClass(self):
        if self.coverage.enabled and self.coverage.global_coverage:
            self.coverage.SetCoverageData(dut=self.dut, isGlobal=True)
        super(VtsHalCoverageMeasurement, self).tearDownClass()

    def RunTestCase(self, test_case):
        '''Runs a test_case.

        Args:
            test_case: a cts test config.
        '''
        logging.info("Installing %s", test_case["apk"])
        self.dut.adb.install(test_case["apk"])
        logging.info("Run %s", test_case["name"])
        self.dut.adb.shell("am instrument -w -r %s/%s" % (test_case["package"],
                                                          test_case["runner"]))

        logging.info("Uninstalling %s", test_case["apk"])
        self.dut.adb.uninstall(test_case["package"])


if __name__ == "__main__":
    test_runner.main()
