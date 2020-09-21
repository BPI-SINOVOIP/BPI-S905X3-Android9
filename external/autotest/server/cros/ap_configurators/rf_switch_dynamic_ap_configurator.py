# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that
# can be found in the LICENSE file.

"""Class to configure APs managed by RF Switch."""

import dynamic_ap_configurator

class RFSwitchDynamicAPConfigurator(
        dynamic_ap_configurator.DynamicAPConfigurator):
    """Base class to configure APs using Webdriver"""

    def power_up_router(self):
        """APs are always ON, bypassing the power up command."""
        pass

    def power_down_router(self):
        """APs are always ON, bypassing the power down command."""
        pass
