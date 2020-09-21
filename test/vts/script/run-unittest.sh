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

pushd ${ANDROID_BUILD_TOP}/test/vts
PYTHONPATH=$PYTHONPATH:.. python -m vts.runners.host.tcp_server.callback_server_test
PYTHONPATH=$PYTHONPATH:.. python -m vts.utils.python.coverage.coverage_report_test
PYTHONPATH=$PYTHONPATH:.. python -m vts.harnesses.host_controller.build.build_provider_pab_test
PYTHONPATH=$PYTHONPATH:.. python -m vts.utils.python.controllers.android_device_test
PYTHONPATH=$PYTHONPATH:.. python -m vts.utils.python.controllers.customflasher_test
PYTHONPATH=$PYTHONPATH:..:../framework/harnesses/ python -m host_controller.build.build_flasher_test
PYTHONPATH=$PYTHONPATH:..:../framework/harnesses/ python -m host_controller.build.build_provider_test
PYTHONPATH=$PYTHONPATH:..:../framework/harnesses/ python -m host_controller.console_test
PYTHONPATH=$PYTHONPATH:.. python -m vts.utils.python.io.file_util_test
popd
