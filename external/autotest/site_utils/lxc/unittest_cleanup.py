#!/usr/bin/python
# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import os

import common
from autotest_lib.client.bin import utils
from autotest_lib.client.common_lib import error
from autotest_lib.site_utils import lxc
from autotest_lib.site_utils.lxc import utils as lxc_utils


TEST_CONTAINER_PATH = os.path.join(lxc.DEFAULT_CONTAINER_PATH, 'test')
TEST_HOST_PATH = os.path.join(TEST_CONTAINER_PATH, 'host')

def main():
    """Clean up the remnants from any old aborted unit tests."""
    # Manually clean out the host dir.
    if lxc_utils.path_exists(TEST_HOST_PATH):
        for host_dir in os.listdir(TEST_HOST_PATH):
            host_dir = os.path.realpath(os.path.join(TEST_HOST_PATH, host_dir))
            try:
                utils.run('sudo umount %s' % host_dir)
            except error.CmdError:
                pass
            utils.run('sudo rm -r %s' % host_dir)

    # Utilize the container_bucket to clear out old test containers.
    bucket = lxc.ContainerBucket(TEST_CONTAINER_PATH, TEST_HOST_PATH)
    bucket.destroy_all()


if __name__ == '__main__':
    main()
