# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""FAFT config setting overrides for Scarlet."""


from autotest_lib.server.cros.faft.config import gru

class Values(gru.Values):
    """Inherit overrides from Gru."""
    has_lid = False
    has_keyboard = False
    ec_capability = ['arm', 'battery', 'charging']

    mode_switcher_type = 'tablet_detachable_switcher'
    fw_bypasser_type = 'tablet_detachable_bypasser'
