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

package android.view.inputmethod.cts;

import static org.junit.Assert.assertEquals;

import android.graphics.Typeface;
import android.os.Parcel;
import android.support.test.filters.SmallTest;
import android.support.test.runner.AndroidJUnit4;
import android.text.Spannable;
import android.text.SpannableStringBuilder;
import android.text.Spanned;
import android.text.style.StyleSpan;
import android.view.inputmethod.ExtractedText;

import org.junit.Test;
import org.junit.runner.RunWith;

@SmallTest
@RunWith(AndroidJUnit4.class)
public class ExtractedTextTest {
    @Test
    public void testWriteToParcel() {
        ExtractedText extractedText = new ExtractedText();
        extractedText.flags = 1;
        extractedText.selectionEnd = 11;
        extractedText.selectionStart = 2;
        extractedText.startOffset = 1;
        CharSequence text = "test";
        extractedText.text = text;
        SpannableStringBuilder hint = new SpannableStringBuilder("hint");
        hint.setSpan(new StyleSpan(Typeface.BOLD), 1, 3, Spanned.SPAN_INCLUSIVE_INCLUSIVE);
        extractedText.hint = hint;
        Parcel p = Parcel.obtain();
        extractedText.writeToParcel(p, 0);
        p.setDataPosition(0);
        ExtractedText target = ExtractedText.CREATOR.createFromParcel(p);
        assertEquals(extractedText.flags, target.flags);
        assertEquals(extractedText.selectionEnd, target.selectionEnd);
        assertEquals(extractedText.selectionStart, target.selectionStart);
        assertEquals(extractedText.startOffset, target.startOffset);
        assertEquals(extractedText.partialStartOffset, target.partialStartOffset);
        assertEquals(extractedText.partialEndOffset, target.partialEndOffset);
        assertEquals(extractedText.text.toString(), target.text.toString());
        assertEquals(extractedText.hint.toString(), target.hint.toString());
        final Spannable hintText = (Spannable) extractedText.hint;
        assertEquals(1, hintText.getSpans(0, hintText.length(), StyleSpan.class).length);

        assertEquals(0, extractedText.describeContents());

        p.recycle();
    }
}
