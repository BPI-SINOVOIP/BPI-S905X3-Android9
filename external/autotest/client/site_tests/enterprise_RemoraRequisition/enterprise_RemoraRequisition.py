# Copyright 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging, os

from autotest_lib.client.bin import test, utils
from autotest_lib.client.common_lib.cros import chrome, enrollment
from autotest_lib.client.cros.multimedia import cfm_facade_native


class enterprise_RemoraRequisition(test.test):
    """Enroll as a Remora device."""
    version = 1


    def run_once(self):
        user_id, password = utils.get_signin_credentials(os.path.join(
                os.path.dirname(os.path.realpath(__file__)), 'credentials.txt'))
        if not (user_id and password):
            logging.warn('No credentials found - exiting test.')
            return

        with chrome.Chrome(auto_login=False,
                           disable_gaia_services=False) as cr:
            enrollment.RemoraEnrollment(cr.browser, user_id, password)
            self.cfm_facade = cfm_facade_native.CFMFacadeNative(cr, 'hotrod')
            self.cfm_facade.check_hangout_extension_context()
