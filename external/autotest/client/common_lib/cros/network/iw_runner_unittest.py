#!/usr/bin/python
#
# Copyright (c) 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import unittest

import common

from autotest_lib.client.common_lib.cros.network import iw_runner

class IwRunnerTest(unittest.TestCase):
    """Unit test for the IWRunner object."""


    class host_cmd(object):
        """Mock host command class."""

        def __init__(self, stdout, stderr, exit_status):
            self._stdout = stdout
            self._stderr = stderr
            self._exit_status = exit_status


        @property
        def stdout(self):
            """Returns stdout."""
            return self._stdout


        @property
        def stderr(self):
            """Returns stderr."""
            return self._stderr


        @property
        def exit_status(self):
            """Returns the exit status."""
            return self._exit_status


    class host(object):
        """Mock host class."""

        def __init__(self, host_cmd):
            self._host_cmd = IwRunnerTest.host_cmd(host_cmd, 1.0, 0)


        def run(self, cmd, ignore_status=False):
            """Returns the mocked output.

            @param cmd: a stub input ignore
            @param ignore_status: a stub input ignore

            """
            return self._host_cmd


    HT20 = str('BSS aa:aa:aa:aa:aa:aa (on wlan0)\n'
        '    freq: 2412\n'
        '    signal: -50.00 dBm\n'
        '    SSID: support_ht20\n'
        '    HT operation:\n'
        '         * secondary channel offset: no secondary\n')

    HT20_IW_BSS = iw_runner.IwBss('aa:aa:aa:aa:aa:aa', 2412,
                                  'support_ht20', iw_runner.SECURITY_OPEN,
                                  iw_runner.HT20, -50.00)

    HT20_2 = str('BSS 11:11:11:11:11:11 (on wlan0)\n'
        '     freq: 2462\n'
        '     signal: -42.00 dBm\n'
        '     SSID: support_ht20\n'
        '     WPA:          * Version: 1\n'
        '     HT operation:\n'
        '          * secondary channel offset: below\n')

    HT20_2_IW_BSS = iw_runner.IwBss('11:11:11:11:11:11', 2462,
                                    'support_ht20', iw_runner.SECURITY_WPA,
                                    iw_runner.HT40_BELOW, -42.00)

    HT40_ABOVE = str('BSS bb:bb:bb:bb:bb:bb (on wlan0)\n'
        '    freq: 5180\n'
        '    signal: -55.00 dBm\n'
        '    SSID: support_ht40_above\n'
        '    RSN:          * Version: 1\n'
        '    HT operation:\n'
        '         * secondary channel offset: above\n')

    HT40_ABOVE_IW_BSS = iw_runner.IwBss('bb:bb:bb:bb:bb:bb', 5180,
                                        'support_ht40_above',
                                        iw_runner.SECURITY_WPA2,
                                        iw_runner.HT40_ABOVE, -55.00)

    HT40_BELOW = str('BSS cc:cc:cc:cc:cc:cc (on wlan0)\n'
        '    freq: 2462\n'
        '    signal: -44.00 dBm\n'
        '    SSID: support_ht40_below\n'
        '    RSN:          * Version: 1\n'
        '    WPA:          * Version: 1\n'
        '    HT operation:\n'
        '        * secondary channel offset: below\n')

    HT40_BELOW_IW_BSS = iw_runner.IwBss('cc:cc:cc:cc:cc:cc', 2462,
                                        'support_ht40_below',
                                        iw_runner.SECURITY_MIXED,
                                        iw_runner.HT40_BELOW, -44.00)

    NO_HT = str('BSS dd:dd:dd:dd:dd:dd (on wlan0)\n'
        '    freq: 2412\n'
        '    signal: -45.00 dBm\n'
        '    SSID: no_ht_support\n')

    NO_HT_IW_BSS = iw_runner.IwBss('dd:dd:dd:dd:dd:dd', 2412,
                                   'no_ht_support', iw_runner.SECURITY_OPEN,
                                   None, -45.00)

    HIDDEN_SSID = str('BSS ee:ee:ee:ee:ee:ee (on wlan0)\n'
        '    freq: 2462\n'
        '    signal: -70.00 dBm\n'
        '    SSID: \n'
        '    HT operation:\n'
        '         * secondary channel offset: no secondary\n')

    SCAN_TIME_OUTPUT = str('real 4.5\n'
        'user 2.1\n'
        'system 3.1\n')

    HIDDEN_SSID_IW_BSS = iw_runner.IwBss('ee:ee:ee:ee:ee:ee', 2462,
                                         None, iw_runner.SECURITY_OPEN,
                                         iw_runner.HT20, -70.00)

    STATION_LINK_INFORMATION = str(
        'Connected to 12:34:56:ab:cd:ef (on wlan0)\n'
        '      SSID: PMKSACaching_4m9p5_ch1\n'
        '      freq: 5220\n'
        '      RX: 5370 bytes (37 packets)\n'
        '      TX: 3604 bytes (15 packets)\n'
        '      signal: -59 dBm\n'
        '      tx bitrate: 13.0 MBit/s MCS 1\n'
        '\n'
        '      bss flags:      short-slot-time\n'
        '      dtim period:    5\n'
        '      beacon int:     100\n')

    STATION_LINK_BSSID = '12:34:56:ab:cd:ef'

    STATION_LINK_IFACE = 'wlan0'

    STATION_LINK_FREQ = '5220'

    STATION_LINK_PARSED = {
        'SSID': 'PMKSACaching_4m9p5_ch1',
        'freq': '5220',
        'RX': '5370 bytes (37 packets)',
        'TX': '3604 bytes (15 packets)',
        'signal': '-59 dBm',
        'tx bitrate': '13.0 MBit/s MCS 1',
        'bss flags': 'short-slot-time',
        'dtim period': '5',
        'beacon int': '100'
        }

    STATION_DUMP_INFORMATION = str(
        'Station dd:ee:ff:33:44:55 (on mesh-5000mhz)\n'
        '        inactive time:  140 ms\n'
        '        rx bytes:       2883498\n'
        '        rx packets:     31981\n'
        '        tx bytes:       1369934\n'
        '        tx packets:     6615\n'
        '        tx retries:     4\n'
        '        tx failed:      0\n'
        '        signal:         -4 dBm\n'
        '        signal avg:     -11 dBm\n'
        '        Toffset:        81715566854 us\n'
        '        tx bitrate:     866.7 MBit/s VHT-MCS 9 80MHz '
        'short GI VHT-NSS 2\n'
        '        rx bitrate:     866.7 MBit/s VHT-MCS 9 80MHz '
        'short GI VHT-NSS 2\n'
        '        mesh llid:      0\n'
        '        mesh plid:      0\n'
        '        mesh plink:     ESTAB\n'
        '        mesh local PS mode:     ACTIVE\n'
        '        mesh peer PS mode:      ACTIVE\n'
        '        mesh non-peer PS mode:  ACTIVE\n'
        '        authorized:     yes\n'
        '        authenticated:  yes\n'
        '        preamble:       long\n'
        '        WMM/WME:        yes\n'
        '        MFP:            yes\n'
        '        TDLS peer:      no\n'
        '        connected time: 8726 seconds\n'
        'Station aa:bb:cc:00:11:22 (on mesh-5000mhz)\n'
        '        inactive time:  140 ms\n'
        '        rx bytes:       2845200\n'
        '        rx packets:     31938\n'
        '        tx bytes:       1309945\n'
        '        tx packets:     6672\n'
        '        tx retries:     0\n'
        '        tx failed:      0\n'
        '        signal:         -21 dBm\n'
        '        signal avg:     -21 dBm\n'
        '        tx bitrate:     866.7 MBit/s VHT-MCS 9 80MHz '
        'short GI VHT-NSS 2\n'
        '        rx bitrate:     650.0 MBit/s VHT-MCS 7 80MHz '
        'short GI VHT-NSS 2\n'
        '        mesh llid:      0\n'
        '        mesh plid:      0\n'
        '        mesh plink:     ESTAB\n'
        '        mesh local PS mode:     ACTIVE\n'
        '        mesh peer PS mode:      ACTIVE\n'
        '        mesh non-peer PS mode:  ACTIVE\n'
        '        authorized:     yes\n'
        '        authenticated:  yes\n'
        '        preamble:       long\n'
        '        WMM/WME:        yes\n'
        '        MFP:            yes\n'
        '        TDLS peer:      no\n'
        '        connected time: 8724 seconds\n'
        'Station ff:aa:bb:aa:44:55 (on mesh-5000mhz)\n'
        '        inactive time:  304 ms\n'
        '        rx bytes:       18816\n'
        '        rx packets:     75\n'
        '        tx bytes:       5386\n'
        '        tx packets:     21\n'
        '        signal:         -29 dBm\n'
        '        tx bitrate:     65.0 MBit/s VHT-MCS 0 80MHz short GI VHT-NSS 2\n'
        '        mesh llid:      0\n'
        '        mesh plid:      0\n'
        '        mesh plink:     ESTAB\n'
        '        mesh local PS mode:     ACTIVE\n'
        '        mesh peer PS mode:      ACTIVE\n'
        '        mesh non-peer PS mode:  ACTIVE\n'
        '        authorized:     yes\n'
        '        authenticated:  yes\n'
        '        preamble:       long\n'
        '        WMM/WME:        yes\n'
        '        MFP:            yes\n'
        '        TDLS peer:      no\n'
        '        connected time: 824 seconds\n')

    STATION_DUMP_INFORMATION_PARSED = [
        {'mac': 'aa:bb:cc:00:11:22', 'rssi_str': '-21 dBm', 'rssi_int': -21,
        'tx_bitrate': '866.7 MBit/s VHT-MCS 9 80MHz short GI VHT-NSS 2',
        'rx_bitrate': '650.0 MBit/s VHT-MCS 7 80MHz short GI VHT-NSS 2'},
        {'mac': 'dd:ee:ff:33:44:55', 'rssi_str': '-4 dBm', 'rssi_int': -4,
        'tx_bitrate': '866.7 MBit/s VHT-MCS 9 80MHz short GI VHT-NSS 2',
        'rx_bitrate': '866.7 MBit/s VHT-MCS 9 80MHz short GI VHT-NSS 2'},
        {'mac': 'ff:aa:bb:aa:44:55', 'rssi_str': '-29 dBm', 'rssi_int': -29,
        'tx_bitrate': '65.0 MBit/s VHT-MCS 0 80MHz short GI VHT-NSS 2',
        'rx_bitrate': None},
        ]

    STATION_DUMP_IFACE = 'mesh-5000mhz'

    INFO_MESH_MODE = str(
        'Interface wlan-2400mhz\n'
        '        ifindex 10\n'
        '        wdev 0x100000002\n'
        '        addr aa:bb:cc:dd:ee:ff\n'
        '        type mesh point\n'
        '        wiphy 1\n'
        '        channel 149 (5745 MHz), width: 80 MHz, center1: 5775 MHz\n')

    INFO_AP_MODE = str(
        'Interface wlan-2400mhz\n'
        '        ifindex 8\n'
        '        wdev 0x1\n'
        '        addr 00:11:22:33:44:55\n'
        '        ssid testap_170501151530_wsvx\n'
        '        type AP\n'
        '        wiphy 0\n'
        '        channel 11 (2462 MHz), width: 20 MHz, center1: 2462 MHz\n')

    RADIO_CONFIG_AP_MODE = {'number': 11, 'freq': 2462, 'width': 20,
                            'center1_freq': 2462}

    INFO_STA_MODE = str(
        'Interface wlan-2400mhz\n'
        '        ifindex 9\n'
        '        wdev 0x1\n'
        '        addr 44:55:66:77:88:99\n'
        '        type managed\n'
        '        wiphy 0\n'
        '        channel 11 (2462 MHz), width: 20 MHz, center1: 2462 MHz\n')

    INFO_IFACE = 'wlan-2400mhz'

    PHY_INFO_FRAGMENTATION = str(
        'Wiphy phy1\n'
        '        max # scan SSIDs: 20\n'
        '        max scan IEs length: 425 bytes\n'
        '        Fragmentation threshold: 256\n'
        '        Retry short limit: 7\n'
        '        Retry long limit: 4\n')

    INFO_PHY = 'phy1'

    PHY_FRAGMENTATION_THRESHOLD = 256


    def verify_values(self, iw_bss_1, iw_bss_2):
        """Checks all of the IWBss values

        @param iw_bss_1: an IWBss object
        @param iw_bss_2: an IWBss object

        """
        self.assertEquals(iw_bss_1.bss, iw_bss_2.bss)
        self.assertEquals(iw_bss_1.ssid, iw_bss_2.ssid)
        self.assertEquals(iw_bss_1.frequency, iw_bss_2.frequency)
        self.assertEquals(iw_bss_1.security, iw_bss_2.security)
        self.assertEquals(iw_bss_1.ht, iw_bss_2.ht)
        self.assertEquals(iw_bss_1.signal, iw_bss_2.signal)


    def search_by_bss(self, scan_output, test_iw_bss):
        """

        @param scan_output: the output of the scan as a string
        @param test_iw_bss: an IWBss object

        Uses the runner to search for a network by bss.
        """
        host = self.host(scan_output + self.SCAN_TIME_OUTPUT)
        runner = iw_runner.IwRunner(remote_host=host)
        network = runner.wait_for_scan_result('wlan0', bsses=[test_iw_bss.bss])
        self.verify_values(test_iw_bss, network[0])


    def test_find_first(self):
        """Test with the first item in the list."""
        scan_output = self.HT20 + self.HT40_ABOVE
        self.search_by_bss(scan_output, self.HT20_IW_BSS)


    def test_find_last(self):
        """Test with the last item in the list."""
        scan_output = self.HT40_ABOVE + self.HT20
        self.search_by_bss(scan_output, self.HT20_IW_BSS)


    def test_find_middle(self):
        """Test with the middle item in the list."""
        scan_output = self.HT40_ABOVE + self.HT20 + self.NO_HT
        self.search_by_bss(scan_output, self.HT20_IW_BSS)


    def test_ht40_above(self):
        """Test with a HT40+ network."""
        scan_output = self.HT20 + self.HT40_ABOVE + self.NO_HT
        self.search_by_bss(scan_output, self.HT40_ABOVE_IW_BSS)


    def test_ht40_below(self):
        """Test with a HT40- network."""
        scan_output = self.HT20 + self.HT40_BELOW + self.NO_HT
        self.search_by_bss(scan_output, self.HT40_BELOW_IW_BSS)


    def test_no_ht(self):
        """Test with a network that doesn't have ht."""
        scan_output = self.HT20 + self.NO_HT + self.HT40_ABOVE
        self.search_by_bss(scan_output, self.NO_HT_IW_BSS)


    def test_hidden_ssid(self):
        """Test with a network with a hidden ssid."""
        scan_output = self.HT20 + self.HIDDEN_SSID + self.NO_HT
        self.search_by_bss(scan_output, self.HIDDEN_SSID_IW_BSS)


    def test_multiple_ssids(self):
        """Test with multiple networks with the same ssids."""
        scan_output = self.HT40_ABOVE + self.HT20 + self.NO_HT + self.HT20_2
        host = self.host(scan_output + self.SCAN_TIME_OUTPUT)
        runner = iw_runner.IwRunner(remote_host=host)
        networks = runner.wait_for_scan_result('wlan 0',
                                               ssids=[self.HT20_2_IW_BSS.ssid])
        for iw_bss_1, iw_bss_2 in zip([self.HT20_IW_BSS, self.HT20_2_IW_BSS],
                                      networks):
            self.verify_values(iw_bss_1, iw_bss_2)


    def test_station_bssid(self):
        """Test parsing of the bssid of a station-mode link."""
        host = self.host(self.STATION_LINK_INFORMATION)
        runner = iw_runner.IwRunner(remote_host=host)
        self.assertEquals(
            runner.get_current_bssid(self.STATION_LINK_IFACE),
            self.STATION_LINK_BSSID)


    def test_station_link_parsing(self):
        """Test all link keys can be parsed from station link information."""
        self.assertEquals(
            iw_runner._get_all_link_keys(self.STATION_LINK_INFORMATION),
            self.STATION_LINK_PARSED)


    def test_station_link_key(self):
        """Test a link key is extracted from station link information."""
        host = self.host(self.STATION_LINK_INFORMATION)
        runner = iw_runner.IwRunner(remote_host=host)
        self.assertEquals(
            runner.get_link_value(self.STATION_LINK_INFORMATION,
                                  iw_runner.IW_LINK_KEY_FREQUENCY),
            self.STATION_LINK_FREQ)


    def test_station_dump(self):
        """Test parsing of a station dump."""
        host = self.host(self.STATION_DUMP_INFORMATION)
        runner = iw_runner.IwRunner(remote_host=host)
        self.assertEquals(
            runner.get_station_dump(self.STATION_DUMP_IFACE),
            self.STATION_DUMP_INFORMATION_PARSED)


    def test_operating_mode_mesh(self):
        """Test mesh operating mode parsing."""
        host = self.host(self.INFO_MESH_MODE)
        runner = iw_runner.IwRunner(remote_host=host)
        self.assertEquals(
            runner.get_operating_mode(self.INFO_IFACE),
            iw_runner.DEV_MODE_MESH_POINT)


    def test_operating_mode_ap(self):
        """Test AP operating mode parsing."""
        host = self.host(self.INFO_AP_MODE)
        runner = iw_runner.IwRunner(remote_host=host)
        self.assertEquals(
            runner.get_operating_mode(self.INFO_IFACE),
            iw_runner.DEV_MODE_AP)


    def test_operating_mode_station(self):
        """Test STA operating mode parsing."""
        host = self.host(self.INFO_STA_MODE)
        runner = iw_runner.IwRunner(remote_host=host)
        self.assertEquals(
            runner.get_operating_mode(self.INFO_IFACE),
            iw_runner.DEV_MODE_STATION)


    def test_radio_information(self):
        """Test radio information parsing."""
        host = self.host(self.INFO_AP_MODE)
        runner = iw_runner.IwRunner(remote_host=host)
        self.assertEquals(
            runner.get_radio_config(self.INFO_IFACE),
            self.RADIO_CONFIG_AP_MODE)


    def test_fragmentation_threshold(self):
        """Test fragmentation threshold parsing."""
        host = self.host(self.PHY_INFO_FRAGMENTATION)
        runner = iw_runner.IwRunner(remote_host=host)
        self.assertEquals(
            runner.get_fragmentation_threshold(self.INFO_PHY),
            self.PHY_FRAGMENTATION_THRESHOLD)


if __name__ == '__main__':
    unittest.main()
