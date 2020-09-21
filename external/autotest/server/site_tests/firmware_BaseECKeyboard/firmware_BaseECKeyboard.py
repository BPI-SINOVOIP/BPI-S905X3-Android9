# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import time
from threading import Timer

from autotest_lib.client.bin.input import linux_input
from autotest_lib.client.common_lib import error
from autotest_lib.server.cros.faft.firmware_test import FirmwareTest


class firmware_BaseECKeyboard(FirmwareTest):
    """Servo-based BaseEC keyboard test.

    The base should be connected to the servo v4 board through an extra
    micro-servo. It talks to the base EC to emulate key-press.
    """
    version = 1

    # Delay to ensure client is ready to read the key press.
    KEY_PRESS_DELAY = 2

    # Delay to wait until the UI starts.
    START_UI_DELAY = 1

    # Delay to wait until developer console is open.
    DEV_CONSOLE_DELAY = 2


    def initialize(self, host, cmdline_args):
        super(firmware_BaseECKeyboard, self).initialize(host, cmdline_args)
        # Don't require USB disk
        self.setup_usbkey(usbkey=False)
        # Only run in normal mode
        self.switcher.setup_mode('normal')


    def cleanup(self):
        # Restart UI anyway, in case the test failed in the middle
        try:
            self.faft_client.system.run_shell_command('start ui | true')
        except Exception as e:
            logging.error("Caught exception: %s", str(e))
        super(firmware_BaseECKeyboard, self).cleanup()


    def _base_keyboard_checker(self, press_action):
        """Press key and check from DUT.

        Args:
          press_action: A callable that would press the keys when called.

        Returns:
          True if passed; or False if failed.
        """
        # Stop UI so that key presses don't go to Chrome.
        self.faft_client.system.run_shell_command('stop ui')

        # Start a thread to perform the key-press action
        Timer(self.KEY_PRESS_DELAY, press_action).start()

        # Invoke client side script to monitor keystrokes.
        # The codes are linux input event codes.
        # The order doesn't matter.
        result = self.faft_client.system.check_keys([
                linux_input.KEY_ENTER,
                linux_input.KEY_LEFTCTRL,
                linux_input.KEY_D])

        # Turn UI back on
        self.faft_client.system.run_shell_command('start ui')
        time.sleep(self.START_UI_DELAY)

        return result


    def keyboard_checker(self):
        """Press 'd', Ctrl, ENTER by servo and check from DUT."""

        def keypress():
            self.servo.enter_key()
            self.servo.ctrl_d()

        return self._base_keyboard_checker(keypress)


    def switch_tty2(self):
        """Switch to tty2 console."""
        self.base_ec.key_down('<ctrl_l>')
        self.base_ec.key_down('<alt_l>')
        self.base_ec.key_down('<f2>')
        self.base_ec.key_up('<f2>')
        self.base_ec.key_up('<alt_l>')
        self.base_ec.key_up('<ctrl_l>')
        time.sleep(self.DEV_CONSOLE_DELAY)


    def reboot_by_keyboard(self):
        """Reboot DUT by keyboard.

        Simulate key press sequence to log into console and then issue reboot
        command.
        """
        # Assume that DUT runs a test image, which has tty2 console and root
        # access.
        self.switch_tty2()
        self.base_ec.send_key_string('root<enter>')
        self.base_ec.send_key_string('test0000<enter>')
        self.base_ec.send_key_string('reboot<enter>')


    def run_once(self):
        if not self.base_ec:
            raise error.TestError('The base not found on servo. Wrong setup?')

        logging.info('Testing keypress by servo...')
        self.check_state(self.keyboard_checker)

        logging.info('Use key press simulation to issue reboot command...')
        self.switcher.mode_aware_reboot('custom', self.reboot_by_keyboard)
