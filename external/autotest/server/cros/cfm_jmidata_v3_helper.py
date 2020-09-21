# Copyright (c) 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import json

from autotest_lib.server.cros import cfm_jmidata_helper_base

# Start index in the JSON object that contains Audio/Video streams related info.
AV_INDEX = 4

SSRC = u'ssrc'
GLOBAL = u'global'

AUDIO_INPUT = u'audioInputLevel'
AUDIO_OUTPUT = u'audioOutputLevel'
BYTES_RECEIVED = u'bytesReceived'
BYTES_SENT = u'bytesSent'
ADAPTATION_CHANGES = u'googAdaptationChanges'
AVERAGE_ENCODE_TIME = u'googAvgEncodeMs'
BANDWIDTH_LIMITED_RESOLUTION = u'googBandwidthLimitedResolution'
CPU_LIMITED_RESOLUTION = u'googCpuLimitedResolution'
VIDEO_ENCODE_CPU_USAGE = u'googEncodeUsagePercent'
VIDEO_RECEIVED_FRAME_HEIGHT = u'googFrameHeightReceived'
VIDEO_RECEIVED_FRAME_WIDTH = u'googFrameWidthReceived'
FRAMERATE_OUTGOING = u'googFrameRateInput'
FRAMERATE_RECEIVED = u'googFrameRateReceived'
FRAMERATE_SENT = u'googFrameRateSent'
FRAMERATE_DECODED = u'googFrameRateDecoded'
FRAMERATE_OUTPUT = u'googFrameRateOutput'
FRAME_WIDTH_SENT = u'googFrameWidthSent'
FRAME_HEIGHT_SENT = u'googFrameHeightSent'
FRAMES_DECODED = u'framesDecoded'
FRAMES_ENCODED = u'framesEncoded'
VIDEO_PACKETS_LOST = u'packetsLost'
VIDEO_PACKETS_SENT = u'packetsSent'

BROWSER_CPU = u'browserCpuUsage'
GPU_CPU = u'gpuCpuUsage'
NUM_PROCESSORS = u'numOfProcessors'
NACL_EFFECTS_CPU = u'pluginCpuUsage'
RENDERER_CPU = u'tabCpuUsage'
TOTAL_CPU = u'totalCpuUsage'


class JMIDataV3Helper(cfm_jmidata_helper_base.JMIDataHelperBase):
    """Helper class for JMI data v3 parsing. This class helps in extracting
    relevant JMI data from javascript log.

    The class takes javascript file as input and extracts jmidata elements from
    the file that is internally stored as a list. Whenever we need to extract
    data i.e. audio received bytes we call a method such as
    getAudioReceivedDataList. This method converts each string element in the
    internal list to a json object and retrieves the relevant info from it which
    is then stored and returned as a list.
    """

    def __init__(self, log_file_content):
        super(JMIDataV3Helper, self).__init__(log_file_content, 'jmidatav3')

    def _ExtractAllJMIDataPointsWithKey(self, jmi_type, is_audio, key):
        """Extracts all values from the data points with the given key."""
        data_list = []
        for jmi_data_point in self._jmi_list:
            json_arr = json.loads(jmi_data_point)
            for i in range(AV_INDEX, len(json_arr)):
                if json_arr[i] and jmi_type in json_arr[i]:
                    jmi_obj = json_arr[i][jmi_type]
                    this_is_audio = (AUDIO_INPUT in jmi_obj or
                                     AUDIO_OUTPUT in jmi_obj)
                    if this_is_audio == is_audio and key in jmi_obj:
                        if (not isinstance(jmi_obj[key], int) and
                                (jmi_obj[key] == 'false' or
                                 jmi_obj[key] == 'true')):
                            jmi_obj[key] = 1 if jmi_obj[key] == 'true' else 0
                        data_list.append(jmi_obj[key])
        if not data_list:
            data_list = [0]
        return data_list

    def GetAudioReceivedBytesList(self):
        return self._ExtractAllJMIDataPointsWithKey(
                jmi_type=SSRC, is_audio=True, key=BYTES_RECEIVED)

    def GetAudioSentBytesList(self):
        return self._ExtractAllJMIDataPointsWithKey(
                jmi_type=SSRC, is_audio=True, key=BYTES_SENT)

    def GetAudioReceivedEnergyList(self):
        return self._ExtractAllJMIDataPointsWithKey(
                jmi_type=SSRC, is_audio=True, key=AUDIO_OUTPUT)

    def GetAudioSentEnergyList(self):
        return self._ExtractAllJMIDataPointsWithKey(
                jmi_type=SSRC, is_audio=True, key=AUDIO_INPUT)

    def GetVideoSentBytesList(self):
        return self._ExtractAllJMIDataPointsWithKey(
                jmi_type=SSRC, is_audio=False, key=BYTES_SENT)

    def GetVideoReceivedBytesList(self):
        return self._ExtractAllJMIDataPointsWithKey(
                jmi_type=SSRC, is_audio=False, key=BYTES_RECEIVED)

    def GetVideoIncomingFramerateReceivedList(self):
        return self._ExtractAllJMIDataPointsWithKey(
                jmi_type=SSRC, is_audio=False, key=FRAMERATE_RECEIVED)

    def GetVideoOutgoingFramerateSentList(self):
        return self._ExtractAllJMIDataPointsWithKey(
                jmi_type=SSRC, is_audio=False, key=FRAMERATE_SENT)

    def GetVideoIncomingFramerateDecodedList(self):
        return self._ExtractAllJMIDataPointsWithKey(
                jmi_type=SSRC, is_audio=False, key=FRAMERATE_DECODED)

    def GetVideoIncomingFramerateList(self):
        return self._ExtractAllJMIDataPointsWithKey(
                jmi_type=SSRC, is_audio=False, key=FRAMERATE_OUTPUT)

    def GetVideoSentFrameWidthList(self):
        return self._ExtractAllJMIDataPointsWithKey(
                jmi_type=SSRC, is_audio=False, key=FRAME_WIDTH_SENT)

    def GetVideoSentFrameHeightList(self):
        return self._ExtractAllJMIDataPointsWithKey(
                jmi_type=SSRC, is_audio=False, key=FRAME_HEIGHT_SENT)

    def GetCPULimitedResolutionList(self):
        return self._ExtractAllJMIDataPointsWithKey(
                jmi_type=SSRC, is_audio=False, key=CPU_LIMITED_RESOLUTION)

    def GetVideoPacketsSentList(self):
        return self._ExtractAllJMIDataPointsWithKey(
                jmi_type=SSRC, is_audio=False, key=VIDEO_PACKETS_SENT)

    def GetVideoPacketsLostList(self):
        return self._ExtractAllJMIDataPointsWithKey(
                jmi_type=SSRC, is_audio=False, key=VIDEO_PACKETS_LOST)

    def GetVideoIncomingFramesDecodedList(self):
        return self._ExtractAllJMIDataPointsWithKey(
                jmi_type=SSRC, is_audio=False, key=FRAMES_DECODED)

    def GetVideoOutgoingFramesEncodedList(self):
        return self._ExtractAllJMIDataPointsWithKey(
                jmi_type=SSRC, is_audio=False, key=FRAMES_ENCODED)

    def GetVideoAdaptationChangeList(self):
        return self._ExtractAllJMIDataPointsWithKey(
                jmi_type=SSRC, is_audio=False, key=ADAPTATION_CHANGES)

    def GetVideoEncodeTimeList(self):
        return self._ExtractAllJMIDataPointsWithKey(
                jmi_type=SSRC, is_audio=False, key=AVERAGE_ENCODE_TIME)

    def GetBandwidthLimitedResolutionList(self):
        return self._ExtractAllJMIDataPointsWithKey(
                jmi_type=SSRC, is_audio=False, key=BANDWIDTH_LIMITED_RESOLUTION)

    def GetVideoReceivedFrameHeightList(self):
        return self._ExtractAllJMIDataPointsWithKey(
                jmi_type=SSRC, is_audio=False, key=VIDEO_RECEIVED_FRAME_HEIGHT)

    def GetVideoOutgoingFramerateInputList(self):
        return self._ExtractAllJMIDataPointsWithKey(
                jmi_type=SSRC, is_audio=False, key=FRAMERATE_OUTGOING)

    def GetVideoReceivedFrameWidthList(self):
        return self._ExtractAllJMIDataPointsWithKey(
                jmi_type=SSRC, is_audio=False, key=VIDEO_RECEIVED_FRAME_WIDTH)

    def GetVideoEncodeCpuUsagePercentList(self):
        return self._ExtractAllJMIDataPointsWithKey(
                jmi_type=SSRC, is_audio=False, key=VIDEO_ENCODE_CPU_USAGE)

    def GetNumberOfActiveIncomingVideoStreams(self):
        """Retrieve number of active incoming video streams."""
        if not self._jmi_list:
            # JMI data hasn't started populating yet.
            return 0

        num_video_streams = []

        # If JMI data has started getting populated and has video stream data.
        for jmi_data_point in self._jmi_list:
            json_arr = json.loads(jmi_data_point)
            video_streams = 0
            for i in range(AV_INDEX, len(json_arr)):
                if json_arr[i] and SSRC in json_arr[i]:
                    ssrc_obj = json_arr[i][SSRC]
                    is_audio = ('audioInputLevel' in ssrc_obj or
                            'audioOutputLevel' in ssrc_obj)
                    is_incoming = 'bytesReceived' in ssrc_obj
                    frame_rate_received = 'googFrameRateReceived' in ssrc_obj
                    if ssrc_obj['mediaType'] == 'video' and \
                            frame_rate_received:
                        frame_rate = ssrc_obj['googFrameRateReceived']
                        if (is_incoming and not is_audio) and \
                                frame_rate != 0:
                            video_streams += 1
            num_video_streams.append(video_streams)
        if not num_video_streams:
            num_video_streams = [0]
        return num_video_streams

    def GetCpuUsageList(self, cpu_type):
        """Retrieves cpu usage data from JMI data.

        @param cpu_type: Cpu usage type.
        @returns List containing CPU usage data.
        """
        data_list = []
        for jmi_data_point in self._jmi_list:
            json_arr = json.loads(jmi_data_point)
            for i in range(AV_INDEX, len(json_arr)):
                if json_arr[i] and GLOBAL in json_arr[i]:
                    global_obj = json_arr[i][GLOBAL]
                    # Some values in JMIDataV3 are set to 'null'.
                    if cpu_type == u'numOfProcessors':
                        return global_obj[cpu_type]
                    elif (cpu_type in global_obj and
                            self.IsFloat(global_obj[cpu_type])):
                        data_list.append(float(global_obj[cpu_type]))
        if not data_list:
            data_list = [0]
        return data_list

    def GetNumOfProcessors(self):
        return self.GetCpuUsageList(NUM_PROCESSORS)

    def GetTotalCpuPercentage(self):
        return self.GetCpuUsageList(TOTAL_CPU)

    def GetBrowserCpuPercentage(self):
        return self.GetCpuUsageList(BROWSER_CPU)

    def GetGpuCpuPercentage(self):
        return self.GetCpuUsageList(GPU_CPU)

    def GetNaclEffectsCpuPercentage(self):
        return self.GetCpuUsageList(NACL_EFFECTS_CPU)

    def GetRendererCpuPercentage(self):
        return self.GetCpuUsageList(RENDERER_CPU)

    def IsFloat(self, value):
        try:
            float(value)
            return True
        except TypeError:
            return False
