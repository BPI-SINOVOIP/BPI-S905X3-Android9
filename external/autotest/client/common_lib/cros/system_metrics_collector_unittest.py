from autotest_lib.client.common_lib.cros import system_metrics_collector

import unittest

# pylint: disable=missing-docstring
class TestSystemMetricsCollector(unittest.TestCase):
    """
    Tests for the system_metrics_collector module.
    """
    def test_mem_usage_metric(self):
        metric = system_metrics_collector.MemUsageMetric(FakeSystemFacade())
        metric.collect_metric()
        self.assertAlmostEqual(60, metric.values[0])

    def test_file_handles_metric(self):
        metric = system_metrics_collector.AllocatedFileHandlesMetric(
                FakeSystemFacade())
        metric.collect_metric()
        self.assertEqual(11, metric.values[0])

    def test_cpu_usage_metric(self):
        metric = system_metrics_collector.CpuUsageMetric(FakeSystemFacade())
        # Collect twice since the first collection only sets the baseline for
        # the following diff calculations.
        metric.collect_metric()
        metric.collect_metric()
        self.assertAlmostEqual(40, metric.values[0])

    def test_tempature_metric(self):
        metric = system_metrics_collector.TemperatureMetric(FakeSystemFacade())
        metric.collect_metric()
        self.assertAlmostEqual(43, metric.values[0])

    def test_collector(self):
        collector = system_metrics_collector.SystemMetricsCollector(
                FakeSystemFacade(), [TestMetric()])
        collector.collect_snapshot()
        d = {}
        def _write_func(**kwargs):
            d.update(kwargs)
        collector.write_metrics(_write_func)
        self.assertEquals('test_description', d['description'])
        self.assertEquals([1], d['value'])
        self.assertEquals(False, d['higher_is_better'])
        self.assertEquals('test_unit', d['units'])

    def test_collector_default_set_of_metrics_no_error(self):
        # Only verify no errors are thrown when collecting using
        # the default metric set.
        collector = system_metrics_collector.SystemMetricsCollector(
                FakeSystemFacade())
        collector.collect_snapshot()
        collector.collect_snapshot()
        collector.write_metrics(lambda **kwargs: None)

    def test_peak_metric_description(self):
        test_metric = TestMetric()
        peak_metric = system_metrics_collector.PeakMetric(test_metric)
        self.assertEqual(peak_metric.description, 'peak_test_description')

    def test_peak_metric_one_element(self):
        test_metric = TestMetric()
        peak_metric = system_metrics_collector.PeakMetric(test_metric)
        test_metric.collect_metric()
        peak_metric.collect_metric()
        self.assertEqual(peak_metric.values, [1])

    def test_peak_metric_many_elements(self):
        test_metric = TestMetric()
        peak_metric = system_metrics_collector.PeakMetric(test_metric)
        test_metric.collect_metric()
        test_metric.value = 2
        test_metric.collect_metric()
        test_metric.value = 0
        test_metric.collect_metric()
        peak_metric.collect_metric()
        self.assertEqual(peak_metric.values, [2])

class FakeSystemFacade(object):
    def __init__(self):
        self.mem_total_mb = 1000.0
        self.mem_free_mb = 400.0
        self.file_handles = 11
        self.active_cpu_time = 0.4
        self.current_temperature_max = 43

    def get_mem_total(self):
        return self.mem_total_mb

    def get_mem_free_plus_buffers_and_cached(self):
        return self.mem_free_mb

    def get_num_allocated_file_handles(self):
        return self.file_handles

    def get_cpu_usage(self):
        return {}

    def compute_active_cpu_time(self, last_usage, current_usage):
        return self.active_cpu_time

    def get_current_temperature_max(self):
        return self.current_temperature_max

class TestMetric(system_metrics_collector.Metric):
    def __init__(self):
        super(TestMetric, self).__init__(
                'test_description', units='test_unit')
        self.value = 1

    def collect_metric(self):
        self.values.append(self.value)


