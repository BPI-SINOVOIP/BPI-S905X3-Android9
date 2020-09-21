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

from acts.controllers.ap_lib import hostapd_constants


class Security(object):
    """The Security class for hostapd representing some of the security
       settings that are allowed in hostapd.  If needed more can be added.
    """

    def __init__(self,
                 security_mode=None,
                 password=None,
                 wpa_cipher=hostapd_constants.WPA_DEFAULT_CIPHER,
                 wpa2_cipher=hostapd_constants.WPA2_DEFAULT_CIPER,
                 wpa_group_rekey=hostapd_constants.WPA_GROUP_KEY_ROTATION_TIME,
                 wpa_strict_rekey=hostapd_constants.WPA_STRICT_REKEY_DEFAULT):
        """Gather all of the security settings for WPA-PSK.  This could be
           expanded later.

        Args:
            security_mode: Type of security modes.
                           Options: wpa, wpa2, wpa/wpa2
            password: The PSK or passphrase for the security mode.
            wpa_cipher: The cipher to be used for wpa.
                        Options: TKIP, CCMP, TKIP CCMP
                        Default: TKIP
            wpa2_cipher: The cipher to be used for wpa2.
                         Options: TKIP, CCMP, TKIP CCMP
                         Default: CCMP
            wpa_group_rekey: How often to refresh the GTK regardless of network
                             changes.
                             Options: An integrer in seconds, None
                             Default: 600 seconds
            wpa_strict_rekey: Whether to do a group key update when client
                              leaves the network or not.
                              Options: True, False
                              Default: True
        """
        self.wpa_cipher = wpa_cipher
        self.wpa2_cipher = wpa2_cipher
        self.wpa_group_rekey = wpa_group_rekey
        self.wpa_strict_rekey = wpa_strict_rekey
        if security_mode == hostapd_constants.WPA_STRING:
            security_mode = hostapd_constants.WPA1
        elif security_mode == hostapd_constants.WPA2_STRING:
            security_mode = hostapd_constants.WPA2
        elif security_mode == hostapd_constants.WPA_MIXED_STRING:
            security_mode = hostapd_constants.MIXED
        else:
            security_mode = None
        self.security_mode = security_mode
        if password:
            if len(password) < hostapd_constants.MIN_WPA_PSK_LENGTH or len(
                    password) > hostapd_constants.MAX_WPA_PSK_LENGTH:
                raise ValueError(
                    'Password must be a minumum of %s characters and a maximum of %s'
                    % (hostapd_constants.MIN_WPA_PSK_LENGTH,
                       hostapd_constants.MAX_WPA_PSK_LENGTH))
            else:
                self.password = password

    def generate_dict(self):
        """Returns: an ordered dictionary of settings"""
        settings = collections.OrderedDict()
        if self.security_mode:
            settings['wpa'] = self.security_mode
            if len(self.password) == hostapd_constants.MAX_WPA_PSK_LENGTH:
                settings['wpa_psk'] = self.password
            else:
                settings['wpa_passphrase'] = self.password

            if self.security_mode == hostapd_constants.MIXED:
                settings['wpa_pairwise'] = self.wpa_cipher
                settings['rsn_pairwise'] = self.wpa2_cipher
            elif self.security_mode == hostapd_constants.WPA1:
                settings['wpa_pairwise'] = self.wpa_cipher
            elif self.security_mode == hostapd_constants.WPA2:
                settings['rsn_pairwise'] = self.wpa2_cipher

            if self.wpa_group_rekey:
                settings['wpa_group_rekey'] = self.wpa_group_rekey
            if self.wpa_strict_rekey:
                settings[
                    'wpa_strict_rekey'] = hostapd_constants.WPA_STRICT_REKEY
        return settings
