# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import os
import pprint
import shutil
import subprocess
import sys
import time
from autotest_lib.client.bin import test, utils
from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib.cros import chrome
from autotest_lib.client.cros import constants, cros_logging

# The name of the Chrome OS Pepper Flash binary.
_BINARY = 'libpepflashplayer.so'
# The path to the system provided (read only) Flash binary.
_SYSTEM_STORE = '/opt/google/chrome/pepper'
# The name of the file containing metainformation for the system binary.
_FLASH_INFO = 'pepper-flash.info'
# The name of the component updated manifest describing version, OS,
# architecture and required ppapi interfaces.
_MANIFEST = 'manifest.json'
# The tmp location Chrome downloads the bits from Omaha to.
_DOWNLOAD_STORE = '/home/chronos/PepperFlash'
# The location the CrOS component updater stores new images in.
_COMPONENT_STORE = '/var/lib/imageloader/PepperFlashPlayer'
# latest-version gets updated after the library in the store. We use it to
# check for completion of download.
_COMPONENT_STORE_LATEST = _COMPONENT_STORE + '/latest-version'
# The location at which the latest component updated Flash binary is mounted
# for execution.
_COMPONENT_MOUNT = '/run/imageloader/PepperFlashPlayer'
# Set of all possible paths at which Flash binary could be found.
_FLASH_PATHS = {
    _SYSTEM_STORE, _DOWNLOAD_STORE, _COMPONENT_STORE, _COMPONENT_MOUNT}

# Run the traditional Flash sanity check (just check that any Flash works).
_CU_ACTION_SANITY = 'sanity'
# Clean out all component update state (in preparation to next update).
_CU_ACTION_DELETE = 'delete-component'
# TODO(ihf): Implement this action to simulated component on component update.
_CU_ACTION_INSTALL_OLD = 'install-old-component'
# Download the latest available component from Omaha.
_CU_ACTION_DOWNLOAD = 'download-omaha-component'
# Using current state of DUT verify the Flash in _COMPONENT_MOUNT.
_CU_ACTION_VERIFY_COMPONENT = 'verify-component-flash'
# Using current state of DUT verify the Flash shipping with the system image.
_CU_ACTION_VERIFY_SYSTEM = 'verify-system-flash'


class desktopui_FlashSanityCheck(test.test):
    """
    Sanity test that ensures flash instance is launched when a swf is played.
    """
    version = 4

    _messages_log_reader = None
    _ui_log_reader = None
    _test_url = None
    _testServer = None
    _time_to_wait_secs = 5
    _swf_runtime = 5
    _retries = 10
    _component_download_timeout_secs = 300

    def verify_file(self, name):
        """
        Does sanity checks on a file on disk.

        @param name: filename to verify.
        """
        if not os.path.exists(name):
            raise error.TestFail('Failed: File does not exist %s' % name)
        if not os.path.isfile(name):
            raise error.TestFail('Failed: Not a file %s' % name)
        if os.path.getsize(name) <= 0:
            raise error.TestFail('Failed: File is too short %s' % name)
        if name.endswith('libpepflashplayer.so'):
            output = subprocess.check_output(['file %s' % name], shell=True)
            if not 'stripped' in output:
                logging.error(output)
                raise error.TestFail('Failed: Flash binary not stripped.')
            if not 'dynamically linked' in output:
                logging.error(output)
                raise error.TestFail('Failed: Flash not dynamically linked.')
            arch = utils.get_arch_userspace()
            logging.info('get_arch_userspace = %s', arch)
            if arch == 'arm' and not 'ARM' in output:
                logging.error(output)
                raise error.TestFail('Failed: Flash binary not for ARM.')
            if arch == 'x86_64' and not 'x86-64' in output:
                logging.error(output)
                raise error.TestFail('Failed: Flash binary not for x86_64.')
            if arch == 'i386' and not '80386' in output:
                logging.error(output)
                raise error.TestFail('Failed: Flash binary not for i386.')
        logging.info('Verified file %s', name)

    def serve_swf_to_browser(self, browser):
        """
        Tries to serve a sample swf to browser.

        A failure of this function does not imply a problem with Flash.
        @param browser: The Browser object to run the test with.
        @return: True if we managed to send swf to browser, False otherwise.
        """
        # Prepare index.html/Trivial.swf to be served.
        browser.platform.SetHTTPServerDirectories(self.bindir)
        test_url = browser.platform.http_server.UrlOf(os.path.join(self.bindir,
                                                      'index.html'))
        tab = None
        # BUG(485108): Work around a telemetry timing out after login.
        try:
            logging.info('Getting tab from telemetry...')
            tab = browser.tabs[0]
        except:
            logging.warning('Unexpected exception getting tab: %s',
                            pprint.pformat(sys.exc_info()[0]))
        if tab is None:
            return False

        logging.info('Initialize reading system logs.')
        self._messages_log_reader = cros_logging.LogReader()
        self._messages_log_reader.set_start_by_current()
        self._ui_log_reader = cros_logging.LogReader('/var/log/ui/ui.LATEST')
        self._ui_log_reader.set_start_by_current()
        logging.info('Done initializing system logs.')

        # Verify that the swf got pulled.
        try:
            tab.Navigate(test_url)
            tab.WaitForDocumentReadyStateToBeComplete()
            return True
        except:
            logging.warning('Unexpected exception waiting for document: %s',
                            pprint.pformat(sys.exc_info()[0]))
            return False


    def verify_flash_process(self, load_path=None):
        """Verifies the Flash process runs and doesn't crash.

        @param load_path: The expected path of the Flash binary. If set
                          function and Flash was loaded from a different path,
                          function will fail the test.
        """
        logging.info('Waiting for Pepper process.')
        # Verify that we see a ppapi process and assume it is Flash.
        ppapi = utils.wait_for_value_changed(
            lambda: (utils.get_process_list('chrome', '--type=ppapi')),
            old_value=[],
            timeout_sec=self._time_to_wait_secs)
        logging.info('ppapi process list at start: %s', ', '.join(ppapi))
        if not ppapi:
            msg = 'flash/platform/pepper/pep_'
            if not self._ui_log_reader.can_find(msg):
                raise error.TestFail(
                    'Failed: Flash did not start (logs) and no ppapi process '
                    'found.'
                )
            # There is a chrome bug where the command line of the ppapi and
            # other processes is shown as "type=zygote". Bail out if we see more
            # than 2. Notice, we already did the waiting, so there is no need to
            # do more of it.
            zygote = utils.get_process_list('chrome', '--type=zygote')
            if len(zygote) > 2:
                logging.warning('Flash probably launched by Chrome as zygote: '
                                '<%s>.', ', '.join(zygote))

        # We have a ppapi process. Let it run for a little and see if it is
        # still alive.
        logging.info('Running Flash content for a little while.')
        time.sleep(self._swf_runtime)
        logging.info('Verifying the Pepper process is still around.')
        ppapi = utils.wait_for_value_changed(
            lambda: (utils.get_process_list('chrome', '--type=ppapi')),
            old_value=[],
            timeout_sec=self._time_to_wait_secs)
        # Notice that we are not checking for equality of ppapi on purpose.
        logging.info('PPapi process list found: <%s>', ', '.join(ppapi))

        # Any better pattern matching?
        msg = ' Received crash notification for ' + constants.BROWSER
        if self._messages_log_reader.can_find(msg):
            raise error.TestFail('Failed: Browser crashed during test.')
        if not ppapi:
            raise error.TestFail(
                'Failed: Pepper process disappeared during test.')

        # At a minimum Flash identifies itself during process start.
        msg = 'flash/platform/pepper/pep_'
        if not self._ui_log_reader.can_find(msg):
            raise error.TestFail(
                'Failed: Saw ppapi process but no Flash output.')

        # Check that libpepflashplayer.so was loaded from the expected path.
        if load_path:
            # Check all current process for Flash library.
            output = subprocess.check_output(
                ['grep libpepflashplayer.so /proc/*/maps'], shell=True)
            # Verify there was no other than the expected location.
            for dont_load_path in _FLASH_PATHS - {load_path}:
                if dont_load_path in output:
                    logging.error('Flash incorrectly loaded from %s',
                                  dont_load_path)
                    logging.info(output)
                    raise error.TestFail('Failed: Flash incorrectly loaded '
                                         'from %s' % dont_load_path)
                logging.info('Verified Flash was indeed not loaded from %s',
                             dont_load_path)
            # Verify at least one of the libraries came from where we expected.
            if not load_path in output:
                # Mystery. We saw a Flash loaded from who knows where.
                logging.error('Flash not loaded from %s', load_path)
                logging.info(output)
                raise error.TestFail('Failed: Flash not loaded from %s' %
                                     load_path)
            logging.info('Saw a flash library loaded from %s.', load_path)


    def action_delete_component(self):
        """
        Deletes all components on the DUT. Notice _COMPONENT_MOUNT cannot be
        deleted. It will remain until after reboot of the DUT.
        """
        if os.path.exists(_COMPONENT_STORE):
            shutil.rmtree(_COMPONENT_STORE)
            if os.path.exists(_COMPONENT_STORE):
                raise error.TestFail('Error: could not delete %s',
                                     _COMPONENT_STORE)
        if os.path.exists(_DOWNLOAD_STORE):
            shutil.rmtree(_DOWNLOAD_STORE)
            if os.path.exists(_DOWNLOAD_STORE):
                raise error.TestFail('Error: could not delete %s',
                                     _DOWNLOAD_STORE)

    def action_download_omaha_component(self):
        """
        Pretend we have no system Flash binary and tell browser to
        accelerate the component update process.
        TODO(ihf): Is this better than pretending the system binary is old?
        """
        # TODO(ihf): Find ways to test component updates on top of component
        # updates maybe by checking hashlib.md5(open(_COMPONENT_STORE_LATEST)).
        if os.path.exists(_COMPONENT_STORE):
            raise error.TestFail('Error: currently unable to test component '
                                 'update as component store not clean before '
                                 'download.')
        # TODO(ihf): Remove --component-updater=test-request once Finch is set
        # up to behave more like a user in the field.
        browser_args = ['--ppapi-flash-path=',
                        '--ppapi-flash-version=0.0.0.0',
                        '--component-updater=fast-update,test-request']
        logging.info(browser_args)
        # Browser will download component, but it will require a subsequent
        # reboot by the caller to use it. (Browser restart is not enough.)
        with chrome.Chrome(extra_browser_args=browser_args,
                           init_network_controller=True) as cr:
            self.serve_swf_to_browser(cr.browser)
            # Wait for the last file to be written by component updater.
            utils.wait_for_value_changed(
                lambda: (os.path.exists(_COMPONENT_STORE_LATEST)),
                False,
                timeout_sec=self._component_download_timeout_secs)
            if not os.path.exists(_COMPONENT_STORE):
                raise error.TestFail('Failed: after download no component at '
                                     '%s' % _COMPONENT_STORE)
            # This may look silly but we prefer giving the system a bit more
            # time to write files to disk before subsequent reboot.
            os.system('sync')
            time.sleep(10)

    def action_install_old_component(self):
        """
        Puts an old/mock manifest and Flash binary into _COMPONENT_STORE.
        """
        # TODO(ihf): Implement. Problem is, mock component binaries need to be
        # signed by Omaha. But if we had this we could test component updating
        # a component update.
        pass

    def action_verify_component_flash(self):
        """
        Verifies that the next use of Flash is from _COMPONENT_MOUNT.
        """
        # Verify there is already a binary in the component store.
        self.verify_file(_COMPONENT_STORE_LATEST)
        # Verify that binary was mounted during boot.
        self.verify_file(os.path.join(_COMPONENT_MOUNT, 'libpepflashplayer.so'))
        self.verify_file(os.path.join(_COMPONENT_MOUNT, 'manifest.json'))
        # Pretend we have a really old Flash revision on system to force using
        # the downloaded component.
        browser_args = ['--ppapi-flash-version=1.0.0.0']
        # Verify that Flash runs from _COMPONENT_MOUNT.
        self.run_flash_test(
            browser_args=browser_args, load_path=_COMPONENT_MOUNT)

    def action_verify_system_flash(self):
        """
        Verifies that next use of Flash is from the _SYSTEM_STORE.
        """
        # Verify there is a binary in the system store.
        self.verify_file(os.path.join(_SYSTEM_STORE, _BINARY))
        # Enable component updates and pretend we have a really new Flash
        # version on the system image.
        browser_args = ['--ppapi-flash-version=9999.0.0.0']
        # Verify that Flash runs from _SYSTEM_STORE.
        self.run_flash_test(browser_args=browser_args, load_path=_SYSTEM_STORE)

    def run_flash_test(self, browser_args=None, load_path=None):
        """
        Verifies that directing the browser to an swf file results in a running
        Pepper Flash process which does not immediately crash.

        @param browser_args: additional browser args.
        @param load_path: flash load path.
        """
        if not browser_args:
            browser_args = []
        # This is Flash. Disable html5 by default feature.
        browser_args += ['--disable-features=PreferHtmlOverPlugins']
        # As this is an end to end test with nontrivial setup we can expect a
        # certain amount of flakes which are *unrelated* to running Flash. We
        # try to hide these unrelated flakes by selective retry.
        for _ in range(0, self._retries):
            logging.info(browser_args)
            with chrome.Chrome(extra_browser_args=browser_args,
                               init_network_controller=True) as cr:
                if self.serve_swf_to_browser(cr.browser):
                    self.verify_flash_process(load_path)
                    return
        raise error.TestFail(
            'Error: Unable to test Flash due to setup problems.')

    def run_once(self, CU_action=_CU_ACTION_SANITY):
        """
        Main entry point for desktopui_FlashSanityCheck.

        Performs an action as specified by control file or
        by the component_UpdateFlash server test. (The current need to reboot
        after switching to/from component binary makes this test a server test.)

        @param CU_action: component updater action to verify (typically called
                          from server test).
        """
        logging.info('+++++ desktopui_FlashSanityCheck +++++')
        logging.info('Performing %s', CU_action)
        if CU_action == _CU_ACTION_DELETE:
            self.action_delete_component()
        elif CU_action == _CU_ACTION_DOWNLOAD:
            self.action_download_omaha_component()
        elif CU_action == _CU_ACTION_INSTALL_OLD:
            self.action_install_old_component()
        elif CU_action == _CU_ACTION_SANITY:
            self.run_flash_test()
        elif CU_action == _CU_ACTION_VERIFY_COMPONENT:
            self.action_verify_component_flash()
        elif CU_action == _CU_ACTION_VERIFY_SYSTEM:
            self.action_verify_system_flash()
        else:
            raise error.TestError('Error: unknown action %s', CU_action)
