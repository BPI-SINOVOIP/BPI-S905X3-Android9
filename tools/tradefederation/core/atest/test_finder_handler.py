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
Test Finder Handler module.
"""

import logging

import atest_enum
from test_finders import test_finder_base
from test_finders import tf_integration_finder
from test_finders import module_finder

# List of default test finder classes.
_TEST_FINDERS = {
    tf_integration_finder.TFIntegrationFinder,
    module_finder.ModuleFinder,
}

# Explanation of REFERENCE_TYPEs:
# ----------------------------------
# 0. MODULE: LOCAL_MODULE or LOCAL_PACKAGE_NAME value in Android.mk/Android.bp.
# 1. MODULE_CLASS: Combo of MODULE and CLASS as "module:class".
# 2. PACKAGE: package in java file. Same as file path to java file.
# 3. MODULE_PACKAGE: Combo of MODULE and PACKAGE as "module:package".
# 4. MODULE_FILE_PATH: File path to dir of tests or test itself.
# 5. INTEGRATION_FILE_PATH: File path to config xml in one of the 4 integration
#                           config directories.
# 6. INTEGRATION: xml file name in one of the 4 integration config directories.
# 7. SUITE: Value of the "run-suite-tag" in xml config file in 4 config dirs.
#           Same as value of "test-suite-tag" in AndroidTest.xml files.
_REFERENCE_TYPE = atest_enum.AtestEnum(['MODULE', 'CLASS', 'QUALIFIED_CLASS',
                                        'MODULE_CLASS', 'PACKAGE',
                                        'MODULE_PACKAGE', 'MODULE_FILE_PATH',
                                        'INTEGRATION_FILE_PATH', 'INTEGRATION',
                                        'SUITE'])

_REF_TYPE_TO_FUNC_MAP = {
    _REFERENCE_TYPE.MODULE: module_finder.ModuleFinder.find_test_by_module_name,
    _REFERENCE_TYPE.CLASS: module_finder.ModuleFinder.find_test_by_class_name,
    _REFERENCE_TYPE.MODULE_CLASS: module_finder.ModuleFinder.find_test_by_module_and_class,
    _REFERENCE_TYPE.QUALIFIED_CLASS: module_finder.ModuleFinder.find_test_by_class_name,
    _REFERENCE_TYPE.PACKAGE: module_finder.ModuleFinder.find_test_by_package_name,
    _REFERENCE_TYPE.MODULE_PACKAGE: module_finder.ModuleFinder.find_test_by_module_and_package,
    _REFERENCE_TYPE.MODULE_FILE_PATH: module_finder.ModuleFinder.find_test_by_path,
    _REFERENCE_TYPE.INTEGRATION_FILE_PATH:
        tf_integration_finder.TFIntegrationFinder.find_int_test_by_path,
    _REFERENCE_TYPE.INTEGRATION:
        tf_integration_finder.TFIntegrationFinder.find_test_by_integration_name,
}


def _get_finder_instance_dict(module_info):
    """Return dict of finder instances.

    Args:
        module_info: ModuleInfo for finder classes to use.

    Returns:
        Dict of finder instances keyed by their name.
    """
    instance_dict = {}
    for finder in _get_test_finders():
        instance_dict[finder.NAME] = finder(module_info=module_info)
    return instance_dict


def _get_test_finders():
    """Returns the test finders.

    If external test types are defined outside atest, they can be try-except
    imported into here.

    Returns:
        Set of test finder classes.
    """
    test_finders_list = _TEST_FINDERS
    # Example import of external test finder:
    try:
        from test_finders import example_finder
        test_finders_list.add(example_finder.ExampleFinder)
    except ImportError:
        pass
    return test_finders_list

# pylint: disable=too-many-return-statements
def _get_test_reference_types(ref):
    """Determine type of test reference based on the content of string.

    Examples:
        The string 'SequentialRWTest' could be a reference to
        a Module or a Class name.

        The string 'cts/tests/filesystem' could be a Path, Integration
        or Suite reference.

    Args:
        ref: A string referencing a test.

    Returns:
        A list of possible REFERENCE_TYPEs (ints) for reference string.
    """
    if ref.startswith('.') or '..' in ref:
        return [_REFERENCE_TYPE.INTEGRATION_FILE_PATH,
                _REFERENCE_TYPE.MODULE_FILE_PATH]
    if '/' in ref:
        if ref.startswith('/'):
            return [_REFERENCE_TYPE.INTEGRATION_FILE_PATH,
                    _REFERENCE_TYPE.MODULE_FILE_PATH]
        return [_REFERENCE_TYPE.INTEGRATION_FILE_PATH,
                _REFERENCE_TYPE.MODULE_FILE_PATH,
                _REFERENCE_TYPE.INTEGRATION,
                # TODO: Comment in SUITE when it's supported
                # _REFERENCE_TYPE.SUITE
               ]
    if '.' in ref:
        ref_end = ref.rsplit('.', 1)[-1]
        ref_end_is_upper = ref_end[0].isupper()
    if ':' in ref:
        if '.' in ref:
            if ref_end_is_upper:
                # Module:fully.qualified.Class or Integration:fully.q.Class
                return [_REFERENCE_TYPE.INTEGRATION,
                        _REFERENCE_TYPE.MODULE_CLASS]
            # Module:some.package
            return [_REFERENCE_TYPE.MODULE_PACKAGE]
        # Module:Class or IntegrationName:Class
        return [_REFERENCE_TYPE.INTEGRATION,
                _REFERENCE_TYPE.MODULE_CLASS]
    if '.' in ref:
        if ref_end in ('java', 'bp', 'mk'):
            return [_REFERENCE_TYPE.MODULE_FILE_PATH]
        if ref_end == 'xml':
            return [_REFERENCE_TYPE.INTEGRATION_FILE_PATH]
        if ref_end_is_upper:
            return [_REFERENCE_TYPE.QUALIFIED_CLASS]
        return [_REFERENCE_TYPE.PACKAGE]
    # Note: We assume that if you're referencing a file in your cwd,
    # that file must have a '.' in its name, i.e. foo.java, foo.xml.
    # If this ever becomes not the case, then we need to include path below.
    return [_REFERENCE_TYPE.INTEGRATION,
            # TODO: Comment in SUITE when it's supported
            # REFERENCE_TYPE.SUITE,
            _REFERENCE_TYPE.MODULE,
            _REFERENCE_TYPE.CLASS]


def _get_registered_find_methods(module_info):
    """Return list of registered find methods.

    This is used to return find methods that were not listed in the
    default find methods but just registered in the finder classes. These
    find methods will run before the default find methods.

    Args:
        module_info: ModuleInfo for finder classes to instantiate with.

    Returns:
        List of registered find methods.
    """
    find_methods = []
    finder_instance_dict = _get_finder_instance_dict(module_info)
    for finder in _get_test_finders():
        finder_instance = finder_instance_dict[finder.NAME]
        for find_method_info in finder_instance.get_all_find_methods():
            find_methods.append(test_finder_base.Finder(
                finder_instance, find_method_info.find_method))
    return find_methods


def _get_default_find_methods(module_info, test):
    """Default find methods to be used based on the given test name.

    Args:
        module_info: ModuleInfo for finder instances to use.
        test: String of test name to help determine which find methods
              to utilize.

    Returns:
        List of find methods to use.
    """
    find_methods = []
    finder_instance_dict = _get_finder_instance_dict(module_info)
    test_ref_types = _get_test_reference_types(test)
    logging.debug('Resolved input to possible references: %s', [
        _REFERENCE_TYPE[t] for t in test_ref_types])
    for test_ref_type in test_ref_types:
        find_method = _REF_TYPE_TO_FUNC_MAP[test_ref_type]
        finder_instance = finder_instance_dict[find_method.im_class.NAME]
        find_methods.append(test_finder_base.Finder(finder_instance,
                                                    find_method))
    return find_methods


def get_find_methods_for_test(module_info, test):
    """Return a list of ordered find methods.

    Args:
      test: String of test name to get find methods for.

    Returns:
        List of ordered find methods.
    """
    registered_find_methods = _get_registered_find_methods(module_info)
    default_find_methods = _get_default_find_methods(module_info, test)
    return registered_find_methods + default_find_methods
