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
import os

from vts.runners.host import const
from vts.runners.host import keys
from vts.runners.host import test_runner
from vts.utils.python.controllers import android_device

from vts.testcases.template.gtest_binary_test import gtest_binary_test
from vts.testcases.kernel.cpu_profiling import cpu_profiling_test_config as config


class CpuProfilingTest(gtest_binary_test.GtestBinaryTest):
    """Runs cpu profiling test cases against Android OS kernel."""

    def setUpClass(self):
        """Creates a remote shell instance, and copies data files."""
        super(CpuProfilingTest, self).setUpClass()
        required_params = ["AndroidDevice"]
        self.getUserParams(req_param_names=required_params)
        self.product_type = self.AndroidDevice[0]['product_type']

    def generateAllTests(self):
        """Runs all gtests. Skip if device is excluded"""
        if self.product_type in config.CPT_HOTPLUG_EXCLUDE_DEVICES:
            logging.info("Skip test on device {}.".format(self.product_type))
            return

        super(CpuProfilingTest, self).generateAllTests()


if __name__ == "__main__":
    test_runner.main()
