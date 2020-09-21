# Copyright 2018, The Android Open Source Project
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

"""
atest exceptions.
"""


class UnsupportedModuleTestError(Exception):
    """Error raised when we find a module that we don't support."""

class TestDiscoveryException(Exception):
    """Base Exception for issues with test discovery."""

class NoTestFoundError(TestDiscoveryException):
    """Raised when no tests are found."""

class TestWithNoModuleError(TestDiscoveryException):
    """Raised when test files have no parent module directory."""

class MissingPackageNameError(Exception):
    """Raised when the test class java file does not contain a package name."""

class TooManyMethodsError(Exception):
    """Raised when input string contains more than one # character."""

class MethodWithoutClassError(Exception):
    """Raised when method is appended via # but no class file specified."""

class UnknownTestRunnerError(Exception):
    """Raised when an unknown test runner is specified."""

class NoTestRunnerName(Exception):
    """Raised when Test Runner class var NAME isn't defined."""

class NoTestRunnerExecutable(Exception):
    """Raised when Test Runner class var EXECUTABLE isn't defined."""

class HostEnvCheckFailed(Exception):
    """Raised when Test Runner's host env check fails."""

class ShouldNeverBeCalledError(Exception):
    """Raised when something is called when it shouldn't, used for testing."""

class FatalIncludeError(SyntaxError):
    """Raised if expanding include tag fails."""
