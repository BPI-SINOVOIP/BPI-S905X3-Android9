# Copyright 2016 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

AUTHOR = 'The Chromium OS Authors,chromeos-audio@google.com'
NAME = 'audio_AlsaAPI.move'
ATTRIBUTES = "suite:audio"
PURPOSE = 'Test that simple ALSA API succeeds to move appl_ptr.'
CRITERIA = """
Check that the ALSA API succeeds.
"""
TIME='SHORT'
TEST_CATEGORY = 'Functional'
TEST_CLASS = "audio"
TEST_TYPE = 'client'

DOC = """
Check ALSA API succeeds to move appl_ptr.
"""

job.run_test('audio_AlsaAPI', to_test='move', tag='move')
