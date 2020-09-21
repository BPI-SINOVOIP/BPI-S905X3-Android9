# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import tempfile
from autotest_lib.client.bin import test
from autotest_lib.client.cros import cryptohome
from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib import file_utils
from autotest_lib.client.common_lib.cros import chrome


class login_GaiaLogin(test.test):
    """Sign into production gaia using Telemetry."""
    version = 1


    def run_once(self, username=None, password=None):
        if username is None:
            username = chrome.CAP_USERNAME

        if password is None:
            with tempfile.NamedTemporaryFile() as cap:
                file_utils.download_file(chrome.CAP_URL, cap.name)
                password = cap.read().rstrip()

        if not password:
          raise error.TestFail('Password not set.')

        with chrome.Chrome(gaia_login=True, username=username,
                                            password=password) as cr:
            if not cryptohome.is_vault_mounted(user=chrome.NormalizeEmail(username)):
                raise error.TestFail('Expected to find a mounted vault for %s'
                                     % username)
            tab = cr.browser.tabs.New()
            # TODO(achuith): Use a better signal of being logged in, instead of
            # parsing accounts.google.com.
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
            ''' % username)
            if not res:
                raise error.TestFail('No references to %s on accounts page.'
                                     % username)
            tab.Close()
