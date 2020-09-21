# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from collections import namedtuple
import fnmatch
import logging
import os

import common
from autotest_lib.client.common_lib import error, gtest_parser
from autotest_lib.client.common_lib.cros import dev_server
from autotest_lib.server import afe_utils
from autotest_lib.server import test


NATIVE_TESTS_PATH = '/data/nativetest'
WHITELIST_FILE = '/data/nativetest/tests.txt'
ANDROID_NATIVE_TESTS_FILE_FMT = (
        '%(build_target)s-continuous_native_tests-%(build_id)s.zip')
BRILLO_NATIVE_TESTS_FILE_FMT = '%(build_target)s-brillo-tests-%(build_id)s.zip'
LIST_TEST_BINARIES_TEMPLATE = (
        'find %(path)s -type f -mindepth 2 -maxdepth 2 '
        '\( -perm -100 -o -perm -010 -o -perm -001 \)')
NATIVE_ONLY_BOARDS = ['dragonboard']

GtestSuite = namedtuple('GtestSuite', ['name', 'path', 'run_as_root', 'args'])

class brillo_Gtests(test.test):
    """Run one or more native gTest Suites."""
    version = 1


    def initialize(self, host=None, gtest_suites=None, use_whitelist=False,
                   filter_tests=None, native_tests=None):
        if not afe_utils.host_in_lab(host):
            return
        # TODO(ralphnathan): Remove this once we can determine this in another
        # way (b/29185385).
        if host.get_board_name() in NATIVE_ONLY_BOARDS:
            self._install_nativetests(
                    host, BRILLO_NATIVE_TESTS_FILE_FMT, 'nativetests')
        else:
            self._install_nativetests(host,
                                      ANDROID_NATIVE_TESTS_FILE_FMT,
                                      'continuous_native_tests')


    def _install_nativetests(self, host, test_file_format, artifact):
        """Install the nativetests zip onto the DUT.

        Device images built by the Android Build System do not have the
        native gTests installed. This method requests the devserver to
        download the nativetests package into the lab, the test station
        will download/unzip the package, and finally install it onto the DUT.

        @param host: host object to install the nativetests onto.
        @param test_file_format: Format of the zip file containing the tests.
        @param artifact: Devserver artifact to stage.
        """
        info = host.host_info_store.get()
        ds = dev_server.AndroidBuildServer.resolve(info.build, host.hostname)
        ds.stage_artifacts(image=info.build, artifacts=[artifact])
        build_url = os.path.join(ds.url(), 'static', info.build)
        nativetests_file = (test_file_format %
                            host.get_build_info_from_build_url(build_url))
        tmp_dir = host.teststation.get_tmp_dir()
        host.download_file(build_url, nativetests_file, tmp_dir, unzip=True)
        host.adb_run('push %s %s' % (os.path.join(tmp_dir, 'DATA',
                                                  'nativetest'),
                                     NATIVE_TESTS_PATH))


    def _get_whitelisted_tests(self, whitelist_path):
        """Return the list of whitelisted tests.

        The whitelist is expected to be a three column CSV file containing:
        * the test name
        * "yes" or "no" whether the test should be run as root or not.
        * optional command line arguments to be passed to the test.

        Anything after a # on a line is considered to be a comment and  ignored.

        @param whitelist_path: Path to the whitelist.

        @return a map of test name to GtestSuite tuple.
        """
        suite_map = dict()
        for line in self.host.run_output(
                'cat %s' % whitelist_path).splitlines():
            # Remove anything after the first # (comments).
            line = line.split('#')[0]
            if line.strip() == '':
                continue

            parts = line.split(',')
            if len(parts) < 2:
                logging.error('badly formatted line in %s: %s', whitelist_path,
                              line)
                continue

            name = parts[0].strip()
            extra_args = parts[2].strip() if len(parts) > 2 else ''
            path = '' # Path will be updated if the test is present on the DUT.
            suite_map[name] = GtestSuite(name, path, parts[1].strip() == 'yes',
                                         extra_args)
        return suite_map


    def _find_all_gtestsuites(self, use_whitelist=False, filter_tests=None):
        """Find all the gTest Suites installed on the DUT.

        @param use_whitelist: Only whitelisted tests found on the system will
                              be used.
        @param filter_tests: Only tests that match these globs will be used.
        """
        list_cmd = LIST_TEST_BINARIES_TEMPLATE % {'path': NATIVE_TESTS_PATH}
        gtest_suites_path = self.host.run_output(list_cmd).splitlines()
        gtest_suites = [GtestSuite(os.path.basename(path), path, True, '')
                        for path in gtest_suites_path]

        if use_whitelist:
            try:
                whitelisted = self._get_whitelisted_tests(WHITELIST_FILE)
                suites_to_run = []
                for suite in gtest_suites:
                    if whitelisted.get(suite.name):
                        whitelisted_suite = whitelisted.get(suite.name)
                        # Get the name and path from the suites on the DUT and
                        # get the other args from the whitelist map.
                        suites_to_run.append(GtestSuite(
                                suite.name, suite.path,
                                whitelisted_suite.run_as_root,
                                whitelisted_suite.args))
                gtest_suites = suites_to_run
                if (len(suites_to_run) != len(whitelisted)):
                    whitelist_test_names = set(whitelisted.keys())
                    found_test_names = set([t.name for t in suites_to_run])
                    diff_tests = list(whitelist_test_names - found_test_names)
                    for t in diff_tests:
                        logging.warning('Could not find %s', t);
                    raise error.TestWarn(
                            'Not all whitelisted tests found on the DUT. '
                            'Expected %i tests but only found %i' %
                            (len(whitelisted), len(suites_to_run)))
            except error.GenericHostRunError:
                logging.error('Failed to read whitelist %s', WHITELIST_FILE)

        if filter_tests:
            gtest_suites = [t for t in gtest_suites
                            if any(fnmatch.fnmatch(t.path, n)
                                   for n in filter_tests)]
            logging.info('Running tests:\n  %s',
                         '\n  '.join(t.path for t in gtest_suites))

        if not gtest_suites:
            raise error.TestWarn('No test executables found on the DUT')
        logging.debug('Test executables found:\n%s',
                      '\n'.join([str(t) for t in gtest_suites]))
        return gtest_suites


    def run_gtestsuite(self, gtestSuite):
        """Run a gTest Suite.

        @param gtestSuite: GtestSuite tuple.

        @return True if the all the tests in the gTest Suite pass. False
                otherwise.
        """
        # Make sure the gTest Suite exists.
        result = self.host.run('test -e %s' % gtestSuite.path,
                               ignore_status=True)
        if not result.exit_status == 0:
            logging.error('Unable to find %s', gtestSuite.path)
            return False

        result = self.host.run('test -x %s' % gtestSuite.path,
                               ignore_status=True)
        if not result.exit_status == 0:
            self.host.run('chmod +x %s' % gtestSuite.path)

        logging.debug('Running: %s', gtestSuite)
        command = '%s %s' % (gtestSuite.path, gtestSuite.args)
        if not gtestSuite.run_as_root:
          command = 'su shell %s' % command

        # host.run() will print the stdout/stderr output in the debug logs
        # properly interleaved.
        result = self.host.run(command, ignore_status=True)

        parser = gtest_parser.gtest_parser()
        for line in result.stdout.splitlines():
            parser.ProcessLogLine(line)
        passed_tests = parser.PassedTests()
        if passed_tests:
            logging.debug('Passed Tests: %s', passed_tests)
        failed_tests = parser.FailedTests(include_fails=True,
                                          include_flaky=True)
        if failed_tests:
            logging.error('Failed Tests: %s', failed_tests)
            for test in failed_tests:
                logging.error('Test %s failed:\n%s', test,
                              parser.FailureDescription(test))
            return False
        if result.exit_status != 0:
            logging.error('%s exited with exit code: %s',
                          gtestSuite, result.exit_status)
            return False
        return True


    def run_once(self, host=None, gtest_suites=None, use_whitelist=False,
                 filter_tests=None, native_tests=None):
        """Run gTest Suites on the DUT.

        @param host: host object representing the device under test.
        @param gtest_suites: List of gTest suites to run. Default is to run
                             every gTest suite on the host.
        @param use_whitelist: If gTestSuites is not passed in and use_whitelist
                              is true, only whitelisted tests found on the
                              system will be used.
        @param filter_tests: If gTestSuites is not passed in, search for tests
                             that match these globs to run instead.
        @param native_tests: Execute these specific tests.

        @raise TestFail: The test failed.
        """
        self.host = host
        if not gtest_suites and native_tests:
            gtest_suites = [GtestSuite('', t, True, '') for t in native_tests]
        if not gtest_suites:
            gtest_suites = self._find_all_gtestsuites(
                    use_whitelist=use_whitelist, filter_tests=filter_tests)

        failed_gtest_suites = []
        for gtestSuite in gtest_suites:
            if not self.run_gtestsuite(gtestSuite):
                failed_gtest_suites.append(gtestSuite)

        if failed_gtest_suites:
            logging.error(
                    'The following gTest Suites failed: \n %s',
                    '\n'.join([str(t) for t in failed_gtest_suites]))
            raise error.TestFail(
                    'Not all gTest Suites completed successfully. '
                    '%s out of %s suites failed. '
                    'Failed Suites: %s'
                    % (len(failed_gtest_suites),
                       len(gtest_suites),
                       failed_gtest_suites))
