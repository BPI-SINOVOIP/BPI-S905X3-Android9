#!/usr/bin/python
# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import unittest

import common

from autotest_lib.server import utils
from autotest_lib.server.hosts.cros_label import BoardLabel
from autotest_lib.server.hosts.cros_label import ModelLabel
from autotest_lib.server.hosts.cros_label import SparseCoverageLabel

# pylint: disable=missing-docstring

NON_UNI_LSB_RELEASE_OUTPUT = """
CHROMEOS_RELEASE_APPID={63A9F698-C1CA-4A75-95E7-6B90181B3718}
CHROMEOS_BOARD_APPID={63A9F698-C1CA-4A75-95E7-6B90181B3718}
CHROMEOS_CANARY_APPID={90F229CE-83E2-4FAF-8479-E368A34938B1}
DEVICETYPE=CHROMEBOOK
CHROMEOS_ARC_VERSION=4234098
CHROMEOS_ARC_ANDROID_SDK_VERSION=25
GOOGLE_RELEASE=9798.0.2017_08_02_1022
CHROMEOS_DEVSERVER=http://shapiroc3.bld.corp.google.com:8080
CHROMEOS_RELEASE_BOARD=pyro
CHROMEOS_RELEASE_BUILD_NUMBER=9798
CHROMEOS_RELEASE_BRANCH_NUMBER=0
CHROMEOS_RELEASE_CHROME_MILESTONE=62
CHROMEOS_RELEASE_PATCH_NUMBER=2017_08_02_1022
CHROMEOS_RELEASE_TRACK=testimage-channel
CHROMEOS_RELEASE_DESCRIPTION=9798.0.2017_08_02_1022 (Test Build)
CHROMEOS_RELEASE_BUILD_TYPE=Test Build
CHROMEOS_RELEASE_NAME=Chromium OS
CHROMEOS_RELEASE_VERSION=9798.0.2017_08_02_1022
CHROMEOS_AUSERVER=http://someserver.bld.corp.google.com:8080/update
"""

UNI_LSB_RELEASE_OUTPUT = """
CHROMEOS_RELEASE_APPID={5A3AB642-2A67-470A-8F37-37E737A53CFC}
CHROMEOS_BOARD_APPID={5A3AB642-2A67-470A-8F37-37E737A53CFC}
CHROMEOS_CANARY_APPID={90F229CE-83E2-4FAF-8479-E368A34938B1}
DEVICETYPE=CHROMEBOOK
CHROMEOS_ARC_VERSION=4340813
CHROMEOS_ARC_ANDROID_SDK_VERSION=25
GOOGLE_RELEASE=9953.0.2017_09_18_1334
CHROMEOS_DEVSERVER=http://server.bld.corp.google.com:8080
CHROMEOS_RELEASE_BOARD=coral
CHROMEOS_RELEASE_BUILD_NUMBER=9953
CHROMEOS_RELEASE_BRANCH_NUMBER=0
CHROMEOS_RELEASE_CHROME_MILESTONE=63
CHROMEOS_RELEASE_PATCH_NUMBER=2017_09_18_1334
CHROMEOS_RELEASE_TRACK=testimage-channel
CHROMEOS_RELEASE_DESCRIPTION=9953.0.2017_09_18_1334 (Test Build)
CHROMEOS_RELEASE_BUILD_TYPE=Test Build
CHROMEOS_RELEASE_NAME=Chromium OS
CHROMEOS_RELEASE_UNIBUILD=1
CHROMEOS_RELEASE_VERSION=9953.0.2017_09_18_1334
CHROMEOS_AUSERVER=http://server.bld.corp.google.com:8080/update
CHROMEOS_RELEASE_MODELS=coral astronaut blue bruce lava nasher
"""


class MockCmd(object):
    """Simple mock command with base command and results"""

    def __init__(self, cmd, exit_status, stdout):
        self.cmd = cmd
        self.stdout = stdout
        self.exit_status = exit_status


class MockAFEHost(utils.EmptyAFEHost):

    def __init__(self, labels=[], attributes={}):
        self.labels = labels
        self.attributes = attributes


class MockHost(object):
    """Simple host for running mock'd host commands"""

    def __init__(self, labels, *args):
        self._afe_host = MockAFEHost(labels)
        self.mock_cmds = {c.cmd: c for c in args}

    def run(self, command, **kwargs):
        """Finds the matching result by command value"""
        return self.mock_cmds[command]


class ModelLabelTests(unittest.TestCase):
    """Unit tests for ModelLabel"""

    def test_cros_config_succeeds(self):
        cat_lsb_release_output = """
CHROMEOS_RELEASE_BOARD=pyro
CHROMEOS_RELEASE_UNIBUILD=1
"""
        host = MockHost([],
                        MockCmd('cros_config / test-label', 0, 'coral\n'),
                        MockCmd('cat /etc/lsb-release', 0,
                                cat_lsb_release_output))
        self.assertEqual(ModelLabel().generate_labels(host), ['coral'])

    def test_cros_config_fails_mosys_succeeds(self):
        cat_lsb_release_output = """
CHROMEOS_RELEASE_BOARD=pyro
CHROMEOS_RELEASE_UNIBUILD=1
"""
        host = MockHost([],
                        MockCmd('cros_config / test-label', 1, ''),
                        MockCmd('mosys platform model', 0, 'coral\n'),
                        MockCmd('cat /etc/lsb-release', 0,
                                cat_lsb_release_output))
        self.assertEqual(ModelLabel().generate_labels(host), ['coral'])

    def test_cros_config_fails_mosys_fails(self):
        cat_lsb_release_output = """
CHROMEOS_RELEASE_BOARD=pyro
CHROMEOS_RELEASE_UNIBUILD=1
"""
        host = MockHost([],
                        MockCmd('cros_config / test-label', 1, ''),
                        MockCmd('mosys platform model', 1, ''),
                        MockCmd('cat /etc/lsb-release', 0,
                                cat_lsb_release_output))
        self.assertEqual(ModelLabel().generate_labels(host), ['pyro'])

    def test_cros_config_only_used_for_unibuilds(self):
        cat_lsb_release_output = """
CHROMEOS_RELEASE_BOARD=pyro
"""
        host = MockHost([],
                        MockCmd('cat /etc/lsb-release', 0,
                                cat_lsb_release_output))
        self.assertEqual(ModelLabel().generate_labels(host), ['pyro'])

    def test_existing_label(self):
        host = MockHost(['model:existing'])
        self.assertEqual(ModelLabel().generate_labels(host), ['existing'])


class BoardLabelTests(unittest.TestCase):
    """Unit tests for BoardLabel"""

    def test_new_label(self):
        cat_cmd = 'cat /etc/lsb-release'
        host = MockHost([], MockCmd(cat_cmd, 0, NON_UNI_LSB_RELEASE_OUTPUT))
        self.assertEqual(BoardLabel().generate_labels(host), ['pyro'])

    def test_existing_label(self):
        host = MockHost(['board:existing'])
        self.assertEqual(BoardLabel().generate_labels(host), ['existing'])


SPARSE_COVERAGE_TEMPLATE = """
CHROMEOS_RELEASE_APPID={{01906EA2-3EB2-41F1-8F62-F0B7120EFD2E}}
CHROMEOS_BOARD_APPID={{01906EA2-3EB2-41F1-8F62-F0B7120EFD2E}}
CHROMEOS_CANARY_APPID={{90F229CE-83E2-4FAF-8479-E368A34938B1}}
DEVICETYPE=CHROMEBOOK
CHROMEOS_ARC_VERSION=4473730
CHROMEOS_ARC_ANDROID_SDK_VERSION=25
GOOGLE_RELEASE={build}.{branch}.{patch}
CHROMEOS_DEVSERVER=
CHROMEOS_RELEASE_BUILDER_PATH=eve-{builder}/R64-{build}.{branch}.{patch}
CHROMEOS_RELEASE_BUILD_NUMBER={build}
CHROMEOS_RELEASE_BRANCH_NUMBER={branch}
CHROMEOS_RELEASE_CHROME_MILESTONE=64
CHROMEOS_RELEASE_PATCH_NUMBER={patch}
CHROMEOS_RELEASE_TRACK=testimage-channel
CHROMEOS_RELEASE_DESCRIPTION={build}.{branch}.{patch} (Official Build) dev-channel eve test
CHROMEOS_RELEASE_BUILD_TYPE=Official Build
CHROMEOS_RELEASE_NAME=Chrome OS
CHROMEOS_RELEASE_BOARD=eve
CHROMEOS_RELEASE_VERSION={build}.{branch}.{patch}
CHROMEOS_AUSERVER=https://tools.google.com/service/update2
"""


def _cat_output(builder, build, branch, patch):
    return SPARSE_COVERAGE_TEMPLATE.format(
        builder=builder, build=build, branch=branch, patch=patch)


class SparseCoverageLabelTests(unittest.TestCase):
    """Unit tests for SparseCoverageLabel"""

    _mock_data = [
        # Master canary build - sparse.
        (('release', '60000', '0', '0'), {2, 3, 5}),
        (('release', '60001', '0', '0'), {}),
        (('release', '60002', '0', '0'), {2}),
        (('release', '60003', '0', '0'), {3}),
        (('release', '60004', '0', '0'), {2}),
        (('release', '60005', '0', '0'), {5}),
        (('release', '60006', '0', '0'), {2, 3}),
        # Branch canary build - not sparse.
        (('release', '60000', '1', '0'), {2, 3, 5}),
        (('release', '60001', '1', '0'), {2, 3, 5}),
        (('release', '60002', '1', '0'), {2, 3, 5}),
        (('release', '60003', '1', '0'), {2, 3, 5}),
        (('release', '60004', '1', '0'), {2, 3, 5}),
        (('release', '60005', '1', '0'), {2, 3, 5}),
        (('release', '60006', '1', '0'), {2, 3, 5}),
        # A CQ/PFQ like build - not sparse.
        (('release', '60000', '0', '0-rc4'), {2, 3, 5}),
        (('release', '60001', '0', '0-rc4'), {2, 3, 5}),
        (('release', '60002', '0', '0-rc4'), {2, 3, 5}),
        (('release', '60003', '0', '0-rc4'), {2, 3, 5}),
        (('release', '60004', '0', '0-rc4'), {2, 3, 5}),
        (('release', '60005', '0', '0-rc4'), {2, 3, 5}),
        (('release', '60006', '0', '0-rc4'), {2, 3, 5}),
        # Not a release build - not sparse.
        (('chrome-pfq', '60000', '0', '0'), {2, 3, 5}),
        (('chrome-pfq', '60001', '0', '0'), {2, 3, 5}),
        (('chrome-pfq', '60002', '0', '0'), {2, 3, 5}),
        (('chrome-pfq', '60003', '0', '0'), {2, 3, 5}),
        (('chrome-pfq', '60004', '0', '0'), {2, 3, 5}),
        (('chrome-pfq', '60005', '0', '0'), {2, 3, 5}),
        (('chrome-pfq', '60006', '0', '0'), {2, 3, 5}),
    ]

    def test_coverage_label(self):
        cat_cmd = 'cat /etc/lsb-release'
        for release, short_labels in self._mock_data:
            host = MockHost([], MockCmd(cat_cmd, 0, _cat_output(*release)))
            expected_labels = set(
                'sparse_coverage_%d' % l for l in short_labels)
            generated_labels = set(SparseCoverageLabel().generate_labels(host))
            self.assertEqual(expected_labels, generated_labels)


if __name__ == '__main__':
    unittest.main()
