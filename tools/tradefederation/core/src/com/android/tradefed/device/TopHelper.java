/*
 * Copyright (C) 2011 The Android Open Source Project
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

package com.android.tradefed.device;

import com.android.ddmlib.MultiLineReceiver;
import com.android.tradefed.log.LogUtil.CLog;
import com.android.tradefed.util.SimpleStats;

import java.io.BufferedWriter;
import java.io.File;
import java.io.FileWriter;
import java.io.IOException;
import java.util.ArrayList;
import java.util.LinkedList;
import java.util.List;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

/**
 * Helper class which runs top continuously on an {@link ITestDevice} and parses the output.
 * <p>
 * Provides a method to record the output of top and get all recorded CPU usage measurements or an
 * average of a specified range of measurements.  Note that top can cause approximately a 10%
 * overhead to the CPU usage while running, so results will not be entirely accurate.
 * </p>
 */
public class TopHelper extends Thread {
    /** The top command to run during the actions. */
    private static final String TOP_CMD = "top -d %d -m 10 -t";
    /** The pattern to match for the top output. */
    private static final Pattern TOP_PERCENT_PATTERN =
            Pattern.compile("User (\\d+)%, System (\\d+)%, IOW (\\d+)%, IRQ (\\d+)%");

    private ITestDevice mTestDevice;
    private int mDelay;

    /**
     * Enum used for distinguishing between the various percentages in the top output.
     */
    enum PercentCategory {
        TOTAL,
        USER,
        SYSTEM,
        IOW,
        IRQ
    }

    /**
     * Class for holding the parsed output for a single top output.
     * <p>
     * Currently, this only holds the percentage info from top but can be extended to contain the
     * process information in the top output.
     * </p>
     */
    public static class TopStats {
        public Double mTotalPercent = null;
        public Double mUserPercent = null;
        public Double mSystemPercent = null;
        public Double mIowPercent = null;
        public Double mIrqPercent = null;
    }

    /**
     * Receiver which parses the output from top.
     */
    static class TopReceiver extends MultiLineReceiver {
        private List<TopStats> mTopStats = new LinkedList<TopStats>();
        private boolean mIsCancelled = false;
        private File mLogFile = null;
        private BufferedWriter mLogWriter = null;

        public TopReceiver() {
            setTrimLine(false);
        }

        /**
         * Specify a file to log the top output to.
         *
         * @param logFile the file to lot output to.
         */
        public synchronized void logToFile(File logFile) {
            try {
                mLogFile = logFile;
                mLogWriter = new BufferedWriter(new FileWriter(mLogFile));
            } catch (IOException e) {
                CLog.e("Error creating fileWriter:");
                CLog.e(e);
                mLogWriter = null;
            }
        }

        /**
         * {@inheritDoc}
         */
        @Override
        public void processNewLines(String[] lines) {
            if (mIsCancelled) {
                return;
            }
            synchronized (this) {
                if (mLogWriter != null) {
                    try {
                        for (String line : lines) {
                            mLogWriter.write(line + "\n");
                        }
                    } catch (IOException e) {
                        CLog.e("Error writing to file:");
                        CLog.e(e);
                    }
                }
            }
            for (String line : lines) {
                line = line.trim();
                Matcher m = TOP_PERCENT_PATTERN.matcher(line);
                if (m.matches()) {
                    TopStats s = new TopStats();

                    // Will not trigger NumberFormatException due to TOP_PATTERN matching.
                    s.mUserPercent = Double.parseDouble(m.group(1));
                    s.mSystemPercent = Double.parseDouble(m.group(2));
                    s.mIowPercent = Double.parseDouble(m.group(3));
                    s.mIrqPercent = Double.parseDouble(m.group(4));
                    s.mTotalPercent = (s.mUserPercent + s.mSystemPercent + s.mIowPercent +
                            s.mIrqPercent);
                    synchronized(this) {
                        mTopStats.add(s);
                    }
                }
            }
        }

        /**
         * Cancels the top command.
         */
        public synchronized void cancel() {
            if (mIsCancelled) {
                return;
            }
            mIsCancelled = true;
            if (mLogWriter != null) {
                try {
                    mLogWriter.flush();
                    mLogWriter.close();
                } catch (IOException e) {
                    CLog.e("Error closing writer:");
                    CLog.e(e);
                } finally {
                    mLogWriter = null;
                }
            }
        }

        /**
         * {@inheritDoc}
         */
        @Override
        public synchronized boolean isCancelled() {
            return mIsCancelled;
        }

        /**
         * Gets a list of {@link TopStats} instances.
         *
         * @return a list of {@link TopStats} instances ordered from oldest to newest.
         */
        public synchronized List<TopStats> getTopStats() {
            return new ArrayList<TopStats>(mTopStats);
        }
    }

    private TopReceiver mReceiver = new TopReceiver();

    /**
     * Create a {@link TopHelper} instance with a delay specified.
     *
     * @param testDevice The device.
     * @param delay The delay time interval for the top command in seconds.
     */
    public TopHelper(ITestDevice testDevice, int delay) {
        super("TopHelper");
        mTestDevice = testDevice;
        mDelay = delay;
    }

    /**
     * Create a {@link TopHelper} instance with a default delay of 1 second.
     *
     * @param testDevice The device.
     */
    public TopHelper(ITestDevice testDevice) {
        this(testDevice, 1);
    }

    /**
     * Specify a file to log the top output to.
     *
     * @param logFile the file to lot output to.
     */
    public void logToFile(File logFile) {
        mReceiver.logToFile(logFile);
    }

    /**
     * Cancels the top command.
     */
    public synchronized void cancel() {
        mReceiver.cancel();
    }

    /**
     * Gets whether the top command is canceled.
     *
     * @return if the top command is canceled.
     */
    public synchronized boolean isCancelled() {
        return mReceiver.isCancelled();
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void run() {
        try {
            mTestDevice.executeShellCommand(String.format(TOP_CMD, mDelay), mReceiver);
        } catch (DeviceNotAvailableException e) {
            CLog.e("Device %s not available:", mTestDevice.getSerialNumber());
            CLog.e(e);
        }
    }

    /**
     * Gets a list of {@link TopStats} instances.
     *
     * @return a list of {@link TopStats} instances ordered from oldest to newest.
     */
    public List<TopStats> getTopStats() {
        return mReceiver.getTopStats();
    }

    /**
     * Get the average total CPU usage for a list of {@link TopStats}.
     *
     * @param topStats the list of {@link TopStats}
     * @return The average usage as a percentage (0 to 100).
     */
    public static Double getTotalAverage(List<TopStats> topStats) {
        return getAveragePercentage(topStats, PercentCategory.TOTAL);
    }

    /**
     * Get the average user CPU usage for a list of {@link TopStats}.
     *
     * @param topStats the list of {@link TopStats}
     * @return The average usage as a percentage (0 to 100).
     */
    public static Double getUserAverage(List<TopStats> topStats) {
        return getAveragePercentage(topStats, PercentCategory.USER);
    }

    /**
     * Get the average system CPU usage for a list of {@link TopStats}.
     *
     * @param topStats the list of {@link TopStats}
     * @return The average usage as a percentage (0 to 100).
     */
    public static Double getSystemAverage(List<TopStats> topStats) {
        return getAveragePercentage(topStats, PercentCategory.SYSTEM);
    }

    /**
     * Get the average IOW CPU usage for a list of {@link TopStats}.
     *
     * @param topStats the list of {@link TopStats}
     * @return The average usage as a percentage (0 to 100).
     */
    public static Double getIowAverage(List<TopStats> topStats) {
        return getAveragePercentage(topStats, PercentCategory.IOW);
    }

    /**
     * Get the average IRQ CPU usage for a list of {@link TopStats}.
     *
     * @param topStats the list of {@link TopStats}
     * @return The average usage as a percentage (0 to 100).
     */
    public static Double getIrqAverage(List<TopStats> topStats) {
        return getAveragePercentage(topStats, PercentCategory.IRQ);
    }


    /**
     * Get the average CPU usage for a list of {@link TopStats} and a given category.
     *
     * @param topStats the list of {@link TopStats}
     * @param category the percentage category
     * @return The average usage as a percentage (0 to 100).
     */
    private static Double getAveragePercentage(List<TopStats> topStats, PercentCategory category)
            throws IndexOutOfBoundsException {
        SimpleStats stats = new SimpleStats();
        for (TopStats s : topStats) {
            switch(category) {
                case TOTAL:
                    stats.add(s.mTotalPercent);
                    break;
                case USER:
                    stats.add(s.mUserPercent);
                    break;
                case SYSTEM:
                    stats.add(s.mSystemPercent);
                    break;
                case IOW:
                    stats.add(s.mIowPercent);
                    break;
                case IRQ:
                    stats.add(s.mIrqPercent);
                    break;
            }
        }
        return stats.mean();
    }

    /**
     * Package protected method used for testing.
     *
     * @return the TopReceiver
     */
    TopReceiver getReceiver() {
        return mReceiver;
    }
}