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

"""Manage various config files."""

from __future__ import print_function

import ConfigParser
import functools
import os
import shlex
import sys

_path = os.path.realpath(__file__ + '/../..')
if sys.path[0] != _path:
    sys.path.insert(0, _path)
del _path

# pylint: disable=wrong-import-position
import rh.hooks
import rh.shell


class Error(Exception):
    """Base exception class."""


class ValidationError(Error):
    """Config file has unknown sections/keys or other values."""


class RawConfigParser(ConfigParser.RawConfigParser):
    """Like RawConfigParser but with some default helpers."""

    @staticmethod
    def _check_args(name, cnt_min, cnt_max, args):
        cnt = len(args)
        if cnt not in (0, cnt_max - cnt_min):
            raise TypeError('%s() takes %i or %i arguments (got %i)' %
                            (name, cnt_min, cnt_max, cnt,))
        return cnt

    def options(self, section, *args):
        """Return the options in |section| (with default |args|).

        Args:
          section: The section to look up.
          args: What to return if |section| does not exist.
        """
        cnt = self._check_args('options', 2, 3, args)
        try:
            return ConfigParser.RawConfigParser.options(self, section)
        except ConfigParser.NoSectionError:
            if cnt == 1:
                return args[0]
            raise

    def get(self, section, option, *args):
        """Return the value for |option| in |section| (with default |args|)."""
        cnt = self._check_args('get', 3, 4, args)
        try:
            return ConfigParser.RawConfigParser.get(self, section, option)
        except (ConfigParser.NoSectionError, ConfigParser.NoOptionError):
            if cnt == 1:
                return args[0]
            raise

    def items(self, section, *args):
        """Return a list of (key, value) tuples for the options in |section|."""
        cnt = self._check_args('items', 2, 3, args)
        try:
            return ConfigParser.RawConfigParser.items(self, section)
        except ConfigParser.NoSectionError:
            if cnt == 1:
                return args[0]
            raise


class PreSubmitConfig(object):
    """Config file used for per-project `repo upload` hooks."""

    FILENAME = 'PREUPLOAD.cfg'
    GLOBAL_FILENAME = 'GLOBAL-PREUPLOAD.cfg'

    CUSTOM_HOOKS_SECTION = 'Hook Scripts'
    BUILTIN_HOOKS_SECTION = 'Builtin Hooks'
    BUILTIN_HOOKS_OPTIONS_SECTION = 'Builtin Hooks Options'
    TOOL_PATHS_SECTION = 'Tool Paths'
    OPTIONS_SECTION = 'Options'

    OPTION_IGNORE_MERGED_COMMITS = 'ignore_merged_commits'
    VALID_OPTIONS = (OPTION_IGNORE_MERGED_COMMITS,)

    def __init__(self, paths=('',), global_paths=()):
        """Initialize.

        All the config files found will be merged together in order.

        Args:
          paths: The directories to look for config files.
          global_paths: The directories to look for global config files.
        """
        config = RawConfigParser()

        def _search(paths, filename):
            for path in paths:
                path = os.path.join(path, filename)
                if os.path.exists(path):
                    self.paths.append(path)
                    try:
                        config.read(path)
                    except ConfigParser.ParsingError as e:
                        raise ValidationError('%s: %s' % (path, e))

        self.paths = []
        _search(global_paths, self.GLOBAL_FILENAME)
        _search(paths, self.FILENAME)

        self.config = config

        self._validate()

    @property
    def custom_hooks(self):
        """List of custom hooks to run (their keys/names)."""
        return self.config.options(self.CUSTOM_HOOKS_SECTION, [])

    def custom_hook(self, hook):
        """The command to execute for |hook|."""
        return shlex.split(self.config.get(self.CUSTOM_HOOKS_SECTION, hook, ''))

    @property
    def builtin_hooks(self):
        """List of all enabled builtin hooks (their keys/names)."""
        return [k for k, v in self.config.items(self.BUILTIN_HOOKS_SECTION, ())
                if rh.shell.boolean_shell_value(v, None)]

    def builtin_hook_option(self, hook):
        """The options to pass to |hook|."""
        return shlex.split(self.config.get(self.BUILTIN_HOOKS_OPTIONS_SECTION,
                                           hook, ''))

    @property
    def tool_paths(self):
        """List of all tool paths."""
        return dict(self.config.items(self.TOOL_PATHS_SECTION, ()))

    def callable_hooks(self):
        """Yield a name and callback for each hook to be executed."""
        for hook in self.custom_hooks:
            options = rh.hooks.HookOptions(hook,
                                           self.custom_hook(hook),
                                           self.tool_paths)
            yield (hook, functools.partial(rh.hooks.check_custom,
                                           options=options))

        for hook in self.builtin_hooks:
            options = rh.hooks.HookOptions(hook,
                                           self.builtin_hook_option(hook),
                                           self.tool_paths)
            yield (hook, functools.partial(rh.hooks.BUILTIN_HOOKS[hook],
                                           options=options))

    @property
    def ignore_merged_commits(self):
        """Whether to skip hooks for merged commits."""
        return rh.shell.boolean_shell_value(
            self.config.get(self.OPTIONS_SECTION,
                            self.OPTION_IGNORE_MERGED_COMMITS, None),
            False)

    def _validate(self):
        """Run consistency checks on the config settings."""
        config = self.config

        # Reject unknown sections.
        valid_sections = set((
            self.CUSTOM_HOOKS_SECTION,
            self.BUILTIN_HOOKS_SECTION,
            self.BUILTIN_HOOKS_OPTIONS_SECTION,
            self.TOOL_PATHS_SECTION,
            self.OPTIONS_SECTION,
        ))
        bad_sections = set(config.sections()) - valid_sections
        if bad_sections:
            raise ValidationError('%s: unknown sections: %s' %
                                  (self.paths, bad_sections))

        # Reject blank custom hooks.
        for hook in self.custom_hooks:
            if not config.get(self.CUSTOM_HOOKS_SECTION, hook):
                raise ValidationError('%s: custom hook "%s" cannot be blank' %
                                      (self.paths, hook))

        # Reject unknown builtin hooks.
        valid_builtin_hooks = set(rh.hooks.BUILTIN_HOOKS.keys())
        if config.has_section(self.BUILTIN_HOOKS_SECTION):
            hooks = set(config.options(self.BUILTIN_HOOKS_SECTION))
            bad_hooks = hooks - valid_builtin_hooks
            if bad_hooks:
                raise ValidationError('%s: unknown builtin hooks: %s' %
                                      (self.paths, bad_hooks))
        elif config.has_section(self.BUILTIN_HOOKS_OPTIONS_SECTION):
            raise ValidationError('Builtin hook options specified, but missing '
                                  'builtin hook settings')

        if config.has_section(self.BUILTIN_HOOKS_OPTIONS_SECTION):
            hooks = set(config.options(self.BUILTIN_HOOKS_OPTIONS_SECTION))
            bad_hooks = hooks - valid_builtin_hooks
            if bad_hooks:
                raise ValidationError('%s: unknown builtin hook options: %s' %
                                      (self.paths, bad_hooks))

        # Verify hooks are valid shell strings.
        for hook in self.custom_hooks:
            try:
                self.custom_hook(hook)
            except ValueError as e:
                raise ValidationError('%s: hook "%s" command line is invalid: '
                                      '%s' % (self.paths, hook, e))

        # Verify hook options are valid shell strings.
        for hook in self.builtin_hooks:
            try:
                self.builtin_hook_option(hook)
            except ValueError as e:
                raise ValidationError('%s: hook options "%s" are invalid: %s' %
                                      (self.paths, hook, e))

        # Reject unknown tools.
        valid_tools = set(rh.hooks.TOOL_PATHS.keys())
        if config.has_section(self.TOOL_PATHS_SECTION):
            tools = set(config.options(self.TOOL_PATHS_SECTION))
            bad_tools = tools - valid_tools
            if bad_tools:
                raise ValidationError('%s: unknown tools: %s' %
                                      (self.paths, bad_tools))

        # Reject unknown options.
        valid_options = set(self.VALID_OPTIONS)
        if config.has_section(self.OPTIONS_SECTION):
            options = set(config.options(self.OPTIONS_SECTION))
            bad_options = options - valid_options
            if bad_options:
                raise ValidationError('%s: unknown options: %s' %
                                      (self.paths, bad_options))
