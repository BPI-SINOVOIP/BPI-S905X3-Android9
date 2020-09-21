# Copyright 2016 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""This class defines the common Label classes."""

import common

from autotest_lib.server.cros.dynamic_suite import constants
from autotest_lib.server.hosts import base_label


class OSLabel(base_label.StringPrefixLabel):
    """Return the os label."""

    _NAME = constants.OS_PREFIX

    # pylint: disable=missing-docstring
    def generate_labels(self, host):
        return [host.get_os_type()]
