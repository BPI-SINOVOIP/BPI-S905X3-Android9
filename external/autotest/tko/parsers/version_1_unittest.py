#!/usr/bin/python

import datetime, time, unittest

import common
from autotest_lib.client.common_lib import utils
from autotest_lib.tko.parsers import version_1


class test_status_line(unittest.TestCase):
    """Tests for status lines."""

    statuses = ['GOOD', 'WARN', 'FAIL', 'ABORT']


    def test_handles_start(self):
        """Tests that START is handled properly."""
        line = version_1.status_line(0, 'START', '----', 'test',
                                     '', {})
        self.assertEquals(line.type, 'START')
        self.assertEquals(line.status, None)


    def test_handles_info(self):
        """Tests that INFO is handled properly."""
        line = version_1.status_line(0, 'INFO', '----', '----',
                                     '', {})
        self.assertEquals(line.type, 'INFO')
        self.assertEquals(line.status, None)


    def test_handles_status(self):
        """Tests that STATUS is handled properly."""
        for stat in self.statuses:
            line = version_1.status_line(0, stat, '----', 'test',
                                         '', {})
            self.assertEquals(line.type, 'STATUS')
            self.assertEquals(line.status, stat)


    def test_handles_endstatus(self):
        """Tests that END is handled properly."""
        for stat in self.statuses:
            line = version_1.status_line(0, 'END ' + stat, '----',
                                         'test', '', {})
            self.assertEquals(line.type, 'END')
            self.assertEquals(line.status, stat)


    def test_fails_on_bad_status(self):
        """Tests that an exception is raised on a bad status."""
        for stat in self.statuses:
            self.assertRaises(AssertionError,
                              version_1.status_line, 0,
                              'BAD ' + stat, '----', 'test',
                              '', {})


    def test_saves_all_fields(self):
        """Tests that all fields are saved."""
        line = version_1.status_line(5, 'GOOD', 'subdir_name',
                                     'test_name', 'my reason here',
                                     {'key1': 'value',
                                      'key2': 'another value',
                                      'key3': 'value3'})
        self.assertEquals(line.indent, 5)
        self.assertEquals(line.status, 'GOOD')
        self.assertEquals(line.subdir, 'subdir_name')
        self.assertEquals(line.testname, 'test_name')
        self.assertEquals(line.reason, 'my reason here')
        self.assertEquals(line.optional_fields,
                          {'key1': 'value', 'key2': 'another value',
                           'key3': 'value3'})


    def test_parses_blank_subdir(self):
        """Tests that a blank subdirectory is parsed properly."""
        line = version_1.status_line(0, 'GOOD', '----', 'test',
                                     '', {})
        self.assertEquals(line.subdir, None)


    def test_parses_blank_testname(self):
        """Tests that a blank test name is parsed properly."""
        line = version_1.status_line(0, 'GOOD', 'subdir', '----',
                                     '', {})
        self.assertEquals(line.testname, None)


    def test_parse_line_smoketest(self):
        """Runs a parse line smoke test."""
        input_data = ('\t\t\tGOOD\t----\t----\t'
                      'field1=val1\tfield2=val2\tTest Passed')
        line = version_1.status_line.parse_line(input_data)
        self.assertEquals(line.indent, 3)
        self.assertEquals(line.type, 'STATUS')
        self.assertEquals(line.status, 'GOOD')
        self.assertEquals(line.subdir, None)
        self.assertEquals(line.testname, None)
        self.assertEquals(line.reason, 'Test Passed')
        self.assertEquals(line.optional_fields,
                          {'field1': 'val1', 'field2': 'val2'})

    def test_parse_line_handles_newline(self):
        """Tests that newlines are handled properly."""
        input_data = ('\t\tGOOD\t----\t----\t'
                      'field1=val1\tfield2=val2\tNo newline here!')
        for suffix in ('', '\n'):
            line = version_1.status_line.parse_line(input_data +
                                                    suffix)
            self.assertEquals(line.indent, 2)
            self.assertEquals(line.type, 'STATUS')
            self.assertEquals(line.status, 'GOOD')
            self.assertEquals(line.subdir, None)
            self.assertEquals(line.testname, None)
            self.assertEquals(line.reason, 'No newline here!')
            self.assertEquals(line.optional_fields,
                              {'field1': 'val1',
                               'field2': 'val2'})


    def test_parse_line_fails_on_untabbed_lines(self):
        """Tests that untabbed lines do not parse."""
        input_data = '   GOOD\trandom\tfields\tof text'
        line = version_1.status_line.parse_line(input_data)
        self.assertEquals(line, None)
        line = version_1.status_line.parse_line(input_data.lstrip())
        self.assertEquals(line.indent, 0)
        self.assertEquals(line.type, 'STATUS')
        self.assertEquals(line.status, 'GOOD')
        self.assertEquals(line.subdir, 'random')
        self.assertEquals(line.testname, 'fields')
        self.assertEquals(line.reason, 'of text')
        self.assertEquals(line.optional_fields, {})


    def test_parse_line_fails_on_incomplete_lines(self):
        """Tests that incomplete lines do not parse."""
        input_data = '\t\tGOOD\tfield\tsecond field'
        complete_data = input_data + '\tneeded last field'
        line = version_1.status_line.parse_line(input_data)
        self.assertEquals(line, None)
        line = version_1.status_line.parse_line(complete_data)
        self.assertEquals(line.indent, 2)
        self.assertEquals(line.type, 'STATUS')
        self.assertEquals(line.status, 'GOOD')
        self.assertEquals(line.subdir, 'field')
        self.assertEquals(line.testname, 'second field')
        self.assertEquals(line.reason, 'needed last field')
        self.assertEquals(line.optional_fields, {})


    def test_good_reboot_passes_success_test(self):
        """Tests good reboot statuses."""
        line = version_1.status_line(0, 'NOSTATUS', None, 'reboot',
                                     'reboot success', {})
        self.assertEquals(line.is_successful_reboot('GOOD'), True)
        self.assertEquals(line.is_successful_reboot('WARN'), True)


    def test_bad_reboot_passes_success_test(self):
        """Tests bad reboot statuses."""
        line = version_1.status_line(0, 'NOSTATUS', None, 'reboot',
                                     'reboot success', {})
        self.assertEquals(line.is_successful_reboot('FAIL'), False)
        self.assertEquals(line.is_successful_reboot('ABORT'), False)


    def test_get_kernel_returns_kernel_plus_patches(self):
        """Tests that get_kernel returns the appropriate info."""
        line = version_1.status_line(0, 'GOOD', 'subdir', 'testname',
                                     'reason text',
                                     {'kernel': '2.6.24-rc40',
                                      'patch0': 'first_patch 0 0',
                                      'patch1': 'another_patch 0 0'})
        kern = line.get_kernel()
        kernel_hash = utils.hash('md5', '2.6.24-rc40,0,0').hexdigest()
        self.assertEquals(kern.base, '2.6.24-rc40')
        self.assertEquals(kern.patches[0].spec, 'first_patch')
        self.assertEquals(kern.patches[1].spec, 'another_patch')
        self.assertEquals(len(kern.patches), 2)
        self.assertEquals(kern.kernel_hash, kernel_hash)


    def test_get_kernel_ignores_out_of_sequence_patches(self):
        """Tests that get_kernel ignores patches that are out of sequence."""
        line = version_1.status_line(0, 'GOOD', 'subdir', 'testname',
                                     'reason text',
                                     {'kernel': '2.6.24-rc40',
                                      'patch0': 'first_patch 0 0',
                                      'patch2': 'another_patch 0 0'})
        kern = line.get_kernel()
        kernel_hash = utils.hash('md5', '2.6.24-rc40,0').hexdigest()
        self.assertEquals(kern.base, '2.6.24-rc40')
        self.assertEquals(kern.patches[0].spec, 'first_patch')
        self.assertEquals(len(kern.patches), 1)
        self.assertEquals(kern.kernel_hash, kernel_hash)


    def test_get_kernel_returns_unknown_with_no_kernel(self):
        """Tests that a missing kernel is handled properly."""
        line = version_1.status_line(0, 'GOOD', 'subdir', 'testname',
                                     'reason text',
                                     {'patch0': 'first_patch 0 0',
                                      'patch2': 'another_patch 0 0'})
        kern = line.get_kernel()
        self.assertEquals(kern.base, 'UNKNOWN')
        self.assertEquals(kern.patches, [])
        self.assertEquals(kern.kernel_hash, 'UNKNOWN')


    def test_get_timestamp_returns_timestamp_field(self):
        """Tests that get_timestamp returns the expected info."""
        timestamp = datetime.datetime(1970, 1, 1, 4, 30)
        timestamp -= datetime.timedelta(seconds=time.timezone)
        line = version_1.status_line(0, 'GOOD', 'subdir', 'testname',
                                     'reason text',
                                     {'timestamp': '16200'})
        self.assertEquals(timestamp, line.get_timestamp())


    def test_get_timestamp_returns_none_on_missing_field(self):
        """Tests that get_timestamp returns None if no timestamp exists."""
        line = version_1.status_line(0, 'GOOD', 'subdir', 'testname',
                                     'reason text', {})
        self.assertEquals(None, line.get_timestamp())


class iteration_parse_line_into_dicts(unittest.TestCase):
    """Tests for parsing iteration keyvals into dictionaries."""

    def parse_line(self, line):
        """
        Invokes parse_line_into_dicts with two empty dictionaries.

        @param line: The line to parse.

        @return A 2-tuple representing the filled-in attr and perf dictionaries,
            respectively.

        """
        attr, perf = {}, {}
        version_1.iteration.parse_line_into_dicts(line, attr, perf)
        return attr, perf


    def test_perf_entry(self):
        """Tests a basic perf keyval line."""
        result = self.parse_line('perf-val{perf}=-173')
        self.assertEqual(({}, {'perf-val': -173}), result)


    def test_attr_entry(self):
        """Tests a basic attr keyval line."""
        result = self.parse_line('attr-val{attr}=173')
        self.assertEqual(({'attr-val': '173'}, {}), result)


    def test_untagged_is_perf(self):
        """Tests that an untagged keyval is considered to be perf by default."""
        result = self.parse_line('untagged=-678.5e5')
        self.assertEqual(({}, {'untagged': -678.5e5}), result)


    def test_invalid_tag_ignored(self):
        """Tests that invalid tags are ignored."""
        result = self.parse_line('bad-tag{invalid}=56')
        self.assertEqual(({}, {}), result)


    def test_non_numeric_perf_ignored(self):
        """Tests that non-numeric perf values are ignored."""
        result = self.parse_line('perf-val{perf}=FooBar')
        self.assertEqual(({}, {}), result)


    def test_non_numeric_untagged_ignored(self):
        """Tests that non-numeric untagged keyvals are ignored."""
        result = self.parse_line('untagged=FooBar')
        self.assertEqual(({}, {}), result)


class perf_value_iteration_parse_line_into_dict(unittest.TestCase):
    """Tests for parsing perf value iterations into a dictionary."""

    def parse_line(self, line):
        """
        Invokes parse_line_into_dict with a line to parse.

        @param line: The string line to parse.

        @return A dictionary containing the information parsed from the line.

        """
        return version_1.perf_value_iteration.parse_line_into_dict(line)

    def test_invalid_json(self):
        """Tests that a non-JSON line is handled properly."""
        result = self.parse_line('{"invalid_json" "string"}')
        self.assertEqual(result, {})

    def test_single_value_int(self):
        """Tests that a single integer value is parsed properly."""
        result = self.parse_line('{"value": 7}')
        self.assertEqual(result, {'value': 7, 'stddev': 0})

    def test_single_value_float(self):
        """Tests that a single float value is parsed properly."""
        result = self.parse_line('{"value": 1.298}')
        self.assertEqual(result, {'value': 1.298, 'stddev': 0})

    def test_value_list_int(self):
        """Tests that an integer list is parsed properly."""
        result = self.parse_line('{"value": [10, 20, 30]}')
        self.assertEqual(result, {'value': 20.0, 'stddev': 10.0})

    def test_value_list_float(self):
        """Tests that a float list is parsed properly."""
        result = self.parse_line('{"value": [2.0, 3.0, 4.0]}')
        self.assertEqual(result, {'value': 3.0, 'stddev': 1.0})


class DummyAbortTestCase(unittest.TestCase):
    """Tests for the make_dummy_abort function."""

    def setUp(self):
        self.indent = 3
        self.subdir = "subdir"
        self.testname = 'testname'
        self.timestamp = 1220565792
        self.reason = 'Job aborted unexpectedly'


    def test_make_dummy_abort_with_timestamp(self):
        """Tests make_dummy_abort with a timestamp specified."""
        abort = version_1.parser.make_dummy_abort(
            self.indent, self.subdir, self.testname, self.timestamp,
            self.reason)
        self.assertEquals(
            abort, '%sEND ABORT\t%s\t%s\ttimestamp=%d\t%s' % (
            '\t' * self.indent, self.subdir, self.testname, self.timestamp,
            self.reason))

    def test_make_dummy_abort_with_no_subdir(self):
        """Tests make_dummy_abort with no subdir specified."""
        abort= version_1.parser.make_dummy_abort(
            self.indent, None, self.testname, self.timestamp, self.reason)
        self.assertEquals(
            abort, '%sEND ABORT\t----\t%s\ttimestamp=%d\t%s' % (
            '\t' * self.indent, self.testname, self.timestamp, self.reason))

    def test_make_dummy_abort_with_no_testname(self):
        """Tests make_dummy_abort with no testname specified."""
        abort= version_1.parser.make_dummy_abort(
        self.indent, self.subdir, None, self.timestamp, self.reason)
        self.assertEquals(
            abort, '%sEND ABORT\t%s\t----\ttimestamp=%d\t%s' % (
            '\t' * self.indent, self.subdir, self.timestamp, self.reason))

    def test_make_dummy_abort_no_timestamp(self):
        """Tests make_dummy_abort with no timestamp specified."""
        abort = version_1.parser.make_dummy_abort(
            self.indent, self.subdir, self.testname, None, self.reason)
        self.assertEquals(
            abort, '%sEND ABORT\t%s\t%s\t%s' % (
            '\t' * self.indent, self.subdir, self.testname, self.reason))


if __name__ == '__main__':
    unittest.main()
