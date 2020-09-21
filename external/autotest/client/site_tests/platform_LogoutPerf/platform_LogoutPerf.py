# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import tempfile

from autotest_lib.client.bin import utils
from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib import file_utils
from autotest_lib.client.common_lib.cros import arc, arc_util
from autotest_lib.client.common_lib.cros import chrome
from autotest_lib.client.cros import cryptohome
from autotest_lib.client.cros.input_playback import keyboard


_CHROME_EXEC_TIME = 'chrome-exec'
_LOGIN_TIME = 'login-prompt-visible'
_LOGOUT_STARTED_TIME = 'logout-started'
_LOGOUT_TIMEOUT = 10  # logout should finsih in 10 seconds.


class platform_LogoutPerf(arc.ArcTest):
    """Measured the time for signing off from a logged on user session.

    The test mainly measures the time for signing off a session and raises
    an exception if it could not be finished in time. First, it uses telemetry
    to login a GAIA account and waits for container boots up if the
    device supports ARC++, then validates the account. Next, it injects the
    ctrl+shift+q twice to logout the session, then waits for new login screen
    , i.e. the event of 'login-visible-prompt' to calculate the time elapsed
    for logging out.
    """
    version = 1


    def _get_latest_uptime(self, filename):
        with open('/tmp/uptime-' + filename) as statfile:
            values = map(lambda l: float(l.split()[0]),
                         statfile.readlines())
        logging.info('timestamp of %s -> %s ', filename, values[-1])
        return values[-1]


    def _validate(self):
        # Validate if the environment is expected.
        if not cryptohome.is_vault_mounted(
                user=chrome.NormalizeEmail(self.username)):
            raise error.TestFail('Expected to find a mounted vault for %s'
                                 % self.username)
        tab = self.cr.browser.tabs.New()
        tab.Navigate('http://accounts.google.com')
        tab.WaitForDocumentReadyStateToBeComplete()
        res = tab.EvaluateJavaScript('''
                var res = '',
                    divs = document.getElementsByTagName('div');
                for (var i = 0; i < divs.length; i++) {
                    res = divs[i].textContent;
                    if (res.search('%s') > 1) {
                    break;
                    }
                }
                res;
        ''' % self.username)
        if not res:
            raise error.TestFail('No references to %s on accounts page.'
                                 % self.username)
        tab.Close()


    def initialize(self):
        self.keyboard = keyboard.Keyboard()
        self.username, password = arc_util.get_test_account_info()
        # Login a user session.
        if utils.is_arc_available():
            super(platform_LogoutPerf, self).initialize(
                    gaia_login=True,
                    disable_arc_opt_in=False)
            self.cr = self._chrome
        else:
            with tempfile.NamedTemporaryFile() as cap:
                file_utils.download_file(arc_util._ARCP_URL, cap.name)
                password = cap.read().rstrip()
            self.cr = chrome.Chrome(gaia_login=True,
                                    username=self.username,
                                    password=password)


    def arc_setup(self):
        # Do nothing here
        logging.info('No ARC++ specific setup required')


    def arc_teardown(self):
        # Do nothing here
        logging.info('No ARC++ specific teardown required')


    def cleanup(self):
        self.keyboard.close()
        if utils.is_arc_available():
            super(platform_LogoutPerf, self).cleanup()


    def run_once(self):
        # Validate the current GAIA login session
        self._validate()

        # Start signing out the session, wait until the new event of
        # 'login-prompt-visible'.
        self.keyboard.press_key('ctrl+shift+q')
        self.keyboard.press_key('ctrl+shift+q')

        # Get current login prompt timestamp
        login_timestamp = self._get_latest_uptime(_LOGIN_TIME)

        # Poll for new login prompt timestamp
        utils.poll_for_condition(
                lambda: self._get_latest_uptime(_LOGIN_TIME) != login_timestamp,
                exception=error.TestFail('Timeout: Could not sign off in time'),
                timeout=_LOGOUT_TIMEOUT,
                sleep_interval=2,
                desc='Polling for user to be logged out.')

        logout_started = self._get_latest_uptime(_LOGOUT_STARTED_TIME)
        logging.info('logout started @%s', logout_started)
        chrome_exec = self._get_latest_uptime(_CHROME_EXEC_TIME)
        logging.info('chrome restarts @%s', chrome_exec)
        elapsed_time = float(chrome_exec) - float(logout_started)
        logging.info('The elapsed time for signout is %s', elapsed_time)
        self.output_perf_value(description='Seconds elapsed for Signout',
                               value=elapsed_time,
                               higher_is_better=False,
                               units='seconds')
