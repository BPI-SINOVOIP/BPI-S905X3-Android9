#!/bin/bash

#
# Copyright (C) 2017 The Android Open Source Project
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

function all-targets() {
    local target_device=$(echo $TARGET_PRODUCT | sed "s/^aosp_//")
    $ANDROID_BUILD_TOP/build/soong/soong_ui.bash --make-mode -j "out/target/product/$target_device/module-info.json"

    python3 -c "import json; print('\n'.join(sorted(json.load(open('$ANDROID_PRODUCT_OUT/module-info.json')).keys())))" \
        | grep -vP "_32$"
}

function update_file() {
    local file_inception="$1"
    local variable_name="$2"
    local pattern="$3"

    local filename="$ANDROID_BUILD_TOP/test/vts/tools/build/tasks/list/$variable_name"

    echo "#
# Copyright (C) $file_inception The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an \"AS IS\" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

$variable_name := \\
$(all-targets | grep -P "$pattern" | awk '{print "  " $0 " \\"}')" > "$filename".mk
}

# note, must come up with pattern to weed out vendor-specific files
update_file 2017 vts_adapter_package_list '@.*-adapter'