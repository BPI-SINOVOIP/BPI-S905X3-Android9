# Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging, os
from autotest_lib.client.bin import utils
from autotest_lib.client.common_lib import error
from autotest_lib.client.cros import cros_ui, upstart
from autotest_lib.client.cros.crash import user_crash_test


_COLLECTION_ERROR_SIGNATURE = 'crash_reporter-user-collection'
_CORE2MD_PATH = '/usr/bin/core2md'
_LEAVE_CORE_PATH = '/root/.leave_core'
_MAX_CRASH_DIRECTORY_SIZE = 32


class logging_UserCrash(user_crash_test.UserCrashTest):
    """Verifies crash reporting for user processes."""
    version = 1


    def _test_reporter_startup(self):
        """Test that the core_pattern is set up by crash reporter."""
        # Turn off crash filtering so we see the original setting.
        self.disable_crash_filtering()
        output = utils.read_file(self._CORE_PATTERN).rstrip()
        expected_core_pattern = ('|%s --user=%%P:%%s:%%u:%%e' %
                                 self._CRASH_REPORTER_PATH)
        if output != expected_core_pattern:
            raise error.TestFail('core pattern should have been %s, not %s' %
                                 (expected_core_pattern, output))

        self._log_reader.set_start_by_reboot(-1)

        if not self._log_reader.can_find('Enabling user crash handling'):
            raise error.TestFail(
                'user space crash handling was not started during last boot')


    def _test_reporter_shutdown(self):
        """Test the crash_reporter shutdown code works."""
        self._log_reader.set_start_by_current()
        utils.system('%s --clean_shutdown' % self._CRASH_REPORTER_PATH)
        output = utils.read_file(self._CORE_PATTERN).rstrip()
        if output != 'core':
            raise error.TestFail('core pattern should have been core, not %s' %
                                 output)


    def _test_no_crash(self):
        """Test that the crasher can exit normally."""
        self._log_reader.set_start_by_current()
        result = self._run_crasher_process_and_analyze(username='root',
                                                       cause_crash=False)
        if (result['crashed'] or
            result['crash_reporter_caught'] or
            result['returncode'] != 0):
            raise error.TestFail('Normal exit of program with dumper failed')


    def _test_chronos_crasher(self):
        """Test a user space crash when running as chronos is handled."""
        self._check_crashing_process('chronos')


    def _test_chronos_crasher_no_consent(self):
        """Test that without consent no files are stored."""
        results = self._check_crashing_process('chronos', consent=False)


    def _test_root_crasher(self):
        """Test a user space crash when running as root is handled."""
        self._check_crashing_process('root')


    def _test_root_crasher_no_consent(self):
        """Test that without consent no files are stored."""
        results = self._check_crashing_process('root', consent=False)


    def _check_filter_crasher(self, should_receive):
        self._log_reader.set_start_by_current()
        crasher_basename = os.path.basename(self._crasher_path)
        utils.system(self._crasher_path, ignore_status=True);
        if should_receive:
            to_find = 'Received crash notification for ' + crasher_basename
        else:
            to_find = 'Ignoring crash from ' + crasher_basename
        utils.poll_for_condition(
            lambda: self._log_reader.can_find(to_find),
            timeout=10,
            exception=error.TestError(
              'Timeout waiting for: ' + to_find + ' in ' +
              self._log_reader.get_logs()))


    def _test_crash_filtering(self):
        """Test that crash filtering (a feature needed for testing) works."""
        crasher_basename = os.path.basename(self._crasher_path)
        self._log_reader.set_start_by_current()

        self.enable_crash_filtering('none')
        self._check_filter_crasher(False)

        self.enable_crash_filtering('sleep')
        self._check_filter_crasher(False)

        self.disable_crash_filtering()
        self._check_filter_crasher(True)


    def _test_max_enqueued_crashes(self):
        """Test that _MAX_CRASH_DIRECTORY_SIZE is enforced."""
        self._log_reader.set_start_by_current()
        username = 'root'

        crash_dir = self._get_crash_dir(username)
        full_message = ('Crash directory %s already full with %d pending '
                        'reports' % (crash_dir, _MAX_CRASH_DIRECTORY_SIZE))

        # Fill up the queue.
        for i in range(0, _MAX_CRASH_DIRECTORY_SIZE):
          result = self._run_crasher_process(username)
          if not result['crashed']:
            raise error.TestFail('failure while setting up queue: %d' %
                                 result['returncode'])
          if self._log_reader.can_find(full_message):
            raise error.TestFail('unexpected full message: ' + full_message)

        crash_dir_size = len(os.listdir(crash_dir))
        # For debugging
        utils.system('ls -l %s' % crash_dir)
        logging.info('Crash directory had %d entries', crash_dir_size)

        # Crash a bunch more times, but make sure no new reports
        # are enqueued.
        for i in range(0, 10):
          self._log_reader.set_start_by_current()
          result = self._run_crasher_process(username)
          logging.info('New log messages: %s', self._log_reader.get_logs())
          if not result['crashed']:
            raise error.TestFail('failure after setting up queue: %d' %
                                 result['returncode'])
          utils.poll_for_condition(
              lambda: self._log_reader.can_find(full_message),
              timeout=20,
              exception=error.TestFail('expected full message: ' +
                                       full_message))
          if crash_dir_size != len(os.listdir(crash_dir)):
            utils.system('ls -l %s' % crash_dir)
            raise error.TestFail('expected no new files (now %d were %d)',
                                 len(os.listdir(crash_dir)),
                                 crash_dir_size)


    def _check_collection_failure(self, test_option, failure_string):
        # Add parameter to core_pattern.
        old_core_pattern = utils.read_file(self._CORE_PATTERN)[:-1]
        try:
            utils.system('echo "%s %s" > %s' % (old_core_pattern, test_option,
                                                self._CORE_PATTERN))
            result = self._run_crasher_process_and_analyze('root',
                                                           consent=True)
            self._check_crashed_and_caught(result)
            if not self._log_reader.can_find(failure_string):
                raise error.TestFail('Did not find fail string in log %s' %
                                     failure_string)
            if result['minidump']:
                raise error.TestFail('failed collection resulted in minidump')
            if not result['log']:
                raise error.TestFail('failed collection had no log')
            log_contents = utils.read_file(result['log'])
            logging.debug('Log contents were: %s', log_contents)
            if not failure_string in log_contents:
                raise error.TestFail('Expected logged error '
                                     '\"%s\" was \"%s\"' %
                                     (failure_string, log_contents))
            # Verify we are generating appropriate diagnostic output.
            if ((not '===ps output===' in log_contents) or
                (not '===meminfo===' in log_contents)):
                raise error.TestFail('Expected full logs, got: ' + log_contents)
            self._check_generated_report_sending(result['meta'],
                                                 result['log'],
                                                 result['basename'],
                                                 'log',
                                                 _COLLECTION_ERROR_SIGNATURE)
        finally:
            utils.system('echo "%s" > %s' % (old_core_pattern,
                                             self._CORE_PATTERN))


    def _test_core2md_failure(self):
        self._check_collection_failure('--core2md_failure',
                                       'Problem during %s [result=1]: Usage:' %
                                       _CORE2MD_PATH)


    def _test_internal_directory_failure(self):
        self._check_collection_failure('--directory_failure',
                                       'Purposefully failing to create')


    def _test_crash_logs_creation(self):
        # Copy and rename crasher to trigger crash_reporter_logs.conf rule.
        logs_triggering_crasher = os.path.join(os.path.dirname(self.bindir),
                                               'crash_log_test')
        result = self._run_crasher_process_and_analyze(
            'root', crasher_path=logs_triggering_crasher)
        self._check_crashed_and_caught(result)
        contents = utils.read_file(result['log'])
        if contents != 'hello world\n':
            raise error.TestFail('Crash log contents unexpected: %s' % contents)
        if not ('log=' + result['log']) in utils.read_file(result['meta']):
            raise error.TestFail('Meta file does not reference log')


    def _test_crash_log_infinite_recursion(self):
        # Copy and rename crasher to trigger crash_reporter_logs.conf rule.
        recursion_triggering_crasher = os.path.join(
            os.path.dirname(self.bindir), 'crash_log_recursion_test')
        # The configuration file hardcodes this path, so make sure it's still
        # the same.
        if (recursion_triggering_crasher !=
            '/usr/local/autotest/tests/crash_log_recursion_test'):
          raise error.TestError('Path to recursion test changed')
        # Simply completing this command means that we avoided
        # infinite recursion.
        result = self._run_crasher_process(
            'root', crasher_path=recursion_triggering_crasher)


    def _check_core_file_persisting(self, expect_persist):
        self._log_reader.set_start_by_current()

        result = self._run_crasher_process('root')

        if not result['crashed']:
            raise error.TestFail('crasher did not crash')

        crash_contents = os.listdir(self._get_crash_dir('root'))

        logging.debug('Contents of crash directory: %s', crash_contents)
        logging.debug('Log messages: %s', self._log_reader.get_logs())

        if expect_persist:
            if not self._log_reader.can_find('Leaving core file at'):
                raise error.TestFail('Missing log message')
            expected_core_files = 1
        else:
            if self._log_reader.can_find('Leaving core file at'):
                raise error.TestFail('Unexpected log message')
            expected_core_files = 0

        dmp_files = 0
        core_files = 0
        for filename in crash_contents:
            if filename.endswith('.dmp'):
                dmp_files += 1
            if filename.endswith('.core'):
                core_files += 1

        if dmp_files != 1:
            raise error.TestFail('Should have been exactly 1 dmp file')
        if core_files != expected_core_files:
            raise error.TestFail('Should have been exactly %d core files' %
                                 expected_core_files)


    def _test_core_file_removed_in_production(self):
        """Test that core files do not stick around for production builds."""
        # Avoid remounting / rw by instead creating a tmpfs in /root and
        # populating it with everything but the
        utils.system('tar -cvz -C /root -f /tmp/root.tgz .')
        utils.system('mount -t tmpfs tmpfs /root')
        try:
            utils.system('tar -xvz -C /root -f /tmp/root.tgz .')
            os.remove(_LEAVE_CORE_PATH)
            if os.path.exists(_LEAVE_CORE_PATH):
                raise error.TestFail('.leave_core file did not disappear')
            self._check_core_file_persisting(False)
        finally:
            os.system('umount /root')


    def initialize(self):
        user_crash_test.UserCrashTest.initialize(self)

        # If the device has a GUI, return the device to the sign-in screen, as
        # some tests will fail inside a user session.
        if upstart.has_service('ui'):
            cros_ui.restart()


    # TODO(kmixter): Test crashing a process as ntp or some other
    # non-root, non-chronos user.

    def run_once(self):
        self._prepare_crasher()
        self._populate_symbols()

        # Run the test once without re-initializing
        # to catch problems with the default crash reporting setup
        self.run_crash_tests(['reporter_startup'],
                              initialize_crash_reporter=False,
                              must_run_all=False)

        self.run_crash_tests(['reporter_startup',
                              'reporter_shutdown',
                              'no_crash',
                              'chronos_crasher',
                              'chronos_crasher_no_consent',
                              'root_crasher',
                              'root_crasher_no_consent',
                              'crash_filtering',
                              'max_enqueued_crashes',
                              'core2md_failure',
                              'internal_directory_failure',
                              'crash_logs_creation',
                              'crash_log_infinite_recursion',
                              'core_file_removed_in_production'],
                              initialize_crash_reporter=True)
