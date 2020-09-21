#/usr/bin/env python3.4
#
# Copyright (C) 2016 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License"); you may not
# use this file except in compliance with the License. You may obtain a copy of
# the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
# License for the specific language governing permissions and limitations under
# the License.

import time
from acts.base_test import BaseTestClass
from acts.test_utils.bt.native_bt_test_utils import setup_native_bluetooth
from acts.test_utils.bt.bt_test_utils import generate_id_by_size

class NativeTest(BaseTestClass):
    tests = None

    def __init__(self, controllers):
        BaseTestClass.__init__(self, controllers)
        self.droid = self.native_android_devices[0].droid
        self.tests = (
                "test_bool_return_true",
                "test_bool_return_false",
                "test_null_return",
                "test_string_empty_return",
                "test_max_param_size",
        )

    def test_bool_return_true(self):
        return self.droid.TestBoolTrueReturn()

    def test_bool_return_false(self):
        return not self.droid.TestBoolFalseReturn()

    def test_null_return(self):
        return not self.droid.TestNullReturn()

    def test_string_empty_return(self):
        return self.droid.TestStringEmptyReturn() == ""

    def test_max_param_size(self):
        json_buffer_size = 64
        max_sl4n_buffer_size = 4096
        test_string = "x" * (max_sl4n_buffer_size - json_buffer_size)
        return test_string == self.droid.TestStringMaxReturn(test_string)

    def test_specific_param_naming(self):
        a = [{"string_test":"test", "int_test":1}]
        return self.droid.TestSpecificParamNaming(a)

