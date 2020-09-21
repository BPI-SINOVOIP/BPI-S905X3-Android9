# Copyright 2016 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""An adapter to remotely access the bluetooth hid facade on DUT."""

from autotest_lib.server.cros.bluetooth.bluetooth_device import BluetoothDevice


class BluetoothHIDFacadeRemoteAdapter(BluetoothDevice):
    """This is an adapter to remotely control DUT bluetooth hid.

    The Autotest host object representing the remote DUT, passed to this
    class on initialization, can be accessed from its _client property.

    """
    def __init__(self, host, remote_facade_proxy):
        """Construct an BluetoothHIDFacadeRemoteAdapter.

        @param host: Host object representing a remote host.
        @param remote_facade_proxy: RemoteFacadeProxy object.

        """
        self._client = host
        self._proxy = remote_facade_proxy
        super(BluetoothHIDFacadeRemoteAdapter, self).__init__(host)
