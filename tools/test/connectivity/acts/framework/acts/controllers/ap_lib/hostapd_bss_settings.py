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

import collections


class BssSettings(object):
    """Settings for a bss.

    Settings for a bss to allow multiple network on a single device.

    Attributes:
        name: string, The name that this bss will go by.
        ssid: string, The name of the ssid to brodcast.
        hidden: bool, If true then the ssid will be hidden.
        security: Security, The security settings to use.
    """

    def __init__(self, name, ssid, hidden=False, security=None, bssid=None):
        self.name = name
        self.ssid = ssid
        self.hidden = hidden
        self.security = security
        self.bssid = bssid

    def generate_dict(self):
        """Returns: A dictionary of bss settings."""
        settings = collections.OrderedDict()
        settings['bss'] = self.name
        if self.bssid:
            settings['bssid'] = self.bssid
        if self.ssid:
            settings['ssid'] = self.ssid
            settings['ignore_broadcast_ssid'] = 1 if self.hidden else 0

        if self.security:
            security_settings = self.security.generate_dict()
            for k, v in security_settings.items():
                settings[k] = v

        return settings
