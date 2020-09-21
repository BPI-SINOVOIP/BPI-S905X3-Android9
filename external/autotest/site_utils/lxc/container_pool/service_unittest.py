#!/usr/bin/python
# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import Queue
import array
import collections
import os
import shutil
import tempfile
import threading
import unittest
from contextlib import contextmanager
from multiprocessing import connection

import common
from autotest_lib.site_utils import lxc
from autotest_lib.site_utils.lxc import unittest_setup
from autotest_lib.site_utils.lxc.container_pool import message
from autotest_lib.site_utils.lxc.container_pool import service
from autotest_lib.site_utils.lxc.container_pool import unittest_client


FakeHostDir = collections.namedtuple('FakeHostDir', ['path'])


class ServiceTests(unittest.TestCase):
    """Unit tests for the Service class."""

    @classmethod
    def setUpClass(cls):
        """Creates a directory for running the unit tests. """
        # Explicitly use /tmp as the tmpdir.  Board specific TMPDIRs inside of
        # the chroot are set to a path that causes the socket address to exceed
        # the maximum allowable length.
        cls.test_dir = tempfile.mkdtemp(prefix='service_unittest_', dir='/tmp')


    @classmethod
    def tearDownClass(cls):
        """Deletes the test directory. """
        shutil.rmtree(cls.test_dir)


    def setUp(self):
        """Per-test setup."""
        # Put each test in its own test dir, so it's hermetic.
        self.test_dir = tempfile.mkdtemp(dir=ServiceTests.test_dir)
        self.host_dir = FakeHostDir(self.test_dir)
        self.address = os.path.join(self.test_dir,
                                    lxc.DEFAULT_CONTAINER_POOL_SOCKET)


    def testConnection(self):
        """Tests a simple connection to the pool service."""
        with self.run_service():
            self.assertTrue(self._pool_is_healthy())


    def testAbortedConnection(self):
        """Tests that a closed connection doesn't crash the service."""
        with self.run_service():
            client = connection.Client(self.address)
            client.close()
            self.assertTrue(self._pool_is_healthy())


    def testCorruptedMessage(self):
        """Tests that corrupted messages don't crash the service."""
        with self.run_service(), self.create_client() as client:
            # Send a raw array of bytes.  This will cause an unpickling error.
            client.send_bytes(array.array('i', range(1, 10)))
            # Verify that the container pool closed the connection.
            with self.assertRaises(EOFError):
                client.recv()
            # Verify that the main container pool service is still alive.
            self.assertTrue(self._pool_is_healthy())


    def testInvalidMessageClass(self):
        """Tests that bad messages don't crash the service."""
        with self.run_service(), self.create_client() as client:
            # Send a valid object but not of the right Message class.
            client.send('foo')
            # Verify that the container pool closed the connection.
            with self.assertRaises(EOFError):
                client.recv()
            # Verify that the main container pool service is still alive.
            self.assertTrue(self._pool_is_healthy())


    def testInvalidMessageType(self):
        """Tests that messages with a bad type don't crash the service."""
        with self.run_service(), self.create_client() as client:
            # Send a valid object but not of the right Message class.
            client.send(message.Message('foo', None))
            # Verify that the container pool closed the connection.
            with self.assertRaises(EOFError):
                client.recv()
            # Verify that the main container pool service is still alive.
            self.assertTrue(self._pool_is_healthy())


    def testStop(self):
        """Tests stopping the service."""
        with self.run_service() as svc, self.create_client() as client:
            self.assertTrue(svc.is_running())
            client.send(message.shutdown())
            client.recv()  # wait for ack
            self.assertFalse(svc.is_running())


    def testStatus(self):
        """Tests querying service status."""
        pool = MockPool()
        with self.run_service(pool) as svc, self.create_client() as client:
            client.send(message.status())
            status = client.recv()
            self.assertTrue(status['running'])
            self.assertEqual(self.address, status['socket_path'])
            self.assertEqual(pool.capacity, status['pool capacity'])
            self.assertEqual(pool.size, status['pool size'])
            self.assertEqual(pool.worker_count, status['pool worker count'])
            self.assertEqual(pool.errors.qsize(), status['pool errors'])

            # Change some values, ensure the changes are reflected.
            pool.capacity = 42
            pool.size = 19
            pool.worker_count = 3
            error_count = 8
            for e in range(error_count):
                pool.errors.put(e)
            client.send(message.status())
            status = client.recv()
            self.assertTrue(status['running'])
            self.assertEqual(self.address, status['socket_path'])
            self.assertEqual(pool.capacity, status['pool capacity'])
            self.assertEqual(pool.size, status['pool size'])
            self.assertEqual(pool.worker_count, status['pool worker count'])
            self.assertEqual(pool.errors.qsize(), status['pool errors'])


    def testGet(self):
        """Tests getting a container from the pool."""
        test_pool = MockPool()
        fake_container = MockContainer()
        test_id = lxc.ContainerId.create(42)
        test_pool.containers.put(fake_container)

        with self.run_service(test_pool):
            with self.create_client() as client:
                client.send(message.get(test_id))
                test_container = client.recv()
                self.assertEqual(test_id, test_container.id)


    def testGet_timeoutImmediate(self):
        """Tests getting a container with timeouts."""
        test_id = lxc.ContainerId.create(42)
        with self.run_service():
            with self.create_client() as client:
                client.send(message.get(test_id))
                test_container = client.recv()
                self.assertIsNone(test_container)


    def testGet_timeoutDelayed(self):
        """Tests getting a container with timeouts."""
        test_id = lxc.ContainerId.create(42)
        with self.run_service():
            with self.create_client() as client:
                client.send(message.get(test_id, timeout=1))
                test_container = client.recv()
                self.assertIsNone(test_container)


    def testMultipleClients(self):
        """Tests multiple simultaneous connections."""
        with self.run_service():
            with self.create_client() as client0:
                with self.create_client() as client1:

                    msg0 = 'two driven jocks help fax my big quiz'
                    msg1 = 'how quickly daft jumping zebras vex'

                    client0.send(message.echo(msg0))
                    client1.send(message.echo(msg1))

                    echo0 = client0.recv()
                    echo1 = client1.recv()

                    self.assertEqual(msg0, echo0)
                    self.assertEqual(msg1, echo1)


    def _pool_is_healthy(self):
        """Verifies that the pool service is still functioning.

        Sends an echo message and tests for a response.  This is a stronger
        signal of aliveness than checking Service.is_running, but a False return
        value does not necessarily indicate that the pool service shut down
        cleanly.  Use Service.is_running to check that.
        """
        with self.create_client() as client:
            msg = 'foobar'
            client.send(message.echo(msg))
            return client.recv() == msg


    @contextmanager
    def run_service(self, pool=None):
        """Creates and cleans up a Service instance."""
        if pool is None:
            pool = MockPool()
        svc = service.Service(self.host_dir, pool)
        thread = threading.Thread(name='service', target=svc.start)
        thread.start()
        try:
            yield svc
        finally:
            svc.stop()
            thread.join(1)


    @contextmanager
    def create_client(self):
        """Creates and cleans up a client connection."""
        client = unittest_client.connect(self.address)
        try:
            yield client
        finally:
            client.close()


class MockPool(object):
    """A mock pool class for testing the service."""

    def __init__(self):
        """Initializes a mock empty pool."""
        self.capacity = 0
        self.size = 0
        self.worker_count = 0
        self.errors = Queue.Queue()
        self.containers = Queue.Queue()


    def cleanup(self):
        """Required by pool interface.  Does nothing."""
        pass


    def get(self, timeout=0):
        """Required by pool interface.

        @return: A pool from the containers queue.
        """
        try:
            return self.containers.get(block=(timeout > 0), timeout=timeout)
        except Queue.Empty:
            return None


class MockContainer(object):
    """A mock container class for testing the service."""

    def __init__(self):
        """Initializes a mock container."""
        self.id = None
        self.name = 'test_container'


if __name__ == '__main__':
    unittest_setup.setup(require_sudo=False)
    unittest.main()
