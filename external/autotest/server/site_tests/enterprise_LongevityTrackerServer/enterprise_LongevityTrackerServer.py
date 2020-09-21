# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import os
import csv
import json
import time
import urllib
import urllib2
import logging
import httplib

import enterprise_longevity_helper
from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib.cros import tpm_utils
from autotest_lib.server import autotest
from autotest_lib.server import test
from autotest_lib.server.cros.multimedia import remote_facade_factory


STABILIZATION_DURATION = 60
MEASUREMENT_DURATION_SECONDS = 10
TMP_DIRECTORY = '/tmp/'
PERF_FILE_NAME_PREFIX = 'perf'
VERSION_PATTERN = r'^(\d+)\.(\d+)\.(\d+)$'
DASHBOARD_UPLOAD_URL = 'https://chromeperf.appspot.com/add_point'
EXPECTED_PARAMS = ['perf_capture_iterations',  'perf_capture_duration',
                   'sample_interval', 'metric_interval', 'test_type',
                   'kiosk_app_attributes']


class PerfUploadingError(Exception):
    """Exception raised in perf_uploader."""
    pass


class enterprise_LongevityTrackerServer(test.test):
    """
    Run Longevity Test: Collect performance data over long duration.

    Run enterprise_KioskEnrollment and clear the TPM as necessary. After
    enterprise enrollment is successful, collect and log cpu, memory, and
    temperature data from the device under test.

    """
    version = 1


    def initialize(self):
        self.temp_dir = os.path.split(self.tmpdir)[0]


    #TODO(krishnargv@): Add a method to retrieve the version of the
    #                   Kiosk app from its manifest.
    def _initialize_test_variables(self):
        """Initialize test variables that will be uploaded to the dashboard."""
        self.board_name = self.system_facade.get_current_board()
        self.chromeos_version = self.system_facade.get_chromeos_release_version()
        epoch_minutes = str(int(time.time() / 60))
        self.point_id = enterprise_longevity_helper.get_point_id(
                self.chromeos_version, epoch_minutes, VERSION_PATTERN)
        self.test_suite_name = self.tagged_testname
        self.perf_capture_duration = self.perf_params['perf_capture_duration']
        self.sample_interval = self.perf_params['sample_interval']
        self.metric_interval = self.perf_params['metric_interval']
        self.perf_results = {'cpu': '0', 'mem': '0', 'temp': '0'}


    def elapsed_time(self, mark_time):
        """
        Get time elapsed since |mark_time|.

        @param mark_time: point in time from which elapsed time is measured.

        @returns time elapsed since the marked time.

        """
        return time.time() - mark_time


    #TODO(krishnargv):  Replace _format_data_for_upload with a call to the
    #                   _format_for_upload method of the perf_uploader.py
    def _format_data_for_upload(self, chart_data):
        """
        Collect chart data into an uploadable data JSON object.

        @param chart_data: performance results formatted as chart data.

        """
        perf_values = {
            'format_version': '1.0',
            'benchmark_name': self.test_suite_name,
            'charts': chart_data,
        }
        #TODO(krishnargv): Add a method to capture the chrome_version.
        dash_entry = {
            'master': 'ChromeOS_Enterprise',
            'bot': 'cros-%s' % self.board_name,
            'point_id': self.point_id,
            'versions': {
                'cros_version': self.chromeos_version,

            },
            'supplemental': {
                'default_rev': 'r_cros_version',
                'kiosk_app_name': 'a_' + self.kiosk_app_name,

            },
            'chart_data': perf_values
        }
        return {'data': json.dumps(dash_entry)}


    #TODO(krishnargv):  Replace _send_to_dashboard with a call to the
    #                   _send_to_dashboard method of the perf_uploader.py
    def _send_to_dashboard(self, data_obj):
        """
        Send formatted perf data to the perf dashboard.

        @param data_obj: data object as returned by _format_data_for_upload().

        @raises PerfUploadingError if an exception was raised when uploading.

        """
        logging.debug('Data_obj to be uploaded: %s', data_obj)
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


    def _write_perf_keyvals(self, perf_results):
        """
        Write perf results to keyval file for AutoTest results.

        @param perf_results: dict of attribute performance metrics.

        """
        perf_keyval = {}
        perf_keyval['cpu_usage'] = perf_results['cpu']
        perf_keyval['memory_usage'] = perf_results['mem']
        perf_keyval['temperature'] = perf_results['temp']
        self.write_perf_keyval(perf_keyval)


    def _write_perf_results(self, perf_results):
        """
        Write perf results to results-chart.json file for Perf Dashboard.

        @param perf_results: dict of attribute performance metrics.

        """
        cpu_metric = perf_results['cpu']
        mem_metric = perf_results['mem']
        ec_metric = perf_results['temp']
        self.output_perf_value(description='cpu_usage', value=cpu_metric,
                               units='percent', higher_is_better=False)
        self.output_perf_value(description='mem_usage', value=mem_metric,
                               units='percent', higher_is_better=False)
        self.output_perf_value(description='max_temp', value=ec_metric,
                               units='Celsius', higher_is_better=False)


    def _record_perf_measurements(self, perf_values, perf_writer):
        """
        Record attribute performance measurements, and write to file.

        @param perf_values: dict of attribute performance values.
        @param perf_writer: file to write performance measurements.

        """
        # Get performance measurements.
        cpu_usage = '%.3f' % enterprise_longevity_helper.get_cpu_usage(
                self.system_facade, MEASUREMENT_DURATION_SECONDS)
        mem_usage = '%.3f' % enterprise_longevity_helper.get_memory_usage(
                    self.system_facade)
        max_temp = '%.3f' % enterprise_longevity_helper.get_temperature_data(
                self.client, self.system_facade)

        # Append measurements to attribute lists in perf values dictionary.
        perf_values['cpu'].append(float(cpu_usage))
        perf_values['mem'].append(float(mem_usage))
        perf_values['temp'].append(float(max_temp))

        # Write performance measurements to perf timestamped file.
        time_stamp = time.strftime('%Y/%m/%d %H:%M:%S')
        perf_writer.writerow([time_stamp, cpu_usage, mem_usage, max_temp])
        logging.info('Time: %s, CPU: %r, Mem: %r, Temp: %r',
                     time_stamp, cpu_usage, mem_usage, max_temp)


    def _setup_kiosk_app_on_dut(self, kiosk_app_attributes=None):
        """Enroll the DUT and setup a Kiosk app."""
        info = self.client.host_info_store.get()
        app_config_id = info.get_label_value('app_config_id')
        if app_config_id and app_config_id.startswith(':'):
            app_config_id = app_config_id[1:]
        if kiosk_app_attributes:
            kiosk_app_attributes = kiosk_app_attributes.rstrip()
            self.kiosk_app_name, ext_id = kiosk_app_attributes.split(':')[:2]

        tpm_utils.ClearTPMOwnerRequest(self.client)
        logging.info("Enrolling the DUT to Kiosk mode")
        autotest.Autotest(self.client).run_test(
                'enterprise_KioskEnrollment',
                kiosk_app_attributes=kiosk_app_attributes,
                check_client_result=True)

        if self.kiosk_app_name == 'riseplayer':
            self.kiosk_facade.config_rise_player(ext_id, app_config_id)


    def _run_perf_capture_cycle(self):
        """
        Track performance of Chrome OS over a long period of time.

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
        survive multiple runs of the server-side test.

        At the end, it opens the perf aggregated file in the test's temp_dir,
        and appends the contents of the perf timestamped file. It then
        copies the perf aggregated file to the results directory as perf.csv.
        This perf.csv file will be consumed by the AutoTest backend when the
        server-side test ends.

        Note that the perf_aggregated.csv file will grow larger with each run
        of longevity_Tracker on the device by the server-side test.

        This method will capture perf metrics every SAMPLE_INTERVAL secs, at
        each METRIC_INTERVAL the 90 percentile of the collected metrics is
        calculated and saved. The perf capture runs for PERF_CAPTURE_DURATION
        secs. At the end of the PERF_CAPTURE_DURATION time interval the median
        value of all 90th percentile metrics is returned.

        @returns list of median performance metrics.

        """
        test_start_time = time.time()

        perf_values = {'cpu': [], 'mem': [], 'temp': []}
        perf_metrics = {'cpu': [], 'mem': [], 'temp': []}

         # Create perf_<timestamp> file and writer.
        timestamp_fname = (PERF_FILE_NAME_PREFIX +
                           time.strftime('_%Y-%m-%d_%H-%M') + '.csv')
        timestamp_fpath = os.path.join(self.temp_dir, timestamp_fname)
        timestamp_file = enterprise_longevity_helper.open_perf_file(
                timestamp_fpath)
        timestamp_writer = csv.writer(timestamp_file)

        # Align time of loop start with the sample interval.
        test_elapsed_time = self.elapsed_time(test_start_time)
        time.sleep(enterprise_longevity_helper.syncup_time(
                test_elapsed_time, self.sample_interval))
        test_elapsed_time = self.elapsed_time(test_start_time)

        metric_start_time = time.time()
        metric_prev_time = metric_start_time

        metric_elapsed_prev_time = self.elapsed_time(metric_prev_time)
        offset = enterprise_longevity_helper.modulo_time(
                metric_elapsed_prev_time, self.metric_interval)
        metric_timer = metric_elapsed_prev_time + offset

        while self.elapsed_time(test_start_time) <= self.perf_capture_duration:
            self._record_perf_measurements(perf_values, timestamp_writer)

            # Periodically calculate and record 90th percentile metrics.
            metric_elapsed_prev_time = self.elapsed_time(metric_prev_time)
            metric_timer = metric_elapsed_prev_time + offset
            if metric_timer >= self.metric_interval:
                enterprise_longevity_helper.record_90th_metrics(
                        perf_values, perf_metrics)
                perf_values = {'cpu': [], 'mem': [], 'temp': []}

            # Set previous time to current time.
                metric_prev_time = time.time()
                metric_elapsed_prev_time = self.elapsed_time(metric_prev_time)

                metric_elapsed_time = self.elapsed_time(metric_start_time)
                offset = enterprise_longevity_helper.modulo_time(
                    metric_elapsed_time, self.metric_interval)

                # Set the timer to time elapsed plus offset to next interval.
                metric_timer = metric_elapsed_prev_time + offset

            # Sync the loop time to the sample interval.
            test_elapsed_time = self.elapsed_time(test_start_time)
            time.sleep(enterprise_longevity_helper.syncup_time(
                    test_elapsed_time, self.sample_interval))

        # Close perf timestamp file.
        timestamp_file.close()

         # Open perf timestamp file to read, and aggregated file to append.
        timestamp_file = open(timestamp_fpath, 'r')
        aggregated_fname = (PERF_FILE_NAME_PREFIX + '_aggregated.csv')
        aggregated_fpath = os.path.join(self.temp_dir, aggregated_fname)
        aggregated_file = enterprise_longevity_helper.open_perf_file(
                aggregated_fpath)

         # Append contents of perf timestamp file to perf aggregated file.
        enterprise_longevity_helper.append_to_aggregated_file(
                timestamp_file, aggregated_file)
        timestamp_file.close()
        aggregated_file.close()

        # Copy perf aggregated file to test results directory.
        enterprise_longevity_helper.copy_aggregated_to_resultsdir(
                self.resultsdir, aggregated_fpath, 'perf.csv')

        # Return median of each attribute performance metric.
        logging.info("Perf_metrics: %r ", perf_metrics)
        return enterprise_longevity_helper.get_median_metrics(perf_metrics)


    def run_once(self, host=None, perf_params=None):
        self.client = host
        self.kiosk_app_name = None
        self.perf_params = perf_params
        logging.info('Perf params: %r', self.perf_params)

        if not enterprise_longevity_helper.verify_perf_params(
                EXPECTED_PARAMS, self.perf_params):
            raise error.TestFail('Missing or incorrect perf_params in the'
                                 ' control file. Refer to the README.txt for'
                                 ' info on perf params.: %r'
                                  %(self.perf_params))

        factory = remote_facade_factory.RemoteFacadeFactory(
                host, no_chrome=True)
        self.system_facade = factory.create_system_facade()
        self.kiosk_facade = factory.create_kiosk_facade()

        self._setup_kiosk_app_on_dut(self.perf_params['kiosk_app_attributes'])
        time.sleep(STABILIZATION_DURATION)

        self._initialize_test_variables()

        for iteration in range(self.perf_params['perf_capture_iterations']):
            #TODO(krishnargv@): Add a method to verify that the Kiosk app is
            #                   active and is running on the DUT.
            logging.info("Running perf_capture Iteration: %d", iteration+1)
            self.perf_results = self._run_perf_capture_cycle()
            self._write_perf_keyvals(self.perf_results)
            self._write_perf_results(self.perf_results)

            # Post perf results directly to performance dashboard. You may view
            # uploaded data at https://chromeperf.appspot.com/new_points,
            # with test path pattern=ChromeOS_Enterprise/cros-*/longevity*/*
            if perf_params['test_type'] == 'multiple_samples':
                chart_data = enterprise_longevity_helper.read_perf_results(
                        self.resultsdir, 'results-chart.json')
                data_obj = self._format_data_for_upload(chart_data)
                self._send_to_dashboard(data_obj)

        tpm_utils.ClearTPMOwnerRequest(self.client)
