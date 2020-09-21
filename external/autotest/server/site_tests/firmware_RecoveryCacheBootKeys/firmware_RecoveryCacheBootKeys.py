# Copyright (c) 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import re
import time

from autotest_lib.client.common_lib import error
from autotest_lib.server.cros.faft.firmware_test import FirmwareTest

class firmware_RecoveryCacheBootKeys(FirmwareTest):
    """
    This test ensures that when booting to recovery mode the device will use the
    cache instead training memory every boot.
    """
    version = 1

    USED_CACHE_MSG = ('MRC: Hash comparison successful. '
                      'Using data from RECOVERY_MRC_CACHE')
    REBUILD_CACHE_MSG = "MRC: cache data 'RECOVERY_MRC_CACHE' needs update."
    RECOVERY_CACHE_SECTION = 'RECOVERY_MRC_CACHE'
    FIRMWARE_LOG_CMD = 'cbmem -c'
    FMAP_CMD = 'mosys eeprom map'
    RECOVERY_REASON_REBUILD_CMD = 'crossystem recovery_request=0xC4'
    APSHUTDOWN_DELAY = 5

    def initialize(self, host, cmdline_args, dev_mode=False):
        super(firmware_RecoveryCacheBootKeys, self).initialize(host,
                                                              cmdline_args)
        self.client = host
        self.dev_mode = dev_mode
        self.switcher.setup_mode('dev' if dev_mode else 'normal')
        self.setup_usbkey(usbkey=True, host=False)

    def cleanup(self):
        super(firmware_RecoveryCacheBootKeys, self).cleanup()

    def boot_to_recovery(self):
        """Boot device into recovery mode."""
        self.switcher.reboot_to_mode(to_mode='rec')

        self.check_state((self.checkers.crossystem_checker,
                          {'mainfw_type': 'recovery'}))

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

    def cache_exist(self):
        """Checks the firmware log to ensure that the recovery cache exists.

        @return True if cache exists
        """
        logging.info("Checking if device has RECOVERY_MRC_CACHE")

        return self.check_command_output(self.FMAP_CMD,
                                         self.RECOVERY_CACHE_SECTION)

    def check_cache_used(self):
        """Checks the firmware log to ensure that the recovery cache was used
        during recovery boot.

        @return True if cache used
        """
        logging.info('Checking if cache was used.')

        return self.check_command_output(self.FIRMWARE_LOG_CMD,
                                         self.USED_CACHE_MSG)

    def check_cache_rebuilt(self):
        """Checks the firmware log to ensure that the recovery cache was rebuilt
        during recovery boot.

        @return True if cache rebuilt
        """
        logging.info('Checking if cache was rebuilt.')

        return self.check_command_output(self.FIRMWARE_LOG_CMD,
                                         self.REBUILD_CACHE_MSG)



    def run_once(self):
        if not self.cache_exist():
            raise error.TestNAError('No RECOVERY_MRC_CACHE was found on DUT.')

        logging.info('Ensure we\'ve done memory training.')
        self.boot_to_recovery()

        logging.info('Checking 3-Key recovery boot.')
        self.boot_to_recovery()

        if not self.check_cache_used():
            raise error.TestFail('[3-Key] - Recovery Cache was not used.')

        logging.info('Checking 4-key recovery rebuilt cache boot.')
        self.ec.send_command_get_output(
            'apshutdown',
            ["\[[0-9\.]+ chipset_force_shutdown\(\)"])
        self.ec.send_command_get_output('hostevent set 0x20004000',
                                        ["Events:\s+0x0000000020004000"])
        time.sleep(self.APSHUTDOWN_DELAY)
        self.ec.send_command('powerbtn')
        self.switcher.wait_for_client()

        if not self.check_cache_rebuilt():
            raise error.TestFail('[4-key] - Recovery Cache was not rebuilt.')

        logging.info('Reboot out of Recovery')
        self.switcher.simple_reboot()

