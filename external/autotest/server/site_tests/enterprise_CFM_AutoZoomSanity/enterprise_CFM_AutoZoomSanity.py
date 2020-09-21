# Copyright (c) 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import glob
import logging
import os
import time

from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib.cros import perf_stat_lib
from autotest_lib.server.cros.cfm import cfm_base_test
from autotest_lib.server.cros import cfm_jmidata_log_collector

_BASE_DIR = '/home/chronos/user/Storage/ext/'
_EXT_ID = 'ikfcpmgefdpheiiomgmhlmmkihchmdlj'
_JMI_DIR = '/0*/File\ System/000/t/00/*'
_JMI_SOURCE_DIR = _BASE_DIR + _EXT_ID + _JMI_DIR
_USB_DIR = '/sys/bus/usb/devices'


class enterprise_CFM_AutoZoomSanity(cfm_base_test.CfmBaseTest):
    """Auto Zoom Sanity test."""
    version = 1

    def get_data_from_jmifile(self, data_type, jmidata):
        """
        Gets data from jmidata log for given data type.

        @param data_type: Type of data to be retrieved from jmi data log.
        @param jmidata: Raw jmi data log to parse.
        @returns Data for given data type from jmidata log.
        """
        return cfm_jmidata_log_collector.GetDataFromLogs(
                self, data_type, jmidata)


    def get_file_to_parse(self):
        """
        Copy jmi logs from client to test's results directory.

        @returns The newest jmi log file.
        """
        self._host.get_file(_JMI_SOURCE_DIR, self.resultsdir)
        source_jmi_files = self.resultsdir + '/0*'
        if not source_jmi_files:
            raise error.TestNAError('JMI data file not found.')
        newest_file = max(glob.iglob(source_jmi_files), key=os.path.getctime)
        return newest_file


    def verify_cfm_sent_resolution(self):
        """Check / verify CFM sent video resolution data from JMI logs."""
        jmi_file = self.get_file_to_parse()
        jmifile_to_parse = open(jmi_file, 'r')
        jmidata = jmifile_to_parse.read()

        cfm_sent_res_list = self.get_data_from_jmifile(
                'video_sent_frame_height', jmidata)
        percentile_95 = perf_stat_lib.get_kth_percentile(
                cfm_sent_res_list, 0.95)

        self.output_perf_value(description='video_sent_frame_height',
                               value=cfm_sent_res_list,
                               units='resolution',
                               higher_is_better=True)
        self.output_perf_value(description='95th percentile res sent',
                               value=percentile_95,
                               units='resolution',
                               higher_is_better=True)

        # TODO(dkaeding): Add logic to examine the cfm sent resolution and
        # take appropriate action.
        logging.info('95th percentile of outgoing video resolution: %s',
                     percentile_95)


    def check_verify_callgrok_logs(self):
        """Verify needed information in callgrok logs."""
        # TODO(dkaeding): Implement this method.
        return NotImplemented


    def get_usb_device_dirs(self):
        """Gets usb device dirs from _USB_DIR path.

        @returns list with number of device dirs else None
        """
        usb_dir_list = list()
        cmd = 'ls %s' % _USB_DIR
        cmd_output = self._host.run(cmd).stdout.strip().split('\n')
        for d in cmd_output:
            usb_dir_list.append(os.path.join(_USB_DIR, d))
        return usb_dir_list


    def file_exists_on_host(self, path):
        """
        Checks if file exists on host.

        @param path: File path
        @returns True or False
        """
        return self._host.run('ls %s' % path,
                              ignore_status=True).exit_status == 0


    def check_peripherals(self, peripheral_dict):
        """
        Check and verify correct peripherals are attached.

        @param peripheral_dict: dict of peripherals that should be connected
        """
        usb_dir_list = self.get_usb_device_dirs()
        peripherals_found = list()
        for d_path in usb_dir_list:
            file_name = os.path.join(d_path, 'product')
            if self.file_exists_on_host(file_name):
                peripherals_found.append(self._host.run(
                        'cat %s' % file_name).stdout.strip())

        logging.info('Attached peripherals: %s', peripherals_found)

        for peripheral in peripheral_dict:
            if peripheral not in peripherals_found:
                raise error.TestFail('%s not found.' % peripheral)


    def run_once(self, session_length, peripheral_dict):
        """Runs the sanity test."""
        self.cfm_facade.wait_for_meetings_telemetry_commands()
        self.check_peripherals(peripheral_dict)
        self.cfm_facade.start_meeting_session()
        time.sleep(session_length)
        self.cfm_facade.end_meeting_session()
        self.verify_cfm_sent_resolution()
        self.check_verify_callgrok_logs()
