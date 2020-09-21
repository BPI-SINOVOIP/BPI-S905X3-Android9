from autotest_lib.client.common_lib.cros.cfm.metrics import (
        media_metrics_collector)

import mock
import unittest

# pylint: disable=missing-docstring
class MediaMetricsCollectorTest(unittest.TestCase):

  def test_data_point_collector_same_timestamp_one_entry(self):
      cfm_facade = mock.MagicMock()
      cfm_facade.get_media_info_data_points = mock.Mock(
              return_value = [{'timestamp': 123.1}])
      data_point_collector = media_metrics_collector.DataPointCollector(
              cfm_facade)
      data_point_collector.collect_snapshot()
      data_point_collector.collect_snapshot()
      self.assertEqual(1, len(data_point_collector.get_data_points()))

  def test_data_point_collector_different_timestamps_many_entries(self):
      cfm_facade = mock.MagicMock()
      cfm_facade.get_media_info_data_points = mock.Mock(
              side_effect=[[{'timestamp': 123.1}],
                           [{'timestamp': 124.1}],
                           [{'timestamp': 125.1}]
              ])
      data_point_collector = media_metrics_collector.DataPointCollector(
              cfm_facade)
      data_point_collector.collect_snapshot()
      data_point_collector.collect_snapshot()
      data_point_collector.collect_snapshot()
      self.assertEqual(3, len(data_point_collector.get_data_points()))






