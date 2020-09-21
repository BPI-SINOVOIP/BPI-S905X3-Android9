# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import re
import sys
import urllib2

from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib import global_config
from autotest_lib.client.common_lib.cros import dev_server
from autotest_lib.server import afe_utils
from autotest_lib.server import test
from autotest_lib.server.cros import provision


_CONFIG = global_config.global_config
# pylint: disable-msg=E1120
_IMAGE_URL_PATTERN = _CONFIG.get_config_value(
        'CROS', 'image_url_pattern', type=str)


class provision_AutoUpdate(test.test):
    """A test that can provision a machine to the correct ChromeOS version."""
    version = 1

    def initialize(self, host, value, force=False, is_test_na=False):
        """Initialize.

        @param host: The host object to update to |value|.
        @param value: The build type and version to install on the host.
        @param force: not used by initialize.
        @param is_test_na: boolean, if True, will simply skip the test
                           and emit TestNAError. The control file
                           determines whether the test should be skipped
                           and passes the decision via this argument. Note
                           we can't raise TestNAError in control file as it won't
                           be caught and handled properly.
        """
        if is_test_na:
            raise error.TestNAError(
                'Test not available for test_that. chroot detected, '
                'you are probably using test_that.')
        # We check value in initialize so that it fails faster.
        if not value:
            raise error.TestFail('No build version specified.')


    def run_once(self, host, value, force=False):
        """The method called by the control file to start the test.

        @param host: The host object to update to |value|.
        @param value: The host object to provision with a build corresponding
                      to |value|.
        @param force: True iff we should re-provision the machine regardless of
                      the current image version.  If False and the image
                      version matches our expected image version, no
                      provisioning will be done.
        """
        with_cheets = False
        logging.debug('Start provisioning %s to %s.', host, value)
        if value.endswith(provision.CHEETS_SUFFIX):
            image = re.sub(provision.CHEETS_SUFFIX + '$', '', value)
            with_cheets = True
        else:
            image = value

        # If the host is already on the correct build, we have nothing to do.
        # Note that this means we're not doing any sort of stateful-only
        # update, and that we're relying more on cleanup to do cleanup.
        # We could just not pass |force_update=True| to |machine_install|,
        # but I'd like the semantics that a provision test 'returns' TestNA
        # if the machine is already properly provisioned.
        if not force:
            info = host.host_info_store.get()
            if info.build == value:
                # We can't raise a TestNA, as would make sense, as that makes
                # job.run_test return False as if the job failed.  However, it'd
                # still be nice to get this into the status.log, so we manually
                # emit an INFO line instead.
                self.job.record('INFO', None, None,
                                'Host already running %s' % value)
                return

        # We're about to reimage a machine, so we need full_payload and
        # stateful.  If something happened where the devserver doesn't have one
        # of these, then it's also likely that it'll be missing autotest.
        # Therefore, we require the devserver to also have autotest staged, so
        # that the test that runs after this provision finishes doesn't error
        # out because the devserver that its job_repo_url is set to is missing
        # autotest test code.
        # TODO(milleral): http://crbug.com/249426
        # Add an asynchronous staging call so that we can ask the devserver to
        # fetch autotest in the background here, and then wait on it after
        # reimaging finishes or at some other point in the provisioning.
        ds = None
        try:
            ds = dev_server.ImageServer.resolve(image, host.hostname)
            ds.stage_artifacts(image, ['full_payload', 'stateful',
                                       'autotest_packages'])
            try:
                ds.stage_artifacts(image, ['quick_provision'])
            except dev_server.DevServerException as e:
                logging.warning('Unable to stage quick provision payload: %s',
                                e)
        except dev_server.DevServerException as e:
            raise error.TestFail, str(e), sys.exc_info()[2]
        finally:
            # If a devserver is resolved, Log what has been downloaded so far.
            if ds:
                try:
                    ds.list_image_dir(image)
                except (dev_server.DevServerException, urllib2.URLError) as e2:
                    logging.warning('Failed to list_image_dir for build %s. '
                                    'Error: %s', image, e2)

        url = _IMAGE_URL_PATTERN % (ds.url(), image)

        logging.debug('Installing image')
        try:
            afe_utils.machine_install_and_update_labels(
                    host,
                    force_update=True,
                    update_url=url,
                    force_full_update=force,
                    with_cheets=with_cheets)
        except error.InstallError as e:
            logging.error(e)
            raise error.TestFail, str(e), sys.exc_info()[2]
        logging.debug('Finished provisioning %s to %s', host, value)
