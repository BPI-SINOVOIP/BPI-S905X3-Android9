#!/usr/bin/env python
#
# Copyright (C) 2018 The Android Open Source Project
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

import datetime
import webapp2

from webapp.src import vtslab_status as Status
from webapp.src.proto import model
from webapp.src.utils import logger

JOB_RESPONSE_TIMEOUT_SECONDS = 300


class PeriodicJobHeartBeat(webapp2.RequestHandler):
    """Main class for /tasks/job_heartbeat.

    Used to find lost jobs and change their status properly.

    Attributes:
        logger: Logger class
    """
    logger = logger.Logger()

    def get(self):
        """Generates an HTML page based on the jobs kept in DB."""
        self.logger.Clear()

        job_query = model.JobModel.query(
            model.JobModel.status == Status.JOB_STATUS_DICT["leased"]
        )
        jobs = job_query.fetch()

        lost_jobs = []
        for job in jobs:
            if job.heartbeat_stamp:
                job_timestamp = job.heartbeat_stamp
            else:
                job_timestamp = job.timestamp
            if (datetime.datetime.now() -
                    job_timestamp).seconds >= JOB_RESPONSE_TIMEOUT_SECONDS:
                lost_jobs.append(job)

        for job in lost_jobs:
            self.logger.Println("Lost job found")
            self.logger.Println(
                "[hostname]{} [device]{} [test_name]{}".format(
                    job.hostname, job.device, job.test_name))
            job.status = Status.JOB_STATUS_DICT["infra-err"]
            job.put()

            device_query = model.DeviceModel.query(
                model.DeviceModel.serial.IN(job.serial)
            )
            devices = device_query.fetch()

            for device in devices:
                device.scheduling_status = Status.DEVICE_SCHEDULING_STATUS_DICT[
                    "free"]
                device.put()

        self.response.write(
            "<pre>\n" + "\n".join(self.logger.Get()) + "\n</pre>")
