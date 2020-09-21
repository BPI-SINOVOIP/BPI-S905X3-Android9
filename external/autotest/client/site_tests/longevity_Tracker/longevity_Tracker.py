# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# pylint: disable=module-missing-docstring
# pylint: disable=docstring-section-name

import csv
import glob
import httplib
import json
import logging
import os
import re
import shutil
import time
import urllib
import urllib2

from autotest_lib.client.bin import test
from autotest_lib.client.bin import utils
from autotest_lib.client.common_lib import error
from autotest_lib.client.cros import constants

# TODO(scunningham): Return to 72000 (20 hrs) after server-side stabilizes.
TEST_DURATION = 10800  # Duration of test (3 hrs) in seconds.
SAMPLE_INTERVAL = 60  # Length of measurement samples in seconds.
METRIC_INTERVAL = 3600  # Length between metric calculation in seconds.
STABILIZATION_DURATION = 60  # Time for test stabilization in seconds.
TMP_DIRECTORY = '/tmp/'
EXIT_FLAG_FILE = TMP_DIRECTORY + 'longevity_terminate'
PERF_FILE_NAME_PREFIX = 'perf'
OLD_FILE_AGE = 14400  # Age of old files to be deleted in minutes = 10 days.
# The manifest.json file for a Chrome Extension contains the app name, id,
# version, and other app info. It is accessible by the OS only when the app
# is running, and thus it's cryptohome directory mounted. Only one Kiosk app
# can be running at a time.
MANIFEST_PATTERN = '/home/.shadow/*/mount/user/Extensions/%s/*/manifest.json'
VERSION_PATTERN = r'^(\d+)\.(\d+)\.(\d+)\.(\d+)$'
DASHBOARD_UPLOAD_URL = 'https://chromeperf.appspot.com/add_point'


class PerfUploadingError(Exception):
    """Exception raised in perf_uploader."""
    pass


class longevity_Tracker(test.test):
    """Monitor device and App stability over long periods of time."""

    version = 1

    def initialize(self):
        self.temp_dir = os.path.split(self.tmpdir)[0]

    def _get_cpu_usage(self):
        """Compute percent CPU in active use over the sample interval.

        Note: This method introduces a sleep period into the test, equal to
        90% of the sample interval.

        @returns float of percent active use of CPU.
        """
        # Time between measurements is ~90% of the sample interval.
        measurement_time_delta = SAMPLE_INTERVAL * 0.90
        cpu_usage_start = utils.get_cpu_usage()
        time.sleep(measurement_time_delta)
        cpu_usage_end = utils.get_cpu_usage()
        return utils.compute_active_cpu_time(cpu_usage_start,
                                                  cpu_usage_end) * 100

    def _get_mem_usage(self):
        """Compute percent memory in active use.

        @returns float of percent memory in use.
        """
        total_memory = utils.get_mem_total()
        free_memory = utils.get_mem_free()
        return ((total_memory - free_memory) / total_memory) * 100

    def _get_max_temperature(self):
        """Get temperature of hottest sensor in Celsius.

        @returns float of temperature of hottest sensor.
        """
        temperature = utils.get_current_temperature_max()
        if not temperature:
            temperature = 0
        return temperature

    def _get_hwid(self):
        """Get hwid of test device, e.g., 'WOLF C4A-B2B-A47'.

        @returns string of hwid (Hardware ID) of device under test.
        """
        with os.popen('crossystem hwid 2>/dev/null', 'r') as hwid_proc:
            hwid = hwid_proc.read()
        if not hwid:
            hwid = 'undefined'
        return hwid

    def elapsed_time(self, mark_time):
        """Get time elapsed since |mark_time|.

        @param mark_time: point in time from which elapsed time is measured.
        @returns time elapsed since the marked time.
        """
        return time.time() - mark_time

    def modulo_time(self, timer, interval):
        """Get time eplased on |timer| for the |interval| modulus.

        Value returned is used to adjust the timer so that it is synchronized
        with the current interval.

        @param timer: time on timer, in seconds.
        @param interval: period of time in seconds.
        @returns time elapsed from the start of the current interval.
        """
        return timer % int(interval)

    def syncup_time(self, timer, interval):
        """Get time remaining on |timer| for the |interval| modulus.

        Value returned is used to induce sleep just long enough to put the
        process back in sync with the timer.

        @param timer: time on timer, in seconds.
        @param interval: period of time in seconds.
        @returns time remaining till the end of the current interval.
        """
        return interval - (timer % int(interval))

    def _record_perf_measurements(self, perf_values, perf_writer):
        """Record attribute performance measurements, and write to file.

        @param perf_values: dict of attribute performance values.
        @param perf_writer: file to write performance measurements.
        """
        # Get performance measurements.
        cpu_usage = '%.3f' % self._get_cpu_usage()
        mem_usage = '%.3f' % self._get_mem_usage()
        max_temp = '%.3f' % self._get_max_temperature()

        # Append measurements to attribute lists in perf values dictionary.
        perf_values['cpu'].append(cpu_usage)
        perf_values['mem'].append(mem_usage)
        perf_values['temp'].append(max_temp)

        # Write performance measurements to perf timestamped file.
        time_stamp = time.strftime('%Y/%m/%d %H:%M:%S')
        perf_writer.writerow([time_stamp, cpu_usage, mem_usage, max_temp])
        logging.info('Time: %s, CPU: %s, Mem: %s, Temp: %s',
                     time_stamp, cpu_usage, mem_usage, max_temp)

    def _record_90th_metrics(self, perf_values, perf_metrics):
        """Record 90th percentile metric of attribute performance values.

        @param perf_values: dict attribute performance values.
        @param perf_metrics: dict attribute 90%-ile performance metrics.
        """
        # Calculate 90th percentile for each attribute.
        cpu_values = perf_values['cpu']
        mem_values = perf_values['mem']
        temp_values = perf_values['temp']
        cpu_metric = sorted(cpu_values)[(len(cpu_values) * 9) // 10]
        mem_metric = sorted(mem_values)[(len(mem_values) * 9) // 10]
        temp_metric = sorted(temp_values)[(len(temp_values) * 9) // 10]
        logging.info('== Performance values: %s', perf_values)
        logging.info('== 90th percentile: cpu: %s, mem: %s, temp: %s',
                     cpu_metric, mem_metric, temp_metric)

        # Append 90th percentile to each attribute performance metric.
        perf_metrics['cpu'].append(cpu_metric)
        perf_metrics['mem'].append(mem_metric)
        perf_metrics['temp'].append(temp_metric)

    def _get_median_metrics(self, metrics):
        """Returns median of each attribute performance metric.

        If no metric values were recorded, return 0 for each metric.

        @param metrics: dict of attribute performance metric lists.
        @returns dict of attribute performance metric medians.
        """
        if len(metrics['cpu']):
            cpu_metric = sorted(metrics['cpu'])[len(metrics['cpu']) // 2]
            mem_metric = sorted(metrics['mem'])[len(metrics['mem']) // 2]
            temp_metric = sorted(metrics['temp'])[len(metrics['temp']) // 2]
        else:
            cpu_metric = 0
            mem_metric = 0
            temp_metric = 0
        logging.info('== Median: cpu: %s, mem: %s, temp: %s',
                     cpu_metric, mem_metric, temp_metric)
        return {'cpu': cpu_metric, 'mem': mem_metric, 'temp': temp_metric}

    def _append_to_aggregated_file(self, ts_file, ag_file):
        """Append contents of perf timestamp file to perf aggregated file.

        @param ts_file: file handle for performance timestamped file.
        @param ag_file: file handle for performance aggregated file.
        """
        next(ts_file)  # Skip fist line (the header) of timestamped file.
        for line in ts_file:
            ag_file.write(line)

    def _copy_aggregated_to_resultsdir(self, aggregated_fpath):
        """Copy perf aggregated file to results dir for AutoTest results.

        Note: The AutoTest results default directory is located at /usr/local/
        autotest/results/default/longevity_Tracker/results

        @param aggregated_fpath: file path to Aggregated performance values.
        """
        results_fpath = os.path.join(self.resultsdir, 'perf.csv')
        shutil.copy(aggregated_fpath, results_fpath)
        logging.info('Copied %s to %s)', aggregated_fpath, results_fpath)

    def _write_perf_keyvals(self, perf_results):
        """Write perf results to keyval file for AutoTest results.

        @param perf_results: dict of attribute performance metrics.
        """
        perf_keyval = {}
        perf_keyval['cpu_usage'] = perf_results['cpu']
        perf_keyval['memory_usage'] = perf_results['mem']
        perf_keyval['temperature'] = perf_results['temp']
        self.write_perf_keyval(perf_keyval)

    def _write_perf_results(self, perf_results):
        """Write perf results to results-chart.json file for Perf Dashboard.

        @param perf_results: dict of attribute performance metrics.
        """
        cpu_metric = perf_results['cpu']
        mem_metric = perf_results['mem']
        ec_metric = perf_results['temp']
        self.output_perf_value(description='cpu_usage', value=cpu_metric,
                               units='%', higher_is_better=False)
        self.output_perf_value(description='mem_usage', value=mem_metric,
                               units='%', higher_is_better=False)
        self.output_perf_value(description='max_temp', value=ec_metric,
                               units='Celsius', higher_is_better=False)

    def _read_perf_results(self):
        """Read perf results from results-chart.json file for Perf Dashboard.

        @returns dict of perf results, formatted as JSON chart data.
        """
        results_file = os.path.join(self.resultsdir, 'results-chart.json')
        with open(results_file, 'r') as fp:
            contents = fp.read()
            chart_data = json.loads(contents)
        return chart_data

    def _get_point_id(self, cros_version, epoch_minutes):
        """Compute point ID from ChromeOS version number and epoch minutes.

        @param cros_version: String of ChromeOS version number.
        @param epoch_minutes: String of minutes since 1970.

        @return unique integer ID computed from given version and epoch.
        """
        # Number of digits from each part of the Chrome OS version string.
        cros_version_col_widths = [0, 4, 3, 2]

        def get_digits(version_num, column_widths):
            if re.match(VERSION_PATTERN, version_num):
                computed_string = ''
                version_parts = version_num.split('.')
                for i, version_part in enumerate(version_parts):
                    if column_widths[i]:
                        computed_string += version_part.zfill(column_widths[i])
                return computed_string
            else:
                return None

        cros_digits = get_digits(cros_version, cros_version_col_widths)
        epoch_digits = epoch_minutes[-8:]
        if not cros_digits:
            return None
        return int(epoch_digits + cros_digits)

    def _get_kiosk_app_info(self, app_id):
        """Get kiosk app name and version from manifest.json file.

        Get the Kiosk App name and version strings from the manifest file of
        the specified |app_id| Extension in the currently running session. If
        |app_id| is empty or None, then return 'none' for the kiosk app info.

        Raise an error if no manifest is found (ie, |app_id| is not running),
        or if multiple manifest files are found (ie, |app_id| is running, but
        the |app_id| dir contains multiple versions or manifest files).

        @param app_id: string kiosk application identification.
        @returns dict of Kiosk name and version number strings.
        @raises: An error.TestError if single manifest is not found.
        """
        kiosk_app_info = {'name': 'none', 'version': 'none'}
        if not app_id:
            return kiosk_app_info

        # Get path to manifest file of the running Kiosk app_id.
        app_manifest_pattern = (MANIFEST_PATTERN % app_id)
        logging.info('app_manifest_pattern: %s', app_manifest_pattern)
        file_paths = glob.glob(app_manifest_pattern)
        # Raise error if current session has no Kiosk Apps running.
        if len(file_paths) == 0:
            raise error.TestError('Kiosk App ID=%s is not running.' % app_id)
        # Raise error if running Kiosk App has multiple manifest files.
        if len(file_paths) > 1:
            raise error.TestError('Kiosk App ID=%s has multiple manifest '
                                  'files.' % app_id)
        kiosk_manifest = open(file_paths[0], 'r').read()
        manifest_json = json.loads(kiosk_manifest)
        # If manifest is missing name or version key, set to 'undefined'.
        kiosk_app_info['name'] = manifest_json.get('name', 'undefined')
        kiosk_app_info['version'] = manifest_json.get('version', 'undefined')
        return kiosk_app_info

    def _format_data_for_upload(self, chart_data):
        """Collect chart data into an uploadable data JSON object.

        @param chart_data: performance results formatted as chart data.
        """
        perf_values = {
            'format_version': '1.0',
            'benchmark_name': self.test_suite_name,
            'charts': chart_data,
        }

        dash_entry = {
            'master': 'ChromeOS_Enterprise',
            'bot': 'cros-%s' % self.board_name,
            'point_id': self.point_id,
            'versions': {
                'cros_version': self.chromeos_version,
                'chrome_version': self.chrome_version,
            },
            'supplemental': {
                'default_rev': 'r_cros_version',
                'hardware_identifier': 'a_' + self.hw_id,
                'kiosk_app_name': 'a_' + self.kiosk_app_name,
                'kiosk_app_version': 'r_' + self.kiosk_app_version
            },
            'chart_data': perf_values
        }
        return {'data': json.dumps(dash_entry)}

    def _send_to_dashboard(self, data_obj):
        """Send formatted perf data to the perf dashboard.

        @param data_obj: data object as returned by _format_data_for_upload().

        @raises PerfUploadingError if an exception was raised when uploading.
        """
        logging.debug('data_obj: %s', data_obj)
        encoded = urllib.urlencode(data_obj)
        req = urllib2.Request(DASHBOARD_UPLOAD_URL, encoded)
        try:
            urllib2.urlopen(req)
        except urllib2.HTTPError as e:
            raise PerfUploadingError('HTTPError: %d %s for JSON %s\n' %
                                     (e.code, e.msg, data_obj['data']))
        except urllib2.URLError as e:
            raise PerfUploadingError('URLError: %s for JSON %s\n' %
                                     (str(e.reason), data_obj['data']))
        except httplib.HTTPException:
            raise PerfUploadingError('HTTPException for JSON %s\n' %
                                     data_obj['data'])

    def _get_chrome_version(self):
        """Get the Chrome version number and milestone as strings.

        Invoke "chrome --version" to get the version number and milestone.

        @return A tuple (chrome_ver, milestone) where "chrome_ver" is the
            current Chrome version number as a string (in the form "W.X.Y.Z")
            and "milestone" is the first component of the version number
            (the "W" from "W.X.Y.Z").  If the version number cannot be parsed
            in the "W.X.Y.Z" format, the "chrome_ver" will be the full output
            of "chrome --version" and the milestone will be the empty string.
        """
        chrome_version = utils.system_output(constants.CHROME_VERSION_COMMAND,
                                             ignore_status=True)
        chrome_version = utils.parse_chrome_version(chrome_version)
        return chrome_version

    def _open_perf_file(self, file_path):
        """Open a perf file. Write header line if new. Return file object.

        If the file on |file_path| already exists, then open file for
        appending only. Otherwise open for writing only.

        @param file_path: file path for perf file.
        @returns file object for the perf file.
        """
        # If file exists, open it for appending. Do not write header.
        if os.path.isfile(file_path):
            perf_file = open(file_path, 'a+')
        # Otherwise, create it for writing. Write header on first line.
        else:
            perf_file = open(file_path, 'w')  # Erase if existing file.
            perf_file.write('Time,CPU,Memory,Temperature (C)\r\n')
        return perf_file

    def _run_test_cycle(self):
        """Track performance of Chrome OS over a long period of time.

        This method collects performance measurements, and calculates metrics
        to upload to the performance dashboard. It creates two files to
        collect and store performance values and results: perf_<timestamp>.csv
        and perf_aggregated.csv.

        At the start, it creates a unique perf timestamped file in the test's
        temp_dir. As the cycle runs, it saves a time-stamped performance
        value after each sample interval. Periodically, it calculates
        the 90th percentile performance metrics from these values.

        The perf_<timestamp> files on the device will survive multiple runs
        of the longevity_Tracker by the server-side test, and will also
        survive multiple runs of the server-side test. The script will
        delete them after 10 days, to prevent filling up the SSD.

        At the end, it opens the perf aggregated file in the test's temp_dir,
        and appends the contents of the perf timestamped file. It then
        copies the perf aggregated file to the results directory as perf.csv.
        This perf.csv file will be consumed by the AutoTest backend when the
        server-side test ends.

        Note that the perf_aggregated.csv file will grow larger with each run
        of longevity_Tracker on the device by the server-side test. However,
        the server-side test will delete file in the end.

        This method also calculates 90th percentile and median metrics, and
        returns the median metrics. Median metrics will be pushed to the perf
        dashboard with a unique point_id.

        @returns list of median performance metrics.
        """
        # Allow system to stabilize before start taking measurements.
        test_start_time = time.time()
        time.sleep(STABILIZATION_DURATION)

        perf_values = {'cpu': [], 'mem': [], 'temp': []}
        perf_metrics = {'cpu': [], 'mem': [], 'temp': []}

        # Create perf_<timestamp> file and writer.
        timestamp_fname = (PERF_FILE_NAME_PREFIX +
                           time.strftime('_%Y-%m-%d_%H-%M') + '.csv')
        timestamp_fpath = os.path.join(self.temp_dir, timestamp_fname)
        timestamp_file = self._open_perf_file(timestamp_fpath)
        timestamp_writer = csv.writer(timestamp_file)

        # Align time of loop start with the sample interval.
        test_elapsed_time = self.elapsed_time(test_start_time)
        time.sleep(self.syncup_time(test_elapsed_time, SAMPLE_INTERVAL))
        test_elapsed_time = self.elapsed_time(test_start_time)

        metric_start_time = time.time()
        metric_prev_time = metric_start_time

        metric_elapsed_prev_time = self.elapsed_time(metric_prev_time)
        offset = self.modulo_time(metric_elapsed_prev_time, METRIC_INTERVAL)
        metric_timer = metric_elapsed_prev_time + offset
        while self.elapsed_time(test_start_time) <= TEST_DURATION:
            if os.path.isfile(EXIT_FLAG_FILE):
                logging.info('Exit flag file detected. Exiting test.')
                break
            self._record_perf_measurements(perf_values, timestamp_writer)

            # Periodically calculate and record 90th percentile metrics.
            metric_elapsed_prev_time = self.elapsed_time(metric_prev_time)
            metric_timer = metric_elapsed_prev_time + offset
            if metric_timer >= METRIC_INTERVAL:
                self._record_90th_metrics(perf_values, perf_metrics)
                perf_values = {'cpu': [], 'mem': [], 'temp': []}

                # Set previous time to current time.
                metric_prev_time = time.time()
                metric_elapsed_prev_time = self.elapsed_time(metric_prev_time)

                # Calculate offset based on the original start time.
                metric_elapsed_time = self.elapsed_time(metric_start_time)
                offset = self.modulo_time(metric_elapsed_time, METRIC_INTERVAL)

                # Set the timer to time elapsed plus offset to next interval.
                metric_timer = metric_elapsed_prev_time + offset

            # Sync the loop time to the sample interval.
            test_elapsed_time = self.elapsed_time(test_start_time)
            time.sleep(self.syncup_time(test_elapsed_time, SAMPLE_INTERVAL))

        # Close perf timestamp file.
        timestamp_file.close()

        # Open perf timestamp file to read, and aggregated file to append.
        timestamp_file = open(timestamp_fpath, 'r')
        aggregated_fname = (PERF_FILE_NAME_PREFIX + '_aggregated.csv')
        aggregated_fpath = os.path.join(self.temp_dir, aggregated_fname)
        aggregated_file = self._open_perf_file(aggregated_fpath)

        # Append contents of perf timestamp file to perf aggregated file.
        self._append_to_aggregated_file(timestamp_file, aggregated_file)
        timestamp_file.close()
        aggregated_file.close()

        # Copy perf aggregated file to test results directory.
        self._copy_aggregated_to_resultsdir(aggregated_fpath)

        # Return median of each attribute performance metric.
        return self._get_median_metrics(perf_metrics)

    def run_once(self, kiosk_app_attributes=None):
        if kiosk_app_attributes:
            app_name, app_id, ext_page = (
                kiosk_app_attributes.rstrip().split(':'))
        self.subtest_name = app_name
        self.board_name = utils.get_board()
        self.hw_id = self._get_hwid()
        self.chrome_version = self._get_chrome_version()[0]
        self.chromeos_version = '0.' + utils.get_chromeos_release_version()
        self.epoch_minutes = str(int(time.time() / 60))  # Minutes since 1970.
        self.point_id = self._get_point_id(self.chromeos_version,
                                           self.epoch_minutes)

        kiosk_info = self._get_kiosk_app_info(app_id)
        self.kiosk_app_name = kiosk_info['name']
        self.kiosk_app_version = kiosk_info['version']
        self.test_suite_name = self.tagged_testname
        if self.subtest_name:
            self.test_suite_name += '.' + self.subtest_name

        # Delete exit flag file at start of test run.
        if os.path.isfile(EXIT_FLAG_FILE):
            os.remove(EXIT_FLAG_FILE)

        # Run a single test cycle.
        self.perf_results = {'cpu': '0', 'mem': '0', 'temp': '0'}
        self.perf_results = self._run_test_cycle()

        # Write results for AutoTest to pick up at end of test.
        self._write_perf_keyvals(self.perf_results)
        self._write_perf_results(self.perf_results)

        # Post perf results directly to performance dashboard. You may view
        # uploaded data at https://chromeperf.appspot.com/new_points,
        # with test path pattern=ChromeOS_Enterprise/cros-*/longevity*/*
        chart_data = self._read_perf_results()
        data_obj = self._format_data_for_upload(chart_data)
        self._send_to_dashboard(data_obj)

    def cleanup(self):
        """Delete aged perf data files and the exit flag file."""
        cmd = ('find %s -name %s* -type f -mmin +%s -delete' %
               (self.temp_dir, PERF_FILE_NAME_PREFIX, OLD_FILE_AGE))
        os.system(cmd)
        if os.path.isfile(EXIT_FLAG_FILE):
            os.remove(EXIT_FLAG_FILE)
