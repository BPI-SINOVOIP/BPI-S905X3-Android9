# Copyright (C) 2016 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License"); you may not
# use this file except in compliance with the License. You may obtain a copy of
# the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
# License for the specific language governing permissions and limitations under
# the License.
import os
import time

from acts.test_utils.bt.BluetoothBaseTest import BluetoothBaseTest
from acts.libs.proto.proto_utils import compile_import_proto, parse_proto_to_ascii
from acts.test_utils.bt.bt_metrics_utils import get_bluetooth_metrics
from acts.utils import create_dir, dump_string_to_file


class BtMetricsBaseTest(BluetoothBaseTest):
    """
    Base class for tests that requires dumping and parsing Bluetooth Metrics
    """

    def __init__(self, controllers):
        BluetoothBaseTest.__init__(self, controllers)
        self.bluetooth_proto_path = None
        self.ad = self.android_devices[0]

    def setup_class(self):
        """
        This method finds bluetooth protobuf definitions from config file,
        compile the protobuf and create a log directory for metrics dumping
        :return: True on success, False on failure
        """
        super(BtMetricsBaseTest, self).setup_class()
        self.bluetooth_proto_path = self.user_params["bluetooth_proto_path"][0]
        if not os.path.isfile(self.bluetooth_proto_path):
            try:
                self.bluetooth_proto_path = "{}/bluetooth.proto".format(
                    os.path.dirname(os.path.realpath(__file__)))
            except Exception:
                self.log.error("File not found.")
            if not os.path.isfile(self.bluetooth_proto_path):
                self.log.error("Unable to find Bluetooth proto {}.".format(
                    self.bluetooth_proto_path))
                return False
        for ad in self.android_devices:
            ad.metrics_path = os.path.join(ad.log_path, "BluetoothMetrics")
            create_dir(ad.metrics_path)
            ad.bluetooth_proto_module = \
                compile_import_proto(ad.metrics_path, self.bluetooth_proto_path)
            if not ad.bluetooth_proto_module:
                self.log.error("Unable to compile bluetooth proto at " +
                               self.bluetooth_proto_path)
                return False
        return True

    def setup_test(self):
        """
        This method clears the current metrics, should be called after child
        class setup_test()
        :return: True
        """
        super(BtMetricsBaseTest, self).setup_test()
        # Clear all metrics
        for ad in self.android_devices:
            get_bluetooth_metrics(ad, ad.bluetooth_proto_module)
        return True

    def collect_bluetooth_manager_metrics_logs(self, ads):
        """
        Collect Bluetooth metrics logs, save an ascii log to disk and return
        both binary and ascii logs to caller
        :param ads: list of active Android devices
        :return: List of binary metrics logs,
                List of ascii metrics logs
        """
        bluetooth_logs = []
        bluetooth_logs_ascii = []
        for ad in ads:
            serial = ad.serial
            out_name = "{}_{}".format(serial, "bluetooth_metrics.txt")
            bluetooth_log = get_bluetooth_metrics(ad,
                                                  ad.bluetooth_proto_module)
            bluetooth_log_ascii = parse_proto_to_ascii(bluetooth_log)
            dump_string_to_file(bluetooth_log_ascii,
                                os.path.join(ad.metrics_path, out_name))
            bluetooth_logs.append(bluetooth_log)
            bluetooth_logs_ascii.append(bluetooth_log_ascii)
        return bluetooth_logs, bluetooth_logs_ascii
