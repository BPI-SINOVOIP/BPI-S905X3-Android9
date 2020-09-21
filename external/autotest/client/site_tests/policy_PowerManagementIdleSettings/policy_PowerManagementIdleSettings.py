# Copyright 2016 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import time

from autotest_lib.client.bin import utils
from autotest_lib.client.common_lib import error
from autotest_lib.client.cros import cryptohome
from autotest_lib.client.cros.enterprise import enterprise_policy_base
from autotest_lib.client.cros.graphics import graphics_utils
from autotest_lib.client.cros.power import power_status, power_utils


class policy_PowerManagementIdleSettings(
          enterprise_policy_base.EnterprisePolicyTest):
    """
    Test effect of PowerManagementIdleSettings policy on Chrome OS behavior.

    This test verifies the effect of the PowerManagementIdleSettings user
    policy on specific Chrome OS client behaviors. It tests two valid values
    for the IdleAction property: 'DoNothing' and Not set, with three test
    cases: DoNothing_Continue, NotSet_Sleep, and Logout_EndSession. It also
    verifies that the screen dims after ScreenDim delay, and then turns off
    after ScreenOff delay (both delays in milliseconds).

    Note: Valid IdleAction values are 'DoNothing', 'Suspend', 'Logout', and
    'Shutdown'. This test exercises only the DoNothing and Logout actions.
    Suspend is tested by enterprise_PowerManager.py. Shutdown can be tested
    only using a Server-side AutoTest.

    Chrome reports user activity to the power manager at most every 5 seconds.
    To accomodate potential delays, the test pads the idle-action delay with
    a 5 second activity report interval.

    Several supporting policies are necessary to facillitate testing, or to
    make testing more reliable. These policies are listed below with a brief
    description of the set value.
    - WaitForInitialUserActivity=False so idle timer starts immediately after
      session starts.
    - UserActivityScreenDimDelayScale=100 to prevent increase delays when
      user activity occurs after screen dim.
    - ChromeOsLockOnIdleSuspend=False to prevent screen lock upon suspend.
    - AllowScreenLock=False to prevent manual screen lock. Will not affect
      this test, but is the safest setting.
    - AllowScreenWakeLocks=False to ignore 'keep awake' requests. Since wake
      locks are not requested during this test, ignoring them is unnecessary.
      But for safety we ignore them when testing suspend.
    - LidCloseAction=3 to invoke no action upon (accidental) lid closure.
    - ResoreOnStartup* polices are set to display the settings and policy
      pages. This is useful when debugging failures.

    """
    version = 1

    def initialize(self, **kwargs):
        """Set up local variables and ensure device is on AC power."""
        self._initialize_test_constants()
        self._power_status = power_status.get_status()
        if not self._power_status.on_ac():
            raise error.TestNAError('Test must be run with DUT on AC power.')
        self._backlight = power_utils.Backlight()
        super(policy_PowerManagementIdleSettings, self).initialize(**kwargs)

    def _initialize_test_constants(self):
        self.POLICY_NAME = 'PowerManagementIdleSettings'
        self.SCREEN_SETTLE_TIME = 0.3
        self.SCREEN_DIM_DELAY = 4
        self.IDLE_WARNING_DELAY = 6
        self.SCREEN_OFF_DELAY = 8
        self.IDLE_ACTION_DELAY = 10
        self.ACTIVITY_REPORT_INTERVAL = 5
        self.IDLE_ACTION_NOTSET = {
            'AC': {
                'Delays': {
                    'ScreenDim': (self.SCREEN_DIM_DELAY * 1000),
                    'IdleWarning': (self.IDLE_WARNING_DELAY * 1000),
                    'ScreenOff': (self.SCREEN_OFF_DELAY * 1000),
                    'Idle': (self.IDLE_ACTION_DELAY * 1000)
                }
            }
        }
        self.IDLE_ACTION_DONOTHING = {
            'AC': {
                'Delays': {
                    'ScreenDim': (self.SCREEN_DIM_DELAY * 1000),
                    'IdleWarning': (self.IDLE_WARNING_DELAY * 1000),
                    'ScreenOff': (self.SCREEN_OFF_DELAY * 1000),
                    'Idle': (self.IDLE_ACTION_DELAY * 1000)
                },
                'IdleAction': 'DoNothing'
            }
        }
        self.IDLE_ACTION_LOGOUT = {
            'AC': {
                'Delays': {
                    'ScreenDim': (self.SCREEN_DIM_DELAY * 1000),
                    'IdleWarning': (self.IDLE_WARNING_DELAY * 1000),
                    'ScreenOff': (self.SCREEN_OFF_DELAY * 1000),
                    'Idle': (self.IDLE_ACTION_DELAY * 1000)
                },
                'IdleAction': 'Logout'
            }
        }
        self.TEST_CASES = {
            'NotSet_Sleep': self.IDLE_ACTION_NOTSET,
            'DoNothing_Continue': self.IDLE_ACTION_DONOTHING,
            'Logout_EndSession': self.IDLE_ACTION_LOGOUT
        }
        self.STARTUP_URLS = ['chrome://settings', 'chrome://policy']
        self.SUPPORTING_POLICIES = {
            'WaitForInitialUserActivity': True,
            'UserActivityScreenDimDelayScale': 100,
            'ChromeOsLockOnIdleSuspend': False,
            'AllowScreenLock': False,
            'AllowScreenWakeLocks': False,
            'LidCloseAction': 3,
            'RestoreOnStartup': 4,
            'RestoreOnStartupURLs': self.STARTUP_URLS
        }


    def elapsed_time(self, start_time):
        """Get time elapsed since |start_time|.

        @param start_time: clock time from which elapsed time is measured.
        @returns time elapsed since the start time.
        """
        return time.time() - start_time


    def _simulate_user_activity(self):
        """Inject user activity via D-bus to restart idle timer.

        Note that if the screen has gone black, these use activities will
        wake up the display again. However, they will not wake up a screen
        that has merely been dimmed.

        """
        graphics_utils.click_mouse()  # Note: Duration is 0.4 seconds.
        graphics_utils.press_keys(['KEY_LEFTCTRL'])


    def _wait_for_login_status(self, attribute, value, timeout):
        """Return when attribute has value, or its current value on timeout.

        Login_status is a dictionary of attributes that describe the login
        status of the current session. It contains values for the following
        attributes: isLoggedIn, isRegularUser, isOwner, isKiosk, isGuest,
        isScreenLocked, userImage, email, and displayEmail.

        @param attribute: String attribute key to be measured.
        @param value: Boolean attribute value expected.
        @param timeout: integer seconds till timeout.
        @returns dict of login status.

        """
        attribute_value = utils.wait_for_value(
            lambda: self.cr.login_status[attribute],
            expected_value=value,
            timeout_sec=timeout)
        return attribute_value


    def _poll_until_user_is_logged_out(self, timeout):
        """Return True when user logs out, False when user remains logged in.

        @returns boolean of user logged out status.

        """
        my_result = utils.poll_for_condition(
            lambda: not cryptohome.is_vault_mounted(user=self.username,
                                                    allow_fail=True),
            exception=None,
            timeout=timeout,
            sleep_interval=2,
            desc='Polling for user to be logged out.')
        return my_result


    def _set_brightness_to_maximum(self):
        """Set screen to maximum brightness."""
        max_level = self._backlight.get_max_level()
        self._backlight.set_level(max_level)


    def _wait_for_brightness_change(self, timeout):
        """Return screen brightness on update, or current value on timeout.

        @returns float of screen brightness percentage.

        """
        initial_brightness = self._backlight.get_percent()
        current_brightness = utils.wait_for_value_changed(
            lambda: self._backlight.get_percent(),
            old_value=initial_brightness,
            timeout_sec=timeout)
        if current_brightness != initial_brightness:
            time.sleep(self.SCREEN_SETTLE_TIME)
            current_brightness = self._backlight.get_percent()
        return current_brightness


    def _test_idle_action(self, policy_value):
        """
        Verify CrOS enforces PowerManagementIdleSettings policy value.

        @param policy_value: policy value for this case.
        @raises: TestFail if idle actions are not performed after their
                 specified delays.

        """
        logging.info('Running _test_idle_action(%s)', policy_value)

        # Wait until UI settles down with user logged in.
        user_is_logged_in = self._wait_for_login_status(
            'isLoggedIn', True, self.IDLE_ACTION_DELAY)
        if not user_is_logged_in:
            raise error.TestFail('User must be logged in at start.')

        # Set screen to maxiumum brightness.
        self._set_brightness_to_maximum()
        max_brightness = self._backlight.get_percent()
        logging.info('Brightness maximized to: %.2f', max_brightness)

        # Induce user activity to start idle timer.
        self._simulate_user_activity()
        start_time = time.time()

        # Verify screen is dimmed after expected delay.
        seconds_to_dim = (
            self.SCREEN_DIM_DELAY - self.elapsed_time(start_time))
        dim_brightness = self._wait_for_brightness_change(seconds_to_dim)
        dim_elapsed_time = self.elapsed_time(start_time)
        logging.info('  Brightness dimmed to: %.2f, ', dim_brightness)
        logging.info('  after %s seconds.', dim_elapsed_time)
        if not (dim_brightness < max_brightness and dim_brightness > 0.0):
            raise error.TestFail('Screen did not dim on delay.')

        # Verify screen is turned off after expected delay.
        seconds_to_off = (
            self.SCREEN_OFF_DELAY - self.elapsed_time(start_time))
        off_brightness = self._wait_for_brightness_change(seconds_to_off)
        off_elapsed_time = self.elapsed_time(start_time)
        logging.info('  Brightness off to: %.2f, ', off_brightness)
        logging.info('  after %s seconds.', off_elapsed_time)
        if not off_brightness < dim_brightness:
            raise error.TestFail('Screen did not turn off on delay.')

        # Verify user is still logged in before IdleAction.
        user_is_logged_in = self.cr.login_status['isLoggedIn']
        if not user_is_logged_in:
            raise error.TestFail('User must be logged in before idle action.')

        # Get user logged in state after IdleAction.
        seconds_to_action = (
            self.IDLE_ACTION_DELAY + self.ACTIVITY_REPORT_INTERVAL
            - self.elapsed_time(start_time))
        try:
            user_is_logged_in = not self._poll_until_user_is_logged_out(
                seconds_to_action)
        except utils.TimeoutError:
            pass
        action_elapsed_time = self.elapsed_time(start_time)
        logging.info('  User logged out: %r, ', not user_is_logged_in)
        logging.info('  after %s seconds.', action_elapsed_time)

        # Verify user status against expected result, based on case.
        if self.case == 'NotSet_Sleep' or self.case == 'DoNothing_Continue':
            if not user_is_logged_in:
                raise error.TestFail('User should be logged in.')
        elif self.case == 'Logout_EndSession':
            if user_is_logged_in:
                raise error.TestFail('User should be logged out.')


    def run_once(self, case):
        """
        Setup and run the test configured for the specified test case.

        @param case: Name of the test case to run.

        """
        case_value = self.TEST_CASES[case]
        self.SUPPORTING_POLICIES[self.POLICY_NAME] = case_value
        self.setup_case(user_policies=self.SUPPORTING_POLICIES)
        self._test_idle_action(case_value)
