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

package android.widget.cts;

import static org.junit.Assert.assertEquals;

import android.support.test.filters.SmallTest;
import android.support.test.runner.AndroidJUnit4;
import android.text.SpannableString;
import android.text.Spanned;
import android.text.style.UnderlineSpan;
import android.widget.MultiAutoCompleteTextView;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;

@SmallTest
@RunWith(AndroidJUnit4.class)
public class MultiAutoCompleteTextView_CommaTokenizerTest {
    private static final String TEST_TEXT = "first token, second token";
    MultiAutoCompleteTextView.CommaTokenizer mCommaTokenizer;

    @Before
    public void setup() {
        mCommaTokenizer = new MultiAutoCompleteTextView.CommaTokenizer();
    }

    @Test
    public void testConstructor() {
        new MultiAutoCompleteTextView.CommaTokenizer();
    }

    @Test
    public void testFindTokenStart() {
        int indexOfSecondToken = TEST_TEXT.indexOf("second");
        assertEquals(indexOfSecondToken,
                mCommaTokenizer.findTokenStart(TEST_TEXT, indexOfSecondToken));
        // cursor point to the space before "second".
        assertEquals(indexOfSecondToken - 1,
                mCommaTokenizer.findTokenStart(TEST_TEXT, indexOfSecondToken - 1));
        assertEquals(indexOfSecondToken,
                mCommaTokenizer.findTokenStart(TEST_TEXT, TEST_TEXT.length()));

        // cursor point to the comma before "second"
        assertEquals(0, mCommaTokenizer.findTokenStart(TEST_TEXT, indexOfSecondToken - 2));
        assertEquals(0, mCommaTokenizer.findTokenStart(TEST_TEXT, 1));

        assertEquals(-1, mCommaTokenizer.findTokenStart(TEST_TEXT, -1));
        assertEquals(-2, mCommaTokenizer.findTokenStart(TEST_TEXT, -2));
    }

    @Test(expected=IndexOutOfBoundsException.class)
    public void testFindTokenStartInvalidCursor() {
        mCommaTokenizer.findTokenStart(TEST_TEXT, TEST_TEXT.length() + 1);
    }

    @Test(expected=NullPointerException.class)
    public void testFindTokenStartNullText() {
        mCommaTokenizer.findTokenStart(null, TEST_TEXT.length());
    }

    @Test
    public void testFindTokenEnd() {
        int indexOfComma = TEST_TEXT.indexOf(",");

        assertEquals(indexOfComma, mCommaTokenizer.findTokenEnd(TEST_TEXT, 0));
        assertEquals(indexOfComma, mCommaTokenizer.findTokenEnd(TEST_TEXT, indexOfComma - 1));
        assertEquals(indexOfComma, mCommaTokenizer.findTokenEnd(TEST_TEXT, indexOfComma));

        assertEquals(TEST_TEXT.length(),
                mCommaTokenizer.findTokenEnd(TEST_TEXT, indexOfComma + 1));
        assertEquals(TEST_TEXT.length(),
                mCommaTokenizer.findTokenEnd(TEST_TEXT, TEST_TEXT.length()));
        assertEquals(TEST_TEXT.length(),
                mCommaTokenizer.findTokenEnd(TEST_TEXT, TEST_TEXT.length() + 1));
    }

    @Test(expected=IndexOutOfBoundsException.class)
    public void testFindTokenEndInvalidCursor() {
        mCommaTokenizer.findTokenEnd(TEST_TEXT, -1);
    }

    @Test(expected=NullPointerException.class)
    public void testFindTokenEndNullText() {
        mCommaTokenizer.findTokenEnd(null, 1);
    }

    @Test
    public void testTerminateToken() {
        String text = "end with comma,";
        assertEquals(text, mCommaTokenizer.terminateToken(text));

        text = "end without comma";
        // ends with both comma and space
        assertEquals(text + ", ", mCommaTokenizer.terminateToken(text));

        text = "end without comma!";
        // ends with both comma and space
        assertEquals(text + ", ", mCommaTokenizer.terminateToken(text));

        text = "has ending spaces   ";
        assertEquals(text + ", ", mCommaTokenizer.terminateToken(text));

        text = "has ending spaces ,   ";
        assertEquals(text, mCommaTokenizer.terminateToken(text));

        text = "test Spanned text";
        SpannableString spannableString = new SpannableString(text);
        SpannableString expected = new SpannableString(text + ", ");
        UnderlineSpan what = new UnderlineSpan();
        spannableString.setSpan(what, 0, spannableString.length(),
                Spanned.SPAN_INCLUSIVE_INCLUSIVE);

        SpannableString actual = (SpannableString) mCommaTokenizer.terminateToken(spannableString);
        assertEquals(expected.toString(), actual.toString());
        assertEquals(0, actual.getSpanStart(what));
        assertEquals(spannableString.length(), actual.getSpanEnd(what));
    }

    @Test(expected=NullPointerException.class)
    public void testTerminateTokenNullText() {
        mCommaTokenizer.terminateToken(null);
    }
}
