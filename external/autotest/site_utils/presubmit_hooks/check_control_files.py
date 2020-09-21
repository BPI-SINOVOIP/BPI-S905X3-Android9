#!/usr/bin/python -u
# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""
Check an autotest control file for required variables.

This wrapper is invoked through autotest's PRESUBMIT.cfg for every commit
that edits a control file.
"""


import argparse
import glob
import os
import re
import subprocess

import common
from autotest_lib.client.common_lib import control_data
from autotest_lib.server.cros.dynamic_suite import reporting_utils


DEPENDENCY_ARC = 'arc'
SUITES_NEED_RETRY = set(['bvt-arc', 'bvt-cq', 'bvt-inline'])
TESTS_NEED_ARC = 'cheets_'


class ControlFileCheckerError(Exception):
    """Raised when a necessary condition of this checker isn't satisfied."""


def IsInChroot():
    """Return boolean indicating if we are running in the chroot."""
    return os.path.exists("/etc/debian_chroot")


def CommandPrefix():
    """Return an argv list which must appear at the start of shell commands."""
    if IsInChroot():
        return []
    else:
        return ['cros_sdk', '--']


def GetOverlayPath(overlay=None):
    """
    Return the path to the overlay directory.

    If the overlay path is not given, the default chromiumos-overlay path
    will be returned instead.

    @param overlay: The overlay repository path for autotest ebuilds.

    @return normalized absolutized path of the overlay repository.
    """
    if not overlay:
        ourpath = os.path.abspath(__file__)
        overlay = os.path.join(os.path.dirname(ourpath),
                               "../../../../chromiumos-overlay/")
    return os.path.normpath(overlay)


def GetAutotestTestPackages(overlay=None):
    """
    Return a list of ebuilds which should be checked for test existance.

    @param overlay: The overlay repository path for autotest ebuilds.

    @return autotest packages in overlay repository.
    """
    overlay = GetOverlayPath(overlay)
    packages = glob.glob(os.path.join(overlay, "chromeos-base/autotest-*"))
    # Return the packages list with the leading overlay path removed.
    return [x[(len(overlay) + 1):] for x in packages]


def GetEqueryWrappers():
    """Return a list of all the equery variants that should be consulted."""
    # Note that we can't just glob.glob('/usr/local/bin/equery-*'), because
    # we might be running outside the chroot.
    pattern = '/usr/local/bin/equery-*'
    cmd = CommandPrefix() + ['sh', '-c', 'echo %s' % pattern]
    wrappers = subprocess.check_output(cmd).split()
    # If there was no match, we get the literal pattern string echoed back.
    if wrappers and wrappers[0] == pattern:
        wrappers = []
    return ['equery'] + wrappers


def GetUseFlags(overlay=None):
    """Get the set of all use flags from autotest packages.

    @param overlay: The overlay repository path for autotest ebuilds.

    @returns: useflags
    """
    useflags = set()
    for equery in GetEqueryWrappers():
        cmd_args = (CommandPrefix() + [equery, '-qC', 'uses'] +
                    GetAutotestTestPackages(overlay))
        child = subprocess.Popen(cmd_args, stdout=subprocess.PIPE,
                                 stderr=subprocess.PIPE)
        new_useflags = child.communicate()[0].splitlines()
        if child.returncode == 0:
            useflags = useflags.union(new_useflags)
    return useflags


def CheckSuites(ctrl_data, test_name, useflags):
    """
    Check that any test in a SUITE is also in an ebuild.

    Throws a ControlFileCheckerError if a test within a SUITE
    does not appear in an ebuild. For purposes of this check,
    the psuedo-suite "manual" does not require a test to be
    in an ebuild.

    @param ctrl_data: The control_data object for a test.
    @param test_name: A string with the name of the test.
    @param useflags: Set of all use flags from autotest packages.

    @returns: None
    """
    if (hasattr(ctrl_data, 'suite') and ctrl_data.suite and
        ctrl_data.suite != 'manual'):
        # To handle the case where a developer has cros_workon'd
        # e.g. autotest-tests on one particular board, and has the
        # test listed only in the -9999 ebuild, we have to query all
        # the equery-* board-wrappers until we find one. We ALSO have
        # to check plain 'equery', to handle the case where e.g. a
        # developer who has never run setup_board, and has no
        # wrappers, is making a quick edit to some existing control
        # file already enabled in the stable ebuild.
        for flag in useflags:
            if flag.startswith('-') or flag.startswith('+'):
                flag = flag[1:]
            if flag == 'tests_%s' % test_name:
                return
        raise ControlFileCheckerError(
                'No ebuild entry for %s. To fix, please do the following: 1. '
                'Add your new test to one of the ebuilds referenced by '
                'autotest-all. 2. cros_workon --board=<board> start '
                '<your_ebuild>. 3. emerge-<board> <your_ebuild>' % test_name)


def CheckValidAttr(ctrl_data, whitelist, test_name):
    """
    Check whether ATTRIBUTES are in the whitelist.

    Throw a ControlFileCheckerError if tags in ATTRIBUTES don't exist in the
    whitelist.

    @param ctrl_data: The control_data object for a test.
    @param whitelist: whitelist set parsed from the attribute_whitelist file.
    @param test_name: A string with the name of the test.

    @returns: None
    """
    if not (whitelist >= ctrl_data.attributes):
        attribute_diff = ctrl_data.attributes - whitelist
        raise ControlFileCheckerError(
            'Attribute(s): %s not in the whitelist in control file for test '
            'named %s. If this is a new attribute, please add it into '
            'AUTOTEST_DIR/site_utils/attribute_whitelist.txt file'
            % (attribute_diff, test_name))


def CheckSuiteLineRemoved(ctrl_file_path):
    """
    Check whether the SUITE line has been removed since it is obsolete.

    @param ctrl_file_path: The path to the control file.

    @raises: ControlFileCheckerError if check fails.
    """
    with open(ctrl_file_path, 'r') as f:
        for line in f.readlines():
            if line.startswith('SUITE'):
                raise ControlFileCheckerError(
                    'SUITE is an obsolete variable, please remove it from %s. '
                    'Instead, add suite:<your_suite> to the ATTRIBUTES field.'
                    % ctrl_file_path)


def CheckRetry(ctrl_data, test_name):
    """
    Check that any test in SUITES_NEED_RETRY has turned on retry.

    @param ctrl_data: The control_data object for a test.
    @param test_name: A string with the name of the test.

    @raises: ControlFileCheckerError if check fails.
    """
    if hasattr(ctrl_data, 'suite') and ctrl_data.suite:
        suites = set(x.strip() for x in ctrl_data.suite.split(',') if x.strip())
        if ctrl_data.job_retries < 2 and SUITES_NEED_RETRY.intersection(suites):
            raise ControlFileCheckerError(
                'Setting JOB_RETRIES to 2 or greater for test in '
                '%s is recommended. Please set it in the control '
                'file for %s.' % (' or '.join(SUITES_NEED_RETRY), test_name))


def CheckDependencies(ctrl_data, test_name):
    """
    Check if any dependencies of a test is required

    @param ctrl_data: The control_data object for a test.
    @param test_name: A string with the name of the test.

    @raises: ControlFileCheckerError if check fails.
    """
    if test_name.startswith(TESTS_NEED_ARC):
        if not DEPENDENCY_ARC in ctrl_data.dependencies:
            raise ControlFileCheckerError(
                    'DEPENDENCIES = \'arc\' for %s is needed' % test_name)


def main():
    """
    Checks if all control files that are a part of this commit conform to the
    ChromeOS autotest guidelines.
    """
    parser = argparse.ArgumentParser(description='Process overlay arguments.')
    parser.add_argument('--overlay', default=None, help='the overlay directory path')
    args = parser.parse_args()
    file_list = os.environ.get('PRESUBMIT_FILES')
    if file_list is None:
        raise ControlFileCheckerError('Expected a list of presubmit files in '
            'the PRESUBMIT_FILES environment variable.')

    # Parse the whitelist set from file, hardcode the filepath to the whitelist.
    path_whitelist = os.path.join(common.autotest_dir,
                                  'site_utils/attribute_whitelist.txt')
    with open(path_whitelist, 'r') as f:
        whitelist = {line.strip() for line in f.readlines() if line.strip()}

    # Delay getting the useflags. The call takes long time, so init useflags
    # only when needed, i.e., the script needs to check any control file.
    useflags = None
    for file_path in file_list.split('\n'):
        control_file = re.search(r'.*/control(?:\.\w+)?$', file_path)
        if control_file:
            ctrl_file_path = control_file.group(0)
            CheckSuiteLineRemoved(ctrl_file_path)
            ctrl_data = control_data.parse_control(ctrl_file_path,
                                                   raise_warnings=True)
            test_name = os.path.basename(os.path.split(file_path)[0])
            try:
                reporting_utils.BugTemplate.validate_bug_template(
                        ctrl_data.bug_template)
            except AttributeError:
                # The control file may not have bug template defined.
                pass

            if not useflags:
                useflags = GetUseFlags(args.overlay)
            CheckSuites(ctrl_data, test_name, useflags)
            CheckValidAttr(ctrl_data, whitelist, test_name)
            CheckRetry(ctrl_data, test_name)
            CheckDependencies(ctrl_data, test_name)


if __name__ == '__main__':
    main()
