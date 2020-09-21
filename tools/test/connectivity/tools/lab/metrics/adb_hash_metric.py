#!/usr/bin/env python
#
#   Copyright 2017 - The Android Open Source Project
#
#   Licensed under the Apache License, Version 2.0 (the "License");
#   you may not use this file except in compliance with the License.
#   You may obtain a copy of the License at
#
#       http://www.apache.org/licenses/LICENSE-2.0
#
#   Unless required by applicable law or agreed to in writing, software
#   distributed under the License is distributed on an "AS IS" BASIS,
#   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#   See the License for the specific language governing permissions and
#   limitations under the License.

import os

from metrics.metric import Metric


class AdbHashMetric(Metric):
    """Gathers metrics on environment variable and a hash of that directory.

    This class will verify that $ADB_VENDOR_KEYS is in the environment variables,
    return True or False, and then verify that the hash of that directory
    matches the 'golden' directory.
    """
    _ADV_ENV_VAR = 'ADB_VENDOR_KEYS'
    _MD5_COMMAND = ('find $ADB_VENDOR_KEYS -not -path \'*/\.*\' -type f '
                    '-exec md5sum {} + | awk \'{print $1}\' | sort | md5sum')
    KEYS_PATH = 'keys_path'
    KEYS_HASH = 'hash'

    def _verify_env(self):
        """Determines the path of ADB_VENDOR_KEYS.

        Returns:
            The path to $ADB_VENDOR_KEYS location, or None if env variable isn't
            set.
        """
        try:
            return os.environ[self._ADV_ENV_VAR]
        except KeyError:
            return None

    def _find_hash(self):
        """Determines the hash of keys in $ADB_VENDOR_KEYS folder.

        As of now, it just gets the hash, and returns it.

        Returns:
            The hash of the $ADB_VENDOR_KEYS directory excluding hidden files.
        """
        return self._shell.run(self._MD5_COMMAND).stdout.split(' ')[0]

    def gather_metric(self):
        """Gathers data on adb keys environment variable, and the hash of the
        directory.

        Returns:
            A dictionary with 'env' set to the location of adb_vendor_keys, and
            key 'hash' with an md5sum as value.
        """
        adb_keys_path = self._verify_env()
        if adb_keys_path is not None:
            adb_keys_hash = self._find_hash()
        else:
            adb_keys_hash = None

        return {self.KEYS_PATH: adb_keys_path, self.KEYS_HASH: adb_keys_hash}
