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
Integration Finder class.
"""

import copy
import logging
import os
import re
import xml.etree.ElementTree as ElementTree

# pylint: disable=import-error
import atest_error
import constants
import test_info
import test_finder_base
import test_finder_utils
from test_runners import atest_tf_test_runner

# Find integration name based on file path of integration config xml file.
# Group matches "foo/bar" given "blah/res/config/blah/res/config/foo/bar.xml
_INT_NAME_RE = re.compile(r'^.*\/res\/config\/(?P<int_name>.*).xml$')
_TF_TARGETS = frozenset(['tradefed', 'tradefed-contrib'])
_GTF_TARGETS = frozenset(['google-tradefed', 'google-tradefed-contrib'])


class TFIntegrationFinder(test_finder_base.TestFinderBase):
    """Integration Finder class."""
    NAME = 'INTEGRATION'
    _TEST_RUNNER = atest_tf_test_runner.AtestTradefedTestRunner.NAME


    def __init__(self, module_info=None):
        super(TFIntegrationFinder, self).__init__()
        self.root_dir = os.environ.get(constants.ANDROID_BUILD_TOP)
        self.module_info = module_info
        # TODO: Break this up into AOSP/google_tf integration finders.
        self.tf_dirs, self.gtf_dirs = self._get_integration_dirs()
        self.integration_dirs = self.tf_dirs + self.gtf_dirs

    def _get_mod_paths(self, module_name):
        """Return the paths of the given module name."""
        if self.module_info:
            return self.module_info.get_paths(module_name)
        return []

    def _get_integration_dirs(self):
        """Get integration dirs from MODULE_INFO based on targets.

        Returns:
            A tuple of lists of strings of integration dir rel to repo root.
        """
        tf_dirs = filter(None, [d for x in _TF_TARGETS for d in self._get_mod_paths(x)])
        gtf_dirs = filter(None, [d for x in _GTF_TARGETS for d in self._get_mod_paths(x)])
        return tf_dirs, gtf_dirs

    def _get_build_targets(self, rel_config):
        config_file = os.path.join(self.root_dir, rel_config)
        xml_root = self._load_xml_file(config_file)
        targets = test_finder_utils.get_targets_from_xml_root(xml_root,
                                                              self.module_info)
        if self.gtf_dirs:
            targets.add(constants.GTF_TARGET)
        return frozenset(targets)

    def _load_xml_file(self, path):
        """Load an xml file with option to expand <include> tags

        Args:
            path: A string of path to xml file.

        Returns:
            An xml.etree.ElementTree.Element instance of the root of the tree.
        """
        tree = ElementTree.parse(path)
        root = tree.getroot()
        self._load_include_tags(root)
        return root

    #pylint: disable=invalid-name
    def _load_include_tags(self, root):
        """Recursively expand in-place the <include> tags in a given xml tree.

        Python xml libraries don't support our type of <include> tags. Logic used
        below is modified version of the built-in ElementInclude logic found here:
        https://github.com/python/cpython/blob/2.7/Lib/xml/etree/ElementInclude.py

        Args:
            root: The root xml.etree.ElementTree.Element.

        Returns:
            An xml.etree.ElementTree.Element instance with include tags expanded
        """
        i = 0
        while i < len(root):
            elem = root[i]
            if elem.tag == 'include':
                # expand included xml file
                integration_name = elem.get('name')
                if not integration_name:
                    logging.warn('skipping <include> tag with no "name" value')
                    continue
                full_path = self._search_integration_dirs(integration_name)
                node = self._load_xml_file(full_path)
                if node is None:
                    raise atest_error.FatalIncludeError("can't load %r" %
                                                        integration_name)
                node = copy.copy(node)
                if elem.tail:
                    node.tail = (node.tail or "") + elem.tail
                root[i] = node
            i = i + 1

    def _search_integration_dirs(self, name):
        """Search integration dirs for name and return full path."""
        for integration_dir in self.integration_dirs:
            abs_path = os.path.join(self.root_dir, integration_dir)
            test_file = test_finder_utils.run_find_cmd(
                test_finder_utils.FIND_REFERENCE_TYPE.INTEGRATION,
                abs_path, name)
            if test_file:
                return test_file
        return None

    def find_test_by_integration_name(self, name):
        """Find the test info matching the given integration name.

        Args:
            name: A string of integration name as seen in tf's list configs.

        Returns:
            A populated TestInfo namedtuple if test found, else None
        """
        class_name = None
        if ':' in name:
            name, class_name = name.split(':')
        test_file = self._search_integration_dirs(name)
        if test_file is None:
            return None
        # Don't use names that simply match the path,
        # must be the actual name used by TF to run the test.
        match = _INT_NAME_RE.match(test_file)
        if not match:
            logging.error('Integration test outside config dir: %s',
                          test_file)
            return None
        int_name = match.group('int_name')
        if int_name != name:
            logging.warn('Input (%s) not valid integration name, '
                         'did you mean: %s?', name, int_name)
            return None
        rel_config = os.path.relpath(test_file, self.root_dir)

        filters = frozenset()
        if class_name:
            class_name, methods = test_finder_utils.split_methods(class_name)
            if '.' not in class_name:
                logging.warn('Looking up fully qualified class name for: %s.'
                             'Improve speed by using fully qualified names.',
                             class_name)
                path = test_finder_utils.find_class_file(self.root_dir,
                                                         class_name)
                if not path:
                    return None
                class_name = test_finder_utils.get_fully_qualified_class_name(
                    path)
            filters = frozenset([test_info.TestFilter(class_name, methods)])
        return test_info.TestInfo(
            test_name=name,
            test_runner=self._TEST_RUNNER,
            build_targets=self._get_build_targets(rel_config),
            data={constants.TI_REL_CONFIG: rel_config,
                  constants.TI_FILTER: filters})

    def find_int_test_by_path(self, path):
        """Find the first test info matching the given path.

        Strategy:
            path_to_integration_file --> Resolve to INTEGRATION
            # If the path is a dir, we return nothing.
            path_to_dir_with_integration_files --> Return None

        Args:
            path: A string of the test's path.

        Returns:
            A populated TestInfo namedtuple if test found, else None
        """
        path, _ = test_finder_utils.split_methods(path)

        # Make sure we're looking for a config.
        if not path.endswith('.xml'):
            return None

        # TODO: See if this can be generalized and shared with methods above
        # create absolute path from cwd and remove symbolic links
        path = os.path.realpath(path)
        if not os.path.exists(path):
            return None
        dir_path, file_name = test_finder_utils.get_dir_path_and_filename(path)

        int_dir = None
        for possible_dir in self.integration_dirs:
            abs_int_dir = os.path.join(self.root_dir, possible_dir)
            if test_finder_utils.is_equal_or_sub_dir(dir_path, abs_int_dir):
                int_dir = abs_int_dir
                break
        if int_dir:
            if not file_name:
                logging.warn('Found dir (%s) matching input (%s).'
                             ' Referencing an entire Integration/Suite dir'
                             ' is not supported. If you are trying to reference'
                             ' a test by its path, please input the path to'
                             ' the integration/suite config file itself.',
                             int_dir, path)
                return None
            rel_config = os.path.relpath(path, self.root_dir)
            match = _INT_NAME_RE.match(rel_config)
            if not match:
                logging.error('Integration test outside config dir: %s',
                              rel_config)
                return None
            int_name = match.group('int_name')
            return test_info.TestInfo(
                test_name=int_name,
                test_runner=self._TEST_RUNNER,
                build_targets=self._get_build_targets(rel_config),
                data={constants.TI_REL_CONFIG: rel_config,
                      constants.TI_FILTER: frozenset()})
        return None
