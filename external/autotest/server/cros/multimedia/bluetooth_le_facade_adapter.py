# Copyright 2016 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""An adapter to remotely access the bluetooth le facade on a DUT."""

from autotest_lib.server.cros.bluetooth.bluetooth_device import BluetoothDevice


class BluetoothLEFacadeRemoteAdapter(BluetoothDevice):
    """An adapter to remotely control DUT bluetooth low-energy (le) device.

    The Autotest host object representing the remote DUT, passed to this
    class on initialization, can be accessed from its _client property.

    """
    def __init__(self, host):
        """Construct an BluetoothLEFacadeRemoteAdapter.

        @param host: Host object representing a remote host.

        """
        self._client = host
        super(BluetoothLEFacadeRemoteAdapter, self).__init__(host)
