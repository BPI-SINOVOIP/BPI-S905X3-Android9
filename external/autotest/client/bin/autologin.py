#!/usr/bin/python
#
# Copyright 2016 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

'''Make Chrome automatically log in.'''

# This sets up import paths for autotest.
import common

import argparse
import sys

from autotest_lib.client.common_lib.cros import chrome


def main(args):
    '''The main function.

    @param args: list of string args passed to program
    '''

    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('-a', '--arc', action='store_true',
                        help='Enable ARC and wait for it to start.')
    parser.add_argument('-d', '--dont_override_profile', action='store_true',
                        help='Keep files from previous sessions.')
    args = parser.parse_args(args)

    # Avoid calling close() on the Chrome object; this keeps the session active.
    chrome.Chrome(
        arc_mode=('enabled' if args.arc else None),
        dont_override_profile=args.dont_override_profile)


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
