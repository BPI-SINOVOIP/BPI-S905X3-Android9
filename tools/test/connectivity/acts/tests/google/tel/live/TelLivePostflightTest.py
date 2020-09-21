#!/usr/bin/env python3.4
#
#   Copyright 2017 - Google
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
    Test Script for Telephony Post Flight check.
"""
import os
from acts import utils
from acts.asserts import fail
from acts.base_test import BaseTestClass
from acts.test_decorators import test_tracker_info
from acts.test_utils.tel.TelephonyBaseTest import TelephonyBaseTest


class TelLivePostflightTest(TelephonyBaseTest):
    def __init__(self, controllers):
        BaseTestClass.__init__(self, controllers)

    def setup_class(self):
        pass

    def teardown_class(self):
        pass

    def setup_test(self):
        pass

    def on_pass(self, *arg):
        pass

    def on_fail(self, *arg):
        pass

    @test_tracker_info(uuid="ba6e260e-d2e1-4c01-9d51-ef2df1591039")
    @TelephonyBaseTest.tel_test_wrap
    def test_check_crash(self):
        msg = ""
        for ad in self.android_devices:
            post_crash = ad.check_crash_report(self.test_id)
            pre_crash = getattr(ad, "crash_report_preflight", [])
            crash_diff = list(set(post_crash).difference(set(pre_crash)))
            if crash_diff:
                msg += "%s find new crash reports %s " % (ad.serial,
                                                          crash_diff)
                ad.log.error("Find new crash reports %s", crash_diff)
                crash_path = os.path.join(ad.log_path, self.test_name,
                                          "Crashes")
                utils.create_dir(crash_path)
                ad.pull_files(crash_diff, crash_path)
                self._ad_take_bugreport(ad, self.test_name, self.begin_time)
        if msg:
            fail(msg)
        return True

    @test_tracker_info(uuid="a94a0145-27be-4610-90f7-3af561d1b1ec")
    @TelephonyBaseTest.tel_test_wrap
    def test_check_dialer_crash(self):
        msg = ""
        for ad in self.android_devices:
            tombstones = ad.get_file_names("/data/tombstones/")
            if not tombstones: continue
            for tombstone in tombstones:
                if ad.adb.shell("cat %s | grep pid | grep dialer" % tombstone):
                    message = "%s dialer crash: %s " % (ad.serial, tombstone)
                    ad.log.error(message)
                    msg += message
                    crash_path = os.path.join(ad.log_path, self.test_name,
                                              "Crashes")
                    utils.create_dir(crash_path)
                    ad.pull_files([tombstone], crash_path)
        if msg:
            fail(msg)
        return True

    @test_tracker_info(uuid="707d4a33-2e21-40ea-bd27-d15f4e3ff0f0")
    @TelephonyBaseTest.tel_test_wrap
    def test_check_data_accounting_failures(self):
        msg = ""
        for ad in self.android_devices:
            ad.log.info("data_accounting_errors: %s", dict(ad.data_accounting))
            if any(ad.data_accounting.values()):
                msg += "%s %s" % (ad.serial, dict(ad.data_accounting))
        if msg:
            fail(msg)
        return True
