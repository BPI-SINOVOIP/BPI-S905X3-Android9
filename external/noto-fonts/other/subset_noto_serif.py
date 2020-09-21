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

"""Create a curated subset of Noto Serif for Android."""

import sys

from nototools import subset
from nototools import unicode_data

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

def main(argv):
    """Subset a Noto Serif font.

    The first argument is the source file name, and the second argument is
    the target file name.
    """

    source_file_name = argv[1]
    target_file_name = argv[2]
    subset.subset_font(
        source_file_name,
        target_file_name,
        exclude=ANDROID_EMOJI)


if __name__ == '__main__':
    main(sys.argv)
