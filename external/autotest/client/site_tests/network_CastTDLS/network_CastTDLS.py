# Copyright 2016 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import json, logging, os, re, tempfile, time, urllib2, zipfile
from autotest_lib.client.bin import test
from autotest_lib.client.common_lib import error, file_utils
from autotest_lib.client.common_lib.cros import chromedriver


UPDATE_CHECK_URL = ('https://clients2.google.com/service/update2/'
                    'crx?x=id%%3D%s%%26v%%3D0%%26uc&prodversion=32.0.0.0')
EXTENSION_ID_BETA = 'dliochdbjfkdbacpmhlcpmleaejidimm'
CHROME_EXTENSION_BASE = 'chrome-extension://'
EXTENSION_OPTIONS_PAGE = 'options.html'
RECEIVER_LOG = 'receiver_debug.log'
EXTENSION_TEST_UTILS_PAGE = 'e2e_test_utils.html'
TEST_URL = 'http://www.vimeo.com'
GET_LOG_URL = 'http://%s:8008/setup/get_log_report'
KEY = ('MIGfMA0GCSqGSIb3DQEBAQUAA4GNADCBiQKBgQC+hlN5FB+tjCsBszmBIvIcD/djLLQm2z'
       'ZfFygP4U4/o++ZM91EWtgII10LisoS47qT2TIOg4Un4+G57elZ9PjEIhcJfANqkYrD3t9d'
       'pEzMNr936TLB2u683B5qmbB68Nq1Eel7KVc+F0BqhBondDqhvDvGPEV0vBsbErJFlNH7SQ'
       'IDAQAB')
WAIT_SECONDS = 3
MIRRORING_DURATION_SECONDS = 20


class network_CastTDLS(test.test):
    """Test loading the sonic extension through chromedriver."""
    version = 1

    def _navigate_url(self, driver, url):
        """Go to options page if current page is not options page.

        @param driver: The chromedriver instance.
        @param url: The URL to navigate to.
        """
        if driver.current_url != url:
            driver.get(url)
            driver.refresh()

    def _set_focus_tab(self, driver, tab_handle):
      """Set the focus on a tab.

      @param driver: The chromedriver instance.
      @param tab_handle: The chrome driver handle of the tab.
      """
      driver.switch_to_window(tab_handle)
      driver.get_screenshot_as_base64()

    def _block_setup_dialog(self, driver, extension_id):
        """Tab cast through the extension.

        @param driver: A chromedriver instance that has the extension loaded.
        @param extension_id: Id of the extension to use.
        """
        test_utils_page = '%s%s/%s' % (
                CHROME_EXTENSION_BASE, extension_id, EXTENSION_TEST_UTILS_PAGE)
        self._navigate_url(driver, test_utils_page)
        time.sleep(WAIT_SECONDS)
        driver.execute_script(
            'localStorage["blockChromekeySetupAutoLaunchOnInstall"] = "true"')

    def _close_popup_tabs(self, driver):
        """Close any popup windows the extension might open by default.

        Since we're going to handle the extension ourselves all we need is
        the main browser window with a single tab. The safest way to handle
        the popup however, is to close the currently active tab, so we don't
        mess with chromedrivers ui debugger.

        @param driver: Chromedriver instance.
        @raises Exception If you close the tab associated with the ui debugger.
        """
        current_tab_handle = driver.current_window_handle
        for handle in driver.window_handles:
            if handle != current_tab_handle:
                try:
                    time.sleep(WAIT_SECONDS)
                    driver.switch_to_window(handle)
                    driver.close()
                except:
                    pass
        driver.switch_to_window(current_tab_handle)

    def _download_extension_crx(self, output_file):
        """Download Cast (Beta) extension from Chrome Web Store to a file.

        @param output_file: The output file of the extension.
        """
        update_check_url = UPDATE_CHECK_URL % EXTENSION_ID_BETA
        response = urllib2.urlopen(update_check_url).read()
        logging.info('Response: %s', response)
        pattern = r'codebase="(.*crx)"'
        regex = re.compile(pattern)
        match = regex.search(response)
        extension_download_url = match.groups()[0]
        logging.info('Extension download link: %s', extension_download_url)
        file_utils.download_file(extension_download_url, output_file)

    def _unzip_file(self, zip_file, output_folder):
        """Unzip a zip file to a folder.

        @param zip_file: Path of the zip file.
        @param output_folder: Output file path.
        """
        zip_ref = zipfile.ZipFile(zip_file, 'r')
        zip_ref.extractall(output_folder)
        zip_ref.close()

    def _modify_manifest(self, extension_folder):
        """Update the Cast extension manifest file by adding a key to it.

        @param extension_folder: Path to the unzipped extension folder.
        """
        manifest_file = os.path.join(extension_folder, 'manifest.json')
        with open(manifest_file) as f:
            manifest_dict = json.load(f)
        manifest_dict['key'] = KEY
        with open(manifest_file, 'wb') as f:
            json.dump(manifest_dict, f)

    def _turn_on_tdls(self, driver, extension_id):
        """Turn on TDLS in the extension option page.

        @param driver: The chromedriver instance of the test.
        @param extension_id: The id of the Cast extension.
        """
        turn_on_tdls_url = ('%s%s/%s''?disableTDLS=false') % (
                CHROME_EXTENSION_BASE, extension_id, EXTENSION_OPTIONS_PAGE)
        self._navigate_url(driver, turn_on_tdls_url)
        time.sleep(WAIT_SECONDS)

    def _start_mirroring(self, driver, extension_id, device_ip, url):
        """Use test util page to start mirroring session on specific device.

        @param driver: The chromedriver instance.
        @param extension_id: The id of the Cast extension.
        @param device_ip: The IP of receiver device to launch mirroring.
        @param url: The URL to mirror.
        """
        test_utils_page = '%s%s/%s' % (
                CHROME_EXTENSION_BASE, extension_id, EXTENSION_TEST_UTILS_PAGE)
        self._navigate_url(driver, test_utils_page)
        time.sleep(WAIT_SECONDS)
        tab_handles = driver.window_handles
        driver.find_element_by_id('receiverIpAddressV2').send_keys(device_ip)
        driver.find_element_by_id('urlToOpenV2').send_keys(url)
        time.sleep(WAIT_SECONDS)
        driver.find_element_by_id('mirrorUrlV2').click()
        time.sleep(WAIT_SECONDS)
        all_handles = driver.window_handles
        test_handle = [x for x in all_handles if x not in tab_handles].pop()
        driver.switch_to_window(test_handle)
        driver.refresh()

    def _stop_mirroring(self, driver, extension_id):
      """Use test util page to stop a mirroring session on a specific device.

      @param driver: The chromedriver instance.
      @param extension_id: The id of the Cast extension.
      """
      test_utils_page = '%s%s/%s' % (
            CHROME_EXTENSION_BASE, extension_id, EXTENSION_TEST_UTILS_PAGE)
      self._navigate_url(driver, test_utils_page)
      time.sleep(WAIT_SECONDS)
      driver.find_element_by_id('stopV2Mirroring').click()

    def _check_tdls_status(self, device_ip):
        """Check the TDLS status of a mirroring session using receiver's log.

        @param device_ip: IP of receiver device used in the mirroring session.
        @raises error.TestFail If there is log about TDLS status.
        @raises error.TestFail If TDLS status is invalid.
        @raises error.TestFail TDLS is not being used in the mirroring session.
        """
        response = urllib2.urlopen(GET_LOG_URL % device_ip).read()
        logging.info('Receiver log is under: %s', self.debugdir)
        with open(os.path.join(self.debugdir, RECEIVER_LOG), 'wb') as f:
            f.write(response)
        tdls_status_list = re.findall(
                r'.*TDLS status.*last TDLS status:.*', response)
        if not len(tdls_status_list):
            raise error.TestFail('There is no TDLS log on %s.', device_ip)
        pattern = r'last TDLS status: (.*)'
        regex = re.compile(pattern)
        match = regex.search(tdls_status_list[-1])
        if not len(match.groups()):
            raise error.TestFail('Invalid TDLS status.')
        if match.groups()[0].lower() != 'enabled':
            raise error.TestFail(
                'TDLS is not initiated in the last mirroring session.')

    def initialize(self):
        """Initialize the test.

        @param extension_dir: Directory of a custom extension.
            If one isn't supplied, the latest ToT extension is
            downloaded and loaded into chromedriver.
        @param live: Use a live url if True. Start a test server
            and server a hello world page if False.
        """
        super(network_CastTDLS, self).initialize()
        self._tmp_folder = tempfile.mkdtemp()


    def cleanup(self):
        """Clean up the test environment, e.g., stop local http server."""
        super(network_CastTDLS, self).cleanup()


    def run_once(self, device_ip):
        """Run the test code.

        @param  device_ip: Device IP to use in the test.
        """
        logging.info('Download Cast extension ... ...')
        crx_file = tempfile.NamedTemporaryFile(
                dir=self._tmp_folder, suffix='.crx').name
        unzip_crx_folder = tempfile.mkdtemp(dir=self._tmp_folder)
        self._download_extension_crx(crx_file)
        self._unzip_file(crx_file, unzip_crx_folder)
        self._modify_manifest(unzip_crx_folder)
        kwargs = { 'extension_paths': [unzip_crx_folder] }
        with chromedriver.chromedriver(**kwargs) as chromedriver_instance:
            driver = chromedriver_instance.driver
            extension_id = chromedriver_instance.get_extension(
                    unzip_crx_folder).extension_id
            # Close popup dialog if there is any
            self._close_popup_tabs(driver)
            self._block_setup_dialog(driver, extension_id)
            time.sleep(WAIT_SECONDS)
            logging.info('Enable TDLS in extension: %s', extension_id)
            self._turn_on_tdls(driver, extension_id)
            extension_tab_handle = driver.current_window_handle
            logging.info('Start mirroring on device: %s', device_ip)
            self._start_mirroring(
                    driver, extension_id, device_ip, TEST_URL)
            time.sleep(MIRRORING_DURATION_SECONDS)
            self._set_focus_tab(driver, extension_tab_handle)
            driver.switch_to_window(extension_tab_handle)
            logging.info('Stop mirroring on device: %s', device_ip)
            self._stop_mirroring(driver, extension_id)
            time.sleep(WAIT_SECONDS * 3)
            logging.info('Verify TDLS status in the mirroring session ... ... ')
            self._check_tdls_status(device_ip)
