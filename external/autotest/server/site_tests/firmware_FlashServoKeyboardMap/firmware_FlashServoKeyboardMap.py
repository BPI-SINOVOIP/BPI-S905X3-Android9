# Copyright 2016 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import os
import time

from autotest_lib.client.common_lib import error
from autotest_lib.server import test

class firmware_FlashServoKeyboardMap(test.test):
    """A test to flash the keyboard map on servo v3."""
    version = 1

    _ATMEGA_RESET_DELAY = 0.2
    _USB_PRESENT_DELAY = 1

    def run_once(self, host=None):
        servo = host.servo
        if host.run('hash dfu-programmer', ignore_status=True).exit_status:
            raise error.TestError(
                    'The image is too old that does not have dfu-programmer.')

        try:
            # Reset the chip and switch mux to let DUT to see the chip.
            servo.set_get_all(['at_hwb:off',
                               'atmega_rst:on',
                               'sleep:%f' % self._ATMEGA_RESET_DELAY,
                               'atmega_rst:off',
                               'usb_mux_sel4:on'])

            # Check the result of lsusb.
            time.sleep(self._USB_PRESENT_DELAY)
            result = host.run('lsusb -d 03eb:').stdout.strip()
            if 'LUFA Keyboard Demo' in result:
                logging.info('Already use the new keyboard map.')
                return

            if not 'Atmel Corp. atmega32u4 DFU bootloader' in result:
                message = 'Not an expected chip: %s' % result
                logging.error(message)
                raise error.TestFail(message)

            # Update the keyboard map.
            local_path = os.path.join(self.bindir, 'test_data', 'keyboard.hex')
            host.send_file(local_path, '/tmp')
            logging.info('Updating the keyboard map...')
            host.run('dfu-programmer atmega32u4 erase --force')
            host.run('dfu-programmer atmega32u4 flash /tmp/keyboard.hex')

            # Reset the chip.
            servo.set_get_all(['atmega_rst:on',
                               'sleep:%f' % self._ATMEGA_RESET_DELAY,
                               'atmega_rst:off'])

            # Check the result of lsusb.
            time.sleep(self._USB_PRESENT_DELAY)
            result = host.run('lsusb -d 03eb:').stdout.strip()
            if 'LUFA Keyboard Demo' in result:
                logging.info('Update successfully!')
            else:
                message = 'Update failed; got the result: %s' % result
                logging.error(message)
                raise error.TestFail(message)

        finally:
            # Restore the default settings.
            servo.set('usb_mux_sel4', 'off')
