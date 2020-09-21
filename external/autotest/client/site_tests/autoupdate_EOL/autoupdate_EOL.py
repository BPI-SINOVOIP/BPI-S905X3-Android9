# Copyright 2018 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
import logging
import shutil

from autotest_lib.client.bin import test, utils
from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib.cros import chrome
from autotest_lib.client.cros.update_engine import nano_omaha_devserver

class autoupdate_EOL(test.test):
    """Tests end of life (EOL) behaviour."""
    version = 1

    _EXPECTED_EOL_STATUS = 'EOL_STATUS=eol'
    _EOL_NOTIFICATION_TITLE = 'This device is no longer supported'

    def cleanup(self):
        shutil.copy('/var/log/update_engine.log', self.resultsdir)
        self._omaha.stop()


    def _check_eol_status(self):
        """Checks update_engines eol status."""
        result = utils.run('update_engine_client --eol_status').stdout.strip()
        if result != self._EXPECTED_EOL_STATUS:
            raise error.TestFail('Expected status %s. Actual: %s' %
                                 (self._EXPECTED_EOL_STATUS, result))


    def _check_eol_notification(self):
        """Checks that we are showing an EOL notification to the user."""
        with chrome.Chrome(autotest_ext=True, logged_in=True) as cr:
            def find_notification():
                notifications = cr.get_visible_notifications()
                if notifications is None:
                    return False
                else:
                    logging.debug(notifications)
                    for n in notifications:
                        if n['title'] == self._EOL_NOTIFICATION_TITLE:
                            return True

            utils.poll_for_condition(condition=lambda: find_notification(),
                                     desc='Notification is found',
                                     timeout=5,
                                     sleep_interval=1)


    def run_once(self):
        utils.run('restart update-engine')

        # Start a devserver to return a response with eol entry.
        self._omaha = nano_omaha_devserver.NanoOmahaDevserver(eol=True)
        self._omaha.start()

        # Try to update using the omaha server. It will fail with noupdate.
        utils.run('update_engine_client -update -omaha_url=' +
                  'http://127.0.0.1:%d/update ' % self._omaha.get_port(),
                  ignore_status=True)

        self._check_eol_status()
        self._check_eol_notification()
