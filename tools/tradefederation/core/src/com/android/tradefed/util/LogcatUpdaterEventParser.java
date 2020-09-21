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

import com.android.loganalysis.item.LogcatItem;
import com.android.loganalysis.item.MiscLogcatItem;
import com.android.loganalysis.parser.LogcatParser;
import com.android.tradefed.device.ILogcatReceiver;
import com.android.tradefed.log.LogUtil.CLog;
import com.android.tradefed.result.InputStreamSource;

import java.io.BufferedReader;
import java.io.Closeable;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.util.AbstractMap.SimpleEntry;
import java.util.List;
import java.util.Map;

/**
 * Parse logcat input for system updater related events.
 *
 * <p>In any system with A/B updates, the updater will log its progress to logcat. This class
 * interprets updater-related logcat messages and can inform listeners of events in both a blocking
 * and non-blocking fashion.
 */
public class LogcatUpdaterEventParser implements Closeable {

    // define a custom logcat parser category for events of our interests
    private static final String UPDATER_CUSTOM_CATEGORY = "updaterEvent";
    // wait time before trying to reobtain logcat stream
    private static final long LOGCAT_REFRESH_WAIT_TIME_MS = 50L;

    private EventTriggerMap mEventTriggerMap;
    private ILogcatReceiver mLogcatReceiver;
    private LogcatParser mInternalParser;
    private BufferedReader mStreamReader = null;
    private InputStreamSource mCurrentLogcatData = null;
    private InputStream mCurrentInputStream = null;
    private InputStreamReader mCurrentStreamReader = null;

    /**
     * Helper class to store (logcat tag, message) with the corresponding {@link UpdaterEventType}.
     *
     * <p>The message registered with {@link #put(String, String, UpdaterEventType)} may be a
     * substring of the actual logcat message.
     */
    private class EventTriggerMap {
        // a map that each tag corresponds to multiple entry of (partial message, response)
        private MultiMap<String, SimpleEntry<String, UpdaterEventType>> mResponses =
                new MultiMap<>();

        /** Register a {@link UpdaterEventType} with a specific logcat tag and message. */
        public void put(String tag, String partialMessage, UpdaterEventType response) {
            mResponses.put(tag, new SimpleEntry<>(partialMessage, response));
        }

        /**
         * Return an {@link UpdaterEventType} with exactly matching tag and partially matching
         * message.
         */
        public UpdaterEventType get(String tag, String message) {
            for (Map.Entry<String, UpdaterEventType> entry : mResponses.get(tag)) {
                if (message.contains(entry.getKey())) {
                    return entry.getValue();
                }
            }
            return null;
        }
    }

    /**
     * A monitor object which allows callers to receive events asynchronously.
     */
    public class AsyncUpdaterEvent {

        private boolean mIsCompleted;
        private UpdaterEventType mResult;

        public AsyncUpdaterEvent() {
            mIsCompleted = false;
        }

        public synchronized boolean isCompleted() {
            return mIsCompleted;
        }

        public synchronized void setCompleted(UpdaterEventType result) {
            if (isCompleted()) {
                throw new IllegalStateException("Wait thread already completed!");
            }
            mIsCompleted = true;
            mResult = result;
        }

        public synchronized void waitUntilCompleted(long timeoutMs) {
            try {
                while (!isCompleted()) {
                    wait(timeoutMs);
                }
            } catch (InterruptedException e) {
                throw new RuntimeException(e);
            }
        }

        public synchronized UpdaterEventType getResult() {
            if (!isCompleted()) {
                throw new IllegalStateException("Wait thread not yet completed.");
            }
            return mResult;
        }
    }

    /** Create a new {@link LogcatUpdaterEventParser} */
    public LogcatUpdaterEventParser(ILogcatReceiver logcatReceiver) {
        mEventTriggerMap = new EventTriggerMap();
        mLogcatReceiver = logcatReceiver;
        mInternalParser = new LogcatParser();

        // events registered first are matched first
        registerEventTrigger(
                "update_engine", "Using this install plan:", UpdaterEventType.UPDATE_START);
        registerEventTrigger(
                "update_engine",
                "ActionProcessor: finished DownloadAction with code ErrorCode::kSuccess",
                UpdaterEventType.DOWNLOAD_COMPLETE);
        registerEventTrigger(
                "update_engine",
                "ActionProcessor: finished DownloadAction with code ErrorCode",
                UpdaterEventType.ERROR);
        registerEventTrigger(
                "update_engine",
                "ActionProcessor: finished FilesystemVerifierAction with code ErrorCode::kSuccess",
                UpdaterEventType.PATCH_COMPLETE);
        registerEventTrigger(
                "update_engine",
                "ActionProcessor: finished FilesystemVerifierAction with code ErrorCode",
                UpdaterEventType.ERROR);
        registerEventTrigger(
                "update_engine",
                "All post-install commands succeeded",
                UpdaterEventType.D2O_COMPLETE);
        registerEventTrigger(
                "update_engine",
                "Update successfully applied",
                UpdaterEventType.UPDATE_COMPLETE);
        registerEventTrigger(
                "update_engine_client",
                "onPayloadApplicationComplete(ErrorCode::kSuccess (0))",
                UpdaterEventType.UPDATE_COMPLETE);
        // kNewRootfsVerificationError is often caused by flaky flashing - b/66996067 (#6)
        registerEventTrigger(
                "update_engine_client",
                "onPayloadApplicationComplete(ErrorCode::kNewRootfsVerificationError (15))",
                UpdaterEventType.ERROR_FLAKY);
        registerEventTrigger(
                "update_engine_client",
                "onPayloadApplicationComplete(ErrorCode",
                UpdaterEventType.ERROR);
        registerEventTrigger(
                "update_verifier",
                "Leaving update_verifier.",
                UpdaterEventType.UPDATE_VERIFIER_COMPLETE);

        mCurrentLogcatData = mLogcatReceiver.getLogcatData();
        mCurrentInputStream = mCurrentLogcatData.createInputStream();
        mCurrentStreamReader = new InputStreamReader(mCurrentInputStream);
        mStreamReader = new BufferedReader(mCurrentStreamReader);
    }

    /**
     * Register an event of given logcat tag and message with the desired response. Message may be
     * partial.
     */
    protected void registerEventTrigger(String tag, String msg, UpdaterEventType response) {
        mEventTriggerMap.put(tag, msg, response);
        mInternalParser.addPattern(null, "I", tag, UPDATER_CUSTOM_CATEGORY);
    }

    /**
     * Internal call that blacks until the specific event is encountered or the timeout is reached.
     * Return the expected event or an error code.
     */
    private UpdaterEventType internalWaitForEvent(
            final UpdaterEventType expectedEvent, long timeoutMs) throws IOException {
        // Parse only a single line at a time, otherwise the LogcatParser won't return until
        // the logcat stream is over
        long startTime = System.currentTimeMillis();
        while (System.currentTimeMillis() - startTime < timeoutMs) {
            String lastLine;
            while ((lastLine = mStreamReader.readLine()) != null) {
                UpdaterEventType parsedEvent = parseEventType(lastLine);
                if (parsedEvent == UpdaterEventType.ERROR
                        || parsedEvent == UpdaterEventType.ERROR_FLAKY
                        || expectedEvent.equals(parsedEvent)) {
                    return parsedEvent;
                }
            }
            // if the stream returns null, we lost our logcat. Wait for a small amount
            // of time, then try to reobtain it.
            CLog.v("Failed to read logcat stream. Attempt to reconnect.");
            RunUtil.getDefault().sleep(LOGCAT_REFRESH_WAIT_TIME_MS);
            refreshLogcatStream();
        }
        CLog.e(
                "waitForEvent():"
                        + expectedEvent.toString()
                        + " timed out after "
                        + TimeUtil.formatElapsedTime(timeoutMs));
        return UpdaterEventType.INFRA_TIMEOUT;
    }

    /** Block until the specified event is encountered or the timeout is reached. */
    public UpdaterEventType waitForEvent(UpdaterEventType expectedEvent, long timeoutMs) {
        try {
            return internalWaitForEvent(expectedEvent, timeoutMs);
        } catch (IOException e) {
            throw new RuntimeException(e);
        }
    }

    /**
     * Wait for an event but do not block. Return an {@link AsyncUpdaterEvent} monitor which the
     * caller may wait on and query.
     */
    public AsyncUpdaterEvent waitForEventAsync(
            final UpdaterEventType expectedEvent, long timeoutMs) {
        final AsyncUpdaterEvent event = new AsyncUpdaterEvent();
        final Thread callingThread = Thread.currentThread();

        Thread waitThread =
                new Thread(
                        () -> {
                            UpdaterEventType result = UpdaterEventType.INFRA_TIMEOUT;
                            try {
                                result = internalWaitForEvent(expectedEvent, timeoutMs);
                            } catch (IOException e) {
                                // Unblock calling thread
                                callingThread.interrupt();
                                throw new RuntimeException(e);
                            } finally {
                                // ensure that the waiting thread drops out of the wait loop
                                synchronized (event) {
                                    event.setCompleted(result);
                                    event.notifyAll();
                                }
                            }
                        });
        waitThread.setName(getClass().getCanonicalName());
        waitThread.setDaemon(true);
        waitThread.start();
        return event;
    }

    /**
     * Parse a logcat line and return the captured event (that were registered with {@link
     * #registerEventTrigger(String, String, UpdaterEventType)}) or null.
     */
    protected UpdaterEventType parseEventType(String lastLine) {
        LogcatItem item = mInternalParser.parse(ArrayUtil.list(lastLine));
        if (item == null) {
            return null;
        }
        List<MiscLogcatItem> miscItems = item.getMiscEvents(UPDATER_CUSTOM_CATEGORY);
        if (miscItems.size() == 0) {
            return null;
        }
        MiscLogcatItem mi = miscItems.get(0);
        mInternalParser.clear();
        return mEventTriggerMap.get(mi.getTag(), mi.getStack());
    }

    /** Reobtain logcat stream when it's lost */
    private void refreshLogcatStream() throws IOException {
        mStreamReader.close();
        StreamUtil.cancel(mCurrentLogcatData);
        mCurrentLogcatData = mLogcatReceiver.getLogcatData();
        mCurrentInputStream = mCurrentLogcatData.createInputStream();
        mCurrentStreamReader = new InputStreamReader(mCurrentInputStream);
        mStreamReader = new BufferedReader(mCurrentStreamReader);
    }

    /** {@inheritDoc} */
    @Override
    public void close() throws IOException {
        StreamUtil.close(mStreamReader);
        StreamUtil.cancel(mCurrentLogcatData);
        StreamUtil.close(mCurrentInputStream);
        StreamUtil.close(mCurrentStreamReader);
    }
}

