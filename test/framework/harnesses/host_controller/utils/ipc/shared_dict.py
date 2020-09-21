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

import multiprocessing

from host_controller import common


class SharedDict(object):
    """Class for the state of devices connected to the host.

    shared between the main process and the process pool.

    Attributes:
        _dict: multiprocessing.SyncManager.dict. A dictionary
               shared among processes.
    """

    def __init__(self):
        manager = multiprocessing.Manager()
        self._dict = manager.dict()

    def __getitem__(self, key):
        """getitem for self._dict.

        Args:
            key: string, serial number of a device.

        Returns:
            integer value defined in _DEVICE_STATUS_DICT.
        """
        if key not in self._dict:
            self._dict[key] = common._DEVICE_STATUS_DICT["unknown"]
        return self._dict[key]

    def __setitem__(self, key, value):
        """setitem for self._dict.

        if the value is not a defined one, device status is set to "unknown".

        Args:
            key: string, serial number of a device.
            value: integer, status value defined in _DEVICE_STATUS_DICT.
        """
        if value not in range(len(common._DEVICE_STATUS_DICT)):
            self._dict[key] = common._DEVICE_STATUS_DICT["unknown"]
        else:
            self._dict[key] = value