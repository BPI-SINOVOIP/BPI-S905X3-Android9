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

"""Functions that implement the actual checks."""

from __future__ import print_function

import json
import os
import platform
import re
import sys

_path = os.path.realpath(__file__ + '/../..')
if sys.path[0] != _path:
    sys.path.insert(0, _path)
del _path

# pylint: disable=wrong-import-position
import rh.results
import rh.git
import rh.utils


class Placeholders(object):
    """Holder class for replacing ${vars} in arg lists.

    To add a new variable to replace in config files, just add it as a @property
    to this class using the form.  So to add support for BIRD:
      @property
      def var_BIRD(self):
        return <whatever this is>

    You can return either a string or an iterable (e.g. a list or tuple).
    """

    def __init__(self, diff=()):
        """Initialize.

        Args:
          diff: The list of files that changed.
        """
        self.diff = diff

    def expand_vars(self, args):
        """Perform place holder expansion on all of |args|.

        Args:
          args: The args to perform expansion on.

        Returns:
          The updated |args| list.
        """
        all_vars = set(self.vars())
        replacements = dict((var, self.get(var)) for var in all_vars)

        ret = []
        for arg in args:
            # First scan for exact matches
            for key, val in replacements.items():
                var = '${%s}' % (key,)
                if arg == var:
                    if isinstance(val, str):
                        ret.append(val)
                    else:
                        ret.extend(val)
                    # We break on first hit to avoid double expansion.
                    break
            else:
                # If no exact matches, do an inline replacement.
                def replace(m):
                    val = self.get(m.group(1))
                    if isinstance(val, str):
                        return val
                    else:
                        return ' '.join(val)
                ret.append(re.sub(r'\$\{(%s)\}' % ('|'.join(all_vars),),
                                  replace, arg))

        return ret

    @classmethod
    def vars(cls):
        """Yield all replacement variable names."""
        for key in dir(cls):
            if key.startswith('var_'):
                yield key[4:]

    def get(self, var):
        """Helper function to get the replacement |var| value."""
        return getattr(self, 'var_%s' % (var,))

    @property
    def var_PREUPLOAD_COMMIT_MESSAGE(self):
        """The git commit message."""
        return os.environ.get('PREUPLOAD_COMMIT_MESSAGE', '')

    @property
    def var_PREUPLOAD_COMMIT(self):
        """The git commit sha1."""
        return os.environ.get('PREUPLOAD_COMMIT', '')

    @property
    def var_PREUPLOAD_FILES(self):
        """List of files modified in this git commit."""
        return [x.file for x in self.diff if x.status != 'D']

    @property
    def var_REPO_ROOT(self):
        """The root of the repo checkout."""
        return rh.git.find_repo_root()

    @property
    def var_BUILD_OS(self):
        """The build OS (see _get_build_os_name for details)."""
        return _get_build_os_name()


class HookOptions(object):
    """Holder class for hook options."""

    def __init__(self, name, args, tool_paths):
        """Initialize.

        Args:
          name: The name of the hook.
          args: The override commandline arguments for the hook.
          tool_paths: A dictionary with tool names to paths.
        """
        self.name = name
        self._args = args
        self._tool_paths = tool_paths

    @staticmethod
    def expand_vars(args, diff=()):
        """Perform place holder expansion on all of |args|."""
        replacer = Placeholders(diff=diff)
        return replacer.expand_vars(args)

    def args(self, default_args=(), diff=()):
        """Gets the hook arguments, after performing place holder expansion.

        Args:
          default_args: The list to return if |self._args| is empty.
          diff: The list of files that changed in the current commit.

        Returns:
          A list with arguments.
        """
        args = self._args
        if not args:
            args = default_args

        return self.expand_vars(args, diff=diff)

    def tool_path(self, tool_name):
        """Gets the path in which the |tool_name| executable can be found.

        This function performs expansion for some place holders.  If the tool
        does not exist in the overridden |self._tool_paths| dictionary, the tool
        name will be returned and will be run from the user's $PATH.

        Args:
          tool_name: The name of the executable.

        Returns:
          The path of the tool with all optional place holders expanded.
        """
        assert tool_name in TOOL_PATHS
        if tool_name not in self._tool_paths:
            return TOOL_PATHS[tool_name]

        tool_path = os.path.normpath(self._tool_paths[tool_name])
        return self.expand_vars([tool_path])[0]


def _run_command(cmd, **kwargs):
    """Helper command for checks that tend to gather output."""
    kwargs.setdefault('redirect_stderr', True)
    kwargs.setdefault('combine_stdout_stderr', True)
    kwargs.setdefault('capture_output', True)
    kwargs.setdefault('error_code_ok', True)
    return rh.utils.run_command(cmd, **kwargs)


def _match_regex_list(subject, expressions):
    """Try to match a list of regular expressions to a string.

    Args:
      subject: The string to match regexes on.
      expressions: An iterable of regular expressions to check for matches with.

    Returns:
      Whether the passed in subject matches any of the passed in regexes.
    """
    for expr in expressions:
        if re.search(expr, subject):
            return True
    return False


def _filter_diff(diff, include_list, exclude_list=()):
    """Filter out files based on the conditions passed in.

    Args:
      diff: list of diff objects to filter.
      include_list: list of regex that when matched with a file path will cause
          it to be added to the output list unless the file is also matched with
          a regex in the exclude_list.
      exclude_list: list of regex that when matched with a file will prevent it
          from being added to the output list, even if it is also matched with a
          regex in the include_list.

    Returns:
      A list of filepaths that contain files matched in the include_list and not
      in the exclude_list.
    """
    filtered = []
    for d in diff:
        if (d.status != 'D' and
                _match_regex_list(d.file, include_list) and
                not _match_regex_list(d.file, exclude_list)):
            # We've got a match!
            filtered.append(d)
    return filtered


def _get_build_os_name():
    """Gets the build OS name.

    Returns:
      A string in a format usable to get prebuilt tool paths.
    """
    system = platform.system()
    if 'Darwin' in system or 'Macintosh' in system:
        return 'darwin-x86'
    else:
        # TODO: Add more values if needed.
        return 'linux-x86'


def _fixup_func_caller(cmd, **kwargs):
    """Wraps |cmd| around a callable automated fixup.

    For hooks that support automatically fixing errors after running (e.g. code
    formatters), this function provides a way to run |cmd| as the |fixup_func|
    parameter in HookCommandResult.
    """
    def wrapper():
        result = _run_command(cmd, **kwargs)
        if result.returncode not in (None, 0):
            return result.output
        return None
    return wrapper


def _check_cmd(hook_name, project, commit, cmd, fixup_func=None, **kwargs):
    """Runs |cmd| and returns its result as a HookCommandResult."""
    return [rh.results.HookCommandResult(hook_name, project, commit,
                                         _run_command(cmd, **kwargs),
                                         fixup_func=fixup_func)]


# Where helper programs exist.
TOOLS_DIR = os.path.realpath(__file__ + '/../../tools')

def get_helper_path(tool):
    """Return the full path to the helper |tool|."""
    return os.path.join(TOOLS_DIR, tool)


def check_custom(project, commit, _desc, diff, options=None, **kwargs):
    """Run a custom hook."""
    return _check_cmd(options.name, project, commit, options.args((), diff),
                      **kwargs)


def check_checkpatch(project, commit, _desc, diff, options=None):
    """Run |diff| through the kernel's checkpatch.pl tool."""
    tool = get_helper_path('checkpatch.pl')
    cmd = ([tool, '-', '--root', project.dir] +
           options.args(('--ignore=GERRIT_CHANGE_ID',), diff))
    return _check_cmd('checkpatch.pl', project, commit, cmd,
                      input=rh.git.get_patch(commit))


def check_clang_format(project, commit, _desc, diff, options=None):
    """Run git clang-format on the commit."""
    tool = get_helper_path('clang-format.py')
    clang_format = options.tool_path('clang-format')
    git_clang_format = options.tool_path('git-clang-format')
    tool_args = (['--clang-format', clang_format, '--git-clang-format',
                  git_clang_format] +
                 options.args(('--style', 'file', '--commit', commit), diff))
    cmd = [tool] + tool_args
    fixup_func = _fixup_func_caller([tool, '--fix'] + tool_args)
    return _check_cmd('clang-format', project, commit, cmd,
                      fixup_func=fixup_func)


def check_google_java_format(project, commit, _desc, _diff, options=None):
    """Run google-java-format on the commit."""

    tool = get_helper_path('google-java-format.py')
    google_java_format = options.tool_path('google-java-format')
    google_java_format_diff = options.tool_path('google-java-format-diff')
    tool_args = ['--google-java-format', google_java_format,
                 '--google-java-format-diff', google_java_format_diff,
                 '--commit', commit] + options.args()
    cmd = [tool] + tool_args
    fixup_func = _fixup_func_caller([tool, '--fix'] + tool_args)
    return _check_cmd('google-java-format', project, commit, cmd,
                      fixup_func=fixup_func)


def check_commit_msg_bug_field(project, commit, desc, _diff, options=None):
    """Check the commit message for a 'Bug:' line."""
    field = 'Bug'
    regex = r'^%s: (None|[0-9]+(, [0-9]+)*)$' % (field,)
    check_re = re.compile(regex)

    if options.args():
        raise ValueError('commit msg %s check takes no options' % (field,))

    found = []
    for line in desc.splitlines():
        if check_re.match(line):
            found.append(line)

    if not found:
        error = ('Commit message is missing a "%s:" line.  It must match:\n'
                 '%s') % (field, regex)
    else:
        return

    return [rh.results.HookResult('commit msg: "%s:" check' % (field,),
                                  project, commit, error=error)]


def check_commit_msg_changeid_field(project, commit, desc, _diff, options=None):
    """Check the commit message for a 'Change-Id:' line."""
    field = 'Change-Id'
    regex = r'^%s: I[a-f0-9]+$' % (field,)
    check_re = re.compile(regex)

    if options.args():
        raise ValueError('commit msg %s check takes no options' % (field,))

    found = []
    for line in desc.splitlines():
        if check_re.match(line):
            found.append(line)

    if len(found) == 0:
        error = ('Commit message is missing a "%s:" line.  It must match:\n'
                 '%s') % (field, regex)
    elif len(found) > 1:
        error = ('Commit message has too many "%s:" lines.  There can be only '
                 'one.') % (field,)
    else:
        return

    return [rh.results.HookResult('commit msg: "%s:" check' % (field,),
                                  project, commit, error=error)]


TEST_MSG = """Commit message is missing a "Test:" line.  It must match:
%s

The Test: stanza is free-form and should describe how you tested your change.
As a CL author, you'll have a consistent place to describe the testing strategy
you use for your work. As a CL reviewer, you'll be reminded to discuss testing
as part of your code review, and you'll more easily replicate testing when you
patch in CLs locally.

Some examples below:

Test: make WITH_TIDY=1 mmma art
Test: make test-art
Test: manual - took a photo
Test: refactoring CL. Existing unit tests still pass.

Check the git history for more examples. It's a free-form field, so we urge
you to develop conventions that make sense for your project. Note that many
projects use exact test commands, which are perfectly fine.

Adding good automated tests with new code is critical to our goals of keeping
the system stable and constantly improving quality. Please use Test: to
highlight this area of your development. And reviewers, please insist on
high-quality Test: descriptions.
"""


def check_commit_msg_test_field(project, commit, desc, _diff, options=None):
    """Check the commit message for a 'Test:' line."""
    field = 'Test'
    regex = r'^%s: .*$' % (field,)
    check_re = re.compile(regex)

    if options.args():
        raise ValueError('commit msg %s check takes no options' % (field,))

    found = []
    for line in desc.splitlines():
        if check_re.match(line):
            found.append(line)

    if not found:
        error = TEST_MSG % (regex)
    else:
        return

    return [rh.results.HookResult('commit msg: "%s:" check' % (field,),
                                  project, commit, error=error)]


def check_cpplint(project, commit, _desc, diff, options=None):
    """Run cpplint."""
    # This list matches what cpplint expects.  We could run on more (like .cxx),
    # but cpplint would just ignore them.
    filtered = _filter_diff(diff, [r'\.(cc|h|cpp|cu|cuh)$'])
    if not filtered:
        return

    cpplint = options.tool_path('cpplint')
    cmd = [cpplint] + options.args(('${PREUPLOAD_FILES}',), filtered)
    return _check_cmd('cpplint', project, commit, cmd)


def check_gofmt(project, commit, _desc, diff, options=None):
    """Checks that Go files are formatted with gofmt."""
    filtered = _filter_diff(diff, [r'\.go$'])
    if not filtered:
        return

    gofmt = options.tool_path('gofmt')
    cmd = [gofmt, '-l'] + options.args((), filtered)
    ret = []
    for d in filtered:
        data = rh.git.get_file_content(commit, d.file)
        result = _run_command(cmd, input=data)
        if result.output:
            ret.append(rh.results.HookResult(
                'gofmt', project, commit, error=result.output,
                files=(d.file,)))
    return ret


def check_json(project, commit, _desc, diff, options=None):
    """Verify json files are valid."""
    if options.args():
        raise ValueError('json check takes no options')

    filtered = _filter_diff(diff, [r'\.json$'])
    if not filtered:
        return

    ret = []
    for d in filtered:
        data = rh.git.get_file_content(commit, d.file)
        try:
            json.loads(data)
        except ValueError as e:
            ret.append(rh.results.HookResult(
                'json', project, commit, error=str(e),
                files=(d.file,)))
    return ret


def check_pylint(project, commit, _desc, diff, options=None):
    """Run pylint."""
    filtered = _filter_diff(diff, [r'\.py$'])
    if not filtered:
        return

    pylint = options.tool_path('pylint')
    cmd = [
        get_helper_path('pylint.py'),
        '--executable-path', pylint,
    ] + options.args(('${PREUPLOAD_FILES}',), filtered)
    return _check_cmd('pylint', project, commit, cmd)


def check_xmllint(project, commit, _desc, diff, options=None):
    """Run xmllint."""
    # XXX: Should we drop most of these and probe for <?xml> tags?
    extensions = frozenset((
        'dbus-xml',  # Generated DBUS interface.
        'dia',       # File format for Dia.
        'dtd',       # Document Type Definition.
        'fml',       # Fuzzy markup language.
        'form',      # Forms created by IntelliJ GUI Designer.
        'fxml',      # JavaFX user interfaces.
        'glade',     # Glade user interface design.
        'grd',       # GRIT translation files.
        'iml',       # Android build modules?
        'kml',       # Keyhole Markup Language.
        'mxml',      # Macromedia user interface markup language.
        'nib',       # OS X Cocoa Interface Builder.
        'plist',     # Property list (for OS X).
        'pom',       # Project Object Model (for Apache Maven).
        'rng',       # RELAX NG schemas.
        'sgml',      # Standard Generalized Markup Language.
        'svg',       # Scalable Vector Graphics.
        'uml',       # Unified Modeling Language.
        'vcproj',    # Microsoft Visual Studio project.
        'vcxproj',   # Microsoft Visual Studio project.
        'wxs',       # WiX Transform File.
        'xhtml',     # XML HTML.
        'xib',       # OS X Cocoa Interface Builder.
        'xlb',       # Android locale bundle.
        'xml',       # Extensible Markup Language.
        'xsd',       # XML Schema Definition.
        'xsl',       # Extensible Stylesheet Language.
    ))

    filtered = _filter_diff(diff, [r'\.(%s)$' % '|'.join(extensions)])
    if not filtered:
        return

    # TODO: Figure out how to integrate schema validation.
    # XXX: Should we use python's XML libs instead?
    cmd = ['xmllint'] + options.args(('${PREUPLOAD_FILES}',), filtered)

    return _check_cmd('xmllint', project, commit, cmd)


# Hooks that projects can opt into.
# Note: Make sure to keep the top level README.md up to date when adding more!
BUILTIN_HOOKS = {
    'checkpatch': check_checkpatch,
    'clang_format': check_clang_format,
    'commit_msg_bug_field': check_commit_msg_bug_field,
    'commit_msg_changeid_field': check_commit_msg_changeid_field,
    'commit_msg_test_field': check_commit_msg_test_field,
    'cpplint': check_cpplint,
    'gofmt': check_gofmt,
    'google_java_format': check_google_java_format,
    'jsonlint': check_json,
    'pylint': check_pylint,
    'xmllint': check_xmllint,
}

# Additional tools that the hooks can call with their default values.
# Note: Make sure to keep the top level README.md up to date when adding more!
TOOL_PATHS = {
    'clang-format': 'clang-format',
    'cpplint': os.path.join(TOOLS_DIR, 'cpplint.py'),
    'git-clang-format': 'git-clang-format',
    'gofmt': 'gofmt',
    'google-java-format': 'google-java-format',
    'google-java-format-diff': 'google-java-format-diff.py',
    'pylint': 'pylint',
}
