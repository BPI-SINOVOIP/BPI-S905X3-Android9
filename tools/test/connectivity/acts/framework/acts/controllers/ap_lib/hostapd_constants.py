#!/usr/bin/env python3.4
#
#   Copyright 2017 - Google
#
#   Licensed under the Apache License, Version 2.0 (the "License");
#   you may not use this file except in compliance with the License.
#   You may obtain a copy of the License at
#
#         http://www.apache.org/licenses/LICENSE-2.0
#
#   Unless required by applicable law or agreed to in writing, software
#   distributed under the License is distributed on an "AS IS" BASIS,
#   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#   See the License for the specific language governing permissions and
#   limitations under the License.

import itertools

BAND_2G = '2g'
BAND_5G = '5g'
WPA1 = 1
WPA2 = 2
MIXED = 3
MAX_WPA_PSK_LENGTH = 64
MIN_WPA_PSK_LENGTH = 8
WPA_STRICT_REKEY = 1
WPA_DEFAULT_CIPHER = 'TKIP'
WPA2_DEFAULT_CIPER = 'CCMP'
WPA_GROUP_KEY_ROTATION_TIME = 600
WPA_STRICT_REKEY_DEFAULT = True
WPA_STRING = 'wpa'
WPA2_STRING = 'wpa2'
WPA_MIXED_STRING = 'wpa/wpa2'
WLAN0_STRING = 'wlan0'
WLAN1_STRING = 'wlan1'
WLAN2_STRING = 'wlan2'
WLAN3_STRING = 'wlan3'
AP_DEFAULT_CHANNEL_2G = 6
AP_DEFAULT_CHANNEL_5G = 36
AP_DEFAULT_MAX_SSIDS_2G = 8
AP_DEFAULT_MAX_SSIDS_5G = 8
AP_SSID_LENGTH_2G = 8
AP_PASSPHRASE_LENGTH_2G = 10
AP_SSID_LENGTH_5G = 8
AP_PASSPHRASE_LENGTH_5G = 10

# A mapping of frequency to channel number.  This includes some
# frequencies used outside the US.
CHANNEL_MAP = {
    2412: 1,
    2417: 2,
    2422: 3,
    2427: 4,
    2432: 5,
    2437: 6,
    2442: 7,
    2447: 8,
    2452: 9,
    2457: 10,
    2462: 11,
    # 12, 13 are only legitimate outside the US.
    # 2467: 12,
    # 2472: 13,
    # 14 is for Japan, DSSS and CCK only.
    # 2484: 14,
    # 34 valid in Japan.
    # 5170: 34,
    # 36-116 valid in the US, except 38, 42, and 46, which have
    # mixed international support.
    5180: 36,
    5190: 38,
    5200: 40,
    5210: 42,
    5220: 44,
    5230: 46,
    5240: 48,
    # DFS channels.
    5260: 52,
    5280: 56,
    5300: 60,
    5320: 64,
    5500: 100,
    5520: 104,
    5540: 108,
    5560: 112,
    5580: 116,
    # 120, 124, 128 valid in Europe/Japan.
    5600: 120,
    5620: 124,
    5640: 128,
    # 132+ valid in US.
    5660: 132,
    5680: 136,
    5700: 140,
    # 144 is supported by a subset of WiFi chips
    # (e.g. bcm4354, but not ath9k).
    5720: 144,
    # End DFS channels.
    5745: 149,
    5755: 151,
    5765: 153,
    5775: 155,
    5795: 159,
    5785: 157,
    5805: 161,
    5825: 165
}

MODE_11A = 'a'
MODE_11B = 'b'
MODE_11G = 'g'
MODE_11N_MIXED = 'n-mixed'
MODE_11N_PURE = 'n-only'
MODE_11AC_MIXED = 'ac-mixed'
MODE_11AC_PURE = 'ac-only'

N_CAPABILITY_LDPC = object()
N_CAPABILITY_HT20 = object()
N_CAPABILITY_HT40_PLUS = object()
N_CAPABILITY_HT40_MINUS = object()
N_CAPABILITY_GREENFIELD = object()
N_CAPABILITY_SGI20 = object()
N_CAPABILITY_SGI40 = object()
N_CAPABILITY_TX_STBC = object()
N_CAPABILITY_RX_STBC1 = object()
N_CAPABILITY_RX_STBC12 = object()
N_CAPABILITY_RX_STBC123 = object()
N_CAPABILITY_DSSS_CCK_40 = object()
N_CAPABILITIES_MAPPING = {
    N_CAPABILITY_LDPC: '[LDPC]',
    N_CAPABILITY_HT20: '[HT20]',
    N_CAPABILITY_HT40_PLUS: '[HT40+]',
    N_CAPABILITY_HT40_MINUS: '[HT40-]',
    N_CAPABILITY_GREENFIELD: '[GF]',
    N_CAPABILITY_SGI20: '[SHORT-GI-20]',
    N_CAPABILITY_SGI40: '[SHORT-GI-40]',
    N_CAPABILITY_TX_STBC: '[TX-STBC]',
    N_CAPABILITY_RX_STBC1: '[RX-STBC1]',
    N_CAPABILITY_RX_STBC12: '[RX-STBC12]',
    N_CAPABILITY_RX_STBC123: '[RX-STBC123]',
    N_CAPABILITY_DSSS_CCK_40: '[DSSS_CCK-40]'
}
N_CAPABILITY_HT40_MINUS_CHANNELS = object()
N_CAPABILITY_HT40_PLUS_CHANNELS = object()
AC_CAPABILITY_VHT160 = object()
AC_CAPABILITY_VHT160_80PLUS80 = object()
AC_CAPABILITY_RXLDPC = object()
AC_CAPABILITY_SHORT_GI_80 = object()
AC_CAPABILITY_SHORT_GI_160 = object()
AC_CAPABILITY_TX_STBC_2BY1 = object()
AC_CAPABILITY_RX_STBC_1 = object()
AC_CAPABILITY_RX_STBC_12 = object()
AC_CAPABILITY_RX_STBC_123 = object()
AC_CAPABILITY_RX_STBC_1234 = object()
AC_CAPABILITY_SU_BEAMFORMER = object()
AC_CAPABILITY_SU_BEAMFORMEE = object()
AC_CAPABILITY_BF_ANTENNA_2 = object()
AC_CAPABILITY_SOUNDING_DIMENSION_2 = object()
AC_CAPABILITY_MU_BEAMFORMER = object()
AC_CAPABILITY_MU_BEAMFORMEE = object()
AC_CAPABILITY_VHT_TXOP_PS = object()
AC_CAPABILITY_HTC_VHT = object()
AC_CAPABILITY_MAX_A_MPDU_LEN_EXP0 = object()
AC_CAPABILITY_MAX_A_MPDU_LEN_EXP1 = object()
AC_CAPABILITY_MAX_A_MPDU_LEN_EXP2 = object()
AC_CAPABILITY_MAX_A_MPDU_LEN_EXP3 = object()
AC_CAPABILITY_MAX_A_MPDU_LEN_EXP4 = object()
AC_CAPABILITY_MAX_A_MPDU_LEN_EXP5 = object()
AC_CAPABILITY_MAX_A_MPDU_LEN_EXP6 = object()
AC_CAPABILITY_MAX_A_MPDU_LEN_EXP7 = object()
AC_CAPABILITY_VHT_LINK_ADAPT2 = object()
AC_CAPABILITY_VHT_LINK_ADAPT3 = object()
AC_CAPABILITY_RX_ANTENNA_PATTERN = object()
AC_CAPABILITY_TX_ANTENNA_PATTERN = object()
AC_CAPABILITY_MAX_MPDU_7991 = object()
AC_CAPABILITY_MAX_MPDU_11454 = object()
AC_CAPABILITIES_MAPPING = {
    AC_CAPABILITY_VHT160: '[VHT160]',
    AC_CAPABILITY_VHT160_80PLUS80: '[VHT160-80PLUS80]',
    AC_CAPABILITY_RXLDPC: '[RXLDPC]',
    AC_CAPABILITY_SHORT_GI_80: '[SHORT-GI-80]',
    AC_CAPABILITY_SHORT_GI_160: '[SHORT-GI-160]',
    AC_CAPABILITY_TX_STBC_2BY1: '[TX-STBC-2BY1]',
    AC_CAPABILITY_RX_STBC_1: '[RX-STBC-1]',
    AC_CAPABILITY_RX_STBC_12: '[RX-STBC-12]',
    AC_CAPABILITY_RX_STBC_123: '[RX-STBC-123]',
    AC_CAPABILITY_RX_STBC_1234: '[RX-STBC-1234]',
    AC_CAPABILITY_SU_BEAMFORMER: '[SU-BEAMFORMER]',
    AC_CAPABILITY_SU_BEAMFORMEE: '[SU-BEAMFORMEE]',
    AC_CAPABILITY_BF_ANTENNA_2: '[BF-ANTENNA-2]',
    AC_CAPABILITY_SOUNDING_DIMENSION_2: '[SOUNDING-DIMENSION-2]',
    AC_CAPABILITY_MU_BEAMFORMER: '[MU-BEAMFORMER]',
    AC_CAPABILITY_MU_BEAMFORMEE: '[MU-BEAMFORMEE]',
    AC_CAPABILITY_VHT_TXOP_PS: '[VHT-TXOP-PS]',
    AC_CAPABILITY_HTC_VHT: '[HTC-VHT]',
    AC_CAPABILITY_MAX_A_MPDU_LEN_EXP0: '[MAX-A-MPDU-LEN-EXP0]',
    AC_CAPABILITY_MAX_A_MPDU_LEN_EXP1: '[MAX-A-MPDU-LEN-EXP1]',
    AC_CAPABILITY_MAX_A_MPDU_LEN_EXP2: '[MAX-A-MPDU-LEN-EXP2]',
    AC_CAPABILITY_MAX_A_MPDU_LEN_EXP3: '[MAX-A-MPDU-LEN-EXP3]',
    AC_CAPABILITY_MAX_A_MPDU_LEN_EXP4: '[MAX-A-MPDU-LEN-EXP4]',
    AC_CAPABILITY_MAX_A_MPDU_LEN_EXP5: '[MAX-A-MPDU-LEN-EXP5]',
    AC_CAPABILITY_MAX_A_MPDU_LEN_EXP6: '[MAX-A-MPDU-LEN-EXP6]',
    AC_CAPABILITY_MAX_A_MPDU_LEN_EXP7: '[MAX-A-MPDU-LEN-EXP7]',
    AC_CAPABILITY_VHT_LINK_ADAPT2: '[VHT-LINK-ADAPT2]',
    AC_CAPABILITY_VHT_LINK_ADAPT3: '[VHT-LINK-ADAPT3]',
    AC_CAPABILITY_RX_ANTENNA_PATTERN: '[RX-ANTENNA-PATTERN]',
    AC_CAPABILITY_TX_ANTENNA_PATTERN: '[TX-ANTENNA-PATTERN]',
    AC_CAPABILITY_MAX_MPDU_11454: '[MAX-MPDU-11454]',
    AC_CAPABILITY_MAX_MPDU_7991: '[MAX-MPDU-7991]'
}
VHT_CHANNEL_WIDTH_40 = 0
VHT_CHANNEL_WIDTH_80 = 1
VHT_CHANNEL_WIDTH_160 = 2
VHT_CHANNEL_WIDTH_80_80 = 3

# This is a loose merging of the rules for US and EU regulatory
# domains as taken from IEEE Std 802.11-2012 Appendix E.  For instance,
# we tolerate HT40 in channels 149-161 (not allowed in EU), but also
# tolerate HT40+ on channel 7 (not allowed in the US).  We take the loose
# definition so that we don't prohibit testing in either domain.
HT40_ALLOW_MAP = {
    N_CAPABILITY_HT40_MINUS_CHANNELS:
    tuple(
        itertools.chain(
            range(6, 14), range(40, 65, 8), range(104, 137, 8), [153, 161])),
    N_CAPABILITY_HT40_PLUS_CHANNELS:
    tuple(
        itertools.chain(
            range(1, 8), range(36, 61, 8), range(100, 133, 8), [149, 157]))
}

PMF_SUPPORT_DISABLED = 0
PMF_SUPPORT_ENABLED = 1
PMF_SUPPORT_REQUIRED = 2
PMF_SUPPORT_VALUES = (PMF_SUPPORT_DISABLED, PMF_SUPPORT_ENABLED,
                      PMF_SUPPORT_REQUIRED)

DRIVER_NAME = 'nl80211'

CENTER_CHANNEL_MAP = {
    VHT_CHANNEL_WIDTH_40: {
        'delta': 2,
        'channels': ((36, 40), (44, 48), (52, 56), (60, 64), (100, 104),
                     (108, 112), (116, 120), (124, 128), (132, 136),
                     (140, 144), (149, 153), (147, 161))
    },
    VHT_CHANNEL_WIDTH_80: {
        'delta': 6,
        'channels': ((36, 48), (52, 64), (100, 112), (116, 128), (132, 144),
                     (149, 161))
    },
    VHT_CHANNEL_WIDTH_160: {
        'delta': 14,
        'channels': ((36, 64), (100, 128))
    }
}
