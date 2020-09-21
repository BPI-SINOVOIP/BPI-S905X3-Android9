# -*- coding:utf-8 -*-
# Copyright 2016 The Android Open Source Project
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

"""Common errors thrown when repo presubmit checks fail."""

from __future__ import print_function

import os
import sys

_path = os.path.realpath(__file__ + '/../..')
if sys.path[0] != _path:
    sys.path.insert(0, _path)
del _path


class HookResult(object):
    """A single hook result."""

    def __init__(self, hook, project, commit, error, files=(), fixup_func=None):
        """Initialize.

        Args:
          hook: The name of the hook.
          project: The name of the project.
          commit: The git commit sha.
          error: A string representation of the hook's result.  Empty on
              success.
          files: The list of files that were involved in the hook execution.
          fixup_func: A callable that will attempt to automatically fix errors
              found in the hook's execution.  Returns an non-empty string if
              this, too, fails.  Can be None if the hook does not support
              automatically fixing errors.
        """
        self.hook = hook
        self.project = project
        self.commit = commit
        self.error = error
        self.files = files
        self.fixup_func = fixup_func

    def __bool__(self):
        return bool(self.error)

    def __nonzero__(self):
        """Python 2/3 glue."""
        return self.__bool__()

    def is_warning(self):
        return False


class HookCommandResult(HookResult):
    """A single hook result based on a CommandResult."""

    def __init__(self, hook, project, commit, result, files=(),
                 fixup_func=None):
        HookResult.__init__(self, hook, project, commit,
                            result.error if result.error else result.output,
                            files=files, fixup_func=fixup_func)
        self.result = result

    def __bool__(self):
        return self.result.returncode not in (None, 0)

    def is_warning(self):
        return self.result.returncode == 77
