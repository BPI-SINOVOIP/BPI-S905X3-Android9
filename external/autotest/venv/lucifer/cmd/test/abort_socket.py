# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Listen on socket until a datagram is received.

This is used for testing leasing.
"""

from __future__ import absolute_import
from __future__ import division
from __future__ import print_function

import logging
import socket
import sys

from lucifer import loglib

logger = logging.getLogger(__name__)


def main(_args):
    """Main function

    @param args: list of command line args
    """
    loglib.configure_logging(name='abort_socket')
    sock = socket.socket(socket.AF_UNIX, socket.SOCK_DGRAM)
    sock.bind(sys.argv[1])
    print('done')
    sock.recv(1)


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
