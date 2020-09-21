# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import dbus
import os
import shutil
import subprocess
import utils

from autotest_lib.client.common_lib import error
from autotest_lib.client.bin import test
from autotest_lib.client.common_lib.cros import dbus_send


class platform_ImageLoader(test.test):
    """Tests the ImageLoader dbus service.
    """

    version = 1
    STORAGE = '/var/lib/imageloader'
    BUS_NAME = 'org.chromium.ImageLoader'
    BUS_PATH = '/org/chromium/ImageLoader'
    BUS_INTERFACE = 'org.chromium.ImageLoaderInterface'
    GET_COMPONENT_VERSION = 'GetComponentVersion'
    REGISTER_COMPONENT = 'RegisterComponent'
    REMOVE_COMPONENT = 'RemoveComponent'
    LOAD_COMPONENT = 'LoadComponent'
    UNMOUNT_COMPONENT = 'UnmountComponent'
    LOAD_COMPONENT_AT_PATH = 'LoadComponentAtPath'
    BAD_RESULT = ''
    USER = 'chronos'
    COMPONENT_NAME = 'TestFlashComponent'
    CORRUPT_COMPONENT_NAME = 'CorruptTestFlashComponent'
    CORRUPT_COMPONENT_PATH = '/tmp/CorruptTestFlashComponent'
    OLD_VERSION = '23.0.0.207'
    NEW_VERSION = '24.0.0.186'

    def _get_component_version(self, name):
        args = [dbus.String(name)]
        return dbus_send.dbus_send(
            self.BUS_NAME,
            self.BUS_INTERFACE,
            self.BUS_PATH,
            self.GET_COMPONENT_VERSION,
            user=self.USER,
            args=args).response

    def _register_component(self, name, version, path):
        args = [dbus.String(name), dbus.String(version), dbus.String(path)]
        return dbus_send.dbus_send(
            self.BUS_NAME,
            self.BUS_INTERFACE,
            self.BUS_PATH,
            self.REGISTER_COMPONENT,
            timeout_seconds=20,
            user=self.USER,
            args=args).response

    def _remove_component(self, name):
        args = [dbus.String(name)]
        return dbus_send.dbus_send(
            self.BUS_NAME,
            self.BUS_INTERFACE,
            self.BUS_PATH,
            self.REMOVE_COMPONENT,
            timeout_seconds=20,
            user=self.USER,
            args=args).response

    def _load_component(self, name):
        args = [dbus.String(name)]
        return dbus_send.dbus_send(
            self.BUS_NAME,
            self.BUS_INTERFACE,
            self.BUS_PATH,
            self.LOAD_COMPONENT,
            timeout_seconds=20,
            user=self.USER,
            args=args).response

    def _load_component_at_path(self, name, path):
        args = [dbus.String(name), dbus.String(path)]
        return dbus_send.dbus_send(
            self.BUS_NAME,
            self.BUS_INTERFACE,
            self.BUS_PATH,
            self.LOAD_COMPONENT_AT_PATH,
            timeout_seconds=20,
            user=self.USER,
            args=args).response

    def _unmount_component(self, name):
        args = [dbus.String(name)]
        return dbus_send.dbus_send(
            self.BUS_NAME,
            self.BUS_INTERFACE,
            self.BUS_PATH,
            self.UNMOUNT_COMPONENT,
            timeout_seconds=20,
            user=self.USER,
            args=args).response

    def _corrupt_and_load_component(self, component, iteration, target, offset):
        """Registers a valid component and then corrupts it by writing
        a random byte to the target file at the given offset.

        It then attemps to load the component and returns whether or
        not that succeeded.
        @component The path to the component to register.
        @iteration A prefix to append to the name of the component, so that
                   multiple registrations do not clash.
        @target    The name of the file in the component to corrupt.
        @offset    The offset in the file to corrupt.
        """

        versioned_name = self.CORRUPT_COMPONENT_NAME + iteration
        if not self._register_component(versioned_name, self.OLD_VERSION,
                                        component):
            raise error.TestError('Failed to register a valid component')

        self._components_to_delete.append(versioned_name)

        corrupt_path = os.path.join('/var/lib/imageloader', versioned_name,
                                    self.OLD_VERSION)
        os.system('printf \'\\xa1\' | dd conv=notrunc of=%s bs=1 seek=%s' %
                  (corrupt_path + '/' + target, offset))

        return self._load_component(versioned_name)

    def _get_unmount_all_paths(self):
        """Returns a set representing all the paths that would be unmounted by
        imageloader.
        """
        return utils.system_output(
                '/usr/sbin/imageloader --dry_run --unmount_all').splitlines()

    def _test_remove_unmount_component(self, component):
        component_name = "cros-termina"
        if not self._register_component(component_name, "10209.0.0",
                                    component):
            raise error.TestError('Failed to register a valid component')

        mount_path = self._load_component(component_name)
        if mount_path == self.BAD_RESULT:
            raise error.TestError('Failed to mount component as dbus service.')

        if not self._unmount_component(component_name):
            raise error.TestError(
                'Failed to unmount component as dbus service.')

        if not self._remove_component(component_name):
            self._components_to_delete.append(component_name)
            raise error.TestError('Failed to remove a removable component')

    def initialize(self):
        """Initialize the test variables."""
        self._paths_to_unmount = []
        self._components_to_delete = []

    def run_once(self, component1=None, component2=None, component3=None):
        """Executes the test cases."""

        if component3 != None:
            self._test_remove_unmount_component(component3)

        if component1 == None or component2 == None:
            raise error.TestError('Must supply two versions of '
                                  'a production signed component.')

        paths_before = self._get_unmount_all_paths()

        # Make sure there is no version returned at first.
        if self._get_component_version(self.COMPONENT_NAME) != self.BAD_RESULT:
            raise error.TestError('There should be no currently '
                                  'registered component version')

        # Register a component and fetch the version.
        if not self._register_component(self.COMPONENT_NAME, self.OLD_VERSION,
                                        component1):
            raise error.TestError('The component failed to register')

        self._components_to_delete.append(self.COMPONENT_NAME)

        if self._get_component_version(self.COMPONENT_NAME) != '23.0.0.207':
            raise error.TestError('The component version is incorrect')

        # Make sure the same version cannot be re-registered.
        if self._register_component(self.COMPONENT_NAME, self.OLD_VERSION,
                                    component1):
            raise error.TestError('ImageLoader allowed registration '
                                  'of duplicate component version')

        # Make sure that ImageLoader matches the reported version to the
        # manifest.
        if self._register_component(self.COMPONENT_NAME, self.NEW_VERSION,
                                    component1):
            raise error.TestError('ImageLoader allowed registration of a '
                                  'mismatched component version')

        # Register a newer component and fetch the version.
        if not self._register_component(self.COMPONENT_NAME, self.NEW_VERSION,
                                        component2):
            raise error.TestError('Failed to register updated version')

        if self._get_component_version(self.COMPONENT_NAME) != '24.0.0.186':
            raise error.TestError('The component version is incorrect')

        # Simulate a rollback.
        if self._register_component(self.COMPONENT_NAME, self.OLD_VERSION,
                                    component1):
            raise error.TestError('ImageLoader allowed a rollback')

        # Test loading a component that lives at an arbitrary path.
        mnt_path = self._load_component_at_path('FlashLoadedAtPath', component1)
        if mnt_path == self.BAD_RESULT:
            raise error.TestError('LoadComponentAtPath failed')
        self._paths_to_unmount.append(mnt_path)

        known_mount_path = '/run/imageloader/TestFlashComponent_testing'
        # Now test loading the component on the command line.
        if utils.system('/usr/sbin/imageloader --mount ' +
                        '--mount_component=TestFlashComponent ' +
                        '--mount_point=%s' % (known_mount_path)) != 0:
            raise error.TestError('Failed to mount component')

        # If the component is already mounted, it should return the path again.
        if utils.system('/usr/sbin/imageloader --mount ' +
                        '--mount_component=TestFlashComponent ' +
                        '--mount_point=%s' % (known_mount_path)) != 0:
            raise error.TestError('Failed to remount component')

        self._paths_to_unmount.append(known_mount_path)
        if not os.path.exists(
                os.path.join(known_mount_path, 'libpepflashplayer.so')):
            raise error.TestError('Flash player file does not exist')

        mount_path = self._load_component(self.COMPONENT_NAME)
        if mount_path == self.BAD_RESULT:
            raise error.TestError('Failed to mount component as dbus service.')
        self._paths_to_unmount.append(mount_path)

        if not os.path.exists(os.path.join(mount_path, 'libpepflashplayer.so')):
            raise error.TestError('Flash player file does not exist ' +
                                  'after loading as dbus service.')

        # Now test some corrupt components.
        shutil.copytree(component1, self.CORRUPT_COMPONENT_PATH)
        # Corrupt the disk image file in the corrupt component.
        os.system('printf \'\\xa1\' | dd conv=notrunc of=%s bs=1 seek=1000000' %
                  (self.CORRUPT_COMPONENT_PATH + '/image.squash'))
        # Make sure registration fails.
        if self._register_component(self.CORRUPT_COMPONENT_NAME,
                                    self.OLD_VERSION,
                                    self.CORRUPT_COMPONENT_PATH):
            raise error.TestError('Registered a corrupt component')

        # Now register a valid component, and then corrupt it.
        mount_path = self._corrupt_and_load_component(component1, '1',
                                                      'image.squash', '1000000')
        if mount_path == self.BAD_RESULT:
            raise error.TestError('Failed to load component with corrupt image')
        self._paths_to_unmount.append(mount_path)

        corrupt_file = os.path.join(mount_path, 'libpepflashplayer.so')
        if not os.path.exists(corrupt_file):
            raise error.TestError('Flash player file does not exist ' +
                                  'for corrupt image')

        # Reading the files should fail.
        # This is a critical test. We assume dm-verity works, but by default it
        # panics the machine and forces a powerwash. For component updates,
        # ImageLoader should configure the dm-verity table to just return an I/O
        # error. If this test does not throw an exception at all, ImageLoader
        # may not be attaching the dm-verity tree correctly.
        try:
            with open(corrupt_file, 'rb') as f:
                byte = f.read(1)
                while byte != '':
                    byte = f.read(1)
        except IOError:
            pass
            # Catching an IOError once we read the corrupt block is the expected
            # behavior.
        else:
            raise error.TestError(
                'Did not receive an I/O error while reading the corrupt image')

        # Modify the signature and make sure the component does not load.
        if self._corrupt_and_load_component(
                component1, '2', 'imageloader.sig.1', '50') != self.BAD_RESULT:
            raise error.TestError('Mounted component with corrupt signature')

        # Modify the manifest and make sure the component does not load.
        if self._corrupt_and_load_component(component1, '3', 'imageloader.json',
                                            '1') != self.BAD_RESULT:
            raise error.TestError('Mounted component with corrupt manifest')

        # Modify the table and make sure the component does not load.
        if self._corrupt_and_load_component(component1, '4', 'table',
                                            '1') != self.BAD_RESULT:
            raise error.TestError('Mounted component with corrupt table')

        # Determine which mount points were added during the test and verify
        # that they match the expected mount points.
        paths_after = self._get_unmount_all_paths()
        measured_unmount = set(paths_after) - set(paths_before);
        expected_unmount = set(self._paths_to_unmount);
        if measured_unmount != expected_unmount:
            raise error.TestError('Discrepency between --unmount_all and '
                                  'paths_to_unmount "%s" and "%s"' %
                                  (measured_unmount, expected_unmount))

        # Clean up each mount point created by the test to verify the
        # functionality of the --unmount flag.
        for path in expected_unmount:
            if utils.system('/usr/sbin/imageloader --unmount --mount_point=%s'
                            % (path,)) != 0:
                raise error.TestError('Failed to unmount component')

        # Verify that the mount points were indeed cleaned up.
        paths_cleanup = self._get_unmount_all_paths()
        if paths_cleanup != paths_before:
            raise error.TestError('--unmount failed.')

    def cleanup(self):
        """Cleans up the files and mounts created by the test."""
        shutil.rmtree(self.CORRUPT_COMPONENT_PATH, ignore_errors=True)
        for name in self._components_to_delete:
            shutil.rmtree(os.path.join(self.STORAGE, name), ignore_errors=True)
        for path in self._paths_to_unmount:
            utils.system('umount %s' % (path), ignore_status=True)
            shutil.rmtree(path, ignore_errors=True)
