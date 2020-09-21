/*
 * Copyright (C) 2013 The Android Open Source Project
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

import com.android.tradefed.config.Option;
import com.android.tradefed.config.Option.Importance;
import com.android.tradefed.config.OptionClass;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.metrics.proto.MetricMeasurement.Metric;
import com.android.tradefed.result.ITestInvocationListener;
import com.android.tradefed.result.TestDescription;

import java.util.HashMap;
import java.util.LinkedHashMap;
import java.util.Map;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

/**
 * A fake test whose purpose is to make it easy to generate repeatable test results.
 */
@OptionClass(alias = "faketest")
public class FakeTest implements IDeviceTest, IRemoteTest {

    @Option(name = "run", description = "Specify a new run to include.  " +
            "The key should be the unique name of the TestRun (which may be a Java class name).  " +
            "The value should specify the sequence of test results, using the characters P[ass], " +
            "or F[ail].  You may use run-length encoding to specify repeats, and you " +
            "may use parentheses for grouping.  So \"(PF)4\" and \"((PF)2)2\" will both expand " +
            "to \"PFPFPFPF\".", importance = Importance.IF_UNSET)
    private Map<String, String> mRuns = new LinkedHashMap<String, String>();

    @Option(name = "fail-invocation-with-cause", description = "If set, the invocation will be " +
            "reported as a failure, with the specified message as the cause.")
    private String mFailInvocationWithCause = null;

    /** A pattern to identify an innermost pair of parentheses */
    private static final Pattern INNER_PAREN_SEGMENT = Pattern.compile(
    /*       prefix  inner parens    count     suffix */
            "(.*?)   \\(([^()]*)\\)   (\\d+)?   (.*?)", Pattern.COMMENTS);

    /** A pattern to identify a run-length-encoded character specification */
    private static final Pattern RLE_SEGMENT = Pattern.compile("^(([PFE])(\\d+)?)");

    static final HashMap<String, Metric> EMPTY_MAP = new HashMap<String, Metric>();

    private ITestDevice mDevice = null;

    /**
     * {@inheritDoc}
     */
    @Override
    public ITestDevice getDevice() {
        return mDevice;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void setDevice(ITestDevice device) {
        mDevice = device;
    }

    /**
     * A small utility that converts a number encoded in a string to an int.  Will convert
     * {@code null} to {@code defValue}.
     */
    int toIntOrDefault(String number, int defValue) throws IllegalArgumentException {
        if (number == null) return defValue;
        try {
            return Integer.parseInt(number);
        } catch (NumberFormatException e) {
            throw new IllegalArgumentException(e);
        }
    }

    /**
     * Decode a possibly run-length-encoded section of a run specification
     */
    String decodeRle(String encoded) throws IllegalArgumentException {
        final StringBuilder out = new StringBuilder();

        int i = 0;
        while (i < encoded.length()) {
            Matcher m = RLE_SEGMENT.matcher(encoded.substring(i));
            if (m.find()) {
                final String c = m.group(2);
                final int repeat = toIntOrDefault(m.group(3), 1);
                if (repeat < 1) {
                    throw new IllegalArgumentException(String.format(
                            "Encountered illegal repeat length %d; expecting a length >= 1",
                            repeat));
                }

                for (int k = 0; k < repeat; ++k) {
                    out.append(c);
                }

                // jump forward by the length of the entire match from the encoded string
                i += m.group(1).length();
            } else {
                throw new IllegalArgumentException(String.format(
                        "Encountered illegal character \"%s\" while parsing segment \"%s\"",
                        encoded.substring(i, i+1), encoded));
            }
        }

        return out.toString();
    }

    /**
     * Decode the run specification
     */
    String decode(String encoded) throws IllegalArgumentException {
        String work = encoded.toUpperCase();

        // The first step is to get expand parenthesized sections so that we have one long RLE
        // string
        Matcher m = INNER_PAREN_SEGMENT.matcher(work);
        for (; m.matches(); m = INNER_PAREN_SEGMENT.matcher(work)) {
            final String prefix = m.group(1);
            final String subsection = m.group(2);
            final int repeat = toIntOrDefault(m.group(3), 1);
            if (repeat < 1) {
                throw new IllegalArgumentException(String.format(
                        "Encountered illegal repeat length %d; expecting a length >= 1",
                        repeat));
            }
            final String suffix = m.group(4);

            // At this point, we have a valid next state.  Just reassemble everything
            final StringBuilder nextState = new StringBuilder(prefix);
            for (int k = 0; k < repeat; ++k) {
                nextState.append(subsection);
            }
            nextState.append(suffix);
            work = nextState.toString();
        }

        // Finally, decode the long RLE string
        return decodeRle(work);
    }


    /**
     * Turn a given test specification into a series of test Run, Failure, and Error outputs
     *
     * @param listener The test listener to use to report results
     * @param runName The test run name to use
     * @param spec A string consisting solely of the characters "P"(ass), "F"(ail), or "E"(rror).
     *     Each character will map to a testcase in the output. Method names will be of the format
     *     "testMethod%d".
     */
    void executeTestRun(ITestInvocationListener listener, String runName, String spec)
            throws IllegalArgumentException {
        listener.testRunStarted(runName, spec.length());
        int i = 0;
        for (char c : spec.toCharArray()) {
            if (c != 'P' && c != 'F') {
                throw new IllegalArgumentException(String.format(
                        "Received unexpected test spec character '%c' in spec \"%s\"", c, spec));
            }

            i++;
            final String testName = String.format("testMethod%d", i);
            final TestDescription test = new TestDescription(runName, testName);

            listener.testStarted(test);
            switch (c) {
                case 'P':
                    // no-op
                    break;
                case 'F':
                    listener.testFailed(test,
                            String.format("Test %s had a predictable boo-boo.", testName));
                    break;
            }
            listener.testEnded(test, EMPTY_MAP);
        }
        listener.testRunEnded(0, EMPTY_MAP);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void run(ITestInvocationListener listener) throws DeviceNotAvailableException {
        for (Map.Entry<String, String> run : mRuns.entrySet()) {
            final String name = run.getKey();
            final String testSpec = decode(run.getValue());
            executeTestRun(listener, name, testSpec);
        }

        if (mFailInvocationWithCause != null) {
            // Goodbye, cruel world
            throw new RuntimeException(mFailInvocationWithCause);
        }
    }
}

