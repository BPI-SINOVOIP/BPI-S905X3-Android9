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
import shutil
import tempfile
import zipfile

from host_controller import common
from vts.runners.host import utils


class BuildProvider(object):
    """The base class for build provider.

    Attributes:
        _IMAGE_FILE_EXTENSIONS: a list of strings which are common image file
                                extensions.
        _BASIC_IMAGE_FILE_NAMES: a list of strings which are the image names in
                                 an artifact zip.
        _CONFIG_FILE_EXTENSION: string, the config file extension.
        _additional_files: a dict containing additionally fetched files that
                           custom features may need. The key is the path
                           relative to temporary directory and the value is the
                           full path.
        _configs: dict where the key is config type and value is the config file
                  path.
        _device_images: dict where the key is image file name and value is the
                        path.
        _test_suites: dict where the key is test suite type and value is the
                      test suite package file path.
        _tmp_dirpath: string, the temp dir path created to keep artifacts.
    """
    _CONFIG_FILE_EXTENSION = ".zip"
    _IMAGE_FILE_EXTENSIONS = [".img", ".bin"]
    _BASIC_IMAGE_FILE_NAMES = ["boot.img", "system.img", "vendor.img"]

    def __init__(self):
        self._additional_files = {}
        self._device_images = {}
        self._test_suites = {}
        self._configs = {}
        tempdir_base = os.path.join(os.getcwd(), "tmp")
        if not os.path.exists(tempdir_base):
            os.mkdir(tempdir_base)
        self._tmp_dirpath = tempfile.mkdtemp(dir=tempdir_base)

    def __del__(self):
        """Deletes the temp dir if still set."""
        if self._tmp_dirpath:
            shutil.rmtree(self._tmp_dirpath)
            self._tmp_dirpath = None

    @property
    def tmp_dirpath(self):
        return self._tmp_dirpath

    def CreateNewTmpDir(self):
        return tempfile.mkdtemp(dir=self._tmp_dirpath)

    def SetDeviceImage(self, name, path):
        """Sets device image `path` for the specified `name`."""
        self._device_images[name] = path

    def _IsFullDeviceImage(self, namelist):
        """Returns true if given namelist list has all common device images."""
        for image_file in self._BASIC_IMAGE_FILE_NAMES:
            if image_file not in namelist:
                return False
        return True

    def _IsImageFile(self, file_path):
        """Returns whether a file is an image.

        Args:
            file_path: string, the file path.

        Returns:
            boolean, whether the file is an image.
        """
        return any(file_path.endswith(ext)
                   for ext in self._IMAGE_FILE_EXTENSIONS)

    def SetDeviceImageZip(self, path):
        """Sets device image(s) using files in a given zip file.

        It extracts image files inside the given zip file and selects
        known Android image files.

        Args:
            path: string, the path to a zip file.
        """
        dest_path = path + ".dir"
        with zipfile.ZipFile(path, 'r') as zip_ref:
            if self._IsFullDeviceImage(zip_ref.namelist()):
                self.SetDeviceImage(common.FULL_ZIPFILE, path)
            else:
                zip_ref.extractall(dest_path)
                self.SetFetchedDirectory(dest_path)

    def GetDeviceImage(self, name=None):
        """Returns device image info."""
        if name is None:
            return self._device_images
        return self._device_images[name]

    def SetTestSuitePackage(self, type, path):
        """Sets test suite package `path` for the specified `type`.

        Args:
            type: string, test suite type such as 'vts' or 'cts'.
            path: string, the path of a file. if a file is a zip file,
                  it's unziped and its main binary is set.
        """
        if path.endswith("android-vts.zip"):
            dest_path = os.path.join(self.tmp_dirpath, "android-vts")
            with zipfile.ZipFile(path, 'r') as zip_ref:
                zip_ref.extractall(dest_path)
                bin_path = os.path.join(dest_path, "android-vts",
                                        "tools", "vts-tradefed")
                os.chmod(bin_path, 0766)
                path = bin_path
        else:
            print("unsupported zip file %s" % path)
        self._test_suites[type] = path

    def GetTestSuitePackage(self, type=None):
        """Returns test suite package info."""
        if type is None:
            return self._test_suites
        return self._test_suites[type]

    def SetConfigPackage(self, config_type, path):
        """Sets test suite package `path` for the specified `type`.

        All valid config files have .zip extension.

        Args:
            config_type: string, config type such as 'prod' or 'test'.
            path: string, the path of a config file.
        """
        if path.endswith(self._CONFIG_FILE_EXTENSION):
            dest_path = os.path.join(
                self.tmp_dirpath, os.path.basename(path) + ".dir")
            with zipfile.ZipFile(path, 'r') as zip_ref:
                zip_ref.extractall(dest_path)
                path = dest_path
        else:
            print("unsupported config package file %s" % path)
        self._configs[config_type] = path

    def GetConfigPackage(self, config_type=None):
        """Returns config package info."""
        if config_type is None:
            return self._configs
        return self._configs[config_type]

    def SetAdditionalFile(self, rel_path, full_path):
        """Sets the key and value of additionally fetched files.

        Args:
            rel_path: the file path relative to temporary directory.
            abs_path: the file path that this process can access.
        """
        self._additional_files[rel_path] = full_path

    def GetAdditionalFile(self, rel_path=None):
        """Returns the paths to fetched files."""
        if rel_path is None:
            return self._additional_files
        return self._additional_files[rel_path]

    def SetFetchedDirectory(self, dir_path, root_path=None):
        """Adds every file in a directory to one of the dictionaries.

        This method follows symlink to file, but skips symlink to directory.

        Args:
            dir_path: string, the directory to find files in.
            root_path: string, the temporary directory that dir_path is in.
                       The default value is dir_path.
        """
        for dir_name, file_name in utils.iterate_files(dir_path):
            full_path = os.path.join(dir_name, file_name)
            self.SetFetchedFile(full_path,
                                (root_path if root_path else dir_path))

    def SetFetchedFile(self, file_path, root_dir=None):
        """Adds a file to one of the dictionaries.

        Args:
            file_path: string, the path to the file.
            root_dir: string, the temporary directory that file_path is in.
                      The default value is file_path if file_path is a
                      directory. Otherwise, the default value is file_path's
                      parent directory.
        """
        file_name = os.path.basename(file_path)
        if os.path.isdir(file_path):
            self.SetFetchedDirectory(file_path, root_dir)
        elif self._IsImageFile(file_path):
            self.SetDeviceImage(file_name, file_path)
        elif file_name == "android-vts.zip":
            self.SetTestSuitePackage("vts", file_path)
        elif file_name.startswith("vti-global-config"):
            self.SetConfigPackage(
                "prod" if "prod" in file_name else "test", file_path)
        elif file_path.endswith(".zip"):
            self.SetDeviceImageZip(file_path)
        else:
            rel_path = (os.path.relpath(file_path, root_dir) if root_dir else
                        os.path.basename(file_path))
            self.SetAdditionalFile(rel_path, file_path)

    def PrintDeviceImageInfo(self):
        """Prints device image info."""
        print("%s" % self.GetDeviceImage())

    def PrintGetTestSuitePackageInfo(self):
        """Prints test suite package info."""
        print("%s" % self.GetTestSuitePackage())
