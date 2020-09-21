#!/usr/bin/env python
# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import collections
import json
import os
import tempfile
import unittest
from contextlib import contextmanager

import common
from autotest_lib.client.bin import utils
from autotest_lib.site_utils.lxc import config as lxc_config
from autotest_lib.site_utils.lxc import utils as lxc_utils

class DeployConfigTest(unittest.TestCase):
    """Test DeployConfigManager.
    """

    def testValidate(self):
        """Test ssp_deploy_config.json can be validated.
        """
        global_deploy_config_file = os.path.join(
                common.autotest_dir, lxc_config.SSP_DEPLOY_CONFIG_FILE)
        with open(global_deploy_config_file) as f:
            deploy_configs = json.load(f)
        for config in deploy_configs:
            if 'append' in config:
                lxc_config.DeployConfigManager.validate(config)
            elif 'mount' in config:
                # validate_mount checks that the path exists, so we can't call
                # it from tests.
                pass
            else:
                self.fail('Non-deploy/mount config %s' % config)


    def testPreStart(self):
        """Verifies that pre-start works correctly.
        Checks that mounts are correctly created in the container.
        """
        with lxc_utils.TempDir() as tmpdir:
            config = [
                {
                    'mount': True,
                    'source': tempfile.mkdtemp(dir=tmpdir),
                    'target': '/target0',
                    'readonly': True,
                    'force_create': False
                },
                {
                    'mount': True,
                    'source': tempfile.mkdtemp(dir=tmpdir),
                    'target': '/target1',
                    'readonly': False,
                    'force_create': False
                },
            ]
            with ConfigFile(config) as test_cfg, MockContainer() as container:
                manager = lxc_config.DeployConfigManager(container, test_cfg)
                manager.deploy_pre_start()
                self.assertEqual(len(config), len(container.mounts))
                for c in config:
                    self.assertTrue(container.has_mount(c))


    def testPreStartWithCreate(self):
        """Verifies that pre-start creates mounted dirs.

        Checks that missing mount points are created when force_create is
        enabled.
        """
        with lxc_utils.TempDir() as tmpdir:
            src_dir = os.path.join(tmpdir, 'foobar')
            config = [{
                'mount': True,
                'source': src_dir,
                'target': '/target0',
                'readonly': True,
                'force_create': True
            }]
            with ConfigFile(config) as test_cfg, MockContainer() as container:
                manager = lxc_config.DeployConfigManager(container, test_cfg)
                # Pre-condition: the path doesn't exist.
                self.assertFalse(lxc_utils.path_exists(src_dir))

                # After calling deploy_pre_start, the path should exist and the
                # mount should be created in the container.
                manager.deploy_pre_start()
                self.assertTrue(lxc_utils.path_exists(src_dir))
                self.assertEqual(len(config), len(container.mounts))
                for c in config:
                    self.assertTrue(container.has_mount(c))


class _MockContainer(object):
    """A test mock for the container class.

    Don't instantiate this directly, use the MockContainer context manager
    defined below.
    """

    def __init__(self):
        self.rootfs = tempfile.mkdtemp()
        self.mounts = []
        self.MountConfig = collections.namedtuple(
                'MountConfig', ['source', 'destination', 'readonly'])


    def cleanup(self):
        """Clean up tmp dirs created by the container."""
        # DeployConfigManager uses sudo to create some directories in the
        # container, so it's necessary to use sudo to clean up.
        utils.run('sudo rm -rf %s' % self.rootfs)


    def mount_dir(self, src, dst, ro):
        """Stub implementation of mount_dir.

        Records calls for later verification.

        @param src: Mount source dir.
        @param dst: Mount destination dir.
        @param ro: Read-only flag.
        """
        self.mounts.append(self.MountConfig(src, dst, ro))


    def has_mount(self, config):
        """Verifies whether an earlier call was made to mount_dir.

        @param config: The config object to verify.

        @return True if an earlier call was made to mount_dir that matches the
                given mount configuration; False otherwise.
        """
        mount = self.MountConfig(config['source'],
                                 config['target'],
                                 config['readonly'])
        return mount in self.mounts


@contextmanager
def MockContainer():
    """Context manager for creating a _MockContainer for testing."""
    container = _MockContainer()
    try:
        yield container
    finally:
        container.cleanup()


@contextmanager
def ConfigFile(config):
    """Context manager for creating a config file.

    The given configs are translated into json and pushed into a temporary file
    that the DeployConfigManager can read.

    @param config: A list of config objects.  Each config object is a dictionary
                   which conforms to the format described in config.py.
    """
    with tempfile.NamedTemporaryFile() as tmp:
        json.dump(config, tmp)
        tmp.flush()
        yield tmp.name


if __name__ == '__main__':
    unittest.main()
