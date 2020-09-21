# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import os
import re
import json
import time
import shutil
import logging

from autotest_lib.client.common_lib.cros import perf_stat_lib


def get_cpu_usage(system_facade, measurement_duration_seconds):
    """
    Returns cpu usage in %.

    @param system_facade: A SystemFacadeRemoteAdapter to access
                          the CPU capture functionality from the DUT.
    @param measurement_duration_seconds: CPU metric capture duration.

    @returns current CPU usage percentage.

    """
    cpu_usage_start = system_facade.get_cpu_usage()
    time.sleep(measurement_duration_seconds)
    cpu_usage_end = system_facade.get_cpu_usage()
    return system_facade.compute_active_cpu_time(
            cpu_usage_start, cpu_usage_end) * 100


def get_memory_usage(system_facade):
    """
    Returns total used memory in %.

    @param system_facade: A SystemFacadeRemoteAdapter to access
                          the memory capture functionality from the DUT.

    @returns current memory used.

    """
    total_memory = system_facade.get_mem_total()
    return ((total_memory - system_facade.get_mem_free())
            * 100 / total_memory)


def get_temperature_data(client, system_facade):
    """
    Returns temperature sensor data in Celcius.

    @param system_facade: A SystemFacadeRemoteAdapter to access the temperature
                          capture functionality from the DUT.

    @returns current CPU temperature.

    """
    ectool = client.run('ectool version', ignore_status=True)
    if not ectool.exit_status:
        ec_temp = system_facade.get_ec_temperatures()
        return ec_temp[1]
    else:
        temp_sensor_name = 'temp0'
        if not temp_sensor_name:
            return 0
        MOSYS_OUTPUT_RE = re.compile('(\w+)="(.*?)"')
        values = {}
        cmd = 'mosys -k sensor print thermal %s' % temp_sensor_name
        for kv in MOSYS_OUTPUT_RE.finditer(client.run_output(cmd)):
            key, value = kv.groups()
            if key == 'reading':
                value = int(value)
            values[key] = value
        return values['reading']


#TODO(krishnargv): Replace _get_point_id with a call to the
#                  _get_id_from_version method of the perf_uploader.py.
def get_point_id(cros_version, epoch_minutes, version_pattern):
    """
    Compute point ID from ChromeOS version number and epoch minutes.

    @param cros_version: String of ChromeOS version number.
    @param epoch_minutes: String of minutes since 1970.

    @returns unique integer ID computed from given version and epoch.

    """
    # Number of digits from each part of the Chrome OS version string.
    cros_version_col_widths = [0, 4, 3, 2]

    def get_digits(version_num, column_widths):
        if re.match(version_pattern, version_num):
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


def open_perf_file(file_path):
    """
    Open a perf file. Write header line if new. Return file object.

    If the file on |file_path| already exists, then open file for
    appending only. Otherwise open for writing only.

    @param file_path: file path for perf file.

    @returns file object for the perf file.

    """
    if os.path.isfile(file_path):
        perf_file = open(file_path, 'a+')
    else:
        perf_file = open(file_path, 'w')
        perf_file.write('Time,CPU,Memory,Temperature (C)\r\n')
    return perf_file


def modulo_time(timer, interval):
    """
    Get time eplased on |timer| for the |interval| modulus.

    Value returned is used to adjust the timer so that it is
    synchronized with the current interval.

    @param timer: time on timer, in seconds.
    @param interval: period of time in seconds.

    @returns time elapsed from the start of the current interval.

    """
    return timer % int(interval)


def syncup_time(timer, interval):
    """
    Get time remaining on |timer| for the |interval| modulus.

    Value returned is used to induce sleep just long enough to put the
    process back in sync with the timer.

    @param timer: time on timer, in seconds.
    @param interval: period of time in seconds.

    @returns time remaining till the end of the current interval.

    """
    return interval - (timer % int(interval))


def append_to_aggregated_file(ts_file, ag_file):
    """
    Append contents of perf timestamp file to perf aggregated file.

    @param ts_file: file handle for performance timestamped file.
    @param ag_file: file handle for performance aggregated file.

    """
    next(ts_file)  # Skip fist line (the header) of timestamped file.
    for line in ts_file:
        ag_file.write(line)


def copy_aggregated_to_resultsdir(resultsdir, aggregated_fpath, f_name):
    """Copy perf aggregated file to results dir for AutoTest results.

    Note: The AutoTest results default directory is located at /usr/local/
    autotest/results/default/longevity_Tracker/results

    @param resultsdir: Directory name where the perf results are stored.
    @param aggregated_fpath: file path to Aggregated performance values.
    @param f_name: Name of the perf File
    """
    results_fpath = os.path.join(resultsdir, f_name)
    shutil.copy(aggregated_fpath, results_fpath)
    logging.info('Copied %s to %s)', aggregated_fpath, results_fpath)


def record_90th_metrics(perf_values, perf_metrics):
    """Record 90th percentile metric of attribute performance values.

    @param perf_values: dict attribute performance values.
    @param perf_metrics: dict attribute 90%-ile performance metrics.
    """
    # Calculate 90th percentile for each attribute.
    cpu_values = perf_values['cpu']
    mem_values = perf_values['mem']
    temp_values = perf_values['temp']
    cpu_metric = perf_stat_lib.get_kth_percentile(cpu_values, .90)
    mem_metric = perf_stat_lib.get_kth_percentile(mem_values, .90)
    temp_metric = perf_stat_lib.get_kth_percentile(temp_values, .90)

    logging.info('Performance values: %s', perf_values)
    logging.info('90th percentile: cpu: %s, mem: %s, temp: %s',
                 cpu_metric, mem_metric, temp_metric)

    # Append 90th percentile to each attribute performance metric.
    perf_metrics['cpu'].append(cpu_metric)
    perf_metrics['mem'].append(mem_metric)
    perf_metrics['temp'].append(temp_metric)


def get_median_metrics(metrics):
    """
    Returns median of each attribute performance metric.

    If no metric values were recorded, return 0 for each metric.

    @param metrics: dict of attribute performance metric lists.

    @returns dict of attribute performance metric medians.

    """
    if len(metrics['cpu']):
        cpu_metric = perf_stat_lib.get_median(metrics['cpu'])
        mem_metric = perf_stat_lib.get_median(metrics['mem'])
        temp_metric = perf_stat_lib.get_median(metrics['temp'])
    else:
        cpu_metric = 0
        mem_metric = 0
        temp_metric = 0
    logging.info('Median of 90th percentile: cpu: %s, mem: %s, temp: %s',
                 cpu_metric, mem_metric, temp_metric)
    return {'cpu': cpu_metric, 'mem': mem_metric, 'temp': temp_metric}


def read_perf_results(resultsdir, resultsfile):
    """
    Read perf results from results-chart.json file for Perf Dashboard.

    @returns dict of perf results, formatted as JSON chart data.

    """
    results_file = os.path.join(resultsdir, resultsfile)
    with open(results_file, 'r') as fp:
        contents = fp.read()
        chart_data = json.loads(contents)
    # TODO(krishnargv): refactor this with a better method to delete.
    open(results_file, 'w').close()
    return chart_data


def verify_perf_params(expected_params, perf_params):
    """
    Verify that all the expected paramaters were passed to the test.

    Return True if the perf_params dict passed via the control file
    has all of the the expected parameters and have valid values.

    @param expected_params: list of expected parameters
    @param perf_params: dict of the paramaters passed via control file.

    @returns True if the perf_params dict is valid, else returns False.

    """
    for param in expected_params:
        if param not in perf_params or not perf_params[param]:
            return False
    return True