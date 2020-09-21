# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import dbus
import logging
import os
import tempfile
from threading import Thread

from autotest_lib.client.bin import test, utils
from autotest_lib.client.common_lib import file_utils
from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib.cros import chrome
from autotest_lib.client.common_lib.cros import dbus_send
from autotest_lib.client.cros import debugd_util
from fake_printer import FakePrinter

_FAKE_SERVER_JOIN_TIMEOUT = 10
_FAKE_PRINTER_ID = 'FakePrinterID'

# Values are from platform/system_api/dbus/debugd/dbus-constants.h.
_CUPS_SUCCESS = 0

class platform_AddPrinter(test.test):
    """
    Chrome is brought up, and a cups printer that requires the epson
    driver to be downloaded as a component is configured.  The test verifies
    that the component is downloaded and a subsequent print command works.
    """
    version = 1

    def initialize(self, ppd_file):
        """
        Args:
        @param ppd_file: ppd file name
        """

        # Instantiate Chrome browser.
        with tempfile.NamedTemporaryFile() as cap:
            file_utils.download_file(chrome.CAP_URL, cap.name)
            password = cap.read().rstrip()

        extra_flags = ['--enable-features=CrOSComponent']
        self.browser = chrome.Chrome(gaia_login=False,
                                     username=chrome.CAP_USERNAME,
                                     password=password,
                                     extra_browser_args=extra_flags)
        self.tab = self.browser.browser.tabs[0]

        # Set file path.
        current_dir = os.path.dirname(os.path.realpath(__file__))
        self.pdf_path = os.path.join(current_dir,
                           'to_print.pdf')
        self.printing_log_path = '/tmp/printing_request.log'

        # Download ppd files
        self.ppd_file = '/tmp/%s' % ppd_file
        file_utils.download_file(
            'https://storage.googleapis.com/chromiumos-test-assets-public'
            '/platform_AddPrinter/%s' % ppd_file,
            self.ppd_file);

        # Start fake printer.
        printer = FakePrinter()
        self.server_thread = Thread(target = printer.start,
                               args = (self.printing_log_path, ))
        self.server_thread.start();

    def cleanup(self):
        """
        Delete downloaded ppd file, fake printer log file, component (if
        downloaded);
        """
        # Remove component.
        if hasattr(self, 'component'):
          self.delete_component(self.component)

        # Close browser.
        if hasattr(self, 'browser'):
            self.browser.close()

        # Remove temp files.
        if os.path.exists(self.ppd_file):
          os.remove(self.ppd_file)
        if os.path.exists(self.printing_log_path):
          os.remove(self.printing_log_path)

    def load_ppd(self, ppd_path):
        """
        Returns the contents of a file as a dbus.ByteArray.

        @param file_name: The path of the file.

        """
        with open(ppd_path, 'rb') as f:
            content = dbus.ByteArray(f.read())
            return content

    def add_a_printer(self, ppd_path):
        """
        Add a printer manually given ppd file.

        Args:
        @param ppd_path: path to ppd file

        @raises: error.TestFail if could not setup a printer
        """
        logging.info('add printer from ppd:' + ppd_path);

        ppd_contents = self.load_ppd(ppd_path)
        result = debugd_util.iface().CupsAddManuallyConfiguredPrinter(
                                     _FAKE_PRINTER_ID, 'socket://127.0.0.1/',
                                                      ppd_contents)
        if result != _CUPS_SUCCESS:
            raise error.TestFail('valid_config - Could not setup valid '
                'printer %d' % result)

    def print_a_page(self, golden_file_path):
        """
        Print a page and check print request output

        Args:
        @param golden_file_path: path to printing request golden file.

        @raises: error.TestFail if printing request generated cannot be
        verified.
        """

        # Issue print request.
        utils.system_output(
            'lp -d %s %s' %
            (_FAKE_PRINTER_ID, self.pdf_path)
        );
        self.server_thread.join(_FAKE_SERVER_JOIN_TIMEOUT)

        # Verify print request with a golden file.
        output = utils.system_output(
            'cmp %s %s' % (self.printing_log_path, golden_file_path)
        )
        if output:
            raise error.TestFail('ERROR: Printing request is not verified!')
        logging.info('cmp output:' + output);


    def delete_component(self, component):
        """
        Delete filter component via dbus API

        Args:
        @param component: name of component
        """
        logging.info('delete component:' + component);

        dbus_send.dbus_send(
            'org.chromium.ComponentUpdaterService',
            'org.chromium.ComponentUpdaterService',
            '/org/chromium/ComponentUpdaterService',
            'UnloadComponent',
            timeout_seconds=20,
            user='root',
            args=[dbus.String(component)])

    def download_component(self, component):
        """
        Download filter component via dbus API

        Args:
        @param component: name of component

        @raises: error.TestFail is component is not loaded.
        """
        logging.info('download component:' + component);

        res = dbus_send.dbus_send(
            'org.chromium.ComponentUpdaterService',
            'org.chromium.ComponentUpdaterService',
            '/org/chromium/ComponentUpdaterService',
            'LoadComponent',
            timeout_seconds=20,
            user='root',
            args=[dbus.String(component)])

        if res.response == '':
          raise error.TestFail('Component could not be loaded.')

    def run_once(self, golden_file, component=None):
        """
        Args:
        @param golden_file: printing request golden file name
        """
        if component:
            self.component = component
            self.download_component(self.component)

        current_dir = os.path.dirname(os.path.realpath(__file__))
        self.add_a_printer(self.ppd_file)
        self.print_a_page(os.path.join(current_dir, golden_file));
