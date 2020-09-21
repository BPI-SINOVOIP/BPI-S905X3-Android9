# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import os
import time

from autotest_lib.server import test
from autotest_lib.client.common_lib.cros import tpm_utils
from autotest_lib.server.cros.multimedia import remote_facade_factory

SHORT_TIMEOUT = 5


class CfmBaseTest(test.test):
    """
    Base class for Cfm enterprise tests.

    CfmBaseTest provides common setup and cleanup methods. This base class is
    agnostic with respect to 'hangouts classic' vs. 'hangouts meet' - it works
    for both flavors.
    """

    def initialize(self, host, run_test_only=False, skip_enrollment=False):
        """
        Initializes common test properties.

        @param host: a host object representing the DUT.
        @param run_test_only: Wheter to run only the test or to also perform
            deprovisioning, enrollment and system reboot. If set to 'True',
            the DUT must already be enrolled and past the OOB screen to be able
            to execute the test.
        @param skip_enrollment: Whether to skip the enrollment step. Cleanup
            at the end of the test is done regardless.
        """
        super(CfmBaseTest, self).initialize()
        self._host = host
        self._run_test_only = run_test_only
        self._skip_enrollment = skip_enrollment
        self._facade_factory = remote_facade_factory.RemoteFacadeFactory(
            self._host, no_chrome = True)
        self.cfm_facade = self._facade_factory.create_cfm_facade()

    def setup(self):
        """
        Performs common test setup operations:
          - clears the TPM
          - sets up servo
          - enrolls the device
          - skips OOBE
        """
        super(CfmBaseTest, self).setup()
        if self._host.servo:
            self._setup_servo()

        if self._run_test_only or self._skip_enrollment:
            # We need to restart the browser to obtain the handle for it when
            # running in test_only mode.
            self.cfm_facade.restart_chrome_for_cfm()
        else:
            tpm_utils.ClearTPMOwnerRequest(self._host)
            self.cfm_facade.enroll_device()
            self.cfm_facade.skip_oobe_after_enrollment()

    def _setup_servo(self):
        """
        Enables the USB port such that any peripheral connected to it is visible
        to the DUT.
        """
        self._host.servo.switch_usbkey('dut')
        self._host.servo.set('usb_mux_sel3', 'dut_sees_usbkey')
        time.sleep(SHORT_TIMEOUT)
        self._host.servo.set('dut_hub1_rst1', 'off')
        time.sleep(SHORT_TIMEOUT)

    def cleanup(self, run_test_only=False):
        """Takes a screenshot, saves log files and clears the TPM."""
        self.take_screenshot('%s' % self.tagged_testname)
        self.save_callgrok_logs()
        self.save_packaged_app_logs()
        if not self._run_test_only:
            tpm_utils.ClearTPMOwnerRequest(self._host)
        super(CfmBaseTest, self).cleanup()

    def take_screenshot(self, screenshot_name):
        """
        Takes a screenshot (in .png format) and saves it in the debug dir.

        @param screenshot_name: Name of the screenshot file without extension.
        """
        try:
            target_dir = self.debugdir
            logging.info('Taking screenshot and saving under %s...',
                         target_dir)
            remote_path = self.cfm_facade.take_screenshot()
            if remote_path:
                # Copy the screenshot from the DUT.
                self._safe_copy_file(
                    remote_path,
                    os.path.join(target_dir, screenshot_name + '.png'))
            else:
                logging.warning('Taking screenshot failed')
        except Exception as e:
            logging.exception('Exception while taking a screenshot.')

    def save_callgrok_logs(self):
        """
        Copies the callgrok logs from the client to test's debug directory.
        """
        callgrok_log_path = self.cfm_facade.get_latest_callgrok_file_path()
        if callgrok_log_path:
            self._safe_copy_file(
                callgrok_log_path,
                os.path.join(self.debugdir, 'callgrok_logs.txt'))
        else:
            logging.warning('No callgrok logs found on DUT.')

    def save_packaged_app_logs(self):
        """
        Copies the packaged app logs from the client to test's debug directory.
        """
        pa_log_path = self.cfm_facade.get_latest_pa_logs_file_path()
        if pa_log_path:
            self._safe_copy_file(
                pa_log_path,
                os.path.join(self.debugdir, 'packaged_app_logs.txt'))
        else:
            logging.warning('No packaged app logs found on DUT.')

    def _safe_copy_file(self, remote_path, local_path):
        """
        Copies the packaged app log file from CFM to test's debug directory.
        """
        try:
            logging.info('Copying file "%s" from client to "%s"...',
                         remote_path, local_path)
            self._host.get_file(remote_path, local_path)
        except Exception as e:
            logging.exception(
                'Exception while copying file "%s"', remote_path)

