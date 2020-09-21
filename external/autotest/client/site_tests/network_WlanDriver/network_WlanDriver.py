# Copyright (c) 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging

from autotest_lib.client.bin import test
from autotest_lib.client.bin import utils
from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib.cros.network import interface
from autotest_lib.client.cros.networking import shill_proxy


class network_WlanDriver(test.test):
    """
    Ensure wireless devices have the expected associated kernel driver.
    """
    version = 1
    EXPECTED_DRIVER = {
            'Atheros AR9280': {
                    '3.4': 'wireless/ath/ath9k/ath9k.ko',
                    '3.8': 'wireless-3.4/ath/ath9k/ath9k.ko'
            },
            'Atheros AR9382': {
                    '3.4': 'wireless/ath/ath9k/ath9k.ko',
                    '3.8': 'wireless-3.4/ath/ath9k/ath9k.ko'
            },
            'Intel 7260': {
                    '3.8': 'wireless/iwl7000/iwlwifi/iwlwifi.ko',
                    '3.10': 'wireless-3.8/iwl7000/iwlwifi/iwlwifi.ko',
                    '3.14': 'wireless-3.8/iwl7000/iwlwifi/iwlwifi.ko',
                    '4.4': 'wireless/iwl7000/iwlwifi/iwlwifi.ko'
            },
            'Intel 7265': {
                    '3.8': 'wireless/iwl7000/iwlwifi/iwlwifi.ko',
                    '3.10': 'wireless-3.8/iwl7000/iwlwifi/iwlwifi.ko',
                    '3.14': 'wireless-3.8/iwl7000/iwlwifi/iwlwifi.ko',
                    '3.18': 'wireless/iwl7000/iwlwifi/iwlwifi.ko',
                    '4.4': 'wireless/iwl7000/iwlwifi/iwlwifi.ko',
                    '4.14': 'wireless/iwl7000/iwlwifi/iwlwifi.ko'
            },
            'Intel 9000': {
                    '4.14': 'wireless/iwl7000/iwlwifi/iwlwifi.ko'
            },
            'Intel 9260': {
                    '4.14': 'wireless/iwl7000/iwlwifi/iwlwifi.ko'
            },
            'Atheros AR9462': {
                    '3.4': 'wireless/ath/ath9k_btcoex/ath9k_btcoex.ko',
                    '3.8': 'wireless-3.4/ath/ath9k_btcoex/ath9k_btcoex.ko'
            },
            'Qualcomm Atheros QCA6174': {
                    '3.18': 'wireless/ar10k/ath/ath10k/ath10k_pci.ko',
                    '4.4': 'wireless/ar10k/ath/ath10k/ath10k_pci.ko',
            },
            'Qualcomm Atheros NFA344A/QCA6174': {
                    '3.18': 'wireless/ar10k/ath/ath10k/ath10k_pci.ko',
                    '4.4': 'wireless/ar10k/ath/ath10k/ath10k_pci.ko',
            },
            'Marvell 88W8797 SDIO': {
                    '3.4': 'wireless/mwifiex/mwifiex_sdio.ko',
                    '3.8': 'wireless-3.4/mwifiex/mwifiex_sdio.ko'
            },
            'Marvell 88W8887 SDIO': {
                     '3.14': 'wireless-3.8/mwifiex/mwifiex_sdio.ko'
            },
            'Marvell 88W8897 PCIE': {
                     '3.8': 'wireless/mwifiex/mwifiex_pcie.ko',
                     '3.10': 'wireless-3.8/mwifiex/mwifiex_pcie.ko'
            },
            'Marvell 88W8897 SDIO': {
                     '3.8': 'wireless/mwifiex/mwifiex_sdio.ko',
                     '3.10': 'wireless-3.8/mwifiex/mwifiex_sdio.ko',
                     '3.14': 'wireless-3.8/mwifiex/mwifiex_sdio.ko',
                     '3.18': 'wireless/mwifiex/mwifiex_sdio.ko'
            },
            'Broadcom BCM4354 SDIO': {
                     '3.8': 'wireless/brcm80211/brcmfmac/brcmfmac.ko',
                     '3.14': 'wireless-3.8/brcm80211/brcmfmac/brcmfmac.ko'
            },
            'Broadcom BCM4356 PCIE': {
                     '3.10': 'wireless-3.8/brcm80211/brcmfmac/brcmfmac.ko'
            },
            'Marvell 88W8997 PCIE': {
                     '4.4': 'wireless/marvell/mwifiex/mwifiex_pcie.ko',
            },
    }
    EXCEPTION_BOARDS = [
            # Exhibits very similar symptoms to http://crbug.com/693724,
            # b/65858242, b/36264732.
            'nyan_kitty',
    ]


    def NoDeviceFailure(self, forgive_flaky, message):
        """
        No WiFi device found. Forgiveable in some suites, for some boards.
        """
        board = utils.get_board()
        if forgive_flaky and board in self.EXCEPTION_BOARDS:
            return error.TestWarn('Exception (%s): %s' % (board, message))
        else:
            return error.TestFail(message)


    def run_once(self, forgive_flaky=False):
        """Test main loop"""
        # full_revision looks like "3.4.0".
        full_revision = utils.system_output('uname -r')
        # base_revision looks like "3.4".
        base_revision = '.'.join(full_revision.split('.')[:2])
        logging.info('Kernel base is %s', base_revision)

        proxy = shill_proxy.ShillProxy()

        uninit = proxy.get_proxy().get_dbus_property(proxy.manager,
                 shill_proxy.ShillProxy.MANAGER_PROPERTY_UNINITIALIZED_TECHNOLOGIES)
        logging.info("Uninitialized technologies: %s", uninit)
        # If Wifi support is not enabled for shill, it will be uninitialized.
        # Don't fail the test if Wifi was intentionally disabled.
        if "wifi" in uninit:
            raise error.TestNAError('Wireless support not enabled')

        wlan_ifs = [nic for nic in interface.get_interfaces()
                        if nic.is_wifi_device()]
        if wlan_ifs:
            net_if = wlan_ifs[0]
        else:
            raise self.NoDeviceFailure(forgive_flaky,
                                       'Found no recognized wireless device')

        # Some systems (e.g., moblab) might blacklist certain devices. We don't
        # rely on shill for most of this test, but it can be a helpful clue if
        # we see shill barfing.
        device_obj = proxy.find_object('Device',
                                       {'Type': proxy.TECHNOLOGY_WIFI})
        if device_obj is None:
            logging.warning("Shill couldn't find wireless device; "
                            "did someone blacklist it?")

        device_description = net_if.device_description
        if not device_description:
            raise error.TestFail('Device %s is not supported' % net_if.name)

        device_name, module_path = device_description
        logging.info('Device name %s, module path %s', device_name, module_path)
        if not device_name in self.EXPECTED_DRIVER:
            raise error.TestFail('Unexpected device name %s' %
                                 device_name)

        if not base_revision in self.EXPECTED_DRIVER[device_name]:
            raise error.TestNAError('Unexpected base kernel revision %s with device name %s' %
                                    (base_revision, device_name))

        expected_driver = self.EXPECTED_DRIVER[device_name][base_revision]
        if module_path != expected_driver:
            raise error.TestFail('Unexpected driver for %s/%s; got %s but expected %s' %
                                 (base_revision, device_name,
                                  module_path, expected_driver))
