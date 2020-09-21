# Copyright 2018 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import re

from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib.cros import cr50_utils
from autotest_lib.server.cros.faft.cr50_test import Cr50Test


class firmware_Cr50GetName(Cr50Test):
    """Verify cr50-get-name.sh

    Verify cr50-get-name sets the correct board id and flags based on the
    given stage.
    """
    version = 1

    GET_NAME_SCRIPT = '/usr/share/cros/cr50-get-name.sh'
    # This translates to 'TEST'
    TEST_BRAND = 0x54455354
    MAX_VAL = 0xffffffff


    def initialize(self, host, cmdline_args, dev_path=''):
        # Restore the original image, rlz code, and board id during cleanup.
        super(firmware_Cr50GetName, self).initialize(host, cmdline_args,
            restore_cr50_state=True, cr50_dev_path=dev_path)

        if not self.host.path_exists(self.GET_NAME_SCRIPT):
            raise error.TestNAError('Device does not have "cr50-get-name"')

        # Update to the dev image so we can erase the board id after we set it.
        # This test is verifying cr50-get-name, so it is ok if cr50 is running a
        # dev image.
        self.cr50_update(self.get_saved_cr50_dev_path())

        # Stop trunksd so it wont interfere with the update
        cr50_utils.StopTrunksd(self.host)

        # Get the current cr50 update messages. The test will keep track of the
        # last message and separate the current output from actual test results.
        self.get_result()


    def erase_bid(self):
        """Erase the cr50 board id"""
        self.cr50.send_command('eraseflashinfo')


    def get_result(self):
        """Return the new cr50 update messages from /var/log/messages"""
        # Get the cr50 messages
        result = self.host.run('grep cr50 /var/log/messages').stdout.strip()

        if hasattr(self, '_last_message'):
            result = result.rsplit(self._last_message, 1)[-1]

        # Save the last line. It will be used to separate the current results
        # from later runs.
        self._last_message = result.rsplit('\n', 1)[-1]
        logging.debug('last cr50 update message: "%s"', self._last_message)
        return result


    def get_expected_result_re(self, brand, flags, erased):
        """Return the expected update message re given the test flags

        Args:
            brand: The board id value to test.
            flags: The flag value to test.
            erased: True if the board id is erased

        Returns:
            A string with info that must be found in /var/log/messages for valid
            update results.
        """
        expected_result = []

        if erased:
            board_id = 'ffffffff:ffffffff:ffffffff'
            # If the board id is erased, the device should update to the prod
            # image.
            ext = 'prod'
            expected_result.append('board ID is erased using prod image')
        else:
            board_id = '%08x:%08x:%08x' % (brand, brand ^ self.MAX_VAL, flags)
            ext = 'prepvt' if flags & 0x10 else 'prod'

        flag_str = board_id.rsplit(':', 1)[-1]

        expected_result.append("board_id: '%s' board_flags: '0x%s', extension: "
                "'%s'" % (board_id, flag_str, ext))
        expected_result.append('hashing /opt/google/cr50/firmware/cr50.bin.%s' %
                ext)
        return '(%s)' % '\n.*'.join(expected_result)


    def check_result(self, brand, flags, erased):
        """Verify the expected result string is found in the update messages

        Args:
            brand: The board id value to test.
            flags: The flag value to test.
            erased: True if the board id is erased

        Raises:
            TestFail if the expected result message did not match the update
            output
        """
        expected_result_re = self.get_expected_result_re(brand, flags, erased)
        result = self.get_result()
        match = re.search(expected_result_re, result)

        logging.debug('EXPECT: %s', expected_result_re)
        logging.debug('GOT: %s', result)

        if not match:
            raise error.TestFail('Unexpected result during update with %s' %
                    ('erased board id' if erased else '%x:%x' % (brand, flags)))

        logging.info('FOUND UPDATE RESULT:\n%s', match.groups()[0])


    def run_update(self, brand, flags, clear_bid=False):
        """Set the board id then run cr50-update

        Args:
            brand: The board id int to test.
            flags: The flag int to test.
            clear_bid: True if the board id should be erased and not reset.
        """
        # Set the board id
        self.erase_bid()

        if not clear_bid:
            self.cr50.send_command('bid 0x%x 0x%x' % (brand, flags))

        # Run the update script script
        self.host.run('start cr50-update')

        # Make sure cr50 used the right image.
        self.check_result(brand, flags, clear_bid)


    def run_once(self):
        """Verify cr50-get-name.sh"""
        # Test the MP flags
        self.run_update(self.TEST_BRAND, 0x7f00)

        # Test the pre-PVT flags
        self.run_update(self.TEST_BRAND, 0x7f10)

        # Erase the board id
        self.run_update(0, 0, clear_bid=True)

        # Make sure the script can tell the difference between an erased board
        # id and one set to 0xffffffff:0xffffffff.
        self.run_update(self.MAX_VAL, self.MAX_VAL)
