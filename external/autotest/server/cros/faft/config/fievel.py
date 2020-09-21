# Copyright 2016 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""FAFT config overrides for Fievel (RK3288 base platform without EC)."""

from autotest_lib.server.cros.faft.config import brain

class Values(brain.Values):
    """Inherit overrides from Brain."""
    delay_reboot_to_ping = 60

