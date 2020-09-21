#
#   Copyright 2017 - The Android Open Source Project
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

from acts import asserts
from acts import base_test
from acts.controllers import adb
from acts.test_decorators import test_tracker_info
from acts.test_utils.tel.tel_data_utils import wait_for_cell_data_connection
from acts.test_utils.tel.tel_test_utils import verify_http_connection
from acts.test_utils.wifi import wifi_test_utils as wutils

dum_class = "com.android.tests.connectivity.uid.ConnectivityTestActivity"


class CoreNetworkingTest(base_test.BaseTestClass):
    """ Tests for UID networking """

    def setup_class(self):
        """ Setup devices for tests and unpack params """
        self.dut = self.android_devices[0]
        wutils.wifi_toggle_state(self.dut, False)
        self.dut.droid.telephonyToggleDataConnection(True)
        wait_for_cell_data_connection(self.log, self.dut, True)
        asserts.assert_true(
            verify_http_connection(self.log, self.dut),
            "HTTP verification failed on cell data connection")

    def teardown_class(self):
        """ Reset devices """
        wutils.wifi_toggle_state(self.dut, True)

    """ Test Cases """

    @test_tracker_info(uuid="0c89d632-aafe-4bbd-a812-7b0eca6aafc7")
    def test_uid_derace_data_saver_mode(self):
        """ Verify UID de-race data saver mode

        Steps:
            1. Connect DUT to data network and verify internet
            2. Enable data saver mode
            3. Launch app and verify internet connectivity
            4. Disable data saver mode
        """
        # Enable data saver mode
        self.log.info("Enable data saver mode")
        self.dut.adb.shell("cmd netpolicy set restrict-background true")

        # Launch app, check internet connectivity and close app
        self.log.info("Launch app and test internet connectivity")
        res = self.dut.droid.launchForResult(dum_class)

        # Disable data saver mode
        self.log.info("Disable data saver mode")
        self.dut.adb.shell("cmd netpolicy set restrict-background false")

        # Return test result
        self.log.info("Internet connectivity status after app launch: %s "
                      % res['extras']['result'])
        return res['extras']['result']
