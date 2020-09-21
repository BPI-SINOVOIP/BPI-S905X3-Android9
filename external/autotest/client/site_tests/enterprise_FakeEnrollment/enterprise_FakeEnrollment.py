# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from autotest_lib.client.cros.enterprise import enterprise_policy_base

class enterprise_FakeEnrollment(enterprise_policy_base.EnterprisePolicyTest):
    """Test to fake enroll and fake login using a fake dmserver."""
    version = 1

    def run_once(self):
        """Enroll and login."""
        self.setup_case(enroll=True, auto_login=True)
