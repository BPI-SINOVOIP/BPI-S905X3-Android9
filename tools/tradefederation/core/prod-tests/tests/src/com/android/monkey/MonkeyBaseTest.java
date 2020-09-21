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

package com.android.monkey;

import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.util.ArrayUtil;

import junit.framework.TestCase;

import org.easymock.EasyMock;

import java.util.ArrayList;
import java.util.Collection;

/**
 * Unit tests for {@link MonkeyBase}
 */
public class MonkeyBaseTest extends TestCase {

    /**
     * Test that {@link MonkeyBase#setSubtract(Collection, Collection)} returns same object if
     * exclude is empty.
     */
    public void testSetSubtract_noExclude() {
        Collection<String> haystack = ArrayUtil.list("a", "b", "c");
        Collection<String> needles = new ArrayList<String>();
        // double-checking comparison assumptions
        assertFalse(haystack == needles);
        Collection<String> output = MonkeyBase.setSubtract(haystack, needles);
        assertTrue(haystack == output);
    }

    /**
     * Test that {@link MonkeyBase#setSubtract(Collection, Collection)} returns the set subtraction
     * if exclude is not empty.
     */
    public void testSetSubtract() {
        Collection<String> haystack = ArrayUtil.list("a", "b", "c");
        Collection<String> needles = ArrayUtil.list("b");
        Collection<String> output = MonkeyBase.setSubtract(haystack, needles);
        assertEquals(2, output.size());
        assertTrue(output.contains("a"));
        assertFalse(output.contains("b"));
        assertTrue(output.contains("c"));
    }

    /**
     * Test success case for {@link MonkeyBase#getUptime()}.
     */
    public void testUptime() throws Exception {
        MonkeyBase monkey = new MonkeyBase();
        ITestDevice mockDevice = EasyMock.createMock(ITestDevice.class);
        monkey.setDevice(mockDevice);
        EasyMock.expect(mockDevice.executeShellCommand("cat /proc/uptime")).andReturn(
                "5278.73 1866.80");
        EasyMock.replay(mockDevice);
        assertEquals("5278.73", monkey.getUptime());
    }

    /**
     * Test case for {@link MonkeyBase#getUptime()} where device is initially unresponsive.
     */
    public void testUptime_fail() throws Exception {
        MonkeyBase monkey = new MonkeyBase();
        ITestDevice mockDevice = EasyMock.createMock(ITestDevice.class);
        monkey.setDevice(mockDevice);
        EasyMock.expect(mockDevice.getSerialNumber()).andStubReturn("serial");
        EasyMock.expect(mockDevice.executeShellCommand("cat /proc/uptime")).andReturn(
                "");
        EasyMock.expect(mockDevice.executeShellCommand("cat /proc/uptime")).andReturn(
                "5278.73 1866.80");
        EasyMock.replay(mockDevice);
        assertEquals("5278.73", monkey.getUptime());
    }
}

