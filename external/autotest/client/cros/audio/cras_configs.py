# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""This module provides cras audio configs."""

INTERNAL_MIC_GAIN_100DB = {
        'chell': 500,
        'auron_yuna': -1000,
        'kevin': 0,
}

def get_proper_internal_mic_gain(board):
    """Return a proper internal mic gain.

    @param board: Board name.

    @returns: A number in 100 dB. E.g., 1000 is 10dB. This is in the same unit
              as cras_utils set_capture_gain. Returns None if there is no such
              entry.
    """
    return INTERNAL_MIC_GAIN_100DB.get(board, None)
