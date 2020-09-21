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

import com.android.tradefed.log.LogUtil.CLog;

import java.util.ArrayList;
import java.util.List;

/**
 * Helper class for gathering and reporting {@link TestSummary} for set of listeners
 */
public class InvocationSummaryHelper {

    private InvocationSummaryHelper() {
    }

    public static void reportInvocationEnded(List<ITestInvocationListener> listeners,
            long elapsedTime) {
        List<TestSummary> summaries = new ArrayList<TestSummary>(listeners.size());
        for (ITestInvocationListener listener : listeners) {
            /*
             * For InvocationListeners (as opposed to SummaryListeners), we call
             * invocationEnded() followed by getSummary().  If getSummary returns a non-null
             * value, we gather it to pass to the SummaryListeners below.
             */
            if (!(listener instanceof ITestSummaryListener)) {
                try {
                    listener.invocationEnded(elapsedTime);
                    TestSummary summary = listener.getSummary();
                    if (summary != null) {
                        summary.setSource(listener.getClass().getName());
                        summaries.add(summary);
                    }
                } catch (RuntimeException e) {
                    CLog.e(
                            "RuntimeException while invoking invocationEnded on %s",
                            listener.getClass().getName());
                    CLog.e(e);
                }
            }
        }

        /*
         * For SummaryListeners (as opposed to InvocationListeners), we now call putSummary()
         * followed by invocationEnded().  This means that the SummaryListeners will have
         * access to the summaries (if any) when invocationEnded is called.
         */
        for (ITestInvocationListener listener : listeners) {
            if (listener instanceof ITestSummaryListener) {
                try {
                    ((ITestSummaryListener) listener).putSummary(summaries);
                    listener.invocationEnded(elapsedTime);
                } catch (RuntimeException e) {
                    CLog.e(
                            "RuntimeException while invoking invocationEnded on %s",
                            listener.getClass().getName());
                    CLog.e(e);
                }
            }
        }
    }
}
