# Copyright 2016 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import requests

from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib import process_utils
from autotest_lib.client.common_lib.brillo import logcat_utils
from autotest_lib.server import test


_WEBSERVD_TEST_CLIENT = 'webservd_testc'

class brillo_WebservdSanity(test.test):
    """Verify that webservd delegates requests to clients."""
    version = 1

    def run_once(self, host=None):
        """Body of the test."""
        # Kill anything the init system knows about and drop signals
        # on everything else, until there is nothing left.
        host.run('stop %s' % _WEBSERVD_TEST_CLIENT)
        process_utils.pkill_process(_WEBSERVD_TEST_CLIENT, host=host)
        # Start up a clean new instance and get its pid.
        host.run('start %s' % _WEBSERVD_TEST_CLIENT)
        pid = int(host.run('pgrep %s' % _WEBSERVD_TEST_CLIENT).stdout.strip())
        # Wait for the clean new instance to report it is connected to the
        # webserver.
        logcat_utils.wait_for_logcat_log(
                '/system/bin/%s' % _WEBSERVD_TEST_CLIENT,
                '.*Webserver is online.*', process_id=pid, host=host)

        # Finally request a test page from our test client.
        host.adb_run('forward tcp:8998 tcp:80')
        r = requests.get('http://localhost:8998/webservd-test-client/ping')
        if r.status_code != 200:
            raise error.TestFail('Expected successful http request but '
                                 'status=%d' % r.status_code)
        if r.text != 'Still alive, still alive!\n':
            raise error.TestFail('Unexpected response: %s' % r.text)
