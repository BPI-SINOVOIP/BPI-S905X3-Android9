# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""This module contains Google Storage utilities.

TODO(ayatane): This should be merged into chromite.lib.gs
"""

import re

# See https://cloud.google.com/storage/docs/bucket-naming#objectnames
# Banned characters: []*?#
# and control characters (hex): 00-1f, 7f-84, 86-ff
_INVALID_GS_PATTERN = r'[[\]*?#\x00-\x1f\x7f-\x84\x86-\xff]'


def escape(name):
    """Escape GS object name.

    @param name: Name string.
    @return: Escaped name string.
    """
    return re.sub(_INVALID_GS_PATTERN,
                  lambda x: _percent_escape(x.group(0)), name)


def _percent_escape(char):
    """Percent escape a character.

    @param char: character to escape.
    @return: Escaped string.
    """
    return '%{:02x}'.format(ord(char))
