# Copyright 2016 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Server side bluetooth tests on adapter standalone working states."""

import logging

from autotest_lib.client.common_lib import error
from autotest_lib.server.cros.bluetooth.bluetooth_adapter_tests import (
        BluetoothAdapterTests)
from autotest_lib.server.cros.multimedia import remote_facade_factory


class bluetooth_AdapterStandalone(BluetoothAdapterTests):
    """Server side bluetooth adapter standalone tests.

    This test tries to thoroughly verify most of the important work
    states of a standalone bluetooth adapter prior to connecting to
    other bluetooth peripherals.

    In particular, the following subtests are performed. Look at the
    docstrings of the subtests for more details.
    - test_start_bluetoothd
    - test_stop_bluetoothd
    - test_adapter_work_state
    - test_power_on_adapter
    - test_power_off_adapter
    - test_reset_on_adapter
    - test_reset_off_adapter
    - test_UUIDs
    - test_start_discovery
    - test_stop_discovery
    - test_discoverable
    - test_nondiscoverable
    - test_pairable
    - test_nonpairable

    Refer to BluetoothAdapterTests for all subtests performed in this test.

    """

    def run_once(self, host, repeat_count=1):
        """Running Bluetooth adapter standalone tests.

        @param host: device under test host
        @param repeat_count: the number of times to repeat the tests.

        """
        self.host = host
        factory = remote_facade_factory.RemoteFacadeFactory(host)
        self.bluetooth_facade = factory.create_bluetooth_hid_facade()

        for i in xrange(1, repeat_count + 1):
            logging.info('repeat count: %d / %d', i, repeat_count)

            # The bluetoothd must be running in the beginning.
            self.test_bluetoothd_running()

            # It is possible that the adapter is not powered on yet.
            # Power it on anyway and verify that it is actually powered on.
            self.test_power_on_adapter()

            # Verify that the adapter could be powered off and powered on,
            # and that it is in the correct working state.
            self.test_power_off_adapter()
            self.test_power_on_adapter()
            self.test_adapter_work_state()

            # Verify that the bluetoothd could be simply stopped and then
            # be started again, and that it is in the correct working state.
            self.test_stop_bluetoothd()
            self.test_start_bluetoothd()
            self.test_adapter_work_state()

            # Verify that the adapter could be reset off and on which includes
            # removing all cached information.
            self.test_reset_off_adapter()
            self.test_reset_on_adapter()
            self.test_adapter_work_state()

            # Verify that the adapter supports basic profiles.
            self.test_UUIDs()

            # Verify that the adapter could start and stop discovery.
            self.test_start_discovery()
            self.test_stop_discovery()
            self.test_start_discovery()

            # Verify that the adapter could set both discoverable and
            # non-discoverable successfully.
            self.test_discoverable()
            self.test_nondiscoverable()
            self.test_discoverable()

            # Verify that the adapter could set pairable and non-pairable.
            self.test_pairable()
            self.test_nonpairable()
            self.test_pairable()

            if self.fails:
                raise error.TestFail(self.fails)
