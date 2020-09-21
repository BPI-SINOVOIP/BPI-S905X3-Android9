# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
import os
import unittest

import tradefed_test


def _load_data(filename):
    """Loads the test data of the given file name."""
    with open(os.path.join(os.path.dirname(os.path.realpath(__file__)),
              'tradefed_test_unittest_data', filename), 'r') as f:
        return f.read()


class TradefedTestTest(unittest.TestCase):
    """Unittest for tradefed_test."""

    def test_parse_tradefed_result(self):
        """Test for parse_tradefed_result."""

        waivers = set([
            'android.app.cts.SystemFeaturesTest#testUsbAccessory',
            'android.widget.cts.GridViewTest#testSetNumColumns',
        ])

        # b/35605415 and b/36520623
        # http://pantheon/storage/browser/chromeos-autotest-results/108103986-chromeos-test/
        # CTS: Tradefed may split a module to multiple chunks.
        # Besides, the module name may not end with "TestCases".
        self.assertEquals((35, 33, 2, 0, 0),
            tradefed_test.parse_tradefed_result(
                _load_data('CtsHostsideNetworkTests.txt'),
                waivers=waivers))

        # b/35530394
        # http://pantheon/storage/browser/chromeos-autotest-results/108291418-chromeos-test/
        # Crashed, but the automatic retry by tradefed executed the rest.
        self.assertEquals((1395, 1386, 9, 0, 0),
            tradefed_test.parse_tradefed_result(
                _load_data('CtsMediaTestCases.txt'),
                waivers=waivers))

        # b/35530394
        # http://pantheon/storage/browser/chromeos-autotest-results/106540705-chromeos-test/
        # Crashed in the middle, and the device didn't came back.
        self.assertEquals((110, 27, 1, 82, 0),
            tradefed_test.parse_tradefed_result(
                _load_data('CtsSecurityTestCases.txt'),
                waivers=waivers))

        # b/36629187
        # http://pantheon/storage/browser/chromeos-autotest-results/108855595-chromeos-test/
        # Crashed in the middle. Tradefed decided not to continue.
        self.assertEquals((739, 573, 3, 163, 0),
            tradefed_test.parse_tradefed_result(
                _load_data('CtsViewTestCases.txt'),
                waivers=waivers))

        # b/36375690
        # http://pantheon/storage/browser/chromeos-autotest-results/109040174-chromeos-test/
        # Mixture of real failures and waivers.
        self.assertEquals((321, 316, 5, 0, 1),
            tradefed_test.parse_tradefed_result(
                _load_data('CtsAppTestCases.txt'),
                waivers=waivers))
        # ... and the retry of the above failing iteration.
        self.assertEquals((5, 1, 4, 0, 1),
            tradefed_test.parse_tradefed_result(
                _load_data('CtsAppTestCases-retry.txt'),
                waivers=waivers))

        # http://pantheon/storage/browser/chromeos-autotest-results/116875512-chromeos-test/
        # When a test case crashed during teardown, tradefed prints the "fail"
        # message twice. Tolerate it and still return an (inconsistent) count.
        self.assertEquals((1194, 1185, 10, 0, 2),
            tradefed_test.parse_tradefed_result(
                _load_data('CtsWidgetTestCases.txt'),
                waivers=waivers))

        # http://pantheon/storage/browser/chromeos-autotest-results/117914707-chromeos-test/
        # When a test case unrecoverably crashed during teardown, tradefed
        # prints the "fail" and failure summary message twice. Tolerate it.
        self.assertEquals((71, 70, 1, 0, 0),
            tradefed_test.parse_tradefed_result(
                _load_data('CtsPrintTestCases.txt'),
                waivers=waivers))

        gts_waivers = set([
            ('com.google.android.placement.gts.CoreGmsAppsTest#' +
                'testCoreGmsAppsPreloaded'),
            ('com.google.android.placement.gts.CoreGmsAppsTest#' +
                'testGoogleDuoPreloaded'),
            'com.google.android.placement.gts.UiPlacementTest#testPlayStore'
        ])

        # crbug.com/748116
        # http://pantheon/storage/browser/chromeos-autotest-results/130080763-chromeos-test/
        # 3 ABIS: x86, x86_64, and armeabi-v7a
        self.assertEquals((15, 6, 9, 0, 9),
            tradefed_test.parse_tradefed_result(
                _load_data('GtsPlacementTestCases.txt'),
                waivers=gts_waivers))

        # b/64095702
        # http://pantheon/storage/browser/chromeos-autotest-results/130211812-chromeos-test/
        # The result of the last chunk not reported by tradefed.
        # The actual dEQP log is too big, hence the test data here is trimmed.
        self.assertEquals((157871, 116916, 0, 40955, 0),
            tradefed_test.parse_tradefed_result(
                _load_data('CtsDeqpTestCases-trimmed.txt'),
                waivers=waivers))

if __name__ == '__main__':
    unittest.main()
