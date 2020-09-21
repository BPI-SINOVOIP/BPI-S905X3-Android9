# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""This module is a hack for backward compatibility.

The real utils module is utils.py
"""

import warnings

import common
from autotest_lib.client.common_lib import deprecation

# pylint: disable=wildcard-import,unused-wildcard-import,redefined-builtin
from .utils import *

warnings.warn(deprecation.APIDeprecationWarning(__name__), stacklevel=2)
