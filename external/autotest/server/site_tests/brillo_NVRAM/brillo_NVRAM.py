# Copyright 2016 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import common
from autotest_lib.client.common_lib import error
from autotest_lib.server import test


class brillo_NVRAM(test.test):
    """Test Access-Controlled NVRAM HAL functionality."""
    version = 1

    TEST_EXECUTABLE = '/data/nativetest/nvram_hal_test/nvram_hal_test'

    def _get_prop(self, host, property_name):
        """Returns the value of a system property.

        @param host: A host object representing the DUT.
        @param property_name: A string holding the property name.

        @return: A string holding the property value.
        """
        return host.run_output('getprop %s' % property_name).strip()


    def run_once(self, host=None):
        """Runs the test.

        @param host: A host object representing the DUT.

        @raise TestFail: The test executable returned an error.
        """
        try:
            if self._get_prop(host, 'ro.hardware.nvram') == 'testing':
                raise error.TestFail('NVRAM HAL is not hardware-backed')
            host.run(self.TEST_EXECUTABLE)
        except error.GenericHostRunError as run_error:
            raise error.TestFail(run_error)
