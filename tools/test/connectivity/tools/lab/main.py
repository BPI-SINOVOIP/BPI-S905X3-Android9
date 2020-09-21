#!/usr/bin/env python
#
#   Copyright 2017 - The Android Open Source Project
#
#   Licensed under the Apache License, Version 2.0 (the "License");
#   you may not use this file except in compliance with the License.
#   You may obtain a copy of the License at
#
#       http://www.apache.org/licenses/LICENSE-2.0
#
#   Unless required by applicable law or agreed to in writing, software
#   distributed under the License is distributed on an "AS IS" BASIS,
#   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#   See the License for the specific language governing permissions and
#   limitations under the License.

from __future__ import absolute_import
from __future__ import division
from __future__ import print_function

import argparse
import json
import os
import sys

import health_checker
from metrics.adb_hash_metric import AdbHashMetric
from metrics.cpu_metric import CpuMetric
from metrics.disk_metric import DiskMetric
from metrics.name_metric import NameMetric
from metrics.network_metric import NetworkMetric
from metrics.num_users_metric import NumUsersMetric
from metrics.process_time_metric import ProcessTimeMetric
from metrics.ram_metric import RamMetric
from metrics.read_metric import ReadMetric
from metrics.system_load_metric import SystemLoadMetric
from metrics.time_metric import TimeMetric
from metrics.time_sync_metric import TimeSyncMetric
from metrics.uptime_metric import UptimeMetric
from metrics.usb_metric import UsbMetric
from metrics.verify_metric import VerifyMetric
from metrics.version_metric import AdbVersionMetric
from metrics.version_metric import FastbootVersionMetric
from metrics.version_metric import KernelVersionMetric
from metrics.version_metric import PythonVersionMetric
from metrics.zombie_metric import ZombieMetric
from reporters.json_reporter import JsonReporter
from reporters.logger_reporter import LoggerReporter
from runner import InstantRunner


class RunnerFactory(object):
    _reporter_constructor = {
        'logger': lambda param, output: [LoggerReporter(param)],
        'json': lambda param, output: [JsonReporter(param, output)]
    }

    _metric_constructor = {
        'usb_io': lambda param: [UsbMetric()],
        'disk': lambda param: [DiskMetric()],
        'uptime': lambda param: [UptimeMetric()],
        'verify_devices':
            lambda param: [VerifyMetric(), AdbHashMetric()],
        'ram': lambda param: [RamMetric()],
        'cpu': lambda param: [CpuMetric()],
        'network': lambda param: [NetworkMetric(param)],
        'hostname': lambda param: [NameMetric()],
        'all': lambda param: [AdbHashMetric(),
                              AdbVersionMetric(),
                              CpuMetric(),
                              DiskMetric(),
                              FastbootVersionMetric(),
                              KernelVersionMetric(),
                              NameMetric(),
                              NetworkMetric(),
                              NumUsersMetric(),
                              ProcessTimeMetric(),
                              PythonVersionMetric(),
                              RamMetric(),
                              ReadMetric(),
                              SystemLoadMetric(),
                              TimeMetric(),
                              TimeSyncMetric(),
                              UptimeMetric(),
                              UsbMetric(),
                              VerifyMetric(),
                              ZombieMetric()]
    }

    @classmethod
    def create(cls, arguments):
        """ Creates the Runner Class that will take care of gather metrics
        and determining how to report those metrics.

        Args:
            arguments: The arguments passed in through command line, a dict.

        Returns:
            Returns a Runner that was created by passing in a list of
            metrics and list of reporters.
        """
        arg_dict = arguments
        metrics = []
        reporters = []

        # Get health config file, if specified
        # If not specified, default to 'config.json'
        config_file = arg_dict.pop('config',
                                   os.path.join(sys.path[0], 'config.json'))
        # Get output file path, if specified
        # If not specified, default to 'output.json'
        output_file = arg_dict.pop('output', 'output.json')

        try:
            with open(config_file) as json_data:
                health_config = json.load(json_data)
        except IOError:
            sys.exit('Config file does not exist')
        # Create health checker
        checker = health_checker.HealthChecker(health_config)

        # Get reporters
        rep_list = arg_dict.pop('reporter')
        if rep_list is not None:
            for rep_type in rep_list:
                reporters += cls._reporter_constructor[rep_type](checker,
                                                                 output_file)
        else:
            # If no reporter specified, default to logger.
            reporters += [LoggerReporter(checker)]

        # Check keys and values to see what metrics to include.
        for key in arg_dict:
            val = arg_dict[key]
            if val is not None:
                metrics += cls._metric_constructor[key](val)

        return InstantRunner(metrics, reporters)


def _argparse():
    parser = argparse.ArgumentParser(
        description='Tool for getting lab health of android testing lab',
        prog='Lab Health')

    parser.add_argument(
        '-v',
        '--version',
        action='version',
        version='%(prog)s v0.1.0',
        help='specify version of program')
    parser.add_argument(
        '-i',
        '--usb-io',
        action='store_true',
        default=None,
        help='display recent USB I/O')
    parser.add_argument(
        '-u',
        '--uptime',
        action='store_true',
        default=None,
        help='display uptime of current lab station')
    parser.add_argument(
        '-d',
        '--disk',
        choices=['size', 'used', 'avail', 'percent'],
        nargs='*',
        help='display the disk space statistics')
    parser.add_argument(
        '-ra',
        '--ram',
        action='store_true',
        default=None,
        help='display the current RAM usage')
    parser.add_argument(
        '-cp',
        '--cpu',
        action='count',
        default=None,
        help='display the current CPU usage as percent')
    parser.add_argument(
        '-vd',
        '--verify-devices',
        action='store_true',
        default=None,
        help=('verify all devices connected are in \'device\' mode, '
              'environment variables set properly, '
              'and hash of directory is correct'))
    parser.add_argument(
        '-r',
        '--reporter',
        choices=['logger', 'json'],
        nargs='+',
        help='choose the reporting method needed')
    parser.add_argument(
        '-p',
        '--program',
        choices=['python', 'adb', 'fastboot', 'os', 'kernel'],
        nargs='*',
        help='display the versions of chosen programs (default = all)')
    parser.add_argument(
        '-n',
        '--network',
        nargs='*',
        default=None,
        help='retrieve status of network')
    parser.add_argument(
        '-a',
        '--all',
        action='store_true',
        default=None,
        help='Display every metric available')
    parser.add_argument(
        '-hn',
        '--hostname',
        action='store_true',
        default=None,
        help='Display the hostname of the current system')
    parser.add_argument(
        '-c',
        '--config',
        nargs='?',
        default='config.json',
        metavar="<PATH>",
        help='Path to health configuration file, defaults to `config.json`')
    parser.add_argument(
        '-o',
        '--output',
        nargs='?',
        default='output.json',
        metavar="<PATH>",
        help='Path to where output file will be written, if applicable,'
        ' defaults to `output.json`')

    return parser


def main():
    parser = _argparse()

    if len(sys.argv) == 1:
        parser.print_help()
        sys.exit(1)

    r = RunnerFactory().create(vars(parser.parse_args()))
    r.run()


if __name__ == '__main__':
    main()
