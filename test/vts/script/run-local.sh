#!/bin/bash
#
# Copyright 2016 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

PYTHONPATH=$PYTHONPATH:.. python -m vts.testcases.host.light.conventional.SampleLightTest
# PYTHONPATH=$PYTHONPATH:.. python -m vts.testcases.host.bluetooth.conventional.SampleBluetoothTest
# PYTHONPATH=$PYTHONPATH:.. python -m vts.testcases.fuzz.hal_light.conventional.LightFuzzTest $ANDROID_BUILD_TOP/test/vts/testcases/fuzz/hal_light/conventional/LightFuzzTest.config
# PYTHONPATH=$PYTHONPATH:.. python -m vts.testcases.fuzz.hal_light.conventional_standalone.StandaloneLightFuzzTest $ANDROID_BUILD_TOP/test/vts/testcases/fuzz/hal_light/conventional_standalone/StandaloneLightFuzzTest.config
# PYTHONPATH=$PYTHONPATH:.. python -m vts.testcases.host.camera.conventional.SampleCameraTest
# PYTHONPATH=$PYTHONPATH:.. python -m vts.testcases.host.camera.conventional.2_1.SampleCameraV2Test
# PYTHONPATH=$PYTHONPATH:.. python -m vts.testcases.host.camera.conventional.3_4.SampleCameraV3Test
# PYTHONPATH=$PYTHONPATH:.. python -m vts.testcases.hal.nfc.V1_0.host.VtsHalNfcV1_0HostTest
# PYTHONPATH=$PYTHONPATH:.. python -m vts.testcases.hal.camera.provider.V2_4.host.VtsHalCameraProviderV2_4HostTest
# PYTHONPATH=$PYTHONPATH:.. python -m vts.testcases.hal.gnss.V1_0.host.VtsHalGnssV1_0HostTest
# PYTHONPATH=$PYTHONPATH:.. python -m vts.testcases.hal.vibrator.V1_0.host.VtsHalVibratorV1_0HostTest
# PYTHONPATH=$PYTHONPATH:.. python -m vts.testcases.hal.automotive.vehicle.V2_0.host.VtsHalAutomotiveVehicleV2_0HostTest
# PYTHONPATH=$PYTHONPATH:.. python -m vts.testcases.hal.vr.V1_0.hidl.host.VtsHalVrV1_0HostTest
# PYTHONPATH=$PYTHONPATH:.. python -m vts.testcases.hal.tv.cec.V1_0.host.VtsHalTvCecV1_0HostTest
# PYTHONPATH=$PYTHONPATH:.. python -m vts.testcases.host.shell.SampleShellTest
# PYTHONPATH=$PYTHONPATH:.. python -m vts.testcases.fuzz_test.lib_bionic.LibBionicLibmFuzzTest
# PYTHONPATH=$PYTHONPATH:.. python -m vts.testcases.system.libc.LibcTest
