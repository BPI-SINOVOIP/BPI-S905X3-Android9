# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

SUPPORTED_BOARDS = ['lumpy', 'daisy', 'falco', 'link', 'parrot', 'peppy',
                    'peach_pi', 'peach_pit', 'auron_paine', 'squawks', 'cyan'
                    'veyron_jerry', 'chell']

DESIRED_WIDTH = 864
DESIRED_HEIGHT = 494

TEST_DIR = '/tmp/test'
GOLDEN_CHECKSUMS_FILENAME = 'golden_checksums.txt'
GOLDEN_CHECKSUM_REMOTE_BASE_DIR = (
    'https://storage.googleapis.com/chromiumos-test-assets-public'
    '/golden_images_video_image_comparison')

IMAGE_FORMAT = 'png'
FCOUNT = 330
MAX_FRAME_REPEAT_COUNT = 5
MAX_DIFF_TOTAL_FCOUNT = 10
MAX_NONMATCHING_FCOUNT = 10
NUM_CAPTURE_TRIES = 10

#Hardware decoding constants
MEDIA_GVD_INIT_STATUS = 'Media.GpuVideoDecoderInitializeStatus'
MEDIA_GVD_ERROR = 'Media.GpuVideoDecoderError'
RTC_INIT_HISTOGRAM = 'Media.RTCVideoDecoderInitDecodeSuccess'
MEDIA_RECORDER_VEA_USED_HISTOGRAM = 'Media.MediaRecorder.VEAUsed'
MEDIA_GVD_BUCKET = 0
RTC_VIDEO_INIT_BUCKET = 1
MEDIA_CAVDA_INIT_STATUS = (
    'Media.GpuArcVideoDecodeAccelerator.InitializeResult')
MEDIA_CAVDA_BUCKET = 0
MEDIA_RECORDER_VEA_USED_BUCKET = 1
MEDIA_RECORDER_VEA_NOT_USED_BUCKET = 0

#Path for video HTML file which helps for video operations
CROS_VIDEO_DIR = '/usr/local/autotest/cros/video'
VIDEO_HTML_FILEPATH = CROS_VIDEO_DIR + '/video.html'

#Playback duration to check video can play
PLAYBACK_TEST_TIME_S = 10
