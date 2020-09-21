# Copyright 2016 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib.cros import kiosk_utils


class KioskFacadeNative(object):
    """Facade to access the Kiosk functionality."""


    def __init__(self, resource):
        """
        Initializes a KioskFacadeNative.

        @param resource: A FacadeResource object.

        """
        self._resource = resource


    def config_rise_player(self, ext_id, app_config_id):
        """
        Configure Rise Player app with a specific display id.

        @param ext_id: extension id of the Rise Player Kiosk App.
        @param app_config_id: display id for the Rise Player app.

        """
        custom_chrome_setup = {"clear_enterprise_policy": False,
                               "dont_override_profile": True,
                               "disable_gaia_services": False,
                               "disable_default_apps": False,
                               "auto_login": False}
        self._resource.start_custom_chrome(custom_chrome_setup)
        kiosk_utils.config_riseplayer(
                self._resource._browser, ext_id, app_config_id)
