class Metric(object):
    """Abstract base class for metrics."""
    def __init__(self,
                 description,
                 units=None,
                 higher_is_better=False):
        """
        Initializes a Metric.
        @param description: Description of the metric, e.g., used as label on a
                dashboard chart
        @param units: Units of the metric, e.g. percent, seconds, MB.
        @param higher_is_better: Whether a higher value is considered better or
                not.
        """
        self.values = []
        self.description = description
        self.units = units
        self.higher_is_better = higher_is_better

    def collect_metric(self):
        """
        Collects one metric.

        Implementations should add a metric value to the self.values list.
        """
        raise NotImplementedError('Subclasses should override')

class PeakMetric(Metric):
    """
    Metric that collects the peak of another metric.
    """
    def __init__(self, metric):
        """
        Initializes with a Metric.

        @param metric The Metric to get the peak from.
        """
        super(PeakMetric, self).__init__(
                'peak_' + metric.description,
                units = metric.units,
                higher_is_better = metric.higher_is_better)
        self.metric = metric

    def collect_metric(self):
        self.values = [max(self.metric.values)] if self.metric.values else []

class MemUsageMetric(Metric):
    """
    Metric that collects memory usage in percent.

    Memory usage is collected in percent. Buffers and cached are calculated
    as free memory.
    """
    def __init__(self, system_facade):
        super(MemUsageMetric, self).__init__('memory_usage', units='percent')
        self.system_facade = system_facade

    def collect_metric(self):
        total_memory = self.system_facade.get_mem_total()
        free_memory = self.system_facade.get_mem_free_plus_buffers_and_cached()
        used_memory = total_memory - free_memory
        usage_percent = (used_memory * 100) / total_memory
        self.values.append(usage_percent)

class CpuUsageMetric(Metric):
    """
    Metric that collects cpu usage in percent.
    """
    def __init__(self, system_facade):
        super(CpuUsageMetric, self).__init__('cpu_usage', units='percent')
        self.last_usage = None
        self.system_facade = system_facade

    def collect_metric(self):
        """
        Collects CPU usage in percent.

        Since the CPU active time we query is a cumulative metric, the first
        collection does not actually save a value. It saves the first value to
        be used for subsequent deltas.
        """
        current_usage = self.system_facade.get_cpu_usage()
        if self.last_usage is not None:
            # Compute the percent of active time since the last update to
            # current_usage.
            usage_percent = 100 * self.system_facade.compute_active_cpu_time(
                    self.last_usage, current_usage)
            self.values.append(usage_percent)
        self.last_usage = current_usage

class AllocatedFileHandlesMetric(Metric):
    """
    Metric that collects the number of allocated file handles.
    """
    def __init__(self, system_facade):
        super(AllocatedFileHandlesMetric, self).__init__(
                'allocated_file_handles', units='handles')
        self.system_facade = system_facade

    def collect_metric(self):
        self.values.append(self.system_facade.get_num_allocated_file_handles())

class TemperatureMetric(Metric):
    """
    Metric that collects the max of the temperatures measured on all sensors.
    """
    def __init__(self, system_facade):
        super(TemperatureMetric, self).__init__('temperature', units='Celsius')
        self.system_facade = system_facade

    def collect_metric(self):
        self.values.append(self.system_facade.get_current_temperature_max())

def create_default_metric_set(system_facade):
    """
    Creates the default set of metrics.

    @param system_facade the system facade to initialize the metrics with.
    @return a list with Metric instances.
    """
    cpu = CpuUsageMetric(system_facade)
    mem = MemUsageMetric(system_facade)
    file_handles = AllocatedFileHandlesMetric(system_facade)
    temperature = TemperatureMetric(system_facade)
    peak_cpu = PeakMetric(cpu)
    peak_mem = PeakMetric(mem)
    peak_temperature = PeakMetric(temperature)
    return [cpu,
            mem,
            file_handles,
            temperature,
            peak_cpu,
            peak_mem,
            peak_temperature]

class SystemMetricsCollector(object):
    """
    Collects system metrics.
    """
    def __init__(self, system_facade, metrics = None):
        """
        Initialize with facade and metric classes.

        @param system_facade The system facade to use for querying the system,
                e.g. system_facade_native.SystemFacadeNative for client tests.
        @param metrics List of metric instances. If None, the default set will
                be created.
        """
        self.metrics = (create_default_metric_set(system_facade)
                        if metrics is None else metrics)

    def collect_snapshot(self):
        """
        Collects one snapshot of metrics.
        """
        for metric in self.metrics:
            metric.collect_metric()

    def write_metrics(self, writer_function):
        """
        Writes the collected metrics using the specified writer function.

        @param writer_function: A function with the following signature:
                 f(description, value, units, higher_is_better)
        """
        for metric in self.metrics:
            writer_function(
                    description=metric.description,
                    value=metric.values,
                    units=metric.units,
                    higher_is_better=metric.higher_is_better)
