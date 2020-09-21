/*
 * Copyright (C) 2013 The Android Open Source Project
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
package com.android.loganalysis.item;

import junit.framework.TestCase;

import org.json.JSONException;
import org.json.JSONObject;

/**
 * Unit test for {@link DvmLockSampleItem}.
 */
public class DvmLockSampleItemTest extends TestCase {
    /**
     * Test that {@link DvmLockSampleItem#toJson()} returns correctly.
     */
    public void testToJson() throws JSONException {
        DvmLockSampleItem item = new DvmLockSampleItem();

        item.setAttribute(DvmLockSampleItem.PROCESS_NAME, "android.support.test.aupt");
        item.setAttribute(DvmLockSampleItem.SENSITIVITY_FLAG, false);
        item.setAttribute(DvmLockSampleItem.WAITING_THREAD_NAME, "Instr: android.support.test.aupt");
        item.setAttribute(DvmLockSampleItem.WAIT_TIME, 75);
        item.setAttribute(DvmLockSampleItem.WAITING_SOURCE_FILE, "AccessibilityCache.java");
        item.setAttribute(DvmLockSampleItem.WAITING_SOURCE_LINE, 256);
        item.setAttribute(DvmLockSampleItem.OWNER_FILE_NAME, "-");
        item.setAttribute(DvmLockSampleItem.OWNER_ACQUIRE_SOURCE_LINE, 96);
        item.setAttribute(DvmLockSampleItem.SAMPLE_PERCENTAGE, 15);

        // Convert to JSON string and back again
        JSONObject output = new JSONObject(item.toJson().toString());

        // Assert that each field is the expected value
        assertEquals("android.support.test.aupt", output.get(DvmLockSampleItem.PROCESS_NAME));
        assertEquals(false, output.get(DvmLockSampleItem.SENSITIVITY_FLAG));
        assertEquals("Instr: android.support.test.aupt", output.get(DvmLockSampleItem.WAITING_THREAD_NAME));
        assertEquals(75, output.get(DvmLockSampleItem.WAIT_TIME));
        assertEquals("AccessibilityCache.java", output.get(DvmLockSampleItem.WAITING_SOURCE_FILE));
        assertEquals(256, output.get(DvmLockSampleItem.WAITING_SOURCE_LINE));
        assertEquals("-", output.get(DvmLockSampleItem.OWNER_FILE_NAME));
        assertEquals(96, output.get(DvmLockSampleItem.OWNER_ACQUIRE_SOURCE_LINE));
        assertEquals(15, output.get(DvmLockSampleItem.SAMPLE_PERCENTAGE));
    }
}
