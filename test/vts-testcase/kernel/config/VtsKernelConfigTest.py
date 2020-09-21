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

import gzip
import logging
import os
import re
import shutil
import tempfile

from vts.runners.host import asserts
from vts.runners.host import base_test
from vts.runners.host import const
from vts.runners.host import keys
from vts.runners.host import test_runner
from vts.utils.python.android import api
from vts.utils.python.controllers import android_device
from vts.utils.python.file import target_file_utils

from vts.testcases.kernel.lib import version


class VtsKernelConfigTest(base_test.BaseTestClass):
    """Test case which check config options in /proc/config.gz.

    Attributes:
        _temp_dir: The temporary directory to which /proc/config.gz is copied.
    """

    PROC_FILE_PATH = "/proc/config.gz"
    KERNEL_CONFIG_FILE_PATH = "vts/testcases/kernel/config/data"

    def setUpClass(self):
        required_params = [keys.ConfigKeys.IKEY_DATA_FILE_PATH]
        self.getUserParams(required_params)
        self.dut = self.android_devices[0]
        self.shell = self.dut.shell
        self._temp_dir = tempfile.mkdtemp()
        self.supported_kernel_versions = version.getSupportedKernels(self.dut)
        self.release_dir = self.getReleaseDir()

    def getReleaseDir(self):
        """Return the appropriate subdirectory in kernel/configs.

        Returns the directory in kernel/configs corresponding to
        the device's first_api_level.

        Returns:
            string: a directory in kernel configs
        """
        api_level = self.dut.getLaunchApiLevel(strict=False)

        if (api_level == 0):
            logging.info("Cound not detect api level, using last release")
            return "p"
        elif api_level == api.PLATFORM_API_LEVEL_P:
            return "p"
        elif api_level == api.PLATFORM_API_LEVEL_O_MR1:
            return "o-mr1"
        elif api_level <= api.PLATFORM_API_LEVEL_O:
            return "o"
        else:
            return "."

    def checkKernelVersion(self):
        """Validate the kernel version of DUT is a valid kernel version.

        Returns:
            string, kernel version of device
        """
        cmd = "uname -a"
        results = self.shell.Execute(cmd)
        logging.info("Shell command '%s' results: %s", cmd, results)

        match = re.search(r"(\d+)\.(\d+)", results[const.STDOUT][0])
        if match is None:
            asserts.fail("Failed to detect kernel version of device.")
        else:
            kernel_version = int(match.group(1))
            kernel_patchlevel = int(match.group(2))
        logging.info("Detected kernel version: %s", match.group(0))

        for v in self.supported_kernel_versions:
            if (kernel_version == v[0] and kernel_patchlevel == v[1]):
                return match.group(0)
        asserts.fail("Detected kernel version is not one of %s" %
                     self.supported_kernel_versions)

    def checkKernelArch(self, configs):
        """Find arch of the device kernel.

        Uses the kernel configuration to determine the architecture
        it is compiled for.

        Args:
            configs: dict containing device kernel configuration options

        Returns:
            A string containing the architecture of the device kernel. If
            the architecture cannot be determined, an empty string is
            returned.
        """

        CONFIG_ARM = "CONFIG_ARM"
        CONFIG_ARM64 = "CONFIG_ARM64"
        CONFIG_X86 = "CONFIG_X86"

        if CONFIG_ARM in configs and configs[CONFIG_ARM] == "y":
            return "arm"
        elif CONFIG_ARM64 in configs and configs[CONFIG_ARM64] == "y":
            return "arm64"
        elif CONFIG_X86 in configs and configs[CONFIG_X86] == "y":
            return "x86"
        else:
            print "Unable to determine kernel architecture."
            return ""

    def parseConfigFileToDict(self, file, configs):
        """Parse kernel config file to a dictionary.

        Args:
            file: file object, android-base.cfg or unzipped /proc/config.gz
            configs: dict to which config options in file will be added

        Returns:
            dict: {config_name: config_state}
        """
        config_lines = [line.rstrip("\n") for line in file.readlines()]

        for line in config_lines:
            if line.startswith("#") and line.endswith("is not set"):
                match = re.search(r"CONFIG_\S+", line)
                if match is None:
                    asserts.fail("Failed to parse config file")
                else:
                    config_name = match.group(0)
                config_state = "n"
            elif line.startswith("CONFIG_"):
                config_name, config_state = line.split("=", 1)
                if config_state.startswith(("'", '"')):
                    config_state = config_state[1:-1]
            else:
                continue
            configs[config_name] = config_state

        return configs

    def testKernelConfigs(self):
        """Ensures all kernel configs conform to Android requirements.

        Detects kernel version of device and validates against appropriate
        Common Android Kernel android-base.cfg and Android Treble
        requirements.
        """
        logging.info("Testing existence of %s" % self.PROC_FILE_PATH)
        target_file_utils.assertPermissionsAndExistence(
            self.shell, self.PROC_FILE_PATH, target_file_utils.IsReadOnly)

        logging.info("Validating kernel version of device.")
        kernel_version = self.checkKernelVersion()

        # Pull configs from the universal config file.
        configs = dict()
        config_file_path = os.path.join(
            self.data_file_path, self.KERNEL_CONFIG_FILE_PATH,
            self.release_dir, "android-" + kernel_version, "android-base.cfg")
        logging.info("Pulling base cfg from %s", config_file_path)
        with open(config_file_path, 'r') as config_file:
            configs = self.parseConfigFileToDict(config_file, configs)

        # Pull configs from device.
        device_configs = dict()
        self.dut.adb.pull("%s %s" % (self.PROC_FILE_PATH, self._temp_dir))
        logging.info("Adb pull %s to %s", self.PROC_FILE_PATH, self._temp_dir)

        localpath = os.path.join(self._temp_dir, "config.gz")
        with gzip.open(localpath, "rb") as device_config_file:
            device_configs = self.parseConfigFileToDict(
                device_config_file, device_configs)

        # Check device architecture and pull arch-specific configs.
        kernelArch = self.checkKernelArch(device_configs)
        if kernelArch is not "":
            config_file_path = os.path.join(self.data_file_path,
                                            self.KERNEL_CONFIG_FILE_PATH,
                                            self.release_dir,
                                            "android-" + kernel_version,
                                            "android-base-%s.cfg" % kernelArch)
            if os.path.isfile(config_file_path):
                logging.info("Pulling arch cfg from %s", config_file_path)
                with open(config_file_path, 'r') as config_file:
                    configs = self.parseConfigFileToDict(config_file, configs)

        # Determine any deviations from the required configs.
        should_be_enabled = []
        should_not_be_set = []
        incorrect_config_state = []
        for config_name, config_state in configs.iteritems():
            if (config_state == "y" and
                (config_name not in device_configs or
                 device_configs[config_name] not in ("y", "m"))):
                should_be_enabled.append(config_name)
            elif (config_state == "n" and (config_name in device_configs) and
                  device_configs[config_name] != "n"):
                should_not_be_set.append(config_name + "=" +
                                         device_configs[config_name])
            elif (config_name in device_configs and
                  device_configs[config_name] != config_state):
                incorrect_config_state.append(config_name + "=" +
                                              device_configs[config_name])

        if ("CONFIG_OF" not in device_configs and
                "CONFIG_ACPI" not in device_configs):
            should_be_enabled.append("CONFIG_OF | CONFIG_ACPI")

        if ("CONFIG_ANDROID_LOW_MEMORY_KILLER" not in device_configs and
                ("CONFIG_MEMCG" not in device_configs or
                 "CONFIG_MEMCG_SWAP" not in device_configs)):
            should_be_enabled.append("CONFIG_ANDROID_LOW_MEMORY_KILLER | "
                                     "(CONFIG_MEMCG & CONFIG_MEMCG_SWAP)")

        asserts.assertTrue(
            len(should_be_enabled) == 0 and len(should_not_be_set) == 0 and
            len(incorrect_config_state) == 0,
            ("The following kernel configs should be enabled: [%s]\n"
             "The following kernel configs should not be set: [%s]\n"
             "THe following kernel configs have incorrect state: [%s]") %
            (", ".join(should_be_enabled), ", ".join(should_not_be_set),
             ", ".join(incorrect_config_state)))

    def tearDownClass(self):
        """Deletes the temporary directory."""
        logging.info("Delete %s", self._temp_dir)
        shutil.rmtree(self._temp_dir)


if __name__ == "__main__":
    test_runner.main()
