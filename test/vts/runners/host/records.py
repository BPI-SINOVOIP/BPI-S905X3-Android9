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
"""This module is where all the record definitions and record containers live.
"""

import json
import logging
import pprint

from vts.runners.host import signals
from vts.runners.host import utils
from vts.utils.python.common import list_utils


class TestResultEnums(object):
    """Enums used for TestResultRecord class.

    Includes the tokens to mark test result with, and the string names for each
    field in TestResultRecord.
    """

    RECORD_NAME = "Test Name"
    RECORD_CLASS = "Test Class"
    RECORD_BEGIN_TIME = "Begin Time"
    RECORD_END_TIME = "End Time"
    RECORD_RESULT = "Result"
    RECORD_UID = "UID"
    RECORD_EXTRAS = "Extras"
    RECORD_EXTRA_ERRORS = "Extra Errors"
    RECORD_DETAILS = "Details"
    RECORD_TABLES = "Tables"
    TEST_RESULT_PASS = "PASS"
    TEST_RESULT_FAIL = "FAIL"
    TEST_RESULT_SKIP = "SKIP"
    TEST_RESULT_ERROR = "ERROR"


class TestResultRecord(object):
    """A record that holds the information of a test case execution.

    Attributes:
        test_name: A string representing the name of the test case.
        begin_time: Epoch timestamp of when the test case started.
        end_time: Epoch timestamp of when the test case ended.
        uid: Unique identifier of a test case.
        result: Test result, PASS/FAIL/SKIP.
        extras: User defined extra information of the test result.
        details: A string explaining the details of the test case.
        tables: A dict of 2-dimensional lists containing tabular results.
    """

    def __init__(self, t_name, t_class=None):
        self.test_name = t_name
        self.test_class = t_class
        self.begin_time = None
        self.end_time = None
        self.uid = None
        self.result = None
        self.extras = None
        self.details = None
        self.extra_errors = {}
        self.tables = {}

    def testBegin(self):
        """Call this when the test case it records begins execution.

        Sets the begin_time of this record.
        """
        self.begin_time = utils.get_current_epoch_time()

    def _testEnd(self, result, e):
        """Class internal function to signal the end of a test case execution.

        Args:
            result: One of the TEST_RESULT enums in TestResultEnums.
            e: A test termination signal (usually an exception object). It can
                be any exception instance or of any subclass of
                vts.runners.host.signals.TestSignal.
        """
        self.end_time = utils.get_current_epoch_time()
        self.result = result
        if isinstance(e, signals.TestSignal):
            self.details = e.details
            self.extras = e.extras
        elif e:
            self.details = str(e)

    def testPass(self, e=None):
        """To mark the test as passed in this record.

        Args:
            e: An instance of vts.runners.host.signals.TestPass.
        """
        self._testEnd(TestResultEnums.TEST_RESULT_PASS, e)

    def testFail(self, e=None):
        """To mark the test as failed in this record.

        Only testFail does instance check because we want "assert xxx" to also
        fail the test same way assert_true does.

        Args:
            e: An exception object. It can be an instance of AssertionError or
                vts.runners.host.base_test.TestFailure.
        """
        self._testEnd(TestResultEnums.TEST_RESULT_FAIL, e)

    def testSkip(self, e=None):
        """To mark the test as skipped in this record.

        Args:
            e: An instance of vts.runners.host.signals.TestSkip.
        """
        self._testEnd(TestResultEnums.TEST_RESULT_SKIP, e)

    def testError(self, e=None):
        """To mark the test as error in this record.

        Args:
            e: An exception object.
        """
        self._testEnd(TestResultEnums.TEST_RESULT_ERROR, e)

    def addError(self, tag, e):
        """Add extra error happened during a test mark the test result as
        ERROR.

        If an error is added the test record, the record's result is equivalent
        to the case where an uncaught exception happened.

        Args:
            tag: A string describing where this error came from, e.g. 'on_pass'.
            e: An exception object.
        """
        self.result = TestResultEnums.TEST_RESULT_ERROR
        self.extra_errors[tag] = str(e)

    def addTable(self, name, rows):
        """Add a table as part of the test result.

        Args:
            name: The table name.
            rows: A 2-dimensional list which contains the data.
        """
        if name in self.tables:
            logging.warning("Overwrite table %s" % name)
        self.tables[name] = rows

    def __str__(self):
        d = self.getDict()
        l = ["%s = %s" % (k, v) for k, v in d.items()]
        s = ', '.join(l)
        return s

    def __repr__(self):
        """This returns a short string representation of the test record."""
        t = utils.epoch_to_human_time(self.begin_time)
        return "%s %s %s" % (t, self.test_name, self.result)

    def getDict(self):
        """Gets a dictionary representating the content of this class.

        Returns:
            A dictionary representating the content of this class.
        """
        d = {}
        d[TestResultEnums.RECORD_NAME] = self.test_name
        d[TestResultEnums.RECORD_CLASS] = self.test_class
        d[TestResultEnums.RECORD_BEGIN_TIME] = self.begin_time
        d[TestResultEnums.RECORD_END_TIME] = self.end_time
        d[TestResultEnums.RECORD_RESULT] = self.result
        d[TestResultEnums.RECORD_UID] = self.uid
        d[TestResultEnums.RECORD_EXTRAS] = self.extras
        d[TestResultEnums.RECORD_DETAILS] = self.details
        d[TestResultEnums.RECORD_EXTRA_ERRORS] = self.extra_errors
        d[TestResultEnums.RECORD_TABLES] = self.tables
        return d

    def jsonString(self):
        """Converts this test record to a string in json format.

        Format of the json string is:
            {
                'Test Name': <test name>,
                'Begin Time': <epoch timestamp>,
                'Details': <details>,
                ...
            }

        Returns:
            A json-format string representing the test record.
        """
        return json.dumps(self.getDict())


class TestResult(object):
    """A class that contains metrics of a test run.

    This class is essentially a container of TestResultRecord objects.

    Attributes:
        self.requested: A list of records for tests requested by user.
        self.failed: A list of records for tests failed.
        self.executed: A list of records for tests that were actually executed.
        self.passed: A list of records for tests passed.
        self.skipped: A list of records for tests skipped.
        self.error: A list of records for tests with error result token.
        self._test_module_name: A string, test module's name.
        self._test_module_timestamp: An integer, test module's execution start
                                     timestamp.
    """

    def __init__(self):
        self.requested = []
        self.failed = []
        self.executed = []
        self.passed = []
        self.skipped = []
        self.error = []
        self._test_module_name = None
        self._test_module_timestamp = None

    def __add__(self, r):
        """Overrides '+' operator for TestResult class.

        The add operator merges two TestResult objects by concatenating all of
        their lists together.

        Args:
            r: another instance of TestResult to be added

        Returns:
            A TestResult instance that's the sum of two TestResult instances.
        """
        if not isinstance(r, TestResult):
            raise TypeError("Operand %s of type %s is not a TestResult." %
                            (r, type(r)))
        r.reportNonExecutedRecord()
        sum_result = TestResult()
        for name in sum_result.__dict__:
            if name.startswith("_test_module"):
                l_value = getattr(self, name)
                r_value = getattr(r, name)
                if l_value is None and r_value is None:
                    continue
                elif l_value is None and r_value is not None:
                    value = r_value
                elif l_value is not None and r_value is None:
                    value = l_value
                else:
                    if name == "_test_module_name":
                        if l_value != r_value:
                            raise TypeError("_test_module_name is different.")
                        value = l_value
                    elif name == "_test_module_timestamp":
                        if int(l_value) < int(r_value):
                            value = l_value
                        else:
                            value = r_value
                    else:
                        raise TypeError("unknown _test_module* attribute.")
                setattr(sum_result, name, value)
            else:
                l_value = list(getattr(self, name))
                r_value = list(getattr(r, name))
                setattr(sum_result, name, l_value + r_value)
        return sum_result

    def reportNonExecutedRecord(self):
        """Check and report any requested tests that did not finish.

        Adds a test record to self.error list iff it is in requested list but not
        self.executed result list.
        """
        for requested in self.requested:
            found = False

            for executed in self.executed:
                if (requested.test_name == executed.test_name and
                        requested.test_class == executed.test_class):
                    found = True
                    break

            if not found:
                requested.testBegin()
                requested.testError(
                    "Unknown error: test case requested but not executed.")
                self.error.append(requested)

    def removeRecord(self, record):
        """Remove a test record from test results.

        Records will be ed using test_name and test_class attribute.
        All entries that match the provided record in all result lists will
        be removed after calling this method.

        Args:
            record: A test record object to add.
        """
        lists = [
            self.requested, self.failed, self.executed, self.passed,
            self.skipped, self.error
        ]

        for l in lists:
            indexToRemove = []
            for idx in range(len(l)):
                if (l[idx].test_name == record.test_name and
                        l[idx].test_class == record.test_class):
                    indexToRemove.append(idx)

            for idx in reversed(indexToRemove):
                del l[idx]

    def addRecord(self, record):
        """Adds a test record to test results.

        A record is considered executed once it's added to the test result.

        Args:
            record: A test record object to add.
        """
        self.executed.append(record)
        if record.result == TestResultEnums.TEST_RESULT_FAIL:
            self.failed.append(record)
        elif record.result == TestResultEnums.TEST_RESULT_SKIP:
            self.skipped.append(record)
        elif record.result == TestResultEnums.TEST_RESULT_PASS:
            self.passed.append(record)
        else:
            self.error.append(record)

    def setTestModuleKeys(self, name, start_timestamp):
        """Sets the test module's name and start_timestamp."""
        self._test_module_name = name
        self._test_module_timestamp = start_timestamp

    def failClass(self, class_name, e):
        """Add a record to indicate a test class setup has failed and no test
        in the class was executed.

        Args:
            class_name: A string that is the name of the failed test class.
            e: An exception object.
        """
        record = TestResultRecord("setup_class", class_name)
        record.testBegin()
        record.testFail(e)
        self.executed.append(record)
        self.failed.append(record)

    def passClass(self, class_name, e=None):
        """Add a record to indicate a test class setup has passed and no test
        in the class was executed.

        Args:
            class_name: A string that is the name of the failed test class.
            e: An exception object.
        """
        record = TestResultRecord("setup_class", class_name)
        record.testBegin()
        record.testPass(e)
        self.executed.append(record)
        self.passed.append(record)

    def skipClass(self, class_name, reason):
        """Add a record to indicate all test cases in the class are skipped.

        Args:
            class_name: A string that is the name of the skipped test class.
            reason: A string that is the reason for skipping.
        """
        record = TestResultRecord("unknown", class_name)
        record.testBegin()
        record.testSkip(signals.TestSkip(reason))
        self.executed.append(record)
        self.skipped.append(record)

    def jsonString(self):
        """Converts this test result to a string in json format.

        Format of the json string is:
            {
                "Results": [
                    {<executed test record 1>},
                    {<executed test record 2>},
                    ...
                ],
                "Summary": <summary dict>
            }

        Returns:
            A json-format string representing the test results.
        """
        records = list_utils.MergeUniqueKeepOrder(
            self.executed, self.failed, self.passed, self.skipped, self.error)
        executed = [record.getDict() for record in records]

        d = {}
        d["Results"] = executed
        d["Summary"] = self.summaryDict()
        d["TestModule"] = self.testModuleDict()
        jsonString = json.dumps(d, indent=4, sort_keys=True)
        return jsonString

    def summary(self):
        """Gets a string that summarizes the stats of this test result.

        The summary rovides the counts of how many test cases fall into each
        category, like "Passed", "Failed" etc.

        Format of the string is:
            Requested <int>, Executed <int>, ...

        Returns:
            A summary string of this test result.
        """
        l = ["%s %d" % (k, v) for k, v in self.summaryDict().items()]
        # Sort the list so the order is the same every time.
        msg = ", ".join(sorted(l))
        return msg

    def summaryDict(self):
        """Gets a dictionary that summarizes the stats of this test result.

        The summary rovides the counts of how many test cases fall into each
        category, like "Passed", "Failed" etc.

        Returns:
            A dictionary with the stats of this test result.
        """
        d = {}
        d["Requested"] = len(self.requested)
        d["Executed"] = len(self.executed)
        d["Passed"] = len(self.passed)
        d["Failed"] = len(self.failed)
        d["Skipped"] = len(self.skipped)
        d["Error"] = len(self.error)
        return d

    def testModuleDict(self):
        """Returns a dict that summarizes the test module DB indexing keys."""
        d = {}
        d["Name"] = self._test_module_name
        d["Timestamp"] = self._test_module_timestamp
        return d
