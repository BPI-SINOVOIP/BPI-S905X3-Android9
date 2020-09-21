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

import time

from acts import asserts
from acts import base_test
from acts.controllers import adb
from acts.test_decorators import test_tracker_info
from acts.test_utils.net import connectivity_const as cconst
from acts.test_utils.tel.tel_data_utils import wait_for_cell_data_connection
from acts.test_utils.tel.tel_test_utils import http_file_download_by_chrome
from acts.test_utils.tel.tel_test_utils import verify_http_connection
from acts.test_utils.tel import tel_test_utils as ttutils
from acts.test_utils.wifi import wifi_test_utils as wutils

conn_test_class = "com.android.tests.connectivity.uid.ConnectivityTestActivity"
android_os_class = "com.quicinc.cne.CNEService.CNEServiceApp"
instr_cmd = "am instrument -w -e command grant-all \
    com.android.permissionutils/.PermissionInstrumentation"

HOUR_IN_MILLIS = 1000 * 60 * 60
BYTE_TO_MB_ANDROID = 1000.0 * 1000.0
BYTE_TO_MB = 1024.0 * 1024.0
DOWNLOAD_PATH = "/sdcard/download/"
DATA_USG_ERR = 2.2
DATA_ERR = 0.2
TIMEOUT = 2 * 60
INC_DATA = 10


class DataUsageTest(base_test.BaseTestClass):
    """ Data usage tests """

    def __init__(self, controllers):
        base_test.BaseTestClass.__init__(self, controllers)
        self.tests = ("test_mobile_data_usage_downlink",
                      "test_wifi_data_usage_downlink",
                      "test_wifi_tethering_mobile_data_usage_downlink",
                      "test_data_usage_limit_downlink",
                      "test_wifi_tethering_data_usage_limit_downlink",)

    def setup_class(self):
        """ Setup devices for tests and unpack params """
        self.dut = self.android_devices[0]
        self.tethered_devices = self.android_devices[1:]
        wutils.reset_wifi(self.dut)
        self.dut.droid.telephonyToggleDataConnection(True)
        wait_for_cell_data_connection(self.log, self.dut, True)
        asserts.assert_true(
            verify_http_connection(self.log, self.dut),
            "HTTP verification failed on cell data connection")

        # unpack user params
        req_params = ("wifi_network", "download_file", "file_size", "network")
        self.unpack_userparams(req_params)
        self.file_path = DOWNLOAD_PATH + self.download_file.split('/')[-1]
        self.file_size = int(self.file_size)
        self.sub_id = str(self.dut.droid.telephonyGetSubscriberId())
        self.android_os_uid = self.dut.droid.getUidForPackage(android_os_class)
        self.conn_test_uid = self.dut.droid.getUidForPackage(conn_test_class)
        for ad in self.android_devices:
            try:
                ad.adb.shell(instr_cmd)
            except adb.AdbError:
                self.log.warn("adb cmd %s failed on %s" % (instr_cmd, ad.serial))

        # Set chrome browser start with no-first-run verification
        # Give permission to read from and write to storage
        commands = ["pm grant com.android.chrome "
                    "android.permission.READ_EXTERNAL_STORAGE",
                    "pm grant com.android.chrome "
                    "android.permission.WRITE_EXTERNAL_STORAGE",
                    "rm /data/local/chrome-command-line",
                    "am set-debug-app --persistent com.android.chrome",
                    'echo "chrome --no-default-browser-check --no-first-run '
                    '--disable-fre" > /data/local/tmp/chrome-command-line']
        for cmd in commands:
            for dut in self.android_devices:
                try:
                    dut.adb.shell(cmd)
                except adb.AdbError:
                    self.log.warn("adb command %s failed on %s" % (cmd, dut.serial))

    def teardown_class(self):
        """ Reset devices """
        wutils.reset_wifi(self.dut)

    """ Helper functions """

    def _download_data_through_app(self, ad):
        """ Download data through app on DUT

        Args:
            1. ad - DUT to download the file on

        Returns:
            True - if file download is successful
            False - if file download is not successful
        """
        intent = self.dut.droid.createIntentForClassName(conn_test_class)
        json_obj = {"url": self.download_file}
        ad.droid.launchForResultWithIntent(intent, json_obj)
        download_status = False
        end_time = time.time() + TIMEOUT
        while time.time() < end_time:
            download_status = ttutils._check_file_existance(
                ad, self.file_path, self.file_size * BYTE_TO_MB)
            if download_status:
                self.log.info("Delete file: %s", self.file_path)
                ad.adb.shell("rm %s" % self.file_path, ignore_status=True)
                break
            time.sleep(8) # wait to check again if download is complete
        return download_status

    def _get_data_usage(self, ad, conn_type):
        """ Get data usage

        Args:
            1. ad - DUT to get data usage from
            2. conn_type - MOBILE/WIFI data usage

        Returns:
            Tuple of Android Os app, Conn UID app, Total data usages
        """
        aos = self._get_data_usage_for_uid_rx(ad, conn_type, self.android_os_uid)
        app = self._get_data_usage_for_uid_rx(ad, conn_type, self.conn_test_uid)
        tot = self._get_data_usage_for_device_rx(ad, conn_type)
        self.log.info("Android Os data usage: %s" % aos)
        self.log.info("Conn UID Test data usage: %s" % app)
        self.log.info("Total data usage: %s" % tot)
        return (aos, app, tot)

    def _get_total_data_usage_for_device(self, conn_type):
        """ Get total data usage in MB for device

        Args:
            1. conn_type - MOBILE/WIFI data usage

        Returns:
            Data usage in MB
        """
        end_time = int(time.time() * 1000) + 2 * HOUR_IN_MILLIS
        data_usage = self.dut.droid.connectivityQuerySummaryForDevice(
            conn_type, self.sub_id, 0, end_time)
        data_usage /= BYTE_TO_MB_ANDROID
        self.log.info("Total data usage is: %s" % data_usage)
        return data_usage

    def _get_data_usage_for_uid_rx(self, ad, conn_type, uid):
        """ Get data usage for UID in Rx Bytes

        Args:
            1. ad - DUT to get data usage from
            2. conn_type - MOBILE/WIFI data usage
            3. uid - UID of the app

        Returns:
            Data usage in MB
        """
        subscriber_id = ad.droid.telephonyGetSubscriberId()
        end_time = int(time.time() * 1000) + 2 * HOUR_IN_MILLIS
        data_usage = ad.droid.connectivityQueryDetailsForUidRxBytes(
            conn_type, subscriber_id, 0, end_time, uid)
        return data_usage/BYTE_TO_MB_ANDROID

    def _get_data_usage_for_device_rx(self, ad, conn_type):
        """ Get total data usage in rx bytes for device

        Args:
            1. ad - DUT to get data usage from
            2. conn_type - MOBILE/WIFI data usage

        Returns:
            Data usage in MB
        """
        subscriber_id = ad.droid.telephonyGetSubscriberId()
        end_time = int(time.time() * 1000) + 2 * HOUR_IN_MILLIS
        data_usage = ad.droid.connectivityQuerySummaryForDeviceRxBytes(
            conn_type, subscriber_id, 0, end_time)
        return data_usage/BYTE_TO_MB_ANDROID

    """ Test Cases """

    @test_tracker_info(uuid="b2d9b36c-3a1c-47ca-a9c1-755450abb20c")
    def test_mobile_data_usage_downlink(self):
        """ Verify mobile data usage

        Steps:
            1. Get the current data usage of ConnUIDTest and Android OS apps
            2. DUT is on LTE data
            3. Download file of size xMB through ConnUIDTest app
            4. Verify that data usage of Android OS app did not change
            5. Verify that data usage of ConnUIDTest app increased by ~xMB
            6. Verify that data usage of device also increased by ~xMB
        """
        # disable wifi
        wutils.wifi_toggle_state(self.dut, False)

        # get pre mobile data usage
        (aos_pre, app_pre, total_pre) = self._get_data_usage(self.dut,
                                                             cconst.TYPE_MOBILE)

        # download file through app
        self._download_data_through_app(self.dut)

        # get new mobile data usage
        (aos_pst, app_pst, total_pst) = self._get_data_usage(self.dut,
                                                           cconst.TYPE_MOBILE)

        # verify data usage
        aos_diff = aos_pst - aos_pre
        app_diff = app_pst - app_pre
        total_diff = total_pst - total_pre
        self.log.info("Data usage of Android os increased by %s" % aos_diff)
        self.log.info("Data usage of ConnUID app increased by %s" % app_diff)
        self.log.info("Data usage on the device increased by %s" % total_diff)
        return (aos_diff < DATA_ERR) and \
            (self.file_size < app_diff < self.file_size + DATA_USG_ERR) and \
            (self.file_size < total_diff < self.file_size + DATA_USG_ERR)

    @test_tracker_info(uuid="72ddb42a-5942-4a6a-8b20-2181c41b2765")
    def test_wifi_data_usage_downlink(self):
        """ Verify wifi data usage

        Steps:
            1. Get the current data usage of ConnUIDTest and Android OS apps
            2. DUT is on LTE data
            3. Download file of size xMB through ConnUIDTest app
            4. Verify that data usage of Android OS app did not change
            5. Verify that data usage of ConnUIDTest app increased by ~xMB
            6. Verify that data usage of device also increased by ~xMB
        """
        # connect to wifi network
        wutils.wifi_connect(self.dut, self.wifi_network)

        # get pre wifi data usage
        (aos_pre, app_pre, total_pre) = self._get_data_usage(self.dut,
                                                             cconst.TYPE_WIFI)

        # download file through app
        self._download_data_through_app(self.dut)

        # get new mobile data usage
        (aos_pst, app_pst, total_pst) = self._get_data_usage(self.dut,
                                                           cconst.TYPE_WIFI)

        # verify data usage
        aos_diff = aos_pst - aos_pre
        app_diff = app_pst - app_pre
        total_diff = total_pst - total_pre
        self.log.info("Data usage of Android os increased by %s" % aos_diff)
        self.log.info("Data usage of ConnUID app increased by %s" % app_diff)
        self.log.info("Data usage on the device increased by %s" % total_diff)
        return (aos_diff < DATA_ERR) and \
            (self.file_size < app_diff < self.file_size + DATA_USG_ERR) and \
            (self.file_size < total_diff < self.file_size + DATA_USG_ERR)

    @test_tracker_info(uuid="fe1390e5-635c-49a9-b050-032e66f52f40")
    def test_wifi_tethering_mobile_data_usage_downlink(self):
        """ Verify mobile data usage with tethered device

        Steps:
            1. Start wifi hotspot and connect tethered device to it
            2. Get the data usage on hotspot device
            3. Download data on tethered device
            4. Get the new data usage on hotspot device
            5. Verify that hotspot device's data usage increased by downloaded file size
        """
        # connect device to wifi hotspot
        ad = self.tethered_devices[0]
        wutils.start_wifi_tethering(self.dut,
                                    self.network[wutils.WifiEnums.SSID_KEY],
                                    self.network[wutils.WifiEnums.PWD_KEY],
                                    ttutils.WIFI_CONFIG_APBAND_2G)
        wutils.wifi_connect(ad, self.network)

        # get pre mobile data usage
        (aos_pre, app_pre, total_pre) = self._get_data_usage(self.dut,
                                                             cconst.TYPE_MOBILE)

        # download file through app
        self._download_data_through_app(ad)

        # get new mobile data usage
        (aos_pst, app_pst, total_pst) = self._get_data_usage(self.dut,
                                                             cconst.TYPE_MOBILE)

        # stop wifi hotspot
        wutils.stop_wifi_tethering(self.dut)

        # verify data usage
        aos_diff = aos_pst - aos_pre
        app_diff = app_pst - app_pre
        total_diff = total_pst - total_pre
        self.log.info("Data usage of Android os increased by %s" % aos_diff)
        self.log.info("Data usage of ConnUID app increased by %s" % app_diff)
        self.log.info("Data usage on the device increased by %s" % total_diff)
        return (aos_diff < DATA_ERR) and (app_diff < DATA_ERR) and \
            (self.file_size < total_diff < self.file_size + DATA_USG_ERR)

    @test_tracker_info(uuid="ac4750fd-20d9-451d-a85b-79fdbaa7da97")
    def test_data_usage_limit_downlink(self):
        """ Verify connectivity when data usage limit reached

        Steps:
            1. Set the data usage limit to current data usage + 10MB
            2. Download 20MB data
            3. File download stops and data limit reached
            4. Device should lose internet connectivity
            5. Verify data usage limit
        """
        # get pre mobile data usage
        total_pre = self._get_total_data_usage_for_device(cconst.TYPE_MOBILE)

        # set data usage limit to current usage limit + 10MB
        self.log.info("Setting data usage limit to %sMB" % (total_pre + INC_DATA))
        self.dut.droid.connectivitySetDataUsageLimit(
            self.sub_id, str(int((total_pre + INC_DATA) * BYTE_TO_MB_ANDROID)))

        # download file through app
        http_file_download_by_chrome(
            self.dut, self.download_file, self.file_size, timeout=120)
        total_pst = self._get_total_data_usage_for_device(cconst.TYPE_MOBILE)

        # verify data usage
        connectivity_status = wutils.validate_connection(self.dut)
        self.dut.droid.connectivityFactoryResetNetworkPolicies(self.sub_id)
        self.log.info("Expected data usage: %s" % (total_pre + INC_DATA))
        self.log.info("Actual data usage: %s" % total_pst)
        asserts.assert_true(
            not connectivity_status,
            "Device has internet connectivity after reaching data limit")
        return total_pst - total_pre - INC_DATA < DATA_USG_ERR

    @test_tracker_info(uuid="7c9ab330-9645-4030-bb1e-dcce126944a2")
    def test_wifi_tethering_data_usage_limit_downlink(self):
        """ Verify connectivity when data usage limit reached

        Steps:
            1. Set the data usage limit to current data usage + 10MB
            2. Start wifi tethering and connect a dut to the SSID
            3. Download 20MB data on tethered device
            4. File download stops and data limit reached
            5. Verify data usage limit
        """
        # connect device to wifi hotspot
        ad = self.tethered_devices[0]
        wutils.toggle_wifi_off_and_on(self.dut)
        wutils.start_wifi_tethering(self.dut,
                                    self.network[wutils.WifiEnums.SSID_KEY],
                                    self.network[wutils.WifiEnums.PWD_KEY],
                                    ttutils.WIFI_CONFIG_APBAND_2G)
        wutils.wifi_connect(ad, self.network)

        # get pre mobile data usage
        total_pre = self._get_total_data_usage_for_device(cconst.TYPE_MOBILE)

        # set data usage limit to current usage limit + 10MB
        self.log.info("Setting data usage limit to %sMB" % (total_pre + INC_DATA))
        self.dut.droid.connectivitySetDataUsageLimit(
            self.sub_id, str(int((total_pre + INC_DATA) * BYTE_TO_MB_ANDROID)))

        # download file from tethered device
        http_file_download_by_chrome(
            ad, self.download_file, self.file_size, timeout=120)
        total_pst = self._get_total_data_usage_for_device(cconst.TYPE_MOBILE)

        # verify data usage
        connectivity_status = wutils.validate_connection(ad)
        self.dut.droid.connectivityFactoryResetNetworkPolicies(self.sub_id)
        wutils.stop_wifi_tethering(self.dut)
        self.log.info("Expected data usage: %s" % (total_pre + INC_DATA))
        self.log.info("Actual data usage: %s" % total_pst)
        asserts.assert_true(
            not connectivity_status,
            "Device has internet connectivity after reaching data limit")
        return total_pst - total_pre - INC_DATA < DATA_USG_ERR
