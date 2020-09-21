# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import socket
import threading
from multiprocessing import connection


def connect(address, timeout=30):
    """Creates an AsyncListener client for testing purposes.

    @param address: Address of the socket to connect to.
    @param timeout: Connection timeout, in seconds.  Default is 30.

    @raises socket.timeout: If a connection is not made before the timeout
                            expires.
    """
    return _ClientFactory(address).connect(timeout)


class _ClientFactory(threading.Thread):
    """Factory class for making client connections with a timeout.

    Instantiate this with an address, and call connect.  The factory will take
    care of polling for a connection.  If a connection is not established within
    a set period of time, the make_connction call will raise a socket.timeout
    exception instead of hanging indefinitely.
    """
    def __init__(self, address):
        super(_ClientFactory, self).__init__()
        # Use a daemon thread, so that if this thread hangs, it doesn't keep the
        # parent thread alive.  All daemon threads die when the parent process
        # dies.
        self.daemon = True
        self._address = address
        self._client = None


    def run(self):
        """Instantiates a connection.Client."""
        self._client = connection.Client(self._address)


    def connect(self, timeout):
        """Attempts to create a connection.Client with a timeout.

        Every 5 seconds a warning will be logged for debugging purposes.  After
        the timeout expires, the function will raise a socket.timout error.

        @param timeout: A connection timeout, in seconds.

        @return: A connection.Client connected using the address that was
                 specified when this factory was created.

        @raises socket.timeout: If the connection is not established before the
                                given timeout expires.
        """
        # Start the thread, which attempts to open the connection.
        self.start()
        # Poll approximately once a second, so clients don't wait forever.
        for i in range(1, timeout + 1):
            self.join(1)
            if self._client is not None:
                return self._client

            # Log a warning when we first detect a potential problem, then every
            # 5 seconds after that.
            if i < 3 or i % 5 == 0:
                logging.warning(
                    'Test client failed to connect after %s seconds', i)
        # Still no connection - time out.
        raise socket.timeout('Test client timed out waiting for connection.')
