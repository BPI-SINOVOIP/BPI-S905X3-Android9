# Copyright 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import json
import logging
import os

from autotest_lib.client.bin import test
from autotest_lib.client.bin import utils
from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib.cros import chrome
from autotest_lib.client.cros import cryptohome
from autotest_lib.client.cros.enterprise import enterprise_fake_dmserver
from autotest_lib.client.cros.power import power_status


class enterprise_PowerManagement(test.test):
    """Verify the power management policy setting."""
    version = 1

    def setup(self):
        """Standard autotest setup."""
        os.chdir(self.srcdir)
        utils.make('OUT_DIR=.')

    def initialize(self, percent_initial_charge_min=10):
        """
        Setup local variables and  init the fake DM server

        @param percent_initial_charge_min: Minimum percentage of battery
                                           required for the test to run.

        """
        # Username and password for the fake dm server can be anything
        # they are not used to authenticate against GAIA.
        self.username = 'fake-user@managedchrome.com'
        self.password = 'fakepassword'

        self._power_status = power_status.get_status()
        if not self._power_status.on_ac():
            # Ensure that the battery has some charge.
            self._power_status.assert_battery_state(percent_initial_charge_min)
        logging.info("Device power type is %s", self._power_type)

        self.fake_dm_server = enterprise_fake_dmserver.FakeDMServer(
                self.srcdir)
        self.fake_dm_server.start(self.tmpdir, self.debugdir)

    def cleanup(self):
        """Close out anything used by this test."""
        self.fake_dm_server.stop()

    @property
    def _power_type(self):
        """
        Returns appropriate power type based on whether DUT is on AC or not.

        @returns string of power type.

        """
        if self._power_status.on_ac():
            return "AC"

        return "Battery"

    def _setup_lock_policy(self):
        """Setup policy to lock screen in 10 seconds of idle time."""
        self._screen_lock_delay = 10
        screen_lock_policy = '{ "%s": %d }' % (
                self._power_type, self._screen_lock_delay*1000)
        policy_blob = """{
            "google/chromeos/user": {
                "mandatory": {
                    "ScreenLockDelays": %s
                }
            },
            "managed_users": [ "*" ],
            "policy_user": "%s",
            "current_key_index": 0,
            "invalidation_source": 16,
            "invalidation_name": "test_policy"
        }""" % (json.dumps(screen_lock_policy), self.username)

        self.fake_dm_server.setup_policy(policy_blob)

    def _setup_logout_policy(self):
        """Setup policy to logout in 10 seconds of idle time."""
        self._screen_logout_delay = 10
        idle_settings_policy = '''{
            "%s": {
                "Delays": {
                    "ScreenDim": 2000,
                    "ScreenOff": 3000,
                    "IdleWarning": 4000,
                    "Idle": %d
                 },
                 "IdleAction": "Logout"
            }
        }''' % (self._power_type, self._screen_logout_delay*1000)

        policy_blob = """{
            "google/chromeos/user": {
                "mandatory": {
                    "PowerManagementIdleSettings": %s
                }
            },
            "managed_users": [ "*" ],
            "policy_user": "%s",
            "current_key_index": 0,
            "invalidation_source": 16,
            "invalidation_name": "test_policy"
        }""" % (json.dumps(idle_settings_policy), self.username)

        self.fake_dm_server.setup_policy(policy_blob)

    def _create_chrome(self):
        """
        Create an instance of chrome.

        @returns a telemetry browser instance.

        """
        extra_browser_args = '--device-management-url=%s ' %(
                self.fake_dm_server.server_url)
        return chrome.Chrome(
                extra_browser_args=extra_browser_args,
                autotest_ext=True,
                disable_gaia_services=False,
                gaia_login=False,
                username=self.username,
                password=self.password,
                expect_policy_fetch=True)

    def run_once(self):
        """Run the power management policy tests."""
        self._setup_lock_policy()
        with self._create_chrome() as cr:
            utils.poll_for_condition(
                    lambda: cr.login_status['isScreenLocked'],
                            exception=error.TestFail('User is not locked'),
                            timeout=self._screen_lock_delay*2,
                            sleep_interval=1,
                            desc='Expects to find Chrome locked.')

        self._setup_logout_policy()
        with self._create_chrome() as cr:
            utils.poll_for_condition(
                    lambda: not cryptohome.is_vault_mounted(user=self.username,
                            allow_fail=True),
                            exception=error.TestFail('User is not logged out'),
                            timeout=self._screen_logout_delay*2,
                            sleep_interval=1,
                            desc='Expects to find user logged out.')
