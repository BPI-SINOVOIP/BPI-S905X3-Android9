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
import logging
import time
from google import protobuf

from acts import asserts
from acts.test_utils.bt.BtMetricsBaseTest import BtMetricsBaseTest
from acts.test_utils.bt.bt_test_utils import clear_bonded_devices
from acts.test_utils.bt.bt_test_utils import pair_pri_to_sec
from acts.test_utils.bt.bt_test_utils import reset_bluetooth
from acts.utils import get_current_epoch_time, sync_device_time


class BtMetricsTest(BtMetricsBaseTest):
    def __init__(self, controllers):
        BtMetricsBaseTest.__init__(self, controllers)
        self.iterations = 1

    def setup_class(self):
        return super(BtMetricsTest, self).setup_class()

    def setup_test(self):
        # Reset bluetooth
        reset_bluetooth(self.android_devices)
        for ad in self.android_devices:
            if not clear_bonded_devices(ad):
                logging.error("Failed to unbound device")
                return False
            # Sync device time for timestamp comparison
            sync_device_time(ad)
        return super(BtMetricsTest, self).setup_test()

    def test_pairing_metric(self):
        """Test if a pairing event generates the correct metric entry

        This test tries to pair two Bluetooth devices and dumps metrics after
        pairing. A correctly implemented stack should record 8 pairing events.

        Steps:
        1. Start pairing between two Bluetooth devices
        2. After pairing is done, dump and parse the metrics
        3. Compare the number of pairing events and the time stamp of the
        pairing event

        Expected Result:
        No errors, 8 pairing events should be generated
        Returns:
          Pass if True
          Fail if False

        TAGS: Classic
        Priority: 1
        """
        time_bonds = []
        for n in range(self.iterations):
            start_time = get_current_epoch_time()
            self.log.info("Pair bluetooth iteration {}.".format(n + 1))
            if (not pair_pri_to_sec(
                    self.android_devices[0],
                    self.android_devices[1],
                    attempts=1,
                    auto_confirm=False)):
                self.log.error("Failed to bond devices.")
                return False
            end_time = get_current_epoch_time()
            time_bonds.append((start_time, end_time))
            # A device bond will trigger a number of system routines that need
            # to settle before unbond
            time.sleep(2)
            for ad in self.android_devices:
                if not clear_bonded_devices(ad):
                    return False
                # Necessary sleep time for entries to update unbonded state
                time.sleep(2)
                bonded_devices = ad.droid.bluetoothGetBondedDevices()
                if len(bonded_devices) > 0:
                    self.log.error("Failed to unbond devices: {}".format(
                        bonded_devices))
                    return False
        end_time = get_current_epoch_time()
        bluetooth_logs, bluetooth_logs_ascii = \
            self.collect_bluetooth_manager_metrics_logs(
                [self.android_devices[0]])
        bluetooth_log = bluetooth_logs[0]
        bluetooth_log_ascii = bluetooth_logs_ascii[0]
        asserts.assert_equal(
            len(bluetooth_log.pair_event), 8, extras=bluetooth_log_ascii)
        for pair_event in bluetooth_log.pair_event:
            t = pair_event.event_time_millis
            asserts.assert_true(start_time <= t <= end_time,
                                "Event time %d not within limit [%d, %d]" %
                                (t, start_time, end_time))
            device_info = pair_event.device_paired_with
            asserts.assert_true(device_info, "Device info is none")
            asserts.assert_equal(device_info.device_type, self.android_devices[
                0].bluetooth_proto_module.DeviceInfo.DEVICE_TYPE_BREDR,
                                 "Device type does not match")

    def test_bluetooth_metrics_parsing(self):
        """Test if metrics could be dumped and parsed

        This test simply dumps Bluetooth metrics and print out the ASCII
        representation

        Steps:
        1. For the first Android device, dump metrics
        2. Parse and print metrics in INFO log using ASCII format

        Expected Result:
        No errors, metrics should be printed to INFO log

        Returns:
          Pass if True
          Fail if False

        TAGS: Classic
        Priority: 1
        """
        bluetooth_logs, bluetooth_logs_ascii = \
            self.collect_bluetooth_manager_metrics_logs(
                [self.android_devices[0]])
        bluetooth_log = bluetooth_logs[0]
        self.log.info(protobuf.text_format.MessageToString(bluetooth_log))
        return True
