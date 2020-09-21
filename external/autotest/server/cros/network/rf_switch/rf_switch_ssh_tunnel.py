# Copyright (c) 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Interface to RF Switch over SSH tunnel.

This helper will help with communicating to a RF Switch which is behind
a firewall and can be accessed through a SSH tunnel. Please make sure you
can login to SSH Tunnel machine without a password by configuring the ssh
pub keys.

                       /|
                      / |
                     /  |
                    |   |
    +---------+     |   |     +--------+      +--------+
    |         |     |   |     |        |      |        |
    |  Test   +-----|  -------+  SSH   +------+   RF   |
    | Machine |     |   |     | Tunnel |      | Switch |
    |         |     |   |     |        |      |        |
    +---------+     |   |     +--------+      +--------+
                    |  /
                    | /
                    |/

                 Firewall

"""

import contextlib

from autotest_lib.server.cros.network.rf_switch import rf_switch
from autotest_lib.server.cros.network.rf_switch import scpi
from autotest_lib.server.cros.network.rf_switch import scpi_ssh_tunnel


class RfSwitchSshTunnel(scpi_ssh_tunnel.ScpiSshTunnel, rf_switch.RfSwitch):
    """RF Switch handler over ssh tunnel."""

    def __init__(self,
                 host,
                 proxy_host,
                 proxy_username,
                 port=scpi_ssh_tunnel.ScpiSshTunnel.SCPI_PORT,
                 proxy_port=None):
        """
        RF Switch with a proxy server.

        @param host: Hostname or IP address of RF Switch
        @param proxy_host: Hostname or IP address of SSH tunnel device
        @param proxy_username: Username for SSH tunnel device
        @param port: SCPI port on device (default 5025)
        @param proxy_port: port number to bind for SSH tunnel

        """
        self.host = host
        self.port = port
        self.proxy_host = proxy_host
        self.proxy_username = proxy_username
        self.proxy_port = proxy_port

        scpi_ssh_tunnel.ScpiSshTunnel.__init__(
            self,
            host=self.host,
            port=self.port,
            proxy_host=self.proxy_host,
            proxy_username=self.proxy_username,
            proxy_port=self.proxy_port)


@contextlib.contextmanager
def RfSwitchSshTunnelSession(host,
                             proxy_host,
                             proxy_username,
                             port=scpi.Scpi.SCPI_PORT,
                             proxy_port=None):
    """Start a RF Switch with ssh tunnel session and close it when done.

    RF Switch with a proxy server.

    @param host: Hostname or IP address of RF Switch
    @param proxy_host: Hostname or IP address of SSH tunnel device
    @param proxy_username: Username for SSH tunnel device
    @param port: SCPI port on device (default 5025)
    @param proxy_port: port number to bind for SSH tunnel

    @yields: an instance of InteractiveSsh
    """
    session = RfSwitchSshTunnel(host=host,
                                proxy_host=proxy_host,
                                proxy_username=proxy_username,
                                port=port,
                                proxy_port=proxy_port)
    try:
        yield session
    finally:
        session.close()
