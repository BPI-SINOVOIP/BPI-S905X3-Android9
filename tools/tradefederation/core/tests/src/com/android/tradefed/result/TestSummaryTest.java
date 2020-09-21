/*
 * Copyright (C) 2010 The Android Open Source Project
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
package com.android.tradefed.result;

import com.android.tradefed.result.TestSummary.TypedString;

import junit.framework.TestCase;

import java.util.Map;

/**
 * Unit tests for {@link TestSummary}
 * A class to represent a test summary.  Provides a single field for a summary, in addition to a
 * key-value store for more detailed summary.  Also provides a place for the summary source to be
 * identified.
 */
public class TestSummaryTest extends TestCase {
    public void testSimpleCreate() {
        final TypedString sumText = new TypedString("file:///path/to/summary.txt");
        final TypedString httpText = new TypedString("http://summary.com/also/a/summary.html");
        final String className = this.getClass().getName();

        TestSummary summary = new TestSummary(sumText);
        summary.addKvEntry("httpText", httpText);
        summary.setSource(className);

        assertEquals(summary.getSummary(), sumText);
        assertEquals(summary.getSource(), className);
        Map<String, TypedString> kv = summary.getKvEntries();
        assertEquals(kv.size(), 1);
        assertEquals(kv.get("httpText"), httpText);
    }
}
