# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import os
import socket
import sys
import threading
from contextlib import contextmanager
from multiprocessing import connection

import common
from autotest_lib.site_utils.lxc import constants
from autotest_lib.site_utils.lxc.container_pool import message


# Default server-side timeout in seconds; limits time to fetch container.
_SERVER_CONNECTION_TIMEOUT = 1
# Extra timeout to use on the client side; limits network communication time.
_CLIENT_CONNECTION_TIMEOUT = 5

class Client(object):
    """A class for communicating with a container pool service.

    The Client class enables clients to communicate with a running container
    pool service - for example, to query current status, or to obtain a new
    container.

    Here is an example usage:

    def status();
      client = Client(pool_address, timeout)
      print(client.get_status())
      client.close()

    In addition, the class provides a context manager for easier cleanup:

    def status():
      with Client.connect(pool_address, timeout) as client:
        print(client.get_status())
    """

    def __init__(self, address=None, timeout=_SERVER_CONNECTION_TIMEOUT):
        """Initializes a new Client object.

        @param address: The address of the pool to connect to.
        @param timeout: A connection timeout, in seconds.

        @raises socket.error: If some other miscelleneous socket error occurs
                              (e.g. if the socket does not exist)
        @raises socket.timeout: If the connection is not established before the
                                given timeout expires.
        """
        if address is None:
            address = os.path.join(
                constants.DEFAULT_SHARED_HOST_PATH,
                constants.DEFAULT_CONTAINER_POOL_SOCKET)
        self._connection = _ConnectionHelper(address).connect(timeout)


    @classmethod
    @contextmanager
    def connect(cls, address, timeout):
        """A ContextManager for Client objects.

        @param address: The address of the pool's communication socket.
        @param timeout: A connection timeout, in seconds.

        @return: A Client connected to the domain socket on the given address.

        @raises socket.error: If some other miscelleneous socket error occurs
                              (e.g. if the socket does not exist)
        @raises socket.timeout: If the connection is not established before the
                                given timeout expires.
        """
        client = Client(address, timeout)
        try:
            yield client
        finally:
            client.close()


    def close(self):
        """Closes the client connection."""
        self._connection.close()
        self._connection = None


    def get_container(self, id, timeout):
        """Retrieves a container from the pool service.

        @param id: A ContainerId to assign to the container.  Containers require
                   an ID when they are dissociated from the pool, so that they
                   can be tracked.
        @param timeout: A timeout (in seconds) to wait for the operation to
                        complete.  A timeout of 0 will return immediately if no
                        containers are available.

        @return: A container from the pool, when one becomes available, or None
                 if no containers were available within the specified timeout.
        """
        self._connection.send(message.get(id, timeout))
        # The service side guarantees that it always returns something
        # (i.e. a Container, or None) within the specified timeout period, or
        # to wait indefinitely if given None.
        # However, we don't entirely trust it and account for network problems.
        if timeout is None or self._connection.poll(
                timeout + _CLIENT_CONNECTION_TIMEOUT):
            return self._connection.recv()
        else:
            logging.debug('No container (id=%s). Connection failed.', id)
            return None


    def get_status(self):
        """Gets the status of the connected Pool."""
        self._connection.send(message.status())
        return self._connection.recv()


    def shutdown(self):
        """Stops the service."""
        self._connection.send(message.shutdown())
        # Wait for ack.
        self._connection.recv()


class _ConnectionHelper(threading.Thread):
    """Factory class for making client connections with a timeout.

    Instantiate this with an address, and call connect.  The factory will take
    care of polling for a connection.  If a connection is not established within
    a set period of time, the make_connction call will raise a socket.timeout
    exception instead of hanging indefinitely.
    """
    def __init__(self, address):
        super(_ConnectionHelper, self).__init__()
        # Use a daemon thread, so that if this thread hangs, it doesn't keep the
        # parent thread alive.  All daemon threads die when the parent process
        # dies.
        self.daemon = True
        self._address = address
        self._client = None
        self._exc_info = None


    def run(self):
        """Instantiates a connection.Client."""
        try:
            logging.debug('Attempting connection to %s', self._address)
            self._client = connection.Client(self._address)
            logging.debug('Connection to %s successful', self._address)
        except Exception:
            self._exc_info = sys.exc_info()


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
        # Range starts at 1 for readability (so the message below doesn't say 0
        # seconds).
        # Range ends at timeout+2 so that a timeout of 0 results in at least 1
        # try.
        for i in range(1, timeout + 2):
            self.join(1)
            if self._exc_info is not None:
                raise self._exc_info[0], self._exc_info[1], self._exc_info[2]
            elif self._client is not None:
                return self._client

            # Log a warning when we first detect a potential problem, then every
            # 5 seconds after that.
            if i < 3 or i % 5 == 0:
                logging.warning(
                    'Test client failed to connect after %s seconds.', i)
        # Still no connection - time out.
        raise socket.timeout('Test client timed out waiting for connection.')
