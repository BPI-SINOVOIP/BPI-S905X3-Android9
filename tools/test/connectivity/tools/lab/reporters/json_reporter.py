#!/usr/bin/env python
#
#   Copyright 2017 - The Android Open Source Project
#
#   Licensed under the Apache License, Version 2.0 (the "License");
#   you may not use this file except in compliance with the License.
#   You may obtain a copy of the License at
#
#       http://www.apache.org/licenses/LICENSE-2.0
#
#   Unless required by applicable law or agreed to in writing, software
#   distributed under the License is distributed on an "AS IS" BASIS,
#   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#   See the License for the specific language governing permissions and
#   limitations under the License.

import json

from metrics.usb_metric import Device
from reporters.reporter import Reporter


class JsonReporter(Reporter):
    """Reporter class that reports information in JSON format to a file.

    This defaults to writing to the current working directory with name
    output.json

    Attributes:
      health_checker: a HealthChecker object
      file_name: Path of file to write to.
    """

    def __init__(self, h_checker, file_name='output.json'):
        super(JsonReporter, self).__init__(h_checker)
        self.file_name = file_name

    def report(self, metric_responses):
        unhealthy_metrics = self.health_checker.get_unhealthy(metric_responses)
        for metric_name in metric_responses:
            if metric_name not in unhealthy_metrics:
                metric_responses[metric_name]['is_healthy'] = True
            else:
                metric_responses[metric_name]['is_healthy'] = False
        # add a total unhealthy score
        metric_responses['total_unhealthy'] = {
            'total_unhealthy': len(set(unhealthy_metrics))
        }
        with open(self.file_name, 'w') as outfile:
            json.dump(
                metric_responses, indent=4, cls=AutoJsonEncoder, fp=outfile)


class AutoJsonEncoder(json.JSONEncoder):
    def default(self, obj):
        if isinstance(obj, Device):
            return {
                'name': obj.name,
                'trans_bytes': obj.trans_bytes,
                'dev_id': obj.dev_id
            }
        else:
            return json.JSONEncoder.default(self, obj)
