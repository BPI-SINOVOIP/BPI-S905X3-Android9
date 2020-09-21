# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that
# can be found in the LICENSE file.

"""Class to provide Static AP configurations for APs managed by RF Switch."""

import static_ap_configurator

class RFSwitchStaticAPConfigurator(
        static_ap_configurator.StaticAPConfigurator):
    """Derived class to provide AP configuration details."""

    def power_up_router(self):
        """APs are always ON, bypassing the power up command."""
        pass

    def power_down_router(self):
        """APs are always ON, bypassing the power down command."""
        pass
