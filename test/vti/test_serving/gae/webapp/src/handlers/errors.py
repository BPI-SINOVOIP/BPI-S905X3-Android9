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

import httplib


class Error(Exception):
    """Base Exception for handlers.

    Attributes:
        code: HTTP 1.1 status code
        msg: the W3C names for HTTP 1.1 status code
    """

    def __init__(self, code=None, msg=None):
        self.code = code or httplib.INTERNAL_SERVER_ERROR
        self.msg = msg or httplib.responses[self.code]
        super(Error, self).__init__(self.code, self.msg)

    def __str__(self):
        return repr([self.code, self.msg])


class FormValidationError(Error):
    """Form Validtion Exception for handlers."""

    def __init__(self, code=None, msg=None, errors=None):
        self.code = code or httplib.BAD_REQUEST
        self.msg = msg or httplib.responses[self.code]
        self.errors = errors or []
        super(FormValidationError, self).__init__(self.code, self.msg)

    def __str__(self):
        return repr([self.code, self.msg, self.errors])


class AclError(Error):
    """Base ACL error."""

    def __init__(self, code):
        self.code = code
        self.msg = httplib.responses[code]
        super(AclError, self).__init__(self.code, self.msg)

    def __str__(self):
        return repr([self.code, self.msg])


class NotFoundError(AclError):
    """The item being access does not exist."""

    def __init__(self):
        super(NotFoundError, self).__init__(httplib.NOT_FOUND)


class UnauthorizedError(AclError):
    """The current user is not authenticated."""

    def __init__(self):
        super(UnauthorizedError, self).__init__(httplib.UNAUTHORIZED)


class ForbiddenError(AclError):
    """The current user does not have proper permission."""

    def __init__(self):
        super(ForbiddenError, self).__init__(httplib.FORBIDDEN)
