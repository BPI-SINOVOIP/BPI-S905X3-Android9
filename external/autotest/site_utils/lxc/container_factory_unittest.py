#!/usr/bin/python
# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import tempfile
import unittest

import common
from autotest_lib.client.bin import utils
from autotest_lib.client.common_lib import error
from autotest_lib.site_utils import lxc
from autotest_lib.site_utils.lxc import unittest_setup


class ContainerFactoryTests(unittest.TestCase):
    """Unit tests for the ContainerFactory class."""

    @classmethod
    def setUpClass(cls):
        cls.test_dir = tempfile.mkdtemp(dir=lxc.DEFAULT_CONTAINER_PATH,
                                        prefix='container_factory_unittest_')

        # Check if a base container exists on this machine and download one if
        # necessary.
        image = lxc.BaseImage()
        try:
            cls.base_container = image.get()
            cls.cleanup_base_container = False
        except error.ContainerError:
            image.setup()
            cls.base_container = image.get()
            cls.cleanup_base_container = True
        assert(cls.base_container is not None)


    @classmethod
    def tearDownClass(cls):
        cls.base_container = None
        if not unittest_setup.config.skip_cleanup:
            if cls.cleanup_base_container:
                lxc.BaseImage().cleanup()
            utils.run('sudo rm -r %s' % cls.test_dir)


    def setUp(self):
        # Create a separate dir for each test, so they are hermetic.
        self.test_dir = tempfile.mkdtemp(dir=ContainerFactoryTests.test_dir)
        self.test_factory = lxc.ContainerFactory(
                base_container=self.base_container,
                lxc_path=self.test_dir)


    def testCreateContainer(self):
        """Tests basic container creation."""
        container = self.test_factory.create_container()

        try:
            container.refresh_status()
        except:
            self.fail('Invalid container:\n%s' % error.format_error())


    def testCreateContainer_noId(self):
        """Tests container creation with default IDs."""
        container = self.test_factory.create_container()
        self.assertIsNone(container.id)


    def testCreateContainer_withId(self):
        """Tests container creation with given IDs. """
        id0 = lxc.ContainerId(1, 2, 3)
        container = self.test_factory.create_container(id0)
        self.assertEquals(id0, container.id)


    def testContainerName(self):
        """Tests that created containers have the right name."""
        id0 = lxc.ContainerId(1, 2, 3)
        id1 = lxc.ContainerId(42, 41, 40)

        container0 = self.test_factory.create_container(id0)
        container1 = self.test_factory.create_container(id1)

        self.assertEqual(str(id0), container0.name)
        self.assertEqual(str(id1), container1.name)


    def testContainerPath(self):
        """Tests that created containers have the right LXC path."""
        dir0 = tempfile.mkdtemp(dir=self.test_dir)
        dir1 = tempfile.mkdtemp(dir=self.test_dir)

        container0 = self.test_factory.create_container(lxc_path=dir0)
        container1 = self.test_factory.create_container(lxc_path=dir1)

        self.assertEqual(dir0, container0.container_path);
        self.assertEqual(dir1, container1.container_path);


    def testCreateContainer_alreadyExists(self):
        """Tests that container ID conflicts raise errors as expected."""
        id0 = lxc.ContainerId(1, 2, 3)

        self.test_factory.create_container(id0)
        with self.assertRaises(error.ContainerError):
            self.test_factory.create_container(id0)


    def testCreateContainer_forceReset(self):
        """Tests that force-resetting containers works."""
        factory = lxc.ContainerFactory(base_container=self.base_container,
                                       lxc_path=self.test_dir,
                                       force_cleanup=True)

        id0 = lxc.ContainerId(1, 2, 3)
        container0 = factory.create_container(id0)
        container0.start(wait_for_network=False)

        # Create a file in the original container.
        tmpfile = container0.attach_run('mktemp').stdout
        exists = 'test -e %s' % tmpfile
        try:
            container0.attach_run(exists)
        except error.CmdError as e:
            self.fail(e)

        # Create a new container in place of the original, then verify that the
        # file is no longer there.
        container1 = factory.create_container(id0)
        container1.start(wait_for_network=False)
        with self.assertRaises(error.CmdError):
            container1.attach_run(exists)


    def testCreateContainer_subclass(self):
        """Tests that the factory produces objects of the requested class."""
        container = self.test_factory.create_container()
        # Don't use isinstance, we want to check the exact type.
        self.assertTrue(type(container) is lxc.Container)

        class _TestContainer(lxc.Container):
            """A test Container subclass"""
            pass

        test_factory = lxc.ContainerFactory(base_container=self.base_container,
                                            container_class=_TestContainer,
                                            lxc_path=self.test_dir)
        test_container = test_factory.create_container()
        self.assertTrue(type(test_container) is _TestContainer)


    def testCreateContainer_snapshotFails(self):
        """Tests the scenario where snapshotting fails.

        Verifies that the factory is still able to produce a Container when
        cloning fails.
        """
        class MockContainerClass(object):
            """A mock object to simulate the container class.

            This mock has a clone method that simulates a failure when clone is
            called with snapshot=True.  Clone calls are recorded so they can be
            verified later.
            """
            def __init__(self):
                """Initializes the mock."""
                self.clone_count = 0
                self.clone_kwargs = []


            def clone(self, *args, **kwargs):
                """Mocks the Container.clone class method. """
                # Record the particulars of this call.
                self.clone_count += 1
                self.clone_kwargs.append(kwargs)
                # Simulate failure if a snapshot is requested, otherwise create
                # and return the clone.
                if kwargs['snapshot']:
                    raise error.CmdError('fake error', None)
                else:
                    return lxc.Container.clone(*args, **kwargs)

        mock = MockContainerClass()
        factory = lxc.ContainerFactory(base_container=self.base_container,
                                       container_class=mock,
                                       snapshot=True,
                                       lxc_path=self.test_dir)

        factory.create_container()
        # The factory should have made 2 calls to mock.clone - the first with
        # snapshot=True, then the second with snapshot=False.
        self.assertEquals(2, mock.clone_count)
        self.assertTrue(mock.clone_kwargs[0]['snapshot'])
        self.assertFalse(mock.clone_kwargs[1]['snapshot'])


if __name__ == '__main__':
    unittest_setup.setup()
    unittest.main()
