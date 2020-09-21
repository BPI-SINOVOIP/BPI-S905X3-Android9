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

import com.android.tradefed.device.ILogcatReceiver;
import com.android.tradefed.result.InputStreamSource;
import com.android.tradefed.util.LogcatUpdaterEventParser.AsyncUpdaterEvent;

import junit.framework.TestCase;

import org.easymock.EasyMock;

import java.io.IOException;
import java.io.InputStream;
import java.io.PipedInputStream;
import java.io.PipedOutputStream;

/**
 * Unit tests for {@link LogcatUpdaterEventParser}.
 */
public class LogcatUpdaterEventParserTest extends TestCase {

    private static final String[] LOGS_UPDATE_COMPLETE = {
        "11-11 00:00:00.001  123 321 I update_engine: foo bar baz\n",
        "11-11 00:00:00.001  123 321 I update_engine: foo bar baz\n",
        "11-11 00:00:00.001  123 321 I update_engine: foo bar baz\n",
        "11-11 00:00:00.001  123 321 I update_engine: foo bar baz\n",
        "11-11 00:00:00.001  123 321 I update_engine: Update successfully applied\n",
    };
    private static final String[] LOGS_ERROR = {
        "11-11 00:00:00.001  123 321 I update_engine: foo bar baz\n",
        "11-11 00:00:00.001  123 321 I update_engine: foo bar baz\n",
        "11-11 00:00:00.001  123 321 I update_engine: foo bar baz\n",
        "11-11 00:00:00.001  123 321 I update_engine: foo bar baz\n",
        "11-11 00:00:00.001  123 321 I update_engine_client: "
                + "onPayloadApplicationComplete(ErrorCode::kPayloadTimestampError (51))\n",
    };
    private static final String[] LOGS_ERROR_FLAKY = {
        "11-11 00:00:00.001  123 321 I update_engine: "
                + "ActionProcessor: Aborting processing due to failure.\n",
        "11-11 00:00:00.001  123 321 I update_engine_client: "
                + "onPayloadApplicationComplete(ErrorCode::kNewRootfsVerificationError (15))\n",
    };
    private static final long EVENT_TIMEOUT_MS = 5 * 1000L;
    private static final long THREAD_TIMEOUT_MS = 5 * 1000L;

    private ILogcatReceiver mMockReceiver = null;
    private LogcatUpdaterEventParser mParser = null;
    private PipedOutputStream mMockPipe = null;

    @Override
    public void setUp() {
        mMockReceiver = EasyMock.createMock(ILogcatReceiver.class);
        mMockPipe = new PipedOutputStream();
        final PipedInputStream p = new PipedInputStream();
        try {
            mMockPipe.connect(p);
        } catch (IOException e) {
            throw new RuntimeException(e);
        }

        EasyMock.expect(mMockReceiver.getLogcatData())
                .andReturn(
                        new InputStreamSource() {
                            @Override
                            public InputStream createInputStream() {
                                return p;
                            }

                            @Override
                            public void close() {
                                // ignore
                            }

                            @Override
                            public long size() {
                                return 0;
                            }
                        });

        EasyMock.replay(mMockReceiver);
        mParser = new LogcatUpdaterEventParser(mMockReceiver);
    }

    /**
     * Test that a known event parses to the expected {@link UpdaterEventType} when there is an
     * exact match.
     */
    public void testParseEventTypeExactMatch() {
        String mappedLogLine =
                "11-11 00:00:00.001  123 321 I update_verifier: Leaving update_verifier.";
        assertEquals(
                UpdaterEventType.UPDATE_VERIFIER_COMPLETE, mParser.parseEventType(mappedLogLine));
    }

    /**
     * Test that a known event parses to the expected {@link UpdaterEventType} when the registered
     * message is a substring of the log message.
     */
    public void testParseEventTypePartialMatch() {
        String log =
                "01-09 17:06:50.799  8688  8688 I update_engine_client: "
                        + "onPayloadApplicationComplete(ErrorCode::kUserCanceled (48))";
        assertEquals(UpdaterEventType.ERROR, mParser.parseEventType(log));
    }

    /** Test that unknown events parse to null. */
    public void testParseEventTypeUnknown() {
        String unmappedLogLine = "11-11 00:00:00.001  123 321 I update_engine: foo bar baz";
        assertEquals(null, mParser.parseEventType(unmappedLogLine));
        unmappedLogLine = "11-11 00:00:00.001  123 321 I foobar_engine: Update succeeded";
        assertEquals(null, mParser.parseEventType(unmappedLogLine));
        unmappedLogLine = "11-11 00:00:00.001  123 321 I foobar_engine: foo bar baz";
        assertEquals(null, mParser.parseEventType(unmappedLogLine));
    }

    /** Test that parser recognize update engine error message */
    public void testParseEventTypeError() {
        String error1 =
                "01-09 17:06:50.799  8688  8688 I update_engine_client: "
                        + "onPayloadApplicationComplete(ErrorCode::kUserCanceled (48))";
        assertEquals(UpdaterEventType.ERROR, mParser.parseEventType(error1));
    }

    /** Test that events registered first are matched first */
    public void testParseEventTypeMatchOrder() {
        mParser.registerEventTrigger(
                "update_engine",
                "finished with ErrorCode::kSuccess",
                UpdaterEventType.PATCH_COMPLETE);
        mParser.registerEventTrigger(
                "update_engine", "finished with ErrorCode", UpdaterEventType.ERROR);
        String notError =
                "11-11 00:00:00.001  123 321 I update_engine: finished with ErrorCode::kSuccess";
        assertEquals(UpdaterEventType.PATCH_COMPLETE, mParser.parseEventType(notError));
    }

    /** Test that waitForEvent returns once it sees a specific expect event. */
    public void testWaitForEvent() throws Exception {
        feedMockPipe(LOGS_UPDATE_COMPLETE);
        UpdaterEventType result =
                mParser.waitForEvent(UpdaterEventType.UPDATE_COMPLETE, EVENT_TIMEOUT_MS);
        assertEquals(UpdaterEventType.UPDATE_COMPLETE, result);
    }

    /** Test that waitForEvent returns when it sees an update error. */
    public void testWaitForEventError() throws Exception {
        feedMockPipe(LOGS_ERROR);
        UpdaterEventType event =
                mParser.waitForEvent(UpdaterEventType.UPDATE_COMPLETE, EVENT_TIMEOUT_MS);
        assertEquals(UpdaterEventType.ERROR, event);
    }

    /** Test that waitForEvent returns when it sees an flaky update error. */
    public void testWaitForEventErrorFlaky() throws Exception {
        feedMockPipe(LOGS_ERROR_FLAKY);
        UpdaterEventType event =
                mParser.waitForEvent(UpdaterEventType.UPDATE_COMPLETE, EVENT_TIMEOUT_MS);
        assertEquals(UpdaterEventType.ERROR_FLAKY, event);
    }

    /** Test that waitForEvent honors the timeout. */
    public void testWaitForEventTimeout() throws Exception {
        feedMockPipe(LOGS_UPDATE_COMPLETE);
        UpdaterEventType event = mParser.waitForEvent(UpdaterEventType.UPDATE_COMPLETE, 0);
        assertEquals(UpdaterEventType.INFRA_TIMEOUT, event);
    }

    /** Test that waitForEventAsync completes when it sees a specific expect event. */
    public void testWaitForEventAsync() throws Exception {
        AsyncUpdaterEvent event =
                mParser.waitForEventAsync(UpdaterEventType.UPDATE_COMPLETE, EVENT_TIMEOUT_MS);
        feedMockPipe(LOGS_UPDATE_COMPLETE);
        event.waitUntilCompleted(THREAD_TIMEOUT_MS);
        assertEquals(UpdaterEventType.UPDATE_COMPLETE, event.getResult());
    }

    /** Test that waitForEventAsync completes when it sees an update error. */
    public void testWaitForEventAsyncError() throws Exception {
        AsyncUpdaterEvent event =
                mParser.waitForEventAsync(UpdaterEventType.UPDATE_COMPLETE, EVENT_TIMEOUT_MS);
        feedMockPipe(LOGS_ERROR);
        event.waitUntilCompleted(THREAD_TIMEOUT_MS);
        assertEquals(UpdaterEventType.ERROR, event.getResult());
    }

    /** Test that waitForEventAsync honors the timeout. */
    public void testWaitForEventAsyncTimeout() throws Exception {
        AsyncUpdaterEvent event = mParser.waitForEventAsync(UpdaterEventType.UPDATE_COMPLETE, 0);
        feedMockPipe(LOGS_UPDATE_COMPLETE);
        event.waitUntilCompleted(THREAD_TIMEOUT_MS);
        assertEquals(UpdaterEventType.INFRA_TIMEOUT, event.getResult());
    }

    private void feedMockPipe(String[] logLines) {
        for (String line : logLines) {
            try {
                mMockPipe.write(line.getBytes());
            } catch (IOException e) {
                fail(e.getLocalizedMessage());
            }
        }
    }
}

