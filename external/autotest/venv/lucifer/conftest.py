# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""pytest extensions."""

from __future__ import absolute_import
from __future__ import division
from __future__ import print_function

import pytest


def pytest_addoption(parser):
    """Add extra pytest options."""
    parser.addoption("--skipslow", action="store_true",
                     default=False, help="Skip slow tests")


def pytest_collection_modifyitems(config, items):
    """Modify tests to remove slow tests if --skipslow was passed."""
    if config.getoption("--skipslow"):  # pragma: no cover
        skip_slow = pytest.mark.skip(reason="--skipslow option was passed")
        for item in items:
            if "slow" in item.keywords:
                item.add_marker(skip_slow)
