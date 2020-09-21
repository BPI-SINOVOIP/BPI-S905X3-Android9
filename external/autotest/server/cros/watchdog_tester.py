# Copyright 2016 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging, re

# http://docs.python.org/2/library/errno.html
import errno

from autotest_lib.client.common_lib import error

class WatchdogTester(object):
    """Helper class to perform various watchdog tests."""

    WD_DEV = '/dev/watchdog'

    def _exists_on_client(self):
        return self._client.run('test -c "%s"' % self.WD_DEV,
                                ignore_status=True).exit_status == 0

    # If daisydog is running, stop it so we can use /dev/watchdog
    def _stop_daemon(self):
        """If running, stop daisydog so we can use /dev/watchdog."""
        self._client.run('stop daisydog', ignore_status=True)

    def _start_daemon(self):
        self._client.run('start daisydog', ignore_status=True)

    def _query_hw_interval(self):
        """Check how long the hardware interval is."""
        output = self._client.run('daisydog -c').stdout
        secs = re.findall(r'HW watchdog interval is (\d*) seconds', output)[0]
        return int(secs)

    def __init__(self, client):
        self._client = client
        self._supported = self._exists_on_client()

    def is_supported(self):
        return self._supported

    def __enter__(self):
        self._stop_daemon()
        self._hw_interval = self._query_hw_interval()

    def trigger_watchdog(self, timeout=60):
        """
        Trigger a watchdog reset by opening the watchdog device but not petting
        it. Will ensure the device goes down and comes back up again.
        """

        try:
            self._client.run('echo "z" > %s' % self.WD_DEV)
        except error.AutoservRunError, e:
            raise error.TestError('write to %s failed (%s)' %
                                  (self.WD_DEV, errno.errorcode[e.errno]))

        logging.info("WatchdogHelper: tickled watchdog on %s (%ds to reboot)",
                     self._client.hostname, self._hw_interval)

        # machine should became unpingable after lockup
        # ...give 5 seconds slack...
        wait_down = self._hw_interval + 5
        if not self._client.wait_down(timeout=wait_down):
            raise error.TestError('machine should be unpingable '
                                  'within %d seconds' % wait_down)

        # make sure the machine comes back,
        # DHCP can take up to 45 seconds in odd cases.
        if not self._client.wait_up(timeout=timeout):
            raise error.TestError('machine did not reboot/ping within '
                                  '%d seconds of HW reset' % timeout)

    def __exit__(self, exception, value, traceback):
        self._start_daemon()
