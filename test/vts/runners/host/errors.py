#
# Copyright (C) 2016 The Android Open Source Project
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

from vts.runners.host import signals


class VtsError(Exception):
    """Raised for general VTS exceptions."""


class BaseTestError(VtsError):
    """Raised for exceptions that occured in BaseTestClass."""


class USERError(VtsError):
    """Raised when a problem is caused by user mistake, e.g. wrong command,
    misformatted config, test info, wrong test paths etc.
    """


class ComponentLoadingError(VtsError):
    """Raised if it is unable to load a target component on a target device."""


class ConnectionRefusedError(VtsError):
    """Failure due to wrong port number.

      Raised when an attempt to connect to a TCP server fails due to incorrect
      port number, such as 21.
    """


class VtsTcpClientCreationError(VtsError):
    """Raised when TcpClient creation failed."""


class VtsTcpCommunicationError(VtsError):
    """Raised when TcpClient communication failed."""


class VtsUnsupportedTypeError(VtsError):
    """Raised when an unsupported type is used."""


class VtsMalformedProtoStringError(VtsError):
    """Raised when a StringDataValueMessage is not populated or corrupted."""

