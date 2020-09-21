"""\
Logic for control file generation.
"""

__author__ = 'showard@google.com (Steve Howard)'

import re, os

import common
from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib.cros import dev_server
from autotest_lib.frontend.afe import model_logic
from autotest_lib.server.cros.dynamic_suite import control_file_getter
from autotest_lib.server.cros.dynamic_suite import suite as SuiteBase
import frontend.settings

AUTOTEST_DIR = os.path.abspath(os.path.join(
    os.path.dirname(frontend.settings.__file__), '..'))

EMPTY_TEMPLATE = 'def step_init():\n'

CLIENT_STEP_TEMPLATE = "    job.next_step('step%d')\n"
SERVER_STEP_TEMPLATE = '    step%d()\n'


def _read_control_file(test):
    """Reads the test control file from local disk.

    @param test The test name.

    @return The test control file string.
    """
    control_file = open(os.path.join(AUTOTEST_DIR, test.path))
    control_contents = control_file.read()
    control_file.close()
    return control_contents


def _add_boilerplate_to_nested_steps(lines):
    """Adds boilerplate magic.

    @param lines The string of lines.

    @returns The string lines.
    """
    # Look for a line that begins with 'def step_init():' while
    # being flexible on spacing.  If it's found, this will be
    # a nested set of steps, so add magic to make it work.
    # See client/bin/job.py's step_engine for more info.
    if re.search(r'^(.*\n)*def\s+step_init\s*\(\s*\)\s*:', lines):
        lines += '\nreturn locals() '
        lines += '# Boilerplate magic for nested sets of steps'
    return lines


def _format_step(item, lines):
    """Format a line item.
    @param item The item number.
    @param lines The string of lines.

    @returns The string lines.
    """
    lines = _indent_text(lines, '    ')
    lines = 'def step%d():\n%s' % (item, lines)
    return lines


def _get_tests_stanza(tests, is_server, prepend=None, append=None,
                     client_control_file='', test_source_build=None):
    """ Constructs the control file test step code from a list of tests.

    @param tests A sequence of test control files to run.
    @param is_server bool, Is this a server side test?
    @param prepend A list of steps to prepend to each client test.
        Defaults to [].
    @param append A list of steps to append to each client test.
        Defaults to [].
    @param client_control_file If specified, use this text as the body of a
        final client control file to run after tests.  is_server must be False.
    @param test_source_build: Build to be used to retrieve test code. Default
                              to None.

    @returns The control file test code to be run.
    """
    assert not (client_control_file and is_server)
    if not prepend:
        prepend = []
    if not append:
        append = []
    if test_source_build:
        raw_control_files = _get_test_control_files_by_build(
                tests, test_source_build)
    else:
        raw_control_files = [_read_control_file(test) for test in tests]
    if client_control_file:
        # 'return locals()' is always appended in case the user forgot, it
        # is necessary to allow for nested step engine execution to work.
        raw_control_files.append(client_control_file + '\nreturn locals()')
    raw_steps = prepend + [_add_boilerplate_to_nested_steps(step)
                           for step in raw_control_files] + append
    steps = [_format_step(index, step)
             for index, step in enumerate(raw_steps)]
    if is_server:
        step_template = SERVER_STEP_TEMPLATE
        footer = '\n\nstep_init()\n'
    else:
        step_template = CLIENT_STEP_TEMPLATE
        footer = ''

    header = ''.join(step_template % i for i in xrange(len(steps)))
    return header + '\n' + '\n\n'.join(steps) + footer


def _indent_text(text, indent):
    """Indent given lines of python code avoiding indenting multiline
    quoted content (only for triple " and ' quoting for now).

    @param text The string of lines.
    @param indent The indent string.

    @return The indented string.
    """
    regex = re.compile('(\\\\*)("""|\'\'\')')

    res = []
    in_quote = None
    for line in text.splitlines():
        # if not within a multinline quote indent the line contents
        if in_quote:
            res.append(line)
        else:
            res.append(indent + line)

        while line:
            match = regex.search(line)
            if match:
                # for an even number of backslashes before the triple quote
                if len(match.group(1)) % 2 == 0:
                    if not in_quote:
                        in_quote = match.group(2)[0]
                    elif in_quote == match.group(2)[0]:
                        # if we found a matching end triple quote
                        in_quote = None
                line = line[match.end():]
            else:
                break

    return '\n'.join(res)


def _get_profiler_commands(profilers, is_server, profile_only):
    prepend, append = [], []
    if profile_only is not None:
        prepend.append("job.default_profile_only = %r" % profile_only)
    for profiler in profilers:
        prepend.append("job.profilers.add('%s')" % profiler.name)
        append.append("job.profilers.delete('%s')" % profiler.name)
    return prepend, append


def _sanity_check_generate_control(is_server, client_control_file):
    """
    Sanity check some of the parameters to generate_control().

    This exists as its own function so that site_control_file may call it as
    well from its own generate_control().

    @raises ValidationError if any of the parameters do not make sense.
    """
    if is_server and client_control_file:
        raise model_logic.ValidationError(
                {'tests' : 'You cannot run server tests at the same time '
                 'as directly supplying a client-side control file.'})


def generate_control(tests, is_server=False, profilers=(),
                     client_control_file='', profile_only=None,
                     test_source_build=None):
    """
    Generate a control file for a sequence of tests.

    @param tests A sequence of test control files to run.
    @param is_server bool, Is this a server control file rather than a client?
    @param profilers A list of profiler objects to enable during the tests.
    @param client_control_file Contents of a client control file to run as the
            last test after everything in tests.  Requires is_server=False.
    @param profile_only bool, should this control file run all tests in
            profile_only mode by default
    @param test_source_build: Build to be used to retrieve test code. Default
                              to None.

    @returns The control file text as a string.
    """
    _sanity_check_generate_control(is_server=is_server,
                                   client_control_file=client_control_file)
    control_file_text = EMPTY_TEMPLATE
    prepend, append = _get_profiler_commands(profilers, is_server, profile_only)
    control_file_text += _get_tests_stanza(tests, is_server, prepend, append,
                                           client_control_file,
                                           test_source_build)
    return control_file_text


def _get_test_control_files_by_build(tests, build, ignore_invalid_tests=False):
    """Get the test control files that are available for the specified build.

    @param tests A sequence of test objects to run.
    @param build: unique name by which to refer to the image.
    @param ignore_invalid_tests: flag on if unparsable tests are ignored.

    @return: A sorted list of all tests that are in the build specified.
    """
    raw_control_files = []
    # shortcut to avoid staging the image.
    if not tests:
        return raw_control_files

    cfile_getter = _initialize_control_file_getter(build)
    if SuiteBase.ENABLE_CONTROLS_IN_BATCH:
        control_file_info_list = cfile_getter.get_suite_info()

    for test in tests:
        # Read and parse the control file
        if SuiteBase.ENABLE_CONTROLS_IN_BATCH:
            control_file = control_file_info_list[test.path]
        else:
            control_file = cfile_getter.get_control_file_contents(
                    test.path)
        raw_control_files.append(control_file)
    return raw_control_files


def _initialize_control_file_getter(build):
    """Get the remote control file getter.

    @param build: unique name by which to refer to a remote build image.

    @return: A control file getter object.
    """
    # Stage the test artifacts.
    try:
        ds = dev_server.ImageServer.resolve(build)
        ds_name = ds.hostname
        build = ds.translate(build)
    except dev_server.DevServerException as e:
        raise ValueError('Could not resolve build %s: %s' %
                         (build, e))

    try:
        ds.stage_artifacts(image=build, artifacts=['test_suites'])
    except dev_server.DevServerException as e:
        raise error.StageControlFileFailure(
                'Failed to stage %s on %s: %s' % (build, ds_name, e))

    # Collect the control files specified in this build
    return control_file_getter.DevServerGetter.create(build, ds)
