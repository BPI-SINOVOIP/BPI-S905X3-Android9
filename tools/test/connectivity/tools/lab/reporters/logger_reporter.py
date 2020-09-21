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

import logging

from reporters.reporter import Reporter


class LoggerReporter(Reporter):
    def report(self, metric_responses):
        # Extra formatter options.
        extra = {
            'metric_name': None,
            'response_key': None,
            'response_val': None
        }

        logger = logging.getLogger(__name__)
        logger.setLevel(logging.INFO)
        # Stop logger from print to stdout.
        logger.propagate = False

        handler = logging.FileHandler('lab_health.log')
        handler.setLevel(logging.INFO)

        formatter = logging.Formatter(
            '%(asctime)s: %(metric_name)s (%(response_key)s %(response_val)s)')
        handler.setFormatter(formatter)
        logger.addHandler(handler)

        logger = logging.LoggerAdapter(logger, extra)
        # add the handlers to the logger
        for metric in metric_responses:
            extra['metric_name'] = metric
            for response in metric_responses[metric]:
                extra['response_key'] = response
                extra['response_val'] = metric_responses[metric][response]
                logger.info(None)
