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
Module Finder class.
"""

import logging
import os
import re

# pylint: disable=import-error
import atest_error
import constants
import test_info
import test_finder_base
import test_finder_utils
from test_runners import atest_tf_test_runner
from test_runners import robolectric_test_runner
from test_runners import vts_tf_test_runner

_JAVA_EXT = '.java'

# Parse package name from the package declaration line of a java file.
# Group matches "foo.bar" of line "package foo.bar;"
_PACKAGE_RE = re.compile(r'\s*package\s+(?P<package>[^;]+)\s*;\s*', re.I)

_MODULES_IN = 'MODULES-IN-%s'
_ANDROID_MK = 'Android.mk'

# These are suites in LOCAL_COMPATIBILITY_SUITE that aren't really suites so
# we can ignore them.
_SUITES_TO_IGNORE = frozenset({'general-tests', 'device-tests', 'tests'})


class ModuleFinder(test_finder_base.TestFinderBase):
    """Module finder class."""
    NAME = 'MODULE'
    _TEST_RUNNER = atest_tf_test_runner.AtestTradefedTestRunner.NAME
    _ROBOLECTRIC_RUNNER = robolectric_test_runner.RobolectricTestRunner.NAME
    _VTS_TEST_RUNNER = vts_tf_test_runner.VtsTradefedTestRunner.NAME

    def __init__(self, module_info=None):
        super(ModuleFinder, self).__init__()
        self.root_dir = os.environ.get(constants.ANDROID_BUILD_TOP)
        self.module_info = module_info

    def _has_test_config(self, mod_info):
        """Validate if this module has a test config.

        A module can have a test config in the following manner:
          - The module name is not for 2nd architecture.
          - AndroidTest.xml at the module path.
          - Auto-generated config via the auto_test_config key in module-info.json.

        Args:
            mod_info: Dict of module info to check.

        Returns:
            True if this module has a test config, False otherwise.
        """
        # Check if the module is for 2nd architecture.
        if test_finder_utils.is_2nd_arch_module(mod_info):
            return False

        # Check for AndroidTest.xml at the module path.
        for path in mod_info.get(constants.MODULE_PATH, []):
            if os.path.isfile(os.path.join(self.root_dir, path,
                                           constants.MODULE_CONFIG)):
                return True

        # Check if the module has an auto-generated config.
        return self._is_auto_gen_test_config(mod_info.get(constants.MODULE_NAME))

    def _is_testable_module(self, mod_info):
        """Check if module is something we can test.

        A module is testable if:
          - it's installed.
          - it's a robolectric module (or shares path with one).

        Args:
            mod_info: Dict of module info to check.

        Returns:
            True if we can test this module, False otherwise.
        """
        if not mod_info:
            return False
        if mod_info.get(constants.MODULE_INSTALLED) and self._has_test_config(mod_info):
            return True
        if self._is_robolectric_test(mod_info.get(constants.MODULE_NAME)):
            return True
        return False

    def _get_first_testable_module(self, path):
        """Returns first testable module given module path.

        Args:
            path: String path of module to look for.

        Returns:
            String of first installed module name.
        """
        for mod in self.module_info.get_module_names(path):
            mod_info = self.module_info.get_module_info(mod)
            if self._is_testable_module(mod_info):
                return mod_info.get(constants.MODULE_NAME)
        return None

    def _is_vts_module(self, module_name):
        """Returns True if the module is a vts module, else False."""
        mod_info = self.module_info.get_module_info(module_name)
        suites = []
        if mod_info:
            suites = mod_info.get('compatibility_suites', [])
        # Pull out all *ts (cts, tvts, etc) suites.
        suites = [suite for suite in suites if suite not in _SUITES_TO_IGNORE]
        return len(suites) == 1 and 'vts' in suites

    def _update_to_vts_test_info(self, test):
        """Fill in the fields with vts specific info.

        We need to update the runner to use the vts runner and also find the
        test specific depedencies

        Args:
            test: TestInfo to update with vts specific details.

        Return:
            TestInfo that is ready for the vts test runner.
        """
        test.test_runner = self._VTS_TEST_RUNNER
        config_file = os.path.join(self.root_dir,
                                   test.data[constants.TI_REL_CONFIG])
        # Need to get out dir (special logic is to account for custom out dirs).
        # The out dir is used to construct the build targets for the test deps.
        out_dir = os.environ.get(constants.ANDROID_HOST_OUT)
        custom_out_dir = os.environ.get(constants.ANDROID_OUT_DIR)
        # If we're not an absolute custom out dir, get relative out dir path.
        if custom_out_dir is None or not os.path.isabs(custom_out_dir):
            out_dir = os.path.relpath(out_dir, self.root_dir)
        vts_out_dir = os.path.join(out_dir, 'vts', 'android-vts', 'testcases')

        # Add in vts test build targets.
        test.build_targets = test_finder_utils.get_targets_from_vts_xml(
            config_file, vts_out_dir, self.module_info)
        test.build_targets.add('vts-test-core')
        test.build_targets.add(test.test_name)
        return test

    def _get_robolectric_test_name(self, module_name):
        """Returns run robolectric module.

        There are at least 2 modules in every robolectric module path, return
        the module that we can run as a build target.

        Arg:
            module_name: String of module.

        Returns:
            String of module that is the run robolectric module, None if none
            could be found.
        """
        module_name_info = self.module_info.get_module_info(module_name)
        if not module_name_info:
            return None
        for mod in self.module_info.get_module_names(
                module_name_info.get(constants.MODULE_PATH, [])[0]):
            mod_info = self.module_info.get_module_info(mod)
            if test_finder_utils.is_robolectric_module(mod_info):
                return mod
        return None

    def _is_robolectric_test(self, module_name):
        """Check if module is a robolectric test.

        A module can be a robolectric test if the specified module has their
        class set as ROBOLECTRIC (or shares their path with a module that does).

        Args:
            module_name: String of module to check.

        Returns:
            True if the module is a robolectric module, else False.
        """
        # Check 1, module class is ROBOLECTRIC
        mod_info = self.module_info.get_module_info(module_name)
        if mod_info and test_finder_utils.is_robolectric_module(mod_info):
            return True
        # Check 2, shared modules in the path have class ROBOLECTRIC_CLASS.
        if self._get_robolectric_test_name(module_name):
            return True
        return False

    def _update_to_robolectric_test_info(self, test):
        """Update the fields for a robolectric test.

        Args:
          test: TestInfo to be updated with robolectric fields.

        Returns:
          TestInfo with robolectric fields.
        """
        test.test_runner = self._ROBOLECTRIC_RUNNER
        test.test_name = self._get_robolectric_test_name(test.test_name)
        return test

    def _process_test_info(self, test):
        """Process the test info and return some fields updated/changed.

        We need to check if the test found is a special module (like vts) and
        update the test_info fields (like test_runner) appropriately.

        Args:
            test: TestInfo that has been filled out by a find method.

        Return:
            TestInfo that has been modified as needed.
        """
        # Check if this is only a vts module.
        if self._is_vts_module(test.test_name):
            return self._update_to_vts_test_info(test)
        elif self._is_robolectric_test(test.test_name):
            return self._update_to_robolectric_test_info(test)
        module_name = test.test_name
        rel_config = test.data[constants.TI_REL_CONFIG]
        test.build_targets = self._get_build_targets(module_name, rel_config)
        return test

    def _is_auto_gen_test_config(self, module_name):
        """Check if the test config file will be generated automatically.

        Args:
            module_name: A string of the module name.

        Returns:
            True if the test config file will be generated automatically.
        """
        if self.module_info.is_module(module_name):
            mod_info = self.module_info.get_module_info(module_name)
            auto_test_config = mod_info.get('auto_test_config', [])
            return auto_test_config and auto_test_config[0]
        return False

    def _get_build_targets(self, module_name, rel_config):
        """Get the test deps.

        Args:
            module_name: name of the test.
            rel_config: XML for the given test.

        Returns:
            Set of build targets.
        """
        targets = set()
        if not self._is_auto_gen_test_config(module_name):
            config_file = os.path.join(self.root_dir, rel_config)
            targets = test_finder_utils.get_targets_from_xml(config_file,
                                                             self.module_info)
        mod_dir = os.path.dirname(rel_config).replace('/', '-')
        targets.add(_MODULES_IN % mod_dir)
        return targets

    def find_test_by_module_name(self, module_name):
        """Find test for the given module name.

        Args:
            module_name: A string of the test's module name.

        Returns:
            A populated TestInfo namedtuple if found, else None.
        """
        mod_info = self.module_info.get_module_info(module_name)
        if self._is_testable_module(mod_info):
            # path is a list with only 1 element.
            rel_config = os.path.join(mod_info['path'][0],
                                      constants.MODULE_CONFIG)
            return self._process_test_info(test_info.TestInfo(
                test_name=module_name,
                test_runner=self._TEST_RUNNER,
                build_targets=set(),
                data={constants.TI_REL_CONFIG: rel_config,
                      constants.TI_FILTER: frozenset()}))
        return None

    def find_test_by_class_name(self, class_name, module_name=None,
                                rel_config=None):
        """Find test files given a class name.

        If module_name and rel_config not given it will calculate it determine
        it by looking up the tree from the class file.

        Args:
            class_name: A string of the test's class name.
            module_name: Optional. A string of the module name to use.
            rel_config: Optional. A string of module dir relative to repo root.

        Returns:
            A populated TestInfo namedtuple if test found, else None.
        """
        class_name, methods = test_finder_utils.split_methods(class_name)
        if rel_config:
            search_dir = os.path.join(self.root_dir,
                                      os.path.dirname(rel_config))
        else:
            search_dir = self.root_dir
        test_path = test_finder_utils.find_class_file(search_dir, class_name)
        if not test_path and rel_config:
            logging.info('Did not find class (%s) under module path (%s), '
                         'researching from repo root.', class_name, rel_config)
            test_path = test_finder_utils.find_class_file(self.root_dir,
                                                          class_name)
        if not test_path:
            return None
        full_class_name = test_finder_utils.get_fully_qualified_class_name(
            test_path)
        test_filter = frozenset([test_info.TestFilter(full_class_name,
                                                      methods)])
        if not rel_config:
            test_dir = os.path.dirname(test_path)
            rel_module_dir = test_finder_utils.find_parent_module_dir(
                self.root_dir, test_dir, self.module_info)
            rel_config = os.path.join(rel_module_dir, constants.MODULE_CONFIG)
        if not module_name:
            module_name = self._get_first_testable_module(os.path.dirname(
                rel_config))
        return self._process_test_info(test_info.TestInfo(
            test_name=module_name,
            test_runner=self._TEST_RUNNER,
            build_targets=set(),
            data={constants.TI_FILTER: test_filter,
                  constants.TI_REL_CONFIG: rel_config}))

    def find_test_by_module_and_class(self, module_class):
        """Find the test info given a MODULE:CLASS string.

        Args:
            module_class: A string of form MODULE:CLASS or MODULE:CLASS#METHOD.

        Returns:
            A populated TestInfo namedtuple if found, else None.
        """
        if ':' not in module_class:
            return None
        module_name, class_name = module_class.split(':')
        module_info = self.find_test_by_module_name(module_name)
        if not module_info:
            return None
        return self.find_test_by_class_name(
            class_name, module_info.test_name,
            module_info.data.get(constants.TI_REL_CONFIG))

    def find_test_by_package_name(self, package, module_name=None,
                                  rel_config=None):
        """Find the test info given a PACKAGE string.

        Args:
            package: A string of the package name.
            module_name: Optional. A string of the module name.
            ref_config: Optional. A string of rel path of config.

        Returns:
            A populated TestInfo namedtuple if found, else None.
        """
        _, methods = test_finder_utils.split_methods(package)
        if methods:
            raise atest_error.MethodWithoutClassError('Method filtering '
                                                      'requires class')
        # Confirm that packages exists and get user input for multiples.
        if rel_config:
            search_dir = os.path.join(self.root_dir,
                                      os.path.dirname(rel_config))
        else:
            search_dir = self.root_dir
        package_path = test_finder_utils.run_find_cmd(
            test_finder_utils.FIND_REFERENCE_TYPE.PACKAGE, search_dir,
            package.replace('.', '/'))
        # package path will be the full path to the dir represented by package
        if not package_path:
            return None
        test_filter = frozenset([test_info.TestFilter(package, frozenset())])
        if not rel_config:
            rel_module_dir = test_finder_utils.find_parent_module_dir(
                self.root_dir, package_path, self.module_info)
            rel_config = os.path.join(rel_module_dir, constants.MODULE_CONFIG)
        if not module_name:
            module_name = self._get_first_testable_module(
                os.path.dirname(rel_config))
        return self._process_test_info(test_info.TestInfo(
            test_name=module_name,
            test_runner=self._TEST_RUNNER,
            build_targets=set(),
            data={constants.TI_FILTER: test_filter,
                  constants.TI_REL_CONFIG: rel_config}))

    def find_test_by_module_and_package(self, module_package):
        """Find the test info given a MODULE:PACKAGE string.

        Args:
            module_package: A string of form MODULE:PACKAGE

        Returns:
            A populated TestInfo namedtuple if found, else None.
        """
        module_name, package = module_package.split(':')
        module_info = self.find_test_by_module_name(module_name)
        if not module_info:
            return None
        return self.find_test_by_package_name(
            package, module_info.test_name,
            module_info.data.get(constants.TI_REL_CONFIG))

    def find_test_by_path(self, path):
        """Find the first test info matching the given path.

        Strategy:
            path_to_java_file --> Resolve to CLASS
            path_to_module_file -> Resolve to MODULE
            path_to_module_dir -> Resolve to MODULE
            path_to_dir_with_class_files--> Resolve to PACKAGE
            path_to_any_other_dir --> Resolve as MODULE

        Args:
            path: A string of the test's path.

        Returns:
            A populated TestInfo namedtuple if test found, else None
        """
        logging.debug('Finding test by path: %s', path)
        path, methods = test_finder_utils.split_methods(path)
        # TODO: See if this can be generalized and shared with methods above
        # create absolute path from cwd and remove symbolic links
        path = os.path.realpath(path)
        if not os.path.exists(path):
            return None
        dir_path, file_name = test_finder_utils.get_dir_path_and_filename(path)
        # Module/Class
        rel_module_dir = test_finder_utils.find_parent_module_dir(
            self.root_dir, dir_path, self.module_info)
        if not rel_module_dir:
            return None
        module_name = self._get_first_testable_module(rel_module_dir)
        rel_config = os.path.join(rel_module_dir, constants.MODULE_CONFIG)
        data = {constants.TI_REL_CONFIG: rel_config,
                constants.TI_FILTER: frozenset()}
        # Path is to java file
        if file_name and file_name.endswith(_JAVA_EXT):
            full_class_name = test_finder_utils.get_fully_qualified_class_name(
                path)
            data[constants.TI_FILTER] = frozenset(
                [test_info.TestFilter(full_class_name, methods)])
        # path to non-module dir, treat as package
        elif (not file_name and not self._is_auto_gen_test_config(module_name)
              and rel_module_dir != os.path.relpath(path, self.root_dir)):
            dir_items = [os.path.join(path, f) for f in os.listdir(path)]
            for dir_item in dir_items:
                if dir_item.endswith(_JAVA_EXT):
                    package_name = test_finder_utils.get_package_name(dir_item)
                    if package_name:
                        # methods should be empty frozenset for package.
                        if methods:
                            raise atest_error.MethodWithoutClassError()
                        data[constants.TI_FILTER] = frozenset(
                            [test_info.TestFilter(package_name, methods)])
                        break
        return self._process_test_info(test_info.TestInfo(
            test_name=module_name,
            test_runner=self._TEST_RUNNER,
            build_targets=set(),
            data=data))
