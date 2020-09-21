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

import datetime

from webapp.src.handlers.base import BaseHandler
from webapp.src.proto import model


class DevicePage(BaseHandler):
    """Main class for /device web page."""

    def get(self):
        """Generates an HTML page based on the device info kept in DB."""
        self.template = "device.html"

        device_query = model.DeviceModel.query()
        devices = device_query.fetch()

        lab_query = model.LabModel.query()
        labs = lab_query.fetch()

        if devices:
            devices = sorted(
                devices, key=lambda x: (x.hostname, x.product, x.status),
                reverse=False)

        template_values = {
            "now": datetime.datetime.now(),
            "devices": devices,
            "labs": labs,
        }

        self.render(template_values)
