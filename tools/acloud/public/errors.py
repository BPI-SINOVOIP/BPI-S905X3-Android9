#!/usr/bin/env python
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

"""Define errors that are raised by the driver."""

import json

HTTP_NOT_FOUND_CODE = 404


class DriverError(Exception):
    """Base Android Gce driver exception."""


class ConfigError(DriverError):
    """Error related to config."""


class CommandArgError(DriverError):
    """Error related to command line args."""


class GceOperationTimeoutError(DriverError):
    """Error raised when a GCE operation timedout."""


class HttpError(DriverError):
    """Error related to http requests."""

    def __init__(self, code, message):
        self.code = code
        super(HttpError, self).__init__(message)

    @staticmethod
    def CreateFromHttpError(http_error):
        """Create from an apiclient.errors.HttpError.

        Parse the error code from apiclient.errors.HttpError
        and create an instance of HttpError from this module
        that has the error code.

        Args:
            http_error: An apiclient.errors.HttpError instance.

        Returns:
            An HttpError instance from this module.
        """
        return HttpError(http_error.resp.status, str(http_error))


class ResourceNotFoundError(HttpError):
    """Error raised when a resource is not found."""


class InvalidVirtualDeviceIpError(DriverError):
    """Invalid virtual device's IP is set.

    Raise this when the virtual device's IP of an AVD instance is invalid.
    """


class DeviceBootTimeoutError(DriverError):
    """Raised when an AVD defice failed to boot within timeout."""


class HasRetriableRequestsError(DriverError):
    """Raised when some retriable requests fail in a batch execution."""


class AuthentcationError(DriverError):
    """Raised when authentication fails."""
