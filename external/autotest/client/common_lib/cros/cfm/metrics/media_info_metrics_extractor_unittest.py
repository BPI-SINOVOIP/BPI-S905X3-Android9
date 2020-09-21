import unittest

from autotest_lib.client.common_lib.cros.cfm.metrics import (
      media_info_metrics_extractor)

MEDIA_TYPE = media_info_metrics_extractor.MediaType
DIRECTION = media_info_metrics_extractor.Direction


# pylint: disable=missing-docstring
class MediaInfoMetricsExtractorTest(unittest.TestCase):

    def setUp(self):
        self.extractor = media_info_metrics_extractor.MediaInfoMetricsExtractor(
                TEST_DATA_POINTS)

    def testGlobalMetric(self):
        metric = self.extractor.get_top_level_metric('processcpuusage')
        self.assertEqual(metric, [(1, [105]), (2, [95])])

    def testMediaMetric(self):
        metric = self.extractor.get_media_metric(
                'fps', media_type=MEDIA_TYPE.VIDEO,
                direction=DIRECTION.RECEIVER)
        self.assertEqual(metric, [(1, [8, 23]), (2, [25, 12])])

    def testPostProcessReturnsScalar(self):
        metric = self.extractor.get_media_metric(
                'fps',
                media_type=MEDIA_TYPE.VIDEO,
                direction=DIRECTION.RECEIVER,
                post_process_func=sum)
        self.assertEqual(metric, [(1, [31]), (2, [37])])

    def testPostProcessReturnsList(self):
        metric = self.extractor.get_media_metric(
                'fps',
                media_type=MEDIA_TYPE.VIDEO,
                direction=DIRECTION.RECEIVER,
                post_process_func=lambda values: [x + 1 for x in values])
        self.assertEqual(metric, [(1, [9, 24]), (2, [26, 13])])

    def testMetricNameDoesNotExist(self):
        self.assertRaises(
                KeyError,
                lambda: self.extractor.get_top_level_metric('does_not_exist'))

    def testWildcardMediaType(self):
        metric = self.extractor.get_media_metric(
                'bytessent', direction=DIRECTION.SENDER)
        self.assertEqual(
            metric, [(1.0, [58978, 3826779]), (2.0, [59206, 3986136])])

    def testNoneValueMediaMetricsSkipped(self):
        metric = self.extractor.get_media_metric(
                'fps',
                media_type=MEDIA_TYPE.VIDEO,
                direction=DIRECTION.BANDWIDTH_ESTIMATION)
        self.assertEquals(0, len(metric))

    def testNoneValueTopLevelMetricsSkipped(self):
        metric = self.extractor.get_top_level_metric('gpuProcessCpuUsage')
        self.assertEqual(metric, [(2.0, [0])])


# Two data points extracted from a real call. Somewhat post processed to make
# numbers easier to use in tests.
TEST_DATA_POINTS = [{
    u'gpuProcessCpuUsage':
        None,
    u'processcpuusage':
        105,
    u'timestamp': 1,
    u'systemcpuusage':
        615,
    u'media': [{
        u'leakybucketdelay': 0,
        u'availablerecvbitrate': 0,
        u'ssrc': None,
        u'availablesendbitrate': 2187820,
        u'direction': 2,
        u'height': None,
        u'fractionlost': -1,
        u'fpsnetwork': None,
        u'width': None,
        u'fps': None,
        u'mediatype': 2
    }, {
        u'bytessent': 58978,
        u'direction': 0,
        u'ssrc': 511990786,
        u'fractionlost': 0,
        u'transmissionbitrate': 1212,
        u'packetssent': 946,
        u'mediatype': 1
    }, {
        u'bytessent': 3826779,
        u'fps': 21,
        u'ssrc': 4134692703,
        u'direction': 0,
        u'height': 720,
        u'mediatype': 2,
        u'encodeUsagePercent': 19,
        u'fpsnetwork': 20,
        u'transmissionbitrate': 1246604,
        u'packetssent': 5166,
        u'fractionlost': 0,
        u'width': 1280,
        u'avgEncodeMs': 7
    }, {
        u'speechExpandRate': 0,
        u'fractionlost': 0,
        u'ssrc': 6666,
        u'packetsreceived': 1129,
        u'recvbitrate': 41523,
        u'direction': 1,
        u'bytesreceived': 111317,
        u'mediatype': 1
    }, {
        u'speechExpandRate': 0,
        u'fractionlost': 0,
        u'ssrc': 6667,
        u'packetsreceived': 1016,
        u'recvbitrate': 41866,
        u'direction': 1,
        u'bytesreceived': 100225,
        u'mediatype': 1
    }, {
        u'frameSpacingMaxMs': 524,
        u'fps': 8,
        u'ssrc': 1491110400,
        u'direction': 1,
        u'packetsreceived': 3475,
        u'recvbitrate': 449595,
        u'fractionlost': 0,
        u'height': 720,
        u'bytesreceived': 3863701,
        u'fpsnetwork': 8,
        u'width': 1280,
        u'mediatype': 2,
        u'fpsdecoded': 8
    }, {
        u'frameSpacingMaxMs': 363,
        u'fps': 23,
        u'ssrc': 2738775122,
        u'direction': 1,
        u'packetsreceived': 3419,
        u'recvbitrate': 2228961,
        u'fractionlost': 0,
        u'height': 180,
        u'bytesreceived': 3829959,
        u'fpsnetwork': 22,
        u'width': 320,
        u'mediatype': 2,
        u'fpsdecoded': 23
    }],
    u'browserProcessCpuUsage':
        46
}, {
    u'gpuProcessCpuUsage':
        0,
    u'processcpuusage':
        95,
    u'timestamp': 2,
    u'systemcpuusage':
        580,
    u'media': [{
        u'leakybucketdelay': 0,
        u'availablerecvbitrate': 0,
        u'ssrc': None,
        u'availablesendbitrate': 2187820,
        u'direction': 2,
        u'height': None,
        u'fractionlost': -1,
        u'fpsnetwork': None,
        u'width': None,
        u'fps': None,
        u'mediatype': 2
    }, {
        u'bytessent': 59206,
        u'direction': 0,
        u'ssrc': 511990786,
        u'fractionlost': 0,
        u'transmissionbitrate': 1820,
        u'packetssent': 952,
        u'mediatype': 1
    }, {
        u'bytessent': 3986136,
        u'fps': 21,
        u'ssrc': 4134692703,
        u'direction': 0,
        u'height': 720,
        u'mediatype': 2,
        u'encodeUsagePercent': 19,
        u'fpsnetwork': 20,
        u'transmissionbitrate': 1272311,
        u'packetssent': 5325,
        u'fractionlost': 0,
        u'width': 1280,
        u'avgEncodeMs': 8
    }, {
        u'speechExpandRate': 0,
        u'fractionlost': 0,
        u'ssrc': 6666,
        u'packetsreceived': 1147,
        u'recvbitrate': 8527,
        u'direction': 1,
        u'bytesreceived': 112385,
        u'mediatype': 1
    }, {
        u'speechExpandRate': 0,
        u'fractionlost': 0,
        u'ssrc': 6667,
        u'packetsreceived': 1062,
        u'recvbitrate': 35321,
        u'direction': 1,
        u'bytesreceived': 104649,
        u'mediatype': 1
    }, {
        u'frameSpacingMaxMs': 330,
        u'fps': 25,
        u'ssrc': 1491110400,
        u'direction': 1,
        u'packetsreceived': 3694,
        u'recvbitrate': 1963721,
        u'fractionlost': 0,
        u'height': 720,
        u'bytesreceived': 4109657,
        u'fpsnetwork': 26,
        u'width': 1280,
        u'mediatype': 2,
        u'fpsdecoded': 25
    }, {
        u'frameSpacingMaxMs': 363,
        u'fps': 12,
        u'ssrc': 2738775122,
        u'direction': 1,
        u'packetsreceived': 3440,
        u'recvbitrate': 147018,
        u'fractionlost': 0,
        u'height': 180,
        u'bytesreceived': 3848373,
        u'fpsnetwork': 13,
        u'width': 320,
        u'mediatype': 2,
        u'fpsdecoded': 12
    }],
    u'browserProcessCpuUsage':
        38
}]


