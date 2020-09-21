# Copyright 2018 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging

from autotest_lib.client.common_lib import error
from autotest_lib.server.cros.faft.firmware_test import FirmwareTest

BIOS_PATH = '/tmp/test.firmware_Bmpblk.bios.bin'

class firmware_Bmpblk(FirmwareTest):
    """
    This test checks whether the BIOS was built with a correctly configured
    bmpblk to ensure crisp firmware screen images and text. The bmpblk for every
    device needs to be explicitly configured for the device's screen resolution
    to ensure optimal quality. Relies on flashrom and cbfstool to inspect BIOS.
    """
    version = 1

    def run_once(self):
        self.faft_client.bios.dump_whole(BIOS_PATH)
        try:
            files = self.faft_client.system.run_shell_command_get_output(
                    'cbfstool %s print' % BIOS_PATH)
            files = '\n'.join(files)
            logging.debug('cbfstool print output:\n\n%s', files)
            if 'ramstage' not in files:
                raise error.TestError("Sanity check failed: Can't read CBFS")
            if 'vbgfx.bin' not in files:
                raise error.TestNAError('This board has no firmware screens')
            if 'vbgfx_not_scaled' in files:
                logging.error("This firmware's bmpblk was built for a generic "
                              "1366x768 display resolution. Images will get "
                              "scaled up at runtime and look blurry. You need "
                              "to explicitly set the panel resolution for this "
                              "board in bmpblk/images/boards.yaml and add it "
                              "to CROS_BOARDS in the sys-boot/chromeos-bmpblk"
                              ".ebuild. Do *not* do this until you are certain "
                              "of the panel resolution that the final product "
                              "will ship with!")
                raise error.TestFail('bmpblk images not scaled for this board')
        finally:
            self.faft_client.system.remove_file(BIOS_PATH)
