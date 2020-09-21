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

import logging
import os

from vts.proto import VtsReportMessage_pb2 as ReportMsg
from vts.runners.host import keys
from vts.utils.python.systrace import systrace_controller
from vts.utils.python.web import feature_utils

_SYSTRACE_CONTROLLER = "systrace_controller"

class SystraceFeature(feature_utils.Feature):
    """Feature object for systrace functionality.

    Attributes:
        enabled: boolean, True if systrace is enabled, False otherwise
        web: (optional) WebFeature, object storing web feature util for test run
    """

    _TOGGLE_PARAM = keys.ConfigKeys.IKEY_ENABLE_SYSTRACE
    _REQUIRED_PARAMS = [
            keys.ConfigKeys.KEY_TESTBED_NAME,
            keys.ConfigKeys.IKEY_ANDROID_DEVICE,
            keys.ConfigKeys.IKEY_DATA_FILE_PATH,
            keys.ConfigKeys.IKEY_SYSTRACE_REPORT_PATH,
            keys.ConfigKeys.IKEY_SYSTRACE_REPORT_URL_PREFIX
        ]
    _OPTIONAL_PARAMS = [keys.ConfigKeys.IKEY_SYSTRACE_PROCESS_NAME]

    def __init__(self, user_params, web=None):
        """Initializes the systrace feature.

        Args:
            user_params: A dictionary from parameter name (String) to parameter value.
            web: (optional) WebFeature, object storing web feature util for test run
        """
        self.ParseParameters(self._TOGGLE_PARAM, self._REQUIRED_PARAMS, self._OPTIONAL_PARAMS,
                             user_params)
        self.web = web
        logging.info("Systrace enabled: %s", self.enabled)

    def StartSystrace(self):
        """Initialize systrace controller if enabled.

        Requires the feature to be enabled; no-op otherwise.
        """
        if not self.enabled:
            return

        process_name = getattr(self, keys.ConfigKeys.IKEY_SYSTRACE_PROCESS_NAME, '')
        process_name = str(process_name)
        data_file_path = getattr(self, keys.ConfigKeys.IKEY_DATA_FILE_PATH)

        # TODO: handle device_serial for multi-device
        android_devices = getattr(self, keys.ConfigKeys.IKEY_ANDROID_DEVICE)
        if not isinstance(android_devices, list):
            logging.warn("android device information not available")
            return
        device_spec = android_devices[0]
        serial = device_spec.get(keys.ConfigKeys.IKEY_SERIAL)
        if not serial:
            logging.error("Serial for device at index 0 is not available.")
            self.enabled = False
        serial = str(serial)

        android_vts_path = os.path.normpath(os.path.join(data_file_path, '..'))
        self.controller = systrace_controller.SystraceController(
            android_vts_path,
            device_serial=serial,
            process_name=process_name)
        self.controller.Start()

    def ProcessAndUploadSystrace(self, test_name):
        """Stops and outputs the systrace data to configured path.

        Requires the feature to be enabled; no-op otherwise.

        Args:
            test_name: String, the name of the test
        """
        if not self.enabled:
            return

        controller = getattr(self, "controller", None)
        if not controller:
            logging.info("ProcessSystrace: missing systrace controller")
            return

        controller.Stop()

        if not controller.has_output:
            logging.info("ProcessSystrace: systrace controller has no output")
            return

        try:
            test_module_name = getattr(self, keys.ConfigKeys.KEY_TESTBED_NAME)
            process = controller.process_name
            time = feature_utils.GetTimestamp()
            report_path = getattr(self, keys.ConfigKeys.IKEY_SYSTRACE_REPORT_PATH)
            report_destination_file_name = '{module}_{test}_{process}_{time}.html'.format(
                module=test_module_name,
                test=test_name,
                process=process,
                time=time)
            report_destination_file_path = os.path.join(report_path, report_destination_file_name)
            if controller.SaveLastOutput(report_destination_file_path):
                logging.info('Systrace output saved to %s', report_destination_file_path)
            else:
                logging.error('Failed to save systrace output.')

            report_url_prefix = getattr(self, keys.ConfigKeys.IKEY_SYSTRACE_REPORT_URL_PREFIX)
            report_url_prefix = str(report_url_prefix)
            report_destination_file_url = '%s%s' % (report_url_prefix, report_destination_file_name)

            if self.web and self.web.enabled:
                self.web.AddSystraceUrl(report_destination_file_url)
            logging.info(
                'systrace result url %s .', report_destination_file_url)
        except Exception as e:  # TODO(yuexima): more specific exceptions catch
            logging.exception('Failed to add systrace to report message %s', e)
        finally:
            if not controller.ClearLastOutput():
                logging.error('failed to clear last systrace output.')
