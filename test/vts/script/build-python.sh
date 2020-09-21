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

# Modifies any import statements (to remove subdir path)
pushd ${ANDROID_BUILD_TOP}/test/vts

## Modifies import statement in proto files.
sed -i 's/import "test\/vts\/proto\/ComponentSpecificationMessage.proto";/import "ComponentSpecificationMessage.proto";/g' proto/AndroidSystemControlMessage.proto
sed -i 's/import "test\/vts\/proto\/ComponentSpecificationMessage.proto";/import "ComponentSpecificationMessage.proto";/g' proto/VtsProfilingMessage.proto
sed -i 's/import "test\/vts\/proto\/ComponentSpecificationMessage.proto";/import "ComponentSpecificationMessage.proto";/g' proto/VtsFuzzTaskMessage.proto
sed -i 's/import "test\/vts\/proto\/VtsReportMessage.proto";/import "VtsReportMessage.proto";/g' proto/VtsFuzzTaskMessage.proto

## Compiles modified proto files to .py files.
protoc -I=proto --python_out=proto proto/AndroidSystemControlMessage.proto
protoc -I=proto --python_out=proto proto/ComponentSpecificationMessage.proto
protoc -I=proto --python_out=proto proto/VtsProfilingMessage.proto
protoc -I=proto --python_out=proto proto/VtsFuzzTaskMessage.proto

## Restores import statement in proto files.
sed -i 's/import "ComponentSpecificationMessage.proto";/import "test\/vts\/proto\/ComponentSpecificationMessage.proto";/g' proto/AndroidSystemControlMessage.proto
sed -i 's/import "ComponentSpecificationMessage.proto";/import "test\/vts\/proto\/ComponentSpecificationMessage.proto";/g' proto/VtsProfilingMessage.proto
sed -i 's/import "ComponentSpecificationMessage.proto";/import "test\/vts\/proto\/ComponentSpecificationMessage.proto";/g' proto/VtsFuzzTaskMessage.proto
sed -i 's/import "VtsReportMessage.proto";/import "test\/vts\/proto\/VtsReportMessage.proto";/g' proto/VtsFuzzTaskMessage.proto

protoc -I=proto --python_out=proto proto/ComponentSpecificationMessage.proto
protoc -I=proto --python_out=proto proto/TestSchedulingPolicyMessage.proto
protoc -I=proto --python_out=proto proto/VtsReportMessage.proto

# Compiles all the python source codes.
python -m compileall .

popd
