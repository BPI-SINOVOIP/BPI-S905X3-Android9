#!/usr/bin/python
# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import os
import tempfile
import shutil
import unittest
from contextlib import contextmanager

import common
from autotest_lib.client.bin import utils
from autotest_lib.client.common_lib import error
from autotest_lib.site_utils import lxc
from autotest_lib.site_utils.lxc import constants
from autotest_lib.site_utils.lxc import unittest_http
from autotest_lib.site_utils.lxc import unittest_setup
from autotest_lib.site_utils.lxc import utils as lxc_utils


@unittest.skipIf(lxc.IS_MOBLAB, 'Zygotes are not supported on moblab.')
class ZygoteTests(unittest.TestCase):
    """Unit tests for the Zygote class."""

    @classmethod
    def setUpClass(cls):
        cls.test_dir = tempfile.mkdtemp(dir=lxc.DEFAULT_CONTAINER_PATH,
                                        prefix='zygote_unittest_')

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

        # Set up the zygote host path.
        cls.shared_host_dir = lxc.SharedHostDir(
                os.path.join(cls.test_dir, 'host'))


    @classmethod
    def tearDownClass(cls):
        cls.base_container = None
        if not unittest_setup.config.skip_cleanup:
            if cls.cleanup_base_container:
                lxc.BaseImage().cleanup()
            cls.shared_host_dir.cleanup()
            shutil.rmtree(cls.test_dir)


    def testCleanup(self):
        """Verifies that the zygote cleans up after itself."""
        with self.createZygote() as zygote:
            host_path = zygote.host_path

            self.assertTrue(os.path.isdir(host_path))

            # Start/stop the zygote to exercise the host mounts.
            zygote.start(wait_for_network=False)
            zygote.stop()

        # After the zygote is destroyed, verify that the host path is cleaned
        # up.
        self.assertFalse(os.path.isdir(host_path))


    def testCleanupWithUnboundHostDir(self):
        """Verifies that cleanup works when the host dir is unbound."""
        with self.createZygote() as zygote:
            host_path = zygote.host_path

            self.assertTrue(os.path.isdir(host_path))
            # Don't start the zygote, so the host mount is not bound.

        # After the zygote is destroyed, verify that the host path is cleaned
        # up.
        self.assertFalse(os.path.isdir(host_path))


    def testCleanupWithNoHostDir(self):
        """Verifies that cleanup works when the host dir is missing."""
        with self.createZygote() as zygote:
            host_path = zygote.host_path

            utils.run('sudo rmdir %s' % zygote.host_path)
            self.assertFalse(os.path.isdir(host_path))
        # Zygote destruction should yield no errors if the host path is
        # missing.


    def testHostDir(self):
        """Verifies that the host dir on the container is created, and correctly
        bind-mounted."""
        with self.createZygote() as zygote:
            self.assertIsNotNone(zygote.host_path)
            self.assertTrue(os.path.isdir(zygote.host_path))

            zygote.start(wait_for_network=False)

            self.verifyBindMount(
                zygote,
                container_path=lxc.CONTAINER_HOST_DIR,
                host_path=zygote.host_path)


    def testHostDirExists(self):
        """Verifies that the host dir is just mounted if it already exists."""
        # Pre-create the host dir and put a file in it.
        test_host_path = os.path.join(self.shared_host_dir.path,
                                      'testHostDirExists')
        test_filename = 'test_file'
        test_host_file = os.path.join(test_host_path, test_filename)
        test_string = 'jackdaws love my big sphinx of quartz.'
        os.makedirs(test_host_path)
        with open(test_host_file, 'w') as f:
            f.write(test_string)

        # Sanity check
        self.assertTrue(lxc_utils.path_exists(test_host_file))

        with self.createZygote(host_path=test_host_path) as zygote:
            zygote.start(wait_for_network=False)

            self.verifyBindMount(
                zygote,
                container_path=lxc.CONTAINER_HOST_DIR,
                host_path=zygote.host_path)

            # Verify that the old directory contents was preserved.
            cmd = 'cat %s' % os.path.join(lxc.CONTAINER_HOST_DIR,
                                          test_filename)
            test_output = zygote.attach_run(cmd).stdout.strip()
            self.assertEqual(test_string, test_output)


    def testInstallSsp(self):
        """Verifies that installing the ssp in the container works."""
        # Hard-coded path to some golden data for this test.
        test_ssp = os.path.join(
                common.autotest_dir,
                'site_utils', 'lxc', 'test', 'test_ssp.tar.bz2')
        # Create a container, install the self-served ssp, then check that it is
        # installed into the container correctly.
        with self.createZygote() as zygote:
            # Note: start the zygote first, then install the SSP.  This mimics
            # the way things would work in the production environment.
            zygote.start(wait_for_network=False)
            with unittest_http.serve_locally(test_ssp) as url:
                zygote.install_ssp(url)

            # The test ssp just contains a couple of text files, in known
            # locations.  Verify the location and content of those files in the
            # container.
            cat = lambda path: zygote.attach_run('cat %s' % path).stdout
            test0 = cat(os.path.join(constants.CONTAINER_AUTOTEST_DIR,
                                     'test.0'))
            test1 = cat(os.path.join(constants.CONTAINER_AUTOTEST_DIR,
                                     'dir0', 'test.1'))
            self.assertEquals('the five boxing wizards jumped quickly',
                              test0)
            self.assertEquals('the quick brown fox jumps over the lazy dog',
                              test1)


    def testInstallControlFile(self):
        """Verifies that installing a control file in the container works."""
        _unused, tmpfile = tempfile.mkstemp()
        with self.createZygote() as zygote:
            # Note: start the zygote first.  This mimics the way things would
            # work in the production environment.
            zygote.start(wait_for_network=False)
            zygote.install_control_file(tmpfile)
            # Verify that the file is found in the zygote.
            zygote.attach_run(
                'test -f %s' % os.path.join(lxc.CONTROL_TEMP_PATH,
                                            os.path.basename(tmpfile)))


    def testCopyFile(self):
        """Verifies that files are correctly copied into the container."""
        control_string = 'amazingly few discotheques provide jukeboxes'
        with tempfile.NamedTemporaryFile() as tmpfile:
            tmpfile.write(control_string)
            tmpfile.flush()

            with self.createZygote() as zygote:
                dst = os.path.join(constants.CONTAINER_AUTOTEST_DIR,
                                   os.path.basename(tmpfile.name))
                zygote.start(wait_for_network=False)
                zygote.copy(tmpfile.name, dst)
                # Verify the file content.
                test_string = zygote.attach_run('cat %s' % dst).stdout
                self.assertEquals(control_string, test_string)


    def testCopyDirectory(self):
        """Verifies that directories are correctly copied into the container."""
        control_string = 'pack my box with five dozen liquor jugs'
        with lxc_utils.TempDir() as tmpdir:
            fd, tmpfile = tempfile.mkstemp(dir=tmpdir)
            f = os.fdopen(fd, 'w')
            f.write(control_string)
            f.close()

            with self.createZygote() as zygote:
                dst = os.path.join(constants.CONTAINER_AUTOTEST_DIR,
                                   os.path.basename(tmpdir))
                zygote.start(wait_for_network=False)
                zygote.copy(tmpdir, dst)
                # Verify the file content.
                test_file = os.path.join(dst, os.path.basename(tmpfile))
                test_string = zygote.attach_run('cat %s' % test_file).stdout
                self.assertEquals(control_string, test_string)


    def testFindHostMount(self):
        """Verifies that zygotes pick up the correct host dirs."""
        with self.createZygote() as zygote0:
            # Not a clone, this just instantiates zygote1 on top of the LXC
            # container created by zygote0.
            zygote1 = lxc.Zygote(container_path=zygote0.container_path,
                                 name=zygote0.name,
                                 attribute_values={})
            # Verify that the new zygote picked up the correct host path
            # from the existing LXC container.
            self.assertEquals(zygote0.host_path, zygote1.host_path)
            self.assertEquals(zygote0.host_path_ro, zygote1.host_path_ro)


    def testDetectExistingMounts(self):
        """Verifies that host mounts are properly reconstructed.

        When a Zygote is instantiated on top of an already-running container,
        any previously-created bind mounts have to be detected.  This enables
        proper cleanup later.
        """
        with lxc_utils.TempDir() as tmpdir, self.createZygote() as zygote0:
            zygote0.start(wait_for_network=False)
            # Create a bind mounted directory.
            zygote0.mount_dir(tmpdir, 'foo')
            # Create another zygote on top of the existing container.
            zygote1 = lxc.Zygote(container_path=zygote0.container_path,
                                 name=zygote0.name,
                                 attribute_values={})
            # Verify that the new zygote contains the same bind mounts.
            self.assertEqual(zygote0.mounts, zygote1.mounts)


    def testMountDirectory(self):
        """Verifies that read-write mounts work."""
        with lxc_utils.TempDir() as tmpdir, self.createZygote() as zygote:
            dst = '/testMountDirectory/testMount'
            zygote.start(wait_for_network=False)
            zygote.mount_dir(tmpdir, dst, readonly=False)

            # Verify that the mount point is correctly bound, and is read-write.
            self.verifyBindMount(zygote, dst, tmpdir)
            zygote.attach_run('test -r {0} -a -w {0}'.format(dst))


    def testMountDirectoryReadOnly(self):
        """Verifies that read-only mounts are mounted, and read-only."""
        with lxc_utils.TempDir() as tmpdir, self.createZygote() as zygote:
            dst = '/testMountDirectoryReadOnly/testMount'
            zygote.start(wait_for_network=False)
            zygote.mount_dir(tmpdir, dst, readonly=True)

            # Verify that the mount point is correctly bound, and is read-only.
            self.verifyBindMount(zygote, dst, tmpdir)
            try:
                zygote.attach_run('test -r {0} -a ! -w {0}'.format(dst))
            except error.CmdError:
                self.fail('Bind mount is not read-only')


    def testMountDirectoryRelativePath(self):
        """Verifies that relative-path mounts work."""
        with lxc_utils.TempDir() as tmpdir, self.createZygote() as zygote:
            dst = 'testMountDirectoryRelativePath/testMount'
            zygote.start(wait_for_network=False)
            zygote.mount_dir(tmpdir, dst, readonly=True)

            # Verify that the mount points is correctly bound..
            self.verifyBindMount(zygote, dst, tmpdir)


    @contextmanager
    def createZygote(self,
                     name = None,
                     attribute_values = None,
                     snapshot = True,
                     host_path = None):
        """Clones a zygote from the test base container.
        Use this to ensure that zygotes got properly cleaned up after each test.

        @param container_path: The LXC path for the new container.
        @param host_path: The host path for the new container.
        @param name: The name of the new container.
        @param attribute_values: Any attribute values for the new container.
        @param snapshot: Whether to create a snapshot clone.
        """
        if name is None:
            name = self.id().split('.')[-1]
        if host_path is None:
            host_path = os.path.join(self.shared_host_dir.path, name)
        if attribute_values is None:
            attribute_values = {}
        zygote = lxc.Zygote(self.test_dir,
                            name,
                            attribute_values,
                            self.base_container,
                            snapshot,
                            host_path)
        try:
            yield zygote
        finally:
            if not unittest_setup.config.skip_cleanup:
                zygote.destroy()


    def verifyBindMount(self, container, container_path, host_path):
        """Verifies that a given path in a container is bind-mounted to a given
        path in the host system.

        @param container: The Container instance to be tested.
        @param container_path: The path in the container to compare.
        @param host_path: The path in the host system to compare.
        """
        container_inode = (container.attach_run('ls -id %s' % container_path)
                           .stdout.split()[0])
        host_inode = utils.run('ls -id %s' % host_path).stdout.split()[0]
        # Compare the container and host inodes - they should match.
        self.assertEqual(container_inode, host_inode)


if __name__ == '__main__':
    unittest_setup.setup()
    unittest.main()
