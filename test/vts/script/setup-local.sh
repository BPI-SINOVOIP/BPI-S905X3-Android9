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

function vts_multidevice_target_setup {
  DEVICE=$1

  pushd ${ANDROID_BUILD_TOP}
  adb root
  adb shell mkdir -p /data/local/tmp/32
  adb shell mkdir -p /data/local/tmp/64
  adb push ${ANDROID_BUILD_TOP}/out/target/product/${DEVICE}/system/bin/vts_hal_driver32 /data/local/tmp/32/
  adb push ${ANDROID_BUILD_TOP}/out/target/product/${DEVICE}/system/bin/vts_hal_driver64 /data/local/tmp/64/
  adb push ${ANDROID_BUILD_TOP}/out/target/product/${DEVICE}/system/bin/vts_shell_driver32 /data/local/tmp/32/
  adb push ${ANDROID_BUILD_TOP}/out/target/product/${DEVICE}/system/bin/vts_shell_driver64 /data/local/tmp/64/
  adb push ${ANDROID_BUILD_TOP}/out/target/product/${DEVICE}/system/bin/vts_hal_agent32 /data/local/tmp/32/
  adb push ${ANDROID_BUILD_TOP}/out/target/product/${DEVICE}/system/bin/vts_hal_agent64 /data/local/tmp/64/

  echo "install vts framework packages"
  adb push ${ANDROID_BUILD_TOP}/out/target/product/${DEVICE}/system/lib/libvts_common.so /data/local/tmp/32/
  adb push ${ANDROID_BUILD_TOP}/out/target/product/${DEVICE}/system/lib64/libvts_common.so /data/local/tmp/64/
  adb push ${ANDROID_BUILD_TOP}/out/target/product/${DEVICE}/system/lib/libvts_interfacespecification.so /data/local/tmp/32/
  adb push ${ANDROID_BUILD_TOP}/out/target/product/${DEVICE}/system/lib64/libvts_interfacespecification.so /data/local/tmp/64/
  adb push ${ANDROID_BUILD_TOP}/out/target/product/${DEVICE}/system/lib/libvts_drivercomm.so /data/local/tmp/32/
  adb push ${ANDROID_BUILD_TOP}/out/target/product/${DEVICE}/system/lib64/libvts_drivercomm.so /data/local/tmp/64/
  adb push ${ANDROID_BUILD_TOP}/out/target/product/${DEVICE}/system/lib/libvts_datatype.so /data/local/tmp/32/
  adb push ${ANDROID_BUILD_TOP}/out/target/product/${DEVICE}/system/lib64/libvts_datatype.so /data/local/tmp/64/
  adb push ${ANDROID_BUILD_TOP}/out/target/product/${DEVICE}/system/lib/libvts_measurement.so /data/local/tmp/32/
  adb push ${ANDROID_BUILD_TOP}/out/target/product/${DEVICE}/system/lib64/libvts_measurement.so /data/local/tmp/64/
  adb push ${ANDROID_BUILD_TOP}/out/target/product/${DEVICE}/system/lib/libvts_codecoverage.so /data/local/tmp/32/
  adb push ${ANDROID_BUILD_TOP}/out/target/product/${DEVICE}/system/lib64/libvts_codecoverage.so /data/local/tmp/64/
  adb push ${ANDROID_BUILD_TOP}/out/target/product/${DEVICE}/system/lib/libvts_multidevice_proto.so /data/local/tmp/32/
  adb push ${ANDROID_BUILD_TOP}/out/target/product/${DEVICE}/system/lib64/libvts_multidevice_proto.so /data/local/tmp/64/

  echo "install system packages that are not always installed by default"
  adb push ${ANDROID_BUILD_TOP}/out/host/linux-x86/vts/android-vts/testcases/DATA/lib/libprotobuf-cpp-full.so /data/local/tmp/32/
  adb push ${ANDROID_BUILD_TOP}/out/host/linux-x86/vts/android-vts/testcases/DATA/lib64/libprotobuf-cpp-full.so /data/local/tmp/64/

  echo "install vts drivers for hidl"
  adb push ${ANDROID_BUILD_TOP}/out/host/linux-x86/vts/android-vts/testcases/DATA/lib/android.hardware.*@*-vts.driver.so /data/local/tmp/32/
  adb push ${ANDROID_BUILD_TOP}/out/host/linux-x86/vts/android-vts/testcases/DATA/lib64/android.hardware.*@*-vts.driver.so /data/local/tmp/64/

  echo "install hal packages"
  adb shell mkdir -p /data/local/tmp/32/hw
  adb shell mkdir -p /data/local/tmp/64/hw
  adb push ${ANDROID_BUILD_TOP}/out/target/product/${DEVICE}/system/lib/hw/lights.vts.so /data/local/tmp/32/hw/
  adb push ${ANDROID_BUILD_TOP}/out/target/product/${DEVICE}/system/lib64/hw/lights.vts.so /data/local/tmp/64/hw/

  adb push ${ANDROID_BUILD_TOP}/out/target/product/${DEVICE}/obj_arm/SHARED_LIBRARIES/android.hardware.tests.libhwbinder@1.0_intermediates/LINKED/android.hardware.tests.libhwbinder@1.0.so /data/local/tmp/32/
  adb push ${ANDROID_BUILD_TOP}/out/target/product/${DEVICE}/obj/SHARED_LIBRARIES/android.hardware.tests.libhwbinder@1.0_intermediates/LINKED/android.hardware.tests.libhwbinder@1.0.so /data/local/tmp/64/

  adb push ${ANDROID_BUILD_TOP}/out/target/product/${DEVICE}/obj_arm/EXECUTABLES/libhwbinder_benchmark_intermediates/LINKED/libhwbinder_benchmark32 /data/local/tmp/32/
  adb push ${ANDROID_BUILD_TOP}/out/target/product/${DEVICE}/obj/EXECUTABLES/libhwbinder_benchmark_intermediates/LINKED/libhwbinder_benchmark64 /data/local/tmp/64/

  # uncomment for hidl in non-treble devices
  # adb push ${ANDROID_BUILD_TOP}/out/target/product/${DEVICE}/system/lib/android.hardware.nfc@1.0-vts.driver.so /data/local/tmp/32/hw/
  # adb push ${ANDROID_BUILD_TOP}/out/target/product/${DEVICE}/system/lib64/android.hardware.nfc@1.0-vts.driver.so /data/local/tmp/64/hw/
  # adb push images/${DEVICE}/32/libhwbinder.so /data/local/tmp/32/libhwbinder.so
  # adb push images/${DEVICE}/64/libhwbinder.so /data/local/tmp/64/libhwbinder.so

  adb shell mkdir -p /data/local/tmp/spec
  adb push ${ANDROID_BUILD_TOP}/test/vts/specification/hal/conventional /data/local/tmp/spec/hal/conventional
  adb push ${ANDROID_BUILD_TOP}/test/vts/specification/lib /data/local/tmp/spec/lib
  hidl-gen -o ${ANDROID_BUILD_TOP}/output -L vts -r android.hardware:hardware/interfaces -r android.hidl:system/libhidl/transport android.hardware.camera.provider@2.4
  hidl-gen -o ${ANDROID_BUILD_TOP}/output -L vts -r android.hardware:hardware/interfaces -r android.hidl:system/libhidl/transport android.hardware.camera.common@1.0
  hidl-gen -o ${ANDROID_BUILD_TOP}/output -L vts -r android.hardware:hardware/interfaces -r android.hidl:system/libhidl/transport android.hardware.camera.device@1.0
  hidl-gen -o ${ANDROID_BUILD_TOP}/output -L vts -r android.hardware:hardware/interfaces -r android.hidl:system/libhidl/transport android.hardware.camera.device@3.2
  hidl-gen -o ${ANDROID_BUILD_TOP}/output -L vts -r android.hardware:hardware/interfaces -r android.hidl:system/libhidl/transport android.hardware.camera.metadata@3.2
  hidl-gen -o ${ANDROID_BUILD_TOP}/output -L vts -r android.hardware:hardware/interfaces -r android.hidl:system/libhidl/transport android.hardware.gnss@1.0
  hidl-gen -o ${ANDROID_BUILD_TOP}/output -L vts -r android.hardware:hardware/interfaces -r android.hidl:system/libhidl/transport android.hardware.nfc@1.0
  hidl-gen -o ${ANDROID_BUILD_TOP}/output -L vts -r android.hardware:hardware/interfaces -r android.hidl:system/libhidl/transport android.hardware.vr@1.0
  hidl-gen -o ${ANDROID_BUILD_TOP}/output -L vts -r android.hardware:hardware/interfaces -r android.hidl:system/libhidl/transport android.hardware.automotive.vehicle@2.0
  hidl-gen -o ${ANDROID_BUILD_TOP}/output -L vts -r android.hardware:hardware/interfaces -r android.hidl:system/libhidl/transport android.hardware.automotive.vehicle@2.1
  hidl-gen -o ${ANDROID_BUILD_TOP}/output -L vts -r android.hardware:hardware/interfaces -r android.hidl:system/libhidl/transport android.hardware.sensors@1.0
  hidl-gen -o ${ANDROID_BUILD_TOP}/output -L vts -r android.hardware:hardware/interfaces -r android.hidl:system/libhidl/transport android.hardware.tv.cec@1.0
  hidl-gen -o ${ANDROID_BUILD_TOP}/output -L vts -r android.hardware:hardware/interfaces -r android.hidl:system/libhidl/transport android.hardware.vibrator@1.0
  hidl-gen -o ${ANDROID_BUILD_TOP}/output -L vts -r android.hardware:hardware/interfaces -r android.hidl:system/libhidl/transport android.hardware.contexthub@1.0
  adb push ${ANDROID_BUILD_TOP}/output/android/hardware /data/local/tmp/spec/android/
  adb push ${ANDROID_BUILD_TOP}/test/vts/specification/lib/ndk/bionic/1.0/libcV1.vts /data/local/tmp/spec/lib/ndk/bionic/1.0/libcV1.vts

  echo "install asan packages"
  # asan
  adb push ${ANDROID_BUILD_TOP}/prebuilts/clang/host/linux-x86/clang-2812033/lib64/clang/3.8/lib/linux/libclang_rt.asan-aarch64-android.so /data/local/tmp/libclang_rt.asan-aarch64-android.so

  adb shell chmod 755 /data/local/tmp/32/vts_hal_driver32
  adb shell chmod 755 /data/local/tmp/64/vts_hal_driver64
  adb shell chmod 755 /data/local/tmp/32/vts_shell_driver32
  adb shell chmod 755 /data/local/tmp/64/vts_shell_driver64
  adb shell chmod 755 /data/local/tmp/32/vts_hal_agent32
  adb shell chmod 755 /data/local/tmp/64/vts_hal_agent64
  popd
}

vts_multidevice_target_setup $1
