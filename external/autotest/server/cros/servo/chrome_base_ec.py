# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import re

from autotest_lib.client.cros import ec
from autotest_lib.server.cros.servo import chrome_ec


class ChromeBaseEC(chrome_ec.ChromeConsole):
    """Manages control of a ChromeBaseEC.

    The ChromeBaseEC object should be instantiated via the create_base_ec()
    method which checks the board name.

    There are several ways to control the Base EC, depending on the setup.
    To simplify, this class assumes the Base EC is connected to a servo-micro
    flex to a servo v4 board. The main EC is also connected to the same servo
    v4 board via either another servo-micro flex or the type-C CCD cable.

    """

    def __init__(self, servo, board):
        """Initialize the object.

        Args:
          servo: An autotest_lib.server.cros.servo.Servo object.
          board: A string of the board name for the base, e.g. "hammer".
        """
        self.board = board
        console_prefix = board + '_ec_uart'
        super(ChromeBaseEC, self).__init__(servo, console_prefix)


    def get_board(self):
        """Get the board name of the Base EC.

        Returns:
          A string of the board name.
        """
        return self.board


    def key_down(self, keyname):
        """Simulate pressing a key.

        Args:
          keyname: Key name, one of the keys of ec.KEYMATRIX.
        """
        self.send_command('kbpress %d %d 1' %
                (ec.KEYMATRIX[keyname][1], ec.KEYMATRIX[keyname][0]))


    def key_up(self, keyname):
        """Simulate releasing a key.

        Args:
          keyname: Key name, one of the keys of KEYMATRIX.
        """
        self.send_command('kbpress %d %d 0' %
                (ec.KEYMATRIX[keyname][1], ec.KEYMATRIX[keyname][0]))


    def key_press(self, keyname):
        """Press and then release a key.

        Args:
          keyname: Key name, one of the keys of KEYMATRIX.
        """
        self.send_command([
                'kbpress %d %d 1' %
                    (ec.KEYMATRIX[keyname][1], ec.KEYMATRIX[keyname][0]),
                'kbpress %d %d 0' %
                    (ec.KEYMATRIX[keyname][1], ec.KEYMATRIX[keyname][0]),
                ])


    def send_key_string_raw(self, string):
        """Send key strokes consisting of only characters.

        Args:
          string: Raw string.
        """
        for c in string:
            self.key_press(c)


    def send_key_string(self, string):
        """Send key strokes that can include special keys.

        Args:
          string: Character string that can include special keys. An example
            is "this is an<tab>example<enter>".
        """
        for m in re.finditer("(<[^>]+>)|([^<>]+)", string):
            sp, raw = m.groups()
            if raw is not None:
                self.send_key_string_raw(raw)
            else:
                self.key_press(sp)


def create_base_ec(servo):
    """Create a Base EC object.

    It gets the base board name from servod and returns a ChromeBaseEC object
    of the board.

    Returns:
      A ChromeBaseEC object, or None if not found.
    """
    base_board = servo.get_base_board()
    if base_board:
        return ChromeBaseEC(servo, base_board)
    else:
        logging.warn('No Base EC found on the servo board')
        return None
