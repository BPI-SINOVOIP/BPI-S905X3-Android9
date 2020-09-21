# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import ast
import functools
import logging
import re
import time

from autotest_lib.client.bin import utils
from autotest_lib.client.common_lib import error
from autotest_lib.client.cros import ec

# Hostevent codes, copied from:
#     ec/include/ec_commands.h
HOSTEVENT_LID_CLOSED        = 0x00000001
HOSTEVENT_LID_OPEN          = 0x00000002
HOSTEVENT_POWER_BUTTON      = 0x00000004
HOSTEVENT_AC_CONNECTED      = 0x00000008
HOSTEVENT_AC_DISCONNECTED   = 0x00000010
HOSTEVENT_BATTERY_LOW       = 0x00000020
HOSTEVENT_BATTERY_CRITICAL  = 0x00000040
HOSTEVENT_BATTERY           = 0x00000080
HOSTEVENT_THERMAL_THRESHOLD = 0x00000100
HOSTEVENT_THERMAL_OVERLOAD  = 0x00000200
HOSTEVENT_THERMAL           = 0x00000400
HOSTEVENT_USB_CHARGER       = 0x00000800
HOSTEVENT_KEY_PRESSED       = 0x00001000
HOSTEVENT_INTERFACE_READY   = 0x00002000
# Keyboard recovery combo has been pressed
HOSTEVENT_KEYBOARD_RECOVERY = 0x00004000
# Shutdown due to thermal overload
HOSTEVENT_THERMAL_SHUTDOWN  = 0x00008000
# Shutdown due to battery level too low
HOSTEVENT_BATTERY_SHUTDOWN  = 0x00010000
HOSTEVENT_INVALID           = 0x80000000

# Time to wait after sending keypress commands.
KEYPRESS_RECOVERY_TIME = 0.5


class ChromeConsole(object):
    """Manages control of a Chrome console.

    We control the Chrome console via the UART of a Servo board. Chrome console
    provides many interfaces to set and get its behavior via console commands.
    This class is to abstract these interfaces.
    """

    CMD = "_cmd"
    REGEXP = "_regexp"
    MULTICMD = "_multicmd"

    def __init__(self, servo, name):
        """Initialize and keep the servo object.

        Args:
          servo: A Servo object.
          name: The console name.
        """
        self.name = name
        self.uart_cmd = self.name + self.CMD
        self.uart_regexp = self.name + self.REGEXP
        self.uart_multicmd = self.name + self.MULTICMD

        self._servo = servo
        self._cached_uart_regexp = None


    def set_uart_regexp(self, regexp):
        if self._cached_uart_regexp == regexp:
            return
        self._cached_uart_regexp = regexp
        self._servo.set(self.uart_regexp, regexp)


    def send_command(self, commands):
        """Send command through UART.

        This function opens UART pty when called, and then command is sent
        through UART.

        Args:
          commands: The commands to send, either a list or a string.
        """
        self.set_uart_regexp('None')
        if isinstance(commands, list):
            try:
                self._servo.set_nocheck(self.uart_multicmd, ';'.join(commands))
            except error.TestFail as e:
                if 'No control named' in str(e):
                    logging.warning(
                            'The servod is too old that uart_multicmd '
                            'not supported. Use uart_cmd instead.')
                    for command in commands:
                        self._servo.set_nocheck(self.uart_cmd, command)
                else:
                    raise
        else:
            self._servo.set_nocheck(self.uart_cmd, commands)


    def send_command_get_output(self, command, regexp_list):
        """Send command through UART and wait for response.

        This function waits for response message matching regular expressions.

        Args:
          command: The command sent.
          regexp_list: List of regular expressions used to match response
            message. Note, list must be ordered.

        Returns:
          List of tuples, each of which contains the entire matched string and
          all the subgroups of the match. None if not matched.
          For example:
            response of the given command:
              High temp: 37.2
              Low temp: 36.4
            regexp_list:
              ['High temp: (\d+)\.(\d+)', 'Low temp: (\d+)\.(\d+)']
            returns:
              [('High temp: 37.2', '37', '2'), ('Low temp: 36.4', '36', '4')]

        Raises:
          error.TestError: An error when the given regexp_list is not valid.
        """
        if not isinstance(regexp_list, list):
            raise error.TestError('Arugment regexp_list is not a list: %s' %
                                  str(regexp_list))

        self.set_uart_regexp(str(regexp_list))
        self._servo.set_nocheck(self.uart_cmd, command)
        return ast.literal_eval(self._servo.get(self.uart_cmd))


class ChromeEC(ChromeConsole):
    """Manages control of a Chrome EC.

    We control the Chrome EC via the UART of a Servo board. Chrome EC
    provides many interfaces to set and get its behavior via console commands.
    This class is to abstract these interfaces.
    """

    def __init__(self, servo, name="ec_uart"):
        super(ChromeEC, self).__init__(servo, name)


    def key_down(self, keyname):
        """Simulate pressing a key.

        Args:
          keyname: Key name, one of the keys of KEYMATRIX.
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
        # Don't spam the EC console as fast as we can; leave some recovery time
        # in between commands.
        time.sleep(KEYPRESS_RECOVERY_TIME)


    def send_key_string_raw(self, string):
        """Send key strokes consisting of only characters.

        Args:
          string: Raw string.
        """
        for c in string:
            self.key_press(c)


    def send_key_string(self, string):
        """Send key strokes including special keys.

        Args:
          string: Character string including special keys. An example
            is "this is an<tab>example<enter>".
        """
        for m in re.finditer("(<[^>]+>)|([^<>]+)", string):
            sp, raw = m.groups()
            if raw is not None:
                self.send_key_string_raw(raw)
            else:
                self.key_press(sp)


    def reboot(self, flags=''):
        """Reboot EC with given flags.

        Args:
          flags: Optional, a space-separated string of flags passed to the
                 reboot command, including:
                   default: EC soft reboot;
                   'hard': EC hard/cold reboot;
                   'ap-off': Leave AP off after EC reboot (by default, EC turns
                             AP on after reboot if lid is open).

        Raises:
          error.TestError: If the string of flags is invalid.
        """
        for flag in flags.split():
            if flag not in ('hard', 'ap-off'):
                raise error.TestError(
                        'The flag %s of EC reboot command is invalid.' % flag)
        self.send_command("reboot %s" % flags)


    def set_flash_write_protect(self, enable):
        """Set the software write protect of EC flash.

        Args:
          enable: True to enable write protect, False to disable.
        """
        if enable:
            self.send_command("flashwp enable")
        else:
            self.send_command("flashwp disable")


    def set_hostevent(self, codes):
        """Set the EC hostevent codes.

        Args:
          codes: Hostevent codes, HOSTEVENT_*
        """
        self.send_command("hostevent set %#x" % codes)
        # Allow enough time for EC to process input and set flag.
        # See chromium:371631 for details.
        # FIXME: Stop importing time module if this hack becomes obsolete.
        time.sleep(1)


    def enable_console_channel(self, channel):
        """Find console channel mask and enable that channel only

        @param channel: console channel name
        """
        # The 'chan' command returns a list of console channels,
        # their channel masks and channel numbers
        regexp = r'(\d+)\s+([\w]+)\s+\*?\s+{0}'.format(channel)
        l = self.send_command_get_output('chan', [regexp])
        # Use channel mask and append the 0x for proper hex input value
        cmd = 'chan 0x' + l[0][2]
        # Set console to only output the desired channel
        self.send_command(cmd)


class ChromeUSBPD(ChromeEC):
    """Manages control of a Chrome USBPD.

    We control the Chrome EC via the UART of a Servo board. Chrome USBPD
    provides many interfaces to set and get its behavior via console commands.
    This class is to abstract these interfaces.
    """

    def __init__(self, servo):
        super(ChromeUSBPD, self).__init__(servo, "usbpd_uart")
