# Copyright (c) 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import re

from autotest_lib.client.common_lib import error
from autotest_lib.server.cros.faft.firmware_test import FirmwareTest
from autotest_lib.server.cros.faft.firmware_test import ConnectionError

class firmware_CorruptRecoveryCache(FirmwareTest):
    """
    This test corrupts RECOVERY_MRC_CACHE and makes sure the DUT recreates the
    cache and boots into recovery. This only applies to intel chips.

    The expected behavior is that if the RECOVERY_MRC_CACHE is corrupted then
    it will be recreated and still boot into recovery mode.
    """
    version = 1

    REBUILD_CACHE_MSG = "MRC: cache data 'RECOVERY_MRC_CACHE' needs update."
    RECOVERY_CACHE_SECTION = 'RECOVERY_MRC_CACHE'
    FIRMWARE_LOG_CMD = 'cbmem -c'
    FMAP_CMD = 'mosys eeprom map'

    def initialize(self, host, cmdline_args, dev_mode=False):
        super(firmware_CorruptRecoveryCache, self).initialize(host,
                                                              cmdline_args)
        self.backup_firmware()
        self.switcher.setup_mode('dev' if dev_mode else 'normal')
        self.setup_usbkey(usbkey=True, host=False)

    def cleanup(self):
        try:
            self.restore_firmware()
        except ConnectionError:
            logging.error("ERROR: DUT did not come up.  Need to cleanup!")
        super(firmware_CorruptRecoveryCache, self).cleanup()

    def cache_exist(self):
        """Checks the firmware log to ensure that the recovery cache exists.

        @return True if cache exists
        """
        logging.info("Checking if device has RECOVERY_MRC_CACHE")

        return self.check_command_output(self.FMAP_CMD,
                                         self.RECOVERY_CACHE_SECTION)

    def check_cache_rebuilt(self):
        """Checks the firmware log to ensure that the recovery cache was rebuilt
        successfully.

        @return True if cache rebuilt otherwise false
        """
        logging.info("Checking if cache was rebuilt.")

        return self.check_command_output(self.FIRMWARE_LOG_CMD,
                                         self.REBUILD_CACHE_MSG)

    def boot_to_recovery(self):
        """Boot device into recovery mode."""
        logging.info('Reboot into Recovery...')
        self.switcher.reboot_to_mode(to_mode='rec')

        self.check_state((self.checkers.crossystem_checker,
                          {'mainfw_type': 'recovery'}))

    def check_command_output(self, cmd, pattern):
        """Checks the output of the given command for the given pattern

        @param cmd: The command to run
        @param pattern: The pattern to search for
        @return True if pattern found
        """

        logging.info('Checking %s output for %s', cmd, pattern)

        checker = re.compile(pattern)

        cmd_output = self.run_command(cmd)

        for line in cmd_output:
            if re.search(checker, line):
                return True

        return False

    def run_command(self, command):
        """Runs the specified command and returns the output
        as a list of strings.

        @param command: The command to run on the DUT
        @return A list of strings of the command output
        """
        logging.info('Command to run: %s', command)

        output = self.faft_client.system.run_shell_command_get_output(command)

        logging.info('Command output: %s', output)

        return output

    def run_once(self):
        if not self.cache_exist():
            raise error.TestNAError('No RECOVERY_MRC_CACHE was found on DUT.')

        self.faft_client.bios.corrupt_body('rec', True)
        self.boot_to_recovery()

        if not self.check_cache_rebuilt():
            raise error.TestFail('Recovery Cache was not rebuilt.')

        logging.info('Reboot out of Recovery')
        self.switcher.simple_reboot()
