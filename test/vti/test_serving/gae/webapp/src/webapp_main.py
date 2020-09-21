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

import os

import webapp2

from webapp.src.dashboard import build_list
from webapp.src.dashboard import device_list
from webapp.src.dashboard import job_list
from webapp.src.dashboard import schedule_list
from webapp.src.handlers.base import BaseHandler
from webapp.src.scheduler import device_heartbeat
from webapp.src.scheduler import job_heartbeat
from webapp.src.scheduler import periodic
from webapp.src.tasks import indexing


class MainPage(BaseHandler):
    """Main web page request handler."""

    def get(self):
        """Generates an HTML page."""
        self.template = "index.html"

        template_values = {}

        self.render(template_values)


config = {}
config['webapp2_extras.sessions'] = {
    'secret_key': os.environ.get('SESSION_SECRET_KEY'),
}

app = webapp2.WSGIApplication(
    [
        ("/", MainPage), ("/build", build_list.BuildPage),
        ("/device", device_list.DevicePage), ("/job", job_list.JobPage),
        ("/create_job", job_list.CreateJobPage),
        ("/create_job_template", job_list.CreateJobTemplatePage),
        ("/result", MainPage), ("/schedule", schedule_list.SchedulePage),
        ("/tasks/schedule", periodic.PeriodicScheduler),
        ("/tasks/device_heartbeat", device_heartbeat.PeriodicDeviceHeartBeat),
        ("/tasks/job_heartbeat", job_heartbeat.PeriodicJobHeartBeat),
        ("/tasks/indexing", indexing.CreateIndex),
        ("/tasks/indexing/build", indexing.CreateBuildModelIndex),
        ("/tasks/indexing/device", indexing.CreateDeviceModelIndex),
        ("/tasks/indexing/job", indexing.CreateJobModelIndex),
        ("/tasks/indexing/lab", indexing.CreateLabModelIndex),
        ("/tasks/indexing/schedule", indexing.CreateScheduleModelIndex)
    ],
    config=config,
    debug=False)
