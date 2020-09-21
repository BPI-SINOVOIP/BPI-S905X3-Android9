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

import ipaddress
import re

from acts.controllers.utils_lib.ssh import connection


class Error(Exception):
    """Exception thrown when a valid ip command experiences errors."""


class NetworkInterfaceDown(Error):
    """Exception thrown when a network interface is down."""


class LinuxRouteCommand(object):
    """Interface for doing standard ip route commands on a linux system."""

    DEFAULT_ROUTE = 'default'

    def __init__(self, runner):
        """
        Args:
            runner: Object that can take unix commands and run them in an
                    enviroment.
        """
        self._runner = runner

    def add_route(self, net_interface, address):
        """Add an entry to the ip routing table.

        Will add a route for either a specific ip address, or a network.

        Args:
            net_interface: string, Any packet that sends through this route
                           will be sent using this network interface
                           (eg. wlan0).
            address: ipaddress.IPv4Address, ipaddress.IPv4Network,
                     or DEFAULT_ROUTE. The address to use. If a network
                     is given then the entire subnet will be routed.
                     If DEFAULT_ROUTE is given then this will set the
                     default route.

        Raises:
            NetworkInterfaceDown: Raised when the network interface is down.
        """
        try:
            self._runner.run('ip route add %s dev %s' %
                             (address, net_interface))
        except connection.CommandError as e:
            if 'File exists' in e.result.stderr:
                raise Error('Route already exists.')
            if 'Network is down' in e.result.stderr:
                raise NetworkInterfaceDown(
                    'Device must be up for adding a route.')
            raise

    def get_routes(self, net_interface=None):
        """Get the routes in the ip routing table.

        Args:
            net_interface: string, If given, only retrive routes that have
                           been registered to go through this network
                           interface (eg. wlan0).

        Returns: An iterator that returns a tuple of (address, net_interface).
                 If it is the default route then address
                 will be the DEFAULT_ROUTE. If the route is a subnet then
                 it will be a ipaddress.IPv4Network otherwise it is a
                 ipaddress.IPv4Address.
        """
        result = self._runner.run('ip route show')

        lines = result.stdout.splitlines()

        # Scan through each line for valid route entries
        # Example output:
        # default via 192.168.1.254 dev eth0  proto static
        # 192.168.1.0/24 dev eth0  proto kernel  scope link  src 172.22.100.19  metric 1
        # 192.168.2.1 dev eth2 proto kernel scope link metric 1
        for line in lines:
            if not 'dev' in line:
                continue

            if line.startswith(self.DEFAULT_ROUTE):
                # The default route entry is formatted differently.
                match = re.search('dev (?P<net_interface>.*)', line)
                pair = None
                if match:
                    # When there is a match for the route entry pattern create
                    # A pair to hold the info.
                    pair = (self.DEFAULT_ROUTE,
                            match.groupdict()['net_interface'])
            else:
                # Test the normal route entry pattern.
                match = re.search(
                    '(?P<address>[^\s]*) dev (?P<net_interface>[^\s]*)', line)
                pair = None
                if match:
                    # When there is a match for the route entry pattern create
                    # A pair to hold the info.
                    d = match.groupdict()
                    # Route can be either a network or specific address
                    try:
                        address = ipaddress.IPv4Address(d['address'])
                    except ipaddress.AddressValueError:
                        address = ipaddress.IPv4Network(d['address'])

                    pair = (address, d['net_interface'])

            # No pair means no pattern was found.
            if not pair:
                continue

            if net_interface:
                # If a net_interface was passed in then only give the pair when it is
                # The correct net_interface.
                if pair[1] == net_interface:
                    yield pair
            else:
                # No net_interface given give all valid route entries.
                yield pair

    def is_route(self, address, net_interface=None):
        """Checks to see if a route exists.

        Args:
            address: ipaddress.IPv4Address, ipaddress.IPv4Network,
                     or DEFAULT_ROUTE, The address to use.
            net_interface: string, If specified, the route must be
                           registered to go through this network interface
                           (eg. wlan0).

        Returns: True if the route is found, False otherwise.
        """
        for route, _ in self.get_routes(net_interface):
            if route == address:
                return True

        return False

    def remove_route(self, address, net_interface=None):
        """Removes a route from the ip routing table.

        Removes a route from the ip routing table. If the route does not exist
        nothing is done.

        Args:
            address: ipaddress.IPv4Address, ipaddress.IPv4Network,
                     or DEFAULT_ROUTE, The address of the route to remove.
            net_interface: string, If specified the route being removed is
                           registered to go through this network interface
                           (eg. wlan0)
        """
        try:
            if net_interface:
                self._runner.run('ip route del %s dev %s' %
                                 (address, net_interface))
            else:
                self._runner.run('ip route del %s' % address)
        except connection.CommandError as e:
            if 'No such process' in e.result.stderr:
                # The route didn't exist.
                return
            raise

    def clear_routes(self, net_interface=None):
        """Clears all routes.

        Args:
            net_interface: The network interface to clear routes on.
            If not given then all routes will be removed on all network
            interfaces (eg. wlan0).
        """
        routes = self.get_routes(net_interface)

        for a, d in routes:
            self.remove_route(a, d)
