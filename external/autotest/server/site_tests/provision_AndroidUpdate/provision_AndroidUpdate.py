# Copyright 2016 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import sys

from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib import global_config
from autotest_lib.server import afe_utils
from autotest_lib.server import test
from autotest_lib.server.hosts import testbed


_CONFIG = global_config.global_config
# pylint: disable-msg=E1120
_IMAGE_URL_PATTERN = _CONFIG.get_config_value(
        'ANDROID', 'image_url_pattern', type=str)


class provision_AndroidUpdate(test.test):
    """A test that can provision a machine to the correct Android version."""
    version = 1

    def initialize(self, host, value, force=False, is_test_na=False,
                   repair=False):
        """Initialize.

        @param host: The host object to update to |value|.
        @param value: String of the image we want to install on the host.
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


    def run_once(self, host, value=None, force=False, repair=False, board=None,
                 os=None):
        """The method called by the control file to start the test.

        @param host: The host object to update to |value|.
        @param value: The host object to provision with a build corresponding
                      to |value|.
        @param force: True iff we should re-provision the machine regardless of
                      the current image version.  If False and the image
                      version matches our expected image version, no
                      provisioning will be done.
        @param repair: If True, we are doing a repair provision, therefore the
                       build to provision is looked up from the AFE's
                       get_stable_version RPC.
        @param board: Board name of the device. For host created in testbed,
                      it does not have host labels and attributes. Therefore,
                      the board name needs to be passed in from the testbed
                      repair call.
        @param os: OS of the device. For host created in testbed, it does not
                   have host labels and attributes. Therefore, the OS needs to
                   be passed in from the testbed repair call.

        """
        logging.debug('Start provisioning %s to %s', host, value)

        if not value and not repair:
            raise error.TestFail('No build provided and this is not a repair '
                                 ' job.')

        info = host.host_info_store.get()
        # If the host is already on the correct build, we have nothing to do.
        if not force and info.build == value:
            # We can't raise a TestNA, as would make sense, as that makes
            # job.run_test return False as if the job failed.  However, it'd
            # still be nice to get this into the status.log, so we manually
            # emit an INFO line instead.
            self.job.record('INFO', None, None,
                            'Host already running %s' % value)
            return

        board = board or info.board
        os = os or info.os
        logging.debug('Host %s is board type: %s, OS type: %s', host, board, os)
        if repair:
            # TODO(kevcheng): remove the board.split() line when all android
            # devices have their board label updated to have no android in
            # there.
            board = board.split('-')[-1]
            value = afe_utils.get_stable_android_version(board)
            if not value:
                raise error.TestFail('No stable version assigned for board: '
                                     '%s' % board)
            logging.debug('Stable version found for board %s: %s', board, value)

        if not isinstance(host, testbed.TestBed):
            url, _ = host.stage_build_for_install(value, os_type=os)
            logging.debug('Installing image from: %s', url)
            args = {'build_url': url, 'os_type': os}
        else:
            logging.debug('Installing image: %s', value)
            args = {'image': value}
        try:
            afe_utils.machine_install_and_update_labels(
                    host, **args)
        except error.InstallError as e:
            logging.error(e)
            raise error.TestFail, str(e), sys.exc_info()[2]
        logging.debug('Finished provisioning %s to %s', host, value)
