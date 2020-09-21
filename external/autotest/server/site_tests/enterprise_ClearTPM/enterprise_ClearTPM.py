# Copyright 2018 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from autotest_lib.client.common_lib.cros import tpm_utils
from autotest_lib.server import test


class enterprise_ClearTPM(test.test):
    """A utility test that clears the TPM."""
    version = 1

    def run_once(self, host):
        """Entry point of this test."""
        tpm_utils.ClearTPMOwnerRequest(host)
