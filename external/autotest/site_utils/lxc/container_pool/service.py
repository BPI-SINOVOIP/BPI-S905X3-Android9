# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import os
import threading
import time

import common
from autotest_lib.client.common_lib import utils
from autotest_lib.site_utils.lxc import base_image
from autotest_lib.site_utils.lxc import constants
from autotest_lib.site_utils.lxc import container_factory
from autotest_lib.site_utils.lxc import zygote
from autotest_lib.site_utils.lxc.constants import \
    CONTAINER_POOL_METRICS_PREFIX as METRICS_PREFIX
from autotest_lib.site_utils.lxc.container_pool import async_listener
from autotest_lib.site_utils.lxc.container_pool import error
from autotest_lib.site_utils.lxc.container_pool import message
from autotest_lib.site_utils.lxc.container_pool import pool

try:
    import cPickle as pickle
except:
    import pickle

try:
    from chromite.lib import metrics
    from infra_libs import ts_mon
except ImportError:
    import mock
    metrics = utils.metrics_mock
    ts_mon = mock.Mock()


# The minimum period between polling for new connections, in seconds.
_MIN_POLLING_PERIOD = 0.1


class Service(object):
    """A manager for a pool of LXC containers.

    The Service class manages client communication with an underlying container
    pool.  It listens for incoming client connections, then spawns threads to
    deal with communication with each client.
    """

    def __init__(self, host_dir, pool=None):
        """Sets up a new container pool service.

        @param host_dir: A SharedHostDir.  This will be used for Zygote
                         configuration as well as for general pool operation
                         (e.g. opening linux domain sockets for communication).
        @param pool: (for testing) A container pool that the service will
                     maintain.  This parameter exists for DI, for testing.
                     Under normal circumstances the service instantiates the
                     container pool internally.
        """
        # Create socket for receiving container pool requests.  This also acts
        # as a mutex, preventing multiple container pools from being
        # instantiated.
        self._socket_path = os.path.join(
                host_dir.path, constants.DEFAULT_CONTAINER_POOL_SOCKET)
        self._connection_listener = async_listener.AsyncListener(
                self._socket_path)
        self._client_threads = []
        self._stop_event = None
        self._running = False
        self._pool = pool


    def start(self, pool_size=constants.DEFAULT_CONTAINER_POOL_SIZE):
        """Starts the service.

        @param pool_size: The desired size of the container pool.  This
                          parameter has no effect if a pre-created pool was DI'd
                          into the Service constructor.
        """
        self._running = True

        # Start the container pool.
        if self._pool is None:
            factory = container_factory.ContainerFactory(
                    base_container=base_image.BaseImage().get(),
                    container_class=zygote.Zygote)
            self._pool = pool.Pool(factory=factory, size=pool_size)

        # Start listening asynchronously for incoming connections.
        self._connection_listener.start()

        # Poll for incoming connections, and spawn threads to handle them.
        logging.debug('Start event loop.')
        while self._stop_event is None:
            self._handle_incoming_connections()
            self._cleanup_closed_connections()
            # TODO(kenobi): Poll for and log errors from pool.
            metrics.Counter(METRICS_PREFIX + '/tick').increment()
            time.sleep(_MIN_POLLING_PERIOD)

        logging.debug('Exit event loop.')

        # Stopped - stop all the client threads, stop listening, then signal
        # that shutdown is complete.
        for thread in self._client_threads:
            thread.stop()
        try:
            self._connection_listener.close()
        except Exception as e:
            logging.error('Error stopping pool service: %r', e)
            raise
        finally:
            # Clean up the container pool.
            self._pool.cleanup()
            # Make sure state is consistent.
            self._stop_event.set()
            self._stop_event = None
            self._running = False
            metrics.Counter(METRICS_PREFIX + '/service_stopped').increment()
            logging.debug('Container pool stopped.')


    def stop(self):
        """Stops the service."""
        self._stop_event = threading.Event()
        return self._stop_event


    def is_running(self):
        """Returns whether or not the service is currently running."""
        return self._running


    def get_status(self):
        """Returns a dictionary of values describing the current status."""
        status = {}
        status['running'] = self._running
        status['socket_path'] = self._socket_path
        if self._running:
            status['pool capacity'] = self._pool.capacity
            status['pool size'] = self._pool.size
            status['pool worker count'] = self._pool.worker_count
            status['pool errors'] = self._pool.errors.qsize()
            status['client thread count'] = len(self._client_threads)
        return status


    def _handle_incoming_connections(self):
        """Checks for connections, and spawn sub-threads to handle requests."""
        connection = self._connection_listener.get_connection()
        if connection is not None:
            # Spawn a thread to deal with the new connection.
            thread = _ClientThread(self, self._pool, connection)
            thread.start()
            self._client_threads.append(thread)
            thread_count = len(self._client_threads)
            metrics.Counter(METRICS_PREFIX + '/client_threads'
                          ).increment_by(thread_count)
            logging.debug('Client thread count: %d', thread_count)


    def _cleanup_closed_connections(self):
        """Cleans up dead client threads."""
        # We don't need to lock because all operations on self._client_threads
        # take place on the main thread.
        self._client_threads = [t for t in self._client_threads if t.is_alive()]


class _ClientThread(threading.Thread):
    """A class that handles communication with a pool client.

    Use a thread-per-connection model instead of select()/poll() for a few
    reasons:
    - the number of simultaneous clients is not expected to be high enough for
      select or poll to really pay off.
    - one thread per connection is more robust - if a single client somehow
      crashes its communication thread, that will not affect the other
      communication threads or the main pool service.
    """

    def __init__(self, service, pool, connection):
        self._service = service
        self._pool = pool
        self._connection = connection
        self._running = False
        super(_ClientThread, self).__init__(name='client_thread')


    def run(self):
        """Handles messages coming in from clients.

        The thread main loop monitors the connection and handles incoming
        messages.  Polling is used so that the loop condition can be checked
        regularly - this enables the thread to exit cleanly if required.

        Any kind of error will exit the event loop and close the connection.
        """
        logging.debug('Start event loop.')
        try:
            self._running = True
            while self._running:
                # Poll and deal with messages every second.  The timeout enables
                # the thread to exit cleanly when stop() is called.
                if self._connection.poll(1):
                    try:
                        msg = self._connection.recv()
                    except (AttributeError,
                            ImportError,
                            IndexError,
                            pickle.UnpicklingError) as e:
                        # All of these can occur while unpickling data.
                        logging.error('Error while receiving message: %r', e)
                        # Exit if an error occurs
                        break
                    except EOFError:
                        # EOFError means the client closed the connection.  This
                        # is not an error - just exit.
                        break

                    try:
                        response = self._handle_message(msg)
                        # Always send the response, even if it's None.  This
                        # provides more consistent client-side behaviour.
                        self._connection.send(response)
                    except error.UnknownMessageTypeError as e:
                        # The message received was a valid python object, but
                        # not a valid Message.
                        logging.error('Message error: %s', e)
                        # Exit if an error occurs
                        break
                    except EOFError:
                        # EOFError means the client closed the connection early.
                        # TODO(chromium:794685): Return container to pool.
                        logging.error('Client closed connection before return.')
                        break

        finally:
            # Always close the connection.
            logging.debug('Exit event loop.')
            self._connection.close()


    def stop(self):
        """Stops the client thread."""
        self._running = False


    def _handle_message(self, msg):
        """Handles incoming messages.

        @param msg: The incoming message to be handled.

        @return: A pickleable object (or None) that should be sent back to the
                 client.
        """

        # Only handle Message objects.
        if not isinstance(msg, message.Message):
            raise error.UnknownMessageTypeError(
                    'Invalid message class %s' % type(msg))

        # Use a dispatch table to simulate switch/case.
        handlers = {
            message.ECHO: self._echo,
            message.GET: self._get,
            message.SHUTDOWN: self._shutdown,
            message.STATUS: self._status,
        }
        try:
            return handlers[msg.type](**msg.args)
        except KeyError:
            raise error.UnknownMessageTypeError(
                    'Invalid message type %s' % msg.type)


    def _echo(self, msg):
        """Handles ECHO messages.

        @param msg: A string that will be echoed back to the client.

        @return: The message, for echoing back to the client.
        """
        # Just echo the message back, for testing aliveness.
        logging.debug('Echo: %r', msg)
        return msg


    def _shutdown(self):
        """Handles SHUTDOWN messages.

        @return: An ACK message.  This function is synchronous (i.e. it blocks,
                 and only returns the ACK when shutdown is complete).
        """
        logging.debug('Received shutdown request.')
        # Request shutdown.  Wait for the service to actually stop before
        # sending the response.
        self._service.stop().wait()
        logging.debug('Service shutdown complete.')
        return message.ack()


    def _status(self):
        """Handles STATUS messages.

        @return: The result of the service status call.
        """
        logging.debug('Received status request.')
        return self._service.get_status()


    def _get(self, id, timeout):
        """Gets a container from the pool.

        @param id: A ContainerId to assign to the new container.
        @param timeout: A timeout (in seconds) to wait for the pool.  If a
                        container is not available from the pool within the
                        given period, None will be returned.

        @return: A container from the pool.
        """
        logging.debug('Received get request (id=%s)', id)
        container = self._pool.get(timeout)
        # Assign an ID to the container as soon as it is removed from the pool.
        # This associates the container with the process to which it will be
        # handed off.
        if container is not None:
            logging.debug(
                'Assigning container (name=%s, id=%s)', container.name, id)
            container.id = id
        else:
            logging.debug('No container (id=%s)', id)
        metrics.Counter(METRICS_PREFIX + '/container_requests',
                        field_spec=[ts_mon.BooleanField('success')]
                        ).increment(fields={'success': (container is not None)})
        return container
