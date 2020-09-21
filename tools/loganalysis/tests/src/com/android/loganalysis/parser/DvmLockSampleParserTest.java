/*
 * Copyright (C) 2014 The Android Open Source Project
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
package com.android.loganalysis.parser;

import com.android.loganalysis.item.DvmLockSampleItem;
import com.android.loganalysis.parser.DvmLockSampleParser;

import junit.framework.TestCase;
import java.util.Arrays;
import java.util.List;

public class DvmLockSampleParserTest extends TestCase {

    /**
     * Sanity-check our DVM lock line parser against a single log line
     */
    public void testSingleDvmLine() {
        List<String> input = Arrays.asList(
            "[android.support.test.aupt,0,Instr: android.support.test.aupt,75," +
            "AccessibilityCache.java,256,-,96,15]");

        DvmLockSampleItem item = new DvmLockSampleParser().parse(input);
        assertEquals("android.support.test.aupt", item.getAttribute(DvmLockSampleItem.PROCESS_NAME));
        assertEquals(Boolean.FALSE, item.getAttribute(DvmLockSampleItem.SENSITIVITY_FLAG));
        assertEquals("Instr: android.support.test.aupt", item.getAttribute(DvmLockSampleItem.WAITING_THREAD_NAME));
        assertEquals(75, item.getAttribute(DvmLockSampleItem.WAIT_TIME));
        assertEquals("AccessibilityCache.java", item.getAttribute(DvmLockSampleItem.WAITING_SOURCE_FILE));
        assertEquals(256, item.getAttribute(DvmLockSampleItem.WAITING_SOURCE_LINE));
        assertEquals("AccessibilityCache.java", item.getAttribute(DvmLockSampleItem.OWNER_FILE_NAME));
        assertEquals(96, item.getAttribute(DvmLockSampleItem.OWNER_ACQUIRE_SOURCE_LINE));
        assertEquals(15, item.getAttribute(DvmLockSampleItem.SAMPLE_PERCENTAGE));
    }
}
