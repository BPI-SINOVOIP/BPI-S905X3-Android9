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
import base64


def get_bluetooth_metrics(ad, bluetooth_proto_module):
    """
    Get metric proto from the Bluetooth stack

    Parameter:
      ad - Android device
      bluetooth_proto_module - the Bluetooth protomodule returned by
                                compile_import_proto()

    Return:
      a protobuf object representing Bluetooth metric

    """
    bluetooth_log = bluetooth_proto_module.BluetoothLog()
    proto_native_str_64 = \
        ad.adb.shell("/system/bin/dumpsys bluetooth_manager --proto-bin")
    proto_native_str = base64.b64decode(proto_native_str_64)
    bluetooth_log.MergeFromString(proto_native_str)
    return bluetooth_log
