/*
 * Copyright (C) 2008 The Android Open Source Project
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

import static android.text.Layout.Alignment.ALIGN_NORMAL;
import static android.text.Spanned.SPAN_EXCLUSIVE_EXCLUSIVE;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;
import static org.mockito.Mockito.mock;

import android.graphics.Paint.FontMetricsInt;
import android.support.test.filters.SmallTest;
import android.support.test.runner.AndroidJUnit4;
import android.text.DynamicLayout;
import android.text.Layout;
import android.text.SpannableStringBuilder;
import android.text.StaticLayout;
import android.text.TextPaint;
import android.text.TextUtils;
import android.text.style.TypefaceSpan;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;

@SmallTest
@RunWith(AndroidJUnit4.class)
public class DynamicLayoutTest {

    private static final float SPACING_MULT_NO_SCALE = 1.0f;
    private static final float SPACING_ADD_NO_SCALE = 0.0f;
    private static final int DEFAULT_OUTER_WIDTH = 150;
    private static final int ELLIPSIZE_WIDTH = 8;
    private static final CharSequence SINGLELINE_CHAR_SEQUENCE = "......";
    private static final String[] TEXT = {"CharSequence\n", "Char\tSequence\n", "CharSequence"};
    private static final CharSequence MULTLINE_CHAR_SEQUENCE = TEXT[0] + TEXT[1] + TEXT[2];
    private static final Layout.Alignment DEFAULT_ALIGN = Layout.Alignment.ALIGN_CENTER;
    private TextPaint mDefaultPaint;

    private static final int LINE0 = 0;
    private static final int LINE0_TOP = 0;
    private static final int LINE1 = 1;
    private static final int LINE2 = 2;
    private static final int LINE3 = 3;

    private static final int ELLIPSIS_UNDEFINED = 0x80000000;

    private DynamicLayout mDynamicLayout;

    @Before
    public void setup() {
        mDefaultPaint = new TextPaint();
        mDynamicLayout = new DynamicLayout(MULTLINE_CHAR_SEQUENCE,
                mDefaultPaint,
                DEFAULT_OUTER_WIDTH,
                DEFAULT_ALIGN,
                SPACING_MULT_NO_SCALE,
                SPACING_ADD_NO_SCALE,
                true);
    }

    @Test
    public void testConstructors() {
        new DynamicLayout(SINGLELINE_CHAR_SEQUENCE,
                MULTLINE_CHAR_SEQUENCE,
                mDefaultPaint,
                DEFAULT_OUTER_WIDTH,
                DEFAULT_ALIGN,
                SPACING_MULT_NO_SCALE,
                SPACING_ADD_NO_SCALE,
                true);
        new DynamicLayout(SINGLELINE_CHAR_SEQUENCE,
                MULTLINE_CHAR_SEQUENCE,
                mDefaultPaint,
                DEFAULT_OUTER_WIDTH,
                DEFAULT_ALIGN,
                SPACING_MULT_NO_SCALE,
                SPACING_ADD_NO_SCALE,
                true,
                TextUtils.TruncateAt.START,
                DEFAULT_OUTER_WIDTH);
        new DynamicLayout(MULTLINE_CHAR_SEQUENCE,
                mDefaultPaint,
                DEFAULT_OUTER_WIDTH,
                DEFAULT_ALIGN,
                SPACING_MULT_NO_SCALE,
                SPACING_ADD_NO_SCALE,
                true);
    }

    @Test
    public void testEllipsis() {
        final DynamicLayout dynamicLayout = new DynamicLayout(SINGLELINE_CHAR_SEQUENCE,
                MULTLINE_CHAR_SEQUENCE,
                mDefaultPaint,
                DEFAULT_OUTER_WIDTH,
                DEFAULT_ALIGN,
                SPACING_MULT_NO_SCALE,
                SPACING_ADD_NO_SCALE,
                true,
                TextUtils.TruncateAt.START,
                DEFAULT_OUTER_WIDTH);
        assertEquals(0, dynamicLayout.getEllipsisCount(LINE1));
        assertEquals(ELLIPSIS_UNDEFINED, dynamicLayout.getEllipsisStart(LINE1));
        assertEquals(DEFAULT_OUTER_WIDTH, dynamicLayout.getEllipsizedWidth());
    }

    /*
     * Test whether include the padding to calculate the layout.
     * 1. Include padding while calculate the layout.
     * 2. Don't include padding while calculate the layout.
     */
    @Test
    public void testIncludePadding() {
        final FontMetricsInt fontMetricsInt = mDefaultPaint.getFontMetricsInt();

        DynamicLayout dynamicLayout = new DynamicLayout(SINGLELINE_CHAR_SEQUENCE,
                mDefaultPaint,
                DEFAULT_OUTER_WIDTH,
                DEFAULT_ALIGN,
                SPACING_MULT_NO_SCALE,
                SPACING_ADD_NO_SCALE,
                true);
        assertEquals(fontMetricsInt.top - fontMetricsInt.ascent, dynamicLayout.getTopPadding());
        assertEquals(fontMetricsInt.bottom - fontMetricsInt.descent,
                dynamicLayout.getBottomPadding());

        dynamicLayout = new DynamicLayout(SINGLELINE_CHAR_SEQUENCE,
                mDefaultPaint,
                DEFAULT_OUTER_WIDTH,
                DEFAULT_ALIGN,
                SPACING_MULT_NO_SCALE,
                SPACING_ADD_NO_SCALE,
                false);
        assertEquals(0, dynamicLayout.getTopPadding());
        assertEquals(0, dynamicLayout.getBottomPadding());
    }

    /*
     * Test the line count and whether include the Tab the layout.
     * 1. Include Tab. 2. Don't include Tab Use the Y-coordinate to calculate the line number
     * Test the line top
     * 1. the Y-coordinate of line top.2. the Y-coordinate of baseline.
     */
    @Test
    public void testLineLayout() {
        assertEquals(TEXT.length, mDynamicLayout.getLineCount());
        assertFalse(mDynamicLayout.getLineContainsTab(LINE0));
        assertTrue(mDynamicLayout.getLineContainsTab(LINE1));

        assertEquals(LINE0_TOP, mDynamicLayout.getLineTop(LINE0));

        assertEquals(mDynamicLayout.getLineBottom(LINE0), mDynamicLayout.getLineTop(LINE1));
        assertEquals(mDynamicLayout.getLineBottom(LINE1), mDynamicLayout.getLineTop(LINE2));
        assertEquals(mDynamicLayout.getLineBottom(LINE2), mDynamicLayout.getLineTop(LINE3));

        try {
            assertEquals(mDynamicLayout.getLineBottom(mDynamicLayout.getLineCount()),
                    mDynamicLayout.getLineTop(mDynamicLayout.getLineCount() + 1));
            fail("Test DynamicLayout fail, should throw IndexOutOfBoundsException.");
        } catch (IndexOutOfBoundsException e) {
            // expected
        }

        assertEquals(mDynamicLayout.getLineDescent(LINE0) - mDynamicLayout.getLineAscent(LINE0),
                mDynamicLayout.getLineBottom(LINE0));

        assertEquals(mDynamicLayout.getLineDescent(LINE1) - mDynamicLayout.getLineAscent(LINE1),
                mDynamicLayout.getLineBottom(LINE1) - mDynamicLayout.getLineBottom(LINE0));

        assertEquals(mDynamicLayout.getLineDescent(LINE2) - mDynamicLayout.getLineAscent(LINE2),
                mDynamicLayout.getLineBottom(LINE2) - mDynamicLayout.getLineBottom(LINE1));

        assertEquals(LINE0, mDynamicLayout.getLineForVertical(mDynamicLayout.getLineTop(LINE0)));

        assertNotNull(mDynamicLayout.getLineDirections(LINE0));
        assertEquals(Layout.DIR_LEFT_TO_RIGHT, mDynamicLayout.getParagraphDirection(LINE0));

        assertEquals(0, mDynamicLayout.getLineStart(LINE0));
        assertEquals(TEXT[0].length(), mDynamicLayout.getLineStart(LINE1));
        assertEquals(TEXT[0].length() + TEXT[1].length(), mDynamicLayout.getLineStart(LINE2));
    }

    @Test
    public void testLineSpacing() {
        SpannableStringBuilder text = new SpannableStringBuilder("a\nb\nc");
        final float spacingMultiplier = 2f;
        final float spacingAdd = 4;
        final int width = 1000;
        final TextPaint textPaint = new TextPaint();
        // create the DynamicLayout
        final DynamicLayout dynamicLayout = new DynamicLayout(text,
                textPaint,
                width,
                ALIGN_NORMAL,
                spacingMultiplier,
                spacingAdd,
                false /*includepad*/);

        // create a StaticLayout with same text, this will define the expectations
        Layout expected = createStaticLayout(text.toString(), textPaint, width, spacingAdd,
                spacingMultiplier);
        assertLineSpecs(expected, dynamicLayout);

        // add a new line to the end, DynamicLayout will re-calculate
        text = text.append("\nd");
        expected = createStaticLayout(text.toString(), textPaint, width, spacingAdd,
                spacingMultiplier);
        assertLineSpecs(expected, dynamicLayout);

        // insert a next line and a char as the new second line
        text = text.insert(TextUtils.indexOf(text, '\n') + 1, "a1\n");
        expected = createStaticLayout(text.toString(), textPaint, width, spacingAdd,
                spacingMultiplier);
        assertLineSpecs(expected, dynamicLayout);
    }

    @Test
    public void testLineSpacing_textEndingWithNextLine() {
        final SpannableStringBuilder text = new SpannableStringBuilder("a\n");
        final float spacingMultiplier = 2f;
        final float spacingAdd = 4f;
        final int width = 1000;
        final TextPaint textPaint = new TextPaint();
        // create the DynamicLayout
        final DynamicLayout dynamicLayout = new DynamicLayout(text,
                textPaint,
                width,
                ALIGN_NORMAL,
                spacingMultiplier,
                spacingAdd,
                false /*includepad*/);

        // create a StaticLayout with same text, this will define the expectations
        final Layout expected = createStaticLayout(text.toString(), textPaint, width, spacingAdd,
                spacingMultiplier);
        assertLineSpecs(expected, dynamicLayout);
    }

    private Layout createStaticLayout(CharSequence text, TextPaint textPaint, int width,
            float spacingAdd, float spacingMultiplier) {
        return StaticLayout.Builder.obtain(text, 0,
                text.length(), textPaint, width)
                .setAlignment(ALIGN_NORMAL)
                .setIncludePad(false)
                .setLineSpacing(spacingAdd, spacingMultiplier)
                .build();
    }

    private void assertLineSpecs(Layout expected, DynamicLayout actual) {
        final int lineCount = expected.getLineCount();
        assertTrue(lineCount > 1);
        assertEquals(lineCount, actual.getLineCount());

        for (int i = 0; i < lineCount; i++) {
            assertEquals(expected.getLineTop(i), actual.getLineTop(i));
            assertEquals(expected.getLineDescent(i), actual.getLineDescent(i));
            assertEquals(expected.getLineBaseline(i), actual.getLineBaseline(i));
            assertEquals(expected.getLineBottom(i), actual.getLineBottom(i));
        }
    }

    @Test
    public void testLineSpacing_notAffectedByPreviousEllipsization() {
        // Create an ellipsized DynamicLayout, but throw it away.
        final String ellipsizedText = "Some arbitrary relatively long text";
        final DynamicLayout ellipsizedLayout = new DynamicLayout(
                ellipsizedText,
                ellipsizedText,
                mDefaultPaint,
                1 << 20 /* width */,
                DEFAULT_ALIGN,
                SPACING_MULT_NO_SCALE,
                SPACING_ADD_NO_SCALE,
                true /* include pad */,
                TextUtils.TruncateAt.END,
                2 * (int) mDefaultPaint.getTextSize() /* ellipsizedWidth */);

        // Now try to measure linespacing in a non-ellipsized DynamicLayout.
        final String text = "a\nb\nc";
        final float spacingMultiplier = 2f;
        final float spacingAdd = 4f;
        final int width = 1000;
        final TextPaint textPaint = new TextPaint();
        // create the DynamicLayout
        final DynamicLayout dynamicLayout = new DynamicLayout(text,
                textPaint,
                width,
                ALIGN_NORMAL,
                spacingMultiplier,
                spacingAdd,
                false /*includepad*/);

        // create a StaticLayout with same text, this will define the expectations
        Layout expected = createStaticLayout(text.toString(), textPaint, width, spacingAdd,
                spacingMultiplier);
        assertLineSpecs(expected, dynamicLayout);
    }

    @Test
    public void testBuilder_obtain() {
        final DynamicLayout.Builder builder = DynamicLayout.Builder.obtain(MULTLINE_CHAR_SEQUENCE,
                mDefaultPaint, DEFAULT_OUTER_WIDTH);
        final DynamicLayout layout = builder.build();
        // Check values passed to obtain().
        assertEquals(MULTLINE_CHAR_SEQUENCE, layout.getText());
        assertEquals(mDefaultPaint, layout.getPaint());
        assertEquals(DEFAULT_OUTER_WIDTH, layout.getWidth());
        // Check default values.
        assertEquals(Layout.Alignment.ALIGN_NORMAL, layout.getAlignment());
        assertEquals(0.0f, layout.getSpacingAdd(), 0.0f);
        assertEquals(1.0f, layout.getSpacingMultiplier(), 0.0f);
        assertEquals(DEFAULT_OUTER_WIDTH, layout.getEllipsizedWidth());
    }

    @Test(expected = NullPointerException.class)
    public void testBuilder_obtainWithNullText() {
        final DynamicLayout.Builder builder = DynamicLayout.Builder.obtain(null, mDefaultPaint, 0);
        final DynamicLayout layout = builder.build();
    }

    @Test(expected = NullPointerException.class)
    public void testBuilder_obtainWithNullPaint() {
        final DynamicLayout.Builder builder = DynamicLayout.Builder.obtain(MULTLINE_CHAR_SEQUENCE,
                null, 0);
        final DynamicLayout layout = builder.build();
    }

    @Test
    public void testBuilder_setDisplayTest() {
        final DynamicLayout.Builder builder = DynamicLayout.Builder.obtain(MULTLINE_CHAR_SEQUENCE,
                mDefaultPaint, DEFAULT_OUTER_WIDTH);
        builder.setDisplayText(SINGLELINE_CHAR_SEQUENCE);
        final DynamicLayout layout = builder.build();
        assertEquals(SINGLELINE_CHAR_SEQUENCE, layout.getText());
    }

    @Test
    public void testBuilder_setAlignment() {
        final DynamicLayout.Builder builder = DynamicLayout.Builder.obtain(MULTLINE_CHAR_SEQUENCE,
                mDefaultPaint, DEFAULT_OUTER_WIDTH);
        builder.setAlignment(DEFAULT_ALIGN);
        final DynamicLayout layout = builder.build();
        assertEquals(DEFAULT_ALIGN, layout.getAlignment());
    }

    @Test
    public void testBuilder_setLineSpacing() {
        final DynamicLayout.Builder builder = DynamicLayout.Builder.obtain(MULTLINE_CHAR_SEQUENCE,
                mDefaultPaint, DEFAULT_OUTER_WIDTH);
        builder.setLineSpacing(1.0f, 2.0f);
        final DynamicLayout layout = builder.build();
        assertEquals(1.0f, layout.getSpacingAdd(), 0.0f);
        assertEquals(2.0f, layout.getSpacingMultiplier(), 0.0f);
    }

    @Test
    public void testBuilder_ellipsization() {
        final DynamicLayout.Builder builder = DynamicLayout.Builder.obtain(MULTLINE_CHAR_SEQUENCE,
                mDefaultPaint, DEFAULT_OUTER_WIDTH);
        builder.setEllipsize(TextUtils.TruncateAt.END)
                .setEllipsizedWidth(ELLIPSIZE_WIDTH);
        final DynamicLayout layout = builder.build();
        assertEquals(ELLIPSIZE_WIDTH, layout.getEllipsizedWidth());
        assertEquals(DEFAULT_OUTER_WIDTH, layout.getWidth());
        for (int i = 0; i < TEXT.length; i++) {
            if (i == TEXT.length - 1) { // last line
                assertTrue(layout.getEllipsisCount(i) > 0);
            } else {
                assertEquals(0, layout.getEllipsisCount(i));
            }
        }
    }

    @Test
    public void testBuilder_otherSetters() {
        // Setter methods that cannot be directly tested.
        // setBreakStrategy, setHyphenationFrequency, setIncludePad, and setJustificationMode.
        final DynamicLayout.Builder builder = DynamicLayout.Builder.obtain(MULTLINE_CHAR_SEQUENCE,
                mDefaultPaint, DEFAULT_OUTER_WIDTH);
        builder.setBreakStrategy(Layout.BREAK_STRATEGY_HIGH_QUALITY)
                .setHyphenationFrequency(Layout.HYPHENATION_FREQUENCY_FULL)
                .setIncludePad(true)
                .setJustificationMode(Layout.JUSTIFICATION_MODE_INTER_WORD);
        final DynamicLayout layout = builder.build();
        assertNotNull(layout);
    }

    @Test
    public void testReflow_afterSpanChangedShouldNotThrowException() {
        final SpannableStringBuilder builder = new SpannableStringBuilder("crash crash crash!!");

        final TypefaceSpan span = mock(TypefaceSpan.class);
        builder.setSpan(span, 1, 4, SPAN_EXCLUSIVE_EXCLUSIVE);

        final DynamicLayout layout = DynamicLayout.Builder.obtain(builder,
                new TextPaint(), Integer.MAX_VALUE).build();
        try {
            builder.insert(1, "Hello there\n\n");
        } catch (Throwable e) {
            throw new RuntimeException("Inserting text into DynamicLayout should not crash", e);
        }
    }

}
