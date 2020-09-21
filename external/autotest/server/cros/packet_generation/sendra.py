#!/usr/bin/python

import argparse
import time

from scapy import all as scapy


def send(dstmac, interval, count, lifetime, iface):
    """Generate IPv6 Router Advertisement and send to destination.

    @param dstmac: string HWAddr of the destination ipv6 node.
    @param interval: int Time to sleep between consecutive packets.
    @param count: int Number of packets to be sent.
    @param lifetime: Router lifetime value for the original RA.
    @param iface: string Router's WiFi interface to send packets over.

    """
    while count:
        ra = (scapy.Ether(dst=dstmac) /
              scapy.IPv6() /
              scapy.ICMPv6ND_RA(routerlifetime=lifetime))
        scapy.sendp(ra, iface=iface)
        count = count - 1
        time.sleep(interval)
        lifetime = lifetime - interval


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument('-m', '--mac-address', action='store', default=None,
                         help='HWAddr to send the packet to.')
    parser.add_argument('-i', '--t-interval', action='store', default=None,
                         type=int, help='Time to sleep between consecutive')
    parser.add_argument('-c', '--pkt-count', action='store', default=None,
                        type=int, help='NUmber of packets to send.')
    parser.add_argument('-l', '--life-time', action='store', default=None,
                        type=int, help='Lifetime in seconds for the first RA')
    parser.add_argument('-in', '--wifi-interface', action='store', default=None,
                        help='The wifi interface to send packets over.')
    args = parser.parse_args()
    send(args.mac_address, args.t_interval, args.pkt_count, args.life_time,
         args.wifi_interface)
