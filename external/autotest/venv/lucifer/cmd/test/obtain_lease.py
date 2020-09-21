# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Obtain a lease file.

This is used for testing leasing.
"""

from __future__ import absolute_import
from __future__ import division
from __future__ import print_function

import logging
import sys

from lucifer import loglib
from lucifer import leasing

logger = logging.getLogger(__name__)


def main(args):
    """Main function

    @param args: list of command line args
    """
    loglib.configure_logging(name='obtain_lease')
    with leasing.obtain_lease(args[0]) as path:
        logger.debug('Obtained lease %s', path)
        print('done')
        raw_input()
        logger.debug('Finishing successfully')
    print('finish')


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
