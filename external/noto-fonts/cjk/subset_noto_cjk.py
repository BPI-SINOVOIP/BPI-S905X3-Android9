#!/usr/bin/python
# coding=UTF-8
#
# Copyright 2016 Google Inc. All rights reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

"""Create a curated subset of Noto CJK for Android."""

import os

from fontTools import ttLib
from nototools import font_data
from nototools import tool_utils
from nototools import ttc_utils

# Characters supported in Noto CJK fonts that UTR #51 recommends default to
# emoji-style.
EMOJI_IN_CJK = {
    0x26BD, # ⚽ SOCCER BALL
    0x26BE, # ⚾ BASEBALL
    0x1F18E, # 🆎 NEGATIVE SQUARED AB
    0x1F191, # 🆑 SQUARED CL
    0x1F192, # 🆒 SQUARED COOL
    0x1F193, # 🆓 SQUARED FREE
    0x1F194, # 🆔 SQUARED ID
    0x1F195, # 🆕 SQUARED NEW
    0x1F196, # 🆖 SQUARED NG
    0x1F197, # 🆗 SQUARED OK
    0x1F198, # 🆘 SQUARED SOS
    0x1F199, # 🆙 SQUARED UP WITH EXCLAMATION MARK
    0x1F19A, # 🆚 SQUARED VS
    0x1F201, # 🈁 SQUARED KATAKANA KOKO
    0x1F21A, # 🈚 SQUARED CJK UNIFIED IDEOGRAPH-7121
    0x1F22F, # 🈯 SQUARED CJK UNIFIED IDEOGRAPH-6307
    0x1F232, # 🈲 SQUARED CJK UNIFIED IDEOGRAPH-7981
    0x1F233, # 🈳 SQUARED CJK UNIFIED IDEOGRAPH-7A7A
    0x1F234, # 🈴 SQUARED CJK UNIFIED IDEOGRAPH-5408
    0x1F235, # 🈵 SQUARED CJK UNIFIED IDEOGRAPH-6E80
    0x1F236, # 🈶 SQUARED CJK UNIFIED IDEOGRAPH-6709
    0x1F238, # 🈸 SQUARED CJK UNIFIED IDEOGRAPH-7533
    0x1F239, # 🈹 SQUARED CJK UNIFIED IDEOGRAPH-5272
    0x1F23A, # 🈺 SQUARED CJK UNIFIED IDEOGRAPH-55B6
    0x1F250, # 🉐 CIRCLED IDEOGRAPH ADVANTAGE
    0x1F251, # 🉑 CIRCLED IDEOGRAPH ACCEPT
}

# Characters we have decided we are doing as emoji-style in Android,
# despite UTR #51's recommendation
ANDROID_EMOJI = {
    0x2600, # ☀ BLACK SUN WITH RAYS
    0x2601, # ☁ CLOUD
    0X260E, # ☎ BLACK TELEPHONE
    0x261D, # ☝ WHITE UP POINTING INDEX
    0x263A, # ☺ WHITE SMILING FACE
    0x2660, # ♠ BLACK SPADE SUIT
    0x2663, # ♣ BLACK CLUB SUIT
    0x2665, # ♥ BLACK HEART SUIT
    0x2666, # ♦ BLACK DIAMOND SUIT
    0x270C, # ✌ VICTORY HAND
    0x2744, # ❄ SNOWFLAKE
    0x2764, # ❤ HEAVY BLACK HEART
}

# We don't want support for ASCII control chars.
CONTROL_CHARS = tool_utils.parse_int_ranges('0000-001F');

EXCLUDED_CODEPOINTS = sorted(EMOJI_IN_CJK | ANDROID_EMOJI | CONTROL_CHARS)


def remove_from_cmap(infile, outfile, exclude=frozenset()):
    """Removes a set of characters from a font file's cmap table."""
    font = ttLib.TTFont(infile)
    font_data.delete_from_cmap(font, exclude)
    font.save(outfile)


TEMP_DIR = 'subsetted'

def remove_codepoints_from_ttc(ttc_name):
    otf_names = ttc_utils.ttcfile_extract(ttc_name, TEMP_DIR)

    with tool_utils.temp_chdir(TEMP_DIR):
        for index, otf_name in enumerate(otf_names):
            print 'Subsetting %s...' % otf_name
            remove_from_cmap(otf_name, otf_name, exclude=EXCLUDED_CODEPOINTS)
        ttc_utils.ttcfile_build(ttc_name, otf_names)
        for f in otf_names:
            os.remove(f)


remove_codepoints_from_ttc('NotoSansCJK-Regular.ttc')
remove_codepoints_from_ttc('NotoSerifCJK-Regular.ttc')
