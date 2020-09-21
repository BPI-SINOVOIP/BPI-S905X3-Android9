# Copyright 2016 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import sys

from autotest_lib.client.common_lib import error
from autotest_lib.server import afe_utils
from autotest_lib.server import test


class provision_TestbedUpdate(test.test):
    """A test that can provision a machine to the correct Android version."""
    version = 1

    def _builds_to_set(self, builds):
        """Helper function to convert a build string into a set of builds.

        @param builds: Testbed build string to convert into a set.

        @returns: A set of the different builds in the build string.
        """
        result = set()
        if not builds:
            return result
        builds = builds.split(',')
        for build in builds:
            # Remove any build multipliers, i.e. <build>#2
            build = build.split('#')[0]
            result.add(build)
        return result


    def initialize(self, host, value, force=False, is_test_na=False,
                   repair=False):
        """Initialize.

        @param host: The testbed object to update to |value|.
                     NOTE: This arg must be called host to align with the other
                           provision actions.
        @param value: String of the image we want to install on the testbed.
        @param force: not used by initialize.
        @param is_test_na: boolean, if True, will simply skip the test
                           and emit TestNAError. The control file
                           determines whether the test should be skipped
                           and passes the decision via this argument. Note
                           we can't raise TestNAError in control file as it won't
                           be caught and handled properly.
        @param repair: not used by initialize.
        """
        if is_test_na:
            raise error.TestNAError('Provisioning not applicable.')
        # We check value in initialize so that it fails faster.
        if not (value or repair):
            raise error.TestFail('No build version specified.')


    def run_once(self, host, value=None, force=False, repair=False):
        """The method called by the control file to start the test.

        @param host: The testbed object to update to |value|.
                     NOTE: This arg must be called host to align with the other
                           provision actions.
        @param value: The testbed object to provision with a build
                      corresponding to |value|.
        @param force: True iff we should re-provision the machine regardless of
                      the current image version.  If False and the image
                      version matches our expected image version, no
                      provisioning will be done.
        @param repair: Not yet supported for testbeds.

        """
        testbed = host
        logging.debug('Start provisioning %s to %s', testbed, value)

        if not value and not repair:
            raise error.TestFail('No build provided and this is not a repair '
                                 ' job.')

        if not force:
            info = testbed.host_info_store.get()
            # If the host is already on the correct build, we have nothing to
            # do.
            if self._builds_to_set(info.build) == self._builds_to_set(value):
                # We can't raise a TestNA, as would make sense, as that makes
                # job.run_test return False as if the job failed.  However, it'd
                # still be nice to get this into the status.log, so we manually
                # emit an INFO line instead.
                self.job.record('INFO', None, None,
                                'Testbed already running %s' % value)
                return

        try:
            afe_utils.machine_install_and_update_labels(
                    host, image=value)
        except error.InstallError as e:
            logging.error(e)
            raise error.TestFail, str(e), sys.exc_info()[2]
        logging.debug('Finished provisioning %s to %s', host, value)
