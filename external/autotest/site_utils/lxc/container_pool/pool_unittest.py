#!/usr/bin/python
# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import Queue
import logging
import threading
import time
import unittest
from contextlib import contextmanager

import common
from autotest_lib.client.common_lib import error
from autotest_lib.site_utils.lxc import unittest_setup
from autotest_lib.site_utils.lxc.container_pool import error as pool_error
from autotest_lib.site_utils.lxc.container_pool import pool


# A timeout (in seconds) for asynchronous operations.  Note the value of
# TEST_TIMEOUT in relation to pool._CONTAINER_CREATION_TIMEOUT is significant -
# if the latter value is unintentionally set to less than TEST_TIMEOUT, then
# tests may start to malfunction due to workers timing out prematurely.  There
# are some instances where it is appropriate to temporarily set the container
# timeout to a value smaller than the test timeout (see
# PoolTests._forceWorkerTimeouts for an example).
TEST_TIMEOUT = 30

# Use an explicit pool size for testing.
POOL_SIZE = 5


class PoolLifecycleTests(unittest.TestCase):
    """Unit tests for Pool lifecycle."""

    def testCreateAndCleanup(self):
        """Verifies creation and cleanup for various sized pools."""
        # 2 is the minimum pool size.  The others are arbitrary numbers for
        # testing.
        for size in [2, 13, 20]:
            factory = TestFactory()
            self._createAndDestroyPool(factory, size)

            # No containers were requested, so the # factory's creation count
            # should be equal to the requested pool size.
            self.assertEquals(size, factory.create_count)

            # Verify that all containers were cleaned up.
            self.assertEquals(size, factory.destroy_count)


    def testInvalidPoolSize(self):
        """Verifies that invalid sized pools cannot be created."""
        # Pool sizes < 2 don't make sense, and aren't allowed.  Test one at
        # least one negative number, and zero as well.
        for size in [-1, 0, 1]:
            with self.assertRaises(ValueError):
                factory = TestFactory()
                pool.Pool(factory, size)


    def testShutdown_hungFactory(self):
        """Verifies that a hung factory does not prevent clean shutdown."""
        factory = TestFactory()
        factory.pause()
        self._createAndDestroyPool(factory, 3, wait=False)


    def _createAndDestroyPool(self, factory, size, wait=True):
        """Creates a container pool, fully populates it, then destroys it.

        @param factory: A ContainerFactory to use for the Pool.
        @param size: The size of the Pool to create.
        @param wait: If true (the default), fail if the pool does not exit
                     cleanly.  Set this to False for tests where a clean
                     shutdown is not expected (e.g. when exercising things like
                     hung worker threads).
        """
        test_pool = pool.Pool(factory, size=size)

        # Wait for the pool to be fully populated.
        if wait:
            factory.wait(size)

        # Clean up the container pool.
        try:
            test_pool.cleanup(timeout=(TEST_TIMEOUT if wait else 0))
        except threading.ThreadError:
            self.fail('Error while cleaning up container pool:\n%s' %
                      error.format_error())


class PoolTests(unittest.TestCase):
    """Unit tests for the Pool class."""

    def setUp(self):
        """Starts tests with a fully populated container pool."""
        self.factory = TestFactory()
        self.pool = pool.Pool(self.factory, size=POOL_SIZE)
        # Wait for the pool to be fully populated.
        self.factory.wait(POOL_SIZE)


    def tearDown(self):
        """Cleans up the test pool."""
        # Resume the factory so all workers get unblocked and can be cleaned up.
        self.factory.resume()
        # Clean up pool.  Raise errors if the pool thread does not exit cleanly.
        self.pool.cleanup(timeout=TEST_TIMEOUT)


    def testRequestContainer(self):
        """Tests requesting a container from the pool."""
        # Retrieve more containers than the pool can hold, to exercise the pool
        # creation.
        self._getAndVerifyContainers(POOL_SIZE + 10)


    def testRequestContainer_factoryPaused(self):
        """Tests pool recovery from a temporarily hung container factory."""
        # Simulate the factory hanging.
        self.factory.pause()

        # Get all created containers
        self._getAndVerifyContainers(self.factory.create_count)

        # Getting another container should fail.
        self._verifyNoContainers()

        # Restart the factory, verify that the container pool recovers.
        self.factory.resume()
        self._getAndVerifyContainers(1)


    def testRequestContainer_factoryHung(self):
        """Verifies that the pool continues working when worker threads hang."""
        # Simulate the factory hanging on all but 1 of the workers.
        self.factory.pause(pool._MAX_CONCURRENT_WORKERS - 1)
        # Pool should still be able to service requests
        self._getAndVerifyContainers(POOL_SIZE + 10)


    def testRequestContainer_factoryHung_timeout(self):
        """Verifies that container creation times out as expected.

        Simulates a situation where all of the pool's worker threads have hung
        up while creating containers.  Then verifies that the threads time out,
        and the pool recovers.
        """
        # Simulate the factory hanging on all worker threads.  This will exhaust
        # the pool's worker allocation, which should cause container requests to
        # start failing.
        self.factory.pause(pool._MAX_CONCURRENT_WORKERS)

        # Get all created containers
        self._getAndVerifyContainers(self.factory.create_count)

        # Getting another container should fail.
        self._verifyNoContainers()

        self._forceWorkerTimeouts()

        # We should start getting containers again.
        self._getAndVerifyContainers(POOL_SIZE + 10)

        # Check for expected timeout errors in the error log.
        error_count = 0
        try:
            while True:
                self.assertEqual(pool_error.WorkerTimeoutError,
                                 type(self.pool.errors.get_nowait()))
                error_count += 1
        except Queue.Empty:
            pass
        self.assertGreater(error_count, 0)


    def testCleanup_timeout(self):
        """Verifies that timed out containers are still destroyed."""
        # Simulate the factory hanging.
        self.factory.pause()

        # Get all created containers.  Destroy them because we are checking
        # destruction counts later.
        original_create_count = POOL_SIZE
        for _ in range(original_create_count):
            self.pool.get(timeout=TEST_TIMEOUT).destroy()

        self._forceWorkerTimeouts()

        # Trigger pool cleanup.  Do not wait for clean shutdown, we know this
        # will not happen because we have hung threads.
        self.pool.cleanup(timeout=0)

        # Count the number of timeouts.
        timeout_count = 0
        while True:
            try:
                e = self.pool.errors.get(timeout=1)
            except Queue.Empty:
                break
            else:
                if type(e) is pool_error.WorkerTimeoutError:
                    timeout_count += 1
        self.assertGreater(timeout_count, 0)

        self.factory.resume()

        # Allow a timeout so the worker threads that were just unpaused above
        # have a chance to complete.
        start_time = time.time()
        while ((time.time() - start_time) < TEST_TIMEOUT and
               self.factory.create_count != self.factory.destroy_count):
            time.sleep(pool._MIN_MONITOR_PERIOD)
        # Assert that all containers were cleaned up.  This validates that
        # the timed-out worker threads actually cleaned up after themselves.
        self.assertEqual(self.factory.create_count, self.factory.destroy_count)


    def testRequestContainer_factoryCrashed(self):
        """Verifies that the pool continues working when worker threads die."""
        # Cause the factory to crash when create is called.
        self.factory.crash_on_create = True

        # Get all created containers
        self._getAndVerifyContainers(self.factory.create_count)

        # Getting another container should fail.
        self._verifyNoContainers()

        # Errors from the factory should have been logged.
        error_count = 0
        try:
            while True:
                self.assertEqual(TestException,
                                 type(self.pool.errors.get_nowait()))
                error_count += 1
        except Queue.Empty:
            pass
        self.assertGreater(error_count, 0)

        # Stop crashing, verify that the container pool recovers.
        self.factory.crash_on_create = False
        self._getAndVerifyContainers(1)


    def _getAndVerifyContainers(self, count):
        """Verifies that the test pool contains at least <count> containers."""
        for _ in range(count):
            # Block with timeout so that the test doesn't hang forever if
            # something goes wrong.  1 second should be sufficient because the
            # test factory is extremely lightweight.
            pool = self.pool.get(timeout=1)
            self.assertIsNotNone(pool)
            self.assertTrue(pool.started)


    def _verifyNoContainers(self):
        self.assertIsNone(self.pool.get(timeout=1))


    def _forceWorkerTimeouts(self):
        """Forces worker thread timeouts. """
        # Wait for at least one worker in the pool.
        start = time.time()
        while ((time.time() - start) < TEST_TIMEOUT and
               self.pool.worker_count == 0):
            time.sleep(pool._MIN_MONITOR_PERIOD)

        # Set the container creation timeout to 0, wait for worker count to drop
        # to zero (this represents workers timing out), then restore the old
        # timout.
        old_timeout = pool._CONTAINER_CREATION_TIMEOUT
        start = time.time()
        try:
            pool._CONTAINER_CREATION_TIMEOUT = 0
            while ((time.time() - start) < TEST_TIMEOUT and
                   self.pool.worker_count > 0):
                time.sleep(pool._MIN_MONITOR_PERIOD)
        finally:
            pool._CONTAINER_CREATION_TIMEOUT = old_timeout



class ThrottleTests(unittest.TestCase):
    """Tests the various throttles in place on the pool class.

    This is a separate TestCase because throttle tests require different setup
    than general pool tests (i.e. we generally want to set up the factory and
    pool within each test case, not in a generalized setup function).
    """

    def setUp(self):
        # Throttling tests change the constants in the pool module, for testing
        # purposes.  Record the original values so that they can be restored.
        self.old_errors_per_hour = pool._MAX_ERRORS_PER_HOUR
        self.old_max_workers = pool._MAX_CONCURRENT_WORKERS


    def tearDown(self):
        # Restore pool constants to their original values.
        pool._MAX_ERRORS_PER_HOUR = self.old_errors_per_hour
        pool._MAX_CONCURRENT_WORKERS = self.old_max_workers


    def testWorkerThrottle(self):
        """Verifies that workers are properly throttled."""
        max_workers = pool._MAX_CONCURRENT_WORKERS
        test_factory = TestFactory()
        test_factory.pause(max_workers*2)
        test_pool = pool.Pool(test_factory, size=max_workers*2)

        try:
            # The factory should receive max_workers calls.
            test_factory.wait(max_workers)

            # There should be no results.  Allow a generous timeout.
            self.assertIsNone(
                test_pool.get(timeout=pool._MIN_MONITOR_PERIOD * 10))

            # Verify that the worker count is as expected.
            self.assertEquals(max_workers, test_pool.worker_count)

            # Unblock one worker.
            test_factory.resume(1)
            # Check for a single result.
            self.assertEqual(TestContainer,
                             type(test_pool.get(timeout=TEST_TIMEOUT)))
            # Verify (again) that no more results are being produced.
            self.assertIsNone(
                test_pool.get(timeout=pool._MIN_MONITOR_PERIOD * 10))

            # Verify that the worker count is as expected.
            self.assertEquals(max_workers, test_pool.worker_count)

        finally:
            test_factory.resume()
            test_pool.cleanup()


    def testErrorThrottle(self):
        """Verifies that the error throttle works properly."""
        test_error_max = 3
        pool._MAX_ERRORS_PER_HOUR = test_error_max
        try:
            with mock_time() as mocktime:
                # Create a test factory that will crash each time it is called.
                # Passing the mocktime object into the factory makes it such
                # that each crash occurs with a unique timestamp.
                test_factory = TestFactory(mocktime)
                test_factory.crash_on_create = True

                # Start the pool, and wait for <test_error_max> factory calls.
                test_pool = pool.Pool(test_factory, POOL_SIZE)
                logging.debug('wait for %d factory calls.', test_error_max)
                test_factory.wait(test_error_max)
                logging.debug('received %d factory calls.', test_error_max)

                # Verify that the error queue contains <test_error_max> errors.
                for _ in range(test_error_max):
                    self.assertEqual(TestException,
                                     type(test_pool.errors.get_nowait()))
                with self.assertRaises(Queue.Empty):
                    test_pool.errors.get_nowait()

                # Bump the mock time.  The first error will have a timestamp of
                # 1 - pick a time that is more than an hour after that.  This
                # should cause one thread to unblock.
                mocktime.current_time = 3602
                logging.debug('wait for 1 factory call.')
                test_factory.wait(test_error_max + 1)
                logging.debug('received 1 factory call.')
                # Verify that one (and only one) error was raised.
                self.assertEqual(TestException,
                                 type(test_pool.errors.get_nowait()))
                with self.assertRaises(Queue.Empty):
                    test_pool.errors.get_nowait()
        finally:
            test_pool.cleanup()


class WorkerTests(unittest.TestCase):
    """Tests for the _Worker class."""

    def setUp(self):
        """Starts tests with a fully populated container pool."""
        self.factory = TestFactory()
        self.worker_results = Queue.Queue()
        self.worker_errors = Queue.Queue()


    def tearDown(self):
        """Cleans up the test pool."""


    def testGetResult(self):
        """Verifies that get_result transfers container ownership."""
        worker = self.createWorker()
        worker.start()
        worker.join(TEST_TIMEOUT)

        # Verify that one result was returned.
        self.assertIsNotNone(self.worker_results.get_nowait())
        with self.assertRaises(Queue.Empty):
            self.worker_results.get_nowait()

        self.assertNoWorkerErrors()


    def testCancel_running(self):
        """Tests cancelling a worker while it's running."""
        worker = self.createWorker()

        self.factory.pause()
        worker.start()
        # Wait for the worker to call the factory.
        self.factory.wait(1)

        # Cancel the worker, then allow the factory call to proceed, then wait
        # for the worker to finish.
        worker.cancel()
        self.factory.resume()
        worker.join(TEST_TIMEOUT)

        # Verify that the container was destroyed.
        self.assertEqual(1, self.factory.destroy_count)

        # Verify that no results were received.
        with self.assertRaises(Queue.Empty):
            self.worker_results.get_nowait()

        self.assertNoWorkerErrors()


    def testCancel_completed(self):
        """Tests cancelling a worker after it's done."""
        worker = self.createWorker()

        # Start the worker and let it finish.
        worker.start()
        worker.join(TEST_TIMEOUT)

        # Cancel the worker after it completes.  Verify that this returns False.
        self.assertFalse(worker.cancel())

        # Verify that one result was delivered.
        self.assertIsNotNone(self.worker_results.get_nowait())
        with self.assertRaises(Queue.Empty):
            self.worker_results.get_nowait()

        self.assertNoWorkerErrors()


    def createWorker(self):
        """Creates a new pool worker for testing."""
        return pool._Worker(self.factory,
                            self.worker_results.put,
                            self.worker_errors.put)


    def assertNoWorkerErrors(self):
        """Fails if the error queue contains errors."""
        with self.assertRaises(Queue.Empty):
            e = self.worker_errors.get_nowait()
            logging.error('Unexpected worker error: %r', e)




class TestFactory(object):
    """A fake ContainerFactory for testing.

    Keeps track of the number of containers created and destroyed.  Includes
    synchronization so clients can wait for containers to be created.  Hangs and
    crashes on demand.
    """

    def __init__(self, mocktime=None):
        """Creates a new TestFactory.

        @param mocktime: If handed an instance of a _MockTime, the test factory
                         will increment the current (fake) time each time
                         create_container is called.  This ensures that each
                         container (and/or error) produced by the factory has a
                         unique timestamp, which is necessary when testing
                         time-sensitive operations like throttling.
        """
        self.create_cv = threading.Condition()
        self.create_count = 0
        self.lock_destroy_count = threading.Lock()
        self.destroy_count = 0
        self.crash_on_create = False
        self.hanging_ids = []
        self.mocktime = mocktime


    def create_container(self):
        """Creates a fake Container.

        This method might crash or hang if the factory is set up to do so.

        @raises Exception: if crash_on_create is set to True on this factory.
        """
        with self.create_cv:
            next_id = self.create_count
            self.create_count += 1
            if self.mocktime:
                self.mocktime.current_time = self.create_count
            self.create_cv.notify()
        # Notify the condition variable before hanging/crashing, so that other
        # create_container calls are not affected.
        if self.crash_on_create:
            raise TestException()
        while next_id in self.hanging_ids or -1 in self.hanging_ids:
            time.sleep(0.1)
        return TestContainer(self)


    def pause(self, count=None):
        """Temporarily stops container creation.

        Calls to create_container will block until resume() is called.  Use this
        to simulate hanging/long container creation times.
        """
        if count is None:
            self.hanging_ids.append(-1)
        else:
            self.hanging_ids.extend(range(self.create_count,
                                          self.create_count + count))


    def resume(self, count=None):
        """Resumes container creation.

        @raises ThreadError: If the factory is not paused when this is called.
        """
        if count is None:
            self.hanging_ids = []
        else:
            self.hanging_ids = self.hanging_ids[count:]


    def wait(self, count):
        """Waits until the factory has created <count> containers.

        @param count: The number of container creations to wait for.
        """
        with self.create_cv:
            while self.create_count < count:
                self.create_cv.wait(TEST_TIMEOUT)


class TestContainer(object):
    """A fake Container class.

    This class does nothing aside from notifying its factory when it is
    destroyed.
    """

    def __init__(self, factory):
        self._factory = factory
        self.started = False


    def start(self, wait_for_network=True):
        """Simulates starting the container."""
        self.started = True


    def destroy(self, *_args, **_kwargs):
        """Destroys the test container.

        A mock implementation of the real Container.destroy method.  Calls back
        to the TestFactory to notify it that a Container destruction has
        occurred.
        """
        with self._factory.lock_destroy_count:
            self._factory.destroy_count += 1


class TestException(Exception):
    """An exception class for the TestFactory to raise."""
    def __init__(self):
        super(TestException, self).__init__('test error')


class _MockTime(object):
    """A mock for time.time.

    Do not instantiate this directly; use the mock_time context manager.
    """
    def __init__(self):
        self._current_time = 0

    @property
    def current_time(self):
        """The current mock time, which is returned by the .time method."""
        return self._current_time

    @current_time.setter
    def current_time(self, value):
        """Sets the time that is returned by the .time method."""
        logging.debug('time: %d', value)
        self._current_time = value

    def time(self):
        """Mocks the time.time method.

        @return: The value of current_time.
        """
        return self.current_time


@contextmanager
def mock_time():
    """Creates a _MockTime context object that replaces time.time.

    Setting current_time on the returned object will change the value returned
    by time.time calls, within the scope of this context.
    """
    mock = _MockTime()
    orig_time = time.time
    try:
        time.time = mock.time
        yield mock
    finally:
        time.time = orig_time


if __name__ == '__main__':
    unittest_setup.setup(require_sudo=False)
    unittest.main()
