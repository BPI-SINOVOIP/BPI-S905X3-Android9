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

import logging
import os
import shutil
import tempfile

from vts.runners.host import asserts
from vts.runners.host import base_test
from vts.runners.host import const
from vts.runners.host import keys
from vts.runners.host import test_runner
from vts.utils.python.file import target_file_utils


class VtsTrebleSysPropTest(base_test.BaseTestClass):
    """Test case which check compatibility of system property.

    Attributes:
        _temp_dir: The temporary directory to which necessary files are copied.
        _PUBLIC_PROPERTY_CONTEXTS_FILE_PATH: The path of public property
                                             contexts file.
        _SYSTEM_PROPERTY_CONTEXTS_FILE_PATH: The path of system property
                                             contexts file.
        _VENDOR_PROPERTY_CONTEXTS_FILE_PATH: The path of vendor property
                                             contexts file.
        _VENDOR_OR_ODM_NAMEPACES: The namepsaces allowed for vendor/odm
                                  properties.
    """

    _PUBLIC_PROPERTY_CONTEXTS_FILE_PATH = ("vts/testcases/security/"
                                           "system_property/data/"
                                           "property_contexts")
    _SYSTEM_PROPERTY_CONTEXTS_FILE_PATH = ("/system/etc/selinux/"
                                           "plat_property_contexts")
    _VENDOR_PROPERTY_CONTEXTS_FILE_PATH = ("/vendor/etc/selinux/"
                                           "vendor_property_contexts")
    _VENDOR_OR_ODM_NAMEPACES = [
            "ctl.odm.",
            "ctl.vendor.",
            "ctl.start$odm.",
            "ctl.start$vendor.",
            "ctl.stop$odm.",
            "ctl.stop$vendor.",
            "ro.boot.",
            "ro.hardware.",
            "ro.odm.",
            "ro.vendor.",
            "odm.",
            "persist.odm.",
            "persist.vendor.",
            "vendor."
    ]

    def setUpClass(self):
        """Initializes tests.

        Data file path, device, remote shell instance and temporary directory
        are initialized.
        """
        required_params = [keys.ConfigKeys.IKEY_DATA_FILE_PATH]
        self.getUserParams(required_params)
        self.dut = self.android_devices[0]
        self.dut.shell.InvokeTerminal( "TrebleSysPropTest")
        self.shell = self.dut.shell.TrebleSysPropTest
        self._temp_dir = tempfile.mkdtemp()

    def tearDownClass(self):
        """Deletes the temporary directory."""
        logging.info("Delete %s", self._temp_dir)
        shutil.rmtree(self._temp_dir)

    def _ParsePropertyDictFromPropertyContextsFile(
            self, property_contexts_file, exact_only=False):
        """Parse property contexts file to a dictionary.

        Args:
            property_contexts_file: file object of property contexts file
            exact_only: whether parsing only properties which require exact
                        matching

        Returns:
            dict: {property_name: property_tokens}
        """
        property_dict = dict()
        for line in property_contexts_file.readlines():
            tokens = line.strip().rstrip("\n").split()
            if len(tokens) > 0 and not tokens[0].startswith("#"):
                if not exact_only:
                    property_dict[tokens[0]] = tokens
                elif len(tokens) >= 4 and tokens[2] == "exact":
                    property_dict[tokens[0]] = tokens

        return property_dict

    def testActionableCompatiblePropertyEnabled(self):
        """Ensures the feature of actionable compatible property is enforced.

        ro.actionable_compatible_property.enabled must be true to enforce the
        feature of actionable compatible property.
        """
        asserts.assertEqual(
                self.dut.getProp("ro.actionable_compatible_property.enabled"),
                "true",
                "ro.actionable_compatible_property.enabled must be true")

    def testVendorPropertyNamespace(self):
        """Ensures vendor properties have proper namespace.

        Vendor or ODM properties must have their own prefix.
        """
        logging.info("Checking existence of %s",
                     self._VENDOR_PROPERTY_CONTEXTS_FILE_PATH)
        target_file_utils.assertPermissionsAndExistence(
                self.shell, self._VENDOR_PROPERTY_CONTEXTS_FILE_PATH,
                target_file_utils.IsReadable)

        # Pull vendor property contexts file from device.
        self.dut.adb.pull("%s %s" % (self._VENDOR_PROPERTY_CONTEXTS_FILE_PATH,
                                     self._temp_dir))
        logging.info("Adb pull %s to %s",
                     self._VENDOR_PROPERTY_CONTEXTS_FILE_PATH,
                     self._temp_dir)

        with open(os.path.join(self._temp_dir, "vendor_property_contexts"),
                  "r") as property_contexts_file:
            property_dict = self._ParsePropertyDictFromPropertyContextsFile(
                    property_contexts_file)
        logging.info("Found %d property names in vendor property contexts",
                     len(property_dict))
        for name in property_dict:
            has_proper_namesapce = False
            for prefix in self._VENDOR_OR_ODM_NAMEPACES:
                if name.startswith(prefix):
                    has_proper_namesapce = True
                    break
            asserts.assertTrue(
                    has_proper_namesapce,
                    "Vendor property (%s) has wrong namespace" % name)

    def testExportedPlatformPropertyIntegrity(self):
        """Ensures public property contexts isn't modified at all.

        Public property contexts must not be modified.
        """
        logging.info("Checking existence of %s",
                     self._SYSTEM_PROPERTY_CONTEXTS_FILE_PATH)
        target_file_utils.assertPermissionsAndExistence(
                self.shell, self._SYSTEM_PROPERTY_CONTEXTS_FILE_PATH,
                target_file_utils.IsReadable)

        # Pull system property contexts file from device.
        self.dut.adb.pull("%s %s" % (self._SYSTEM_PROPERTY_CONTEXTS_FILE_PATH,
                                     self._temp_dir))
        logging.info("Adb pull %s to %s",
                     self._SYSTEM_PROPERTY_CONTEXTS_FILE_PATH,
                     self._temp_dir)

        with open(os.path.join(self._temp_dir, "plat_property_contexts"),
                  "r") as property_contexts_file:
            sys_property_dict = self._ParsePropertyDictFromPropertyContextsFile(
                    property_contexts_file, True)
        logging.info("Found %d exact-matching properties "
                     "in system property contexts", len(sys_property_dict))

        pub_property_contexts_file_path = os.path.join(
                self.data_file_path, self._PUBLIC_PROPERTY_CONTEXTS_FILE_PATH)
        with open(pub_property_contexts_file_path,
                  "r") as property_contexts_file:
            pub_property_dict = self._ParsePropertyDictFromPropertyContextsFile(
                    property_contexts_file, True)

        for name in pub_property_dict:
            public_tokens = pub_property_dict[name]
            asserts.assertTrue(name in sys_property_dict,
                               "Exported property (%s) doesn't exist" % name)
            system_tokens = sys_property_dict[name]
            asserts.assertEqual(public_tokens, system_tokens,
                                "Exported property (%s) is modified" % name)


if __name__ == "__main__":
    test_runner.main()
