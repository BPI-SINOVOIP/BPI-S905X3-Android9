#!/bin/bash

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

# A helper script that launches Trade Federation's "Verify" entrypoint, to perform
# standalone command file verification

shdir=`dirname $0`/
source "${shdir}/script_help.sh"
# At this point, we're guaranteed to have the right Java version, and the following
# env variables will be set, if appropriate:
# JAVA_VERSION, RDBG_FLAG, TF_PATH, TRADEFED_OPTS


# Note: must leave $RDBG_FLAG and $TRADEFED_OPTS unquoted so that they go away when unset
java $RDBG_FLAG -XX:+HeapDumpOnOutOfMemoryError -XX:-OmitStackTraceInFastThrow $TRADEFED_OPTS \
  -cp "${TF_PATH}" com.android.tradefed.command.CommandRunner "$@"
