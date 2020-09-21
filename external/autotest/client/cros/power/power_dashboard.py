# Copyright (c) 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import json, numpy, os, time, urllib, urllib2

from autotest_lib.client.bin import utils
from autotest_lib.client.common_lib import lsbrelease_utils
from autotest_lib.client.cros.power import power_status
from autotest_lib.client.cros.power import power_utils


class BaseDashboard(object):
    """Base class that implements method for prepare and upload data to power
    dashboard.
    """

    def __init__(self, logger, testname, resultsdir=None, uploadurl=None):
        """Create BaseDashboard objects

        Args:
            logger: object that store the log. This will get convert to
                    dictionary by self._convert()
            testname: name of current test
            resultsdir: directory to save the power json
            uploadurl: url to upload power data
        """
        self._logger = logger
        self._testname = testname
        self._resultsdir = resultsdir
        self._uploadurl = uploadurl


    def _create_powerlog_dict(self, raw_measurement):
        """Create powerlog dictionary from raw measurement data
        Data format in go/power-dashboard-data

        Args:
            raw_measurement: dictionary contains raw measurement data.

        Returns:
            A dictionary of powerlog.
        """
        powerlog_dict = {
            'format_version': 2,
            'timestamp': time.time(),
            'test': self._testname,
            'dut': {
                'board': utils.get_board(),
                'version': {
                    'hw': utils.get_hardware_revision(),
                    'milestone':
                            lsbrelease_utils.get_chromeos_release_milestone(),
                    'os': lsbrelease_utils.get_chromeos_release_version(),
                    'channel': lsbrelease_utils.get_chromeos_channel(),
                    'firmware': utils.get_firmware_version(),
                    'ec': utils.get_ec_version(),
                    'kernel': utils.get_kernel_version(),
                },
                'sku' : {
                    'cpu': utils.get_cpu_name(),
                    'memory_size': utils.get_mem_total_gb(),
                    'storage_size':
                            utils.get_disk_size_gb(utils.get_root_device()),
                    'display_resolution': utils.get_screen_resolution(),
                },
                'ina': {
                    'version': 0,
                    'ina': raw_measurement['data'].keys()
                },
                'note': ''
            },
            'power': raw_measurement
        }

        if power_utils.has_battery():
            # Round the battery size to nearest tenth because it is fluctuated
            # for platform without battery norminal voltage data.
            powerlog_dict['dut']['sku']['battery_size'] = round(
                    power_status.get_status().battery[0].energy_full_design, 1)
            powerlog_dict['dut']['sku']['battery_shutdown_percent'] = \
                    power_utils.get_low_battery_shutdown_percent()
        return powerlog_dict


    def _save_json(self, powerlog_dict, resultsdir, filename='power_log.json'):
        """Convert powerlog dict to human readable formatted JSON and
        save to <resultsdir>/<filename>

        Args:
            powerlog_dict: dictionary of power data
            resultsdir: directory to save formatted JSON object
            filename: filename to save
        """
        filename = os.path.join(resultsdir, filename)
        with file(filename, 'w') as f:
            json.dump(powerlog_dict, f, indent=4, separators=(',', ': '))


    def _upload(self, powerlog_dict, uploadurl):
        """Convert powerlog dict to minimal size JSON and upload to dashboard.

        Args:
            powerlog_dict: dictionary of power data
        """
        data_obj = {'data': json.dumps(powerlog_dict)}
        encoded = urllib.urlencode(data_obj)
        req = urllib2.Request(uploadurl, encoded)
        urllib2.urlopen(req)


    def _convert(self):
        """Convert data from self._logger object to raw power measurement
        dictionary.

        MUST be implemented in subclass

        Return:
            raw measurement dictionary
        """
        raise NotImplementedError

    def upload(self):
        """Upload powerlog to dashboard and save data to results directory.
        """
        raw_measurement = self._convert()
        powerlog_dict = self._create_powerlog_dict(raw_measurement)
        if self._resultsdir is not None:
            self._save_json(powerlog_dict, self._resultsdir)
        if self._uploadurl is not None:
            self._upload(powerlog_dict, self._uploadurl)


class MeasurementLoggerDashboard(BaseDashboard):
    """Dashboard class for power_status.MeasurementLogger
    """

    def _convert(self):
        """Convert data from power_status.MeasurementLogger object to raw
        power measurement dictionary.

        Return:
            raw measurement dictionary
        """
        power_dict = {
            'sample_count': len(self._logger.readings),
            'sample_duration': 0,
            'average': dict(),
            'data': dict()
        }
        if power_dict['sample_count'] > 1:
            total_duration = self._logger.times[-1] - self._logger.times[0]
            power_dict['sample_duration'] = \
                    1.0 * total_duration / (power_dict['sample_count'] - 1)

        for i, domain_readings in enumerate(zip(*self._logger.readings)):
            domain = self._logger.domains[i]
            power_dict['data'][domain] = domain_readings
            power_dict['average'][domain] = numpy.average(domain_readings)
        return power_dict


class PowerLoggerDashboard(MeasurementLoggerDashboard):
    """Dashboard class for power_status.PowerLogger
    """

    def __init__(self, logger, testname, resultsdir=None, uploadurl=None):
        if uploadurl is None:
            uploadurl = 'http://chrome-power.appspot.com/rapl'
        super(PowerLoggerDashboard, self).__init__(logger, testname, resultsdir,
                                                   uploadurl)
