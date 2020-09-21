#!/usr/bin/env python3.4
#
# Copyright 2016 - The Android Open Source Project
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

import inspect
import os


class TraceLogger():
    def __init__(self, logger):
        self._logger = logger

    @staticmethod
    def _get_trace_info(level=1):
        # We want the stack frame above this and above the error/warning/info
        inspect_stack = inspect.stack()
        trace_info = ""
        for i in range(level):
            try:
                stack_frames = inspect_stack[2 + i]
                info = inspect.getframeinfo(stack_frames[0])
                trace_info = "%s[%s:%s:%s]" % (trace_info,
                                               os.path.basename(info.filename),
                                               info.function, info.lineno)
            except IndexError:
                break
        return trace_info

    def debug(self, msg, *args, **kwargs):
        trace_info = TraceLogger._get_trace_info(level=3)
        self._logger.debug("%s %s" % (msg, trace_info), *args, **kwargs)

    def error(self, msg, *args, **kwargs):
        trace_info = TraceLogger._get_trace_info(level=3)
        self._logger.error("%s %s" % (msg, trace_info), *args, **kwargs)

    def warn(self, msg, *args, **kwargs):
        trace_info = TraceLogger._get_trace_info(level=1)
        self._logger.warn("%s %s" % (msg, trace_info), *args, **kwargs)

    def warning(self, msg, *args, **kwargs):
        trace_info = TraceLogger._get_trace_info(level=1)
        self._logger.warning("%s %s" % (msg, trace_info), *args, **kwargs)

    def info(self, msg, *args, **kwargs):
        trace_info = TraceLogger._get_trace_info(level=1)
        self._logger.info("%s %s" % (msg, trace_info), *args, **kwargs)

    def __getattr__(self, name):
        return getattr(self._logger, name)
