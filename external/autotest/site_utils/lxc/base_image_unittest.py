#!/usr/bin/python
# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import os
import shutil
import tempfile
import unittest
from contextlib import contextmanager

import common
from autotest_lib.client.common_lib import error
from autotest_lib.site_utils import lxc
from autotest_lib.site_utils.lxc import BaseImage
from autotest_lib.site_utils.lxc import constants
from autotest_lib.site_utils.lxc import unittest_setup
from autotest_lib.site_utils.lxc import utils as lxc_utils


test_dir = None
# A reference to an existing base container that can be copied for tests that
# need a base container.  This is an optimization.
reference_container = None
# The reference container can either be a reference to an existing container, or
# to a container that was downloaded by this test.  If the latter, then it needs
# to be cleaned up when the tests are complete.
cleanup_ref_container = False


class BaseImageTests(unittest.TestCase):
    """Unit tests to verify the BaseImage class."""

    def testCreate_existing(self):
        """Verifies that BaseImage works with existing base containers."""
        with TestBaseContainer() as control:
            manager = BaseImage(control.container_path,
                                control.name)
            self.assertIsNotNone(manager.base_container)
            self.assertEquals(control.container_path,
                              manager.base_container.container_path)
            self.assertEquals(control.name, manager.base_container.name)
            try:
                manager.base_container.refresh_status()
            except error.ContainerError:
                self.fail('Base container was not valid.\n%s' %
                          error.format_error())


    def testCleanup_noClones(self):
        """Verifies that cleanup cleans up the base image."""
        base = lxc.Container.clone(src=reference_container,
                                   new_name=constants.BASE,
                                   new_path=test_dir,
                                   snapshot=True)

        manager = BaseImage(base.container_path, base.name)
        # Precondition: ensure base exists and is a valid container.
        base.refresh_status()

        manager.cleanup()

        # Verify that the base container was cleaned up.
        self.assertFalse(lxc_utils.path_exists(
                os.path.join(base.container_path, base.name)))


    def testCleanup_withClones(self):
        """Verifies that cleanup cleans up the base image.

        Ensure that it works even when clones of the base image exist.
        """
        # Do not snapshot, as snapshots of snapshots behave differently than
        # snapshots of full container clones.  BaseImage cleanup code assumes
        # that the base container is not a snapshot.
        base = lxc.Container.clone(src=reference_container,
                                   new_name=constants.BASE,
                                   new_path=test_dir,
                                   snapshot=False)
        manager = BaseImage(base.container_path, base.name)
        clones = []
        for i in range(3):
            clones.append(lxc.Container.clone(src=base,
                                              new_name='clone_%d' % i,
                                              snapshot=True))


        # Precondition: all containers are valid.
        base.refresh_status()
        for container in clones:
            container.refresh_status()

        manager.cleanup()

        # Verify that all containers were cleaned up
        self.assertFalse(lxc_utils.path_exists(
                os.path.join(base.container_path, base.name)))
        for container in clones:
            if constants.SUPPORT_SNAPSHOT_CLONE:
                # Snapshot clones should get deleted along with the base
                # container.
                self.assertFalse(lxc_utils.path_exists(
                        os.path.join(container.container_path, container.name)))
            else:
                # If snapshot clones aren't supported (e.g. on moblab), the
                # clones should not be affected by the destruction of the base
                # container.
                try:
                    container.refresh_status()
                except error.ContainerError:
                    self.fail(error.format_error())



class BaseImageSetupTests(unittest.TestCase):
    """Unit tests to verify the setup of specific images.

    Some images differ in layout from others.  These tests test specific images
    to make sure the setup code works for all of them.
    """

    def setUp(self):
        self.manager = BaseImage(container_path=test_dir)


    def tearDown(self):
        self.manager.cleanup()


    def testSetupBase05(self):
        """Verifies that setup works for moblab base container.

        Verifies that the code for installing the rootfs location into the
        lxc config, is working correctly.
        """
        # Set up the bucket, then start the base container, and verify it works.
        self.manager.setup('base_05')
        container = self.manager.base_container

        container.start(wait_for_network=False)
        self.assertTrue(container.is_running())


    @unittest.skipIf(constants.IS_MOBLAB,
                     "Moblab does not support the regular base container.")
    def testSetupBase09(self):
        """Verifies that setup works for base container.

        Verifies that the code for installing the rootfs location into the
        lxc config, is working correctly.
        """
        self.manager.setup('base_09')
        container = self.manager.base_container

        container.start(wait_for_network=False)
        self.assertTrue(container.is_running())


@contextmanager
def TestBaseContainer(name=constants.BASE):
    """Context manager for creating a scoped base container for testing.

    @param name: (optional) Name of the base container.  If this is not
                 provided, the default base container name is used.
    """
    container = lxc.Container.clone(src=reference_container,
                                    new_name=name,
                                    new_path=test_dir,
                                    snapshot=True,
                                    cleanup=False)
    try:
        yield container
    finally:
        if not unittest_setup.config.skip_cleanup:
            container.destroy()


def setUpModule():
    """Module setup for base image unittests.

    Sets up a test directory along with a reference container that is used by
    tests that need an existing base container.
    """
    global test_dir
    global reference_container
    global cleanup_ref_container

    test_dir = tempfile.mkdtemp(dir=lxc.DEFAULT_CONTAINER_PATH,
                                prefix='base_container_manager_unittest_')
    # Unfortunately, aside from duping the BaseImage code completely, there
    # isn't an easy way to download and configure a base container.  So even
    # though this is the BaseImage unittest, we use a BaseImage to set it up.
    bcm = BaseImage()
    if bcm.base_container is None:
        bcm.setup()
        cleanup_ref_container = True
    reference_container = bcm.base_container


def tearDownModule():
    """Deletes the test dir and reference container."""
    if not unittest_setup.config.skip_cleanup:
        if cleanup_ref_container:
            reference_container.destroy()
        shutil.rmtree(test_dir)


if __name__ == '__main__':
    unittest_setup.setup()
    unittest.main()
