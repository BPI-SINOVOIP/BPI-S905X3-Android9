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
import math
import os

from time import sleep
from vts.runners.host import asserts
from vts.runners.host import base_test
from vts.runners.host import const
from vts.runners.host import test_runner
from vts.utils.python.controllers import android_device


class AudioLoopbackTest(base_test.BaseTestClass):
    """A test module for the Audio Loopback Benchmark."""

    # Threshold for average latency and standard deviation.
    THRESHOLD = {"MAX_LATENCY": 20000000, }

    TEST_DATA_DIR = "audiotest"
    FULL_DATA_DIR_PATH = os.path.join("/mnt/sdcard/", TEST_DATA_DIR)
    TEST_FILE_PREFIX = "out"
    TEST_FILE_NAME = os.path.join(FULL_DATA_DIR_PATH,
                                  TEST_FILE_PREFIX + ".txt")
    ITERATION = 100

    def setUpClass(self):
        self.dut = self.registerController(android_device)[0]
        self.dut.shell.InvokeTerminal("one")
        self.dut.adb.shell("mkdir -p %s" % self.FULL_DATA_DIR_PATH)
        # install Loopback.apk
        self.dut.adb.shell("pm install -r -g /data/local/tmp/Loopback.apk")

    def tearDown(self):
        self.dut.adb.shell("rm -rf %s" % self.FULL_DATA_DIR_PATH)

    def ProcessTestResults(self, latencies, confidences):
        """Process test results and upload to web dashboard.

        Calculate the average and standard deviation of latencies from all test run.
        Only test results with confidence = 1.0 are counted.

        Args:
           latencies: List of latency (in ns) get from each run. e.g.[8040000]
           confidences: List of confidence get from each run. e.g. [0.98, 1.0]
        """
        total_latency = 0
        total_run = 0
        for latency, confidence in zip(latencies, confidences):
            # filter test runs with confidence < 1.0
            if confidence < 1.0:
                latencies.remove(latency)
            else:
                total_latency += latency
                total_run += 1
        asserts.assertLess(0, total_run, "No valid runs.")
        self.web.AddProfilingDataUnlabeledVector(
            "AVG_LATENCY",
            latencies,
            x_axis_label="AVG Roundtrip latency (ns)",
            y_axis_label="Frequency")
        # calculate the average latency.
        avg_latency = total_latency / total_run
        logging.info("avg_latency: %s", avg_latency)
        asserts.assertLess(avg_latency, self.THRESHOLD["MAX_LATENCY"],
                           "avg latency exceeds threshold")
        # calculate the standard deviation of latencies.
        sd = 0.0
        for latency in latencies:
            sd += (latency - avg_latency) * (latency - avg_latency)
        sd = int(math.sqrt(sd / total_run))
        logging.info("standard_deviation: %s", sd)
        self.web.AddProfilingDataUnlabeledVector(
            "STANDARD_DEVIATION", [sd],
            x_axis_label="Standard deviation",
            y_axis_label="Frequency")

    def testRun(self):
        """Runs test in audio Loopback.apk and process the test results."""
        latencies = []
        confidences = []
        for i in range(0, self.ITERATION):
            self.dut.shell.one.Execute(
                ["rm -f %s/*" % self.FULL_DATA_DIR_PATH])
            self.dut.shell.one.Execute([
                "am start -n org.drrickorang.loopback/.LoopbackActivity "
                "--es FileName %s/%s --ei AudioLevel 11 --ei TestType 222;" %
                (self.TEST_DATA_DIR, self.TEST_FILE_PREFIX)
            ])
            # wait until the test finished.
            results = self.dut.shell.one.Execute(
                ["ls %s" % self.TEST_FILE_NAME])
            while results[const.EXIT_CODE][0]:
                logging.info("Test is running...")
                sleep(1)
                results = self.dut.shell.one.Execute(
                    ["ls %s" % self.TEST_FILE_NAME])

            results = self.dut.shell.one.Execute(
                ["cat %s" % self.TEST_FILE_NAME])
            asserts.assertFalse(results[const.EXIT_CODE][0],
                                "Fail to get the test output")
            stdout_lines = results[const.STDOUT][0].split("\n")
            for line in stdout_lines:
                if line.startswith("LatencyMs"):
                    latencies.append(
                        int(
                            float(line.replace("LatencyMs = ", "")) * 1000000))
                if line.startswith("LatencyConfidence"):
                    confidences.append(
                        float(line.replace("LatencyConfidence = ", "")))
        self.ProcessTestResults(latencies, confidences)


if __name__ == "__main__":
    test_runner.main()
