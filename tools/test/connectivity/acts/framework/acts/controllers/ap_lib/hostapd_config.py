#   Copyright 2016 - The Android Open Source Project
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

import enum
import logging
import os
import collections
import itertools

from acts.controllers.ap_lib import hostapd_constants


def ht40_plus_allowed(channel):
    """Returns: True iff HT40+ is enabled for this configuration."""
    channel_supported = (channel in hostapd_constants.HT40_ALLOW_MAP[
        hostapd_constants.N_CAPABILITY_HT40_PLUS_CHANNELS])
    return (channel_supported)


def ht40_minus_allowed(channel):
    """Returns: True iff HT40- is enabled for this configuration."""
    channel_supported = (channel in hostapd_constants.HT40_ALLOW_MAP[
        hostapd_constants.N_CAPABILITY_HT40_MINUS_CHANNELS])
    return (channel_supported)


def get_frequency_for_channel(channel):
    """The frequency associated with a given channel number.

    Args:
        value: int channel number.

    Returns:
        int, frequency in MHz associated with the channel.

    """
    for frequency, channel_iter in \
        hostapd_constants.CHANNEL_MAP.items():
        if channel == channel_iter:
            return frequency
    else:
        raise ValueError('Unknown channel value: %r.' % channel)


def get_channel_for_frequency(frequency):
    """The channel number associated with a given frequency.

    Args:
        value: int frequency in MHz.

    Returns:
        int, frequency associated with the channel.

    """
    return hostapd_constants.CHANNEL_MAP[frequency]


class HostapdConfig(object):
    """The root settings for the router.

    All the settings for a router that are not part of an ssid.
    """

    def _get_11ac_center_channel_from_channel(self, channel):
        """Returns the center channel of the selected channel band based
           on the channel and channel bandwidth provided.
        """
        channel = int(channel)
        center_channel_delta = hostapd_constants.CENTER_CHANNEL_MAP[
            self._vht_oper_chwidth]['delta']

        for channel_map in hostapd_constants.CENTER_CHANNEL_MAP[
                self._vht_oper_chwidth]['channels']:
            lower_channel_bound, upper_channel_bound = channel_map
            if lower_channel_bound <= channel <= upper_channel_bound:
                return lower_channel_bound + center_channel_delta
        raise ValueError('Invalid channel for {channel_width}.'.format(
            channel_width=self._vht_oper_chwidth))

    @property
    def _get_default_config(self):
        """Returns: dict of default options for hostapd."""
        return collections.OrderedDict([
            ('logger_syslog', '-1'),
            ('logger_syslog_level', '0'),
            # default RTS and frag threshold to ``off''
            ('rts_threshold', '2347'),
            ('fragm_threshold', '2346'),
            ('driver', hostapd_constants.DRIVER_NAME)
        ])

    @property
    def _hostapd_ht_capabilities(self):
        """Returns: string suitable for the ht_capab= line in a hostapd config.
        """
        ret = []
        for cap in hostapd_constants.N_CAPABILITIES_MAPPING.keys():
            if cap in self._n_capabilities:
                ret.append(hostapd_constants.N_CAPABILITIES_MAPPING[cap])
        return ''.join(ret)

    @property
    def _hostapd_vht_capabilities(self):
        """Returns: string suitable for the vht_capab= line in a hostapd config.
        """
        ret = []
        for cap in hostapd_constants.AC_CAPABILITIES_MAPPING.keys():
            if cap in self._ac_capabilities:
                ret.append(hostapd_constants.AC_CAPABILITIES_MAPPING[cap])
        return ''.join(ret)

    @property
    def _require_ht(self):
        """Returns: True iff clients should be required to support HT."""
        # TODO(wiley) Why? (crbug.com/237370)
        # DOES THIS APPLY TO US?
        logging.warning('Not enforcing pure N mode because Snow does '
                        'not seem to support it...')
        return False

    @property
    def _require_vht(self):
        """Returns: True if clients should be required to support VHT."""
        return self._mode == hostapd_constants.MODE_11AC_PURE

    @property
    def hw_mode(self):
        """Returns: string hardware mode understood by hostapd."""
        if self._mode == hostapd_constants.MODE_11A:
            return hostapd_constants.MODE_11A
        if self._mode == hostapd_constants.MODE_11B:
            return hostapd_constants.MODE_11B
        if self._mode == hostapd_constants.MODE_11G:
            return hostapd_constants.MODE_11G
        if self.is_11n or self.is_11ac:
            # For their own historical reasons, hostapd wants it this way.
            if self._frequency > 5000:
                return hostapd_constants.MODE_11A
            return hostapd_constants.MODE_11G
        raise ValueError('Invalid mode.')

    @property
    def is_11n(self):
        """Returns: True if we're trying to host an 802.11n network."""
        return self._mode in (hostapd_constants.MODE_11N_MIXED,
                              hostapd_constants.MODE_11N_PURE)

    @property
    def is_11ac(self):
        """Returns: True if we're trying to host an 802.11ac network."""
        return self._mode in (hostapd_constants.MODE_11AC_MIXED,
                              hostapd_constants.MODE_11AC_PURE)

    @property
    def channel(self):
        """Returns: int channel number for self.frequency."""
        return get_channel_for_frequency(self.frequency)

    @channel.setter
    def channel(self, value):
        """Sets the channel number to configure hostapd to listen on.

        Args:
            value: int, channel number.

        """
        self.frequency = get_frequency_for_channel(value)

    @property
    def bssid(self):
        return self._bssid

    @bssid.setter
    def bssid(self, value):
        self._bssid = value

    @property
    def frequency(self):
        """Returns: int, frequency for hostapd to listen on."""
        return self._frequency

    @frequency.setter
    def frequency(self, value):
        """Sets the frequency for hostapd to listen on.

        Args:
            value: int, frequency in MHz.

        """
        if value not in hostapd_constants.CHANNEL_MAP:
            raise ValueError('Tried to set an invalid frequency: %r.' % value)

        self._frequency = value

    @property
    def bss_lookup(self):
        return self._bss_lookup

    @property
    def ssid(self):
        """Returns: SsidSettings, The root Ssid settings being used."""
        return self._ssid

    @ssid.setter
    def ssid(self, value):
        """Sets the ssid for the hostapd.

        Args:
            value: SsidSettings, new ssid settings to use.

        """
        self._ssid = value

    @property
    def hidden(self):
        """Returns: bool, True if the ssid is hidden, false otherwise."""
        return self._hidden

    @hidden.setter
    def hidden(self, value):
        """Sets if this ssid is hidden.

        Args:
            value: bool, If true the ssid will be hidden.
        """
        self.hidden = value

    @property
    def security(self):
        """Returns: The security type being used."""
        return self._security

    @security.setter
    def security(self, value):
        """Sets the security options to use.

        Args:
            value: Security, The type of security to use.
        """
        self._security = value

    @property
    def ht_packet_capture_mode(self):
        """Get an appropriate packet capture HT parameter.

        When we go to configure a raw monitor we need to configure
        the phy to listen on the correct channel.  Part of doing
        so is to specify the channel width for HT channels.  In the
        case that the AP is configured to be either HT40+ or HT40-,
        we could return the wrong parameter because we don't know which
        configuration will be chosen by hostap.

        Returns:
            string, HT parameter for frequency configuration.

        """
        if not self.is_11n:
            return None

        if ht40_plus_allowed(self.channel):
            return 'HT40+'

        if ht40_minus_allowed(self.channel):
            return 'HT40-'

        return 'HT20'

    @property
    def beacon_footer(self):
        """Returns: bool _beacon_footer value."""
        return self._beacon_footer

    def beacon_footer(self, value):
        """Changes the beacon footer.

        Args:
            value: bool, The beacon footer vlaue.
        """
        self._beacon_footer = value

    @property
    def scenario_name(self):
        """Returns: string _scenario_name value, or None."""
        return self._scenario_name

    @property
    def min_streams(self):
        """Returns: int, _min_streams value, or None."""
        return self._min_streams

    def __init__(self,
                 interface=None,
                 mode=None,
                 channel=None,
                 frequency=None,
                 n_capabilities=[],
                 beacon_interval=None,
                 dtim_period=None,
                 frag_threshold=None,
                 short_preamble=None,
                 ssid=None,
                 hidden=False,
                 security=None,
                 bssid=None,
                 force_wmm=None,
                 pmf_support=hostapd_constants.PMF_SUPPORT_DISABLED,
                 obss_interval=None,
                 vht_channel_width=None,
                 vht_center_channel=None,
                 ac_capabilities=[],
                 beacon_footer='',
                 spectrum_mgmt_required=None,
                 scenario_name=None,
                 min_streams=None,
                 bss_settings=[],
                 set_ap_defaults_model=None):
        """Construct a HostapdConfig.

        You may specify channel or frequency, but not both.  Both options
        are checked for validity (i.e. you can't specify an invalid channel
        or a frequency that will not be accepted).

        Args:
            interface: string, The name of the interface to use.
            mode: string, MODE_11x defined above.
            channel: int, channel number.
            frequency: int, frequency of channel.
            n_capabilities: list of N_CAPABILITY_x defined above.
            beacon_interval: int, beacon interval of AP.
            dtim_period: int, include a DTIM every |dtim_period| beacons.
            frag_threshold: int, maximum outgoing data frame size.
            short_preamble: Whether to use a short preamble.
            ssid: string, The name of the ssid to brodcast.
            hidden: bool, Should the ssid be hidden.
            security: Security, the secuirty settings to use.
            bssid: string, a MAC address like string for the BSSID.
            force_wmm: True if we should force WMM on, False if we should
                force it off, None if we shouldn't force anything.
            pmf_support: one of PMF_SUPPORT_* above.  Controls whether the
                client supports/must support 802.11w.
            obss_interval: int, interval in seconds that client should be
                required to do background scans for overlapping BSSes.
            vht_channel_width: object channel width
            vht_center_channel: int, center channel of segment 0.
            ac_capabilities: list of AC_CAPABILITY_x defined above.
            beacon_footer: string, containing (unvalidated) IE data to be
                placed at the end of the beacon.
            spectrum_mgmt_required: True if we require the DUT to support
                spectrum management.
            scenario_name: string to be included in file names, instead
                of the interface name.
            min_streams: int, number of spatial streams required.
            control_interface: The file name to use as the control interface.
            bss_settings: The settings for all bss.
        """
        self._interface = interface
        if channel is not None and frequency is not None:
            raise ValueError('Specify either frequency or channel '
                             'but not both.')

        self._wmm_enabled = False
        unknown_caps = [
            cap for cap in n_capabilities
            if cap not in hostapd_constants.N_CAPABILITIES_MAPPING
        ]
        if unknown_caps:
            raise ValueError('Unknown capabilities: %r' % unknown_caps)

        self._frequency = None
        if channel:
            self.channel = channel
        elif frequency:
            self.frequency = frequency
        else:
            raise ValueError('Specify either frequency or channel.')
        '''
        if set_ap_defaults_model:
            ap_default_config = hostapd_ap_default_configs.APDefaultConfig(
                profile_name=set_ap_defaults_model, frequency=self.frequency)
            force_wmm = ap_default_config.force_wmm
            beacon_interval = ap_default_config.beacon_interval
            dtim_period = ap_default_config.dtim_period
            short_preamble = ap_default_config.short_preamble
            self._interface = ap_default_config.interface
            mode = ap_default_config.mode
            if ap_default_config.n_capabilities:
                n_capabilities = ap_default_config.n_capabilities
            if ap_default_config.ac_capabilities:
                ap_default_config = ap_default_config.ac_capabilities
        '''

        self._n_capabilities = set(n_capabilities)
        if self._n_capabilities:
            self._wmm_enabled = True
        if self._n_capabilities and mode is None:
            mode = hostapd_constants.MODE_11N_PURE
        self._mode = mode

        if not self.supports_frequency(self.frequency):
            raise ValueError('Configured a mode %s that does not support '
                             'frequency %d' % (self._mode, self.frequency))

        self._beacon_interval = beacon_interval
        self._dtim_period = dtim_period
        self._frag_threshold = frag_threshold
        self._short_preamble = short_preamble

        self._ssid = ssid
        self._hidden = hidden
        self._security = security
        self._bssid = bssid
        if force_wmm is not None:
            self._wmm_enabled = force_wmm
        if pmf_support not in hostapd_constants.PMF_SUPPORT_VALUES:
            raise ValueError('Invalid value for pmf_support: %r' % pmf_support)

        self._pmf_support = pmf_support
        self._obss_interval = obss_interval
        if self.is_11ac:
            if str(vht_channel_width) == '40' or str(
                    vht_channel_width) == '20':
                self._vht_oper_chwidth = hostapd_constants.VHT_CHANNEL_WIDTH_40
            elif str(vht_channel_width) == '80':
                self._vht_oper_chwidth = hostapd_constants.VHT_CHANNEL_WIDTH_80
            elif str(vht_channel_width) == '160':
                self._vht_oper_chwidth = hostapd_constants.VHT_CHANNEL_WIDTH_160
            elif str(vht_channel_width) == '80+80':
                self._vht_oper_chwidth = hostapd_constants.VHT_CHANNEL_WIDTH_80_80
            elif vht_channel_width is not None:
                raise ValueError('Invalid channel width')
            else:
                logging.warning(
                    'No channel bandwidth specified.  Using 80MHz for 11ac.')
                self._vht_oper_chwidth = 1
            if not vht_channel_width == 20:
                if not vht_center_channel:
                    self._vht_oper_centr_freq_seg0_idx = self._get_11ac_center_channel_from_channel(
                        self.channel)
            else:
                self._vht_oper_centr_freq_seg0_idx = vht_center_channel
            self._ac_capabilities = set(ac_capabilities)
        self._beacon_footer = beacon_footer
        self._spectrum_mgmt_required = spectrum_mgmt_required
        self._scenario_name = scenario_name
        self._min_streams = min_streams

        self._bss_lookup = {}
        for bss in bss_settings:
            if bss.name in self._bss_lookup:
                raise ValueError('Cannot have multiple bss settings with the'
                                 ' same name.')
            self._bss_lookup[bss.name] = bss

    def __repr__(self):
        return ('%s(mode=%r, channel=%r, frequency=%r, '
                'n_capabilities=%r, beacon_interval=%r, '
                'dtim_period=%r, frag_threshold=%r, ssid=%r, bssid=%r, '
                'wmm_enabled=%r, security_config=%r, '
                'spectrum_mgmt_required=%r)' %
                (self.__class__.__name__, self._mode, self.channel,
                 self.frequency, self._n_capabilities, self._beacon_interval,
                 self._dtim_period, self._frag_threshold, self._ssid,
                 self._bssid, self._wmm_enabled, self._security,
                 self._spectrum_mgmt_required))

    def supports_channel(self, value):
        """Check whether channel is supported by the current hardware mode.

        @param value: int channel to check.
        @return True iff the current mode supports the band of the channel.

        """
        for freq, channel in hostapd_constants.CHANNEL_MAP.iteritems():
            if channel == value:
                return self.supports_frequency(freq)

        return False

    def supports_frequency(self, frequency):
        """Check whether frequency is supported by the current hardware mode.

        @param frequency: int frequency to check.
        @return True iff the current mode supports the band of the frequency.

        """
        if self._mode == hostapd_constants.MODE_11A and frequency < 5000:
            return False

        if self._mode in (hostapd_constants.MODE_11B,
                          hostapd_constants.MODE_11G) and frequency > 5000:
            return False

        if frequency not in hostapd_constants.CHANNEL_MAP:
            return False

        channel = hostapd_constants.CHANNEL_MAP[frequency]
        supports_plus = (channel in hostapd_constants.HT40_ALLOW_MAP[
            hostapd_constants.N_CAPABILITY_HT40_PLUS_CHANNELS])
        supports_minus = (channel in hostapd_constants.HT40_ALLOW_MAP[
            hostapd_constants.N_CAPABILITY_HT40_MINUS_CHANNELS])
        if (hostapd_constants.N_CAPABILITY_HT40_PLUS in self._n_capabilities
                and not supports_plus):
            return False

        if (hostapd_constants.N_CAPABILITY_HT40_MINUS in self._n_capabilities
                and not supports_minus):
            return False

        return True

    def add_bss(self, bss):
        """Adds a new bss setting.

        Args:
            bss: The bss settings to add.
        """
        if bss.name in self._bss_lookup:
            raise ValueError('A bss with the same name already exists.')

        self._bss_lookup[bss.name] = bss

    def remove_bss(self, bss_name):
        """Removes a bss setting from the config."""
        del self._bss_lookup[bss_name]

    def package_configs(self):
        """Package the configs.

        Returns:
            A list of dictionaries, one dictionary for each section of the
            config.
        """
        # Start with the default config parameters.
        conf = self._get_default_config
        if self._interface:
            conf['interface'] = self._interface
        if self._bssid:
            conf['bssid'] = self._bssid
        if self._ssid:
            conf['ssid'] = self._ssid
            conf['ignore_broadcast_ssid'] = 1 if self._hidden else 0
        conf['channel'] = self.channel
        conf['hw_mode'] = self.hw_mode
        if self.is_11n or self.is_11ac:
            conf['ieee80211n'] = 1
            conf['ht_capab'] = self._hostapd_ht_capabilities
        if self.is_11ac:
            conf['ieee80211ac'] = 1
            conf['vht_oper_chwidth'] = self._vht_oper_chwidth
            conf['vht_oper_centr_freq_seg0_idx'] = \
                    self._vht_oper_centr_freq_seg0_idx
            conf['vht_capab'] = self._hostapd_vht_capabilities
        if self._wmm_enabled:
            conf['wmm_enabled'] = 1
        if self._require_ht:
            conf['require_ht'] = 1
        if self._require_vht:
            conf['require_vht'] = 1
        if self._beacon_interval:
            conf['beacon_int'] = self._beacon_interval
        if self._dtim_period:
            conf['dtim_period'] = self._dtim_period
        if self._frag_threshold:
            conf['fragm_threshold'] = self._frag_threshold
        if self._pmf_support:
            conf['ieee80211w'] = self._pmf_support
        if self._obss_interval:
            conf['obss_interval'] = self._obss_interval
        if self._short_preamble:
            conf['preamble'] = 1
        if self._spectrum_mgmt_required:
            # To set spectrum_mgmt_required, we must first set
            # local_pwr_constraint. And to set local_pwr_constraint,
            # we must first set ieee80211d. And to set ieee80211d, ...
            # Point being: order matters here.
            conf['country_code'] = 'US'  # Required for local_pwr_constraint
            conf['ieee80211d'] = 1  # Required for local_pwr_constraint
            conf['local_pwr_constraint'] = 0  # No local constraint
            conf['spectrum_mgmt_required'] = 1  # Requires local_pwr_constraint

        if self._security:
            for k, v in self._security.generate_dict().items():
                conf[k] = v

        all_conf = [conf]

        for bss in self._bss_lookup.values():
            bss_conf = collections.OrderedDict()
            for k, v in (bss.generate_dict()).items():
                bss_conf[k] = v
            all_conf.append(bss_conf)

        return all_conf
