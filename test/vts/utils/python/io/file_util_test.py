#!/usr/bin/env python
#
# Copyright (C) 2017 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

import os
import random
import string
import unittest

from vts.utils.python.io import file_util


class FileUtilTest(unittest.TestCase):
    def setUp(self):
        """Resets generated directory list"""
        self._dirs = []

    def tearDown(self):
        """Removes existing directories"""
        for path in self._dirs:
            file_util.Rmdirs(path)

    def testMakeAndRemoveDirs(self):
        """Tests making and removing directories """
        dir_name = ''.join(
            random.choice(string.ascii_lowercase + string.digits)
            for _ in range(12))
        self._dirs.append(dir_name)

        # make non-existing directory
        result = file_util.Makedirs(dir_name)
        self.assertEqual(True, result)

        # make existing directory
        result = file_util.Makedirs(dir_name)
        self.assertEqual(False, result)

        # delete existing directory
        result = file_util.Rmdirs(dir_name)
        self.assertEqual(True, result)

        # delete non-existing directory
        result = file_util.Rmdirs(dir_name)
        self.assertEqual(False, result)

    def testMakeTempDir(self):
        """Tests making temp directory """
        base_dir = ''.join(
            random.choice(string.ascii_lowercase + string.digits)
            for _ in range(12))
        self._dirs.append(base_dir)

        # make temp directory
        result = file_util.MakeTempDir(base_dir)
        self.assertTrue(os.path.join(base_dir, "tmp"))

    def testMakeException(self):
        """Tests making directory and raise exception """
        dir_name = ''.join(
            random.choice(string.ascii_lowercase + string.digits)
            for _ in range(12))
        self._dirs.append(dir_name)

        file_util.Makedirs(dir_name)
        self.assertRaises(Exception,
                          file_util.Makedirs(dir_name, skip_if_exists=False))

    def testRemoveException(self):
        """Tests removing directory and raise exception """
        dir_name = ''.join(
            random.choice(string.ascii_lowercase + string.digits)
            for _ in range(12))
        self._dirs.append(dir_name)

        link_name = ''.join(
            random.choice(string.ascii_lowercase + string.digits)
            for _ in range(12))

        file_util.Makedirs(dir_name)
        os.symlink(dir_name, link_name)
        try:
            self.assertRaises(Exception,
                              file_util.Rmdirs(link_name, ignore_errors=False))
        finally:
            os.remove(link_name)


if __name__ == "__main__":
    unittest.main()
