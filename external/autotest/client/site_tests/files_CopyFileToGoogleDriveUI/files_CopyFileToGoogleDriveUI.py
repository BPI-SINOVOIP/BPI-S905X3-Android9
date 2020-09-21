# Copyright 2018 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import os
import time

from commands import *

from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib.cros import chromedriver
from selenium.webdriver.common.keys import Keys
from autotest_lib.client.cros.graphics import graphics_utils
from selenium.webdriver.common.action_chains import ActionChains
from selenium.common.exceptions import WebDriverException

TIMEOUT_TO_COPY = 1800  # in Secs. This timeout is for files beyond 1GB
SEARCH_BUTTON_ID = "search-button"
SEARCH_BOX_CSS = "div#search-box"
PAPER_CONTAINTER = "paper-input-container"
DELETE_BUTTON_ID = "delete-button"
FILE_LIST_ID = "file-list"
LABLE_ENTRY_CSS = "span.label.entry-name"
CR_DIALOG_CLASS = "cr-dialog-ok"
USER_LOCATION = "/home/chronos/user"
# Using graphics_utils to simulate below keys
OPEN_FILES_APPLICATION_KEYS = ["KEY_RIGHTSHIFT", "KEY_LEFTALT", "KEY_M"]
SWITCH_TO_APP_KEY_COMBINATION = ["KEY_LEFTALT", 'KEY_TAB']
SELECT_ALL_KEY_COMBINATION = ["KEY_LEFTCTRL", "KEY_A"]
PASTE_KEY_COMBINATION = ["KEY_LEFTCTRL", "KEY_V"]
GOOGLE_DRIVE = 'My Drive'


class files_CopyFileToGoogleDriveUI(graphics_utils.GraphicsTest):

    """Copy a file from Downloads folder to Google drive"""

    version = 1
    TIME_DELAY = 5
    _WAIT_TO_LOAD = 5

    def initialize(self):
        """Autotest initialize function"""
        super(files_CopyFileToGoogleDriveUI, self).initialize(
                raise_error_on_hang=True)

    def cleanup(self):
        """Autotest cleanup function"""
        if self._GSC:
            keyvals = self._GSC.get_memory_difference_keyvals()
            for key, val in keyvals.iteritems():
                self.output_perf_value(
                    description=key,
                    value=val,
                    units='bytes',
                    higher_is_better=False)
            self.write_perf_keyval(keyvals)
        super(files_CopyFileToGoogleDriveUI, self).cleanup()
        # If test fails then script will collect the screen shot to know at
        # which instance failure occurred.
        if not self.success:
            graphics_utils.take_screenshot(os.path.join(self.debugdir),
                                           "chrome")

    def switch_to_app(self, driver, title):
        """Switching to application using title

        @param driver: chrome driver object
        @param title: Title of the application
        @return: True if the app is detected otherwise False
        """
        windows = driver.window_handles
        logging.debug("Windows opened: %s", windows)
        # Checking current window initially..
        logging.debug("Current window is %s", driver.title)
        if driver.title.strip().lower() == title.lower():
            return True
        # Switching to all opened windows to find out the required window
        for window in windows:
            try:
                logging.debug("Switching to window")
                driver.switch_to_window(window)
                logging.debug("Switched to window: %s", driver.title)
                time.sleep(2)
                if driver.title.strip().lower() == title.lower():
                    logging.info("%s application opened!", title)
                    return True
            except WebDriverException as we:
                logging.debug("Webdriver exception occurred. Exception: %s",
                              str(we))
            except Exception as e:
                logging.debug("Exception: %s", str(e))
        return False

    def open_files_application(self, driver):
        """Open and switch to files application using graphics_utils.py

        @param driver: chrome driver object
        """
        logging.info("Opening files application")
        graphics_utils.press_keys(OPEN_FILES_APPLICATION_KEYS)
        time.sleep(self._WAIT_TO_LOAD)
        try:
            self.switch_to_files(driver)
        except Exception as e:
            logging.error("Exception when switching files application.. %s",
                          str(e))
            logging.error("Failed to find files application. Trying again.")
            graphics_utils.press_keys(OPEN_FILES_APPLICATION_KEYS)
            time.sleep(self._WAIT_TO_LOAD)
            self.switch_to_files(driver)

    def switch_to_files(self, driver, title="Downloads"):
        """Switch to files application

        @param driver: chrome driver object
        @param title: Title of the Files application
        """
        logging.debug("Switching/Focus on the Files app")
        if self.switch_to_app(driver, title):
            logging.info("Focused on Files application")
            graphics_utils.press_keys(SWITCH_TO_APP_KEY_COMBINATION)
            time.sleep(1)
        else:
            raise error.TestFail("Failed to open on Files application")

    def check_folder_opened(self, driver, title):
        """Check the selected folder is opened or not

        @param driver: chrome driver object
        @param title: Folder name
        @return: Returns True if expected folder is opened otherwise False
        """
        logging.info("Actual files application title is %s", driver.title)
        logging.info("Expected files application title is %s", title)
        if driver.title == title:
            return True
        return False

    def open_folder(self, driver, folder):
        """Open given folder

        @param driver: chrome driver object
        @param folder: Directory name
        """
        folder_webelements = driver.find_elements_by_css_selector(
            LABLE_ENTRY_CSS)
        for element in folder_webelements:
            try:
                logging.debug("Found folder name: %s", element.text.strip())
                if folder == element.text.strip():
                    element.click()
                    time.sleep(3)
                    if self.check_folder_opened(driver, element.text.strip()):
                        logging.info("Folder is opened!")
                        return
            except Exception as e:
                logging.error("Exception when getting Files application "
                              "folders %s", str(e))
        raise error.TestError("Folder :%s is not opened or found", folder)

    def list_files(self, driver):
        """List files in the folder

        @param driver: chrome driver object
        @return: Returns list of files
        """
        return driver.find_element_by_id(
            FILE_LIST_ID).find_elements_by_tag_name('li')

    def search_file(self, driver, file_name):
        """Search given file in Files application

        @param driver: chrome driver object
        @param file_name: Required file
        """
        driver.find_element_by_id(SEARCH_BUTTON_ID).click()
        search_box_element = driver.find_element_by_css_selector(
            SEARCH_BOX_CSS)
        search_box_element.find_element_by_css_selector(
            PAPER_CONTAINTER).find_element_by_tag_name('input').clear()
        search_box_element.find_element_by_css_selector(
            PAPER_CONTAINTER).find_element_by_tag_name('input').send_keys(
            file_name)

    def copy_file(self, driver, source, destination, file_name, clean=True):
        """Copy file from one directory to another

        @param driver: chrome driver object
        @param source: Directory name from where to copy
        @param destination: Directory name to where to copy
        @param file_name: File to copy
        @param clean: Cleans destination if True otherwise nothing
        """
        self.open_folder(driver, source)
        self.search_file(driver, file_name)
        files = self.list_files(driver)
        action_chains = ActionChains(driver)

        for item in files:
            logging.info("Selecting file to copy in %s", file_name)
            item.click()
            file_size = item.text.split()[1].strip()
            file_size_units = item.text.split()[2].strip()
            logging.debug("Select copy")
            action_chains.move_to_element(item) \
                .click(item).key_down(Keys.CONTROL) \
                .send_keys("c") \
                .key_up(Keys.CONTROL) \
                .perform()
            self.open_folder(driver, destination)
            if clean:
                drive_files = self.list_files(driver)
                if len(drive_files) != 0:
                    logging.info("Removing existing files from %s",
                                 destination)
                    drive_files[0].click()
                    logging.debug("Select all files/dirs")
                    graphics_utils.press_keys(SELECT_ALL_KEY_COMBINATION)
                    time.sleep(0.2)
                    driver.find_element_by_id(DELETE_BUTTON_ID).click()
                    driver.find_element_by_class_name(CR_DIALOG_CLASS).click()
                    time.sleep(self.TIME_DELAY)
            logging.debug("Pressing control+v to paste the file in required "
                          "location")
            graphics_utils.press_keys(PASTE_KEY_COMBINATION)
            time.sleep(self.TIME_DELAY)
            # Take dummy values initially
            required_file_size = "0"
            required_file_size_units = "KB"
            required_file = None
            # wait till the data copied
            start_time = time.time()
            while required_file_size != file_size and \
                required_file_size_units != file_size_units and \
                    (time.time() - start_time <= TIMEOUT_TO_COPY):
                drive_files_during_copy = self.list_files(driver)
                if len(drive_files_during_copy) == 0:
                    raise error.TestError("File copy not started!")
                for i_item in drive_files_during_copy:
                    if i_item.text.strip().split()[0].strip() == file_name:
                        logging.info("File found %s", i_item.text.split()[
                            0].strip())
                        required_file = file
                if not required_file:
                    raise error.TestError("No such file/directory in drive, "
                                          "%s", required_file)
                logging.info(required_file.text.split())
                required_file_size = required_file.text.split()[1]
                required_file_size_units = required_file.text.split()[2]
                time.sleep(5)
                logging.debug("%s %s data copied" % (required_file_size,
                                                     required_file_size_units))
            # Validation starts here
            found = False
            drive_files_after_copy = self.list_files(driver)
            for copied_file in drive_files_after_copy:
                logging.debug("File in destination: %s",
                              copied_file.text.strip())
                if copied_file.find_element_by_class_name(
                        'entry-name').text.strip() == file_name:
                    found = True
                    break

            if found:
                logging.info("Copied the file successfully!")
            else:
                raise error.TestFail("File not transferred successfully!")

    def catch_info_or_error_messages(self, driver):
        """Logic to catch the error

        @param driver: chrome driver object
        """
        errors = []
        try:
            driver.find_element_by_css_selector(
                'div.button-frame').find_element_by_class_name('open').click()
        except Exception as e:
            logging.info("Error in open error messages")
            logging.info(str(e))
        error_elements = driver.find_elements_by_css_selector(
            'div.progress-frame')
        if len(error_elements) != 0:
            for error_element in error_elements:
                info_text = error_element.find_element_by_tag_name(
                    'label').text
                if info_text != "":
                    errors.append(info_text)
        return errors

    def create_file(self, filename):
        """Create a file"""
        status, output = getstatusoutput('dd if=/dev/zero of=%s bs=%s '
                                         'count=1 iflag=fullblock' %
                                         (filename, 1024))
        if status:
            raise error.TestError("Failed to create file")

    def run_once(self, username=None, password=None, source="Downloads",
                 file_name='test.dat'):
        """Copy file to Google Drive in Files application

        @param username: Real user(Not default autotest user)
        @param password: Password for the user.
        @param source: From where to copy file
        @param file_name: File name
        """
        self.success = False  # Used to capture the screenshot if the TC fails
        with chromedriver.chromedriver(username=username,
                                       password=password,
                                       disable_default_apps=False,
                                       gaia_login=True) as cr_instance:
            driver = cr_instance.driver
            self.open_files_application(driver)
            self.create_file(os.path.join(os.path.join(USER_LOCATION,
                                                       source), file_name))
            self.copy_file(driver, source, GOOGLE_DRIVE, file_name)
            errors = self.catch_info_or_error_messages(driver)
            if len(errors):
                raise error.TestFail("Test failed with the following"
                                     " errors. %s", errors)
        self.success = True
