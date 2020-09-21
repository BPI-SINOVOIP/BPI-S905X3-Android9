# Copyright 2016 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging

from autotest_lib.client.common_lib import error
from autotest_lib.server import autotest
from autotest_lib.server import test


class component_UpdateFlash(test.test):
    """Downloads, installs, and verifies component updated flash."""
    version = 1

    def _run_client_test(self, name, CU_action):
        """Runs client test."""
        logging.info('+++++ component_UpdateFlash +++++')
        logging.info('Performing %s', CU_action)
        try:
            self.autotest_client.run_test(name,
                                          CU_action=CU_action,
                                          tag=CU_action,
                                          check_client_result=True)
        except:
            raise error.TestFail('Failed: %s.' % CU_action)

    def _run_flash_sanity(self):
        """Verify that simple Flash site runs."""
        self._run_client_test('desktopui_FlashSanityCheck', CU_action='sanity')

    def _delete_component(self):
        """Deletes any installed Flash component binaries."""
        self._run_client_test(
            'desktopui_FlashSanityCheck', CU_action='delete-component')
        # Unfortunately deleting the component requires a reboot to get rid of
        # the mount at /run/imageloader/PepperFlashPlayer.
        self._reboot()

    def _download_omaha_component(self):
        """Forces the download of components from Omaha server."""
        self._run_client_test(
            'desktopui_FlashSanityCheck', CU_action='download-omaha-component')

    def _verify_component_flash(self):
        """Runs Flash and verifies it uses component binaries."""
        self._run_client_test(
            'desktopui_FlashSanityCheck', CU_action='verify-component-flash')

    def _verify_system_flash(self):
        """Runs Flash and verifies it uses system binaries."""
        self._run_client_test(
            'desktopui_FlashSanityCheck', CU_action='verify-system-flash')

    def _verify_clean_shutdown(self):
        """Verifies that the stateful partition was cleaned up on shutdown."""
        try:
            self.autotest_client.run_test('platform_CleanShutdown',
                                          check_client_result=True)
        except:
            raise error.TestFail('Failed: platform_CleanShutdown')

    def _reboot(self):
        """Reboot the DUT."""
        self.host.reboot()

    def cleanup(self):
        """Leaves this test without a component Flash installed."""
        logging.info('+++++ Cleaning DUT for other tests +++++')
        self._delete_component()

    def run_once(self, host):
        """Runs Flash component update test."""
        self.host = host
        self.autotest_client = autotest.Autotest(self.host)
        # Paranoia: by rebooting the DUT we protect this test from other tests
        # leaving running webservers behind.
        logging.info('+++++ Rebooting DUT on start to reduce flakes +++++')
        self._reboot()
        # Test Flash once with whatever was the default on the DUT.
        self._run_flash_sanity()
        # Start with a clean slate.
        self._delete_component()
        self._verify_system_flash()
        # Force a download.
        self._download_omaha_component()
        # Currently mounting the component binaries requires a reboot.
        self._reboot()
        self._verify_component_flash()
        # Reboot again to see if a clean unmount happens.
        self._reboot()
        self._verify_clean_shutdown()
