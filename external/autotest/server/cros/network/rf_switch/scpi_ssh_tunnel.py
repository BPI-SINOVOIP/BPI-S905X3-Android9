# Copyright (c) 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Interface for SCPI Protocol through a SSH tunnel.

This helper will help with communicating to a SCPI device which is behind
a firewall and can be accessed through a SSH tunnel. Please make sure you
can login to SSH Tunnel machine without a password by configuring the ssh
pub keys.

                       /|
                      / |
                     /  |
                    |   |
    +---------+     |   |     +--------+      +--------+
    |         |     |   |     |        |      |        |
    |  Test   +-----|  -------+  SSH   +------+  SCPI  |
    | Machine |     |   |     | Tunnel |      | Device |
    |         |     |   |     |        |      |        |
    +---------+     |   |     +--------+      +--------+
                    |  /
                    | /
                    |/

                 Firewall
"""

import logging
import shlex
import socket
import subprocess
import sys
import time

from autotest_lib.client.common_lib import utils
from autotest_lib.server.cros.network.rf_switch import scpi


class ScpiSshTunnel(scpi.Scpi):
    """Class for SCPI protocol though SSH tunnel."""

    _TUNNEL_ESTABLISH_TIME_SECS = 10

    def __init__(self,
                 host,
                 proxy_host,
                 proxy_username,
                 port=scpi.Scpi.SCPI_PORT,
                 proxy_port=None):
        """SCPI handler through a proxy server.

        @param host: Hostname or IP address of SCPI device
        @param proxy_host: Hostname or IP address of SSH tunnel device
        @param proxy_username: Username for SSH tunnel device
        @param port: SCPI port on device (default 5025)
        @param proxy_port: port number to bind for SSH tunnel. If
            none will pick an available free port

        """
        self.host = host
        self.port = port
        self.proxy_host = proxy_host
        self.proxy_username = proxy_username
        self.proxy_port = proxy_port or utils.get_unused_port()

        # We will override the parent initialize method and use a tunnel
        # to connect to the socket connection

        # Start SSH tunnel
        try:
            tunnel_command = self._make_tunnel_command()
            logging.debug('Tunnel command: %s', tunnel_command)
            args = shlex.split(tunnel_command)
            self.ssh_tunnel = subprocess.Popen(args)
            time.sleep(self._TUNNEL_ESTABLISH_TIME_SECS)
            logging.debug(
                'Started ssh tunnel, local = %d, remote = %d, pid = %d',
                self.proxy_port, self.port, self.ssh_tunnel.pid)
        except OSError as e:
            logging.exception('Error starting SSH tunnel to SCPI device.')
            raise scpi.ScpiException(cause=e), None, sys.exc_info()[2]

        # Open a socket connection for communication with chassis
        # using the SSH Tunnel.
        try:
            self.socket = socket.socket()
            self.socket.connect(('127.0.0.1', self.proxy_port))
        except (socket.error, socket.timeout) as e:
            logging.error('Error connecting to SCPI device.')
            raise scpi.ScpiException(cause=e), None, sys.exc_info()[2]

    def _make_tunnel_command(self, hosts_file='/dev/null',
                             connect_timeout=30, alive_interval=300):
        tunnel_command = ('/usr/bin/ssh -ax -o StrictHostKeyChecking=no'
                          ' -o UserKnownHostsFile=%s -o BatchMode=yes'
                          ' -o ConnectTimeout=%s -o ServerAliveInterval=%s'
                          ' -l %s -L %s:%s:%s  -nNq %s') % (
                              hosts_file, connect_timeout, alive_interval,
                              self.proxy_username, self.proxy_port,
                              self.host, self.port, self.proxy_host)
        return tunnel_command

    def close(self):
        """Close the connection."""
        if hasattr(self, 's'):
            self.socket.close()
            del self.socket
        if self.ssh_tunnel:
            self.ssh_tunnel.kill()
            del self.ssh_tunnel
