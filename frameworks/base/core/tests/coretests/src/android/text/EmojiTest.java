/*
 * Copyright 2018 The Android Open Source Project
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

package android.text;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;

import android.icu.lang.UCharacterDirection;
import android.icu.text.Bidi;
import android.support.test.filters.SmallTest;
import android.support.test.runner.AndroidJUnit4;

import org.junit.Test;
import org.junit.runner.RunWith;

/**
 * Emoji and ICU drops does not happen at the same time. Therefore there are almost always cases
 * where the existing ICU version is not aware of the latest emoji that Android supports.
 * This test covers Emoji and ICU related functions where other components such as
 * {@link AndroidBidi}, {@link BidiFormatter} depend on. The tests are collected into the same
 * class since the changes effect all those classes.
 */
@SmallTest
@RunWith(AndroidJUnit4.class)
public class EmojiTest {

    @Test
    public void testIsNewEmoji_Emoji5() {
        // each row in the data is the range of emoji
        final int[][][] data = new int[][][]{
                {       // EMOJI 5
                        // range of emoji: i.e from 0x1F6F7 to 0x1F6F8 inclusive
                        {0x1F6F7, 0x1F6F8},
                        {0x1F91F, 0x1F91F},
                        {0x1F928, 0x1F92F},
                        {0x1F94C, 0x1F94C},
                        {0x1F95F, 0x1F96B},
                        {0x1F992, 0x1F997},
                        {0x1F9D0, 0x1F9E6},
                },
                {       // EMOJI 11
                        {0x265F, 0x265F},
                        {0x267E, 0x267E},
                        {0x1F6F9, 0x1F6F9},
                        {0x1F94D, 0x1F94F},
                        {0x1F96C, 0x1F970},
                        {0x1F973, 0x1F976},
                        {0x1F97A, 0x1F97A},
                        {0x1F97C, 0x1F97F},
                        {0x1F998, 0x1F9A2},
                        {0x1F9B0, 0x1F9B9},
                        {0x1F9C1, 0x1F9C2},
                        {0x1F9E7, 0x1F9FF},
                }
        };

        final Bidi icuBidi = new Bidi(0 /* maxLength */, 0 /* maxRunCount */);
        icuBidi.setCustomClassifier(new AndroidBidi.EmojiBidiOverride());

        for (int version = 0; version < data.length; version++) {
            for (int row = 0; row < data[version].length; row++) {
                for (int c = data[version][row][0]; c < data[version][row][1]; c++) {
                    assertTrue(Integer.toHexString(c) + " should be emoji", Emoji.isEmoji(c));

                    assertEquals(Integer.toHexString(c) + " should have neutral directionality",
                            Character.DIRECTIONALITY_OTHER_NEUTRALS,
                            BidiFormatter.DirectionalityEstimator.getDirectionality(c));

                    assertEquals(Integer.toHexString(c) + " shoud be OTHER_NEUTRAL for ICU Bidi",
                            UCharacterDirection.OTHER_NEUTRAL, icuBidi.getCustomizedClass(c));
                }
            }
        }
    }

    @Test
    public void testisEmojiModifierBase_LegacyCompat() {
        assertTrue(Emoji.isEmojiModifierBase(0x1F91D));
        assertTrue(Emoji.isEmojiModifierBase(0x1F93C));
    }

    @Test
    public void testisEmojiModifierBase() {
        // each row in the data is the range of emoji
        final int[][][] data = new int[][][]{
                {       // EMOJI 5
                        // range of emoji: i.e from 0x1F91F to 0x1F91F inclusive
                        {0x1F91F, 0x1F91F},
                        {0x1F931, 0x1F932},
                        {0x1F9D1, 0x1F9DD},
                },
                {       // EMOJI 11
                        {0x1F9B5, 0x1F9B6},
                        {0x1F9B8, 0x1F9B9}
                }
        };
        for (int version = 0; version < data.length; version++) {
            for (int row = 0; row < data[version].length; row++) {
                for (int c = data[version][row][0]; c < data[version][row][1]; c++) {
                    assertTrue(Integer.toHexString(c) + " should be emoji modifier base",
                            Emoji.isEmojiModifierBase(c));
                }
            }
        }
    }
}
