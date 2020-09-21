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

"""Functions for working with shell code."""

from __future__ import print_function

import os
import sys

_path = os.path.realpath(__file__ + '/../..')
if sys.path[0] != _path:
    sys.path.insert(0, _path)
del _path


# For use by ShellQuote.  Match all characters that the shell might treat
# specially.  This means a number of things:
#  - Reserved characters.
#  - Characters used in expansions (brace, variable, path, globs, etc...).
#  - Characters that an interactive shell might use (like !).
#  - Whitespace so that one arg turns into multiple.
# See the bash man page as well as the POSIX shell documentation for more info:
#   http://www.gnu.org/software/bash/manual/bashref.html
#   http://pubs.opengroup.org/onlinepubs/9699919799/utilities/V3_chap02.html
_SHELL_QUOTABLE_CHARS = frozenset('[|&;()<> \t!{}[]=*?~$"\'\\#^')
# The chars that, when used inside of double quotes, need escaping.
# Order here matters as we need to escape backslashes first.
_SHELL_ESCAPE_CHARS = r'\"`$'


def shell_quote(s):
    """Quote |s| in a way that is safe for use in a shell.

    We aim to be safe, but also to produce "nice" output.  That means we don't
    use quotes when we don't need to, and we prefer to use less quotes (like
    putting it all in single quotes) than more (using double quotes and escaping
    a bunch of stuff, or mixing the quotes).

    While python does provide a number of alternatives like:
     - pipes.quote
     - shlex.quote
    They suffer from various problems like:
     - Not widely available in different python versions.
     - Do not produce pretty output in many cases.
     - Are in modules that rarely otherwise get used.

    Note: We don't handle reserved shell words like "for" or "case".  This is
    because those only matter when they're the first element in a command, and
    there is no use case for that.  When we want to run commands, we tend to
    run real programs and not shell ones.

    Args:
      s: The string to quote.

    Returns:
      A safely (possibly quoted) string.
    """
    s = s.encode('utf-8')

    # See if no quoting is needed so we can return the string as-is.
    for c in s:
        if c in _SHELL_QUOTABLE_CHARS:
            break
    else:
        if not s:
            return "''"
        else:
            return s

    # See if we can use single quotes first.  Output is nicer.
    if "'" not in s:
        return "'%s'" % s

    # Have to use double quotes.  Escape the few chars that still expand when
    # used inside of double quotes.
    for c in _SHELL_ESCAPE_CHARS:
        if c in s:
            s = s.replace(c, r'\%s' % c)
    return '"%s"' % s


def shell_unquote(s):
    """Do the opposite of ShellQuote.

    This function assumes that the input is a valid escaped string.
    The behaviour is undefined on malformed strings.

    Args:
      s: An escaped string.

    Returns:
      The unescaped version of the string.
    """
    if not s:
        return ''

    if s[0] == "'":
        return s[1:-1]

    if s[0] != '"':
        return s

    s = s[1:-1]
    output = ''
    i = 0
    while i < len(s) - 1:
        # Skip the backslash when it makes sense.
        if s[i] == '\\' and s[i + 1] in _SHELL_ESCAPE_CHARS:
            i += 1
        output += s[i]
        i += 1
    return output + s[i] if i < len(s) else output


def cmd_to_str(cmd):
    """Translate a command list into a space-separated string.

    The resulting string should be suitable for logging messages and for
    pasting into a terminal to run.  Command arguments are surrounded by
    quotes to keep them grouped, even if an argument has spaces in it.

    Examples:
      ['a', 'b'] ==> "'a' 'b'"
      ['a b', 'c'] ==> "'a b' 'c'"
      ['a', 'b\'c'] ==> '\'a\' "b\'c"'
      [u'a', "/'$b"] ==> '\'a\' "/\'$b"'
      [] ==> ''
      See unittest for additional (tested) examples.

    Args:
      cmd: List of command arguments.

    Returns:
      String representing full command.
    """
    # Use str before repr to translate unicode strings to regular strings.
    return ' '.join(shell_quote(arg) for arg in cmd)


def boolean_shell_value(sval, default):
    """See if |sval| is a value users typically consider as boolean."""
    if sval is None:
        return default

    if isinstance(sval, basestring):
        s = sval.lower()
        if s in ('yes', 'y', '1', 'true'):
            return True
        elif s in ('no', 'n', '0', 'false'):
            return False

    raise ValueError('Could not decode as a boolean value: %r' % (sval,))
