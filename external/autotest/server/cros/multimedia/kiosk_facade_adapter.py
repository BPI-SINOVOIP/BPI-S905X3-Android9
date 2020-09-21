# Copyright 2016 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.


class KioskFacadeRemoteAdapter(object):
    """KioskFacadeRemoteAdapter is an adapter to remotely control Kiosk on DUT.

    The Autotest host object representing the remote DUT, passed to this
    class on initialization, can be accessed from its _client property.
    """

    def __init__(self, host, remote_facade_proxy):
        """Construct a KioskFacadeRemoteAdapter.

        @param host: Host object representing a remote host.
        @param remote_facade_proxy: RemoteFacadeProxy object.
        """
        self._client = host
        self._proxy = remote_facade_proxy


    @property
    def _kiosk_proxy(self):
        return self._proxy.kiosk


    def config_rise_player(self, ext_id, app_config_id ):
        """
        Configure Rise Player Kiosk App.

        @param ext_id: extension id of the Rise Player Kiosk App.
        @param app_config_id: display id for the Rise Player app .

        """
        self._kiosk_proxy.config_rise_player(ext_id, app_config_id)
