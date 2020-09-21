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
package android.longevity.core.listener;

import org.junit.runner.Description;
import org.junit.runner.notification.RunNotifier;

import java.util.Map;
import java.util.concurrent.TimeUnit;

/**
 * A {@link RunTerminator} for terminating early on test end due to long duration for host suites.
 */
public class TimeoutTerminator extends RunTerminator {
    protected static final String OPTION = "suite-timeout_msec";
    protected static final long DEFAULT = TimeUnit.MINUTES.toMillis(30L);
    protected static final long UNSET_TIMESTAMP = -1;

    protected long mStartTimestamp = UNSET_TIMESTAMP;
    protected long mSuiteTimeout; // ms

    public TimeoutTerminator(RunNotifier notifier, Map<String, String> args) {
        super(notifier);
        mSuiteTimeout = args.containsKey(OPTION) ?
            Long.parseLong(args.get(OPTION)) : DEFAULT;
    }

    /**
     * {@inheritDoc}
     *
     * Note: this initializes the countdown timer if unset.
     */
    @Override
    public void testStarted(Description description) {
        if (mStartTimestamp == UNSET_TIMESTAMP) {
            mStartTimestamp = getCurrentTimestamp();
        }
    }

    @Override
    public void testFinished(Description description) {
      if (mStartTimestamp != UNSET_TIMESTAMP
          && (getCurrentTimestamp() - mStartTimestamp) > mSuiteTimeout) {
            kill("the suite timed out");
        }
    }

    /**
     * Returns the current timestamp in ms for calculating time differences.
     * <p>
     * Note: this is exposed for the platform-specific {@code TimeoutTerminator}.
     */
    protected long getCurrentTimestamp() {
        return System.currentTimeMillis();
    }
}
