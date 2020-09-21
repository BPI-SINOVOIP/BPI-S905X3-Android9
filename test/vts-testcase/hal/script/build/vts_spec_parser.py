#!/usr/bin/env python3.4
#
# Copyright (C) 2017 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the 'License');
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an 'AS IS' BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

import os
import re
import shutil
import subprocess
import sys
import tempfile

from utils.const import Constant

ANDROID_BUILD_TOP = os.environ.get('ANDROID_BUILD_TOP')
if not ANDROID_BUILD_TOP:
    print 'Run "lunch" command first.'
    sys.exit(1)

# TODO(trong): use proper packaging without referencing modules from source.
TEST_VTS_DIR = os.path.join(ANDROID_BUILD_TOP, 'test', 'vts')
sys.path.append(TEST_VTS_DIR)
from proto import ComponentSpecificationMessage_pb2 as CompSpecMsg
from google.protobuf import text_format
import build_rule_gen_utils as utils


class VtsSpecParser(object):
    """Provides an API to generate a parse .vts spec files."""

    def __init__(self,
                 package_root=Constant.HAL_PACKAGE_PREFIX,
                 path_root=Constant.HAL_INTERFACE_PATH):
        """VtsSpecParser constructor.

        For every unique pair of (hal name, hal version) available under
        path_root, generates .vts files using hidl-gen.

        Args:
            tmp_dir: string, temporary directory to which to write .vts files.
        """
        self._cache = set()
        self._tmp_dir = tempfile.mkdtemp()
        self._package_root = package_root
        self._path_root = path_root
        hal_list = self.HalNamesAndVersions()

    def __del__(self):
        """VtsSpecParser destructor.

        Removes all temporary files that were generated.
        """
        print "Removing temp files."
        if os.path.exists(self._tmp_dir):
            shutil.rmtree(self._tmp_dir)

    def ImportedPackagesList(self, hal_name, hal_version):
        """Returns a list of imported packages.

        Args:
          hal_name: string, name of the hal, e.g. 'vibrator'.
          hal_version: string, version of the hal, e.g '7.4'

        Returns:
          list of strings. For example,
              ['android.hardware.vibrator@1.3', 'android.hidl.base@1.7']
        """
        self.GenerateVtsSpecs(hal_name, hal_version)
        vts_spec_protos = self.VtsSpecProtos(hal_name, hal_version)

        imported_packages = set()
        for vts_spec in vts_spec_protos:
            for package in getattr(vts_spec, 'import', []):
                package = package.split('::')[0]
                imported_packages.add(package)

        # Exclude the current package and packages with no corresponding libs.
        exclude_packages = [
            "android.hidl.base@1.0", "android.hidl.manager@1.0",
            '%s.%s@%s' % (self._package_root, hal_name, hal_version)
        ]

        return sorted(list(set(imported_packages) - set(exclude_packages)))

    def GenerateVtsSpecs(self, hal_name, hal_version):
        """Generates VTS specs.

        Uses hidl-gen to generate .vts files under a tmp directory.

        Args:
          hal_name: string, name of the hal, e.g. 'vibrator'.
          hal_version: string, version of the hal, e.g '7.4'
          tmp_dir: string, location to which to write tmp files.
        """
        if (hal_name, hal_version) in self._cache:
            return
        hidl_gen_cmd = (
            'hidl-gen -o {TEMP_DIR} -L vts -r {PACKAGE_ROOT}:{PATH_ROOT} '
            '{PACKAGE_ROOT}.{HAL_NAME}@{HAL_VERSION}').format(
                TEMP_DIR=self._tmp_dir,
                PACKAGE_ROOT=self._package_root,
                PATH_ROOT=self._path_root,
                HAL_NAME=hal_name,
                HAL_VERSION=hal_version)
        subprocess.call(hidl_gen_cmd, shell=True)
        self._cache.add((hal_name, hal_version))

    def HalNamesAndVersions(self):
        """Returns a list of hals and versions under hal interface directory.

        Returns:
            List of tuples of strings containing hal names and hal versions.
            For example, [('vibrator', '1.3'), ('sensors', '1.7')]
        """
        full_path_root = os.path.join(ANDROID_BUILD_TOP, self._path_root)
        result = set()
        # Walk through ANDROID_BUILD_TOP/self._path_root and heuristically
        # figure out all the HAL names and versions in the source tree.
        for base, dirs, files in os.walk(full_path_root):
            has_hals = any(f.endswith('.hal') for f in files)
            if not has_hals:
                continue

            hal_dir = os.path.relpath(base, full_path_root)
            # Find the first occurance of version in directory path.
            match = re.search("(\d+)\.(\d+)", hal_dir)
            if match and 'example' not in hal_dir:
                hal_version = match.group(0)
                # Name of the hal preceds hal version in the directory path.
                hal_dir = hal_dir[:match.end()]
                hal_name = os.path.dirname(hal_dir).replace('/', '.')
                result.add((hal_name, hal_version))
        return sorted(result)

    def VtsSpecNames(self, hal_name, hal_version):
        """Returns list of .vts file names for given hal name and version.

        hal_name: string, name of the hal, e.g. 'vibrator'.
        hal_version: string, version of the hal, e.g '7.4'

        Returns:
          list of string, .vts files for given hal name and version,
              e.g. ['Vibrator.vts', 'types.vts']
        """
        self.GenerateVtsSpecs(hal_name, hal_version)
        vts_spec_dir = os.path.join(self._tmp_dir,
                                    self._package_root.replace('.', '/'),
                                    utils.HalNameDir(hal_name), hal_version)
        vts_spec_names = filter(lambda x: x.endswith('.vts'),
                                os.listdir(vts_spec_dir))
        return sorted(vts_spec_names)

    def VtsSpecProtos(self, hal_name, hal_version):
        """Returns list of .vts protos for given hal name and version.

        hal_name: string, name of the hal, e.g. 'vibrator'.
        hal_version: string, version of the hal, e.g '7.4'

        Returns:
          list of ComponentSpecificationMessages
        """
        self.GenerateVtsSpecs(hal_name, hal_version)
        vts_spec_dir = os.path.join(self._tmp_dir,
                                    self._package_root.replace('.', '/'),
                                    utils.HalNameDir(hal_name), hal_version)
        vts_spec_protos = []
        for vts_spec in self.VtsSpecNames(hal_name, hal_version):
            spec_proto = CompSpecMsg.ComponentSpecificationMessage()
            vts_spec_path = os.path.join(vts_spec_dir, vts_spec)
            with open(vts_spec_path, 'r') as spec_file:
                spec_string = spec_file.read()
                text_format.Merge(spec_string, spec_proto)

            vts_spec_protos.append(spec_proto)
        return vts_spec_protos
