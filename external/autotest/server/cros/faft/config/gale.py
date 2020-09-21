# Copyright 2016 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""FAFT configuration overrides for Gale."""

from autotest_lib.server.cros.faft.config import jetstream


class Values(jetstream.Values):
    """Inherit overrides from Jetstream."""
    pass
