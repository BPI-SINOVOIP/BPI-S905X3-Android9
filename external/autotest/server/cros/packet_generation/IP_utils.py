# Copyright 2016 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import os

from autotest_lib.server.hosts import ssh_host

RA_SCRIPT = 'sendra.py'
SCAPY = 'scapy-2.2.0.tar.gz'
SCAPY_INSTALL_COMMAND = 'sudo python setup.py install'
PROC_NET_SNMP6 = '/proc/net/snmp6'
MULTICAST_ADDR = '33:33:00:00:00:01'
IFACE = 'managed0'
LIFETIME = 180


class IPutils(object):

    def __init__(self, host):
        """Initializes an IP utility interface.

        @param host: Router host object.

        """
        self.host = host
        self.install_path = self.host.run('mktemp -d').stdout.rstrip()


    def install_scapy(self):
        """Installs scapy on the target device. Scapy and all related files and
        scripts will be installed in a temp directory under /tmp.

        """
        scapy = os.path.join(self.install_path, SCAPY)
        ap_sshhost = ssh_host.SSHHost(hostname=self.host.hostname)
        current_dir = os.path.dirname(os.path.realpath(__file__))
        send_ra_script = os.path.join(current_dir, RA_SCRIPT)
        send_scapy = os.path.join(current_dir, SCAPY)
        ap_sshhost.send_file(send_scapy, self.install_path)
        ap_sshhost.send_file(send_ra_script, self.install_path)

        self.host.run('tar -xvf %s -C %s' % (scapy, self.install_path))
        self.host.run('cd %s; %s' % (self.install_path, SCAPY_INSTALL_COMMAND))


    def cleanup_scapy(self):
        """Remove all scapy related files and scripts from device.

        @param host: Router host object.

        """
        self.host.run('rm -rf %s' % self.install_path)


    def send_ra(self, mac=MULTICAST_ADDR, interval=1, count=None, iface=IFACE,
                lifetime=LIFETIME):
        """Invoke scapy and send RA to the device.

        @param host: Router host object.
        @param mac: string HWAddr/MAC address to send the packets to.
        @param interval: int Time to sleep between consecutive packets.
        @param count: int Number of packets to be sent.
        @param iface: string of the WiFi interface to use for sending packets.
        @param lifetime: int original RA's router lifetime in seconds.

        """
        scapy_command = os.path.join(self.install_path, RA_SCRIPT)
        options = ' -m %s -i %d -c %d -l %d -in %s' %(mac, interval, count,
                                                      lifetime, iface)
        self.host.run(scapy_command + options)


    def get_icmp6intype134(self, host):
        """Read the value of Icmp6InType134 and return integer.

        @param host: DUT host object.

        @returns integer value >0 if grep is successful; 0 otherwise.

        """
        ra_count_str = host.run(
                       'grep Icmp6InType134 %s || true' % PROC_NET_SNMP6).stdout
        if ra_count_str:
            return int(ra_count_str.split()[1])
        # If grep failed it means that there is no entry for Icmp6InType134 in file.
        return 0
