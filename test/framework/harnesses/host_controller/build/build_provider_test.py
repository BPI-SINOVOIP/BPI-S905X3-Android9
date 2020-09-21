#!/usr/bin/env python
#
# Copyright (C) 2018 The Android Open Source Project
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
import shutil
import tempfile
import unittest
import zipfile

from host_controller import common
from host_controller.build import build_provider

try:
    from unittest import mock
except ImportError:
    import mock


class BuildProviderTest(unittest.TestCase):
    """Tests for build_provider.

    Attributes:
        _build_provider: The BuildProvider object under test.
        _temp_dir: The path to the temporary directory for test files.
    """

    def setUp(self):
        """Creates temporary directory."""
        self._build_provider = build_provider.BuildProvider()
        self._temp_dir = tempfile.mkdtemp()

    def tearDown(self):
        """Deletes temporary directory."""
        shutil.rmtree(self._temp_dir)

    def _CreateFile(self, name):
        """Creates an empty file as test data.

        Args:
            name: string, the name of the file.

        Returns:
            string, the path to the file.
        """
        path = os.path.join(self._temp_dir, name)
        with open(path, "w"):
            pass
        return path

    def _CreateZip(self, name, *paths):
        """Creates a zip file containing empty files.

        Args:
            name: string, the name of the zip file.
            *paths: strings, the file paths to create in the zip file.

        Returns:
            string, the path to the zip file.
        """
        empty_path = self._CreateFile("empty")
        zip_path = os.path.join(self._temp_dir, name)
        with zipfile.ZipFile(zip_path, "w") as zip_file:
            for path in paths:
                zip_file.write(empty_path, path)
        return zip_path

    def _CreateVtsPackage(self):
        """Creates an android-vts.zip containing vts-tradefed.

        Returns:
            string, the path to the zip file.
        """
        return self._CreateZip(
            "android-vts.zip",
            os.path.join("android-vts", "tools", "vts-tradefed"))

    def _CreateDeviceImageZip(self):
        """Creates a zip containing common device images.

        Returns:
            string, the path to the zip file.
        """
        return self._CreateZip(
            "img.zip", "system.img", "vendor.img", "boot.img")

    def _CreateProdConfig(self):
        """Creates a zip containing config files.

        Returns:
            string, the path to the zip file.
        """
        return self._CreateZip(
            "vti-global-config-prod.zip", os.path.join("test", "prod.config"))

    def testSetTestSuitePackage(self):
        """Tests setting a VTS package."""
        vts_path = self._CreateVtsPackage()
        self._build_provider.SetTestSuitePackage("vts", vts_path)

        self.assertTrue(
            os.path.exists(self._build_provider.GetTestSuitePackage("vts")))

    def testSetDeviceImageZip(self):
        """Tests setting a device image zip."""
        img_path = self._CreateDeviceImageZip()
        self._build_provider.SetDeviceImageZip(img_path)

        self.assertEqual(
            img_path,
            self._build_provider.GetDeviceImage(common.FULL_ZIPFILE))

    def testSetConfigPackage(self):
        """Tests setting a config package."""
        config_path = self._CreateProdConfig()
        self._build_provider.SetConfigPackage("prod", config_path)

        self.assertTrue(
            os.path.exists(self._build_provider.GetConfigPackage("prod")))

    def testSetFetchedDirectory(self):
        """Tests setting all files in a directory."""
        self._CreateVtsPackage()
        self._CreateProdConfig()
        img_zip = self._CreateDeviceImageZip()
        img_file = self._CreateFile("userdata.img")
        txt_file = self._CreateFile("additional.txt")
        self._build_provider.SetFetchedDirectory(self._temp_dir)

        self.assertDictContainsSubset(
            {common.FULL_ZIPFILE: img_zip, "userdata.img": img_file},
            self._build_provider.GetDeviceImage())
        self.assertTrue(
            os.path.exists(self._build_provider.GetTestSuitePackage("vts")))
        self.assertTrue(
            os.path.exists(self._build_provider.GetConfigPackage("prod")))
        self.assertDictContainsSubset(
            {"additional.txt": txt_file},
            self._build_provider.GetAdditionalFile())


if __name__ == "__main__":
    unittest.main()
