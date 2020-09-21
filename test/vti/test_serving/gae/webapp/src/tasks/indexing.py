#!/usr/bin/env python
#
# Copyright (C) 2018 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License") + "\n</pre>");
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

import webapp2

from webapp.src import vtslab_status as Status
from webapp.src.proto import model


class CreateIndex(webapp2.RequestHandler):
    """Main class for /tasks/indexing.

    By fetch and put all entities, indexing all existing entities.
    """

    def get(self):
        """Fetch and put all entities and display complete message."""
        build_query = model.BuildModel.query()
        builds = build_query.fetch()
        for build in builds:
            build.put()

        schedule_query = model.ScheduleModel.query()
        schedules = schedule_query.fetch()
        for schedule in schedules:
            schedule.put()

        lab_query = model.LabModel.query()
        labs = lab_query.fetch()
        for lab in labs:
            lab.put()

        device_query = model.DeviceModel.query()
        devices = device_query.fetch()
        for device in devices:
            device.put()

        job_query = model.JobModel.query()
        jobs = job_query.fetch()
        for job in jobs:
            job.put()

        self.response.write("<pre>Indexing has been completed.</pre>")


class CreateBuildModelIndex(webapp2.RequestHandler):
    """Main class for /tasks/indexing/build.

    By fetch and put all entities, indexing all existing BuildModel entities.
    """

    def get(self):
        """Fetch and put all BuildModel entities"""
        build_query = model.BuildModel.query()
        builds = build_query.fetch()
        for build in builds:
            build.put()

        self.response.write("<pre>BuildModel indexing has been completed.</pre>")


class CreateDeviceModelIndex(webapp2.RequestHandler):
    """Main class for /tasks/indexing/device.

    By fetch and put all entities, indexing all existing DeviceModel entities.
    """

    def get(self):
        """Fetch and put all DeviceModel entities"""
        device_query = model.DeviceModel.query()
        devices = device_query.fetch()
        for device in devices:
            device.put()

        self.response.write(
            "<pre>DeviceModel indexing has been completed.</pre>")


class CreateJobModelIndex(webapp2.RequestHandler):
    """Main class for /tasks/indexing/job.

    By fetch and put all entities, indexing all existing JobModel entities.
    """

    def get(self):
        """Fetch and put all JobModel entities"""
        job_query = model.JobModel.query()
        jobs = job_query.fetch()
        for job in jobs:
            job.put()

        self.response.write(
            "<pre>JobModel indexing has been completed.</pre>")


class CreateLabModelIndex(webapp2.RequestHandler):
    """Main class for /tasks/indexing/lab.

    By fetch and put all entities, indexing all existing LabModel entities.
    """

    def get(self):
        """Fetch and put all LabModel entities"""
        lab_query = model.LabModel.query()
        labs = lab_query.fetch()
        for lab in labs:
            lab.put()

        self.response.write(
            "<pre>LabModel indexing has been completed.</pre>")


class CreateScheduleModelIndex(webapp2.RequestHandler):
    """Main class for /tasks/indexing/schedule.

    By fetch and put all entities, indexing all existing ScheduleModel entities.
    """

    def get(self):
        """Fetch and put all ScheduleModel entities"""
        schedule_query = model.ScheduleModel.query()
        schedules = schedule_query.fetch()
        for schedule in schedules:
            if schedule.build_storage_type is None:
                schedule.build_storage_type = Status.STORAGE_TYPE_DICT["PAB"]
            schedule.put()

        self.response.write(
            "<pre>ScheduleModel indexing has been completed.</pre>")
