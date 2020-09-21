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

from webapp.src.handlers.base import BaseHandler
from webapp.src.proto import model


class SchedulePage(BaseHandler):
    """Main class for /schedule web page."""

    def get(self):
        """Generates an HTML page based on the task schedules kept in DB."""
        self.template = "schedule.html"

        schedule_query = model.ScheduleModel.query()
        schedules = schedule_query.fetch()

        if schedules:
            schedules = sorted(
                schedules, key=lambda x: (x.manifest_branch, x.build_target),
                reverse=False)

        template_values = {
            "schedules": schedules,
        }

        self.render(template_values)