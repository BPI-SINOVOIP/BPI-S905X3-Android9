# Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging

from autotest_lib.client.bin import utils
from autotest_lib.client.common_lib import error
from autotest_lib.client.cros import touch_playback_test_base


class touch_UpdateErrors(touch_playback_test_base.touch_playback_test_base):
    """Check that touch update is tried and that there are no update errors."""
    version = 1

    # Older devices with Synaptics touchpads do not report firmware updates.
    _INVALID_BOARDS = ['x86-alex', 'x86-alex_he', 'x86-zgb', 'x86-zgb_he',
                       'x86-mario', 'stout']

    # Devices which have errors in older builds but not newer ones.
    _IGNORE_OLDER_LOGS = ['expresso', 'enguarde', 'cyan', 'wizpig']

    # Devices which have errors in the first build after update.
    _IGNORE_AFTER_UPDATE_LOGS = ['link']

    def _find_logs_start_line(self):
        """Find where in /var/log/messages this build's logs start.

        Prevent bugs such as crosbug.com/p/31012, where unfixable errors from
        FSI builds remain in the logs.

        For devices where this applies, split the logs by Linux version.  Since
        this line can repeat, find the last chunk of logs where the version is
        all the same - all for the build under test.

        @returns: string of the line number to start looking at logs

        """
        if not (self._platform in self._IGNORE_OLDER_LOGS or
                self._platform in self._IGNORE_AFTER_UPDATE_LOGS):
            return '0'

        log_cmd = 'grep -ni "Linux version " /var/log/messages'

        version_entries = utils.run(log_cmd).stdout.strip().split('\n')

        # Separate the line number and the version date (i.e. remove timestamp).
        lines, dates = [], []
        for entry in version_entries:
            lines.append(entry[:entry.find(':')])
            dates.append(entry[entry.find('Linux version '):])
        latest = dates[-1]
        start_line = lines[-1]
        start_line_index = -1

        # Find where logs from this build start by checking backwards for the
        # first change in build.  Some of these dates may be duplicated.
        for i in xrange(len(lines)-1, -1, -1):
            if dates[i] != latest:
                break
            start_line = lines[i]
            start_line_index = i

        if start_line_index == 0:
            return '0'

        logging.info('This build has an older build; skipping some logs, '
                     'as was hardcoded for this platform.')

        # Ignore the first build after update if required.
        if self._platform in self._IGNORE_AFTER_UPDATE_LOGS:
            start_line_index += 1
            if start_line_index >= len(lines):
                raise error.TestError(
                        'Insufficent logs: aborting test to avoid a known '
                        'issue!  Please reboot and try again.')
            start_line = lines[start_line_index]

        return start_line

    def _check_updates(self, input_type):
        """Fail the test if device has problems with touch firmware update.

        @param input_type: string of input type, e.g. 'touchpad'

        @raises: TestFail if no update attempt occurs or if there is an error.

        """
        hw_id = self.player.devices[input_type].hw_id
        if not hw_id:
            raise error.TestError('%s has no valid hw_id!' % input_type)

        start_line = self._find_logs_start_line()
        log_cmd = 'tail -n +%s /var/log/messages | grep -i touch' % start_line

        pass_terms = ['touch-firmware-update']
        if hw_id == 'wacom': # Wacom styluses don't have Product ids.
            pass_terms.append(hw_id)
        else:
            pass_terms.append('"Product[^a-z0-9]ID[^a-z0-9]*%s"' % hw_id)
        fail_terms = ['error[^s]', 'err[^a-z]']
        ignore_terms = ['touchview','autotest']

        # Remove lines that match ignore_terms.
        for term in ignore_terms:
            log_cmd += ' | grep -v -i %s' % term

        # Check for key terms in touch logs.
        for term in pass_terms + fail_terms:
            search_cmd = '%s | grep -i %s' % (log_cmd, term)
            log_entries = utils.run(search_cmd, ignore_status=True).stdout
            if term in fail_terms and len(log_entries) > 0:
                error_msg = log_entries.split('\n')[0]
                error_msg = error_msg[error_msg.find(term)+len(term):].strip()
                raise error.TestFail(error_msg)
            if term in pass_terms and len(log_entries) == 0:
                logging.info('Did not find "%s"!', term)
                raise error.TestFail('Touch firmware did not attempt update.')

    def run_once(self, input_type='touchpad'):
        """Entry point of this test."""
        if not self.player.has(input_type):
            raise error.TestError('No %s found on this device!' % input_type)

        # Skip run on invalid touch inputs.
        if self._platform in self._INVALID_BOARDS:
            logging.info('This touchpad is not supported for this test.')
            return

        self._check_updates(input_type)
