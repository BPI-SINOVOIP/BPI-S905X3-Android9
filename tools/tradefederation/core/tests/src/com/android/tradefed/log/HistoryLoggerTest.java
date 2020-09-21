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
package com.android.tradefed.log;

import com.android.ddmlib.Log.LogLevel;
import com.android.tradefed.log.ILogRegistry.EventType;

import org.junit.After;
import org.junit.Assert;
import org.junit.Test;

import java.io.IOException;
import java.util.HashMap;
import java.util.Map;

/** Unit tests for {@link HistoryLogger}. */
public class HistoryLoggerTest {

    private HistoryLogger mLogger;
    private boolean mWasCalled = false;

    @After
    public void tearDown() {
        mWasCalled = false;
        if (mLogger != null) {
            mLogger.closeLog();
        }
    }

    /**
     * Test for {@link HistoryLogger#logEvent(LogLevel, EventType, Map)} logging the expected
     * values.
     */
    @Test
    public void testLogEvent() throws Exception {
        Map<String, String> expectedArgs = new HashMap<>();
        expectedArgs.put("test", "value");
        mWasCalled = false;
        mLogger =
                new HistoryLogger() {
                    @Override
                    void writeToLog(String outMessage) throws IOException {
                        Assert.assertEquals("INVOCATION_END: {\"test\":\"value\"}\n", outMessage);
                        mWasCalled = true;
                    }
                };
        mLogger.init();
        mLogger.logEvent(LogLevel.INFO, EventType.INVOCATION_END, expectedArgs);
        Assert.assertTrue(mWasCalled);
    }

    /**
     * Test for {@link HistoryLogger#logEvent(LogLevel, EventType, Map)} logging the expected
     * values.
     */
    @Test
    public void testLogEvent_nullArgs() throws Exception {
        mWasCalled = false;
        mLogger =
                new HistoryLogger() {
                    @Override
                    void writeToLog(String outMessage) throws IOException {
                        Assert.assertEquals("INVOCATION_END: {}\n", outMessage);
                        mWasCalled = true;
                    }
                };
        mLogger.init();
        mLogger.logEvent(LogLevel.INFO, EventType.INVOCATION_END, null);
        Assert.assertTrue(mWasCalled);
    }
}
