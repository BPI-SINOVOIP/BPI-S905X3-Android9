# Copyright 2016 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""FAFT config setting overrides for Asuka."""

from autotest_lib.server.cros.faft.config import kunimitsu

class Values(kunimitsu.Values):
    """Inherit overrides from Kunimitsu."""
    pass
