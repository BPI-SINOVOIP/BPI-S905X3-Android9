/*
 * Copyright (C) 2017 The Android Open Source Project
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

package android.text.cts;

import static android.text.TextDirectionHeuristics.LTR;
import static android.text.TextDirectionHeuristics.RTL;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;

import android.content.Context;
import android.graphics.Color;
import android.graphics.Rect;
import android.graphics.Typeface;
import android.support.test.InstrumentationRegistry;
import android.support.test.filters.SmallTest;
import android.support.test.runner.AndroidJUnit4;
import android.text.Layout;
import android.text.PrecomputedText;
import android.text.PrecomputedText.Params;
import android.text.Spannable;
import android.text.SpannableStringBuilder;
import android.text.Spanned;
import android.text.TextDirectionHeuristics;
import android.text.TextPaint;
import android.text.style.BackgroundColorSpan;
import android.text.style.LocaleSpan;
import android.text.style.TextAppearanceSpan;

import org.junit.Test;
import org.junit.runner.RunWith;

import java.util.Locale;

@SmallTest
@RunWith(AndroidJUnit4.class)
public class PrecomputedTextTest {

    private static final CharSequence NULL_CHAR_SEQUENCE = null;
    private static final String STRING = "Hello, World!";
    private static final String MULTIPARA_STRING = "Hello,\nWorld!";

    private static final int SPAN_START = 3;
    private static final int SPAN_END = 7;
    private static final LocaleSpan SPAN = new LocaleSpan(Locale.US);
    private static final Spanned SPANNED;
    static {
        final SpannableStringBuilder ssb = new SpannableStringBuilder(STRING);
        ssb.setSpan(SPAN, SPAN_START, SPAN_END, Spanned.SPAN_INCLUSIVE_EXCLUSIVE);
        SPANNED = ssb;
    }

    private static final TextPaint PAINT = new TextPaint();

    @Test
    public void testParams_create() {
        assertNotNull(new Params.Builder(PAINT).build());
        assertNotNull(new Params.Builder(PAINT)
                .setBreakStrategy(Layout.BREAK_STRATEGY_SIMPLE).build());
        assertNotNull(new Params.Builder(PAINT)
                .setBreakStrategy(Layout.BREAK_STRATEGY_SIMPLE)
                .setHyphenationFrequency(Layout.HYPHENATION_FREQUENCY_NORMAL).build());
        assertNotNull(new Params.Builder(PAINT)
                .setBreakStrategy(Layout.BREAK_STRATEGY_SIMPLE)
                .setHyphenationFrequency(Layout.HYPHENATION_FREQUENCY_NORMAL)
                .setTextDirection(LTR).build());
    }

    @Test
    public void testParams_SetGet() {
        assertEquals(Layout.BREAK_STRATEGY_SIMPLE, new Params.Builder(PAINT)
                .setBreakStrategy(Layout.BREAK_STRATEGY_SIMPLE).build().getBreakStrategy());
        assertEquals(Layout.HYPHENATION_FREQUENCY_NONE, new Params.Builder(PAINT)
                .setHyphenationFrequency(Layout.HYPHENATION_FREQUENCY_NONE).build()
                        .getHyphenationFrequency());
        assertEquals(RTL, new Params.Builder(PAINT).setTextDirection(RTL).build()
                .getTextDirection());
    }

    @Test
    public void testParams_GetDefaultValues() {
        assertEquals(Layout.BREAK_STRATEGY_HIGH_QUALITY,
                     new Params.Builder(PAINT).build().getBreakStrategy());
        assertEquals(Layout.HYPHENATION_FREQUENCY_NORMAL,
                     new Params.Builder(PAINT).build().getHyphenationFrequency());
        assertEquals(TextDirectionHeuristics.FIRSTSTRONG_LTR,
                     new Params.Builder(PAINT).build().getTextDirection());
    }

    @Test
    public void testParams_equals() {
        final Params base = new Params.Builder(PAINT)
                .setBreakStrategy(Layout.BREAK_STRATEGY_HIGH_QUALITY)
                .setHyphenationFrequency(Layout.HYPHENATION_FREQUENCY_NORMAL)
                .setTextDirection(LTR).build();

        assertFalse(base.equals(null));
        assertTrue(base.equals(base));
        assertFalse(base.equals(new Object()));

        Params other = new Params.Builder(PAINT)
                .setBreakStrategy(Layout.BREAK_STRATEGY_HIGH_QUALITY)
                .setHyphenationFrequency(Layout.HYPHENATION_FREQUENCY_NORMAL)
                .setTextDirection(LTR).build();
        assertTrue(base.equals(other));
        assertTrue(other.equals(base));
        assertEquals(base.hashCode(), other.hashCode());

        other = new Params.Builder(PAINT)
                .setBreakStrategy(Layout.BREAK_STRATEGY_SIMPLE)
                .setHyphenationFrequency(Layout.HYPHENATION_FREQUENCY_NORMAL)
                .setTextDirection(LTR).build();
        assertFalse(base.equals(other));
        assertFalse(other.equals(base));

        other = new Params.Builder(PAINT)
                .setBreakStrategy(Layout.BREAK_STRATEGY_HIGH_QUALITY)
                .setHyphenationFrequency(Layout.HYPHENATION_FREQUENCY_NONE)
                .setTextDirection(LTR).build();
        assertFalse(base.equals(other));
        assertFalse(other.equals(base));


        other = new Params.Builder(PAINT)
                .setBreakStrategy(Layout.BREAK_STRATEGY_HIGH_QUALITY)
                .setHyphenationFrequency(Layout.HYPHENATION_FREQUENCY_NORMAL)
                .setTextDirection(RTL).build();
        assertFalse(base.equals(other));
        assertFalse(other.equals(base));


        TextPaint anotherPaint = new TextPaint(PAINT);
        anotherPaint.setTextSize(PAINT.getTextSize() * 2.0f);
        other = new Params.Builder(anotherPaint)
                .setBreakStrategy(Layout.BREAK_STRATEGY_HIGH_QUALITY)
                .setHyphenationFrequency(Layout.HYPHENATION_FREQUENCY_NORMAL)
                .setTextDirection(LTR).build();
        assertFalse(base.equals(other));
        assertFalse(other.equals(base));

    }

    @Test
    public void testCreate_withNull() {
        final Params param = new Params.Builder(PAINT).build();
        try {
            PrecomputedText.create(NULL_CHAR_SEQUENCE, param);
            fail();
        } catch (NullPointerException e) {
            // pass
        }
        try {
            PrecomputedText.create(STRING, null);
            fail();
        } catch (NullPointerException e) {
            // pass
        }
    }

    @Test
    public void testCharSequenceInteface() {
        final Params param = new Params.Builder(PAINT).build();
        final CharSequence s = PrecomputedText.create(STRING, param);
        assertEquals(STRING.length(), s.length());
        assertEquals('H', s.charAt(0));
        assertEquals('e', s.charAt(1));
        assertEquals('l', s.charAt(2));
        assertEquals('l', s.charAt(3));
        assertEquals('o', s.charAt(4));
        assertEquals(',', s.charAt(5));
        assertEquals("Hello, World!", s.toString());

        final CharSequence s3 = s.subSequence(0, 3);
        assertEquals(3, s3.length());
        assertEquals('H', s3.charAt(0));
        assertEquals('e', s3.charAt(1));
        assertEquals('l', s3.charAt(2));

    }

    @Test
    public void testSpannedInterface_Spanned() {
        final Params param = new Params.Builder(PAINT).build();
        final Spanned s = PrecomputedText.create(SPANNED, param);
        final LocaleSpan[] spans = s.getSpans(0, s.length(), LocaleSpan.class);
        assertNotNull(spans);
        assertEquals(1, spans.length);
        assertEquals(SPAN, spans[0]);

        assertEquals(SPAN_START, s.getSpanStart(SPAN));
        assertEquals(SPAN_END, s.getSpanEnd(SPAN));
        assertTrue((s.getSpanFlags(SPAN) & Spanned.SPAN_INCLUSIVE_EXCLUSIVE) != 0);

        assertEquals(SPAN_START, s.nextSpanTransition(0, s.length(), LocaleSpan.class));
        assertEquals(SPAN_END, s.nextSpanTransition(SPAN_START, s.length(), LocaleSpan.class));
    }

    @Test
    public void testSpannedInterface_Spannable() {
        final BackgroundColorSpan span = new BackgroundColorSpan(Color.RED);
        final Params param = new Params.Builder(PAINT).build();
        final Spannable s = PrecomputedText.create(STRING, param);
        assertEquals(0, s.getSpans(0, s.length(), BackgroundColorSpan.class).length);

        s.setSpan(span, SPAN_START, SPAN_END, Spanned.SPAN_INCLUSIVE_EXCLUSIVE);

        final BackgroundColorSpan[] spans = s.getSpans(0, s.length(), BackgroundColorSpan.class);
        assertEquals(SPAN_START, s.getSpanStart(span));
        assertEquals(SPAN_END, s.getSpanEnd(span));
        assertTrue((s.getSpanFlags(span) & Spanned.SPAN_INCLUSIVE_EXCLUSIVE) != 0);

        assertEquals(SPAN_START, s.nextSpanTransition(0, s.length(), BackgroundColorSpan.class));
        assertEquals(SPAN_END,
                s.nextSpanTransition(SPAN_START, s.length(), BackgroundColorSpan.class));

        s.removeSpan(span);
        assertEquals(0, s.getSpans(0, s.length(), BackgroundColorSpan.class).length);
    }

    @Test(expected = IllegalArgumentException.class)
    public void testSpannedInterface_Spannable_setSpan_MetricsAffectingSpan() {
        final Params param = new Params.Builder(PAINT).build();
        final Spannable s = PrecomputedText.create(SPANNED, param);
        s.setSpan(SPAN, SPAN_START, SPAN_END, Spanned.SPAN_INCLUSIVE_EXCLUSIVE);
    }

    @Test(expected = IllegalArgumentException.class)
    public void testSpannedInterface_Spannable_removeSpan_MetricsAffectingSpan() {
        final Params param = new Params.Builder(PAINT).build();
        final Spannable s = PrecomputedText.create(SPANNED, param);
        s.removeSpan(SPAN);
    }

    @Test
    public void testSpannedInterface_String() {
        final Params param = new Params.Builder(PAINT).build();
        final Spanned s = PrecomputedText.create(STRING, param);
        LocaleSpan[] spans = s.getSpans(0, s.length(), LocaleSpan.class);
        assertNotNull(spans);
        assertEquals(0, spans.length);

        assertEquals(-1, s.getSpanStart(SPAN));
        assertEquals(-1, s.getSpanEnd(SPAN));
        assertEquals(0, s.getSpanFlags(SPAN));

        assertEquals(s.length(), s.nextSpanTransition(0, s.length(), LocaleSpan.class));
    }

    @Test
    public void testGetParagraphCount() {
        final Params param = new Params.Builder(PAINT).build();
        final PrecomputedText pm = PrecomputedText.create(STRING, param);
        assertEquals(1, pm.getParagraphCount());
        assertEquals(0, pm.getParagraphStart(0));
        assertEquals(STRING.length(), pm.getParagraphEnd(0));

        final PrecomputedText pm1 = PrecomputedText.create(MULTIPARA_STRING, param);
        assertEquals(2, pm1.getParagraphCount());
        assertEquals(0, pm1.getParagraphStart(0));
        assertEquals(7, pm1.getParagraphEnd(0));
        assertEquals(7, pm1.getParagraphStart(1));
        assertEquals(pm1.length(), pm1.getParagraphEnd(1));
    }

    @Test
    public void testGetWidth() {
        final Context context = InstrumentationRegistry.getInstrumentation().getTargetContext();

        // The test font has following coverage and width.
        // U+0020: 10em
        // U+002E (.): 10em
        // U+0043 (C): 100em
        // U+0049 (I): 1em
        // U+004C (L): 50em
        // U+0056 (V): 5em
        // U+0058 (X): 10em
        // U+005F (_): 0em
        // U+FFFD (invalid surrogate will be replaced to this): 7em
        // U+10331 (\uD800\uDF31): 10em
        final Typeface tf = new Typeface.Builder(context.getAssets(),
                "fonts/StaticLayoutLineBreakingTestFont.ttf").build();
        final TextPaint paint = new TextPaint();
        paint.setTypeface(tf);
        paint.setTextSize(1);  // Make 1em = 1px

        final Params param = new Params.Builder(paint).build();
        assertEquals(0.0f, PrecomputedText.create("", param).getWidth(0, 0), 0.0f);

        assertEquals(0.0f, PrecomputedText.create("I", param).getWidth(0, 0), 0.0f);
        assertEquals(0.0f, PrecomputedText.create("I", param).getWidth(1, 1), 0.0f);
        assertEquals(1.0f, PrecomputedText.create("I", param).getWidth(0, 1), 0.0f);

        assertEquals(0.0f, PrecomputedText.create("V", param).getWidth(0, 0), 0.0f);
        assertEquals(0.0f, PrecomputedText.create("V", param).getWidth(1, 1), 0.0f);
        assertEquals(5.0f, PrecomputedText.create("V", param).getWidth(0, 1), 0.0f);

        assertEquals(0.0f, PrecomputedText.create("IV", param).getWidth(0, 0), 0.0f);
        assertEquals(0.0f, PrecomputedText.create("IV", param).getWidth(1, 1), 0.0f);
        assertEquals(0.0f, PrecomputedText.create("IV", param).getWidth(2, 2), 0.0f);
        assertEquals(1.0f, PrecomputedText.create("IV", param).getWidth(0, 1), 0.0f);
        assertEquals(5.0f, PrecomputedText.create("IV", param).getWidth(1, 2), 0.0f);
        assertEquals(6.0f, PrecomputedText.create("IV", param).getWidth(0, 2), 0.0f);

        assertEquals(0.0f, PrecomputedText.create("I\nV", param).getWidth(0, 0), 0.0f);
        assertEquals(0.0f, PrecomputedText.create("I\nV", param).getWidth(1, 1), 0.0f);
        assertEquals(0.0f, PrecomputedText.create("I\nV", param).getWidth(2, 2), 0.0f);
        assertEquals(0.0f, PrecomputedText.create("I\nV", param).getWidth(3, 3), 0.0f);
        assertEquals(1.0f, PrecomputedText.create("I\nV", param).getWidth(0, 1), 0.0f);
        assertEquals(1.0f, PrecomputedText.create("I\nV", param).getWidth(0, 2), 0.0f);
        assertEquals(5.0f, PrecomputedText.create("I\nV", param).getWidth(2, 3), 0.0f);
    }

    @Test
    public void testGetWidth_multiStyle() {
        final Context context = InstrumentationRegistry.getInstrumentation().getTargetContext();
        final SpannableStringBuilder ssb = new SpannableStringBuilder("II");
        ssb.setSpan(new TextAppearanceSpan(null /* family */, Typeface.NORMAL, 1 /* text size */,
                null /* color */, null /* link color */), 0, 1, Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);
        ssb.setSpan(new TextAppearanceSpan(null /* family */, Typeface.NORMAL, 5 /* text size */,
                null /* color */, null /* link color */), 1, 2, Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);

        final Typeface tf = new Typeface.Builder(context.getAssets(),
                "fonts/StaticLayoutLineBreakingTestFont.ttf").build();
        final TextPaint paint = new TextPaint();
        paint.setTypeface(tf);
        paint.setTextSize(1);  // Make 1em = 1px

        final Params param = new Params.Builder(paint).build();

        assertEquals(0.0f, PrecomputedText.create(ssb, param).getWidth(0, 0), 0.0f);
        assertEquals(0.0f, PrecomputedText.create(ssb, param).getWidth(1, 1), 0.0f);
        assertEquals(0.0f, PrecomputedText.create(ssb, param).getWidth(2, 2), 0.0f);

        assertEquals(1.0f, PrecomputedText.create(ssb, param).getWidth(0, 1), 0.0f);
        assertEquals(5.0f, PrecomputedText.create(ssb, param).getWidth(1, 2), 0.0f);

        assertEquals(6.0f, PrecomputedText.create(ssb, param).getWidth(0, 2), 0.0f);
    }

    @Test
    public void testGetWidth_multiStyle2() {
        final Context context = InstrumentationRegistry.getInstrumentation().getTargetContext();
        final SpannableStringBuilder ssb = new SpannableStringBuilder("IVI");
        ssb.setSpan(new TextAppearanceSpan(null /* family */, Typeface.NORMAL, 1 /* text size */,
                null /* color */, null /* link color */), 0, 1, Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);
        ssb.setSpan(new TextAppearanceSpan(null /* family */, Typeface.NORMAL, 5 /* text size */,
                null /* color */, null /* link color */), 1, 2, Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);
        ssb.setSpan(new TextAppearanceSpan(null /* family */, Typeface.NORMAL, 5 /* text size */,
                null /* color */, null /* link color */), 2, 3, Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);

        final Typeface tf = new Typeface.Builder(context.getAssets(),
                "fonts/StaticLayoutLineBreakingTestFont.ttf").build();
        final TextPaint paint = new TextPaint();
        paint.setTypeface(tf);
        paint.setTextSize(1);  // Make 1em = 1px

        final Params param = new Params.Builder(paint).build();

        assertEquals(0.0f, PrecomputedText.create(ssb, param).getWidth(0, 0), 0.0f);
        assertEquals(0.0f, PrecomputedText.create(ssb, param).getWidth(1, 1), 0.0f);
        assertEquals(0.0f, PrecomputedText.create(ssb, param).getWidth(2, 2), 0.0f);
        assertEquals(0.0f, PrecomputedText.create(ssb, param).getWidth(3, 3), 0.0f);

        assertEquals(1.0f, PrecomputedText.create(ssb, param).getWidth(0, 1), 0.0f);
        assertEquals(25.0f, PrecomputedText.create(ssb, param).getWidth(1, 2), 0.0f);
        assertEquals(5.0f, PrecomputedText.create(ssb, param).getWidth(2, 3), 0.0f);

        assertEquals(26.0f, PrecomputedText.create(ssb, param).getWidth(0, 2), 0.0f);
        assertEquals(30.0f, PrecomputedText.create(ssb, param).getWidth(1, 3), 0.0f);
        assertEquals(31.0f, PrecomputedText.create(ssb, param).getWidth(0, 3), 0.0f);
    }

    @Test(expected = IllegalArgumentException.class)
    public void testGetWidth_negative_start_offset() {
        final Params param = new Params.Builder(PAINT).build();
        PrecomputedText.create("a", param).getWidth(-1, 0);
    }

    @Test(expected = IllegalArgumentException.class)
    public void testGetWidth_negative_end_offset() {
        final Params param = new Params.Builder(PAINT).build();
        PrecomputedText.create("a", param).getWidth(0, -1);
    }

    @Test(expected = IllegalArgumentException.class)
    public void testGetWidth_index_out_of_bounds_start_offset() {
        final Params param = new Params.Builder(PAINT).build();
        PrecomputedText.create("a", param).getWidth(2, 2);
    }

    @Test(expected = IllegalArgumentException.class)
    public void testGetWidth_index_out_of_bounds_end_offset() {
        final Params param = new Params.Builder(PAINT).build();
        PrecomputedText.create("a", param).getWidth(0, 2);
    }

    @Test(expected = IllegalArgumentException.class)
    public void testGetWidth_reverse_offset() {
        final Params param = new Params.Builder(PAINT).build();
        PrecomputedText.create("a", param).getWidth(1, 0);
    }

    @Test(expected = IllegalArgumentException.class)
    public void testGetWidth_across_paragraph_boundary() {
        final Params param = new Params.Builder(PAINT).build();
        PrecomputedText.create("a\nb", param).getWidth(0, 3);
    }

    @Test
    public void testGetBounds() {
        final Context context = InstrumentationRegistry.getInstrumentation().getTargetContext();

        // The test font has following coverage and width.
        // U+0020: 10em
        // U+002E (.): 10em
        // U+0043 (C): 100em
        // U+0049 (I): 1em
        // U+004C (L): 50em
        // U+0056 (V): 5em
        // U+0058 (X): 10em
        // U+005F (_): 0em
        // U+FFFD (invalid surrogate will be replaced to this): 7em
        // U+10331 (\uD800\uDF31): 10em
        final Typeface tf = new Typeface.Builder(context.getAssets(),
                "fonts/StaticLayoutLineBreakingTestFont.ttf").build();
        final TextPaint paint = new TextPaint();
        paint.setTypeface(tf);
        paint.setTextSize(1);  // Make 1em = 1px

        final Params param = new Params.Builder(paint).build();
        final Rect rect = new Rect();

        rect.set(0, 0, 0, 0);
        PrecomputedText.create("", param).getBounds(0, 0, rect);
        assertEquals(new Rect(0, 0, 0, 0), rect);

        rect.set(0, 0, 0, 0);
        PrecomputedText.create("I", param).getBounds(0, 1, rect);
        assertEquals(new Rect(0, -1, 1, 0), rect);

        rect.set(0, 0, 0, 0);
        PrecomputedText.create("I", param).getBounds(1, 1, rect);
        assertEquals(new Rect(0, 0, 0, 0), rect);

        rect.set(0, 0, 0, 0);
        PrecomputedText.create("IV", param).getBounds(0, 0, rect);
        assertEquals(new Rect(0, 0, 0, 0), rect);

        rect.set(0, 0, 0, 0);
        PrecomputedText.create("IV", param).getBounds(0, 0, rect);
        assertEquals(new Rect(0, 0, 0, 0), rect);

        rect.set(0, 0, 0, 0);
        PrecomputedText.create("IV", param).getBounds(1, 1, rect);
        assertEquals(new Rect(0, 0, 0, 0), rect);

        rect.set(0, 0, 0, 0);
        PrecomputedText.create("IV", param).getBounds(2, 2, rect);
        assertEquals(new Rect(0, 0, 0, 0), rect);

        rect.set(0, 0, 0, 0);
        PrecomputedText.create("IV", param).getBounds(0, 1, rect);
        assertEquals(new Rect(0, -1, 1, 0), rect);

        rect.set(0, 0, 0, 0);
        PrecomputedText.create("IV", param).getBounds(1, 2, rect);
        assertEquals(new Rect(0, -5, 5, 0), rect);

        rect.set(0, 0, 0, 0);
        PrecomputedText.create("IV", param).getBounds(0, 2, rect);
        assertEquals(new Rect(0, -5, 6, 0), rect);
    }

    @Test
    public void testGetBounds_multiStyle() {
        final Context context = InstrumentationRegistry.getInstrumentation().getTargetContext();
        final SpannableStringBuilder ssb = new SpannableStringBuilder("II");
        ssb.setSpan(new TextAppearanceSpan(null /* family */, Typeface.NORMAL, 1 /* text size */,
                null /* color */, null /* link color */), 0, 1, Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);
        ssb.setSpan(new TextAppearanceSpan(null /* family */, Typeface.NORMAL, 5 /* text size */,
                null /* color */, null /* link color */), 1, 2, Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);

        final Typeface tf = new Typeface.Builder(context.getAssets(),
                "fonts/StaticLayoutLineBreakingTestFont.ttf").build();
        final TextPaint paint = new TextPaint();
        paint.setTypeface(tf);
        paint.setTextSize(1);  // Make 1em = 1px

        final Params param = new Params.Builder(paint).build();
        final Rect rect = new Rect();

        rect.set(0, 0, 0, 0);
        PrecomputedText.create(ssb, param).getBounds(0, 0, rect);
        assertEquals(new Rect(0, 0, 0, 0), rect);

        rect.set(0, 0, 0, 0);
        PrecomputedText.create(ssb, param).getBounds(0, 1, rect);
        assertEquals(new Rect(0, -1, 1, 0), rect);

        rect.set(0, 0, 0, 0);
        PrecomputedText.create(ssb, param).getBounds(1, 2, rect);
        assertEquals(new Rect(0, -5, 5, 0), rect);

        rect.set(0, 0, 0, 0);
        PrecomputedText.create(ssb, param).getBounds(0, 2, rect);
        assertEquals(new Rect(0, -5, 6, 0), rect);
    }

    @Test
    public void testGetBounds_multiStyle2() {
        final Context context = InstrumentationRegistry.getInstrumentation().getTargetContext();
        final SpannableStringBuilder ssb = new SpannableStringBuilder("IVI");
        ssb.setSpan(new TextAppearanceSpan(null /* family */, Typeface.NORMAL, 1 /* text size */,
                null /* color */, null /* link color */), 0, 1, Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);
        ssb.setSpan(new TextAppearanceSpan(null /* family */, Typeface.NORMAL, 5 /* text size */,
                null /* color */, null /* link color */), 1, 2, Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);
        ssb.setSpan(new TextAppearanceSpan(null /* family */, Typeface.NORMAL, 5 /* text size */,
                null /* color */, null /* link color */), 2, 3, Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);

        final Typeface tf = new Typeface.Builder(context.getAssets(),
                "fonts/StaticLayoutLineBreakingTestFont.ttf").build();
        final TextPaint paint = new TextPaint();
        paint.setTypeface(tf);
        paint.setTextSize(1);  // Make 1em = 1px

        final Params param = new Params.Builder(paint).build();
        final Rect rect = new Rect();

        rect.set(0, 0, 0, 0);
        PrecomputedText.create(ssb, param).getBounds(0, 0, rect);
        assertEquals(new Rect(0, 0, 0, 0), rect);

        rect.set(0, 0, 0, 0);
        PrecomputedText.create(ssb, param).getBounds(0, 1, rect);
        assertEquals(new Rect(0, -1, 1, 0), rect);

        rect.set(0, 0, 0, 0);
        PrecomputedText.create(ssb, param).getBounds(1, 2, rect);
        assertEquals(new Rect(0, -25, 25, 0), rect);

        rect.set(0, 0, 0, 0);
        PrecomputedText.create(ssb, param).getBounds(2, 3, rect);
        assertEquals(new Rect(0, -5, 5, 0), rect);

        rect.set(0, 0, 0, 0);
        PrecomputedText.create(ssb, param).getBounds(0, 2, rect);
        assertEquals(new Rect(0, -25, 26, 0), rect);

        rect.set(0, 0, 0, 0);
        PrecomputedText.create(ssb, param).getBounds(1, 3, rect);
        assertEquals(new Rect(0, -25, 30, 0), rect);

        rect.set(0, 0, 0, 0);
        PrecomputedText.create(ssb, param).getBounds(0, 3, rect);
        assertEquals(new Rect(0, -25, 31, 0), rect);
    }

    @Test(expected = IllegalArgumentException.class)
    public void testGetBounds_negative_start_offset() {
        final Rect rect = new Rect();
        final Params param = new Params.Builder(PAINT).build();
        PrecomputedText.create("a", param).getBounds(-1, 0, rect);
    }

    @Test(expected = IllegalArgumentException.class)
    public void testGetBounds_negative_end_offset() {
        final Rect rect = new Rect();
        final Params param = new Params.Builder(PAINT).build();
        PrecomputedText.create("a", param).getBounds(0, -1, rect);
    }

    @Test(expected = IllegalArgumentException.class)
    public void testGetBounds_index_out_of_bounds_start_offset() {
        final Rect rect = new Rect();
        final Params param = new Params.Builder(PAINT).build();
        PrecomputedText.create("a", param).getBounds(2, 2, rect);
    }

    @Test(expected = IllegalArgumentException.class)
    public void testGetBounds_index_out_of_bounds_end_offset() {
        final Rect rect = new Rect();
        final Params param = new Params.Builder(PAINT).build();
        PrecomputedText.create("a", param).getBounds(0, 2, rect);
    }

    @Test(expected = IllegalArgumentException.class)
    public void testGetBounds_reverse_offset() {
        final Rect rect = new Rect();
        final Params param = new Params.Builder(PAINT).build();
        PrecomputedText.create("a", param).getBounds(1, 0, rect);
    }

    @Test(expected = NullPointerException.class)
    public void testGetBounds_null_rect() {
        final Params param = new Params.Builder(PAINT).build();
        PrecomputedText.create("a", param).getBounds(0, 1, null);
    }

    @Test(expected = IllegalArgumentException.class)
    public void testGetBounds_across_paragraph_boundary() {
        final Rect rect = new Rect();
        final Params param = new Params.Builder(PAINT).build();
        PrecomputedText.create("a\nb", param).getBounds(0, 3, rect);
    }

}
