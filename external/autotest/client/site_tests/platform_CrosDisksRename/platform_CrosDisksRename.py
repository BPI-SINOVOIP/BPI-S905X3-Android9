# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import json
import tempfile

from autotest_lib.client.bin import test
from autotest_lib.client.bin import utils
from autotest_lib.client.common_lib import error
from autotest_lib.client.cros.cros_disks import CrosDisksTester
from autotest_lib.client.cros.cros_disks import VirtualFilesystemImage
from autotest_lib.client.cros.cros_disks import DefaultFilesystemTestContent


class CrosDisksRenameTester(CrosDisksTester):
    """A tester to verify rename support in CrosDisks.
    """
    def __init__(self, test, test_configs):
        super(CrosDisksRenameTester, self).__init__(test)
        self._test_configs = test_configs

    def _run_test_config(self, config):
        logging.info('Testing "%s"', config['description'])
        filesystem_type = config.get('filesystem_type')
        mount_filesystem_type = config.get('mount_filesystem_type')
        mount_options = config.get('mount_options')
        volume_name = config.get('volume_name')

        # Create a zero-filled virtual filesystem image to help simulate
        # a removable drive and test disk renaming for different file system
        # types.
        with VirtualFilesystemImage(
                block_size=1024,
                block_count=65536,
                filesystem_type=filesystem_type,
                mount_filesystem_type=mount_filesystem_type,
                mkfs_options=config.get('mkfs_options')) as image:
            # Create disk with the target file system type.
            image.format()
            image.mount(options=['sync'])
            test_content = DefaultFilesystemTestContent()
            image.unmount()

            device_file = image.loop_device
            # Mount through API to assign appropriate group on block device that
            # depends on file system type.
            self.cros_disks.mount(device_file, filesystem_type,
                                  mount_options)
            expected_mount_completion = {
                'status': config['expected_mount_status'],
                'source_path': device_file,
            }

            if 'expected_mount_path' in config:
                expected_mount_completion['mount_path'] = \
                    config['expected_mount_path']
            result = self.cros_disks.expect_mount_completion(
                    expected_mount_completion)
            self.cros_disks.unmount(device_file)

            self.cros_disks.rename(device_file, volume_name)
            expected_rename_completion = {
                'path': device_file
            }

            if 'expected_rename_status' in config:
                expected_rename_completion['status'] = \
                        config['expected_rename_status']
            result = self.cros_disks.expect_rename_completion(
                expected_rename_completion)

            if result['status'] == 0:
                # Test creating and verifying content of the renamed device.
                logging.info("Test filesystem access on renamed device")
                test_content = DefaultFilesystemTestContent()
                mount_path = image.mount()
                if not test_content.create(mount_path):
                    raise error.TestFail("Failed to create test content")
                if not test_content.verify(mount_path):
                    raise error.TestFail("Failed to verify test content")
                # Verify new volume name
                if volume_name != image.get_volume_label():
                    raise error.TestFail("Failed to rename the drive")

    def test_using_virtual_filesystem_image(self):
        try:
            for config in self._test_configs:
                self._run_test_config(config)
        except RuntimeError:
            cmd = 'ls -la %s' % tempfile.gettempdir()
            logging.debug(utils.run(cmd))
            raise

    def get_tests(self):
        return [self.test_using_virtual_filesystem_image]


class platform_CrosDisksRename(test.test):
    version = 1

    def run_once(self, *args, **kwargs):
        test_configs = []
        config_file = '%s/%s' % (self.bindir, kwargs['config_file'])
        with open(config_file, 'rb') as f:
            test_configs.extend(json.load(f))

        tester = CrosDisksRenameTester(self, test_configs)
        tester.run(*args, **kwargs)
