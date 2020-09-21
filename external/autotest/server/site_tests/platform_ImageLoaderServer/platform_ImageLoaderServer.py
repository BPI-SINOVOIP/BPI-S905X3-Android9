# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging

from autotest_lib.client.common_lib import error
from autotest_lib.server import autotest
from autotest_lib.server import test
from autotest_lib.server import utils


class platform_ImageLoaderServer(test.test):
    """Does the server side file downloading for the ImageLoader autotest.
    """
    version = 1

    def _run_client_test(self, version1, version2, version3):
        """Runs client test."""
        try:
            self.autotest_client.run_test(
                'platform_ImageLoader',
                component1=version1,
                component2=version2,
                component3=version3,
                check_client_result=True)
            logging.info('platform_ImageLoader succeeded')
        except:
            raise error.TestFail('Failed: platform_ImageLoader')

    def run_once(self, host):
        """Runs platform ImageLoader tests."""
        self.host = host
        self.autotest_client = autotest.Autotest(self.host)
        # Download sample production signed components for simulated updates
        # from Google Storage. This needs to be done by a server test as the
        # client is unable to access Google Storage.
        try:
            version1 = '/tmp/prod_signed_23.0.0.207.tar.gz'
            utils.run('gsutil',
                      args=('cp', 'gs://chromeos-localmirror-private/'
                            'testing/components/prod_signed_23.0.0.207.tar.gz',
                            version1),
                      timeout=300,
                      ignore_status=False,
                      verbose=True,
                      stderr_is_expected=False,
                      ignore_timeout=False)

            version2 = '/tmp/prod_signed_24.0.0.186.tar.gz'
            utils.run('gsutil',
                      args=('cp', 'gs://chromeos-localmirror-private/'
                            'testing/components/prod_signed_24.0.0.186.tar.gz',
                            version2),
                      timeout=300,
                      ignore_status=False,
                      verbose=True,
                      stderr_is_expected=False,
                      ignore_timeout=False)

            version3 = '/tmp/prod_signed_10209.0.0.tar.gz'
            utils.run('gsutil',
                      args=('cp', 'gs://chromeos-localmirror-private/'
                            'testing/components/prod_signed_10209.0.0.tar.gz',
                            version3),
                      timeout=300,
                      ignore_status=False,
                      verbose=True,
                      stderr_is_expected=False,
                      ignore_timeout=False)
        except error.CmdTimeoutError:
            raise error.TestError('Slow network')
        except error.CmdError:
            raise error.TestError('Lack of Google Storage access permissions.')

        self.host.send_file(version1, version1)
        self.host.send_file(version2, version2)
        self.host.send_file(version3, version3)

        self.host.run('tar xvf "%s" -C "%s"' % (version1, '/home/chronos'))
        self.host.run('tar xvf "%s" -C "%s"' % (version2, '/home/chronos'))
        self.host.run('tar xvf "%s" -C "%s"' % (version3, '/home/chronos'))
        version1_unpack = '/home/chronos/prod_signed_23.0.0.207'
        version2_unpack = '/home/chronos/prod_signed_24.0.0.186'
        version3_unpack = '/home/chronos/prod_signed_10209.0.0'
        self.host.run('chmod -R 0755 "%s"' % (version1_unpack))
        self.host.run('chmod -R 0755 "%s"' % (version2_unpack))
        self.host.run('chmod -R 0755 "%s"' % (version3_unpack))
        # Run the actual test (installing and verifying component updates on
        # the client.
        self._run_client_test(version1_unpack, version2_unpack, version3_unpack)

        self.host.run('rm -rf "%s" "%s" "%s" "%s" "%s" "%s"'
                      % (version1,
                         version2,
                         version3,
                         version1_unpack,
                         version2_unpack,
                         version3_unpack))
