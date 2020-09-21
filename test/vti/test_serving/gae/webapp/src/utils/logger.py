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


class Logger(object):
    """A class to log messages to a list of strings.

    Attributes:
        log_message: a list of strings, containing the log messages.
        log_indent: integer, representing the index level
    """

    def __init__(self):
        self.log_message = []
        self.log_indent = 0

    def Clear(self):
        """Clears the log buffer."""
        self.log_message = []
        self.log_indent = 0

    def Get(self):
        """Retruns a list of all log message strings."""
        return self.log_message
        
    def Println(self, msg):
        """Stores a new string `msg` to the log buffer."""
        indent = "  " * self.log_indent
        self.log_message.append(indent + msg)

    def Indent(self):
        """Increase indent of log message."""
        self.log_indent += 1

    def Unindent(self):
        """Decrease indent of log message."""
        if self.log_indent > 0:
            self.log_indent -= 1
