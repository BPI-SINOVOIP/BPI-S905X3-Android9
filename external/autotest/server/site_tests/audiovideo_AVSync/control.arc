# Copyright 2016 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from autotest_lib.server import utils

AUTHOR = "chromeos-chameleon"
NAME = 'audiovideo_AVSync.arc'
PURPOSE = "Remotely controlled HDMI audio/video test."
ATTRIBUTES = "suite:chameleon_audio_nightly, suite:chameleon_audio"
TIME = 'SHORT'
TEST_CATEGORY = 'Performance'
TEST_CLASS = 'audiovideo'
TEST_TYPE = 'server'
DEPENDENCIES = 'chameleon:hdmi, arc'
JOB_RETRIES = 2

DOC = """
This test measure the audio/video synchronization quality while playing a
1080p 30fps MP4 video (video: mpeg4, audio: aac) on ARC.
"""

args_dict = utils.args_to_dict(args)
chameleon_args = hosts.CrosHost.get_chameleon_arguments(args_dict)

VIDEO_URL = ('http://commondatastorage.googleapis.com/'
             'chromiumos-test-assets-public/chameleon/'
             'audiovideo_AVSync/1080p_30fps.mp4')

def run(machine):
    host = hosts.create_host(machine, chameleon_args=chameleon_args)
    job.run_test("audiovideo_AVSync", host=host, video_url=VIDEO_URL, arc=True)

parallel_simple(run, machines)
