# Copyright 2016 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import os

from autotest_lib.client.bin import test, utils
from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib.cros import chrome
from autotest_lib.client.common_lib.cros import enrollment
from autotest_lib.client.common_lib.cros import kiosk_utils


class enterprise_KioskEnrollment(test.test):
    """Enroll the device in enterprise."""
    version = 1

    APP_NAME = 'chromesign'
    EXT_ID = 'odjaaghiehpobimgdjjfofmablbaleem'
    EXT_PAGE = 'viewer.html'

    def _CheckKioskExtensionContexts(self, browser):
        ext_contexts = kiosk_utils.wait_for_kiosk_ext(
                browser, self.EXT_ID)
        ext_urls = set([context.EvaluateJavaScript('location.href;')
                       for context in ext_contexts])
        expected_urls = set(
                ['chrome-extension://' + self.EXT_ID + '/' + path
                for path in [self.EXT_PAGE,
                             '_generated_background_page.html']])
        if expected_urls != ext_urls:
            raise error.TestFail(
                    'Unexpected extension context urls, expected %s, got %s'
                    % (expected_urls, ext_urls))


    def run_once(self, kiosk_app_attributes=None):
        if kiosk_app_attributes:
            self.APP_NAME, self.EXT_ID, self.EXT_PAGE = \
                    kiosk_app_attributes.rstrip().split(':')
        user_id, password = utils.get_signin_credentials(os.path.join(
                os.path.dirname(os.path.realpath(__file__)),
                'credentials.' + self.APP_NAME))
        if not (user_id and password):
            logging.warn('No credentials found - exiting test.')
            return

        with chrome.Chrome(auto_login=False,
                           disable_gaia_services=False) as cr:
            enrollment.EnterpriseEnrollment(cr.browser, user_id, password)
            self._CheckKioskExtensionContexts(cr.browser)
