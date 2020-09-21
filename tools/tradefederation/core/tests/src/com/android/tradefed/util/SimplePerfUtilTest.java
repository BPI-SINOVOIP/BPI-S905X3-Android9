/*
 * Copyright (C) 2016 The Android Open Source Project
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
package com.android.tradefed.util;

import com.android.tradefed.device.ITestDevice;

import com.android.tradefed.util.SimplePerfUtil.SimplePerfType;

import junit.framework.TestCase;

import org.easymock.EasyMock;
import org.junit.Assert;

import java.util.ArrayList;
import java.util.List;

/**
 * Unit tests for SimplePerfUtil
 */
public class SimplePerfUtilTest extends TestCase {

    private SimplePerfUtil spu = null;
    private ITestDevice mockDevice = null;

    @Override
    protected void setUp() throws Exception {
        super.setUp();
        mockDevice = EasyMock.createMock(ITestDevice.class);
    }

    public void testCommandStringPreparerWithoutParams() {
        spu = SimplePerfUtil.newInstance(mockDevice, SimplePerfType.STAT);
        String command = "ls -l";
        String fullCommand = spu.commandStringPreparer(command);
        Assert.assertEquals("simpleperf stat " + command, fullCommand);
    }

    public void testCommandStringPreparerWithParams() {
        spu = SimplePerfUtil.newInstance(mockDevice, SimplePerfType.RECORD);
        String command = "sleep 10";
        // Use LinkedHashMap to maintain order
        List<String> params = new ArrayList<>();
        params.add("-e cpu-cycles:k");
        params.add("--no-inherit");
        params.add("perf.data");
        spu.setArgumentList(params);
        String fullCommand = spu.commandStringPreparer(command);
        Assert.assertEquals("simpleperf record -e cpu-cycles:k --no-inherit perf.data " + command,
                fullCommand);
    }

    public void testCommandStringPreparerWithNullCommand() {
        spu = SimplePerfUtil.newInstance(mockDevice, SimplePerfType.LIST);
        String fullCommand = spu.commandStringPreparer(null);
        Assert.assertEquals("simpleperf list ", fullCommand);
    }

    public void testConstructorWithNullParameter() {
        try {
            spu = SimplePerfUtil.newInstance(null, SimplePerfType.STAT);
            fail("Missing Exception");
        } catch (NullPointerException npe) {
            Assert.assertTrue(true);
        }
    }
}
