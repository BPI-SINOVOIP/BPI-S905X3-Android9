# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import re

from autotest_lib.client.bin import test, utils
from autotest_lib.client.common_lib import error

_RESULT_STRING = 'Average page access speed: (\d+)'

class hardware_MemoryZRAMThroughput(test.test):
    """
    This test uses a large buffer to measure the page access throughput with
    and without ZRAM.
    """
    version = 1

    def initialize(self):
        utils.system('stop ui', ignore_status=True)
        self._PATTERN = re.compile(_RESULT_STRING)

    def _log_results(self, key, out):
        logging.info("test output: %s", out)
        pages_per_second = 0
        data_points = 0
        for line in out.splitlines():
            result = self._PATTERN.match(line)
            if result:
                pages_per_second += float(result.group(1))
                data_points += 1
                continue

        if data_points == 0:
            raise error.TestError('Test results not found')

        # Take the average of all data points. Currently when --fork is
        # passed to memory-eater, there will be two data points from two
        # processes.
        average_pages_per_second = pages_per_second / data_points
        self.output_perf_value(description=key,
                               value=average_pages_per_second,
                               higher_is_better=True,
                               units='pages_per_sec')

        return average_pages_per_second

    def run_once(self):
        mem_size = utils.memtotal()
        swap_size = utils.swaptotal()
        logging.info("MemTotal: %.0f KB", mem_size)
        logging.info("SwapTotal: %.0f KB", swap_size)

        # First run memory-eater wth 60% of total memory size to measure the
        # page access throughput
        cmd = ("memory-eater --size %d --speed --repeat 4 --chunk 500 "
               "--wait 0" % long(mem_size * 0.60 / 1024))
        logging.debug('cmd: %s', cmd)
        out = utils.system_output(cmd)
        self._log_results("60_Percent_RAM", out)

        # Then run memory-eater wth total memory + 30% swap size to measure the
        # page access throughput. On 32-bit system with 4GB of RAM, the memory
        # footprint needed to generate enough memory pressure is larger than
        # a single user-space process can own. So we divide the memory usage
        # by half and the test itself will fork a child process to double the
        # memory usage. Each process will take turns to access 500 pages
        # (via --chunk) until all pages are accessed 4 times (via --repeat).
        half_mem_pressure_size = long((mem_size+swap_size * 0.3) / 1024) / 2;
        cmd = ("memory-eater --size %d --speed --fork --repeat 4 --chunk 500"
               "--wait 0" % half_mem_pressure_size)
        logging.debug('cmd: %s', cmd)
        out = utils.system_output(cmd)
        self._log_results("30_Percent_SWAP", out)

    def cleanup(self):
        utils.system('start ui')

