# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Implements a Cast OS host type."""

from autotest_lib.server.hosts import ssh_host

OS_TYPE_CAST_OS = 'cast_os'


class CastOSHost(ssh_host.SSHHost):
    """Implements a Cast OS host type."""


    def get_os_type(self):
        """Returns the host OS descriptor."""
        return OS_TYPE_CAST_OS


    def get_wifi_interface_name(self):
        """Gets the WiFi interface name."""
        return self.run('getprop wifi.interface').stdout.rstrip()


    @property
    def is_client_install_supported(self):
        return False
