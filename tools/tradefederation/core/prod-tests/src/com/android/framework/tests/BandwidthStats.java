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

package com.android.framework.tests;

import com.android.tradefed.log.LogUtil.CLog;

import java.util.ArrayList;
import java.util.Collection;
import java.util.List;

/**
 * Simple container class used to store network Stats.
 */
public class BandwidthStats {
    private long mRxBytes = 0;
    private long mRxPackets = 0;
    private long mTxBytes = 0;
    private long mTxPackets = 0;

    public BandwidthStats (long rxBytes, long rxPackets, long txBytes, long txPackets) {
        mRxBytes = rxBytes;
        mRxPackets = rxPackets;
        mTxBytes = txBytes;
        mTxPackets = txPackets;
    }

    public BandwidthStats() {
    }

    /**
     * Compute percent difference between a and b.
     * @param a
     * @param b
     * @return % difference of a and b.
     */
    static float computePercentDifference(float a, float b) {
        if (a == b) {
            return 0;
        }
        if (a == 0) {
            CLog.d("Invalid value for a: %f", a);
            return Float.MIN_VALUE;
        }
        return ( a - b) / a * 100;
    }

    public long getRxBytes() {
        return mRxBytes;
    }

    public void setRxBytes(long rxBytes) {
        this.mRxBytes = rxBytes;
    }

    public long getRxPackets() {
        return mRxPackets;
    }

    public void setRxPackets(long rxPackets) {
        this.mRxPackets = rxPackets;
    }

    public long getTxBytes() {
        return mTxBytes;
    }

    public void setTxBytes(long txBytes) {
        this.mTxBytes = txBytes;
    }

    public long getTxPackets() {
        return mTxPackets;
    }

    public void setTxPackets(long txPackets) {
        this.mTxPackets = txPackets;
    }

    public CompareResult compareAll(BandwidthStats stats, float mDifferenceThreshold) {
        CompareResult result = new CompareResult();
        result.addRecord(this.compareRb(stats), mDifferenceThreshold);
        result.addRecord(this.compareRp(stats), mDifferenceThreshold);
        result.addRecord(this.compareTb(stats), mDifferenceThreshold);
        result.addRecord(this.compareTp(stats), mDifferenceThreshold);
        return result;
    }

    private ComparisonRecord compareTp(BandwidthStats stats) {
        float difference = BandwidthStats.computePercentDifference(
                this.mTxPackets, stats.mTxPackets);

        ComparisonRecord result = new ComparisonRecord("Tp", this.mTxPackets, stats.mTxPackets,
                difference);

        return result;
    }

    private ComparisonRecord compareTb(BandwidthStats stats) {
        float difference = BandwidthStats.computePercentDifference(
                this.mTxBytes, stats.mTxBytes);

        ComparisonRecord result = new ComparisonRecord("Tb", this.mTxBytes, stats.mTxBytes,
                difference);

        return result;
    }

    private ComparisonRecord compareRp(BandwidthStats stats) {
        float difference = BandwidthStats.computePercentDifference(
                this.mRxPackets, stats.mRxPackets);

        ComparisonRecord result = new ComparisonRecord("Rp", this.mRxPackets, stats.mRxPackets,
                difference);

        return result;
    }

    private ComparisonRecord compareRb(BandwidthStats stats) {
        float difference = BandwidthStats.computePercentDifference(
                this.mRxBytes, stats.mRxBytes);

        ComparisonRecord result = new ComparisonRecord("Rb", this.mRxBytes, stats.mRxBytes,
                difference);

        return result;
    }

    /**
     * Holds the record of comparing a single stat like transmitted packets. All comparisons are
     * recorded so it is easier to see which one failed later.
     */
    public static class ComparisonRecord {
        private String mStatName;
        private long mFirst;
        private long mSecond;
        private float mDifference;

        public ComparisonRecord(String statName, long first, long second, float difference) {
            this.mStatName = statName;
            this.mFirst = first;
            this.mSecond = second;
            this.mDifference = difference;
        }

        public String getStatName() {
            return mStatName;
        }

        public long getFirst() {
            return mFirst;
        }

        public long getSecond() {
            return mSecond;
        }

        public float getDifference() {
            return mDifference;
        }

        @Override
        public String toString() {
            return String.format("%s expected %d actual %d difference is %f%%",
                    mStatName, mFirst, mSecond, mDifference);
        }
    }

    /**
     * Holds the result of comparing bandwidth stats from different sources. The result is passed
     * if all individual stats are within a threshold. Also keeps track of which individual stats
     * failed the comparison for debugging purposes.
     *
     */
    public static class CompareResult {
        private boolean mPassed;
        private List<ComparisonRecord> mFailures;

        public CompareResult() {
            mPassed = true;
            mFailures = new ArrayList<ComparisonRecord>();
        }

        public void addRecord(ComparisonRecord record, float threshold) {
            if (record.getDifference() < threshold) {
                mPassed &= true;
            } else {
                mPassed = false;
                mFailures.add(record);
            }
        }

        public boolean getResult() {
            return mPassed;
        }

        public Collection<ComparisonRecord> getFailures() {
            return mFailures;
        }
    }
}
