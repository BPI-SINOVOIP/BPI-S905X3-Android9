# -*- coding:utf-8 -*-
# Copyright 2016 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

"""Terminal utilities

This module handles terminal interaction including ANSI color codes.
"""

from __future__ import print_function

import os
import sys

_path = os.path.realpath(__file__ + '/../..')
if sys.path[0] != _path:
    sys.path.insert(0, _path)
del _path

# pylint: disable=wrong-import-position
import rh.shell


class Color(object):
    """Conditionally wraps text in ANSI color escape sequences."""

    BLACK, RED, GREEN, YELLOW, BLUE, MAGENTA, CYAN, WHITE = range(8)
    BOLD = -1
    COLOR_START = '\033[1;%dm'
    BOLD_START = '\033[1m'
    RESET = '\033[0m'

    def __init__(self, enabled=None):
        """Create a new Color object, optionally disabling color output.

        Args:
          enabled: True if color output should be enabled.  If False then this
              class will not add color codes at all.
        """
        self._enabled = enabled

    def start(self, color):
        """Returns a start color code.

        Args:
          color: Color to use, .e.g BLACK, RED, etc.

        Returns:
          If color is enabled, returns an ANSI sequence to start the given
          color, otherwise returns empty string
        """
        if self.enabled:
            return self.COLOR_START % (color + 30)
        return ''

    def stop(self):
        """Returns a stop color code.

        Returns:
          If color is enabled, returns an ANSI color reset sequence, otherwise
          returns empty string
        """
        if self.enabled:
            return self.RESET
        return ''

    def color(self, color, text):
        """Returns text with conditionally added color escape sequences.

        Args:
          color: Text color -- one of the color constants defined in this class.
          text: The text to color.

        Returns:
          If self._enabled is False, returns the original text.  If it's True,
          returns text with color escape sequences based on the value of color.
        """
        if not self.enabled:
            return text
        if color == self.BOLD:
            start = self.BOLD_START
        else:
            start = self.COLOR_START % (color + 30)
        return start + text + self.RESET

    @property
    def enabled(self):
        """See if the colorization is enabled."""
        if self._enabled is None:
            if 'NOCOLOR' in os.environ:
                self._enabled = not rh.shell.boolean_shell_value(
                    os.environ['NOCOLOR'], False)
            else:
                self._enabled = is_tty(sys.stderr)
        return self._enabled


def is_tty(fh):
    """Returns whether the specified file handle is a TTY.

    Args:
      fh: File handle to check.

    Returns:
      True if |fh| is a TTY
    """
    try:
        return os.isatty(fh.fileno())
    except IOError:
        return False


def print_status_line(line, print_newline=False):
    """Clears the current terminal line, and prints |line|.

    Args:
      line: String to print.
      print_newline: Print a newline at the end, if sys.stderr is a TTY.
    """
    if is_tty(sys.stderr):
        output = '\r' + line + '\x1B[K'
        if print_newline:
            output += '\n'
    else:
        output = line + '\n'

    sys.stderr.write(output)
    sys.stderr.flush()


def get_input(prompt):
    """Python 2/3 glue for raw_input/input differences."""
    try:
        return raw_input(prompt)
    except NameError:
        # Python 3 renamed raw_input() to input(), which is safe to call since
        # it does not evaluate the input.
        # pylint: disable=bad-builtin
        return input(prompt)


def boolean_prompt(prompt='Do you want to continue?', default=True,
                   true_value='yes', false_value='no', prolog=None):
    """Helper function for processing boolean choice prompts.

    Args:
      prompt: The question to present to the user.
      default: Boolean to return if the user just presses enter.
      true_value: The text to display that represents a True returned.
      false_value: The text to display that represents a False returned.
      prolog: The text to display before prompt.

    Returns:
      True or False.
    """
    true_value, false_value = true_value.lower(), false_value.lower()
    true_text, false_text = true_value, false_value
    if true_value == false_value:
        raise ValueError('true_value and false_value must differ: got %r'
                         % true_value)

    if default:
        true_text = true_text[0].upper() + true_text[1:]
    else:
        false_text = false_text[0].upper() + false_text[1:]

    prompt = ('\n%s (%s/%s)? ' % (prompt, true_text, false_text))

    if prolog:
        prompt = ('\n%s\n%s' % (prolog, prompt))

    while True:
        try:
            response = get_input(prompt).lower()
        except EOFError:
            # If the user hits CTRL+D, or stdin is disabled, use the default.
            print()
            response = None
        except KeyboardInterrupt:
            # If the user hits CTRL+C, just exit the process.
            print()
            raise

        if not response:
            return default
        if true_value.startswith(response):
            if not false_value.startswith(response):
                return True
            # common prefix between the two...
        elif false_value.startswith(response):
            return False
