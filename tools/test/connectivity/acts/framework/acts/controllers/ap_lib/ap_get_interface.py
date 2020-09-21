#!/usr/bin/env python3.4
#
#   Copyright 2017 - Google, Inc.
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

import logging
from acts.libs.proc import job

GET_ALL_INTERFACE = 'ls /sys/class/net'
GET_VIRTUAL_INTERFACE = 'ls /sys/devices/virtual/net'
BRCTL_SHOW = 'brctl show'


class ApInterfacesError(Exception):
    """Error related to AP interfaces."""


class ApInterfaces(object):
    """Class to get network interface information for the device.

    """

    def __init__(self, ap):
        """Initialize the ApInterface class.

        Args:
            ap: the ap object within ACTS
        """
        self.ssh = ap.ssh

    def get_all_interface(self):
        """Get all network interfaces on the device.

        Returns:
            interfaces_all: list of all the network interfaces on device
        """
        output = self.ssh.run(GET_ALL_INTERFACE)
        interfaces_all = output.stdout.split('\n')

        return interfaces_all

    def get_virtual_interface(self):
        """Get all virtual interfaces on the device.

        Returns:
            interfaces_virtual: list of all the virtual interfaces on device
        """
        output = self.ssh.run(GET_VIRTUAL_INTERFACE)
        interfaces_virtual = output.stdout.split('\n')

        return interfaces_virtual

    def get_physical_interface(self):
        """Get all the physical interfaces of the device.

        Get all physical interfaces such as eth ports and wlan ports
        Returns:
            interfaces_phy: list of all the physical interfaces
        """
        interfaces_all = self.get_all_interface()
        interfaces_virtual = self.get_virtual_interface()
        interfaces_phy = list(set(interfaces_all) - set(interfaces_virtual))

        return interfaces_phy

    def get_bridge_interface(self):
        """Get all the bridge interfaces of the device.

        Returns:
            interfaces_bridge: the list of bridge interfaces, return None if
                bridge utility is not available on the device
        """
        interfaces_bridge = []
        try:
            output = self.ssh.run(BRCTL_SHOW)
            lines = output.stdout.split('\n')
            for line in lines:
                interfaces_bridge.append(line.split('\t')[0])
            interfaces_bridge.pop(0)
            interfaces_bridge = [x for x in interfaces_bridge if x is not '']
            return interfaces_bridge
        except job.Error:
            logging.info('No brctl utility is available')
            return None

    def get_wlan_interface(self):
        """Get all WLAN interfaces and specify 2.4 GHz and 5 GHz interfaces.

        Returns:
            interfaces_wlan: all wlan interfaces
        Raises:
            ApInterfacesError: Missing at least one WLAN interface
        """
        wlan_2g = None
        wlan_5g = None
        interfaces_phy = self.get_physical_interface()
        for iface in interfaces_phy:
            IW_LIST_FREQ = 'iwlist %s freq' % iface
            output = self.ssh.run(IW_LIST_FREQ)
            if 'Channel 06' in output.stdout and 'Channel 36' not in output.stdout:
                wlan_2g = iface
            elif 'Channel 36' in output.stdout and 'Channel 06' not in output.stdout:
                wlan_5g = iface

        interfaces_wlan = [wlan_2g, wlan_5g]

        if None not in interfaces_wlan:
            return interfaces_wlan

        raise ApInterfacesError('Missing at least one WLAN interface')

    def get_wan_interface(self):
        """Get the WAN interface which has internet connectivity.

        Returns:
            wan: the only one WAN interface
        Raises:
            ApInterfacesError: no running WAN can be found
        """
        wan = None
        interfaces_phy = self.get_physical_interface()
        interfaces_wlan = self.get_wlan_interface()
        interfaces_eth = list(set(interfaces_phy) - set(interfaces_wlan))
        for iface in interfaces_eth:
            network_status = self.check_ping(iface)
            if network_status == 1:
                wan = iface
                break
        if wan:
            return wan

        output = self.ssh.run('ifconfig')
        interfaces_all = output.stdout.split('\n')
        logging.info("IFCONFIG output = %s" % interfaces_all)

        raise ApInterfacesError('No WAN interface available')

    def get_lan_interface(self):
        """Get the LAN interface connecting to local devices.

        Returns:
            lan: the only one running LAN interface of the devices
        Raises:
            ApInterfacesError: no running LAN can be found
        """
        lan = None
        interfaces_phy = self.get_physical_interface()
        interfaces_wlan = self.get_wlan_interface()
        interfaces_eth = list(set(interfaces_phy) - set(interfaces_wlan))
        interface_wan = self.get_wan_interface()
        interfaces_eth.remove(interface_wan)
        for iface in interfaces_eth:
            LAN_CHECK = 'ifconfig %s' % iface
            output = self.ssh.run(LAN_CHECK)
            if 'RUNNING' in output.stdout:
                lan = iface
                break
        if lan:
            return lan

        raise ApInterfacesError(
            'No running LAN interface available, check connection')

    def check_ping(self, iface):
        """Check the ping status on specific interface to determine the WAN.

        Args:
            iface: the specific interface to check
        Returns:
            network_status: the connectivity status of the interface
        """
        PING = 'ping -c 1 -I %s 8.8.8.8' % iface
        try:
            self.ssh.run(PING)
            return 1
        except job.Error:
            return 0
