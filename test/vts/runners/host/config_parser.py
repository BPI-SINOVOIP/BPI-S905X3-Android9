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

from builtins import str

import copy
import signal
import sys
import traceback

from vts.runners.host import keys
from vts.runners.host import errors
from vts.runners.host import signals
from vts.runners.host import utils

_DEFAULT_CONFIG_TEMPLATE = {
    "test_bed": {
        "AndroidDevice": "*",
    },
    "log_path": "/tmp/logs",
    "test_paths": ["./"],
    "enable_web": False,
}


def GetDefaultConfig(test_name):
    """Returns a default config data structure (when no config file is given)."""
    result = copy.deepcopy(_DEFAULT_CONFIG_TEMPLATE)
    result[keys.ConfigKeys.KEY_TESTBED][
        keys.ConfigKeys.KEY_TESTBED_NAME] = test_name
    return result


def load_test_config_file(test_config_path,
                          tb_filters=None,
                          baseline_config=None):
    """Processes the test configuration file provided by user.

    Loads the configuration file into a json object, unpacks each testbed
    config into its own json object, and validate the configuration in the
    process.

    Args:
        test_config_path: Path to the test configuration file.
        tb_filters: A list of strings, each is a test bed name. If None, all
                    test beds are picked up. Otherwise only test bed names
                    specified will be picked up.
        baseline_config: dict, the baseline config to use (used iff
                         test_config_path does not have device info).

    Returns:
        A list of test configuration json objects to be passed to TestRunner.
    """
    try:
        configs = utils.load_config(test_config_path)
        if keys.ConfigKeys.KEY_TESTBED not in configs and baseline_config:
            configs.update(baseline_config)

        if tb_filters:
            tbs = []
            for tb in configs[keys.ConfigKeys.KEY_TESTBED]:
                if tb[keys.ConfigKeys.KEY_TESTBED_NAME] in tb_filters:
                    tbs.append(tb)
            if len(tbs) != len(tb_filters):
                print("Expect to find %d test bed configs, found %d." %
                      (len(tb_filters), len(tbs)))
                print("Check if you have the correct test bed names.")
                return None
            configs[keys.ConfigKeys.KEY_TESTBED] = tbs
        _validate_test_config(configs)
        _validate_testbed_configs(configs[keys.ConfigKeys.KEY_TESTBED])
        k_log_path = keys.ConfigKeys.KEY_LOG_PATH
        configs[k_log_path] = utils.abs_path(configs[k_log_path])
        tps = configs[keys.ConfigKeys.KEY_TEST_PATHS]
    except errors.USERError as e:
        print("Something is wrong in the test configurations.")
        print(str(e))
        return None
    except Exception as e:
        print("Error loading test config {}".format(test_config_path))
        print(traceback.format_exc())
        return None
    # Unpack testbeds into separate json objects.
    beds = configs.pop(keys.ConfigKeys.KEY_TESTBED)
    config_jsons = []
    for original_bed_config in beds:
        new_test_config = dict(configs)
        new_test_config[keys.ConfigKeys.KEY_TESTBED] = original_bed_config
        # Keys in each test bed config will be copied to a level up to be
        # picked up for user_params. If the key already exists in the upper
        # level, the local one defined in test bed config overwrites the
        # general one.
        new_test_config.update(original_bed_config)
        config_jsons.append(new_test_config)
    return config_jsons


def parse_test_list(test_list):
    """Parse user provided test list into internal format for test_runner.

    Args:
        test_list: A list of test classes/cases.

    Returns:
        A list of tuples, each has a test class name and a list of test case
        names.
    """
    result = []
    for elem in test_list:
        result.append(_parse_one_test_specifier(elem))
    return result


def _validate_test_config(test_config):
    """Validates the raw configuration loaded from the config file.

    Making sure all the required keys exist.

    Args:
        test_config: A dict that is the config to validate.

    Raises:
        errors.USERError is raised if any required key is missing from the
        config.
    """
    for k in keys.ConfigKeys.RESERVED_KEYS:
        if k not in test_config:
            raise errors.USERError(("Required key {} missing in test "
                                    "config.").format(k))


def _parse_one_test_specifier(item):
    """Parse one test specifier from command line input.

    This also verifies that the test class name and test case names follow
    ACTS's naming conventions. A test class name has to end with "Test"; a test
    case name has to start with "test".

    Args:
        item: A string that specifies a test class or test cases in one test
            class to run.

    Returns:
        A tuple of a string and a list of strings. The string is the test class
        name, the list of strings is a list of test case names. The list can be
        None.
    """
    tokens = item.split(':')
    if len(tokens) > 2:
        raise errors.USERError("Syntax error in test specifier %s" % item)
    if len(tokens) == 1:
        # This should be considered a test class name
        test_cls_name = tokens[0]
        _validate_test_class_name(test_cls_name)
        return (test_cls_name, None)
    elif len(tokens) == 2:
        # This should be considered a test class name followed by
        # a list of test case names.
        test_cls_name, test_case_names = tokens
        clean_names = []
        _validate_test_class_name(test_cls_name)
        for elem in test_case_names.split(','):
            test_case_name = elem.strip()
            if not test_case_name.startswith("test_"):
                raise errors.USERError(
                    ("Requested test case '%s' in test class "
                     "'%s' does not follow the test case "
                     "naming convention test_*.") % (test_case_name,
                                                     test_cls_name))
            clean_names.append(test_case_name)
        return (test_cls_name, clean_names)


def _parse_test_file(fpath):
    """Parses a test file that contains test specifiers.

    Args:
        fpath: A string that is the path to the test file to parse.

    Returns:
        A list of strings, each is a test specifier.
    """
    try:
        with open(fpath, 'r') as f:
            tf = []
            for line in f:
                line = line.strip()
                if not line:
                    continue
                if len(tf) and (tf[-1].endswith(':') or tf[-1].endswith(',')):
                    tf[-1] += line
                else:
                    tf.append(line)
            return tf
    except:
        print("Error loading test file.")
        raise


def _validate_test_class_name(test_cls_name):
    """Checks if a string follows the test class name convention.

    Args:
        test_cls_name: A string that should be a test class name.

    Raises:
        errors.USERError is raised if the input does not follow test class
        naming convention.
    """
    if not test_cls_name.endswith("Test"):
        raise errors.USERError(
            ("Requested test class '%s' does not follow the test "
             "class naming convention *Test.") % test_cls_name)


def _validate_testbed_configs(testbed_configs):
    """Validates the testbed configurations.

    Args:
        testbed_configs: A list of testbed configuration json objects.

    Raises:
        If any part of the configuration is invalid, errors.USERError is raised.
    """
    seen_names = set()
    # Cross checks testbed configs for resource conflicts.
    for config in testbed_configs:
        # Check for conflicts between multiple concurrent testbed configs.
        # No need to call it if there's only one testbed config.
        name = config[keys.ConfigKeys.KEY_TESTBED_NAME]
        _validate_testbed_name(name)
        # Test bed names should be unique.
        if name in seen_names:
            raise errors.USERError("Duplicate testbed name {} found.".format(
                name))
        seen_names.add(name)


def _validate_testbed_name(name):
    """Validates the name of a test bed.

    Since test bed names are used as part of the test run id, it needs to meet
    certain requirements.

    Args:
        name: The test bed's name specified in config file.

    Raises:
        If the name does not meet any criteria, errors.USERError is raised.
    """
    if not name:
        raise errors.USERError("Test bed names can't be empty.")
    if not isinstance(name, str) and not isinstance(name, basestring):
        raise errors.USERError("Test bed names have to be string. Found: %s" %
                               type(name))
    for l in name:
        if l not in utils.valid_filename_chars:
            raise errors.USERError(
                "Char '%s' is not allowed in test bed names." % l)
