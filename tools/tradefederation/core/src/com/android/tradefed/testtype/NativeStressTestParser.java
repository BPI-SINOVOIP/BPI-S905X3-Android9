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
package com.android.tradefed.testtype;

import com.android.ddmlib.IShellOutputReceiver;
import com.android.ddmlib.Log;
import com.android.ddmlib.MultiLineReceiver;

import java.util.regex.Matcher;
import java.util.regex.Pattern;

/**
* A {@link IShellOutputReceiver} that parses the stress test data output, collecting metrics on
* number of iterations complete and average time per iteration.
* <p/>
* Looks for the following output
* <p/>
* <code>
* pass 0
* ...
* ==== pass X
* Successfully completed X passes
* </code>
* <br/>
* where 'X' refers to the iteration number
*/
public class NativeStressTestParser extends MultiLineReceiver {

    private final static String LOG_TAG = "NativeStressTestParser";

    private final static Pattern ITERATION_COMPLETE_PATTERN = Pattern.compile(
            "^====\\s*Completed\\s*pass:\\s*(\\d+)");

    private final String mTestRunName;
    private boolean mIsCanceled = false;
    private int mTotalIterations = 0;

    /**
     * Creates a {@link NativeStressTestParser}.
     *
     * @param runName the run name. Used for logging purposes.
     */
    public NativeStressTestParser(String runName) {
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
        Matcher matcher = ITERATION_COMPLETE_PATTERN.matcher(line);
        if (matcher.find()) {
            parseIterationValue(line, matcher.group(1));
        }
    }

    private void parseIterationValue(String line, String iterationString) {
        try {
            int currentIteration = Integer.parseInt(iterationString);
            Log.i(LOG_TAG, String.format("%s: pass %d", mTestRunName, currentIteration));
            mTotalIterations++;
        } catch (NumberFormatException e) {
            // this should never happen, since regular expression matches on digits
            Log.e(LOG_TAG, String.format("Unexpected iteration content %s", line));
        }
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public boolean isCancelled() {
        return mIsCanceled;
    }

    /**
     * @return the name of the test run.
     */
    public String getRunName() {
        return mTestRunName;
    }

    /**
     * @return the total number of iterations completed across one or more runs
     */
    public int getIterationsCompleted() {
        return mTotalIterations;
    }
}
