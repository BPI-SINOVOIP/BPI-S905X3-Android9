# Copyright 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import bz2
import glob
import json
import logging
import os
import re
import shutil
import tempfile
import time
import xml.etree.ElementTree as et
from autotest_lib.client.bin import test, utils
from autotest_lib.client.common_lib import error
from autotest_lib.client.cros import cros_logging, service_stopper
from autotest_lib.client.cros.graphics import graphics_utils

RERUN_RATIO = 0.02  # Ratio to rerun failing test for hasty mode


class graphics_dEQP(graphics_utils.GraphicsTest):
    """Run the drawElements Quality Program test suite.
    """
    version = 1
    _services = None
    _hasty = False
    _hasty_batch_size = 100  # Batch size in hasty mode.
    _shard_number = 0
    _shard_count = 1
    _board = None
    _cpu_type = None
    _gpu_type = None
    _surface = None
    _filter = None
    _width = 256  # Use smallest width for which all tests run/pass.
    _height = 256  # Use smallest height for which all tests run/pass.
    _timeout = 70  # Larger than twice the dEQP watchdog timeout at 30s.
    _test_names = None
    _test_names_file = None
    _log_path = None  # Location for detailed test output logs (in /tmp/).
    _debug = False  # Analyze kernel messages.
    _log_reader = None  # Reader to analyze (kernel) messages log.
    _log_filter = re.compile('.* .* kernel:')  # kernel messages filter.
    _env = None  # environment for test processes
    DEQP_MODULES = {
        'dEQP-EGL': 'egl',
        'dEQP-GLES2': 'gles2',
        'dEQP-GLES3': 'gles3',
        'dEQP-GLES31': 'gles31',
        'dEQP-VK': 'vk',
    }
    # We do not consider these results as failures.
    TEST_RESULT_FILTER = [
        'pass', 'notsupported', 'internalerror', 'qualitywarning',
        'compatibilitywarning', 'skipped'
    ]

    def initialize(self):
        super(graphics_dEQP, self).initialize()
        self._api_helper = graphics_utils.GraphicsApiHelper()
        self._board = utils.get_board()
        self._cpu_type = utils.get_cpu_soc_family()
        self._gpu_type = utils.get_gpu_family()

        # deqp may depend on libraries that are present only on test images.
        # Those libraries are installed in /usr/local.
        self._env = os.environ.copy()
        old_ld_path = self._env.get('LD_LIBRARY_PATH', '')
        if old_ld_path:
            self._env['LD_LIBRARY_PATH'] = '/usr/local/lib:/usr/local/lib64:' + old_ld_path
        else:
            self._env['LD_LIBRARY_PATH'] = '/usr/local/lib:/usr/local/lib64'

        self._services = service_stopper.ServiceStopper(['ui', 'powerd'])
        # Valid choices are fbo and pbuffer. The latter avoids dEQP assumptions.
        self._surface = 'pbuffer'
        self._services.stop_services()

    def cleanup(self):
        if self._services:
            self._services.restore_services()
        super(graphics_dEQP, self).cleanup()

    def _parse_test_results(self, result_filename,
                            test_results=None, failing_test=None):
        """Handles result files with one or more test results.

        @param result_filename: log file to parse.
        @param test_results: Result parsed will be appended to it.
        @param failing_test: Tests considered failed will append to it.

        @return: dictionary of parsed test results.
        """
        xml = ''
        xml_start = False
        xml_complete = False
        xml_bad = False
        result = 'ParseTestResultFail'

        if test_results is None:
            test_results = {}

        if not os.path.isfile(result_filename):
            return test_results

        with open(result_filename) as result_file:
            for line in result_file.readlines():
                # If the test terminates early, the XML will be incomplete
                # and should not be parsed.
                if line.startswith('#terminateTestCaseResult'):
                    result = line.strip().split()[1]
                    xml_bad = True
                # Will only see #endTestCaseResult if the test does not
                # terminate early.
                elif line.startswith('#endTestCaseResult'):
                    xml_complete = True
                elif xml_start:
                    xml += line
                elif line.startswith('#beginTestCaseResult'):
                    # If we see another begin before an end then something is
                    # wrong.
                    if xml_start:
                        xml_bad = True
                    else:
                        xml_start = True

                if xml_complete or xml_bad:
                    if xml_complete:
                        myparser = et.XMLParser(encoding='ISO-8859-1')
                        root = et.fromstring(xml, parser=myparser)
                        test_case = root.attrib['CasePath']
                        result = root.find('Result').get('StatusCode').strip()
                        xml_complete = False
                    test_results[result] = test_results.get(result, 0) + 1
                    if (result.lower() not in self.TEST_RESULT_FILTER and
                        failing_test != None):
                        failing_test.append(test_case)
                    xml_bad = False
                    xml_start = False
                    result = 'ParseTestResultFail'
                    xml = ''

        return test_results

    def _load_not_passing_cases(self, test_filter):
        """Load all test cases that are in non-'Pass' expectations."""
        not_passing_cases = []
        expectations_dir = os.path.join(self.bindir, 'expectations',
                                        self._gpu_type)
        subset_spec = '%s.*' % test_filter
        subset_paths = glob.glob(os.path.join(expectations_dir, subset_spec))
        for subset_file in subset_paths:
            # Filter against extra hasty failures only in hasty mode.
            if (not '.Pass.bz2' in subset_file and
               (self._hasty or '.hasty.' not in subset_file)):
                not_passing_cases.extend(
                    bz2.BZ2File(subset_file).read().splitlines())
        not_passing_cases.sort()
        return not_passing_cases

    def _translate_name_to_api(self, name):
        """Translate test_names or test_filter to api."""
        test_prefix = name.split('.')[0]
        if test_prefix in self.DEQP_MODULES:
            api = self.DEQP_MODULES[test_prefix]
        else:
            raise error.TestFail('Failed: Invalid test name: %s' % name)
        return api

    def _get_executable(self, api):
        """Return the executable path of the api."""
        return self._api_helper.get_deqp_executable(api)

    def _can_run(self, api):
        """Check if specific api is supported in this board."""
        return api in self._api_helper.get_supported_apis()

    def _bootstrap_new_test_cases(self, test_filter):
        """Ask dEQP for all test cases and removes non-Pass'ing ones.

        This function will query dEQP for test cases and remove all cases that
        are not in 'Pass'ing expectations from the list. This can be used
        incrementally updating failing/hangin tests over several runs.

        @param test_filter: string like 'dEQP-GLES2.info', 'dEQP-GLES3.stress'.

        @return: List of dEQP tests to run.
        """
        test_cases = []
        api = self._translate_name_to_api(test_filter)
        if self._can_run(api):
            executable = self._get_executable(api)
        else:
            return test_cases

        # Must be in the executable directory when running for it to find it's
        # test data files!
        os.chdir(os.path.dirname(executable))

        not_passing_cases = self._load_not_passing_cases(test_filter)
        # We did not find passing cases in expectations. Assume everything else
        # that is there should not be run this time.
        expectations_dir = os.path.join(self.bindir, 'expectations',
                                        self._gpu_type)
        subset_spec = '%s.*' % test_filter
        subset_paths = glob.glob(os.path.join(expectations_dir, subset_spec))
        for subset_file in subset_paths:
            # Filter against hasty failures only in hasty mode.
            if self._hasty or '.hasty.' not in subset_file:
                not_passing_cases.extend(
                    bz2.BZ2File(subset_file).read().splitlines())

        # Now ask dEQP executable nicely for whole list of tests. Needs to be
        # run in executable directory. Output file is plain text file named
        # e.g. 'dEQP-GLES2-cases.txt'.
        command = ('%s '
                   '--deqp-runmode=txt-caselist '
                   '--deqp-surface-type=%s '
                   '--deqp-gl-config-name=rgba8888d24s8ms0 ' % (executable,
                                                                self._surface))
        logging.info('Running command %s', command)
        utils.run(command,
                  env=self._env,
                  timeout=60,
                  stderr_is_expected=False,
                  ignore_status=False,
                  stdin=None)

        # Now read this caselist file.
        caselist_name = '%s-cases.txt' % test_filter.split('.')[0]
        caselist_file = os.path.join(os.path.dirname(executable), caselist_name)
        if not os.path.isfile(caselist_file):
            raise error.TestFail('Failed: No caselist file at %s!' %
                                 caselist_file)

        # And remove non-Pass'ing expectations from caselist.
        caselist = open(caselist_file).read().splitlines()
        # Contains lines like "TEST: dEQP-GLES2.capability"
        test_cases = []
        match = 'TEST: %s' % test_filter
        logging.info('Bootstrapping test cases matching "%s".', match)
        for case in caselist:
            if case.startswith(match):
                case = case.split('TEST: ')[1]
                test_cases.append(case)

        test_cases = list(set(test_cases) - set(not_passing_cases))
        if not test_cases:
            raise error.TestFail('Failed: Unable to bootstrap %s!' % test_filter)

        test_cases.sort()
        return test_cases

    def _get_test_cases_from_names_file(self):
        if os.path.exists(self._test_names_file):
            file_path = self._test_names_file
            test_cases = [line.rstrip('\n') for line in open(file_path)]
            return [test for test in test_cases if test and not test.isspace()]
        return []

    def _get_test_cases(self, test_filter, subset):
        """Gets the test cases for 'Pass', 'Fail' etc. expectations.

        This function supports bootstrapping of new GPU families and dEQP
        binaries. In particular if there are not 'Pass' expectations found for
        this GPU family it will query the dEQP executable for a list of all
        available tests. It will then remove known non-'Pass'ing tests from
        this list to avoid getting into hangs/crashes etc.

        @param test_filter: string like 'dEQP-GLES2.info', 'dEQP-GLES3.stress'.
        @param subset: string from 'Pass', 'Fail', 'Timeout' etc.

        @return: List of dEQP tests to run.
        """
        expectations_dir = os.path.join(self.bindir, 'expectations',
                                        self._gpu_type)
        subset_name = '%s.%s.bz2' % (test_filter, subset)
        subset_path = os.path.join(expectations_dir, subset_name)
        if not os.path.isfile(subset_path):
            if subset == 'NotPass':
                # TODO(ihf): Running hasty and NotPass together is an invitation
                # for trouble (stability). Decide if it should be disallowed.
                return self._load_not_passing_cases(test_filter)
            if subset != 'Pass':
                raise error.TestFail('Failed: No subset file found for %s!' %
                                     subset_path)
            # Ask dEQP for all cases and remove the failing ones.
            return self._bootstrap_new_test_cases(test_filter)

        test_cases = bz2.BZ2File(subset_path).read().splitlines()
        if not test_cases:
            raise error.TestFail(
                'Failed: No test cases found in subset file %s!' % subset_path)
        return test_cases

    def _run_tests_individually(self, test_cases, failing_test=None):
        """Runs tests as isolated from each other, but slowly.

        This function runs each test case separately as a command.
        This means a new context for each test etc. Failures will be more
        isolated, but runtime quite high due to overhead.

        @param test_cases: List of dEQP test case strings.
        @param failing_test: Tests considered failed will be appended to it.

        @return: dictionary of test results.
        """
        test_results = {}
        width = self._width
        height = self._height

        i = 0
        for test_case in test_cases:
            i += 1
            logging.info('[%d/%d] TestCase: %s', i, len(test_cases), test_case)
            result_prefix = os.path.join(self._log_path, test_case)
            log_file = '%s.log' % result_prefix
            debug_file = '%s.debug' % result_prefix
            api = self._translate_name_to_api(test_case)
            if not self._can_run(api):
                result = 'Skipped'
                logging.info('Skipping on %s: %s', self._gpu_type, test_case)
            else:
                executable = self._get_executable(api)
                command = ('%s '
                           '--deqp-case=%s '
                           '--deqp-surface-type=%s '
                           '--deqp-gl-config-name=rgba8888d24s8ms0 '
                           '--deqp-log-images=disable '
                           '--deqp-watchdog=enable '
                           '--deqp-surface-width=%d '
                           '--deqp-surface-height=%d '
                           '--deqp-log-filename=%s' % (
                               executable,
                               test_case,
                               self._surface,
                               width,
                               height,
                               log_file)
                           )
                logging.debug('Running single: %s', command)

                # Must be in the executable directory when running for it to find it's
                # test data files!
                os.chdir(os.path.dirname(executable))

                # Must initialize because some errors don't repopulate
                # run_result, leaving old results.
                run_result = {}
                start_time = time.time()
                try:
                    run_result = utils.run(command,
                                           env=self._env,
                                           timeout=self._timeout,
                                           stderr_is_expected=False,
                                           ignore_status=True)
                    result_counts = self._parse_test_results(
                        log_file,
                        failing_test=failing_test)
                    if result_counts:
                        result = result_counts.keys()[0]
                    else:
                        result = 'Unknown'
                except error.CmdTimeoutError:
                    result = 'TestTimeout'
                except error.CmdError:
                    result = 'CommandFailed'
                except Exception:
                    result = 'UnexpectedError'
                end_time = time.time()

                if self._debug:
                    # Collect debug info and save to json file.
                    output_msgs = {
                        'start_time': start_time,
                        'end_time': end_time,
                        'stdout': [],
                        'stderr': [],
                        'dmesg': []
                    }
                    logs = self._log_reader.get_logs()
                    self._log_reader.set_start_by_current()
                    output_msgs['dmesg'] = [
                        msg for msg in logs.splitlines()
                        if self._log_filter.match(msg)
                    ]
                    if run_result:
                        output_msgs['stdout'] = run_result.stdout.splitlines()
                        output_msgs['stderr'] = run_result.stderr.splitlines()
                    with open(debug_file, 'w') as fd:
                        json.dump(
                            output_msgs,
                            fd,
                            indent=4,
                            separators=(',', ' : '),
                            sort_keys=True)

            logging.info('Result: %s', result)
            test_results[result] = test_results.get(result, 0) + 1

        return test_results

    def _run_tests_hasty(self, test_cases, failing_test=None):
        """Runs tests as quickly as possible.

        This function runs all the test cases, but does not isolate tests and
        may take shortcuts/not run all tests to provide maximum coverage at
        minumum runtime.

        @param test_cases: List of dEQP test case strings.
        @param failing_test: Test considered failed will append to it.

        @return: dictionary of test results.
        """
        # TODO(ihf): It saves half the test time to use 32*32 but a few tests
        # fail as they need surfaces larger than 200*200.
        width = self._width
        height = self._height
        results = {}

        # All tests combined less than 1h in hasty.
        batch_timeout = min(3600, self._timeout * self._hasty_batch_size)
        num_test_cases = len(test_cases)

        # We are dividing the number of tests into several shards but run them
        # in smaller batches. We start and end at multiples of batch_size
        # boundaries.
        shard_start = self._hasty_batch_size * (
            (self._shard_number * (num_test_cases / self._shard_count)) /
            self._hasty_batch_size)
        shard_end = self._hasty_batch_size * ((
            (self._shard_number + 1) * (num_test_cases / self._shard_count)) /
                                              self._hasty_batch_size)
        # The last shard will be slightly larger than the others. Extend it to
        # cover all test cases avoiding rounding problems with the integer
        # arithmetics done to compute shard_start and shard_end.
        if self._shard_number + 1 == self._shard_count:
            shard_end = num_test_cases

        for batch in xrange(shard_start, shard_end, self._hasty_batch_size):
            batch_to = min(batch + self._hasty_batch_size, shard_end)
            batch_cases = '\n'.join(test_cases[batch:batch_to])
            # This assumes all tests in the batch are kicked off via the same
            # executable.
            api = self._translate_name_to_api(test_cases[batch])
            if not self._can_run(api):
                logging.info('Skipping tests on %s: %s', self._gpu_type,
                             batch_cases)
            else:
                executable = self._get_executable(api)
                log_file = os.path.join(self._log_path,
                                        '%s_hasty_%d.log' % (self._filter, batch))
                command = ('%s '
                           '--deqp-stdin-caselist '
                           '--deqp-surface-type=%s '
                           '--deqp-gl-config-name=rgba8888d24s8ms0 '
                           '--deqp-log-images=disable '
                           '--deqp-visibility=hidden '
                           '--deqp-watchdog=enable '
                           '--deqp-surface-width=%d '
                           '--deqp-surface-height=%d '
                           '--deqp-log-filename=%s' % (
                               executable,
                               self._surface,
                               width,
                               height,
                               log_file)
                           )

                logging.info('Running tests %d...%d out of %d:\n%s\n%s',
                             batch + 1, batch_to, num_test_cases, command,
                             batch_cases)

                # Must be in the executable directory when running for it to
                # find it's test data files!
                os.chdir(os.path.dirname(executable))

                try:
                    utils.run(command,
                              env=self._env,
                              timeout=batch_timeout,
                              stderr_is_expected=False,
                              ignore_status=False,
                              stdin=batch_cases)
                except Exception:
                    pass
                # We are trying to handle all errors by parsing the log file.
                results = self._parse_test_results(log_file, results,
                                                   failing_test)
                logging.info(results)
        return results

    def _run_once(self, test_cases):
        """Run dEQP test_cases in individual/hasty mode.
        @param test_cases: test cases to run.
        """
        failing_test = []
        if self._hasty:
            logging.info('Running in hasty mode.')
            test_results = self._run_tests_hasty(test_cases, failing_test)
        else:
            logging.info('Running each test individually.')
            test_results = self._run_tests_individually(test_cases,
                                                        failing_test)
        return test_results, failing_test

    def run_once(self, opts=None):
        options = dict(filter='',
                       test_names='',  # e.g., dEQP-GLES3.info.version,
                                       # dEQP-GLES2.functional,
                                       # dEQP-GLES3.accuracy.texture, etc.
                       test_names_file='',
                       timeout=self._timeout,
                       subset_to_run='Pass',  # Pass, Fail, Timeout, NotPass...
                       hasty='False',
                       shard_number='0',
                       shard_count='1',
                       debug='False',
                       perf_failure_description=None)
        if opts is None:
            opts = []
        options.update(utils.args_to_dict(opts))
        logging.info('Test Options: %s', options)

        self._hasty = (options['hasty'] == 'True')
        self._timeout = int(options['timeout'])
        self._test_names_file = options['test_names_file']
        self._test_names = options['test_names']
        self._shard_number = int(options['shard_number'])
        self._shard_count = int(options['shard_count'])
        self._debug = (options['debug'] == 'True')
        if not (self._test_names_file or self._test_names):
            self._filter = options['filter']
            if not self._filter:
                raise error.TestFail('Failed: No dEQP test filter specified')
        if options['perf_failure_description']:
            self._test_failure_description = options['perf_failure_description']
        else:
            # Do not report failure if failure description is not specified.
            self._test_failure_report_enable = False

        # Some information to help post-process logs into blacklists later.
        logging.info('ChromeOS BOARD = %s', self._board)
        logging.info('ChromeOS CPU family = %s', self._cpu_type)
        logging.info('ChromeOS GPU family = %s', self._gpu_type)

        # Create a place to put detailed test output logs.
        filter_name = self._filter or os.path.basename(self._test_names_file)
        logging.info('dEQP test filter = %s', filter_name)
        self._log_path = os.path.join(tempfile.gettempdir(), '%s-logs' %
                                                             filter_name)
        shutil.rmtree(self._log_path, ignore_errors=True)
        os.mkdir(self._log_path)

        # Load either tests specified by test_names_file, test_names or filter.
        test_cases = []
        if self._test_names_file:
            test_cases = self._get_test_cases_from_names_file()
        elif self._test_names:
            test_cases = []
            for name in self._test_names.split(','):
                test_cases.extend(self._get_test_cases(name, 'Pass'))
        elif self._filter:
            test_cases = self._get_test_cases(self._filter,
                                              options['subset_to_run'])

        if self._debug:
            # LogReader works on /var/log/messages by default.
            self._log_reader = cros_logging.LogReader()
            self._log_reader.set_start_by_current()

        # Assume all tests failed at the beginning.
        for test_case in test_cases:
            self.add_failures(test_case)

        test_results, failing_test = self._run_once(test_cases)
        # Rerun the test if we are in hasty mode.
        if self._hasty and len(failing_test) > 0:
            if len(failing_test) < sum(test_results.values()) * RERUN_RATIO:
                logging.info("Because we are in hasty mode, we will rerun the "
                             "failing tests one at a time")
                rerun_results, failing_test = self._run_once(failing_test)
                # Update failing test result from the test_results
                for result in test_results:
                    if result.lower() not in self.TEST_RESULT_FILTER:
                        test_results[result] = 0
                for result in rerun_results:
                    test_results[result] = (test_results.get(result, 0) +
                                            rerun_results[result])
            else:
                logging.info("There are too many failing tests. It would "
                             "take too long to rerun them. Giving up.")

        # Update failing tests to the chrome perf dashboard records.
        for test_case in test_cases:
            if test_case not in failing_test:
                self.remove_failures(test_case)

        logging.info('Test results:')
        logging.info(test_results)
        logging.debug('Test Failed: %s', failing_test)
        self.write_perf_keyval(test_results)

        test_count = 0
        test_failures = 0
        test_passes = 0
        test_skipped = 0
        for result in test_results:
            test_count += test_results[result]
            if result.lower() in ['pass']:
                test_passes += test_results[result]
            if result.lower() not in self.TEST_RESULT_FILTER:
                test_failures += test_results[result]
            if result.lower() in ['skipped']:
                test_skipped += test_results[result]
        # The text "Completed all tests." is used by the process_log.py script
        # and should always appear at the end of a completed test run.
        logging.info(
            'Completed all tests. Saw %d tests, %d passes and %d failures.',
            test_count, test_passes, test_failures)

        if self._filter and test_count == 0 and options[
                'subset_to_run'] != 'NotPass':
            logging.warning('No test cases found for filter: %s!', self._filter)

        if test_failures:
            raise error.TestFail('Failed: on %s %d/%d tests failed.' %
                                 (self._gpu_type, test_failures, test_count))
        if test_skipped > 0:
            logging.info('On %s %d tests skipped, %d passes' %
                         (self._gpu_type, test_skipped, test_passes))
