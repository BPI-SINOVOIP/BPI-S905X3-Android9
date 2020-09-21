# Copyright 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging

from autotest_lib.client.bin import utils
from autotest_lib.client.common_lib.cros import chrome


def _ExecuteOobeCmd(browser, cmd):
    logging.info('Invoking ' + cmd)
    oobe = browser.oobe
    oobe.WaitForJavaScriptCondition('typeof Oobe !== \'undefined\'',
                                    timeout=10)
    oobe.ExecuteJavaScript(cmd)


def SwitchToRemora(browser):
    """Switch to Remora enrollment.

    @param browser: telemetry browser object.
    """
    chrome.Chrome.wait_for_browser_restart(
            lambda: _ExecuteOobeCmd(browser,
                                    'Oobe.remoraRequisitionForTesting();'),
            browser)
    utils.poll_for_condition(lambda: browser.oobe_exists, timeout=30)


def RemoraEnrollment(browser, user_id, password):
    """Enterprise login for a Remora device.

    @param browser: telemetry browser object.
    @param user_id: login credentials user_id.
    @param password: login credentials password.
    """
    SwitchToRemora(browser)
    browser.oobe.NavigateGaiaLogin(
            user_id, password, enterprise_enroll=True,
            for_user_triggered_enrollment=False)


def EnterpriseEnrollment(browser, user_id, password, auto_login=False):
    """Enterprise login for a kiosk device.

    @param browser: telemetry browser object.
    @param user_id: login credentials user_id.
    @param password: login credentials password.
    @param auto_login: also login after enrollment.
    """
    browser.oobe.NavigateGaiaLogin(user_id, password,
                                   enterprise_enroll=True,
                                   for_user_triggered_enrollment=True)
    if auto_login:
        browser.oobe.NavigateGaiaLogin(user_id, password)
        # TODO(achuith): Replace with WaitForLogin.
        utils.poll_for_condition(lambda: not browser.oobe_exists, timeout=30)


def EnterpriseFakeEnrollment(browser, user_id, password, gaia_id,
                             auto_login=False):
    """Enterprise fake login.

    @param browser: telemetry browser object.
    @param user_id: login credentials user_id.
    @param password: login credentials password.
    @param gaia_id: login credentials gaia_id.
    @param auto_login: also login after enrollment.
    """
    browser.oobe.NavigateFakeLogin(user_id, password, gaia_id,
                                   enterprise_enroll=True)
    if auto_login:
        browser.oobe.NavigateFakeLogin(user_id, password, gaia_id)
        # TODO(achuith): Replace with WaitForLogin.
        utils.poll_for_condition(lambda: not browser.oobe_exists, timeout=30)
