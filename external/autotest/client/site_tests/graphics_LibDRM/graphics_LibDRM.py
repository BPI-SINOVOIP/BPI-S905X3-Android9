# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
import logging

from autotest_lib.client.bin import test, utils
from autotest_lib.client.common_lib import error
from autotest_lib.client.cros import service_stopper
from autotest_lib.client.cros.graphics import graphics_utils

class graphics_LibDRM(graphics_utils.GraphicsTest):
    version = 1
    _services = None

    def initialize(self):
        super(graphics_LibDRM, self).initialize()
        self._services = service_stopper.ServiceStopper(['ui'])

    def cleanup(self):
        super(graphics_LibDRM, self).cleanup()
        if self._services:
            self._services.restore_services()

    def run_once(self):
        keyvals = {}

        # These are tests to run for all platforms.
        tests_common = ['modetest']

        # Determine which tests to run based on the architecture type.
        tests_exynos5 = ['kmstest']
        tests_mediatek = ['kmstest']
        tests_rockchip = ['kmstest']
        arch_tests = {
            'amd': [],
            'arm': [],
            'exynos5': tests_exynos5,
            'i386': [],
            'mediatek': tests_mediatek,
            'rockchip': tests_rockchip,
            'tegra': [],
            'x86_64': []
        }
        soc = utils.get_cpu_soc_family()
        if not soc in arch_tests:
            raise error.TestFail('Error: Architecture "%s" not supported.',
                                 soc)
        elif soc == 'tegra':
            logging.warning('Tegra does not support DRM.')
            return
        tests = tests_common + arch_tests[soc]

        # If UI is running, we must stop it and restore later.
        self._services.stop_services()

        for test in tests:
            self.add_failures(test)
            # Make sure the test exists on this system.  Not all tests may be
            # present on a given system.
            if utils.system('which %s' % test):
                logging.error('Could not find test %s.', test)
                keyvals[test] = 'NOT FOUND'
                continue

            # Run the test and check for success based on return value.
            return_value = utils.system(test)
            if return_value:
                logging.error('%s returned %d', test, return_value)
                keyvals[test] = 'FAILED'
            else:
                keyvals[test] = 'PASSED'
                self.remove_failures(test)

        self.write_perf_keyval(keyvals)
        if self.get_failures():
            raise error.TestFail('Failed: %d libdrm tests failed.'
                                 % len(self.get_failures()))
