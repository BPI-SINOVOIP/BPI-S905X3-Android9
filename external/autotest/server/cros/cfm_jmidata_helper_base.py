# Copyright (c) 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import re

from autotest_lib.client.common_lib import error


class JMIDataHelperBase(object):
    """This is a base class for JMIDataV3Helper.

    It helps in extracting relevant JMI data from javascript log file.
    """

    def __init__(self, log_file_content, jmidata_str):
        if not log_file_content:
            raise error.TestNAError('Logfile is empty.')
        self._log_file = log_file_content
        self._ExtractJMIDataFromLogFile(jmidata_str)

    def _ExtractJMIDataFromLogFile(self, jmidata_str):
        jmi_data_from_log_reg = r'(\[\s*"%s"\s*,.*\])' % jmidata_str
        self._jmi_list = re.findall(jmi_data_from_log_reg, self._log_file)
        if len(self._jmi_list) <= 0:
            raise error.TestNAError("Logfile doesn't contain any jmi data.")

    def GetAudioReceivedBytesList(self):
        raise NotImplementedError

    def GetAudioSentBytesList(self):
        raise NotImplementedError

    def GetAudioReceivedEnergyList(self):
        raise NotImplementedError

    def GetAudioSentEnergyList(self):
        raise NotImplementedError

    def GetVideoSentBytesList(self):
        raise NotImplementedError

    def GetVideoReceivedBytesList(self):
        raise NotImplementedError

    def GetVideoIncomingFramerateReceivedList(self):
        raise NotImplementedError

    def GetVideoOutgoingFramerateSentList(self):
        raise NotImplementedError

    def GetVideoIncomingFramerateDecodedList(self):
        raise NotImplementedError

    def GetVideoIncomingFramerateList(self):
        raise NotImplementedError

    def GetVideoIncomingFramerateListForAudioOnlyUser(self):
        raise NotImplementedError

    def GetVideoSentFrameWidthList(self):
        raise NotImplementedError

    def GetVideoSentFrameHeightList(self):
        raise NotImplementedError

    def GetCPULimitedResolutionList(self):
        raise NotImplementedError

    def GetVideoPacketsSentList(self):
        raise NotImplementedError

    def GetVideoPacketsLostList(self):
        raise NotImplementedError

    def GetVideoIncomingFramesDecodedList(self):
        raise NotImplementedError

    def GetVideoOutgoingFramesEncodedList(self):
        raise NotImplementedError

    def GetVideoAdaptationChangeList(self):
        raise NotImplementedError

    def GetVideoEncodeTimeList(self):
        raise NotImplementedError

    def GetBandwidthLimitedResolutionList(self):
        raise NotImplementedError

    def GetVideoReceivedFrameHeightList(self):
        raise NotImplementedError

    def GetVideoOutgoingFramerateInputList(self):
        raise NotImplementedError

    def GetVideoReceivedFrameWidthList(self):
        raise NotImplementedError

    def GetVideoEncodeCpuUsagePercentList(self):
        raise NotImplementedError

    def GetNumberOfActiveIncomingVideoStreams(self):
        raise NotImplementedError

    def GetCpuUsageList(self, cpu_type):
        raise NotImplementedError

    def GetNumOfProcessors(self):
        raise NotImplementedError

    def GetTotalCpuPercentage(self):
        raise NotImplementedError

    def GetBrowserCpuPercentage(self):
        raise NotImplementedError

    def GetGpuCpuPercentage(self):
        raise NotImplementedError

    def GetNaclEffectsCpuPercentage(self):
        raise NotImplementedError

    def GetRendererCpuPercentage(self):
        raise NotImplementedError
