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

import collections
import copy
import ipaddress


class Subnet(object):
    """Configs for a subnet  on the dhcp server.

    Attributes:
        network: ipaddress.IPv4Network, the network that this subnet is in.
        start: ipaddress.IPv4Address, the start ip address.
        end: ipaddress.IPv4Address, the end ip address.
        router: The router to give to all hosts in this subnet.
        lease: The lease time of all hosts in this subnet.
    """

    def __init__(self,
                 subnet,
                 start=None,
                 end=None,
                 router=None,
                 lease_time=None):
        """
        Args:
            subnet_address: ipaddress.IPv4Network, The network that this
                            subnet is.
            start: ipaddress.IPv4Address, The start of the address range to
                   give hosts in this subnet. If not given then the first ip in
                   the network is used.
            end: ipaddress.IPv4Address, The end of the address range to give
                 hosts. If not given then the last ip in the network is used.
            router: ipaddress.IPv4Address, The router hosts should use in this
                    subnet. If not given the first ip in the network is used.
            lease_time: int, The amount of lease time in seconds
                        hosts in this subnet have.
        """
        self.network = subnet

        if start:
            self.start = start
        else:
            self.start = self.network[2]

        if not self.start in self.network:
            raise ValueError('The start range is not in the subnet.')
        if self.start.is_reserved:
            raise ValueError('The start of the range cannot be reserved.')

        if end:
            self.end = end
        else:
            self.end = self.network[-2]

        if not self.end in self.network:
            raise ValueError('The end range is not in the subnet.')
        if self.end.is_reserved:
            raise ValueError('The end of the range cannot be reserved.')
        if self.end < self.start:
            raise ValueError(
                'The end must be an address larger than the start.')

        if router:
            if router >= self.start and router <= self.end:
                raise ValueError('Router must not be in pool range.')
            if not router in self.network:
                raise ValueError('Router must be in the given subnet.')

            self.router = router
        else:
            # TODO: Use some more clever logic so that we don't have to search
            # every host potentially.
            self.router = None
            for host in self.network.hosts():
                if host < self.start or host > self.end:
                    self.router = host
                    break

            if not self.router:
                raise ValueError('No useable host found.')

        self.lease_time = lease_time


class StaticMapping(object):
    """Represents a static dhcp host.

    Attributes:
        identifier: How id of the host (usually the mac addres
                    e.g. 00:11:22:33:44:55).
        address: ipaddress.IPv4Address, The ipv4 address to give the host.
        lease_time: How long to give a lease to this host.
    """

    def __init__(self, identifier, address, lease_time=None):
        self.identifier = identifier
        self.ipv4_address = address
        self.lease_time = lease_time


class DhcpConfig(object):
    """The configs for a dhcp server.

    Attributes:
        subnets: A list of all subnets for the dhcp server to create.
        static_mappings: A list of static host addresses.
        default_lease_time: The default time for a lease.
        max_lease_time: The max time to allow a lease.
    """

    def __init__(self,
                 subnets=None,
                 static_mappings=None,
                 default_lease_time=600,
                 max_lease_time=7200):
        self.subnets = copy.deepcopy(subnets) if subnets else []
        self.static_mappings = (copy.deepcopy(static_mappings)
                                if static_mappings else [])
        self.default_lease_time = default_lease_time
        self.max_lease_time = max_lease_time
