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
package com.android.tradefed.testtype;

import com.android.ddmlib.IShellOutputReceiver;
import com.android.ddmlib.Log;
import com.android.ddmlib.MultiLineReceiver;

import java.text.DecimalFormat;
import java.text.ParseException;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

/**
* A {@link IShellOutputReceiver} that parses the benchmark test data output, collecting metrics on
* average time per operation.
* <p/>
* Looks for the following output
* <p/>
* <code>
* Time per iteration min: X avg: Y max: Z
* </code>
*/
public class NativeBenchmarkTestParser extends MultiLineReceiver {

    private final static String LOG_TAG = "NativeBenchmarkTestParser";

    // just parse any string
    private final static String FLOAT_STRING = "\\s*(.*)\\s*";
    private final static String COMPLETE_STRING = String.format(
            "Time per iteration min:%savg:%smax:%s", FLOAT_STRING, FLOAT_STRING, FLOAT_STRING);
    private final static Pattern COMPLETE_PATTERN = Pattern.compile(COMPLETE_STRING);

    private final String mTestRunName;
    private boolean mIsCanceled = false;
    private double mMinOpTime = 0;
    private double mAvgOpTime = 0;
    private double mMaxOpTime = 0;

    /**
     * Creates a {@link NativeBenchmarkTestParser}.
     *
     * @param runName the run name. Used for logging purposes.
     */
    public NativeBenchmarkTestParser(String runName) {
        mTestRunName = runName;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void processNewLines(String[] lines) {
        for (String line : lines) {
            parseLine(line);
        }
    }

    private void parseLine(String line) {
        Log.d(LOG_TAG, line);
        Matcher matcher = COMPLETE_PATTERN.matcher(line);
        if (matcher.find()) {
            Log.i(LOG_TAG, String.format("Found result for benchmark %s: %s", getRunName(), line));
            mMinOpTime = parseDoubleValue(line, matcher.group(1));
            mAvgOpTime = parseDoubleValue(line, matcher.group(2));
            mMaxOpTime = parseDoubleValue(line, matcher.group(3));
        }
    }

    private double parseDoubleValue(String line, String valueString) {
        try {
            return Double.parseDouble(valueString);
        } catch (NumberFormatException e) {
            // fall through
        }
        Log.w(LOG_TAG, String.format("Value was not a double (%s), trying for scientfic",
                line));
        DecimalFormat format = new DecimalFormat("0.0E0");
        try {
            Number num = format.parse(valueString);
            return num.doubleValue();
        } catch (ParseException e) {
            Log.e(LOG_TAG, String.format("Could not parse double value in (%s)",
                    line));
        }
        return 0;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public boolean isCancelled() {
        return mIsCanceled;
    }

    /**
     * @return The name of the Test Run.
     */
    public String getRunName() {
        return mTestRunName;
    }

    /**
     * @return the average operation time
     */
    public double getAvgOperationTime() {
        return mAvgOpTime;
    }

    /**
     * @return the minimum operation time
     */
    public double getMinOperationTime() {
        return mMinOpTime;
    }

    /**
     * @return the maximum operation time
     */
    public double getMaxOperationTime() {
        return mMaxOpTime;
    }
}
