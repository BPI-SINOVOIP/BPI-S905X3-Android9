#/usr/bin/env python3.4
#
#   Copyright 2016 - The Android Open Source Project
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
"""
Sanity tests for voice tests in telephony
"""
import time

from acts.test_utils.tel.anritsu_utils import make_ims_call
from acts.test_utils.tel.anritsu_utils import tear_down_call
from acts.test_utils.tel.TelephonyBaseTest import TelephonyBaseTest
from acts.test_utils.tel.TelephonyLabPowerTest import TelephonyLabPowerTest

DEFAULT_CALL_NUMBER = "+11234567891"
WAIT_TIME_VOLTE = 5


class TelLabPowerVoLTETest(TelephonyLabPowerTest):

    # TODO Keep if we want to add more in here for this class.
    def __init__(self, controllers):
        TelephonyLabPowerTest.__init__(self, controllers)

    def setup_class(self):
        self.log.info("entering setup_class TelLabPowerVoLTETest")
        TelephonyLabPowerTest.setup_class(self)
        self.log.info("Making MO VoLTE call")
        # make a VoLTE MO call
        self.log.info("DEFAULT_CALL_NUMBER = " + DEFAULT_CALL_NUMBER)
        if not make_ims_call(self.log, self.ad, self.anritsu,
                             DEFAULT_CALL_NUMBER):
            self.log.error("Phone {} Failed to make volte call to {}"
                           .format(self.ad.serial, DEFAULT_CALL_NUMBER))
            return False
        self.log.info("wait for %d seconds" % WAIT_TIME_VOLTE)
        time.sleep(WAIT_TIME_VOLTE)
        return True

    def teardown_test(self):
        # check if the phone stay in call
        if not self.ad.droid.telecomIsInCall():
            self.log.error("Call is already ended in the phone.")
            return False
        self.log.info("End of teardown_test()")
        return True

    def teardown_class(self):
        if not tear_down_call(self.log, self.ad, self.anritsu):
            self.log.error("Phone {} Failed to tear down"
                           .format(self.ad.serial))
            return False
        # Always take down the simulation
        TelephonyLabPowerTest.teardown_class(self)
        return True

    """ Tests Begin """

    @TelephonyBaseTest.tel_test_wrap
    def test_volte_power_n40_n20(self):
        """ Measure power consumption of a VoLTE call with DL/UL -40/-20dBm
        Steps:
        1. Assume a VoLTE call is already in place by Setup_Class.
        2. Set DL/UL power and Dynamic scheduling
        3. Measure power consumption.

        Expected Results:
        1. power consumption measurement is successful
        2. measurement results is saved accordingly

        Returns:
            True if pass; False if fail
        """
        return self.power_test(-40, -20)

    @TelephonyBaseTest.tel_test_wrap
    def test_volte_power_n60_0(self):
        """ Measure power consumption of a VoLTE call with DL/UL -60/0dBm
        Steps:
        1. Assume a VoLTE call is already in place by Setup_Class.
        2. Set DL/UL power and Dynamic scheduling
        3. Measure power consumption.

        Expected Results:
        1. power consumption measurement is successful
        2. measurement results is saved accordingly

        Returns:
            True if pass; False if fail
        """
        return self.power_test(-60, 0)

    @TelephonyBaseTest.tel_test_wrap
    def test_volte_power_n80_20(self):
        """ Measure power consumption of a VoLTE call with DL/UL -80/+20dBm
        Steps:
        1. Assume a VoLTE call is already in place by Setup_Class.
        2. Set DL/UL power and Dynamic scheduling
        3. Measure power consumption.

        Expected Results:
        1. power consumption measurement is successful
        2. measurement results is saved accordingly

        Returns:
            True if pass; False if fail
        """
        return self.power_test(-80, 20)

    """ Tests End """
