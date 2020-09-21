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

package com.android.media.tests;

import junit.framework.TestCase;

import java.util.regex.Matcher;


public class MediaTest extends TestCase {
    private String mPositiveOutput = "testStressAddRemoveEffects diff :  472";
    private String mNegativeOutput = "testStressAddRemoveEffects diff :  -123";

    private String mExpectedTestCaseName = "testStressAddRemoveEffects";
    private String mExpectedPositiveOut = "472";
    private String mExpectedNegativeOut = "-123";

    public void testVideoEditorPositiveOut() {
        Matcher m = VideoEditingMemoryTest.TOTAL_MEM_DIFF_PATTERN.matcher(mPositiveOutput);
        assertTrue(m.matches());
        assertEquals("Parse test case name", mExpectedTestCaseName, m.group(1));
        assertEquals("Paser positive out", mExpectedPositiveOut, m.group(2));
    }

    public void testVideoEditorNegativeOut() {
        Matcher m = VideoEditingMemoryTest.TOTAL_MEM_DIFF_PATTERN.matcher(mNegativeOutput);
        assertTrue(m.matches());
        assertEquals("Parse test case name", mExpectedTestCaseName, m.group(1));
        assertEquals("Paser negative out", mExpectedNegativeOut, m.group(2));
    }
}
