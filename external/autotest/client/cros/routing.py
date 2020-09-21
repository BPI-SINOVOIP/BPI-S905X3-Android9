# Copyright 2018 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Return information about routing table entries

Read and parse the system routing table. There are
four classes defined here: NetworkRoutes, which contains
information about all routes; IPv4Route, which describes
a single IPv4 routing table entry; IPv6Route, which
does the same for IPv6; and Route, which has common code
for IPv4Route and IPv6Route.
"""

ROUTES_V4_FILE = "/proc/net/route"
ROUTES_V6_FILE = "/proc/net/ipv6_route"

# The following constants are from <net/route.h>
RTF_UP      = 0x0001
RTF_GATEWAY = 0x0002
RTF_HOST    = 0x0004
# IPv6 constants from <net/route.h>
RTF_DEFAULT = 0x10000

import socket
import struct

class Route(object):
    def __init__(self, iface, dest, gway, flags, mask):
        self.interface = iface
        self.destination = dest
        self.gateway = gway
        self.flagbits = flags
        self.netmask = mask

    def __str__(self):
        flags = ""
        if self.flagbits & RTF_UP:
            flags += "U"
        if self.flagbits & RTF_GATEWAY:
            flags += "G"
        if self.flagbits & RTF_HOST:
            flags += "H"
        if self.flagbits & RTF_DEFAULT:
            flags += "D"
        return "<%s dest: %s gway: %s mask: %s flags: %s>" % (
                self.interface,
                self._intToIp(self.destination),
                self._intToIp(self.gateway),
                self._intToIp(self.netmask),
                flags)

    def isUsable(self):
        return self.flagbits & RTF_UP

    def isHostRoute(self):
        return self.flagbits & RTF_HOST

    def isGatewayRoute(self):
        return self.flagbits & RTF_GATEWAY

    def isInterfaceRoute(self):
        return (self.flagbits & RTF_GATEWAY) == 0

    def matches(self, ip):
        try:
            return (self._ipToInt(ip) & self.netmask) == self.destination
        except socket.error:
            return False


class IPv4Route(Route):
    def __init__(self, iface, dest, gway, flags, mask):
        super(IPv4Route, self).__init__(
            iface, int(dest, 16), int(gway, 16), int(flags, 16), int(mask, 16))

    def _intToIp(self, addr):
        return socket.inet_ntoa(struct.pack('@I', addr))

    def _ipToInt(self, ip):
        return struct.unpack('I', socket.inet_aton(ip))[0]

    def isDefaultRoute(self):
        return (self.flagbits & RTF_GATEWAY) and self.destination == 0

def parseIPv4Routes(routelist):
    # The first line is headers that will allow us
    # to correctly interpret the values in the following
    # lines
    headers = routelist[0].split()
    col_map = {token: pos for (pos, token) in enumerate(headers)}

    routes = []
    for routeline in routelist[1:]:
        route = routeline.split()
        interface = route[col_map["Iface"]]
        destination = route[col_map["Destination"]]
        gateway = route[col_map["Gateway"]]
        flags = route[col_map["Flags"]]
        mask = route[col_map["Mask"]]
        routes.append(IPv4Route(interface, destination, gateway, flags, mask))

    return routes


class IPv6Route(Route):
    def __init__(self, iface, dest, gway, flags, plen):
        super(IPv6Route, self).__init__(
            iface,
            long(dest, 16),
            long(gway, 16),
            long(flags, 16),
            # netmask = set first plen bits to 1, all following to 0
            (1 << 128) - (1 << (128 - int(plen, 16))))

    def _intToIp(self, addr):
        return socket.inet_ntop(socket.AF_INET6, ("%032x" % addr).decode("hex"))

    def _ipToInt(self, ip):
        return long(socket.inet_pton(socket.AF_INET6, ip).encode("hex"), 16)

    def isDefaultRoute(self):
        return self.flagbits & RTF_DEFAULT

def parseIPv6Routes(routelist):
    # ipv6_route has no headers, so the routing table looks like the following:
    # Dest DestPrefix Src SrcPrefix Gateway Metric RefCnt UseCnt Flags Iface
    routes = []
    for routeline in routelist:
        route = routeline.split()
        interface = route[9]
        destination = route[0]
        gateway = route[4]
        flags = route[8]
        prefix = route[1]
        routes.append(IPv6Route(interface, destination, gateway, flags, prefix))

    return routes


class NetworkRoutes(object):
    def __init__(self, routelist_v4=None, routelist_v6=None):
        if routelist_v4 is None:
            with open(ROUTES_V4_FILE) as routef_v4:
                routelist_v4 = routef_v4.readlines()

        self.routes = parseIPv4Routes(routelist_v4)

        if routelist_v6 is None:
            with open(ROUTES_V6_FILE) as routef_v6:
                routelist_v6 = routef_v6.readlines()

        self.routes += parseIPv6Routes(routelist_v6)

    def _filterUsableRoutes(self):
        return (rr for rr in self.routes if rr.isUsable())

    def hasDefaultRoute(self, interface):
        return any(rr for rr in self._filterUsableRoutes()
                   if (rr.interface == interface and rr.isDefaultRoute()))

    def getDefaultRoutes(self):
        return [rr for rr in self._filterUsableRoutes() if rr.isDefaultRoute()]

    def hasInterfaceRoute(self, interface):
        return any(rr for rr in self._filterUsableRoutes()
                   if (rr.interface == interface and rr.isInterfaceRoute()))

    def getRouteFor(self, ip):
        for rr in self._filterUsableRoutes():
            if rr.matches(ip):
                return rr
        return None
