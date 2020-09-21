# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import Queue
import logging
import os
import socket
import threading
from multiprocessing import connection

import common


class AsyncListener(object):
    """A class for asynchronous listening on a unix socket.

    This class opens a unix socket with the given address and auth key.
    Connections are listened for on a separate thread, and queued up to be dealt
    with.
    """
    def __init__(self, address):
        """Opens a socket with the given address and key.

        @param address: The socket address.

        @raises socket.error: If the address is already in use or is not a valid
                              path.
        @raises TypeError: If the address is not a valid unix domain socket
                           address.
        """
        self._socket = connection.Listener(address, family='AF_UNIX')

        # This is done mostly for local testing/dev purposes - the easiest/most
        # reliable way to run the container pool locally is as root, but then
        # only other processes owned by root can connect to the container.
        # Setting open permissions on the socket makes it so that other users
        # can connect, which enables developers to then run tests without sudo.
        os.chmod(address, 0777)

        self._address = address
        self._queue = Queue.Queue()
        self._thread = None
        self._running = False


    def start(self):
        """Starts listening for connections.

        Starts a child thread that listens asynchronously for connections.
        After calling this function, incoming connections may be retrieved by
        calling the get_connection method.
        """
        logging.debug('Starting connection listener.')
        self._running = True
        self._thread = threading.Thread(name='connection_listener',
                                        target=self._poll)
        self._thread.start()


    def is_running(self):
        """Returns whether the listener is currently running."""
        return self._running


    def stop(self):
        """Stop listening for connections.

        Stops the listening thread.  After this is called, connections will no
        longer be received by the socket.  Note, however, that the socket is not
        destroyed and that calling start again, will resume listening for
        connections.

        This function is expected to be called when the container pool service
        is being killed/restarted, so it doesn't make an extraordinary effort to
        ensure that the listener thread is cleanly destroyed.

        @return: True if the listener thread was successfully killed, False
                 otherwise.
        """
        if not self._running:
            return False

        logging.debug('Stopping connection listener.')
        # Setting this to false causes the thread's event loop to exit on the
        # next iteration.
        self._running = False
        # Initiate a connection to force a trip through the event loop.  Use raw
        # sockets because the connection module's convenience classes don't
        # support timeouts, which leads to deadlocks.
        try:
            fake_connection = socket.socket(socket.AF_UNIX)
            fake_connection.settimeout(0)  # non-blocking
            fake_connection.connect(self._address)
            fake_connection.close()
        except socket.timeout:
            logging.error('Timeout while attempting to close socket listener.')
            return False

        logging.debug('Socket closed. Waiting for thread to terminate.')
        self._thread.join(1)
        return not self._thread.isAlive()


    def close(self):
        """Closes and destroys the socket.

        If the listener thread is running, it is first stopped.
        """
        logging.debug('AsyncListener.close called.')
        if self._running:
            self.stop()
        self._socket.close()


    def get_connection(self, timeout=0):
        """Returns a connection, if one is pending.

        The listener thread queues up connections for the main process to
        handle.  This method returns a pending connection on the queue.  If no
        connections are pending, None is returned.

        @param timeout: Optional timeout.  If set to 0 (the default), the method
                        will return instantly if no connections are awaiting.
                        Otherwise, the method will wait the specified number of
                        seconds before returning.

        @return: A pending connection, or None of no connections are pending.
        """
        try:
            return self._queue.get(block=timeout>0, timeout=timeout)
        except Queue.Empty:
            return None


    def _poll(self):
        """Polls the socket for incoming connections.

        This function is intended to be run on the listener thread.  It accepts
        incoming socket connections, and queues them up to be handled.
        """
        logging.debug('Start event loop.')
        while self._running:
            try:
                self._queue.put(self._socket.accept())
                logging.debug('Received incoming connection.')
            except IOError:
                # The stop method uses a fake connection to unblock the polling
                # thread.  This results in an IOError but this is an expected
                # outcome.
                logging.debug('Connection aborted.')
        logging.debug('Exit event loop.')
