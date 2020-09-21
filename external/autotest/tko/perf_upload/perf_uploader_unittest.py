#!/usr/bin/python

"""Unit tests for the perf_uploader.py module.

"""

import json, unittest

import common
from autotest_lib.tko import models as tko_models
from autotest_lib.tko.perf_upload import perf_uploader


class test_aggregate_iterations(unittest.TestCase):
    """Tests for the aggregate_iterations function."""

    _PERF_ITERATION_DATA = {
        '1': [
            {
                'description': 'metric1',
                'value': 1,
                'stddev': 0.0,
                'units': 'units1',
                'higher_is_better': True,
                'graph': None
            },
            {
                'description': 'metric2',
                'value': 10,
                'stddev': 0.0,
                'units': 'units2',
                'higher_is_better': True,
                'graph': 'graph1',
            },
            {
                'description': 'metric2',
                'value': 100,
                'stddev': 1.7,
                'units': 'units3',
                'higher_is_better': False,
                'graph': 'graph2',
            }
        ],
        '2': [
            {
                'description': 'metric1',
                'value': 2,
                'stddev': 0.0,
                'units': 'units1',
                'higher_is_better': True,
                'graph': None,
            },
            {
                'description': 'metric2',
                'value': 20,
                'stddev': 0.0,
                'units': 'units2',
                'higher_is_better': True,
                'graph': 'graph1',
            },
            {
                'description': 'metric2',
                'value': 200,
                'stddev': 21.2,
                'units': 'units3',
                'higher_is_better': False,
                'graph': 'graph2',
            }
        ],
    }


    def setUp(self):
        """Sets up for each test case."""
        self._perf_values = []
        for iter_num, iter_data in self._PERF_ITERATION_DATA.iteritems():
            self._perf_values.append(
                    tko_models.perf_value_iteration(iter_num, iter_data))



class test_json_config_file_sanity(unittest.TestCase):
    """Sanity tests for the JSON-formatted presentation config file."""

    def test_parse_json(self):
        """Verifies _parse_config_file function."""
        perf_uploader._parse_config_file(
                perf_uploader._PRESENTATION_CONFIG_FILE)


    def test_proper_json(self):
        """Verifies the file can be parsed as proper JSON."""
        try:
            with open(perf_uploader._PRESENTATION_CONFIG_FILE, 'r') as fp:
                json.load(fp)
        except:
            self.fail('Presentation config file could not be parsed as JSON.')


    def test_unique_test_names(self):
        """Verifies that each test name appears only once in the JSON file."""
        json_obj = []
        try:
            with open(perf_uploader._PRESENTATION_CONFIG_FILE, 'r') as fp:
                json_obj = json.load(fp)
        except:
            self.fail('Presentation config file could not be parsed as JSON.')

        name_set = set([x['autotest_name'] for x in json_obj])
        self.assertEqual(len(name_set), len(json_obj),
                         msg='Autotest names not unique in the JSON file.')


    def test_required_master_name(self):
        """Verifies that master name must be specified."""
        json_obj = []
        try:
            with open(perf_uploader._PRESENTATION_CONFIG_FILE, 'r') as fp:
                json_obj = json.load(fp)
        except:
            self.fail('Presentation config file could not be parsed as JSON.')

        for entry in json_obj:
            if not 'master_name' in entry:
                self.fail('Missing master field for test %s.' %
                          entry['autotest_name'])


class test_gather_presentation_info(unittest.TestCase):
    """Tests for the gather_presentation_info function."""

    _PRESENT_INFO = {
        'test_name': {
            'master_name': 'new_master_name',
            'dashboard_test_name': 'new_test_name',
        }
    }

    _PRESENT_INFO_MISSING_MASTER = {
        'test_name': {
            'dashboard_test_name': 'new_test_name',
        }
    }


    def test_test_name_specified(self):
        """Verifies gathers presentation info correctly."""
        result = perf_uploader._gather_presentation_info(
                self._PRESENT_INFO, 'test_name')
        self.assertTrue(
                all([key in result for key in
                     ['test_name', 'master_name']]),
                msg='Unexpected keys in resulting dictionary: %s' % result)
        self.assertEqual(result['master_name'], 'new_master_name',
                         msg='Unexpected "master_name" value: %s' %
                             result['master_name'])
        self.assertEqual(result['test_name'], 'new_test_name',
                         msg='Unexpected "test_name" value: %s' %
                             result['test_name'])


    def test_test_name_not_specified(self):
        """Verifies exception raised if test is not there."""
        self.assertRaises(
                perf_uploader.PerfUploadingError,
                perf_uploader._gather_presentation_info,
                        self._PRESENT_INFO, 'other_test_name')


    def test_master_not_specified(self):
        """Verifies exception raised if master is not there."""
        self.assertRaises(
                perf_uploader.PerfUploadingError,
                perf_uploader._gather_presentation_info,
                    self._PRESENT_INFO_MISSING_MASTER, 'test_name')


class test_get_id_from_version(unittest.TestCase):
    """Tests for the _get_id_from_version function."""

    def test_correctly_formatted_versions(self):
        """Verifies that the expected ID is returned when input is OK."""
        chrome_version = '27.0.1452.2'
        cros_version = '27.3906.0.0'
        # 1452.2 + 3906.0.0
        # --> 01452 + 002 + 03906 + 000 + 00
        # --> 14520020390600000
        self.assertEqual(
                14520020390600000,
                perf_uploader._get_id_from_version(
                        chrome_version, cros_version))

        chrome_version = '25.10.1000.0'
        cros_version = '25.1200.0.0'
        # 1000.0 + 1200.0.0
        # --> 01000 + 000 + 01200 + 000 + 00
        # --> 10000000120000000
        self.assertEqual(
                10000000120000000,
                perf_uploader._get_id_from_version(
                        chrome_version, cros_version))

    def test_returns_none_when_given_invalid_input(self):
        """Checks the return value when invalid input is given."""
        chrome_version = '27.0'
        cros_version = '27.3906.0.0'
        self.assertIsNone(perf_uploader._get_id_from_version(
                chrome_version, cros_version))


class test_get_version_numbers(unittest.TestCase):
    """Tests for the _get_version_numbers function."""

    def test_with_valid_versions(self):
      """Checks the version numbers used when data is formatted as expected."""
      self.assertEqual(
              ('34.5678.9.0', '34.5.678.9'),
              perf_uploader._get_version_numbers(
                  {
                      'CHROME_VERSION': '34.5.678.9',
                      'CHROMEOS_RELEASE_VERSION': '5678.9.0',
                  }))

    def test_with_missing_version_raises_error(self):
      """Checks that an error is raised when a version is missing."""
      with self.assertRaises(perf_uploader.PerfUploadingError):
          perf_uploader._get_version_numbers(
              {
                  'CHROMEOS_RELEASE_VERSION': '5678.9.0',
              })

    def test_with_unexpected_version_format_raises_error(self):
      """Checks that an error is raised when there's a rN suffix."""
      with self.assertRaises(perf_uploader.PerfUploadingError):
          perf_uploader._get_version_numbers(
              {
                  'CHROME_VERSION': '34.5.678.9',
                  'CHROMEOS_RELEASE_VERSION': '5678.9.0r1',
              })

    def test_with_valid_release_milestone(self):
      """Checks the version numbers used when data is formatted as expected."""
      self.assertEqual(
              ('54.5678.9.0', '34.5.678.9'),
              perf_uploader._get_version_numbers(
                  {
                      'CHROME_VERSION': '34.5.678.9',
                      'CHROMEOS_RELEASE_VERSION': '5678.9.0',
                      'CHROMEOS_RELEASE_CHROME_MILESTONE': '54',
                  }))


class test_format_for_upload(unittest.TestCase):
    """Tests for the format_for_upload function."""

    _PERF_DATA = {
     "charts": {
            "metric1": {
                "summary": {
                    "improvement_direction": "down",
                    "type": "scalar",
                    "units": "msec",
                    "value": 2.7,
                }
            },
            "metric2": {
                "summary": {
                    "improvement_direction": "up",
                    "type": "scalar",
                    "units": "frames_per_sec",
                    "value": 101.35,
                }
            }
        },
    }
    _PRESENT_INFO = {
        'master_name': 'new_master_name',
        'test_name': 'new_test_name',
    }

    def setUp(self):
        self._perf_data = self._PERF_DATA

    def _verify_result_string(self, actual_result, expected_result):
        """Verifies a JSON string matches the expected result.

        This function compares JSON objects rather than strings, because of
        possible floating-point values that need to be compared using
        assertAlmostEqual().

        @param actual_result: The candidate JSON string.
        @param expected_result: The reference JSON string that the candidate
            must match.

        """
        actual = json.loads(actual_result)
        expected = json.loads(expected_result)

        def ordered(obj):
            if isinstance(obj, dict):
               return sorted((k, ordered(v)) for k, v in obj.items())
            if isinstance(obj, list):
               return sorted(ordered(x) for x in obj)
            else:
               return obj
        fail_msg = 'Unexpected result string: %s' % actual_result
        self.assertEqual(ordered(expected), ordered(actual), msg=fail_msg)


    def test_format_for_upload(self):
        """Verifies format_for_upload generates correct json data."""
        result = perf_uploader._format_for_upload(
                'platform', '25.1200.0.0', '25.10.1000.0', 'WINKY E2A-F2K-Q35',
                'i7', 'test_machine', self._perf_data, self._PRESENT_INFO,
                '52926644-username/hostname')
        expected_result_string = (
          '{"versions":  {'
             '"cros_version": "25.1200.0.0",'
             '"chrome_version": "25.10.1000.0"'
          '},'
          '"point_id": 10000000120000000,'
          '"bot": "cros-platform-i7",'
          '"chart_data": {'
             '"charts": {'
               '"metric2": {'
                 '"summary": {'
                   '"units": "frames_per_sec",'
                   '"type": "scalar",'
                   '"value": 101.35,'
                   '"improvement_direction": "up"'
                 '}'
               '},'
               '"metric1": {'
                 '"summary": {'
                 '"units": "msec",'
                 '"type": "scalar",'
                 '"value": 2.7,'
                 '"improvement_direction": "down"}'
               '}'
             '}'
          '},'
          '"master": "new_master_name",'
          '"supplemental": {'
             '"hardware_identifier": "WINKY E2A-F2K-Q35",'
             '"jobname": "52926644-username/hostname",'
             '"hardware_hostname": "test_machine",'
             '"default_rev": "r_cros_version",'
             '"variant_name": "i7"}'
           '}')
        self._verify_result_string(result['data'], expected_result_string)


if __name__ == '__main__':
    unittest.main()
