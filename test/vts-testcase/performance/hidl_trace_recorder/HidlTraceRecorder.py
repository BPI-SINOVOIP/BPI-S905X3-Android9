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

import datetime
import logging
import os

from vts.runners.host import asserts
from vts.runners.host import test_runner
from vts.testcases.template.cts_test import cts_test


class HidlTraceRecorder(cts_test.CtsTest):
    """A HIDL HAL API trace recorder.

    This class uses an apk which is packaged as part of VTS. It uses to test the
    VTS TF's instrumentation preparer and post-processing modules. Those two
    Java TF-side modules are cherry-picked to CTS to collect HIDL traces while
    running various CTS test cases without having to package them as part of
    VTS.
    """

    REMOTE_PROFILINT_TRACE_PATH = "/google/data/rw/teams/android-vts/cts-traces"

    def RunTestCase(self, test_case):
        '''Runs a test_case.

        Args:
            test_case: a cts test config.
        '''
        # before running the cts test module enable profiling.
        self.profiling.EnableVTSProfiling(
            self.dut.shell.one, hal_instrumentation_lib_path="")
        self.dut.stop()  # stop framework
        self.dut.start()  # start framework

        profiling_trace_path = os.path.join(
            self.REMOTE_PROFILINT_TRACE_PATH,
            datetime.datetime.now().strftime("%Y%m%d"),
            self.GetTestName(test_case))
        if not os.path.exists(profiling_trace_path):
            os.makedirs(profiling_trace_path)

        logging.info("Run %s", self.GetTestName(test_case))
        self.dut.adb.shell("am instrument -w -r %s/%s" % (test_case["package"],
                                                          test_case["runner"]))

        # after running the cts test module, copy trace files and disable profiling.
        self.profiling.GetTraceFiles(self.dut, profiling_trace_path)
        self.profiling.DisableVTSProfiling(self.dut.shell.one)

if __name__ == "__main__":
    test_runner.main()
