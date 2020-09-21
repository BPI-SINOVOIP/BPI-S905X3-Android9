#!/bin/bash

# Copyright (C) 2015 The Android Open Source Project
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

# A helper script that launches Trade Federation for atest

shdir=`dirname $0`/

source "${shdir}/script_help.sh"

# TODO b/63295046 (sbasi) - Remove this when LOCAL_JAVA_LIBRARIES includes
# installation.
# Include any host-side dependency jars.
if [ ! -z "${ANDROID_HOST_OUT}" ]; then
    deps="compatibility-host-util.jar hosttestlib.jar cts-tradefed.jar vts-tradefed.jar host-libprotobuf-java-full.jar"
    for dep in $deps; do
        if [ -f "${ANDROID_HOST_OUT}"/framework/$dep ]; then
            TF_PATH=${TF_PATH}:"${ANDROID_HOST_OUT}"/framework/$dep
        fi
    done
fi

if [ "$(uname)" == "Darwin" ]; then
    local_tmp_dir="${ANDROID_HOST_OUT}/tmp"
    if [ ! -f "${local_tmp_dir}" ]; then
        mkdir -p "${local_tmp_dir}"
    fi
    java_tmp_dir_opt="-Djava.io.tmpdir=${local_tmp_dir}"
fi

# Note: must leave $RDBG_FLAG and $TRADEFED_OPTS unquoted so that they go away when unset
java $RDBG_FLAG -XX:+HeapDumpOnOutOfMemoryError -XX:-OmitStackTraceInFastThrow $TRADEFED_OPTS \
  -cp "${TF_PATH}" -DTF_JAR_DIR=${TF_JAR_DIR} ${java_tmp_dir_opt} com.android.tradefed.command.Console "$@"
