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

import com.android.loganalysis.item.AnrItem;
import com.android.loganalysis.item.BugreportItem;
import com.android.loganalysis.item.IItem;
import com.android.loganalysis.item.JavaCrashItem;
import com.android.loganalysis.item.LogcatItem;
import com.android.loganalysis.item.MonkeyLogItem;
import com.android.loganalysis.item.NativeCrashItem;
import com.android.loganalysis.parser.BugreportParser;
import com.android.loganalysis.parser.MonkeyLogParser;
import com.android.tradefed.log.LogUtil.CLog;
import com.android.tradefed.result.ITestInvocationListener;
import com.android.tradefed.result.InputStreamSource;
import com.android.tradefed.result.LogDataType;
import com.android.tradefed.result.ResultForwarder;
import com.android.tradefed.result.TestDescription;

import com.google.common.base.Throwables;

import org.junit.Assert;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.util.HashMap;
import java.util.Map;

/**
 * A {@link ResultForwarder} that intercepts monkey and bug report logs, extracts relevant metrics
 * from them using brillopad, and forwards the results to the specified
 * {@link ITestInvocationListener}.
 */
public class MonkeyBrillopadForwarder extends ResultForwarder {

    private enum MonkeyStatus {
        FINISHED("Money completed without errors."),
        CRASHED("Monkey run stopped because of a crash."),
        MISSING_COUNT("Monkey run failed to complete due to an unknown reason. " +
                "Check logs for details."),
        FALSE_COUNT("Monkey run reported an invalid count. " +
                "Check logs for details."),
        UPTIME_FAILURE("Monkey output is indicating an invalid uptime. " +
                "Device may have reset during run."),
        TIMEOUT("Monkey did not complete within the specified time");

        private String mDescription;

        MonkeyStatus(String desc) {
            mDescription = desc;
        }

        /** Returns a User friendly description of the status. */
        String getDescription() {
            return mDescription;
        }
    }

    private BugreportItem mBugreport = null;
    private MonkeyLogItem mMonkeyLog = null;
    private final long mMonkeyTimeoutMs;

    public MonkeyBrillopadForwarder(ITestInvocationListener listener, long monkeyTimeoutMs) {
        super(listener);
        mMonkeyTimeoutMs = monkeyTimeoutMs;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void testLog(String dataName, LogDataType dataType, InputStreamSource dataStream) {
        try {
            // just parse the logs for now. Forwarding of results will happen on test completion
            if (LogDataType.BUGREPORT.equals(dataType)) {
                CLog.i("Parsing %s", dataName);
                mBugreport = new BugreportParser().parse(new BufferedReader(new InputStreamReader(
                        dataStream.createInputStream())));
            }
            if (LogDataType.MONKEY_LOG.equals(dataType)) {
                CLog.i("Parsing %s", dataName);
                mMonkeyLog = new MonkeyLogParser().parse(new BufferedReader(new InputStreamReader(
                        dataStream.createInputStream())));
            }
        } catch (IOException e) {
            CLog.e("Could not parse file %s", dataName);
        }
        super.testLog(dataName, dataType, dataStream);
    }

    /** {@inheritDoc} */
    @Override
    public void testEnded(TestDescription monkeyTest, Map<String, String> metrics) {
        Map<String, String> monkeyMetrics = new HashMap<String, String>();
        try {
            Assert.assertNotNull("Failed to parse or retrieve bug report", mBugreport);
            Assert.assertNotNull("Cannot report run to brillopad, monkey log does not exist",
                mMonkeyLog);
            Assert.assertNotNull("monkey log is missing start time info",
                    mMonkeyLog.getStartUptimeDuration());
            Assert.assertNotNull("monkey log is missing stop time info",
                    mMonkeyLog.getStopUptimeDuration());
            LogcatItem systemLog = mBugreport.getSystemLog();

            MonkeyStatus status = reportMonkeyStats(mMonkeyLog, monkeyMetrics);
            StringBuilder crashTrace = new StringBuilder();
            reportAnrs(mMonkeyLog, monkeyMetrics, crashTrace);
            reportJavaCrashes(mMonkeyLog, monkeyMetrics, crashTrace);
            if (systemLog != null) {
                reportNativeCrashes(systemLog, monkeyMetrics, crashTrace);
            } else {
                CLog.w("Failed to get system log from bugreport");
            }

            if (!status.equals(MonkeyStatus.FINISHED)) {
                String failure = String.format("%s.\n%s", status.getDescription(),
                        crashTrace.toString());
                super.testFailed(monkeyTest, failure);
            }
        } catch (AssertionError e) {
            super.testFailed(monkeyTest, Throwables.getStackTraceAsString(e));
        } catch (RuntimeException e) {
            super.testFailed(monkeyTest, Throwables.getStackTraceAsString(e));
        } finally {
            super.testEnded(monkeyTest, monkeyMetrics);
        }
    }

    /**
     * Report stats about the monkey run from the monkey log.
     */
    private MonkeyStatus reportMonkeyStats(MonkeyLogItem monkeyLog,
            Map<String, String> monkeyMetrics) {
        MonkeyStatus status = getStatus(monkeyLog);
        monkeyMetrics.put("throttle", convertToString(monkeyLog.getThrottle()));
        monkeyMetrics.put("status", status.toString());
        monkeyMetrics.put("target_count", convertToString(monkeyLog.getTargetCount()));
        monkeyMetrics.put("injected_count", convertToString(monkeyLog.getFinalCount()));
        monkeyMetrics.put("run_duration", convertToString(monkeyLog.getTotalDuration()));
        monkeyMetrics.put("uptime", convertToString((monkeyLog.getStopUptimeDuration() -
                monkeyLog.getStartUptimeDuration())));
        return status;
    }

    /**
     * A utility method that converts an {@link Integer} to a {@link String}, and that can handle
     * null.
     */
    private static String convertToString(Integer integer) {
        return integer == null ? "" : integer.toString();
    }

    /**
     * A utility method that converts a {@link Long} to a {@link String}, and that can handle null.
     */
    private static String convertToString(Long longInt) {
        return longInt == null ? "" : longInt.toString();
    }

    /**
     * Report stats about Java crashes from the monkey log.
     */
    private void reportJavaCrashes(MonkeyLogItem monkeyLog, Map<String, String> metrics,
            StringBuilder crashTrace) {

        if (monkeyLog.getCrash() != null && monkeyLog.getCrash() instanceof JavaCrashItem) {
            JavaCrashItem jc = (JavaCrashItem) monkeyLog.getCrash();
            metrics.put("java_crash", "1");
            crashTrace.append("Detected java crash:\n");
            crashTrace.append(jc.getStack());
            crashTrace.append("\n");
        }
    }

    /**
     * Report stats about the native crashes from the bugreport.
     */
    private void reportNativeCrashes(LogcatItem systemLog, Map<String, String> metrics,
            StringBuilder crashTrace)  {
        if (systemLog.getEvents().size() > 0) {
            int nativeCrashes = 0;
            for (IItem item : systemLog.getEvents()) {
                if (item instanceof NativeCrashItem) {
                    nativeCrashes++;
                    crashTrace.append("Detected native crash:\n");
                    crashTrace.append(((NativeCrashItem)item).getStack());
                    crashTrace.append("\n");
                }
            }
            metrics.put("native_crash", Integer.toString(nativeCrashes));
        }
    }

    /**
     * Report stats about the ANRs from the monkey log.
     */
    private void reportAnrs(MonkeyLogItem monkeyLog, Map<String, String> metrics,
            StringBuilder crashTrace) {
        if (monkeyLog.getCrash() != null && monkeyLog.getCrash() instanceof AnrItem) {
            AnrItem anr = (AnrItem) monkeyLog.getCrash();
            metrics.put("anr_crash", "1");
            crashTrace.append("Detected ANR:\n");
            crashTrace.append(anr.getStack());
            crashTrace.append("\n");
        }
    }

    /**
     * Return the {@link MonkeyStatus} based on how the monkey run ran.
     */
    private MonkeyStatus getStatus(MonkeyLogItem monkeyLog) {
        // Uptime
        try {
            long startUptime = monkeyLog.getStartUptimeDuration();
            long stopUptime = monkeyLog.getStopUptimeDuration();
            long totalDuration = monkeyLog.getTotalDuration();
            if (stopUptime - startUptime < totalDuration - MonkeyBase.UPTIME_BUFFER) {
                return MonkeyStatus.UPTIME_FAILURE;
            }
            if (totalDuration >= mMonkeyTimeoutMs) {
                return MonkeyStatus.TIMEOUT;
            }
        } catch (NullPointerException e) {
            return MonkeyStatus.UPTIME_FAILURE;
        }

        // False count
        if (monkeyLog.getIsFinished() &&
                monkeyLog.getIntermediateCount() + 100 < monkeyLog.getTargetCount()) {
            return MonkeyStatus.FALSE_COUNT;
        }

        // Finished
        if (monkeyLog.getIsFinished()) {
            return MonkeyStatus.FINISHED;
        }

        // Crashed
        if (monkeyLog.getFinalCount() != null) {
            return MonkeyStatus.CRASHED;
        }

        // Missing count
        return MonkeyStatus.MISSING_COUNT;
    }
}
