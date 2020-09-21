/*
 * Copyright (C) 2012 The Android Open Source Project
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

import static org.junit.Assert.assertEquals;

import android.content.Context;;
import android.graphics.Typeface;
import android.support.test.InstrumentationRegistry;
import android.support.test.filters.SmallTest;
import android.support.test.runner.AndroidJUnit4;
import android.text.Layout;
import android.text.Layout.Alignment;
import android.text.SpannableStringBuilder;
import android.text.Spanned;
import android.text.StaticLayout;
import android.text.TextDirectionHeuristics;
import android.text.TextPaint;
import android.text.style.MetricAffectingSpan;
import android.util.Log;

import org.junit.Test;
import org.junit.runner.RunWith;

@SmallTest
@RunWith(AndroidJUnit4.class)
public class StaticLayoutLineBreakingTest {
    // Span test are currently not supported because text measurement uses the MeasuredText
    // internal mWorkPaint instead of the provided MockTestPaint.
    private static final boolean SPAN_TESTS_SUPPORTED = false;
    private static final boolean DEBUG = false;

    private static final float SPACE_MULTI = 1.0f;
    private static final float SPACE_ADD = 0.0f;
    private static final int WIDTH = 100;
    private static final Alignment ALIGN = Alignment.ALIGN_NORMAL;

    private static final char SURR_FIRST = '\uD800';
    private static final char SURR_SECOND = '\uDF31';

    private static final int[] NO_BREAK = new int[] {};

    private static final TextPaint sTextPaint = new TextPaint();

    static {
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
        Context context = InstrumentationRegistry.getTargetContext();
        sTextPaint.setTypeface(Typeface.createFromAsset(context.getAssets(),
                  "fonts/StaticLayoutLineBreakingTestFont.ttf"));
        sTextPaint.setTextSize(1.0f);  // Make 1em == 1px.
    }

    private static StaticLayout getStaticLayout(CharSequence source, int width,
            int breakStrategy) {
        return StaticLayout.Builder.obtain(source, 0, source.length(), sTextPaint, width)
                .setAlignment(ALIGN)
                .setLineSpacing(SPACE_ADD, SPACE_MULTI)
                .setIncludePad(false)
                .setBreakStrategy(breakStrategy)
                .build();
    }

    private static int[] getBreaks(CharSequence source) {
        return getBreaks(source, WIDTH, Layout.BREAK_STRATEGY_SIMPLE);
    }

    private static int[] getBreaks(CharSequence source, int width, int breakStrategy) {
        final StaticLayout staticLayout = getStaticLayout(source, width, breakStrategy);

        final int[] breaks = new int[staticLayout.getLineCount() - 1];
        for (int line = 0; line < breaks.length; line++) {
            breaks[line] = staticLayout.getLineEnd(line);
        }
        return breaks;
    }

    private static void debugLayout(CharSequence source, StaticLayout staticLayout) {
        if (DEBUG) {
            int count = staticLayout.getLineCount();
            Log.i("SLLBTest", "\"" + source.toString() + "\": "
                    + count + " lines");
            for (int line = 0; line < count; line++) {
                int lineStart = staticLayout.getLineStart(line);
                int lineEnd = staticLayout.getLineEnd(line);
                Log.i("SLLBTest", "Line " + line + " [" + lineStart + ".."
                        + lineEnd + "]\t" + source.subSequence(lineStart, lineEnd));
            }
        }
    }

    private static void layout(CharSequence source, int[] breaks) {
        layout(source, breaks, WIDTH);
    }

    private static void layout(CharSequence source, int[] breaks, int width) {
        final int[] breakStrategies = {Layout.BREAK_STRATEGY_SIMPLE,
                Layout.BREAK_STRATEGY_HIGH_QUALITY};
        for (int breakStrategy : breakStrategies) {
            final StaticLayout staticLayout = getStaticLayout(source, width, breakStrategy);

            debugLayout(source, staticLayout);

            final int lineCount = breaks.length + 1;
            assertEquals("Number of lines", lineCount, staticLayout.getLineCount());

            for (int line = 0; line < lineCount; line++) {
                final int lineStart = staticLayout.getLineStart(line);
                final int lineEnd = staticLayout.getLineEnd(line);

                if (line == 0) {
                    assertEquals("Line start for first line", 0, lineStart);
                } else {
                    assertEquals("Line start for line " + line, breaks[line - 1], lineStart);
                }

                if (line == lineCount - 1) {
                    assertEquals("Line end for last line", source.length(), lineEnd);
                } else {
                    assertEquals("Line end for line " + line, breaks[line], lineEnd);
                }
            }
        }
    }

    private static void layoutMaxLines(CharSequence source, int[] breaks, int maxLines) {
        final StaticLayout staticLayout = StaticLayout.Builder
                .obtain(source, 0, source.length(), sTextPaint, WIDTH)
                .setAlignment(ALIGN)
                .setTextDirection(TextDirectionHeuristics.LTR)
                .setLineSpacing(SPACE_ADD, SPACE_MULTI)
                .setIncludePad(false)
                .setMaxLines(maxLines)
                .build();

        debugLayout(source, staticLayout);

        final int lineCount = staticLayout.getLineCount();

        for (int line = 0; line < lineCount; line++) {
            int lineStart = staticLayout.getLineStart(line);
            int lineEnd = staticLayout.getLineEnd(line);

            if (line == 0) {
                assertEquals("Line start for first line", 0, lineStart);
            } else {
                assertEquals("Line start for line " + line, breaks[line - 1], lineStart);
            }

            if (line == lineCount - 1 && line != breaks.length - 1) {
                assertEquals("Line end for last line", source.length(), lineEnd);
            } else {
                assertEquals("Line end for line " + line, breaks[line], lineEnd);
            }
        }
    }

    private static final int MAX_SPAN_COUNT = 10;
    private static final int[] sSpanStarts = new int[MAX_SPAN_COUNT];
    private static final int[] sSpanEnds = new int[MAX_SPAN_COUNT];

    private static MetricAffectingSpan getMetricAffectingSpan() {
        return new MetricAffectingSpan() {
            @Override
            public void updateDrawState(TextPaint tp) { /* empty */ }

            @Override
            public void updateMeasureState(TextPaint p) { /* empty */ }
        };
    }

    /**
     * Replaces the "<...>" blocks by spans, assuming non overlapping, correctly defined spans
     * @param text
     * @return A CharSequence with '<' '>' replaced by MetricAffectingSpan
     */
    private static CharSequence spanify(String text) {
        int startIndex = text.indexOf('<');
        if (startIndex < 0) return text;

        int spanCount = 0;
        do {
            int endIndex = text.indexOf('>');
            if (endIndex < 0) throw new IllegalArgumentException("Unbalanced span markers");

            text = text.substring(0, startIndex) + text.substring(startIndex + 1, endIndex)
                    + text.substring(endIndex + 1);

            sSpanStarts[spanCount] = startIndex;
            sSpanEnds[spanCount] = endIndex - 2;
            spanCount++;

            startIndex = text.indexOf('<');
        } while (startIndex >= 0);

        SpannableStringBuilder result = new SpannableStringBuilder(text);
        for (int i = 0; i < spanCount; i++) {
            result.setSpan(getMetricAffectingSpan(), sSpanStarts[i], sSpanEnds[i],
                    Spanned.SPAN_INCLUSIVE_INCLUSIVE);
        }
        return result;
    }

    @Test
    public void testNoLineBreak() {
        // Width lower than WIDTH
        layout("", NO_BREAK);
        layout("I", NO_BREAK);
        layout("V", NO_BREAK);
        layout("X", NO_BREAK);
        layout("L", NO_BREAK);
        layout("I VILI", NO_BREAK);
        layout("XXXX", NO_BREAK);
        layout("LXXXX", NO_BREAK);

        // Width equal to WIDTH
        layout("C", NO_BREAK);
        layout("LL", NO_BREAK);
        layout("L XXXX", NO_BREAK);
        layout("XXXXXXXXXX", NO_BREAK);
        layout("XXX XXXXXX", NO_BREAK);
        layout("XXX XXXX X", NO_BREAK);
        layout("XXX XXXXX ", NO_BREAK);
        layout(" XXXXXXXX ", NO_BREAK);
        layout("  XX  XXX ", NO_BREAK);
        //      0123456789

        // Width greater than WIDTH, but no break
        layout("  XX  XXX  ", NO_BREAK);
        layout("XX XXX XXX ", NO_BREAK);
        layout("XX XXX XXX     ", NO_BREAK);
        layout("XXXXXXXXXX     ", NO_BREAK);
        //      01234567890
    }

    @Test
    public void testOneLineBreak() {
        //      01234567890
        layout("XX XXX XXXX", new int[] {7});
        layout("XX XXXX XXX", new int[] {8});
        layout("XX XXXXX XX", new int[] {9});
        layout("XX XXXXXX X", new int[] {10});
        //      01234567890
        layout("XXXXXXXXXXX", new int[] {10});
        layout("XXXXXXXXX X", new int[] {10});
        layout("XXXXXXXX XX", new int[] {9});
        layout("XXXXXXX XXX", new int[] {8});
        layout("XXXXXX XXXX", new int[] {7});
        //      01234567890
        layout("LL LL", new int[] {3});
        layout("LLLL", new int[] {2});
        layout("C C", new int[] {2});
        layout("CC", new int[] {1});
    }

    @Test
    public void testSpaceAtBreak() {
        //      0123456789012
        layout("XXXX XXXXX X", new int[] {11});
        layout("XXXXXXXXXX X", new int[] {11});
        layout("XXXXXXXXXV X", new int[] {11});
        layout("C X", new int[] {2});
    }

    @Test
    public void testMultipleSpacesAtBreak() {
        //      0123456789012
        layout("LXX XXXX", new int[] {4});
        layout("LXX  XXXX", new int[] {5});
        layout("LXX   XXXX", new int[] {6});
        layout("LXX    XXXX", new int[] {7});
        layout("LXX     XXXX", new int[] {8});
    }

    @Test
    public void testZeroWidthCharacters() {
        //      0123456789012345678901234
        layout("X_X_X_X_X_X_X_X_X_X", NO_BREAK);
        layout("___X_X_X_X_X_X_X_X_X_X___", NO_BREAK);
        layout("C_X", new int[] {2});
        layout("C__X", new int[] {3});
    }

    /**
     * Note that when the text has spans, StaticLayout does not use the provided TextPaint to
     * measure text runs anymore. This is probably a bug.
     * To be able to use the fake sTextPaint and make this test pass, use mPaint instead of
     * mWorkPaint in MeasuredText#addStyleRun
     */
    @Test
    public void testWithSpans() {
        if (!SPAN_TESTS_SUPPORTED) return;

        layout(spanify("<012 456 89>"), NO_BREAK);
        layout(spanify("012 <456> 89"), NO_BREAK);
        layout(spanify("<012> <456>< 89>"), NO_BREAK);
        layout(spanify("<012> <456> <89>"), NO_BREAK);

        layout(spanify("<012> <456> <89>012"), new int[] {8});
        layout(spanify("<012> <456> 89<012>"), new int[] {8});
        layout(spanify("<012> <456> <89><012>"), new int[] {8});
        layout(spanify("<012> <456> 89 <123>"), new int[] {11});
        layout(spanify("<012> <456> 89< 123>"), new int[] {11});
        layout(spanify("<012> <456> <89> <123>"), new int[] {11});
        layout(spanify("012 456 89 <LXX> XX XX"), new int[] {11, 18});
    }

    /*
     * Adding a span to the string should not change the layout, since the metrics are unchanged.
     */
    @Test
    public void testWithOneSpan() {
        if (!SPAN_TESTS_SUPPORTED) return;

        String[] texts = new String[] { "0123", "012 456", "012 456 89 123", "012 45678 012",
                "012 456 89012 456 89012", "0123456789012" };

        MetricAffectingSpan metricAffectingSpan = getMetricAffectingSpan();

        for (String text : texts) {
            // Get the line breaks without any span
            int[] breaks = getBreaks(text);

            // Add spans on all possible offsets
            for (int spanStart = 0; spanStart < text.length(); spanStart++) {
                for (int spanEnd = spanStart; spanEnd < text.length(); spanEnd++) {
                    SpannableStringBuilder ssb = new SpannableStringBuilder(text);
                    ssb.setSpan(metricAffectingSpan, spanStart, spanEnd,
                            Spanned.SPAN_INCLUSIVE_INCLUSIVE);
                    layout(ssb, breaks);
                }
            }
        }
    }

    @Test
    public void testWithTwoSpans() {
        if (!SPAN_TESTS_SUPPORTED) return;

        String[] texts = new String[] { "0123", "012 456", "012 456 89 123", "012 45678 012",
                "012 456 89012 456 89012", "0123456789012" };

        MetricAffectingSpan metricAffectingSpan1 = getMetricAffectingSpan();
        MetricAffectingSpan metricAffectingSpan2 = getMetricAffectingSpan();

        for (String text : texts) {
            // Get the line breaks without any span
            int[] breaks = getBreaks(text);

            // Add spans on all possible offsets
            for (int spanStart1 = 0; spanStart1 < text.length(); spanStart1++) {
                for (int spanEnd1 = spanStart1; spanEnd1 < text.length(); spanEnd1++) {
                    SpannableStringBuilder ssb = new SpannableStringBuilder(text);
                    ssb.setSpan(metricAffectingSpan1, spanStart1, spanEnd1,
                            Spanned.SPAN_INCLUSIVE_INCLUSIVE);

                    for (int spanStart2 = 0; spanStart2 < text.length(); spanStart2++) {
                        for (int spanEnd2 = spanStart2; spanEnd2 < text.length(); spanEnd2++) {
                            ssb.setSpan(metricAffectingSpan2, spanStart2, spanEnd2,
                                    Spanned.SPAN_INCLUSIVE_INCLUSIVE);
                            layout(ssb, breaks);
                        }
                    }
                }
            }
        }
    }

    public static String replace(String string, char c, char r) {
        return string.replaceAll(String.valueOf(c), String.valueOf(r));
    }

    @Test
    public void testWithSurrogate() {
        layout("LX" + SURR_FIRST + SURR_SECOND, NO_BREAK);
        layout("LXXXX" + SURR_FIRST + SURR_SECOND, NO_BREAK);
        // LXXXXI (91) + SURR_FIRST + SURR_SECOND (10). Do not break in the middle point of
        // surrogatge pair.
        layout("LXXXXI" + SURR_FIRST + SURR_SECOND, new int[] {6});

        // LXXXXI (91) + SURR_SECOND (replaced with REPLACEMENT CHARACTER. width is 7px) fits.
        // Break just after invalid trailing surrogate.
        layout("LXXXXI" + SURR_SECOND + SURR_FIRST, new int[] {7});

        layout("C" + SURR_FIRST + SURR_SECOND, new int[] {1});
    }

    @Test
    public void testNarrowWidth() {
        int[] widths = new int[] { 0, 4, 10 };
        String[] texts = new String[] { "", "X", " ", "XX", " X", "XXX" };

        for (String text: texts) {
            // 15 is such that only one character will fit
            int[] breaks = getBreaks(text, 15, Layout.BREAK_STRATEGY_SIMPLE);

            // Width under 15 should all lead to the same line break
            for (int width: widths) {
                layout(text, breaks, width);
            }
        }
    }

    @Test
    public void testNarrowWidthZeroWidth() {
        int[] widths = new int[] { 1, 4 };
        for (int width: widths) {
            layout("X.", new int[] {1}, width);
            layout("X__", NO_BREAK, width);
            layout("X__X", new int[] {3}, width);
            layout("X__X_", new int[] {3}, width);

            layout("_", NO_BREAK, width);
            layout("__", NO_BREAK, width);

            // TODO: The line breaking algorithms break the line too frequently in the presence of
            // zero-width characters. The following cases document how line-breaking should behave
            // in some cases, where the current implementation does not seem reasonable. (Breaking
            // between a zero-width character that start the line and a character with positive
            // width does not make sense.) Line-breaking should be fixed so that all the following
            // tests end up on one line, with no breaks.
            // layout("_X", NO_BREAK, width);
            // layout("_X_", NO_BREAK, width);
            // layout("__X__", NO_BREAK, width);
        }
    }

    @Test
    public void testMaxLines() {
        layoutMaxLines("C", NO_BREAK, 1);
        layoutMaxLines("C C", new int[] {2}, 1);
        layoutMaxLines("C C", new int[] {2}, 2);
        layoutMaxLines("CC", new int[] {1}, 1);
        layoutMaxLines("CC", new int[] {1}, 2);
    }
}
