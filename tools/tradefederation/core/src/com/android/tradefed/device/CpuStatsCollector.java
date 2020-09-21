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

package com.android.tradefed.device;

import com.android.ddmlib.MultiLineReceiver;
import com.android.tradefed.log.LogUtil.CLog;
import com.android.tradefed.util.SimpleStats;

import java.io.BufferedWriter;
import java.io.File;
import java.io.FileWriter;
import java.io.IOException;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.LinkedList;
import java.util.List;
import java.util.Map;

/**
 * Helper class which runs {@code cpustats} continuously on an {@link ITestDevice} and parses the
 * output.
 * <p>
 * Provides a method to record the output of {@code cpustats} and get all the cpu usage measurements
 * as well methods for performing calculations on that data to find the mean of the cpu workload,
 * the approximate cpu frequency, and the percentage of used cpu frequency.
 * </p><p>
 * This is meant to be a replacement for {@link TopHelper}, which does not provide stats about cpu
 * frequency and has a higher overhead due to measuring processes/threads.
 * </p><p>
 * The {@code cpustats} command was added in the Jellybean release, so this collector should only be
 * used for new tests.
 * </p>
 * @see TopHelper
 */
public class CpuStatsCollector extends Thread {
    private static final String CPU_STATS_CMD = "cpustats -m -d %s";

    private final ITestDevice mTestDevice;
    private long mDelay;

    /**
     * Used to distinguish between the different CPU time categories.
     */
    enum TimeCategory {
        USER,
        NICE,
        SYS,
        IDLE,
        IOW,
        IRQ,
        SIRQ
    }

    /**
     * Class for holding parsed output data for a single {@code cpustats} output.
     * <p>
     * This class holds parsed output, and also performs simple calculations on that data. The
     * methods which perform these calucations should only be called after the object has been
     * populated.
     * </p>
     */
    public static class CpuStats {
        public Map<TimeCategory, Integer> mTimeStats = new HashMap<TimeCategory, Integer>();
        public Map<Integer, Integer> mFreqStats = new HashMap<Integer, Integer>();
        private Map<TimeCategory, Double> mPercentageStats = new HashMap<TimeCategory, Double>();
        private Integer mTotalTime = null;
        private Double mAverageMhz = null;

        /**
         * Get the percentage of cycles used on a given category.
         */
        public Double getPercentage(TimeCategory category) {
            if (!mPercentageStats.containsKey(category)) {
                mPercentageStats.put(category, 100.0 * mTimeStats.get(category) / getTotalTime());
            }
            return mPercentageStats.get(category);
        }

        /**
         * Estimate the MHz used by the cpu during the duration.
         * <p>
         * This is calculated by:
         * </p><code>
         * ((sum(c_time) - idle) / sum(c_time)) * (sum(freq * f_time) / sum(f_time))
         * </code><p>
         * where {@code c_time} is the time for a given category, {@code idle} is the time in the
         * idle state, {@code freq} is a frequency and {@code f_time} is the time spent in that
         * frequency.
         * </p>
         */
        public Double getEstimatedMhz() {
            if (mFreqStats.isEmpty()) {
                return null;
            }
            return getTotalUsage() * getAverageMhz();
        }

        /**
         * Get the amount of MHz as a percentage of available MHz used by the cpu during the
         * duration.
         * <p>
         * This is calculated by:
         * </p><code>
         * 100 * sum(freq * f_time) / (max_freq * sum(f_time))
         * </code><p>
         * where {@code freq} is a frequency, {@code f_time} is the time spent in that frequency,
         * and {@code max_freq} is the maximum frequency the cpu is capable of.
         * </p>
         */
        public Double getUsedMhzPercentage() {
            if (mFreqStats.isEmpty()) {
                return null;
            }
            return 100.0 * getAverageMhz() / getMaxMhz();
        }

        /**
         * Get the total usage, or the sum of all the times except idle over the sum of all the
         * times.
         */
        private Double getTotalUsage() {
            return (double) (getTotalTime() - mTimeStats.get(TimeCategory.IDLE)) / getTotalTime();
        }

        /**
         * Get the average MHz.
         * <p>
         * This is calculated by:
         * </p><code>
         * sum(freq * f_time) / sum(f_time))
         * </code><p>
         * where {@code freq} is a frequency and {@code f_time} is the time spent in that frequency.
         * </p>
         */
        private Double getAverageMhz() {
            if (mFreqStats.isEmpty()) {
                return null;
            }
            if (mAverageMhz == null) {
                double sumFreqTime = 0.0;
                long sumTime = 0;
                for (Map.Entry<Integer, Integer> e : mFreqStats.entrySet()) {
                    sumFreqTime += e.getKey() * e.getValue() / 1000.0;
                    sumTime += e.getValue();
                }
                mAverageMhz = sumFreqTime / sumTime;
            }
            return mAverageMhz;
        }

        /**
         * Get the maximum possible MHz.
         */
        private Double getMaxMhz() {
            if (mFreqStats.isEmpty()) {
                return null;
            }
            int max = 0;
            for (int freq : mFreqStats.keySet()) {
                max = Math.max(max, freq);
            }
            return max / 1000.0;
        }

        /**
         * Get the total amount of time cycles.
         */
        private Integer getTotalTime() {
            if (mTotalTime == null) {
                int sum = 0;
                for (int time : mTimeStats.values()) {
                    sum += time;
                }
                mTotalTime = sum;
            }
            return mTotalTime;
        }
    }

    /**
     * Receiver which parses the output from {@code cpustats} and optionally logs to a file.
     */
    public static class CpuStatsReceiver extends MultiLineReceiver {
        private Map<String, List<CpuStats>> mCpuStats = new HashMap<String, List<CpuStats>>(4);

        private boolean mIsCancelled = false;
        private File mLogFile = null;
        private BufferedWriter mLogWriter = null;

        public CpuStatsReceiver() {
            setTrimLine(false);
        }

        /**
         * Specify a file to log the output to.
         * <p>
         * This can be called at any time in the receivers life cycle, but only new output will be
         * logged to the file.
         * </p>
         */
        public synchronized void logToFile(File logFile) {
            try {
                mLogFile = logFile;
                mLogWriter = new BufferedWriter(new FileWriter(mLogFile));
            } catch (IOException e) {
                CLog.e("IOException when creating a fileWriter:");
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
                        CLog.e("Error writing to file");
                        CLog.e(e);
                    }
                }
            }
            for (String line : lines) {
                String[] args = line.trim().split(",");
                if (args.length >= 8) {
                    try {
                        CpuStats s = new CpuStats();
                        s.mTimeStats.put(TimeCategory.USER, Integer.parseInt(args[1]));
                        s.mTimeStats.put(TimeCategory.NICE, Integer.parseInt(args[2]));
                        s.mTimeStats.put(TimeCategory.SYS, Integer.parseInt(args[3]));
                        s.mTimeStats.put(TimeCategory.IDLE, Integer.parseInt(args[4]));
                        s.mTimeStats.put(TimeCategory.IOW, Integer.parseInt(args[5]));
                        s.mTimeStats.put(TimeCategory.IRQ, Integer.parseInt(args[6]));
                        s.mTimeStats.put(TimeCategory.SIRQ, Integer.parseInt(args[7]));
                        for (int i = 0; i + 8 < args.length; i += 2) {
                            s.mFreqStats.put(Integer.parseInt(args[8 + i]),
                                    Integer.parseInt(args[9 + i]));
                        }
                        synchronized(this) {
                            if (!mCpuStats.containsKey(args[0])) {
                                mCpuStats.put(args[0], new LinkedList<CpuStats>());
                            }
                            mCpuStats.get(args[0]).add(s);
                        }
                    } catch (NumberFormatException e) {
                        CLog.w("Unexpected input: %s", line.trim());
                    } catch (IndexOutOfBoundsException e) {
                        CLog.w("Unexpected input: %s", line.trim());
                    }
                } else if (args.length > 1 || !"".equals(args[0])) {
                    CLog.w("Unexpected input: %s", line.trim());
                }
            }
        }

        /**
         * Cancels the {@code cpustats} command.
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
                    CLog.e("Error closing writer");
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
         * Get all the parsed data as a map from label to lists of {@link CpuStats} objects.
         */
        public synchronized Map<String, List<CpuStats>> getCpuStats() {
            Map<String, List<CpuStats>> copy = new HashMap<String, List<CpuStats>>(
                    mCpuStats.size());
            for (String k  : mCpuStats.keySet()) {
                copy.put(k, new ArrayList<CpuStats>(mCpuStats.get(k)));
            }
            return copy;
        }
    }

    private CpuStatsReceiver mReceiver = new CpuStatsReceiver();

    /**
     * Create a {@link CpuStatsCollector}.
     *
     * @param testDevice The test device
     */
    public CpuStatsCollector(ITestDevice testDevice) {
        this(testDevice, 1);
    }

    /**
     * Create a {@link CpuStatsCollector} with a delay specified.
     *
     * @param testDevice The test device
     * @param delay The delay time in seconds
     */
    public CpuStatsCollector(ITestDevice testDevice, int delay) {
        super("CpuStatsCollector");
        mTestDevice = testDevice;
        mDelay = delay;
    }

    /**
     * Specify a file to log output to.
     *
     * @param logFile the file to log output to.
     */
    public void logToFile(File logFile) {
        mReceiver.logToFile(logFile);
    }

    /**
     * Cancels the {@code cpustats} command.
     */
    public synchronized void cancel() {
        mReceiver.cancel();
    }

    /**
     * Gets whether the {@code cpustats} command is canceled.
     *
     * @return if the {@code cpustats} command is canceled.
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
            mTestDevice.executeShellCommand(String.format(CPU_STATS_CMD, mDelay), mReceiver);
        } catch (DeviceNotAvailableException e) {
            CLog.e("Device %s not available:", mTestDevice.getSerialNumber());
            CLog.e(e);
        }
    }

    /**
     * Get the mapping of labels to lists of {@link CpuStats} instances.
     *
     * @return a mapping of labels to lists of {@link CpuStats} instances. The labels will include
     * "Total" and "cpu0"..."cpuN" for each CPU on the device.
     */
    public Map<String, List<CpuStats>> getCpuStats() {
        return mReceiver.getCpuStats();
    }

    /**
     * Get the mean of the total CPU usage for a list of {@link CpuStats}.
     *
     * @param cpuStats the list of {@link CpuStats}
     * @return The average usage as a percentage (0 to 100).
     */
    public static Double getTotalPercentageMean(List<CpuStats> cpuStats) {
        SimpleStats stats = new SimpleStats();
        for (CpuStats s : cpuStats) {
            if (s.getTotalUsage() != null) {
                stats.add(s.getTotalUsage());
            }
        }
        return 100 * stats.mean();
    }

    /**
     * Get the mean of the user and nice CPU usage for a list of {@link CpuStats}.
     *
     * @param cpuStats the list of {@link CpuStats}
     * @return The average usage as a percentage (0 to 100).
     */
    public static Double getUserPercentageMean(List<CpuStats> cpuStats) {
        return (getPercentageMean(cpuStats, TimeCategory.USER) +
                getPercentageMean(cpuStats, TimeCategory.NICE));
    }

    /**
     * Get the mean of the system CPU usage for a list of {@link CpuStats}.
     *
     * @param cpuStats the list of {@link CpuStats}
     * @return The average usage as a percentage (0 to 100).
     */
    public static Double getSystemPercentageMean(List<CpuStats> cpuStats) {
        return getPercentageMean(cpuStats, TimeCategory.SYS);
    }

    /**
     * Get the mean of the iow CPU usage for a list of {@link CpuStats}.
     *
     * @param cpuStats the list of {@link CpuStats}
     * @return The average usage as a percentage (0 to 100).
     */
    public static Double getIowPercentageMean(List<CpuStats> cpuStats) {
        return getPercentageMean(cpuStats, TimeCategory.IOW);
    }

    /**
     * Get the mean of the IRQ and SIRQ CPU usage for a list of {@link CpuStats}.
     *
     * @param cpuStats the list of {@link CpuStats}
     * @return The average usage as a percentage (0 to 100).
     */
    public static Double getIrqPercentageMean(List<CpuStats> cpuStats) {
        return (getPercentageMean(cpuStats, TimeCategory.IRQ) +
                getPercentageMean(cpuStats, TimeCategory.SIRQ));
    }

    /**
     * Get the mean of the estimated MHz for a list of {@link CpuStats}.
     *
     * @param cpuStats the list of {@link CpuStats}
     * @return The average estimated MHz in MHz.
     * @see CpuStats#getEstimatedMhz()
     */
    public static Double getEstimatedMhzMean(List<CpuStats> cpuStats) {
        SimpleStats stats = new SimpleStats();
        for (CpuStats s : cpuStats) {
            if (!s.mFreqStats.isEmpty()) {
                stats.add(s.getEstimatedMhz());
            }
        }
        return stats.mean();
    }

    /**
     * Get the mean of the used MHz for a list of {@link CpuStats}.
     *
     * @param cpuStats the list of {@link CpuStats}
     * @return The average used MHz as a percentage (0 to 100).
     * @see CpuStats#getUsedMhzPercentage()
     */
    public static Double getUsedMhzPercentageMean(List<CpuStats> cpuStats) {
        SimpleStats stats = new SimpleStats();
        for (CpuStats s : cpuStats) {
            if (!s.mFreqStats.isEmpty()) {
                stats.add(s.getUsedMhzPercentage());
            }
        }
        return stats.mean();
    }

    /**
     * Helper method for calculating the percentage mean for a {@link TimeCategory}.
     */
    private static Double getPercentageMean(List<CpuStats> cpuStats, TimeCategory category) {
        SimpleStats stats = new SimpleStats();
        for (CpuStats s : cpuStats) {
            stats.add(s.getPercentage(category));
        }
        return stats.mean();
    }

    /**
     * Method to access the receiver used to parse the cpu stats. Used for unit testing.
     */
    CpuStatsReceiver getReceiver() {
        return mReceiver;
    }
}
