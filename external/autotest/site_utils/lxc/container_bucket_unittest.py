#!/usr/bin/python
# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import os
import shutil
import tempfile
import unittest

import common
from autotest_lib.site_utils import lxc
from autotest_lib.site_utils.lxc import unittest_setup
from autotest_lib.site_utils.lxc import container_bucket


container_path = None

def setUpModule():
    """Creates a directory for running the unit tests. """
    global container_path
    container_path = tempfile.mkdtemp(
            dir=lxc.DEFAULT_CONTAINER_PATH,
            prefix='container_bucket_unittest_')


def tearDownModule():
    """Deletes the test directory. """
    shutil.rmtree(container_path)


class DummyClient(object):
    """Mock client for bucket test"""
    def get_container(*args, **xargs):
        return None


class ContainerBucketTests(unittest.TestCase):
    """Unit tests for the ContainerBucket class."""

    def setUp(self):
        self.tmpdir = tempfile.mkdtemp()
        self.shared_host_path = os.path.realpath(os.path.join(self.tmpdir,
                                                              'host'))

    def tearDown(self):
        shutil.rmtree(self.tmpdir)


    def testEmptyPool(self):
        """Verifies that the bucket falls back to creating a new container if it
        can't get one from the pool."""
        id = lxc.ContainerId.create(3)
        factory = container_bucket.ContainerBucket()._factory
        factory._client = DummyClient()
        container = factory.create_container(id)
        self.assertIsNotNone(container)


if __name__ == '__main__':
    unittest_setup.setup()
    unittest.main()
