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
package com.android.tradefed.util;

import java.util.Collection;
import java.util.Collections;
import java.util.LinkedList;
import java.util.List;

/**
 * A small utility class that calculates a few statistical measures given a numerical dataset.  The
 * values are stored internally as {@link Double}s.
 */
public class SimpleStats {
    private List<Double> mData = new LinkedList<Double>();

    // cached values
    private double mSum = 0;

    /**
     * Add a number of measurements to the dataset.
     *
     * @throws NullPointerException if the collection contains any {@code null} elements
     */
    public void addAll(Collection<? extends Double> c) {
        for (Double meas : c) {
            if (meas == null) {
                throw new NullPointerException();
            }
            add(meas);
        }
    }

    /**
     * Add a measurement to the dataset.
     */
    public void add(double meas) {
        mData.add(meas);
        mSum += meas;
    }

    /**
     * Retrieve the dataset.
     */
    public List<Double> getData() {
        return mData;
    }

    /**
     * Check if the dataset is empty.
     */
    public boolean isEmpty() {
        return mData.isEmpty();
    }

    /**
     * Check how many elements are in the dataset.
     */
    public int size() {
        return mData.size();
    }

    /**
     * Calculate and return the mean of the dataset, or {@code null} if the dataset is empty.
     */
    public Double mean() {
        if (isEmpty()) {
            return null;
        }

        return mSum / size();
    }

    /**
     * Calculate and return the median of the dataset, or {@code null} if the dataset is empty.
     */
    public Double median() {
        if (isEmpty()) {
            return null;
        }

        Collections.sort(mData);
        if ((mData.size() & 0x1) == 1) {
            // odd count of items, pick the middle element.  Note that we don't +1 since indices
            // are zero-based rather than one-based
            int idx = size() / 2;
            return mData.get(idx);
        } else {
            // even count of items, average the two middle elements
            int idx = size() / 2;
            return (mData.get(idx - 1) + mData.get(idx)) / 2;
        }
    }

    /**
     * Return the minimum value in the dataset, or {@code null} if the dataset is empty.
     */
    public Double min() {
        if (isEmpty()) {
            return null;
        }

        Collections.sort(mData);
        return mData.get(0);
    }

    /**
     * Return the maximum value in the dataset, or {@code null} if the dataset is empty.
     */
    public Double max() {
        if (isEmpty()) {
            return null;
        }

        Collections.sort(mData);
        return mData.get(size() - 1);
    }

    /**
     * Return the standard deviation of the dataset, or {@code null} if the dataset is empty.
     * <p />
     * Note that this method calculates the population standard deviation, not the sample standard
     * deviation.  That is, it assumes that the dataset is entirely contained in the
     * {@link SimpleStats} instance.
     */
    public Double stdev() {
        if (isEmpty()) {
            return null;
        }

        Double avg = mean();
        Double ssd = 0.0;  // sum of squared differences
        for (Double meas : mData) {
            Double diff = meas - avg;
            ssd += diff * diff;
        }

        return Math.sqrt(ssd / size());
    }

    /**
     * return the average value of the samples that are within one stdev
     * e.g
     * 2.55 50.3 50.4 48.5 50.1 29.8 30 46 48 49
     * average: 40.45, stdev: 15.54
     * average of the values within one stdev is: 44.67
     */
    public Double meanOverOneStandardDeviationRange() {
        if (isEmpty()) {
            return null;
        }

        Double avg = mean();
        Double std = stdev();
        Double upper = avg + std;
        Double lower = avg - std;
        Double sum = 0.0;
        int count = 0;
        for (Double meas : mData) {
            if (meas > lower && meas < upper) {
                sum += meas;
                count++;
            }
        }
        return sum / count;
    }
}

