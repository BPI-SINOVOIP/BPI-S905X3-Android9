# Copyright (c) 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging

from autotest_lib.server.cros import cfm_jmidata_v3_helper


def GetDataFromLogs(testcase, data_type, log_lines):
    """Returns a list of data_type from JMI data in javascript log.

    @param testcase: Testcase instance.
    @param data_type: Type of data to be retrieved (audio sent/audio received
            etc. See above for complete list.).
    @param log_lines: log_file to be parsed.

    @return A list of data_type from javascript log. If the data_type is not
        supported for the given type of data (i.e. the underlying helper raises
        NotImplementedError), an empty list is returned.
    """
    helper = cfm_jmidata_v3_helper.JMIDataV3Helper(log_lines)
    data_type_to_func_map = {
        'video_sent_bytes': helper.GetVideoSentBytesList,
        'video_received_bytes': helper.GetVideoReceivedBytesList,
        'audio_sent_bytes': helper.GetAudioSentBytesList,
        'audio_received_bytes': helper.GetAudioReceivedBytesList,
        'audio_received_energy_level': helper.GetAudioReceivedEnergyList,
        'audio_sent_energy_level': helper.GetAudioSentEnergyList,
        'framerate_received': helper.GetVideoIncomingFramerateReceivedList,
        'framerate_sent': helper.GetVideoOutgoingFramerateSentList,
        'framerate_decoded': helper.GetVideoIncomingFramerateDecodedList,
        'frames_decoded': helper.GetVideoIncomingFramesDecodedList,
        'frames_encoded': helper.GetVideoOutgoingFramesEncodedList,
        'average_encode_time': helper.GetVideoEncodeTimeList,
        'framerate_to_renderer': helper.GetVideoIncomingFramerateList,
        'framerate_outgoing': helper.GetVideoOutgoingFramerateInputList,
        'video_sent_frame_width': helper.GetVideoSentFrameWidthList,
        'video_received_frame_width': helper.GetVideoReceivedFrameWidthList,
        'video_sent_frame_height': helper.GetVideoSentFrameHeightList,
        'video_received_frame_height': helper.GetVideoReceivedFrameHeightList,
        'cpu_adaptation': helper.GetCPULimitedResolutionList,
        'bandwidth_adaptation': helper.GetBandwidthLimitedResolutionList,
        'adaptation_changes': helper.GetVideoAdaptationChangeList,
        'video_packets_sent': helper.GetVideoPacketsSentList,
        'video_packets_lost': helper.GetVideoPacketsLostList,
        'video_encode_cpu_usage': helper.GetVideoEncodeCpuUsagePercentList,
        'num_active_vid_in_streams':
                helper.GetNumberOfActiveIncomingVideoStreams,
        'cpu_processors': helper.GetNumOfProcessors,
        'cpu_percent': helper.GetTotalCpuPercentage,
        'browser_cpu_percent': helper.GetBrowserCpuPercentage,
        'gpu_cpu_percent': helper.GetGpuCpuPercentage,
        'nacl_effects_cpu_percent': helper.GetNaclEffectsCpuPercentage,
        'renderer_cpu_percent': helper.GetRendererCpuPercentage,
    }

    try:
        data_array = data_type_to_func_map[data_type]()
    except NotImplementedError as e:
        logging.warning('data_type "%s" is not implemented in helper %s, '
                        'returning empty data set',
                        data_type,
                        helper,
                        exc_info = True)
        data_array = [0]
    logging.info('Data Type: %s, Data Array: %s', data_type, str(data_array))
    # Ensure we always return at least one element, or perf uploads will be
    # sad.
    return data_array or [0]

