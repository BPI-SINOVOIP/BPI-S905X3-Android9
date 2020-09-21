# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.


class UnknownMessageTypeError(ValueError):
    """Indicates that a message with an unrecoganized type was received."""


class WorkerTimeoutError(RuntimeError):
    """Indicates that a factory worker timed out."""
    def __init__(self):
        super(WorkerTimeoutError, self).__init__('worker timed out')
