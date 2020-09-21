#!/usr/bin/env python
#
# Copyright 2016 - The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

"""This module defines an AVD instance.

TODO(fdeng):
  The current implementation only initialize an object
  with IP and instance name. A complete implementation
  will include the following features.
  - Connect
  - Disconnect
  - HasApp
  - InstallApp
  - UninstallApp
  - GrantAppPermission
  Merge cloud/android/platform/devinfra/caci/framework/app_manager.py
  with this module and updated any callers.
"""

import logging

logger = logging.getLogger(__name__)


class AndroidVirtualDevice(object):
    """Represent an Android device."""

    def __init__(self, instance_name, ip=None):
        """Initialize.

        Args:
            instance_name: Name of the gce instance, e.g. "instance-1"
            ip: Ip address of the gce instance, e.g. "140.110.20.1"
        """
        self._ip = ip
        self._instance_name = instance_name

    @property
    def ip(self):
        if not self._ip:
            raise ValueError("IP of instance %s is unknown yet." %
                             self._instance_name)
        return self._ip

    @ip.setter
    def ip(self, value):
        self._ip = value

    @property
    def instance_name(self):
        return self._instance_name

    def __str__(self):
        """Return a string representation."""
        return "<ip: %s, instance_name: %s >" % (self._ip, self._instance_name)
