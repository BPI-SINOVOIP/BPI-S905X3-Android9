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
package android.device.collectors;

import android.os.Bundle;
import android.support.annotation.VisibleForTesting;
import android.util.Log;

import org.junit.runner.Description;
import org.junit.runner.Result;

import java.util.Timer;
import java.util.TimerTask;

/**
 * Implementation of {@link BaseMetricListener} that allows to run a periodic collection during the
 * instrumentation run. Implementing {@link #collect(DataRecord, Description)} as the periodic task
 * running. It is possible to run some actions before and at the end of the periodic run using
 * {@link #onStart(DataRecord, Description)} and {@link #onEnd(DataRecord, Result)}.
 */
public abstract class ScheduledRunMetricListener extends BaseMetricListener {

    public static final String INTERVAL_ARG_KEY = "interval";
    private static final long DEFAULT_INTERVAL_MS = 60 * 1000l; // 1 min

    private Timer mTimer;

    public ScheduledRunMetricListener() {}

    @VisibleForTesting
    ScheduledRunMetricListener(Bundle argsBundle) {
        super(argsBundle);
    }

    @Override
    public final void onTestRunStart(final DataRecord runData, final Description description) {
        Log.d(getTag(), "Starting");
        onStart(runData, description);
        // TODO: could this be done with Handlers ?
        mTimer = new Timer();
        TimerTask timerTask =
                new TimerTask() {
                    @Override
                    public void run() {
                        try {
                            collect(runData, description);
                        } catch (InterruptedException e) {
                            mTimer.cancel();
                            Thread.currentThread().interrupt();
                            Log.e(getTag(), "Interrupted exception thrown from task:", e);
                        }
                    }
                };

        mTimer.scheduleAtFixedRate(timerTask, 0, getIntervalFromArgs());
    }

    @Override
    public final void onTestRunEnd(DataRecord runData, Result result) {
        if (mTimer != null) {
            mTimer.cancel();
            mTimer.purge();
        }
        onEnd(runData, result);
        Log.d(getTag(), "Finished");
    }

    /**
     * Executed when entering this collector.
     *
     * @param runData the {@link DataRecord} where to put metrics.
     * @param description the {@link Description} for the test run.
     */
    void onStart(DataRecord runData, Description description) {
        // Does nothing.
    }

    /**
     * Executed when finishing this collector.
     *
     * @param runData the {@link DataRecord} where to put metrics.
     * @param result the {@link Result} of the test run about to end.
     */
    void onEnd(DataRecord runData, Result result) {
        // Does nothing.
    }

    /**
     * Task periodically & asynchronously run during the test running.
     *
     * @param runData the {@link DataRecord} where to put metrics.
     * @param description the {@link Description} of the run in progress.
     * @throws InterruptedException
     */
    public abstract void collect(DataRecord runData, Description description)
            throws InterruptedException;

    /**
     * Extract the interval from the instrumentation arguments or use the default interval value.
     */
    private long getIntervalFromArgs() {
        String intervalValue = getArgsBundle().getString(INTERVAL_ARG_KEY);
        long interval = 0L;
        try {
            interval = Long.parseLong(intervalValue);
        } catch (NumberFormatException e) {
            Log.e(getTag(), "Failed to parse the interval value.", e);
        }
        // In case invalid or no interval was specified, we use the default one.
        if (interval <= 0l) {
            Log.d(getTag(),
                    String.format(
                            "Using default interval %s for periodic task. %s could not be used.",
                            DEFAULT_INTERVAL_MS, interval));
            interval = DEFAULT_INTERVAL_MS;
        }
        return interval;
    }
}
