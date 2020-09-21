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

from acts.controllers.ap_lib import hostapd_config
from acts.controllers.ap_lib import hostapd_constants


def create_ap_preset(profile_name,
                     channel=None,
                     dtim=2,
                     frequency=None,
                     security=None,
                     ssid=None,
                     vht_bandwidth=80,
                     bss_settings=[],
                     iface_wlan_2g=hostapd_constants.WLAN0_STRING,
                     iface_wlan_5g=hostapd_constants.WLAN1_STRING):
    """AP preset config generator.  This a wrapper for hostapd_config but
       but supplies the default settings for the preset that is selected.

        You may specify channel or frequency, but not both.  Both options
        are checked for validity (i.e. you can't specify an invalid channel
        or a frequency that will not be accepted).

    Args:
        profile_name: The name of the device want the preset for.
                      Options: whirlwind
        channel: int, channel number.
        dtim: int, DTIM value of the AP, default is 2.
        frequency: int, frequency of channel.
        security: Security, the secuirty settings to use.
        ssid: string, The name of the ssid to brodcast.
        vht_bandwidth: VHT bandwidth for 11ac operation.
        bss_settings: The settings for all bss.
        iface_wlan_2g: the wlan 2g interface name of the AP.
        iface_wlan_5g: the wlan 5g interface name of the AP.

    Returns: A hostapd_config object that can be used by the hostapd object.
    """

    force_wmm = None
    force_wmm = None
    beacon_interval = None
    dtim_period = None
    short_preamble = None
    interface = None
    mode = None
    n_capabilities = None
    ac_capabilities = None
    if channel:
        frequency = hostapd_config.get_frequency_for_channel(channel)
    elif frequency:
        channel = hostapd_config.get_channel_for_frequency(frequency)
    else:
        raise ValueError('Specify either frequency or channel.')

    if (profile_name == 'whirlwind'):
        force_wmm = True
        beacon_interval = 100
        short_preamble = True
        if frequency < 5000:
            interface = iface_wlan_2g
            mode = hostapd_constants.MODE_11N_MIXED
            n_capabilities = [
                hostapd_constants.N_CAPABILITY_LDPC,
                hostapd_constants.N_CAPABILITY_SGI20,
                hostapd_constants.N_CAPABILITY_SGI40,
                hostapd_constants.N_CAPABILITY_TX_STBC,
                hostapd_constants.N_CAPABILITY_RX_STBC1,
                hostapd_constants.N_CAPABILITY_DSSS_CCK_40
            ]
            config = hostapd_config.HostapdConfig(
                ssid=ssid,
                security=security,
                interface=interface,
                mode=mode,
                force_wmm=force_wmm,
                beacon_interval=beacon_interval,
                dtim_period=dtim_period,
                short_preamble=short_preamble,
                frequency=frequency,
                n_capabilities=n_capabilities,
                bss_settings=bss_settings)
        else:
            interface = iface_wlan_5g
            mode = hostapd_constants.MODE_11AC_MIXED
            if hostapd_config.ht40_plus_allowed(channel):
                extended_channel = hostapd_constants.N_CAPABILITY_HT40_PLUS
            elif hostapd_config.ht40_minus_allowed(channel):
                extended_channel = hostapd_constants.N_CAPABILITY_HT40_MINUS
            # Define the n capability vector for 20 MHz and higher bandwidth
            if vht_bandwidth >= 40:
                n_capabilities = [
                    hostapd_constants.N_CAPABILITY_LDPC, extended_channel,
                    hostapd_constants.N_CAPABILITY_SGI20,
                    hostapd_constants.N_CAPABILITY_SGI40,
                    hostapd_constants.N_CAPABILITY_TX_STBC,
                    hostapd_constants.N_CAPABILITY_RX_STBC1
                ]
            else:
                n_capabilities = [
                    hostapd_constants.N_CAPABILITY_LDPC,
                    hostapd_constants.N_CAPABILITY_SGI20,
                    hostapd_constants.N_CAPABILITY_SGI40,
                    hostapd_constants.N_CAPABILITY_TX_STBC,
                    hostapd_constants.N_CAPABILITY_RX_STBC1,
                    hostapd_constants.N_CAPABILITY_HT20
                ]
            ac_capabilities = [
                hostapd_constants.AC_CAPABILITY_MAX_MPDU_11454,
                hostapd_constants.AC_CAPABILITY_RXLDPC,
                hostapd_constants.AC_CAPABILITY_SHORT_GI_80,
                hostapd_constants.AC_CAPABILITY_TX_STBC_2BY1,
                hostapd_constants.AC_CAPABILITY_RX_STBC_1,
                hostapd_constants.AC_CAPABILITY_MAX_A_MPDU_LEN_EXP7,
                hostapd_constants.AC_CAPABILITY_RX_ANTENNA_PATTERN,
                hostapd_constants.AC_CAPABILITY_TX_ANTENNA_PATTERN
            ]
            config = hostapd_config.HostapdConfig(
                ssid=ssid,
                security=security,
                interface=interface,
                mode=mode,
                force_wmm=force_wmm,
                vht_channel_width=vht_bandwidth,
                beacon_interval=beacon_interval,
                dtim_period=dtim_period,
                short_preamble=short_preamble,
                frequency=frequency,
                n_capabilities=n_capabilities,
                ac_capabilities=ac_capabilities,
                bss_settings=bss_settings)
    else:
        raise ValueError('Invalid ap model specified (%s)' % profile_name)
    return config
