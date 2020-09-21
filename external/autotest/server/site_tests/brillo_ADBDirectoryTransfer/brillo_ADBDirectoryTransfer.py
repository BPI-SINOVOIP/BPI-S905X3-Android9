# Copyright 2016 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import filecmp
import os
import shutil
import tempfile

from autotest_lib.client.common_lib import error
from autotest_lib.server import test


_DATA_STR_A = 'Alluminum, linoleum, magnesium, petrolium.'
_DATA_STR_B = ('A basket of biscuits, a basket of mixed biscuits,'
               'and a biscuit mixer.')
_DATA_STR_C = 'foo\nbar\nbaz'


class brillo_ADBDirectoryTransfer(test.test):
    """Verify that ADB directory transfers work."""
    version = 1


    def setup(self):
        # Create a test directory tree to send and receive:
        #   test_dir/
        #     file_a
        #     file_b
        #     test_subdir/
        #       file_c
        self.temp_dir = tempfile.mkdtemp()
        self.test_dir = os.path.join(self.temp_dir, 'test_dir')
        os.mkdir(self.test_dir)
        os.mkdir(os.path.join(self.test_dir, 'subdir'))

        with open(os.path.join(self.test_dir, 'file_a'), 'w') as f:
            f.write(_DATA_STR_A)

        with open(os.path.join(self.test_dir, 'file_b'), 'w') as f:
            f.write(_DATA_STR_B)

        with open(os.path.join(self.test_dir, 'subdir', 'file_c'), 'w') as f:
            f.write(_DATA_STR_C)


    def run_once(self, host=None):
        """Body of the test."""
        device_temp_dir = host.get_tmp_dir()
        device_test_dir = os.path.join(device_temp_dir, 'test_dir')

        return_dir = os.path.join(self.temp_dir, 'return_dir')
        return_test_dir = os.path.join(return_dir, 'test_dir')

        # Copy test_dir to the device then back into return_dir.
        host.send_file(self.test_dir, device_temp_dir, delete_dest=True)
        host.get_file(device_test_dir, return_dir, delete_dest=True)

        for path in ('file_a', 'file_b', os.path.join('subdir', 'file_c')):
            original = os.path.join(self.test_dir, path)
            returned = os.path.join(return_test_dir, path)
            if not filecmp.cmp(original, returned, shallow=False):
                raise error.TestFail(path + ' changed in transit')


    def cleanup(self):
        shutil.rmtree(self.temp_dir)
