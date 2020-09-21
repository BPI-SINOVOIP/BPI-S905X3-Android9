#!/usr/bin/env python
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
#

import endpoints

from webapp.src.endpoint import build_info
from webapp.src.endpoint import host_info
from webapp.src.endpoint import job_queue
from webapp.src.endpoint import lab_info
from webapp.src.endpoint import schedule_info

api = endpoints.api_server([
    build_info.BuildInfoApi,
    host_info.HostInfoApi,
    lab_info.LabInfoApi,
    schedule_info.ScheduleInfoApi,
    schedule_info.GreenScheduleInfoApi,
    job_queue.JobQueueApi])
