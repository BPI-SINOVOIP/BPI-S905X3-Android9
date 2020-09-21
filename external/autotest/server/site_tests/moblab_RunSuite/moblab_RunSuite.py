# Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging

from autotest_lib.client.common_lib import error
from autotest_lib.server.cros import moblab_test
from autotest_lib.server.hosts import moblab_host
from autotest_lib.utils import labellib


_CLEANUP_TIME_M = 5
_MOBLAB_IMAGE_STORAGE = '/mnt/moblab/static'

class moblab_RunSuite(moblab_test.MoblabTest):
    """
    Moblab run suite test. Ensures that a Moblab can run a suite from start
    to finish by kicking off a suite which will have the Moblab stage an
    image, provision its DUTs and run the tests.
    """
    version = 1


    def run_once(self, host, suite_name, moblab_suite_max_retries,
                 target_build='', clear_devserver_cache=True,
                 test_timeout_hint_m=None):
        """Runs a suite on a Moblab Host against its test DUTS.

        @param host: Moblab Host that will run the suite.
        @param suite_name: Name of the suite to run.
        @param moblab_suite_max_retries: The maximum number of test retries
                allowed within the suite launched on moblab.
        @param target_build: Optional build to be use in the run_suite
                call on moblab. This argument is passed as is to run_suite. It
                must be a sensible build target for the board of the sub-DUTs
                attached to the moblab.
        @param clear_devserver_cache: If True, image cache of the devserver
                running on moblab is cleared before running the test to validate
                devserver imaging staging flow.
        @param test_timeout_hint_m: (int) Optional overall timeout for the test.
                For this test, it is very important to collect post failure data
                from the moblab device. If the overall timeout is provided, the
                test will try to fail early to save some time for log collection
                from the DUT.

        @raises AutoservRunError if the suite does not complete successfully.
        """
        self._host = host

        self._maybe_clear_devserver_cache(clear_devserver_cache)
        # Fetch the board of the DUT's assigned to this Moblab. There should
        # only be one type.
        try:
            dut = host.afe.get_hosts()[0]
        except IndexError:
            raise error.TestFail('All hosts for this MobLab are down. Please '
                                 'request the lab admins to take a look.')

        labels = labellib.LabelsMapping(dut.labels)
        board = labels['board']

        if not target_build:
            stable_version_map = host.afe.get_stable_version_map(
                    host.afe.CROS_IMAGE_TYPE)
            target_build = stable_version_map.get_image_name(board)

        logging.info('Running suite: %s.', suite_name)
        cmd = ("%s/site_utils/run_suite.py --pool='' --board=%s --build=%s "
               "--suite_name=%s --retry=True " "--max_retries=%d" %
               (moblab_host.AUTOTEST_INSTALL_DIR, board, target_build,
                suite_name, moblab_suite_max_retries))
        cmd, run_suite_timeout_s = self._append_run_suite_timeout(
                cmd,
                test_timeout_hint_m,
        )

        logging.debug('Run suite command: %s', cmd)
        try:
            result = host.run_as_moblab(cmd, timeout=run_suite_timeout_s)
        except error.AutoservRunError as e:
            if _is_run_suite_error_critical(e.result_obj.exit_status):
                raise
        else:
            logging.debug('Suite Run Output:\n%s', result.stdout)
            # Cache directory can contain large binaries like CTS/CTS zip files
            # no need to offload those in the results.
            # The cache is owned by root user
            host.run('rm -fR /mnt/moblab/results/shared/cache',
                      timeout=600)

    def _append_run_suite_timeout(self, cmd, test_timeout_hint_m):
        """Modify given run_suite command with timeout.

        @param cmd: run_suite command str.
        @param test_timeout_hint_m: (int) timeout for the test, or None.
        @return cmd, run_suite_timeout_s: cmd is the updated command str,
                run_suite_timeout_s is the timeout to use for the run_suite
                call, in seconds.
        """
        if test_timeout_hint_m is None:
            return cmd, 10800

        # Arguments passed in via test_args may be all str, depending on how
        # they're passed in.
        test_timeout_hint_m = int(test_timeout_hint_m)
        elasped_m = self.elapsed.total_seconds() / 60
        run_suite_timeout_m = (
                test_timeout_hint_m - elasped_m - _CLEANUP_TIME_M)
        logging.info('Overall test timeout hint provided (%d minutes)',
                     test_timeout_hint_m)
        logging.info('%d minutes have already elasped', elasped_m)
        logging.info(
                'Keeping %d minutes for cleanup, will allow %d minutes for '
                'the suite to run.', _CLEANUP_TIME_M, run_suite_timeout_m)
        cmd += ' --timeout_mins %d' % run_suite_timeout_m
        return cmd, run_suite_timeout_m * 60

    def _maybe_clear_devserver_cache(self, clear_devserver_cache):
        # When passed in via test_args, all arguments are str
        if not isinstance(clear_devserver_cache, bool):
            clear_devserver_cache = (clear_devserver_cache.lower() == 'true')
        if clear_devserver_cache:
            self._host.run('rm -rf %s/*' % _MOBLAB_IMAGE_STORAGE)


def _is_run_suite_error_critical(return_code):
    # We can't actually import run_suite here because importing run_suite pulls
    # in certain MySQLdb dependencies that fail to load in the context of a
    # test.
    # OTOH, these return codes are unlikely to change because external users /
    # builders depend on them.
    return return_code not in (
            0,  # run_suite.RETURN_CODES.OK
            2,  # run_suite.RETURN_CODES.WARNING
    )
