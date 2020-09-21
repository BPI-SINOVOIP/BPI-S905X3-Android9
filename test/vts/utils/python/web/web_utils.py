#
# Copyright (C) 2017 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

import base64
import getpass
import logging
import os
import socket
import time

from vts.proto import VtsReportMessage_pb2 as ReportMsg
from vts.runners.host import keys
from vts.utils.python.web import dashboard_rest_client
from vts.utils.python.web import feature_utils

_PROFILING_POINTS = "profiling_points"


class WebFeature(feature_utils.Feature):
    """Feature object for web functionality.

    Attributes:
        enabled: boolean, True if systrace is enabled, False otherwise
        report_msg: TestReportMessage, Proto summarizing the test run
        current_test_report_msg: TestCaseReportMessage, Proto summarizing the current test case
        rest_client: DashboardRestClient, client to which data will be posted
    """

    _TOGGLE_PARAM = keys.ConfigKeys.IKEY_ENABLE_WEB
    _REQUIRED_PARAMS = [
        keys.ConfigKeys.IKEY_DASHBOARD_POST_COMMAND,
        keys.ConfigKeys.IKEY_SERVICE_JSON_PATH,
        keys.ConfigKeys.KEY_TESTBED_NAME, keys.ConfigKeys.IKEY_BUILD,
        keys.ConfigKeys.IKEY_ANDROID_DEVICE, keys.ConfigKeys.IKEY_ABI_NAME,
        keys.ConfigKeys.IKEY_ABI_BITNESS
    ]
    _OPTIONAL_PARAMS = [keys.ConfigKeys.RUN_AS_VTS_SELFTEST]

    def __init__(self, user_params):
        """Initializes the web feature.

        Parses the arguments and initializes the web functionality.

        Args:
            user_params: A dictionary from parameter name (String) to parameter value.
        """
        self.ParseParameters(
            toggle_param_name=self._TOGGLE_PARAM,
            required_param_names=self._REQUIRED_PARAMS,
            optional_param_names=self._OPTIONAL_PARAMS,
            user_params=user_params)
        if not self.enabled:
            return

        # Initialize the dashboard client
        post_cmd = getattr(self, keys.ConfigKeys.IKEY_DASHBOARD_POST_COMMAND)
        service_json_path = str(
            getattr(self, keys.ConfigKeys.IKEY_SERVICE_JSON_PATH))
        self.rest_client = dashboard_rest_client.DashboardRestClient(
            post_cmd, service_json_path)
        if not self.rest_client.Initialize():
            self.enabled = False

        self.report_msg = ReportMsg.TestReportMessage()
        self.report_msg.test = str(
            getattr(self, keys.ConfigKeys.KEY_TESTBED_NAME))
        self.report_msg.test_type = ReportMsg.VTS_HOST_DRIVEN_STRUCTURAL
        self.report_msg.start_timestamp = feature_utils.GetTimestamp()
        self.report_msg.host_info.hostname = socket.gethostname()

        android_devices = getattr(self, keys.ConfigKeys.IKEY_ANDROID_DEVICE,
                                  None)
        if not android_devices or not isinstance(android_devices, list):
            logging.warn("android device information not available")
            return

        for device_spec in android_devices:
            dev_info = self.report_msg.device_info.add()
            for elem in [
                    keys.ConfigKeys.IKEY_PRODUCT_TYPE,
                    keys.ConfigKeys.IKEY_PRODUCT_VARIANT,
                    keys.ConfigKeys.IKEY_BUILD_FLAVOR,
                    keys.ConfigKeys.IKEY_BUILD_ID, keys.ConfigKeys.IKEY_BRANCH,
                    keys.ConfigKeys.IKEY_BUILD_ALIAS,
                    keys.ConfigKeys.IKEY_API_LEVEL, keys.ConfigKeys.IKEY_SERIAL
            ]:
                if elem in device_spec:
                    setattr(dev_info, elem, str(device_spec[elem]))
            # TODO: get abi information differently for multi-device support.
            setattr(dev_info, keys.ConfigKeys.IKEY_ABI_NAME,
                    str(getattr(self, keys.ConfigKeys.IKEY_ABI_NAME)))
            setattr(dev_info, keys.ConfigKeys.IKEY_ABI_BITNESS,
                    str(getattr(self, keys.ConfigKeys.IKEY_ABI_BITNESS)))

    def SetTestResult(self, result=None):
        """Set the current test case result to the provided result.

        If None is provided as a result, the current test report will be cleared, which results
        in a silent skip.

        Requires the feature to be enabled; no-op otherwise.

        Args:
            result: ReportMsg.TestCaseResult, the result of the current test or None.
        """
        if not self.enabled:
            return

        if not result:
            self.report_msg.test_case.remove(self.current_test_report_msg)
            self.current_test_report_msg = None
        else:
            self.current_test_report_msg.test_result = result

    def AddTestReport(self, test_name):
        """Creates a report for the specified test.

        Requires the feature to be enabled; no-op otherwise.

        Args:
            test_name: String, the name of the test
        """
        if not self.enabled:
            return
        self.current_test_report_msg = self.report_msg.test_case.add()
        self.current_test_report_msg.name = test_name
        self.current_test_report_msg.start_timestamp = feature_utils.GetTimestamp(
        )

    def AddCoverageReport(self,
                          coverage_vec,
                          src_file_path,
                          git_project_name,
                          git_project_path,
                          revision,
                          covered_count,
                          line_count,
                          isGlobal=True):
        """Adds a coverage report to the VtsReportMessage.

        Processes the source information, git project information, and processed
        coverage information and stores it into a CoverageReportMessage within the
        report message.

        Args:
            coverage_vec: list, list of coverage counts (int) for each line
            src_file_path: the path to the original source file
            git_project_name: the name of the git project containing the source
            git_project_path: the path from the root to the git project
            revision: the commit hash identifying the source code that was used to
                      build a device image
            covered_count: int, number of lines covered
            line_count: int, total number of lines
            isGlobal: boolean, True if the coverage data is for the entire test, False if only for
                      the current test case.
        """
        if not self.enabled:
            return

        if isGlobal:
            report = self.report_msg
        else:
            report = self.current_test_report_msg

        coverage = report.coverage.add()
        coverage.total_line_count = line_count
        coverage.covered_line_count = covered_count
        coverage.line_coverage_vector.extend(coverage_vec)

        src_file_path = os.path.relpath(src_file_path, git_project_path)
        coverage.file_path = src_file_path
        coverage.revision = revision
        coverage.project_name = git_project_name

    def AddProfilingDataTimestamp(
            self,
            name,
            start_timestamp,
            end_timestamp,
            x_axis_label="Latency (nano secs)",
            y_axis_label="Frequency",
            regression_mode=ReportMsg.VTS_REGRESSION_MODE_INCREASING):
        """Adds the timestamp profiling data to the web DB.

        Requires the feature to be enabled; no-op otherwise.

        Args:
            name: string, profiling point name.
            start_timestamp: long, nanoseconds start time.
            end_timestamp: long, nanoseconds end time.
            x-axis_label: string, the x-axis label title for a graph plot.
            y-axis_label: string, the y-axis label title for a graph plot.
            regression_mode: specifies the direction of change which indicates
                             performance regression.
        """
        if not self.enabled:
            return

        if not hasattr(self, _PROFILING_POINTS):
            setattr(self, _PROFILING_POINTS, set())

        if name in getattr(self, _PROFILING_POINTS):
            logging.error("profiling point %s is already active.", name)
            return

        getattr(self, _PROFILING_POINTS).add(name)
        profiling_msg = self.report_msg.profiling.add()
        profiling_msg.name = name
        profiling_msg.type = ReportMsg.VTS_PROFILING_TYPE_TIMESTAMP
        profiling_msg.regression_mode = regression_mode
        profiling_msg.start_timestamp = start_timestamp
        profiling_msg.end_timestamp = end_timestamp
        profiling_msg.x_axis_label = x_axis_label
        profiling_msg.y_axis_label = y_axis_label

    def AddProfilingDataVector(
            self,
            name,
            labels,
            values,
            data_type,
            options=[],
            x_axis_label="x-axis",
            y_axis_label="y-axis",
            regression_mode=ReportMsg.VTS_REGRESSION_MODE_INCREASING):
        """Adds the vector profiling data in order to upload to the web DB.

        Requires the feature to be enabled; no-op otherwise.

        Args:
            name: string, profiling point name.
            labels: a list or set of labels.
            values: a list or set of values where each value is an integer.
            data_type: profiling data type.
            options: a set of options.
            x-axis_label: string, the x-axis label title for a graph plot.
            y-axis_label: string, the y-axis label title for a graph plot.
            regression_mode: specifies the direction of change which indicates
                             performance regression.
        """
        if not self.enabled:
            return

        if not hasattr(self, _PROFILING_POINTS):
            setattr(self, _PROFILING_POINTS, set())

        if name in getattr(self, _PROFILING_POINTS):
            logging.error("profiling point %s is already active.", name)
            return

        getattr(self, _PROFILING_POINTS).add(name)
        profiling_msg = self.report_msg.profiling.add()
        profiling_msg.name = name
        profiling_msg.type = data_type
        profiling_msg.regression_mode = regression_mode
        if labels:
            profiling_msg.label.extend(labels)
        profiling_msg.value.extend(values)
        profiling_msg.x_axis_label = x_axis_label
        profiling_msg.y_axis_label = y_axis_label
        profiling_msg.options.extend(options)

    def AddProfilingDataLabeledVector(
            self,
            name,
            labels,
            values,
            options=[],
            x_axis_label="x-axis",
            y_axis_label="y-axis",
            regression_mode=ReportMsg.VTS_REGRESSION_MODE_INCREASING):
        """Adds the labeled vector profiling data in order to upload to the web DB.

        Requires the feature to be enabled; no-op otherwise.

        Args:
            name: string, profiling point name.
            labels: a list or set of labels.
            values: a list or set of values where each value is an integer.
            options: a set of options.
            x-axis_label: string, the x-axis label title for a graph plot.
            y-axis_label: string, the y-axis label title for a graph plot.
            regression_mode: specifies the direction of change which indicates
                             performance regression.
        """
        self.AddProfilingDataVector(
            name, labels, values, ReportMsg.VTS_PROFILING_TYPE_LABELED_VECTOR,
            options, x_axis_label, y_axis_label, regression_mode)

    def AddProfilingDataUnlabeledVector(
            self,
            name,
            values,
            options=[],
            x_axis_label="x-axis",
            y_axis_label="y-axis",
            regression_mode=ReportMsg.VTS_REGRESSION_MODE_INCREASING):
        """Adds the unlabeled vector profiling data in order to upload to the web DB.

        Requires the feature to be enabled; no-op otherwise.

        Args:
            name: string, profiling point name.
            values: a list or set of values where each value is an integer.
            options: a set of options.
            x-axis_label: string, the x-axis label title for a graph plot.
            y-axis_label: string, the y-axis label title for a graph plot.
            regression_mode: specifies the direction of change which indicates
                             performance regression.
        """
        self.AddProfilingDataVector(
            name, None, values, ReportMsg.VTS_PROFILING_TYPE_UNLABELED_VECTOR,
            options, x_axis_label, y_axis_label, regression_mode)

    def AddSystraceUrl(self, url):
        """Creates a systrace report message with a systrace URL.

        Adds a systrace report to the current test case report and supplies the
        url to the systrace report.

        Requires the feature to be enabled; no-op otherwise.

        Args:
            url: String, the url of the systrace report.
        """
        if not self.enabled:
            return
        systrace_msg = self.current_test_report_msg.systrace.add()
        systrace_msg.url.append(url)

    def AddLogUrls(self, urls):
        """Creates a log message with log file URLs.

        Adds a log message to the current test module report and supplies the
        url to the log files.

        Requires the feature to be enabled; no-op otherwise.

        Args:
            urls: list of string, the URLs of the logs.
        """
        if not self.enabled or urls is None:
            return

        for url in urls:
            log_msg = self.report_msg.log.add()
            log_msg.url = url
            log_msg.name = os.path.basename(url)

    def GetTestModuleKeys(self):
        """Returns the test module name and start timestamp.

        Those two values can be used to find the corresponding entry
        in a used nosql database without having to lock all the data
        (which is infesiable) thus are essential for strong consistency.
        """
        return self.report_msg.test, self.report_msg.start_timestamp

    def GenerateReportMessage(self, requested, executed):
        """Uploads the result to the web service.

        Requires the feature to be enabled; no-op otherwise.

        Args:
            requested: list, A list of test case records requested to run
            executed: list, A list of test case records that were executed

        Returns:
            binary string, serialized report message.
            None if web is not enabled.
        """
        if not self.enabled:
            return None

        # Handle case when runner fails, tests aren't executed
        if (not getattr(self, keys.ConfigKeys.RUN_AS_VTS_SELFTEST, False)
            and executed and executed[-1].test_name == "setup_class"):
            # Test failed during setup, all tests were not executed
            start_index = 0
        else:
            # Runner was aborted. Remaining tests weren't executed
            start_index = len(executed)

        for test in requested[start_index:]:
            msg = self.report_msg.test_case.add()
            msg.name = test.test_name
            msg.start_timestamp = feature_utils.GetTimestamp()
            msg.end_timestamp = msg.start_timestamp
            msg.test_result = ReportMsg.TEST_CASE_RESULT_FAIL

        self.report_msg.end_timestamp = feature_utils.GetTimestamp()

        build = getattr(self, keys.ConfigKeys.IKEY_BUILD)
        if keys.ConfigKeys.IKEY_BUILD_ID in build:
            build_id = str(build[keys.ConfigKeys.IKEY_BUILD_ID])
            self.report_msg.build_info.id = build_id

        logging.info("_tearDownClass hook: start (username: %s)",
                     getpass.getuser())

        if len(self.report_msg.test_case) == 0:
            logging.info("_tearDownClass hook: skip uploading (no test case)")
            return ''

        post_msg = ReportMsg.DashboardPostMessage()
        post_msg.test_report.extend([self.report_msg])

        self.rest_client.AddAuthToken(post_msg)

        message_b = base64.b64encode(post_msg.SerializeToString())

        logging.info('Result proto message generated. size: %s',
                     len(message_b))

        logging.info("_tearDownClass hook: status upload time stamp %s",
                     str(self.report_msg.start_timestamp))

        return message_b
