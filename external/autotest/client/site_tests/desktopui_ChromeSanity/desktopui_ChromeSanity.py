# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import gobject
import logging
import os
import time

from autotest_lib.client.bin import test
from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib.cros import chrome, session_manager
from autotest_lib.client.cros import cryptohome
from autotest_lib.client.cros.graphics import graphics_utils

from dbus.mainloop.glib import DBusGMainLoop


class desktopui_ChromeSanity(test.test):
    """Performs basic integration testing for Chrome.

    This test performs very basic tests to verify that Chrome is somewhat
    usable in conjunction with the rest of the system.
    """
    version = 1

    _CHECK_CHROME_TIMEOUT_SEC = 30
    _SESSION_START_TIMEOUT_SEC = 20

    _TEST_FILENAME = 'test.html'
    _TEST_CONTENT = 'Page loaded successfully.'

    _SCREENSHOT_DIR = '/usr/local/autotest/results/default/' \
            'desktopui_ChromeSanity/results/'


    def initialize(self):
        super(desktopui_ChromeSanity, self).initialize()


    def run_once(self):
        """
        Runs the test.
        """
        dbus_loop = DBusGMainLoop(set_as_default=True)
        listener = session_manager.SessionSignalListener(gobject.MainLoop())
        listener.listen_for_session_state_change('started')

        logging.info('Logging in...')
        with chrome.Chrome(init_network_controller=True) as cr:
            # Check that Chrome asks session_manager to start a session.
            listener.wait_for_signals(
                    desc=('SessionStateChanged "started" D-Bus signal from '
                          'session_manager'),
                    timeout=self._SESSION_START_TIMEOUT_SEC)
            logging.info('Successfully logged in as "%s"', cr.username)

            # Check that the user's encrypted home directory was mounted.
            if not cryptohome.is_vault_mounted(user=cr.username,
                                               allow_fail=False):
                raise error.TestFail(
                        'Didn\'t find mounted cryptohome for "%s"' %
                        cr.username)

            # Check that Chrome is able to load a web page.
            cr.browser.platform.SetHTTPServerDirectories(self.bindir)
            url = cr.browser.platform.http_server.UrlOf(
                    os.path.join(self.bindir, self._TEST_FILENAME))
            logging.info('Loading %s...', url)

            try:
                tab = cr.browser.tabs.New()
                tab.Navigate(url)
                tab.WaitForDocumentReadyStateToBeComplete()
                content = tab.EvaluateJavaScript(
                        'document.documentElement.innerText')
                if content != self._TEST_CONTENT:
                    raise error.TestFail(
                            'Expected page content "%s" but got "%s"' %
                            (self._TEST_CONTENT, content))
                logging.info('Saw expected content')
            except Exception as e:
                prefix = 'screenshot-%s' % time.strftime('%Y%m%d-%H%M%S')
                logging.info(
                        'Got exception; saving screenshot to %s/%s',
                        self._SCREENSHOT_DIR, prefix)
                if not os.path.exists(self._SCREENSHOT_DIR):
                    os.makedirs(self._SCREENSHOT_DIR)
                graphics_utils.take_screenshot(self._SCREENSHOT_DIR, prefix)

                if isinstance(e, error.TestFail):
                    raise e
                else:
                    raise error.TestFail(str(e))
