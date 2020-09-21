#
# Copyright (C) 2016 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

#Bluetooth POCs
module_testname := kernel_bluetooth/30149612
module_src_files := kernel_bluetooth/30149612/poc.cpp
module_cflags :=
module_c_includes :=
module_static_libraries :=
module_shared_libraries :=
include $(build_poc_test)

#Sound POCs
module_testname := kernel_sound/28838221
module_src_files := kernel_sound/28838221/poc.cpp
module_cflags :=
module_c_includes :=
module_static_libraries :=
module_shared_libraries :=
include $(build_poc_test)

#Wifi POCs
module_testname := kernel_wifi/32219453
module_src_files := kernel_wifi/32219453/poc.cpp
module_cflags :=
module_c_includes :=
module_static_libraries :=
module_shared_libraries := libnl
include $(build_poc_test)

module_testname := kernel_wifi/31707909
module_src_files := kernel_wifi/31707909/poc.cpp
module_cflags :=
module_c_includes :=
module_static_libraries :=
module_shared_libraries :=
include $(build_poc_test)

module_testname := kernel_wifi/32402310
module_src_files := kernel_wifi/32402310/poc.cpp
module_cflags :=
module_c_includes :=
module_static_libraries :=
module_shared_libraries := libnl
include $(build_poc_test)

