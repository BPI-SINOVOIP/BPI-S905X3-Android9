# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import errno
import hashlib
import logging
import math
import mmap
import os
import re

from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib import file_utils
from autotest_lib.client.cros import chrome_binary_test
from autotest_lib.client.cros.video import helper_logger


DOWNLOAD_BASE = ('http://commondatastorage.googleapis.com'
                 '/chromiumos-test-assets-public/')

VEA_BINARY = 'video_encode_accelerator_unittest'
TIME_BINARY = '/usr/local/bin/time'

# The format used for 'time': <real time> <kernel time> <user time>
TIME_OUTPUT_FORMAT = '%e %S %U'

FRAME_STATS_SUFFIX = 'frame-data.csv'
TEST_LOG_SUFFIX = 'test.log'
TIME_LOG_SUFFIX = 'time.log'

# Performance keys:
# FPS (i.e. encoder throughput)
KEY_FPS = 'fps'
# Encode latencies at the 50th, 75th, and 95th percentiles.
# Encode latency is the delay from input of a frame to output of the encoded
# bitstream.
KEY_ENCODE_LATENCY_50 = 'encode_latency.50_percentile'
KEY_ENCODE_LATENCY_75 = 'encode_latency.75_percentile'
KEY_ENCODE_LATENCY_95 = 'encode_latency.95_percentile'
# CPU usage in kernel space
KEY_CPU_KERNEL_USAGE = 'cpu_usage.kernel'
# CPU usage in user space
KEY_CPU_USER_USAGE = 'cpu_usage.user'

# Units of performance values:
UNIT_MILLISECOND = 'milliseconds'
UNIT_MICROSECOND = 'us'
UNIT_RATIO = 'ratio'
UNIT_FPS = 'fps'

RE_FPS = re.compile(r'^Measured encoder FPS: ([+\-]?[0-9.]+)$', re.MULTILINE)
RE_ENCODE_LATENCY_50 = re.compile(
    r'^Encode latency for the 50th percentile: (\d+) us$',
    re.MULTILINE)
RE_ENCODE_LATENCY_75 = re.compile(
    r'^Encode latency for the 75th percentile: (\d+) us$',
    re.MULTILINE)
RE_ENCODE_LATENCY_95 = re.compile(
    r'^Encode latency for the 95th percentile: (\d+) us$',
    re.MULTILINE)


def _remove_if_exists(filepath):
    try:
        os.remove(filepath)
    except OSError, e:
        if e.errno != errno.ENOENT:  # no such file
            raise


class video_VEAPerf(chrome_binary_test.ChromeBinaryTest):
    """
    This test monitors several performance metrics reported by Chrome test
    binary, video_encode_accelerator_unittest.
    """

    version = 1

    def _logperf(self, test_name, key, value, units, higher_is_better=False):
        description = '%s.%s' % (test_name, key)
        self.output_perf_value(
                description=description, value=value, units=units,
                higher_is_better=higher_is_better)


    def _analyze_fps(self, test_name, log_file):
        """
        Analyzes FPS info from result log file.
        """
        with open(log_file, 'r') as f:
            mm = mmap.mmap(f.fileno(), 0, access=mmap.ACCESS_READ)
            fps = [float(m.group(1)) for m in RE_FPS.finditer(mm)]
            mm.close()
        if len(fps) != 1:
            raise error.TestError('Parsing FPS failed w/ %d occurrence(s).' %
                                  len(fps))
        self._logperf(test_name, KEY_FPS, fps[0], UNIT_FPS, True)


    def _analyze_encode_latency(self, test_name, log_file):
        """
        Analyzes encode latency from result log file.
        """
        with open(log_file, 'r') as f:
            mm = mmap.mmap(f.fileno(), 0, access=mmap.ACCESS_READ)
            latency_50 = [int(m.group(1)) for m in
                          RE_ENCODE_LATENCY_50.finditer(mm)]
            latency_75 = [int(m.group(1)) for m in
                          RE_ENCODE_LATENCY_75.finditer(mm)]
            latency_95 = [int(m.group(1)) for m in
                          RE_ENCODE_LATENCY_95.finditer(mm)]
            mm.close()
        if any([len(l) != 1 for l in [latency_50, latency_75, latency_95]]):
            raise error.TestError('Parsing encode latency failed.')
        self._logperf(test_name, KEY_ENCODE_LATENCY_50, latency_50[0],
                      UNIT_MICROSECOND)
        self._logperf(test_name, KEY_ENCODE_LATENCY_75, latency_75[0],
                      UNIT_MICROSECOND)
        self._logperf(test_name, KEY_ENCODE_LATENCY_95, latency_95[0],
                      UNIT_MICROSECOND)


    def _analyze_cpu_usage(self, test_name, time_log_file):
        """
        Analyzes CPU usage from the output of 'time' command.
        """
        with open(time_log_file) as f:
            content = f.read()
        r, s, u = (float(x) for x in content.split())
        self._logperf(test_name, KEY_CPU_USER_USAGE, u / r, UNIT_RATIO)
        self._logperf(test_name, KEY_CPU_KERNEL_USAGE, s / r, UNIT_RATIO)

    def _analyze_frame_stats(self, test_name, frame_stats_file):
        """
        Analyzes quality from --frame_stats output CSV. Assumes YUV420 (for MSE
        samples per channel).
        """
        def mse_to_psnr(samples, peak, mse):
            """
            Generate PSNR from MSE for a frame.
            """
            MAX_PSNR = 100.0
            # Prevent a divide-by-zero, MSE at 0 is perfect quality (no error).
            if mse == 0:
                return MAX_PSNR
            psnr = 10.0 * math.log10(peak * peak * samples / float(mse))
            return min(psnr, MAX_PSNR)

        frame_ssim = {'y': [], 'u': [], 'v': [], 'combined': []}
        frame_psnr = {'y': [], 'u': [], 'v': [], 'combined': []}
        for line in open(frame_stats_file):
            (frame, width, height,
                ssim_y, ssim_u, ssim_v, mse_y, mse_u, mse_v) = line.split(',')
            # Skip CSV header.
            if frame == 'frame':
                continue
            frame = int(frame)
            width = int(width)
            height = int(height)
            ssim_y = float(ssim_y)
            ssim_u = float(ssim_u)
            ssim_v = float(ssim_v)
            mse_y = int(mse_y)
            mse_u = int(mse_u)
            mse_v = int(mse_v)

            frame_ssim['y'].append(ssim_y)
            frame_ssim['u'].append(ssim_u)
            frame_ssim['v'].append(ssim_v)
            # Weighting of YUV channels for SSIM taken from libvpx.
            frame_ssim['combined'].append(
                0.8 * ssim_y + 0.1 * (ssim_u + ssim_v))

            # Samples per MSE score assumes YUV420 subsampling.
            frame_psnr['y'].append(
                mse_to_psnr(width * height * 4 / 4, 255, mse_y))
            frame_psnr['u'].append(
                mse_to_psnr(width * height * 1 / 4, 255, mse_u))
            frame_psnr['v'].append(
                mse_to_psnr(width * height * 1 / 4, 255, mse_v))
            frame_psnr['combined'].append(
                mse_to_psnr(
                    width * height * 6 / 4, 255, mse_y + mse_u + mse_v))

        for channel in ['y', 'u', 'v', 'combined']:
            # Log stats with a key similar to 'quality.ssim.y.max'. For combined
            # stats the channel is omitted ('quality.ssim.max').
            key = 'quality.%s'
            if channel is not 'combined':
                key += '.' + channel
            key += '.%s'
            for (stat, func) in [('min', min), ('max', max),
                                 ('avg', lambda x: sum(x) / len(x))]:
                self._logperf(test_name, key % ('ssim', stat),
                              func(frame_ssim[channel]), None,
                              higher_is_better=True)
                self._logperf(test_name, key % ('psnr', stat),
                              func(frame_psnr[channel]), None,
                              higher_is_better=True)

    def _get_profile_name(self, profile):
        """
        Gets profile name from a profile index.
        """
        if profile == 1:
            return 'h264'
        elif profile == 11:
            return 'vp8'
        else:
            raise error.TestError('Internal error.')


    def _convert_test_name(self, path, on_cloud, profile):
        """Converts source path to test name and output video file name.

        For example: for the path on cloud
            "tulip2/tulip2-1280x720-1b95123232922fe0067869c74e19cd09.yuv"

        We will derive the test case's name as "tulip2-1280x720.vp8" or
        "tulip2-1280x720.h264" depending on the profile. The MD5 checksum in
        path will be stripped.

        For the local file, we use the base name directly.

        @param path: The local path or download path.
        @param on_cloud: Whether the file is on cloud.
        @param profile: Profile index.

        @returns a pair of (test name, output video file name)
        """
        s = os.path.basename(path)
        name = s[:s.rfind('-' if on_cloud else '.')]
        profile_name = self._get_profile_name(profile)
        return (name + '_' + profile_name, name + '.' + profile_name)


    def _download_video(self, path_on_cloud, local_file):
        url = '%s%s' % (DOWNLOAD_BASE, path_on_cloud)
        logging.info('download "%s" to "%s"', url, local_file)

        file_utils.download_file(url, local_file)

        with open(local_file, 'r') as r:
            md5sum = hashlib.md5(r.read()).hexdigest()
            if md5sum not in path_on_cloud:
                raise error.TestError('unmatched md5 sum: %s' % md5sum)


    def _get_result_filename(self, test_name, subtype, suffix):
        return os.path.join(self.resultsdir,
                            '%s_%s_%s' % (test_name, subtype, suffix))


    def _run_test_case(self, test_name, test_stream_data):
        """
        Runs a VEA unit test.

        @param test_name: Name of this test case.
        @param test_stream_data: Parameter to --test_stream_data in vea_unittest.
        """
        # Get FPS.
        test_log_file = self._get_result_filename(test_name, 'fullspeed',
                                                  TEST_LOG_SUFFIX)
        vea_args = [
            '--gtest_filter=EncoderPerf/*/0',
            '--test_stream_data=%s' % test_stream_data,
            '--output_log="%s"' % test_log_file,
            helper_logger.chrome_vmodule_flag(),
            '--ozone-platform=gbm']
        self.run_chrome_test_binary(VEA_BINARY, ' '.join(vea_args))
        self._analyze_fps(test_name, test_log_file)

        # Get CPU usage and encode latency under specified frame rate.
        test_log_file = self._get_result_filename(test_name, 'fixedspeed',
                                                  TEST_LOG_SUFFIX)
        time_log_file = self._get_result_filename(test_name, 'fixedspeed',
                                                  TIME_LOG_SUFFIX)
        vea_args = [
            '--gtest_filter=SimpleEncode/*/0',
            '--test_stream_data=%s' % test_stream_data,
            '--run_at_fps', '--measure_latency',
            '--output_log="%s"' % test_log_file,
            helper_logger.chrome_vmodule_flag(),
            '--ozone-platform=gbm']
        time_cmd = ('%s -f "%s" -o "%s" ' %
                    (TIME_BINARY, TIME_OUTPUT_FORMAT, time_log_file))
        self.run_chrome_test_binary(VEA_BINARY, ' '.join(vea_args),
                                    prefix=time_cmd)
        self._analyze_encode_latency(test_name, test_log_file)
        self._analyze_cpu_usage(test_name, time_log_file)

        # TODO(pbos): Measure quality at more bitrates.
        # Generate SSIM/PSNR scores (objective quality metrics).
        test_log_file = self._get_result_filename(test_name, 'quality',
                                                  TEST_LOG_SUFFIX)
        frame_stats_file = self._get_result_filename(test_name, 'quality',
                                                    FRAME_STATS_SUFFIX)
        vea_args = [
            '--gtest_filter=SimpleEncode/*/0',
            '--test_stream_data=%s' % test_stream_data,
            '--frame_stats="%s"' % frame_stats_file,
            '--output_log="%s"' % test_log_file,
            helper_logger.chrome_vmodule_flag(),
            '--ozone-platform=gbm']
        self.run_chrome_test_binary(VEA_BINARY, ' '.join(vea_args))
        self._analyze_frame_stats(test_name, frame_stats_file)

    @helper_logger.video_log_wrapper
    @chrome_binary_test.nuke_chrome
    def run_once(self, test_cases):
        """
        Tests ChromeOS video hardware encoder performance.
        """
        last_error = None
        for (path, on_cloud, width, height, requested_bit_rate,
             profile, requested_frame_rate) in test_cases:
            try:
                test_name, output_name = self._convert_test_name(
                    path, on_cloud, profile)
                if on_cloud:
                    input_path = os.path.join(self.tmpdir,
                                              os.path.basename(path))
                    self._download_video(path, input_path)
                else:
                    input_path = os.path.join(self.cr_source_dir, path)
                output_path = os.path.join(self.tmpdir, output_name)
                test_stream_data = '%s:%s:%s:%s:%s:%s:%s' % (
                    input_path, width, height, profile, output_path,
                    requested_bit_rate, requested_frame_rate)
                self._run_test_case(test_name, test_stream_data)
            except Exception as last_error:
                # Log the error and continue to the next test case.
                logging.exception(last_error)
            finally:
                if on_cloud:
                    _remove_if_exists(input_path)
                _remove_if_exists(output_path)

        if last_error:
            raise last_error
