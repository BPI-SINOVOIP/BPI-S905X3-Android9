#
# Copyright (C) 2017 The Android Open Source Project
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

import importlib
import json
import logging
import os
import sys
import time
import yaml

from vts.runners.host import asserts
from vts.runners.host import base_test
from vts.runners.host import config_parser
from vts.runners.host import keys
from vts.runners.host import records
from vts.runners.host import test_runner
from vts.utils.python.io import capture_printout
from vts.utils.python.io import file_util

from mobly import test_runner as mobly_test_runner


LIST_TEST_OUTPUT_START = '==========> '
LIST_TEST_OUTPUT_END = ' <=========='
# Temp directory inside python log path. The name is required to be
# the set value for tradefed to skip reading contents as logs.
TEMP_DIR_NAME = 'temp'
CONFIG_FILE_NAME = 'test_config.yaml'
MOBLY_RESULT_JSON_FILE_NAME = 'test_run_summary.json'
MOBLY_RESULT_YAML_FILE_NAME = 'test_summary.yaml'


MOBLY_CONFIG_TEXT = '''TestBeds:
  - Name: {module_name}
    Controllers:
        AndroidDevice:
          - serial: {serial1}
          - serial: {serial2}

MoblyParams:
    LogPath: {log_path}
'''

#TODO(yuexima):
# 1. make DEVICES_REQUIRED configurable
# 2. add include filter function
DEVICES_REQUIRED = 2

RESULT_KEY_TYPE = 'Type'
RESULT_TYPE_SUMMARY = 'Summary'
RESULT_TYPE_RECORD = 'Record'
RESULT_TYPE_TEST_NAME_LIST = 'TestNameList'
RESULT_TYPE_CONTROLLER_INFO = 'ControllerInfo'


class MoblyTest(base_test.BaseTestClass):
    '''Template class for running mobly test cases.

    Attributes:
        mobly_dir: string, mobly test temp directory for mobly runner
        mobly_config_file_path: string, mobly test config file path
        result_handlers: dict, map of result type and handler functions
    '''
    def setUpClass(self):
        asserts.assertEqual(
            len(self.android_devices), DEVICES_REQUIRED,
            'Exactly %s devices are required for this test.' % DEVICES_REQUIRED
        )

        for ad in self.android_devices:
            logging.info('Android device serial: %s' % ad.serial)

        logging.info('Test cases: %s' % self.ListTestCases())

        self.mobly_dir = os.path.join(logging.log_path, TEMP_DIR_NAME,
                                      'mobly', str(time.time()))

        file_util.Makedirs(self.mobly_dir)

        logging.info('mobly log path: %s' % self.mobly_dir)

        self.result_handlers = {
            RESULT_TYPE_SUMMARY: self.HandleSimplePrint,
            RESULT_TYPE_RECORD: self.HandleRecord,
            RESULT_TYPE_TEST_NAME_LIST: self.HandleSimplePrint,
            RESULT_TYPE_CONTROLLER_INFO: self.HandleSimplePrint,
        }

    def tearDownClass(self):
        ''' Clear the mobly directory.'''
        file_util.Rmdirs(self.mobly_dir, ignore_errors=True)

    def PrepareConfigFile(self):
        '''Prepare mobly config file for running test.'''
        self.mobly_config_file_path = os.path.join(self.mobly_dir,
                                                   CONFIG_FILE_NAME)
        config_text = MOBLY_CONFIG_TEXT.format(
              module_name=self.test_module_name,
              serial1=self.android_devices[0].serial,
              serial2=self.android_devices[1].serial,
              log_path=self.mobly_dir
        )
        with open(self.mobly_config_file_path, 'w') as f:
            f.write(config_text)

    def ListTestCases(self):
        '''List test cases.

        Returns:
            List of string, test names.
        '''
        classes = mobly_test_runner._find_test_class()

        with capture_printout.CaptureStdout() as output:
            mobly_test_runner._print_test_names(classes)

        test_names = []

        for line in output:
            if (not line.startswith(LIST_TEST_OUTPUT_START)
                and line.endswith(LIST_TEST_OUTPUT_END)):
                test_names.append(line)
                tr_record = records.TestResultRecord(line, self.test_module_name)
                self.results.requested.append(tr_record)

        return test_names

    def RunMoblyModule(self):
        '''Execute mobly test module.'''
        # Because mobly and vts uses a similar runner, both will modify
        # log_path from python logging. The following step is to preserve
        # log path after mobly test finishes.

        # An alternative way is to start a new python process through shell
        # command. In that case, test print out needs to be piped.
        # This will also help avoid log overlapping

        logger = logging.getLogger()
        logger_path = logger.log_path
        logging_path = logging.log_path

        try:
            mobly_test_runner.main(argv=['-c', self.mobly_config_file_path])
        finally:
            logger.log_path = logger_path
            logging.log_path = logging_path

    def GetMoblyResults(self):
        '''Get mobly module run results and put in vts results.'''
        file_handlers = (
            (MOBLY_RESULT_YAML_FILE_NAME, self.ParseYamlResults),
            (MOBLY_RESULT_JSON_FILE_NAME, self.ParseJsonResults),
        )

        for pair in file_handlers:
            file_path = file_util.FindFile(self.mobly_dir, pair[0])

            if file_path:
                logging.info('Mobly test yaml result path: %s', file_path)
                pair[1](file_path)
                return

        asserts.fail('Mobly test result file not found.')

    def generateAllTests(self):
        '''Run the mobly test module and parse results.'''
        #TODO(yuexima): report test names

        self.PrepareConfigFile()
        self.RunMoblyModule()
        #TODO(yuexima): check whether DEBUG logs from mobly run are included
        self.GetMoblyResults()

    def ParseJsonResults(self, result_path):
        '''Parse mobly test json result.

        Args:
            result_path: string, result json file path.
        '''
        with open(path, 'r') as f:
            mobly_summary = json.load(f)

        mobly_results = mobly_summary['Results']
        for result in mobly_results:
            logging.info('Adding result for %s' % result[records.TestResultEnums.RECORD_NAME])
            record = records.TestResultRecord(result[records.TestResultEnums.RECORD_NAME])
            record.test_class = result[records.TestResultEnums.RECORD_CLASS]
            record.begin_time = result[records.TestResultEnums.RECORD_BEGIN_TIME]
            record.end_time = result[records.TestResultEnums.RECORD_END_TIME]
            record.result = result[records.TestResultEnums.RECORD_RESULT]
            record.uid = result[records.TestResultEnums.RECORD_UID]
            record.extras = result[records.TestResultEnums.RECORD_EXTRAS]
            record.details = result[records.TestResultEnums.RECORD_DETAILS]
            record.extra_errors = result[records.TestResultEnums.RECORD_EXTRA_ERRORS]

            self.results.addRecord(record)

    def ParseYamlResults(self, result_path):
        '''Parse mobly test yaml result.

        Args:
            result_path: string, result yaml file path.
        '''
        with open(result_path, 'r') as stream:
            try:
                docs = yaml.load_all(stream)
                for doc in docs:
                    type = doc.get(RESULT_KEY_TYPE)
                    if type is None:
                        logging.warn(
                            'Mobly result document type unrecognized: %s', doc)
                        continue

                    logging.info('Parsing result type: %s', type)

                    handler = self.result_handlers.get(type)
                    if handler is None:
                        logging.info('Unknown result type: %s', type)
                        handler = self.HandleSimplePrint

                    handler(doc)
            except yaml.YAMLError as exc:
                print(exc)

    def HandleRecord(self, doc):
        '''Handle record result document type.

        Args:
            doc: dict, result document item
        '''
        logging.info('Adding result for %s' % doc.get(records.TestResultEnums.RECORD_NAME))
        record = records.TestResultRecord(doc.get(records.TestResultEnums.RECORD_NAME))
        record.test_class = doc.get(records.TestResultEnums.RECORD_CLASS)
        record.begin_time = doc.get(records.TestResultEnums.RECORD_BEGIN_TIME)
        record.end_time = doc.get(records.TestResultEnums.RECORD_END_TIME)
        record.result = doc.get(records.TestResultEnums.RECORD_RESULT)
        record.uid = doc.get(records.TestResultEnums.RECORD_UID)
        record.extras = doc.get(records.TestResultEnums.RECORD_EXTRAS)
        record.details = doc.get(records.TestResultEnums.RECORD_DETAILS)
        record.extra_errors = doc.get(records.TestResultEnums.RECORD_EXTRA_ERRORS)

        # 'Stacktrace' in yaml result is ignored. 'Stacktrace' is a more
        # detailed version of record.details when exception is emitted.

        self.results.addRecord(record)

    def HandleSimplePrint(self, doc):
        '''Simply print result document to log.

        Args:
            doc: dict, result document item
        '''
        for k, v in doc.items():
            logging.info(str(k) + ": " + str(v))

def GetTestModuleNames():
    '''Returns a list of mobly test module specified in test configuration.'''
    configs = config_parser.load_test_config_file(sys.argv[1])
    reduce_func = lambda x, y: x + y.get(keys.ConfigKeys.MOBLY_TEST_MODULE, [])
    return reduce(reduce_func, configs, [])

def ImportTestModules():
    '''Dynamically import mobly test modules.'''
    for module_name in GetTestModuleNames():
        module, cls = module_name.rsplit('.', 1)
        sys.modules['__main__'].__dict__[cls] = getattr(
            importlib.import_module(module), cls)

if __name__ == "__main__":
    ImportTestModules()
    test_runner.main()
