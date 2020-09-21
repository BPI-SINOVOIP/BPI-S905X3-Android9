# Copyright 2018 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import os
from autotest_lib.client.common_lib import file_utils
from autotest_lib.client.cros import chrome_binary_test
from autotest_lib.client.cros.video import helper_logger

DOWNLOAD_BASE = ('http://commondatastorage.googleapis.com/'
                 'chromiumos-test-assets-public/')
TEST_LOG = 'test_log'
REPEAT_TIMES = 100
TIME_UNIT = 'millisecond'
HW_ENCODE_LABEL = 'hw_encode_time'
SW_ENCODE_LABEL = 'sw_encode_time'
LATENCY_50_SUFFIX = '.encode_latency.50_percentile'
LATENCY_75_SUFFIX = '.encode_latency.75_percentile'
LATENCY_95_SUFFIX = '.encode_latency.95_percentile'
HW_PREFIX = 'hw_'
SW_PREFIX = 'sw_'

class video_JEAPerf(chrome_binary_test.ChromeBinaryTest):
    """
    This test monitors performance metrics reported by Chrome test binary,
    jpeg_encode_accelerator_unittest.
    """

    version = 1
    binary = 'jpeg_encode_accelerator_unittest'

    def report_perf_results(self, test_name, output_path):
        hw_times = []
        sw_times = []
        with open(output_path, 'r') as f:
            lines = f.readlines()
            lines = [line.strip() for line in lines]
            for line in lines:
                key_value = line.split(':')
                if len(key_value) != 2:
                    continue
                (key, value) = (key_value[0].strip(), key_value[1].strip())
                if key == HW_ENCODE_LABEL:
                    hw_times.append(int(value))
                if key == SW_ENCODE_LABEL:
                    sw_times.append(int(value))
        hw_times.sort()
        sw_times.sort()

        if len(hw_times) > 0:
            percentile_50 = len(hw_times) / 2
            percentile_75 = len(hw_times) * 3 / 4
            percentile_95 = len(hw_times) * 95 / 100
            test_title = HW_PREFIX + test_name
            self.output_perf_value(description=(test_title + LATENCY_50_SUFFIX),
                value=hw_times[percentile_50],
                units=TIME_UNIT, higher_is_better=False)
            self.output_perf_value(description=(test_title + LATENCY_75_SUFFIX),
                value=hw_times[percentile_75],
                units=TIME_UNIT, higher_is_better=False)
            self.output_perf_value(description=(test_title + LATENCY_95_SUFFIX),
                value=hw_times[percentile_95],
                units=TIME_UNIT, higher_is_better=False)

        if len(sw_times) > 0:
            percentile_50 = len(sw_times) / 2
            percentile_75 = len(sw_times) * 3 / 4
            percentile_95 = len(sw_times) * 95 / 100
            test_title = SW_PREFIX + test_name
            self.output_perf_value(description=(test_title + LATENCY_50_SUFFIX),
                value=sw_times[percentile_50],
                units=TIME_UNIT, higher_is_better=False)
            self.output_perf_value(description=(test_title + LATENCY_75_SUFFIX),
                value=sw_times[percentile_75],
                units=TIME_UNIT, higher_is_better=False)
            self.output_perf_value(description=(test_title + LATENCY_95_SUFFIX),
                value=sw_times[percentile_95],
                units=TIME_UNIT, higher_is_better=False)

    def remove_if_exists(self, file_path):
        try:
            os.remove(file_path)
        except OSError as e:
            if e.errno != errno.ENOENT:  # no such file
                raise

    @helper_logger.video_log_wrapper
    @chrome_binary_test.nuke_chrome
    def run_once(self, test_cases):
        """
        Runs JpegEncodeAcceleratorTest.SimpleEncode on the device and reports
        latency values for HW and SW.
        """

        for (image_path, width, height) in test_cases:
            url = DOWNLOAD_BASE + image_path
            file_name = os.path.basename(image_path)
            input_path = os.path.join(self.bindir, file_name)
            file_utils.download_file(url, input_path)
            test_name = ('%s_%dx%d' % (file_name, width, height))
            output_path = os.path.join(self.tmpdir, TEST_LOG)

            cmd_line_list = [helper_logger.chrome_vmodule_flag()] + [
                '--gtest_filter=JpegEncodeAcceleratorTest.SimpleEncode',
                ('--output_log=%s' % output_path),
                ('--repeat=%d' % REPEAT_TIMES),
                ('--yuv_filenames="%s:%dx%d"' % (input_path, width, height))
            ]
            cmd_line = ' '.join(cmd_line_list)
            try:
                self.run_chrome_test_binary(self.binary, cmd_line)
                self.report_perf_results(test_name, output_path)
            except Exception as last_error:
                # Log the error and continue to the next test case.
                logging.exception(last_error)
            finally:
                self.remove_if_exists(input_path)
                self.remove_if_exists(output_path)

