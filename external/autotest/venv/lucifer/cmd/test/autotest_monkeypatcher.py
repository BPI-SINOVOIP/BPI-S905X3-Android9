# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Monkeypatch autotest and import common.

This is used for testing Autotest monkeypatching.
"""

from __future__ import absolute_import
from __future__ import division
from __future__ import print_function

import sys

import lucifer.autotest
from lucifer import loglib


def main(args):
    """Main function

    @param args: list of command line args
    """
    del args
    loglib.configure_logging(name='autotest_monkeypatcher')

    lucifer.autotest.monkeypatch()
    from autotest_lib import common

    print(common.__file__)
    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
