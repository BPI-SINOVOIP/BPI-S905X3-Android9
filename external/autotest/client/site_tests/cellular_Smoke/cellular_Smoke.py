# Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import time
import urlparse

from autotest_lib.client.bin import test
from autotest_lib.client.common_lib import error
from autotest_lib.client.cros import network
from autotest_lib.client.cros.networking import shill_context
from autotest_lib.client.cros.networking import shill_proxy


# Default timeouts in seconds
CONNECT_TIMEOUT = 120
DISCONNECT_TIMEOUT = 60

class cellular_Smoke(test.test):
    """
    Tests that 3G modem can connect to the network

    The test attempts to connect using the 3G network. The test then
    disconnects from the network, and verifies that the modem still
    responds to modem manager DBUS API calls.  It repeats the
    connect/disconnect sequence several times.

    """
    version = 1


    def run_once_internal(self):
        """
        Executes the test.

        """
        old_modem_info = self.test_env.modem.GetModemProperties()

        for _ in xrange(self.connect_count):
            device = self.test_env.shill.find_cellular_device_object()
            if not device:
                raise error.TestError('No cellular device found.')

            service = self.test_env.shill.wait_for_cellular_service_object()
            if not service:
                raise error.TestError('No cellular service found.')

            logging.info('Connecting to service %s', service.object_path)
            self.test_env.shill.connect_service_synchronous(
                    service, CONNECT_TIMEOUT)

            state = self.test_env.shill.get_dbus_property(
                    service, shill_proxy.ShillProxy.SERVICE_PROPERTY_STATE)
            logging.info('Service state = %s', state)

            if state == 'portal':
                url_pattern = ('https://quickaccess.verizonwireless.com/'
                               'images_b2c/shared/nav/'
                               'vz_logo_quickaccess.jpg?foo=%d')
                bytes_to_fetch = 4476
            else:
                url_pattern = network.FETCH_URL_PATTERN_FOR_TEST
                bytes_to_fetch = 64 * 1024

            interface = self.test_env.shill.get_dbus_property(
                    device, shill_proxy.ShillProxy.DEVICE_PROPERTY_INTERFACE)
            logging.info('Expected interface for %s: %s',
                         service.object_path, interface)
            network.CheckInterfaceForDestination(
                urlparse.urlparse(url_pattern).hostname,
                interface)

            fetch_time = network.FetchUrl(url_pattern, bytes_to_fetch,
                                          self.fetch_timeout)
            self.write_perf_keyval({
                'seconds_3G_fetch_time': fetch_time,
                'bytes_3G_bytes_received': bytes_to_fetch,
                'bits_second_3G_speed': 8 * bytes_to_fetch / fetch_time
            })

            self.test_env.shill.disconnect_service_synchronous(
                    service, DISCONNECT_TIMEOUT)

            # Verify that we can still get information about the modem
            logging.info('Old modem info: %s', ', '.join(old_modem_info))
            new_modem_info = self.test_env.modem.GetModemProperties()
            if len(new_modem_info) != len(old_modem_info):
                logging.info('New modem info: %s', ', '.join(new_modem_info))
                raise error.TestFail('Test shutdown: '
                                     'failed to leave modem in working state.')

            if self.sleep_kludge:
                logging.info('Sleeping for %.1f seconds', self.sleep_kludge)
                time.sleep(self.sleep_kludge)


    def run_once(self, test_env, connect_count=5, sleep_kludge=5,
                 fetch_timeout=120):
        with test_env, shill_context.ServiceAutoConnectContext(
                test_env.shill.find_cellular_service_object, False):
            self.test_env = test_env
            self.connect_count = connect_count
            self.sleep_kludge = sleep_kludge
            self.fetch_timeout = fetch_timeout

            self.run_once_internal()
