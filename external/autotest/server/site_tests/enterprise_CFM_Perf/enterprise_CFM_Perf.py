# Copyright (c) 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import datetime
import glob
import os
import time

from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib.cros import system_metrics_collector
from autotest_lib.client.common_lib.cros.cfm.metrics import (
        media_metrics_collector)
from autotest_lib.server.cros import cfm_jmidata_log_collector
from autotest_lib.server.cros.cfm import cfm_base_test

_SHORT_TIMEOUT = 5
_MEASUREMENT_DURATION_SECONDS = 10
_TOTAL_TEST_DURATION_SECONDS = 900

_BASE_DIR = '/home/chronos/user/Storage/ext/'
_EXT_ID = 'ikfcpmgefdpheiiomgmhlmmkihchmdlj'
_JMI_DIR = '/0*/File\ System/000/t/00/*'
_JMI_SOURCE_DIR = _BASE_DIR + _EXT_ID + _JMI_DIR

class ParticipantCountMetric(system_metrics_collector.Metric):
    """
    Metric for getting the current participant count in a call.
    """
    def __init__(self, cfm_facade):
        """
        Initializes with a cfm_facade.

        @param cfm_facade object having a get_participant_count() method.
        """
        super(ParticipantCountMetric, self).__init__(
                'participant_count',
                'participants',
                higher_is_better=True)
        self.cfm_facade = cfm_facade

    def collect_metric(self):
        """
        Collects one metric value.
        """
        self.values.append(self.cfm_facade.get_participant_count())


class enterprise_CFM_Perf(cfm_base_test.CfmBaseTest):
    """This is a server test which clears device TPM and runs
    enterprise_RemoraRequisition client test to enroll the device in to hotrod
    mode. After enrollment is successful, it collects and logs cpu, memory and
    temperature data from the device under test."""
    version = 1

    def start_hangout(self):
        """Waits for the landing page and starts a hangout session."""
        self.cfm_facade.wait_for_hangouts_telemetry_commands()
        current_date = datetime.datetime.now().strftime("%Y-%m-%d")
        hangout_name = current_date + '-cfm-perf'
        self.cfm_facade.start_new_hangout_session(hangout_name)


    def join_meeting(self):
        """Waits for the landing page and joins a meeting session."""
        self.cfm_facade.wait_for_meetings_landing_page()
        # Daily meeting for perf testing with 9 remote participants.
        meeting_code = 'nis-rhmz-dyh'
        self.cfm_facade.join_meeting_session(meeting_code)


    def collect_perf_data(self):
        """
        Collects run time data from the DUT using system_metrics_collector.
        Writes the data to the chrome perf dashboard.
        """
        start_time = time.time()
        while (time.time() - start_time) < _TOTAL_TEST_DURATION_SECONDS:
            time.sleep(_MEASUREMENT_DURATION_SECONDS)
            self.metrics_collector.collect_snapshot()
            if self.is_meeting:
                # Media metrics collector is only available for Meet.
                self.media_metrics_collector.collect_snapshot()
        self.metrics_collector.write_metrics(self.output_perf_value)

    def _get_average(self, data_type, jmidata):
        """Computes mean of a list of numbers.

        @param data_type: Type of data to be retrieved from jmi data log.
        @param jmidata: Raw jmi data log to parse.
        @return Mean computed from the list of numbers.
        """
        data = self._get_data_from_jmifile(data_type, jmidata)
        if not data:
            return 0
        return float(sum(data)) / len(data)


    def _get_max_value(self, data_type, jmidata):
        """Computes maximum value of a list of numbers.

        @param data_type: Type of data to be retrieved from jmi data log.
        @param jmidata: Raw jmi data log to parse.
        @return Maxium value from the list of numbers.
        """
        data = self._get_data_from_jmifile(data_type, jmidata)
        if not data:
            return 0
        return max(data)


    def _get_sum(self, data_type, jmidata):
        """Computes sum of a list of numbers.

        @param data_type: Type of data to be retrieved from jmi data log.
        @param jmidata: Raw jmi data log to parse.
        @return Sum computed from the list of numbers.
        """
        data = self._get_data_from_jmifile(data_type, jmidata)
        if not data:
            return 0
        return sum(data)


    def _get_last_value(self, data_type, jmidata):
        """Gets last value of a list of numbers.

        @param data_type: Type of data to be retrieved from jmi data log.
        @param jmidata: Raw jmi data log to parse.
        @return The last value in the jmidata for the specified data_type. 0 if
                there are no values in the jmidata for this data_type.
        """
        data = self._get_data_from_jmifile(data_type, jmidata)
        if not data:
            return 0
        return data[-1]


    def _get_data_from_jmifile(self, data_type, jmidata):
        """Gets data from jmidata log for given data type.

        @param data_type: Type of data to be retrieved from jmi data log.
        @param jmidata: Raw jmi data log to parse.
        @return Data for given data type from jmidata log.
        """
        if self.is_meeting:
            try:
                timestamped_values = self.media_metrics_collector.get_metric(
                        data_type)
            except KeyError:
                # Ensure we always return at least one element, or perf uploads
                # will be sad.
                return [0]
            # Strip timestamps.
            values = [x[1] for x in timestamped_values]
            # Each entry in values is a list, extract the raw values:
            res = []
            for value_list in values:
                res.extend(value_list)
            # Ensure we always return at least one element, or perf uploads will
            # be sad.
            return res or [0]
        else:
            return cfm_jmidata_log_collector.GetDataFromLogs(
                    self, data_type, jmidata)


    def _get_file_to_parse(self):
        """Copy jmi logs from client to test's results directory.

        @return The newest jmi log file.
        """
        self._host.get_file(_JMI_SOURCE_DIR, self.resultsdir)
        source_jmi_files = self.resultsdir + '/0*'
        if not source_jmi_files:
            raise error.TestNAError('JMI data file not found.')
        newest_file = max(glob.iglob(source_jmi_files), key=os.path.getctime)
        return newest_file

    def upload_jmidata(self):
        """
        Write jmidata results to results-chart.json file for Perf Dashboard.
        """
        jmi_file = self._get_file_to_parse()
        jmifile_to_parse = open(jmi_file, 'r')
        jmidata = jmifile_to_parse.read()

        # Compute and save aggregated stats from JMI.
        self.output_perf_value(description='sum_vid_in_frames_decoded',
                value=self._get_sum('frames_decoded', jmidata), units='frames',
                higher_is_better=True)

        self.output_perf_value(description='sum_vid_out_frames_encoded',
                value=self._get_sum('frames_encoded', jmidata), units='frames',
                higher_is_better=True)

        self.output_perf_value(description='vid_out_adapt_changes',
                value=self._get_last_value('adaptation_changes', jmidata),
                units='count', higher_is_better=False)

        self.output_perf_value(description='video_out_encode_time',
                value=self._get_data_from_jmifile(
                        'average_encode_time', jmidata),
                units='ms', higher_is_better=False)

        self.output_perf_value(description='max_video_out_encode_time',
                value=self._get_max_value('average_encode_time', jmidata),
                units='ms', higher_is_better=False)

        self.output_perf_value(description='vid_out_bandwidth_adapt',
                value=self._get_average('bandwidth_adaptation', jmidata),
                units='bool', higher_is_better=False)

        self.output_perf_value(description='vid_out_cpu_adapt',
                value=self._get_average('cpu_adaptation', jmidata),
                units='bool', higher_is_better=False)

        self.output_perf_value(description='video_in_res',
                value=self._get_data_from_jmifile(
                        'video_received_frame_height', jmidata),
                units='px', higher_is_better=True)

        self.output_perf_value(description='video_out_res',
                value=self._get_data_from_jmifile(
                        'video_sent_frame_height', jmidata),
                units='resolution', higher_is_better=True)

        self.output_perf_value(description='vid_in_framerate_decoded',
                value=self._get_data_from_jmifile(
                        'framerate_decoded', jmidata),
                units='fps', higher_is_better=True)

        self.output_perf_value(description='vid_out_framerate_input',
                value=self._get_data_from_jmifile(
                        'framerate_outgoing', jmidata),
                units='fps', higher_is_better=True)

        self.output_perf_value(description='vid_in_framerate_to_renderer',
                value=self._get_data_from_jmifile(
                        'framerate_to_renderer', jmidata),
                units='fps', higher_is_better=True)

        self.output_perf_value(description='vid_in_framerate_received',
                value=self._get_data_from_jmifile(
                        'framerate_received', jmidata),
                units='fps', higher_is_better=True)

        self.output_perf_value(description='vid_out_framerate_sent',
                value=self._get_data_from_jmifile('framerate_sent', jmidata),
                units='fps', higher_is_better=True)

        self.output_perf_value(description='vid_in_frame_width',
                value=self._get_data_from_jmifile(
                        'video_received_frame_width', jmidata),
                units='px', higher_is_better=True)

        self.output_perf_value(description='vid_out_frame_width',
                value=self._get_data_from_jmifile(
                        'video_sent_frame_width', jmidata),
                units='px', higher_is_better=True)

        self.output_perf_value(description='vid_out_encode_cpu_usage',
                value=self._get_data_from_jmifile(
                        'video_encode_cpu_usage', jmidata),
                units='percent', higher_is_better=False)

        total_vid_packets_sent = self._get_sum('video_packets_sent', jmidata)
        total_vid_packets_lost = self._get_sum('video_packets_lost', jmidata)
        lost_packet_percentage = float(total_vid_packets_lost)*100/ \
                                 float(total_vid_packets_sent) if \
                                 total_vid_packets_sent else 0

        self.output_perf_value(description='lost_packet_percentage',
                value=lost_packet_percentage, units='percent',
                higher_is_better=False)
        self.output_perf_value(description='cpu_usage_jmi',
                value=self._get_data_from_jmifile('cpu_percent', jmidata),
                units='percent', higher_is_better=False)
        self.output_perf_value(description='renderer_cpu_usage',
                value=self._get_data_from_jmifile(
                    'renderer_cpu_percent', jmidata),
                units='percent', higher_is_better=False)
        self.output_perf_value(description='browser_cpu_usage',
                value=self._get_data_from_jmifile(
                        'browser_cpu_percent', jmidata),
                units='percent', higher_is_better=False)

        self.output_perf_value(description='gpu_cpu_usage',
                value=self._get_data_from_jmifile(
                        'gpu_cpu_percent', jmidata),
                units='percent', higher_is_better=False)

        self.output_perf_value(description='active_streams',
                value=self._get_data_from_jmifile(
                        'num_active_vid_in_streams', jmidata),
                units='count', higher_is_better=True)


    def initialize(self, host, run_test_only=False):
        """
        Initializes common test properties.

        @param host: a host object representing the DUT.
        @param run_test_only: Wheter to run only the test or to also perform
            deprovisioning, enrollment and system reboot. See cfm_base_test.
        """
        super(enterprise_CFM_Perf, self).initialize(host, run_test_only)
        self.system_facade = self._facade_factory.create_system_facade()
        metrics = system_metrics_collector.create_default_metric_set(
                self.system_facade)
        metrics.append(ParticipantCountMetric(self.cfm_facade))
        self.metrics_collector = (system_metrics_collector.
                                  SystemMetricsCollector(self.system_facade,
                                                         metrics))
        data_point_collector = media_metrics_collector.DataPointCollector(
                self.cfm_facade)
        self.media_metrics_collector = (media_metrics_collector
                                        .MetricsCollector(data_point_collector))

    def run_once(self, is_meeting=False):
        """Stays in a meeting/hangout and collects perf data."""
        self.is_meeting = is_meeting
        if is_meeting:
            self.join_meeting()
        else:
            self.start_hangout()

        self.collect_perf_data()

        if is_meeting:
            self.cfm_facade.end_meeting_session()
        else:
            self.cfm_facade.end_hangout_session()

        self.upload_jmidata()

