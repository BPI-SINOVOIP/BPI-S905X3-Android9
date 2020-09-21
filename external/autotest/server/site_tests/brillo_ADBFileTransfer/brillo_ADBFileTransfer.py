# Copyright 2016 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import filecmp
import os
import tempfile

from autotest_lib.client.common_lib import error
from autotest_lib.server import test


_DATA_SIZE = 20 * 1000 * 1000
_DATA_STR = ''.join(chr(i) for i in range(256))
_DATA_STR *= (len(_DATA_STR) - 1 + _DATA_SIZE) / len(_DATA_STR)
_DATA_STR = _DATA_STR[:_DATA_SIZE]


class brillo_ADBFileTransfer(test.test):
    """Verify that ADB file transfers work."""
    version = 1


    def setup(self):
        self.temp_file = tempfile.NamedTemporaryFile()
        self.temp_file.write(_DATA_STR)
        self.temp_file.flush()
        os.fsync(self.temp_file.file.fileno())


    def run_once(self, host=None, iterations=10):
        """
        Conduct a file transfer and verify that it succeeds.

        @param iterations: Number of times to perform the file transfer.
        """

        device_temp_file = os.path.join(host.get_tmp_dir(), 'adb_test_file')

        while iterations:
            iterations -= 1
            with tempfile.NamedTemporaryFile() as return_temp_file:
                host.send_file(self.temp_file.name, device_temp_file,
                               delete_dest=True)
                host.get_file(device_temp_file, return_temp_file.name,
                              delete_dest=True)

                if not filecmp.cmp(self.temp_file.name,
                                   return_temp_file.name, shallow=False):
                    raise error.TestFail('Got back different file than we sent')

            r = host.run('cat %s' % device_temp_file)
            if r.stdout != _DATA_STR:
                raise error.TestFail('Cat did not return same contents we sent')


    def cleanup(self):
        self.temp_file.close()
