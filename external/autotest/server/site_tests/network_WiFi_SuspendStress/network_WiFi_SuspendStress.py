# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import json
import logging
import time
import sys

from autotest_lib.client.bin import utils
from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib.cros.network import xmlrpc_datatypes
from autotest_lib.server.cros import stress
from autotest_lib.server.cros.network import wifi_cell_test_base
from autotest_lib.server.cros.multimedia import remote_facade_factory

_DELAY = 10
_START_TIMEOUT_SECONDS = 20


class network_WiFi_SuspendStress(wifi_cell_test_base.WiFiCellTestBase):
    """
    1. If the control file is control.integration, the test uses servo to
       repeatedly close & open lid while running BrowserTests.
    2. Any other control file , the test suspend and resume with powerd_dbus
       command and assert wifi connectivity to router.
    """
    version = 1


    def parse_additional_arguments(self, commandline_args, additional_params):
        """Hook into super class to take control files parameters.

        @param commandline_args dict of parsed parameters from the autotest.
        @param additional_params list of tuple(HostapConfig,
                                               AssociationParameters).
        """
        self._configurations = additional_params


    def logged_in(self):
        """Checks if the host has a logged in user.

        @return True if a user is logged in on the device.

        """
        try:
            out = self._host.run('cryptohome --action=status').stdout
        except:
            return False
        try:
            status = json.loads(out.strip())
        except ValueError:
            logging.info('Cryptohome did not return a value.')
            return False

        success = any((mount['mounted'] for mount in status['mounts']))
        if success:
            # Chrome needs a few moments to get ready, otherwise an immediate
            # suspend will power down the system.
            time.sleep(5)
        return success


    def check_servo_lid_open_support(self):
        """ Check if DUT supports servo lid_open/close. Ref:http://b/37436993"""
        try:
            self._host.servo.lid_close()
            self._host.servo.lid_open()
        except AssertionError:
            raise error.TestNAError('Error setting lid_open status. '
                                    'Skipping test run on this board. %s.'
                                    % sys.exc_info()[1])


    def stress_wifi_suspend(self, try_servo=True):
        """ Perform the suspend stress.

        @param try_servo:option to toogle use powerd vs servo_lid to suspend.
        """
        # servo might be taking time to come up; wait a bit
        if not self._host.servo and try_servo:
           time.sleep(10)

        boot_id = self._host.get_boot_id()
        if (not try_servo or
                self._host.servo.get('lid_open') == 'not_applicable'):
            self.context.client.do_suspend(_DELAY)
        else:
            self._host.servo.lid_close()
            self._host.wait_down(timeout=_DELAY)
            self._host.servo.lid_open()
            self._host.wait_up(timeout=_DELAY)

        self._host.test_wait_for_resume(boot_id)
        state_info = self.context.wait_for_connection(
            self.context.router.get_ssid())
        self._timings.append(state_info.time)


    def powerd_non_servo_wifi_suspend(self):
        """ Perform the suspend stress with powerdbus."""
        self.stress_wifi_suspend(try_servo=False)


    def run_once(self, suspends=5, integration=None):
        self._host = self.context.client.host

        for router_conf, client_conf in self._configurations:
            self.context.configure(configuration_parameters=router_conf)
            assoc_params = xmlrpc_datatypes.AssociationParameters(
                is_hidden=client_conf.is_hidden,
                security_config=client_conf.security_config,
                ssid=self.context.router.get_ssid())
            self.context.assert_connect_wifi(assoc_params)
            self._timings = list()

            if integration:
                if not self._host.servo:
                    raise error.TestNAError('Servo object returned None. '
                                            'Check if servo is missing or bad')

                # If the DUT is up and cold_reset is set to on, that means the
                # DUT does not support cold_reset. We can't run the test,
                # because it may get in a bad state and we won't be able
                # to recover.
                if self._host.servo.get('cold_reset') == 'on':
                    raise error.TestNAError('This DUT does not support '
                                            'cold reset, exiting')
                self.factory = remote_facade_factory.RemoteFacadeFactory(
                        self._host, no_chrome=True)
                logging.info('Running Wifi Suspend Stress Integration test.')
                browser_facade = self.factory.create_browser_facade()
                browser_facade.start_default_chrome()
                utils.poll_for_condition(condition=self.logged_in,
                                         timeout=_START_TIMEOUT_SECONDS,
                                         sleep_interval=1,
                                         desc='User not logged in.')
                self.check_servo_lid_open_support()
                stressor = stress.CountedStressor(self.stress_wifi_suspend)
            else:
                logging.info('Running Non integration Wifi Stress test.')
                stressor = stress.CountedStressor(
                        self.powerd_non_servo_wifi_suspend)

            stressor.start(suspends)
            stressor.wait()

            perf_dict = {'fastest': max(self._timings),
                         'slowest': min(self._timings),
                         'average': (float(sum(self._timings)) /
                                     len(self._timings))}
            for key in perf_dict:
                self.output_perf_value(description=key,
                    value=perf_dict[key],
                    units='seconds',
                    higher_is_better=False,
                    graph=router_conf.perf_loggable_description)


    def cleanup(self):
        """Cold reboot the device so the WiFi card is back in a good state."""
        if self._host.servo and self._host.servo.get('cold_reset') == 'off':
            self._host.servo.get_power_state_controller().reset()
