#!/bin/bash
#
# Copyright 2018 The Android Open Source Project
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

if [ "$#" -ne 1 ]; then
  echo "usage: build.sh prod|test"
  exit 1
fi

if [ $1 = "prod" ]; then
  SERVICE="vtslab-schedule-prod.appspot.com"
else
  SERVICE="vtslab-schedule-test.appspot.com"
fi

echo "Building the webapp for $SERVICE ..."

python lib/endpoints/endpointscfg.py get_openapi_spec webapp.src.endpoint.build_info.BuildInfoApi --hostname $SERVICE
python lib/endpoints/endpointscfg.py get_openapi_spec webapp.src.endpoint.host_info.HostInfoApi --hostname $SERVICE
python lib/endpoints/endpointscfg.py get_openapi_spec webapp.src.endpoint.schedule_info.ScheduleInfoApi --hostname $SERVICE
python lib/endpoints/endpointscfg.py get_openapi_spec webapp.src.endpoint.lab_info.LabInfoApi --hostname $SERVICE

echo "Build complete."
