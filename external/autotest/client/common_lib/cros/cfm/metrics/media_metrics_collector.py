from __future__ import division

from autotest_lib.client.common_lib.cros.cfm.metrics import (
        media_info_metrics_extractor)

MEDIA_TYPE = media_info_metrics_extractor.MediaType
DIRECTION = media_info_metrics_extractor.Direction

# Delta used to determine if floating point values are equal.
FLOATING_POINT_COMPARISON_DELTA = 0.00001

def _avg(l):
    """
    Returns the average of the list or 0 if the list is empty.
    """
    return sum(l)/len(l) if l else 0


def _max(l):
    """
    Returns the max of the list or 0 if the list is empty.
    """
    return max(l) if l else 0


def _get_number_of_incoming_active_video_streams(extractor):
    """
    Calculates the number of incoming video streams.

    @param extractor media_info_metrics_extractor.MediaInfoMetricsExtractor.

    @returns List with (timestamp, number of incoming video streams) tuples.
    """
    # Get metrics of a kind we know exists and count the nuber of values
    # for each data point.
    fps_metrics = extractor.get_media_metric(
            'fps', direction=DIRECTION.RECEIVER, media_type=MEDIA_TYPE.VIDEO)
    return [(x, [len(y)]) for x, y in fps_metrics]


# Sum for all received streams per data point.
BROWSER_CPU_PERCENT_OF_TOTAL = 'browser_cpu_percent'
CPU_PERCENT_OF_TOTAL = 'cpu_percent'
FRAMERATE_CAPTURED = 'framerate_catured'
# Mean for all received streams per data point.
FRAMERATE_DECODED = 'framerate_decoded'
# Max for all received streams per data point.
FRAMERATE_DECODED_MAX = 'framerate_decoded_max'
# Mean for all received streams per data point.
FRAMERATE_RECEIVED = 'framerate_received'
# Max for all received streams per data point.
FRAMERATE_RECEIVED_MAX = 'framerate_received_max'
FRAMERATE_SENT = 'framerate_sent'
GPU_PERCENT_OF_TOTAL = 'gpu_cpu_percent'
LEAKY_BUCKET_DELAY = 'leaky_bucket_delay'
NUMBER_OF_ACTIVE_INCOMING_VIDEO_STREAMS = 'num_active_vid_in_streams'
RENDERER_CPU_PERCENT_OF_TOTAL = 'renderer_cpu_percent'
SPEECH_EXPAND_RATE = 'speech_expand_rate'
VIDEO_RECEIVED_FRAME_HEIGHT = 'video_received_frame_height'
VIDEO_RECEIVED_FRAME_HEIGHT_MAX = 'video_received_frame_height_max'
VIDEO_RECEIVED_FRAME_WIDTH = 'video_received_frame_width'
VIDEO_RECEIVED_FRAME_WIDTH_MAX = 'video_received_frame_width_max'
VIDEO_SENT_FRAME_HEIGHT = 'video_sent_frame_height'
VIDEO_SENT_FRAME_WIDTH = 'video_sent_frame_width'


# Mapping between metric names and how to extract the named metric using the
# MediaInfoMetricsExtractor.
METRIC_NAME_TO_EXTRACTOR_FUNC_DICT = {
    BROWSER_CPU_PERCENT_OF_TOTAL:
      lambda x: x.get_top_level_metric('browserProcessCpuUsage'),
    CPU_PERCENT_OF_TOTAL:
        lambda x: x.get_top_level_metric('systemcpuusage'),
    FRAMERATE_CAPTURED:
        lambda x: x.get_media_metric('fps',
                                   direction=DIRECTION.SENDER,
                                   media_type=MEDIA_TYPE.VIDEO),
    FRAMERATE_DECODED:
        lambda x: x.get_media_metric('fpsdecoded',
                                   direction=DIRECTION.RECEIVER,
                                   media_type=MEDIA_TYPE.VIDEO,
                                   post_process_func=_avg),
    FRAMERATE_DECODED_MAX:
        lambda x: x.get_media_metric('fpsdecoded',
                                   direction=DIRECTION.RECEIVER,
                                   media_type=MEDIA_TYPE.VIDEO,
                                   post_process_func=_max),
    FRAMERATE_RECEIVED:
        lambda x: x.get_media_metric('fpsnetwork',
                                   direction=DIRECTION.RECEIVER,
                                   media_type=MEDIA_TYPE.VIDEO,
                                   post_process_func=_avg),
    FRAMERATE_RECEIVED_MAX:
        lambda x: x.get_media_metric('fpsnetwork',
                                   direction=DIRECTION.RECEIVER,
                                   media_type=MEDIA_TYPE.VIDEO,
                                   post_process_func=_max),
    FRAMERATE_SENT:
        lambda x: x.get_media_metric('fpsnetwork',
                                   direction=DIRECTION.SENDER,
                                   media_type=MEDIA_TYPE.VIDEO),
    GPU_PERCENT_OF_TOTAL:
        lambda x: x.get_top_level_metric('gpuProcessCpuUsage'),
    LEAKY_BUCKET_DELAY:
        lambda x: x.get_media_metric('leakybucketdelay',
                                   direction=DIRECTION.BANDWIDTH_ESTIMATION),
    NUMBER_OF_ACTIVE_INCOMING_VIDEO_STREAMS:
        _get_number_of_incoming_active_video_streams,
    SPEECH_EXPAND_RATE:
        lambda x: x.get_media_metric('speechExpandRate',
                                   direction=DIRECTION.RECEIVER,
                                   media_type=MEDIA_TYPE.AUDIO,
                                   post_process_func=_avg),
    RENDERER_CPU_PERCENT_OF_TOTAL:
        lambda x: x.get_top_level_metric('processcpuusage'),
    VIDEO_RECEIVED_FRAME_WIDTH:
        lambda x: x.get_media_metric('width',
                                   direction=DIRECTION.RECEIVER,
                                   media_type=MEDIA_TYPE.VIDEO,
                                   post_process_func=_avg),
    VIDEO_RECEIVED_FRAME_WIDTH_MAX:
        lambda x: x.get_media_metric('width',
                                   direction=DIRECTION.RECEIVER,
                                   media_type=MEDIA_TYPE.VIDEO,
                                   post_process_func=_max),
    VIDEO_RECEIVED_FRAME_HEIGHT:
        lambda x: x.get_media_metric('height',
                                   direction=DIRECTION.RECEIVER,
                                   media_type=MEDIA_TYPE.VIDEO,
                                   post_process_func=_avg),
    VIDEO_RECEIVED_FRAME_HEIGHT_MAX:
        lambda x: x.get_media_metric('height',
                                   direction=DIRECTION.RECEIVER,
                                   media_type=MEDIA_TYPE.VIDEO,
                                   post_process_func=_max),
    VIDEO_SENT_FRAME_HEIGHT:
        lambda x: x.get_media_metric('height',
                                   direction=DIRECTION.SENDER,
                                   media_type=MEDIA_TYPE.VIDEO),
    VIDEO_SENT_FRAME_WIDTH:
        lambda x: x.get_media_metric('width',
                                   direction=DIRECTION.SENDER,
                                   media_type=MEDIA_TYPE.VIDEO),
}

class MetricsCollector(object):
    """Collects metrics for a test run."""

    def __init__(self, data_point_collector):
        """
        Initializes.

        @param data_point_collector
        """
        self._data_point_collector = data_point_collector
        self._extractor = None

    def collect_snapshot(self):
        """
        Stores new merics since the last call.

        Metrics can then be retrieved by calling get_metric.
        """
        self._data_point_collector.collect_snapshot()
        data_points = self._data_point_collector.get_data_points()
        # Replace the extractor on each snapshot to always support
        # getting metrics
        self._extractor = (media_info_metrics_extractor
                           .MediaInfoMetricsExtractor(data_points))

    def get_metric(self, name):
        """
        Gets the metric with the specified name.

        @param name The name of the metric
        @returns a list with (timestamp, value_list) tuples
        """
        if self._extractor is None:
            raise RuntimeError(
                    'collect_snapshot() must be called at least once.')
        return METRIC_NAME_TO_EXTRACTOR_FUNC_DICT[name](
                self._extractor)


class DataPointCollector(object):
    """Collects data points."""

    def __init__(self, cfm_facade):
        """
        Initializes.

        @param cfm_facade The cfm facade to use for getting data points.
        """
        self._cfm_facade = cfm_facade
        self._data_points = []

    def collect_snapshot(self):
        """
        Collects any new datapoints since the last collection.
        """
        data_points = self._cfm_facade.get_media_info_data_points()
        last_timestamp = (self._data_points[-1]['timestamp']
                          if self._data_points else 0)
        for data_point in data_points:
            if (data_point['timestamp'] >
                    last_timestamp + FLOATING_POINT_COMPARISON_DELTA):
                self._data_points.append(data_point)

    def get_data_points(self):
        """
        Gets all collected data points.
        """
        return self._data_points

