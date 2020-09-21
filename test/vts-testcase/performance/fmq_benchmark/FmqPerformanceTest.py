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

from vts.runners.host import asserts
from vts.runners.host import base_test
from vts.runners.host import const
from vts.runners.host import test_runner
from vts.utils.python.controllers import android_device
from vts.utils.python.cpu import cpu_frequency_scaling


class FmqPerformanceTest(base_test.BaseTestClass):
    """A testcase for the Fast Message Queue(fmq) Performance Benchmarking.

    Attributes:
        dut: the target DUT (device under test) instance.
        _cpu_freq: CpuFrequencyScalingController instance of self.dut.
    """
    # Latency threshold for the benchmark,  unit: nanoseconds.
    THRESHOLD = {
        32: {
            "64": 300,
            "128": 300,
            "256": 300,
            "512": 350,
        },
        64: {
            "64": 150,
            "128": 150,
            "256": 150,
            "512": 150,
        }
    }

    def setUpClass(self):
        self.dut = self.registerController(android_device)[0]
        self.dut.shell.InvokeTerminal("one")
        self._cpu_freq = cpu_frequency_scaling.CpuFrequencyScalingController(
            self.dut)
        self._cpu_freq.DisableCpuScaling()

    def tearDownClass(self):
        self._cpu_freq.EnableCpuScaling()

    def setUp(self):
        self._cpu_freq.SkipIfThermalThrottling(retry_delay_secs=30)

    def tearDown(self):
        self._cpu_freq.SkipIfThermalThrottling()

    def testRunBenchmark32Bit(self):
        """A testcase which runs the 32-bit benchmark."""
        self.RunBenchmark(32)

    def testRunBenchmark64Bit(self):
        """A testcase which runs the 64-bit benchmark."""
        self.RunBenchmark(64)

    def RunBenchmark(self, bits):
        """Runs the native binary and parses its result.

        Args:
            bits: integer (32 or 64), the number of bits in a word chosen
                  at the compile time (e.g., 32- vs. 64-bit library).
        """
        # Start the benchmark service.
        logging.info("Start the benchmark service(%s bit mode)", bits)
        binary = "/data/local/tmp/%s/mq_benchmark_service%s" % (bits, bits)
        results = self.dut.shell.one.Execute([
            "chmod 755 %s" % binary,
            "VTS_ROOT_PATH=/data/local/tmp TREBLE_TESTING_OVERRIDE=true " \
            "LD_LIBRARY_PATH=/data/local/tmp/%s:"
            "$LD_LIBRARY_PATH %s&" % (bits, binary)
        ])
        asserts.assertEqual(len(results[const.STDOUT]), 2)
        asserts.assertFalse(
            any(results[const.EXIT_CODE]),
            "Failed to start the benchmark service.")

        # Runs the benchmark.
        logging.info("Start to run the benchmark (%s bit mode)", bits)
        binary = "/data/local/tmp/%s/mq_benchmark_client%s" % (bits, bits)

        results = self.dut.shell.one.Execute([
            "chmod 755 %s" % binary,
            "TREBLE_TESTING_OVERRIDE=true LD_LIBRARY_PATH=/data/local/tmp/%s:"
            "$LD_LIBRARY_PATH %s" % (bits, binary)
        ])

        # Stop the benchmark service.
        self.dut.shell.one.Execute("kill -9 `pidof mq_benchmark_service%s`" %
                                   bits)

        # Parses the result.
        asserts.assertEqual(len(results[const.STDOUT]), 2)
        asserts.assertFalse(
            any(results[const.EXIT_CODE]), "FmqPerformanceTest failed.")
        read_label = []
        read_latency = []
        write_label = []
        write_latency = []
        stdout_lines = results[const.STDOUT][1].split("\n")
        for line in stdout_lines:
            if line.startswith("Average time to read"):
                read_result = line.replace("Average time to read", "").replace(
                    "bytes", "").replace("ns", "")
                (label, value) = read_result.split(": ")
                read_label.append(label)
                read_latency.append(int(value))
            if line.startswith("Average time to write"):
                write_result = line.replace("Average time to write ",
                                            "").replace("bytes",
                                                        "").replace("ns", "")
                (label, value) = write_result.split(": ")
                write_label.append(label)
                write_latency.append(int(value))

        # To upload to the web DB.
        self.web.AddProfilingDataLabeledVector(
            "fmq_read_latency_benchmark_%sbits" % bits,
            read_label,
            read_latency,
            x_axis_label="Message Size (Bytes)",
            y_axis_label="Average Latency (nanoseconds)")

        self.web.AddProfilingDataLabeledVector(
            "fmq_write_latency_benchmark_%sbits" % bits,
            write_label,
            write_latency,
            x_axis_label="Message Size (Bytes)",
            y_axis_label="Average Latency (nanoseconds)")

        # Assertions to check the performance requirements
        for label, value in zip(read_label, read_latency):
            if label in self.THRESHOLD[bits]:
                asserts.assertLess(
                    value, self.THRESHOLD[bits][label],
                    "%s ns for %s is longer than the threshold %s ns" % (
                        value, label, self.THRESHOLD[bits][label]))

        for label, value in zip(write_label, write_latency):
            if label in self.THRESHOLD[bits]:
                asserts.assertLess(
                    value, self.THRESHOLD[bits][label],
                    "%s ns for %s is longer than the threshold %s ns" % (
                        value, label, self.THRESHOLD[bits][label]))


if __name__ == "__main__":
    test_runner.main()
