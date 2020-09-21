/*
 * Copyright (C) 2018 The Android Open Source Project
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

package android.widget.cts;

import android.content.Context;
import android.graphics.Typeface;
import android.support.test.InstrumentationRegistry;
import android.support.test.filters.SmallTest;
import android.text.Layout;
import android.text.PrecomputedText;
import android.text.PrecomputedText.Params;
import android.text.PrecomputedText.Params.Builder;
import android.text.TextDirectionHeuristic;
import android.text.TextDirectionHeuristics;
import android.text.TextPaint;
import android.text.TextUtils;
import android.util.Pair;
import android.widget.TextView;

import static org.junit.Assert.fail;

import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.Parameterized;

import java.util.ArrayList;
import java.util.Collection;
import java.util.Locale;

/**
 * Tests for TextView with precomputed text.
 */
@RunWith(Parameterized.class)
public class TextViewPrecomputedTextTest {
    private static final String TEXT = "Hello, World!";

    @Parameterized.Parameter(0)
    public boolean differentTextSize;
    @Parameterized.Parameter(1)
    public boolean differentScaleX;
    @Parameterized.Parameter(2)
    public boolean differentSkewX;
    @Parameterized.Parameter(3)
    public boolean differentLetterSpacing;
    @Parameterized.Parameter(4)
    public boolean differentTextLocale;
    @Parameterized.Parameter(5)
    public boolean differentTypeface;
    @Parameterized.Parameter(6)
    public boolean differentElegantTextHeight;
    @Parameterized.Parameter(7)
    public boolean differentBreakStrategy;
    @Parameterized.Parameter(8)
    public boolean differentHyphenationFrequency;
    @Parameterized.Parameter(9)
    public boolean differentTextDir;
    @Parameterized.Parameter(10)
    public boolean differentFontVariationSettings;

    // text size from the default value.
    private Pair<Params, String[]> makeDifferentParams(Params params) {
        final TextPaint paint = new TextPaint(params.getTextPaint());
        ArrayList<String> differenceList = new ArrayList();

        if (differentTextSize) {
            paint.setTextSize(paint.getTextSize() * 2.0f + 1.0f);
            differenceList.add("Text Size");
        }
        if (differentScaleX) {
            paint.setTextScaleX(paint.getTextScaleX() * 2.0f + 1.0f);
            differenceList.add("Text Scale X");
        }
        if (differentSkewX) {
            paint.setTextSkewX(paint.getTextSkewX() * 2.0f + 1.0f);
            differenceList.add("Text Skew X");
        }
        if (differentLetterSpacing) {
            paint.setLetterSpacing(paint.getLetterSpacing() * 2.0f + 1.0f);
            differenceList.add("Letter Spacing");
        }
        if (differentTextLocale) {
            paint.setTextLocale(Locale.US.equals(paint.getTextLocale()) ? Locale.JAPAN : Locale.US);
        }
        if (differentTypeface) {
            final Typeface tf = paint.getTypeface();
            if (tf == null || tf == Typeface.DEFAULT) {
                paint.setTypeface(Typeface.SERIF);
            } else {
                paint.setTypeface(Typeface.DEFAULT);
            }
            differenceList.add("Typeface");
        }
        if (differentFontVariationSettings) {
            final String wght = "'wght' 700";
            final String wdth = "'wdth' 100";

            final String varSettings = paint.getFontVariationSettings();
            if (varSettings == null || varSettings.equals(wght)) {
                paint.setFontVariationSettings(wdth);
            } else {
                paint.setFontVariationSettings(wght);
            }
            differenceList.add("Font variation settings");
        }
        if (differentElegantTextHeight) {
            paint.setElegantTextHeight(!paint.isElegantTextHeight());
            differenceList.add("Elegant Text Height");
        }

        int strategy = params.getBreakStrategy();
        if (differentBreakStrategy) {
            strategy = strategy == Layout.BREAK_STRATEGY_SIMPLE
                    ?  Layout.BREAK_STRATEGY_HIGH_QUALITY : Layout.BREAK_STRATEGY_SIMPLE;
            differenceList.add("Break strategy");
        }

        int hyFreq = params.getHyphenationFrequency();
        if (differentHyphenationFrequency) {
            hyFreq = hyFreq == Layout.HYPHENATION_FREQUENCY_NONE
                    ? Layout.HYPHENATION_FREQUENCY_FULL : Layout.HYPHENATION_FREQUENCY_NONE;
            differenceList.add("Hyphenation Frequency");
        }

        TextDirectionHeuristic dir = params.getTextDirection();
        if (differentTextDir) {
            dir = dir == TextDirectionHeuristics.LTR
                    ?  TextDirectionHeuristics.RTL : TextDirectionHeuristics.LTR;
            differenceList.add("Text Direction");
        }

        final Params outParams = new Builder(paint).setBreakStrategy(strategy)
                .setHyphenationFrequency(hyFreq).setTextDirection(dir).build();
        return new Pair(outParams, differenceList.toArray(new String[differenceList.size()]));
    }

    private static Context getContext() {
        return InstrumentationRegistry.getInstrumentation().getTargetContext();
    }

    @Parameterized.Parameters
    public static Collection<Object[]> getParameters() {
        ArrayList<Object[]> allParams = new ArrayList<>();

        // Compute the powerset except for all false case.
        final int allParameterCount = 11;
        // The 11-th bit is for font variation settings. Don't add test case if the system don't
        // have variable fonts.
        final int fullBits = hasVarFont()
                ? (1 << allParameterCount) - 1 : (1 << (allParameterCount - 1)) - 1;
        for (int bits = 1; bits <= fullBits; ++bits) {
            Object[] param = new Object[allParameterCount];
            for (int j = 0; j < allParameterCount; ++j) {
                param[j] = new Boolean((bits & (1 << j)) != 0);
            }
            allParams.add(param);
        }
        return allParams;
    }

    private static boolean hasVarFont() {
        final TextPaint copied = new TextPaint((new TextView(getContext())).getPaint());
        return copied.setFontVariationSettings("'wght' 400")
                && copied.setFontVariationSettings("'wdth' 100");
    }

    @SmallTest
    @Test
    public void setText() {
        final TextView tv = new TextView(getContext());
        final Params tvParams = tv.getTextMetricsParams();
        final Pair<Params, String[]> testConfig = makeDifferentParams(tvParams);
        final Params pctParams = testConfig.first;

        final PrecomputedText pct = PrecomputedText.create(TEXT, pctParams);
        try {
            tv.setText(pct);
            fail("Test Case: {" + TextUtils.join(",", testConfig.second) + "}, "
                    + tvParams.toString() + " vs " + pctParams.toString());
        } catch (IllegalArgumentException e) {
            // pass
        }

        tv.setTextMetricsParams(pctParams);
        tv.setText(pct);
    }
}
