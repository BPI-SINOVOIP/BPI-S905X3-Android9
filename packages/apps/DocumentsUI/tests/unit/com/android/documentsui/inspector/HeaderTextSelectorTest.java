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
package com.android.documentsui.inspector;

import static junit.framework.Assert.assertEquals;
import static junit.framework.Assert.assertTrue;

import android.content.Context;
import android.support.test.InstrumentationRegistry;
import android.support.test.runner.AndroidJUnit4;

import android.test.suitebuilder.annotation.SmallTest;
import android.text.Spannable;
import android.text.SpannableString;
import android.widget.TextView;
import com.android.documentsui.inspector.HeaderTextSelector.Selector;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;

@RunWith(AndroidJUnit4.class)
@SmallTest
public class HeaderTextSelectorTest {

    private Context mContext;
    private TestTextView mTestTextView;
    private TestSelector mTestSelector;
    private HeaderTextSelector mSelector;

    @Before
    public void setUp() throws Exception {
        mContext = InstrumentationRegistry.getTargetContext();
        mTestTextView = new TestTextView(mContext);
        mTestSelector = new TestSelector();
        mSelector = new HeaderTextSelector(mTestTextView, mTestSelector);
    }

    @Test
    public void testSimpleFileName() throws Exception {
        CharSequence fileName = "filename";
        String extension = ".txt";
        String testSequence = fileName + extension;

        mTestTextView.setText(testSequence);
        mSelector.onCreateActionMode(null, null);
        mTestSelector.assertCalled();

        CharSequence selectedSequence =
                testSequence.subSequence(mTestSelector.mStart, mTestSelector.mStop);
        assertEquals(selectedSequence, fileName);
    }


    @Test
    public void testLotsOfPeriodsInFileName() throws Exception {
        CharSequence fileName = "filename";
        String extension = ".jpg.zip.pdf.txt";
        String testSequence = fileName + extension;

        mTestTextView.setText(testSequence);
        mSelector.onCreateActionMode(null, null);
        mTestSelector.assertCalled();

        CharSequence selectedSequence =
            testSequence.subSequence(mTestSelector.mStart, mTestSelector.mStop);
        assertEquals(selectedSequence, fileName);
    }

    @Test
    public void testNoPeriodsInFileName() throws Exception {
        String testSequence = "filename";

        mTestTextView.setText(testSequence);
        mSelector.onCreateActionMode(null, null);
        mTestSelector.assertCalled();

        CharSequence selectedSequence =
            testSequence.subSequence(mTestSelector.mStart, mTestSelector.mStop);
        assertEquals(selectedSequence, testSequence);
    }

    private static class TestTextView extends TextView {

        public TestTextView(Context context) {
            super(context);
        }

        private String textViewString;

        public void setText(String text) {
            textViewString = text;
        }

        @Override
        public CharSequence getText() {
            return new SpannableString(textViewString);
        }
    }

    private static class TestSelector implements Selector {

        private boolean mCalled;
        private int mStart;
        private int mStop;

        public TestSelector() {
            mCalled = false;
            mStart = -1;
            mStop = -1;
        }

        @Override
        public void select(Spannable text, int start, int stop) {
            mCalled = true;
            mStart = start;
            mStop = stop;
        }

        public void assertCalled() {
            assertTrue(mCalled);
        }
    }
}