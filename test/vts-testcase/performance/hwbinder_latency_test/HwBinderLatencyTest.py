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

import json
import logging

from vts.runners.host import asserts
from vts.runners.host import base_test
from vts.runners.host import const
from vts.runners.host import test_runner
from vts.utils.python.controllers import android_device
from vts.utils.python.cpu import cpu_frequency_scaling


class HwBinderLatencyTest(base_test.BaseTestClass):
    """A test case for the hwbinder latency benchmarking.

    Sample output of libhwbinder_latency
    {
      "cfg":{"pair":6,"iterations":166,"deadline_us":2500,"passthrough":1},
      "fifo_0_data": [15052, ...],
      "fifo_1_data": [...],
      ...
      "ALL":{"SYNC":"GOOD","S":1992,"I":1992,"R":1,
        "other_ms":{ "avg":0.0048, "wst":0.32, "bst":0.0022, "miss":0, "meetR":1},
        "fifo_ms": { "avg":0.0035, "wst":0.037, "bst":0.0021, "miss":0, "meetR":1},
        "otherdis":{ "p50":0.19531, "p90":0.19531, "p95":0.19531, "p99": 0.19531},
        "fifodis": { "p50":0.19531, "p90":0.19531, "p95":0.19531, "p99": 0.19531}
      },
      "P0":{...
      },
      ...
      "inheritance": "PASS"
    }
    """
    # The order of the columns in the output table
    _MS_COLUMNS = ["avg", "wst", "bst", "miss", "meetR"]
    _DIS_COLUMNS = ["p50", "p90", "p95", "p99"]
    # The keys in the JSON object
    _CFG = "cfg"
    _PAIR = "pair"
    _ALL = "ALL"
    _OTHER_MS = "other_ms"
    _FIFO_MS = "fifo_ms"
    _OTHERDIS = "otherdis"
    _FIFODIS = "fifodis"
    _INHERITANCE = "inheritance"

    def setUpClass(self):
        required_params = ["hidl_hal_mode"]
        self.getUserParams(required_params)
        self.dut = self.registerController(android_device)[0]
        self.dut.shell.InvokeTerminal("one")
        self._cpu_freq = cpu_frequency_scaling.CpuFrequencyScalingController(self.dut)
        self._cpu_freq.DisableCpuScaling()

    def setUp(self):
        self._cpu_freq.SkipIfThermalThrottling(retry_delay_secs=30)

    def tearDown(self):
        self._cpu_freq.SkipIfThermalThrottling()

    def tearDownClass(self):
        self._cpu_freq.EnableCpuScaling()

    def testRunBenchmark32Bit(self):
        result = self._runBenchmark(32)
        self._addBenchmarkTableToResult(result, 32)
        self._uploadResult(result, 32)

    def testRunBenchmark64Bit(self):
        result = self._runBenchmark(64)
        self._addBenchmarkTableToResult(result, 64)
        self._uploadResult(result, 64)

    def _runBenchmark(self, bits):
        """Runs the native binary and parses its result.

        Args:
            bits: integer (32 or 64), the bitness of the binary to run.

        Returns:
            dict, the benchmarking result converted from native binary's JSON
            output.
        """
        logging.info("Start %d-bit hwbinder latency test with HIDL mode=%s",
                     bits, self.hidl_hal_mode)
        binary = "/data/local/tmp/%s/libhwbinder_latency%s" % (bits, bits)
        min_cpu, max_cpu = self._cpu_freq.GetMinAndMaxCpuNo()
        iterations = 1000 // (max_cpu - min_cpu)
        results = self.dut.shell.one.Execute([
            "chmod 755 %s" % binary,
            "VTS_ROOT_PATH=/data/local/tmp " \
            "LD_LIBRARY_PATH=/system/lib%s:/data/local/tmp/%s/hw:"
            "/data/local/tmp/%s:$LD_LIBRARY_PATH "
            "%s -raw_data -pair %d -i %d -m %s" % (bits, bits, bits,
                binary, max_cpu - min_cpu, iterations,
                self.hidl_hal_mode.encode("utf-8"))])
        # Parses the result.
        asserts.assertEqual(len(results[const.STDOUT]), 2)
        logging.info("stderr: %s", results[const.STDERR][1])
        logging.info("stdout: %s", results[const.STDOUT][1])
        asserts.assertFalse(
            any(results[const.EXIT_CODE]),
            "testRunBenchmark%sBit failed." % (bits))
        json_result = json.loads(results[const.STDOUT][1]);
        asserts.assertTrue(json_result[self._INHERITANCE] == "PASS",
            "Scheduler does not support priority inheritance.");
        return json_result

    def _createRow(self, pair_name, ms_result, dis_result):
        """Creates a row from the JSON output.

        Args:
            pair_name: string, the pair name in the first column.
            ms_result: dict, the fifo_ms or other_ms object.
            dis_result: dict, the fifodis or otherdis object.

        Returns:
            the list containing pair_name and the values in the objects.
        """
        row = [pair_name]
        row.extend([ms_result[x] for x in self._MS_COLUMNS])
        row.extend([dis_result[x] for x in self._DIS_COLUMNS])
        return row

    def _addBenchmarkTableToResult(self, result, bits):
        pair_cnt = result[self._CFG][self._PAIR]
        row_names = ["P" + str(i) for i in range(pair_cnt)] + [self._ALL]
        col_names = ["pair"] + self._MS_COLUMNS + self._DIS_COLUMNS
        fifo_table = [col_names]
        other_table = [col_names]
        for row_name in row_names:
            pair_result = result[row_name]
            fifo_table.append(self._createRow(row_name,
                pair_result[self._FIFO_MS], pair_result[self._FIFODIS]))
            other_table.append(self._createRow(row_name,
                pair_result[self._OTHER_MS], pair_result[self._OTHERDIS]))
        self.addTableToResult(
            "hwbinder_latency_%sbits_fifo" % bits,fifo_table)
        self.addTableToResult(
            "hwbinder_latency_%sbits_other" % bits, other_table)

    def _uploadResult(self, result, bits):
        """Uploads the output of benchmark program to web DB.

        Args:
            result: dict which is the benchmarking result.
            bits: integer (32 or 64).
        """
        opts = ["hidl_hal_mode=%s" % self.hidl_hal_mode.encode("utf-8")];
        min_cpu, max_cpu = self._cpu_freq.GetMinAndMaxCpuNo()
        for i in range(max_cpu - min_cpu):
            self.web.AddProfilingDataUnlabeledVector(
                "hwbinder_latency_%sbits" % bits,
                result["fifo_%d_data" % i], options=opts,
                x_axis_label="hwbinder latency",
                y_axis_label="Frequency")


if __name__ == "__main__":
    test_runner.main()
