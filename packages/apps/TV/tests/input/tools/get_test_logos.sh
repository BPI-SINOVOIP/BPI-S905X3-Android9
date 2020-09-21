#!/bin/bash
#
# Copyright (C) 2017 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

BASE="http://www.google.com/chart?chs=32&chst=d_simple_text_icon_above"

#  From http://developers.google.com/chart/image/docs/gallery/dynamic_icons#basic-icons
icons=(
   academy activities airport amusement aquarium
   art-gallery atm baby bank-dollar bank-euro
   bank-intl bank-pound bank-yen bar barber
   beach beer bicycle books bowling
   bus cafe camping car-dealer car-rental
   car-repair casino caution cemetery-grave cemetery-tomb
   cinema civic-building computer corporate courthouse
   fire flag floral helicopter home
   info landslide legal location locomotive
   medical mobile motorcycle music parking
   pet petrol phone picnic postal
   repair restaurant sail school scissors
   ship shoppingbag shoppingcart ski snack
   snow sport star swim taxi train
   truck wc-female wc-male wc
   wheelchair
   )

# The 500s from https://spec.googleplex.com/quantumpalette#
colors=(
    DB4437 E91E63 9C27B0 673AB7 3F51B5
    4285F4 03A9F4 00BCD4 009688 0F9D58
    8BC34A CDDC39 FFEB3B F4B400 FF9800
    FF5722 795548 9E9E9E 607D8B
)


# See https://developers.google.com/chart/image/docs/gallery/dynamic_icons
for n in `seq 1 80`;
do
  i=n%76
  c=$(($RANDOM%19))
  # <font_size>|<font_fill_color>|<icon_name>|<icon_size>|<icon_fill_color>|<icon_and_text_border_color>
  url=${BASE}"&chld=Ch+"${n}"|7|00F|${icons[${i}]}|24|${colors[${c}]}|FFF"
  echo ${url}
  curl ${url} -o tests/input/res/drawable-xhdpi/ch_${n}_logo.png
done;