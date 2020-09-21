# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging

from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib.cros import cr50_utils, tpm_utils
from autotest_lib.server.cros.faft.cr50_test import Cr50Test


class firmware_Cr50SetBoardId(Cr50Test):
    """Verify cr50-set-board-id.sh

    Verify cr50-set-board-id sets the correct board id and flags based on the
    given stage.
    """
    version = 1

    BID_SCRIPT = '/usr/share/cros/cr50-set-board-id.sh'

    # the command to get the brand name
    GET_BRAND = 'mosys platform brand'

    # Used when the flags were not initialized in the factory.
    UNKNOWN_FLAGS = 0xff00
    # Used for dev, proto, EVT, and DVT phases.
    DEVELOPMENT_FLAGS = 0xff7f
    # Used for PVT and MP builds.
    RELEASE_FLAGS = 0x7f80
    PHASE_FLAGS_DICT = {
        'unknown' : UNKNOWN_FLAGS,

        'dev' : DEVELOPMENT_FLAGS,
        'proto' : DEVELOPMENT_FLAGS,
        'evt' : DEVELOPMENT_FLAGS,
        'dvt' : DEVELOPMENT_FLAGS,

        'mp' : RELEASE_FLAGS,
        'pvt' : RELEASE_FLAGS,
    }

    # The response strings from cr50-set-board-id
    SUCCESS = ["Successfully updated board ID to 'BID' with phase 'PHASE'.",
               0]
    ERROR_UNKNOWN_PHASE = ['Unknown phase (PHASE)', 1]
    ERROR_INVALID_RLZ = ['Invalid RLZ brand code (BID).', 1]
    ERROR_ALREADY_SET = ['Board ID and flag have already been set.', 2]
    ERROR_BID_SET_DIFFERENTLY = ['Board ID has been set differently.', 3]
    ERROR_FLAG_SET_DIFFERENTLY = ['Flag has been set differently.', 3]

    def initialize(self, host, cmdline_args, dev_path='', bid=''):
        # Restore the original image, rlz code, and board id during cleanup.
        super(firmware_Cr50SetBoardId, self).initialize(host, cmdline_args,
            restore_cr50_state=True, cr50_dev_path=dev_path)
        if self.cr50.using_ccd():
            raise error.TestNAError('Use a flex cable instead of CCD cable.')

        # Update to the dev image so we can erase the board id after we set it.
        # This test is verifying cr50-set-board-id and not the actual getting/
        # setting of the board id on the cr50 side, so it is ok if the cr50 is
        # running a dev image.
        self.cr50_update(self.get_saved_cr50_dev_path())

        if not self.cr50.has_command('bid'):
            raise error.TestNAError('Cr50 image does not support board id')

        if not self.host.path_exists(self.BID_SCRIPT):
            raise error.TestNAError('Device does not have "cr50-set-board-id"')

        result = self.host.run(self.GET_BRAND, ignore_status=True)
        platform_brand = result.stdout.strip()
        if result.exit_status or not platform_brand:
            raise error.TestNAError('Could not get "mosys platform brand"')
        self.platform_brand = platform_brand
        self.erase_bid()
        cr50_utils.StopTrunksd(self.host)


    def cleanup(self):
        """Clear the TPM Owner"""
        super(firmware_Cr50SetBoardId, self).cleanup()
        tpm_utils.ClearTPMOwnerRequest(self.host)


    def erase_bid(self):
        """Erase the current board id"""
        self.cr50.send_command('eraseflashinfo')


    def run_script(self, expected_result, phase, board_id=''):
        """Run the bid script with the given phase and board id

        Args:
            expected_result: a list with the result message and exit status
            phase: The phase string.
            board_id: The board id string.

        Raises:
            TestFail if the expected result message did not match the script
            output
        """
        message, exit_status = expected_result

        # If we expect an error ignore the exit status
        ignore_status = not not exit_status

        # Run the script with the phase and board id
        cmd = '%s %s %s' % (self.BID_SCRIPT, phase, board_id)
        result = self.host.run(cmd, ignore_status=ignore_status)

        # If we don't give the script a board id, it will use the platform
        # brand
        expected_board_id = board_id if board_id else self.platform_brand
        # Replace the placeholders with the expected board id and phase
        message = message.replace('BID', expected_board_id)
        message = message.replace('PHASE', phase)

        # Compare the expected script output to the actual script result
        if message not in result.stdout or exit_status != result.exit_status:
            logging.debug(result)
            raise error.TestFail('Expected "%s" got "%s"' % (message,
                                 result.stdout))
        logging.info(result.stdout)


    def run_once(self):
        """Verify cr50-set-board-id.sh"""
        # 'A' is too short to be a valid rlz code
        self.run_script(self.ERROR_INVALID_RLZ, 'dvt', 'A')
        # dummy_phase is not a valid phase
        self.run_script(self.ERROR_UNKNOWN_PHASE, 'dummy_phase')
        # The rlz code is checked before the phase
        self.run_script(self.ERROR_INVALID_RLZ, 'dummy_phase', 'A')

        # Set the board id so we can verify cr50-set-board-id has the correct
        # response to the board id already being set.
        self.run_script(self.SUCCESS, 'dvt', 'TEST')
        # mp has different flags than dvt
        self.run_script(self.ERROR_FLAG_SET_DIFFERENTLY, 'mp', 'TEST')
        # try setting the dvt flags with a different board id
        self.run_script(self.ERROR_BID_SET_DIFFERENTLY, 'dvt', 'test')
        # running with the same phase and board id will raise an error that the
        # board id is already set
        self.run_script(self.ERROR_ALREADY_SET, 'dvt', 'TEST')

        # Verify each stage sets the right flags
        for phase, flags in self.PHASE_FLAGS_DICT.iteritems():
            # Erase the board id so we can change it.
            self.erase_bid()

            # Run the script to set the board id and flags for the given phase.
            self.run_script(self.SUCCESS, phase)

            # Check that the board id and flags are actually set.
            cr50_utils.CheckChipBoardId(self.host, self.platform_brand, flags)
