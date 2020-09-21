#
#   Copyright 2018 - The Android Open Source Project
#
#   Licensed under the Apache License, Version 2.0 (the "License");
#   you may not use this file except in compliance with the License.
#   You may obtain a copy of the License at
#
#       http://www.apache.org/licenses/LICENSE-2.0
#
#   Unless required by applicable law or agreed to in writing, software
#   distributed under the License is distributed on an "AS IS" BASIS,
#   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#   See the License for the specific language governing permissions and
#   limitations under the License.

import logging
import os
import random
import socket
import threading
import time

from acts import asserts
from acts import base_test
from acts import test_runner
from acts.controllers import adb
from acts.test_decorators import test_tracker_info
from acts.test_utils.tel.tel_data_utils import wait_for_cell_data_connection
from acts.test_utils.tel.tel_test_utils import verify_http_connection
from acts.test_utils.tel.tel_test_utils import _check_file_existance
from acts.test_utils.tel.tel_test_utils import _generate_file_directory_and_file_name
from acts.test_utils.wifi import wifi_test_utils as wutils
from acts.test_utils.net.connectivity_const import MULTIPATH_PREFERENCE_NONE as NONE
from acts.test_utils.net.connectivity_const import MULTIPATH_PREFERENCE_HANDOVER as HANDOVER
from acts.test_utils.net.connectivity_const import MULTIPATH_PREFERENCE_RELIABILITY as RELIABILITY
from acts.test_utils.net.connectivity_const import MULTIPATH_PREFERENCE_PERFORMANCE as PERFORMANCE

DOWNLOAD_PATH = "/sdcard/Download/"
RELIABLE = RELIABILITY | HANDOVER

class DataCostTest(base_test.BaseTestClass):
    """ Tests for Wifi Tethering """

    def setup_class(self):
        """ Setup devices for tethering and unpack params """

        req_params = ("wifi_network", "download_file")
        self.unpack_userparams(req_params)

        for ad in self.android_devices:
            wutils.reset_wifi(ad)
            ad.droid.telephonyToggleDataConnection(True)
            wait_for_cell_data_connection(self.log, ad, True)
            asserts.assert_true(
                verify_http_connection(self.log, ad),
                "HTTP verification failed on cell data connection")

    def teardown_class(self):
        for ad in self.android_devices:
            wutils.reset_wifi(ad)
            ad.droid.telephonyToggleDataConnection(True)

    """ Helper functions """

    def _get_total_data_usage_for_device(self, ad, conn_type, sub_id):
        """ Get total data usage in MB for device

        Args:
            ad: Android device object
            conn_type: MOBILE/WIFI data usage
            sub_id: subscription id

        Returns:
            Data usage in MB
        """
        # end time should be in milli seconds and at least 2 hours more than the
        # actual end time. NetStats:bucket is of size 2 hours and to ensure to
        # get the most recent data usage, end_time should be +2hours
        end_time = int(time.time() * 1000) + 2 * 1000 * 60 * 60
        data_usage = ad.droid.connectivityQuerySummaryForDevice(
            conn_type, sub_id, 0, end_time)
        data_usage /= 1000.0 * 1000.0 # convert data_usage to MB
        self.log.info("Total data usage is: %s" % data_usage)
        return data_usage

    def _check_if_multipath_preference_valid(self, val, exp):
        """ Check if multipath value is same as expected

        Args:
            val: multipath preference for the network
            exp: expected multipath preference value
        """
        if exp == NONE:
            asserts.assert_true(val == exp, "Multipath value should be 0")
        else:
            asserts.assert_true(val >= exp,
                                "Multipath value should be at least %s" % exp)

    def _verify_multipath_preferences(self,
                                      ad,
                                      wifi_pref,
                                      cell_pref,
                                      wifi_network,
                                      cell_network):
        """ Verify mutlipath preferences for wifi and cell networks

        Args:
            ad: Android device object
            wifi_pref: Expected multipath value for wifi network
            cell_pref: Expected multipath value for cell network
            wifi_network: Wifi network id on the device
            cell_network: Cell network id on the device
        """
        wifi_multipath = \
            ad.droid.connectivityGetMultipathPreferenceForNetwork(wifi_network)
        cell_multipath = \
            ad.droid.connectivityGetMultipathPreferenceForNetwork(cell_network)
        self.log.info("WiFi multipath preference: %s" % wifi_multipath)
        self.log.info("Cell multipath preference: %s" % cell_multipath)
        self.log.info("Checking multipath preference for wifi")
        self._check_if_multipath_preference_valid(wifi_multipath, wifi_pref)
        self.log.info("Checking multipath preference for cell")
        self._check_if_multipath_preference_valid(cell_multipath, cell_pref)

    """ Test Cases """

    @test_tracker_info(uuid="e86c8108-3e84-4668-bae4-e5d2c8c27910")
    def test_multipath_preference_low_data_limit(self):
        """ Verify multipath preference when mobile data limit is low

        Steps:
            1. DUT has WiFi and LTE data
            2. Set mobile data usage limit to low value
            3. Verify that multipath preference is 0 for cell network
        """
        # set vars
        ad = self.android_devices[0]
        sub_id = str(ad.droid.telephonyGetSubscriberId())
        cell_network = ad.droid.connectivityGetActiveNetwork()
        self.log.info("cell network %s" % cell_network)
        wutils.wifi_connect(ad, self.wifi_network)
        wifi_network = ad.droid.connectivityGetActiveNetwork()
        self.log.info("wifi network %s" % wifi_network)

        # verify mulipath preference values
        self._verify_multipath_preferences(
            ad, RELIABLE, RELIABLE, wifi_network, cell_network)

        # set low data limit on mobile data
        total_pre = self._get_total_data_usage_for_device(ad, 0, sub_id)
        self.log.info("Setting data usage limit to %sMB" % (total_pre + 5))
        ad.droid.connectivitySetDataUsageLimit(
            sub_id, int((total_pre + 5) * 1000.0 * 1000.0))
        self.log.info("Setting data warning limit to %sMB" % (total_pre + 5))
        ad.droid.connectivitySetDataWarningLimit(
            sub_id, int((total_pre + 5) * 1000.0 * 1000.0))

        # reset data limit
        ad.droid.connectivityFactoryResetNetworkPolicies(sub_id)
        ad.droid.connectivitySetDataWarningLimit(sub_id, -1)

        # verify multipath preference values
        self._verify_multipath_preferences(
            ad, RELIABLE, NONE, wifi_network, cell_network)

    @test_tracker_info(uuid="a2781411-d880-476a-9f40-2c67e0f97db9")
    def test_multipath_preference_data_download(self):
        """ Verify multipath preference when large file is downloaded

        Steps:
            1. DUT has WiFi and LTE data
            2. WiFi is active network
            3. Download large file over cell network
            4. Verify multipath preference on cell network is 0
        """
        # set vars
        ad = self.android_devices[1]
        cell_network = ad.droid.connectivityGetActiveNetwork()
        self.log.info("cell network %s" % cell_network)
        wutils.wifi_connect(ad, self.wifi_network)
        wifi_network = ad.droid.connectivityGetActiveNetwork()
        self.log.info("wifi network %s" % wifi_network)

        # verify multipath preference for wifi and cell networks
        self._verify_multipath_preferences(
            ad, RELIABLE, RELIABLE, wifi_network, cell_network)

        # download file with cell network
        ad.droid.connectivityNetworkOpenConnection(cell_network,
                                                   self.download_file)
        file_folder, file_name = _generate_file_directory_and_file_name(
            self.download_file, DOWNLOAD_PATH)
        file_path = os.path.join(file_folder, file_name)
        self.log.info("File path: %s" % file_path)
        if _check_file_existance(ad, file_path):
            self.log.info("File exists. Removing file %s" % file_name)
            ad.adb.shell("rm -rf %s%s" % (DOWNLOAD_PATH, file_name))

        #  verify multipath preference values
        self._verify_multipath_preferences(
            ad, RELIABLE, NONE, wifi_network, cell_network)

    # TODO gmoturu@: Need to add tests that use the mobility rig and test when
    # the WiFi signal is poor and data signal is good.
