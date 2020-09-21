# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import dbus

from autotest_lib.client.bin import test
from autotest_lib.client.common_lib import error
from autotest_lib.client.cros import cups
from autotest_lib.client.cros import debugd_util

_GENERIC_PPD = 'GenericPostScript.ppd.gz'

# Values are from platform/system_api/dbus/debugd/dbus-constants.h.
_CUPS_SUCCESS = 0
_CUPS_INVALID_PPD_ERROR = 2
_CUPS_LPADMIN_ERROR = 3
_CUPS_AUTOCONF_FAILURE = 4


class platform_DebugDaemonCupsAddPrinters(test.test):
    """
    Exercise CupsAddManuallyConfiguredPrinter from debugd.

    Exercise the various add printer conditions and verify that the
    error codes are correct.

    """
    version = 1

    def load_ppd(self, file_name):
        """
        Returns the contents of a file as a dbus.ByteArray.

        @param file_name: The name of the file.

        """
        abs_path = '%s/%s' % (self.srcdir, file_name)
        with open(abs_path, 'rb') as f:
            content = dbus.ByteArray(f.read())
        return content

    def test_autoconf(self):
        """
        Attempt to add an unreachable autoconfigured printer.

        Verifies that upon autoconf failure, the error code is
        CUPS_AUTOCONF_FAILURE.

        @raises TestFail: If the test failed.

        """
        autoconfig_result = debugd_util.iface().CupsAddAutoConfiguredPrinter(
                            'AutoconfPrinter', 'ipp://127.0.0.1/ipp/print')
        # There's no printer at this address.  Autoconf failure expected.
        # CUPS_AUTOCONF_FAILURE.
        if autoconfig_result != _CUPS_AUTOCONF_FAILURE:
            raise error.TestFail('autoconf - Incorrect error code received: '
                '%i' % autoconfig_result)

    def test_ppd_error(self):
        """
        Validates that malformed PPDs are rejected.

        The expected error code is CUPS_INVALID_PPD error.

        @raises TestFail: If the test failed.

        """
        ppd_contents = dbus.ByteArray('This is not a valid ppd')
        result = debugd_util.iface().CupsAddManuallyConfiguredPrinter(
                'ManualPrinterBreaks', 'socket://127.0.0.1/ipp/fake_printer',
                ppd_contents)
        # PPD is invalid.  Expect a CUPS_INVALID_PPD error.
        if result != _CUPS_INVALID_PPD_ERROR:
            raise error.TestFail('ppd_error - Incorrect error code received '
                '%d' % result)

    def test_valid_config(self):
        """
        Validates that a printer can be installed.

        Verifies that given a valid configuration and a well formed PPD,
        DebugDaemon reports a CUPS_SUCCESS error code indicating
        success.

        @raises TestFail: If the result from debugd was not CUPS_SUCCESS.

        """
        ppd_contents = self.load_ppd(_GENERIC_PPD)
        result = debugd_util.iface().CupsAddManuallyConfiguredPrinter(
                 'ManualPrinterGood', 'socket://127.0.0.1/ipp/fake_printer',
                 ppd_contents)
        # PPD is valid.  Printer doesn't need to be reachable.  This is
        # expected to pass with CUPS_SUCCESS.
        if result != _CUPS_SUCCESS:
            raise error.TestFail('valid_config - Could not setup valid '
                'printer %d' % result)

    def test_lpadmin(self):
        """
        Verify the error for a failure in lpadmin.

        The failure is reported as CUPS_LPADMIN_FAILURE.

        @raises TestFail: If the error code from debugd is incorrect.

        """
        ppd_contents = self.load_ppd(_GENERIC_PPD)
        result = debugd_util.iface().CupsAddManuallyConfiguredPrinter(
                 'CUPS rejects names with spaces',
                 'socket://127.0.0.1/ipp/fake_printer',
                 ppd_contents)
        if result != _CUPS_LPADMIN_ERROR:
            raise error.TestFail(
                'lpadmin - Names with spaces should be rejected by CUPS '
                '%d' % result)

        result = debugd_util.iface().CupsAddManuallyConfiguredPrinter(
                 'UnrecognizedProtocol',
                 'badbadbad://127.0.0.1/ipp/fake_printer',
                  ppd_contents)
        if result != _CUPS_LPADMIN_ERROR:
            raise error.TestFail(
                  'lpadmin - Unrecognized protocols should be rejected by '
                  'CUPS. %d' %
                  result)

    def run_once(self):
        """
        Runs tests based on the designated situation.

        @raises TestError: If an unrecognized situation was used.

        """
        # Exits test if platform does not have CUPS
        cups.has_cups_or_die()

        self.test_valid_config()
        self.test_lpadmin()
        self.test_ppd_error()
        self.test_autoconf()
