/*
 * Copyright (C) 2016 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.android.car.radio;

import android.annotation.ColorInt;
import android.annotation.NonNull;
import android.content.Context;
import android.hardware.radio.ProgramSelector;

import com.android.car.broadcastradio.support.platform.ProgramSelectorExt;

/**
 * A class that will take a {@link Program} and return its corresponding color. The colors
 * for different channels can be found on go/aae-ncar-por in the radio section.
 */
public class RadioChannelColorMapper {
    // AM values range from 530 - 1700. The following two values represent where this range is cut
    // into thirds.
    private static final int AM_LOW_THIRD_RANGE = 920;
    private static final int AM_HIGH_THIRD_RANGE = 1210;

    // FM values range from 87.9 - 108.1 kHz. The following two values represent where this range
    // is cut into thirds.
    private static final int FM_LOW_THIRD_RANGE = 94600;
    private static final int FM_HIGH_THIRD_RANGE = 101300;

    @ColorInt private final int mDefaultColor;
    @ColorInt private final int mAmRange1Color;
    @ColorInt private final int mAmRange2Color;
    @ColorInt private final int mAmRange3Color;
    @ColorInt private final int mFmRange1Color;
    @ColorInt private final int mFmRange2Color;
    @ColorInt private final int mFmRange3Color;

    public static RadioChannelColorMapper getInstance(Context context) {
        return new RadioChannelColorMapper(context);
    }

    private RadioChannelColorMapper(Context context) {
        mDefaultColor = context.getColor(R.color.car_radio_bg_color);
        mAmRange1Color = context.getColor(R.color.am_range_1_bg_color);
        mAmRange2Color = context.getColor(R.color.am_range_2_bg_color);
        mAmRange3Color = context.getColor(R.color.am_range_3_bg_color);
        mFmRange1Color = context.getColor(R.color.fm_range_1_bg_color);
        mFmRange2Color = context.getColor(R.color.fm_range_2_bg_color);
        mFmRange3Color = context.getColor(R.color.fm_range_3_bg_color);
    }

    /**
     * Returns the default color for the radio.
     */
    @ColorInt
    public int getDefaultColor() {
        return mDefaultColor;
    }

    /**
     * Convenience method for returning a color based on a {@link Program}.
     *
     * @see #getColorForStation
     */
    @ColorInt
    public int getColorForProgram(@NonNull ProgramSelector sel) {
        if (!ProgramSelectorExt.isAmFmProgram(sel)
                || !ProgramSelectorExt.hasId(sel, ProgramSelector.IDENTIFIER_TYPE_AMFM_FREQUENCY)) {
            return mDefaultColor;
        }
        return getColorForChannel(sel.getFirstId(ProgramSelector.IDENTIFIER_TYPE_AMFM_FREQUENCY));
    }

    /**
     * Returns the color that should be used for the given radio band and channel. If a match cannot
     * be made, then {@link #mDefaultColor} is returned.
     *
     * @param freq The channel frequency in Hertz.
     */
    @ColorInt
    public int getColorForChannel(long freq) {
        if (ProgramSelectorExt.isAmFrequency(freq)) {
                if (freq < AM_LOW_THIRD_RANGE) return mAmRange1Color;
                else if (freq > AM_HIGH_THIRD_RANGE) return mAmRange3Color;
                else return mAmRange2Color;
        } else if (ProgramSelectorExt.isFmFrequency(freq)) {
                if (freq < FM_LOW_THIRD_RANGE) return mFmRange1Color;
                else if (freq > FM_HIGH_THIRD_RANGE) return mFmRange3Color;
                else return mFmRange2Color;
        } else {
            return mDefaultColor;
        }
    }
}
