/*
 * Copyright (C) 2014 The Android Open Source Project
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

package com.android.graphics.tests;

import com.android.tradefed.config.Option;
import com.android.tradefed.config.Option.Importance;
import com.android.tradefed.device.CollectingOutputReceiver;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.log.LogUtil.CLog;
import com.android.tradefed.result.ITestInvocationListener;
import com.android.tradefed.testtype.IDeviceTest;
import com.android.tradefed.testtype.IRemoteTest;
import com.android.tradefed.util.AbiFormatter;
import com.android.tradefed.util.RunUtil;
import com.android.tradefed.util.proto.TfMetricProtoUtil;

import org.junit.Assert;

import java.util.HashMap;
import java.util.Map;
import java.util.concurrent.TimeUnit;

/**
 * Test Runner for graphics Flatland Benchmark test.
 * <p>
 * Flatland test is a benchmark for measuring GPU performance in various 2D UI rendering and
 * window composition scenarios.
 * <p>
 * Since it is measuring the hardware performance, the test should be executed in
 * as consistent and static an environment as possible.
 * <ul>
 * <li>The display should be turned off and background service should be stopped before
 * running the benchmark. Running 'adb shell stop' is probably sufficient for this,
 * but if there are device specific background services that consume
 * much CPU cycles, memory bandwidth, or might otherwise interfere with GPU rendering,
 * those should be stopped as well
 * <li>All relevant hardware clocks should be locked at particular frequency when running the test.
 * </ul>
 * <p>
 * If running the benchmark with clocks locked causes thermal throttling, set option "--sleep-time"
 * to 10 to 50 (ms) to insert sleep between each benchmark sample run.
 * <p>
 * Output interpretation:
 * For each test case, the expected time in milliseconds that a single frame of the scenario
 * takes to complete will be printed out. Four types of values could displayed:
 * <ul>
 * <li>fast - frames of the scenarios are completed too fast to be reliably benchmarked. This
 * corresponds to frame time less than 3 ms. The scenario was skipped. "0" will be posted into
 * the dashboard.
 * <li>slow - frame time is too long, normally orver 50 ms. The scenario was skipped. "1000" will
 * be posted into the dashboard.
 * <li>varies - frame time was not stable. rerun the test to get a stable results. If that results
 * show repeatedly, something is wrong with the environment, signal to file a bug.
 * <li>decimal number - frame time for the scenarios are measured.
 * </ul>
 */
public class FlatlandTest implements IDeviceTest, IRemoteTest {

    private static final long SHELL_TIMEOUT = 30*60*1000;
    private static final String COMMAND = "flatland|#ABI32#|";
    private static final String FIRST_LINE = "cmdline:";
    private static final String TITLE = "Scenario";
    private static final long START_TIMER = 2 * 60 * 1000; // 2 minutes
    private static final String RESULT_FAST = "fast";
    private static final String RESULT_SLOW = "slow";
    private static final String RESULT_VARIES = "varies";

    private ITestDevice mTestDevice = null;
    // HashMap to store results for
    public Map<String, String> mResultMap = new HashMap<String, String>();

    @Option(name = "ru-key", description = "Reporting unit key to use when posting results")
    private String mRuKey = "flatland";

    @Option(name = "run-path",
            description = "path for the binary")
    private String mRunPath = "/data/local/tmp/";

    @Option(name = "sleep-time",
            description = "sleep for N ms between samples, set to 10 - 50 ms if the locked CPU"
                    + " frequency causes thermal throttle.")
    private int mSleepTime = 50;

    @Option(name = "schema-map",
            description = "map a test case name to a schema key")
    private Map<String, String> mSchemaMap = new HashMap<String, String>();

    @Option(name = AbiFormatter.FORCE_ABI_STRING,
            description = AbiFormatter.FORCE_ABI_DESCRIPTION,
            importance = Importance.IF_UNSET)
    private String mForceAbi = null;

    @Override
    public void setDevice(ITestDevice testDevice) {
        mTestDevice = testDevice;
    }

    @Override
    public ITestDevice getDevice() {
        return mTestDevice;
    }

    @Override
    public void run(ITestInvocationListener standardListener) throws DeviceNotAvailableException {
        Assert.assertNotNull(mRunPath);
        RunUtil.getDefault().sleep(START_TIMER);

        // execute test
        StringBuilder cmd = new StringBuilder();
        cmd.append(mRunPath);
        cmd.append(COMMAND);
        if (mSleepTime > 0) {
            cmd.append(" -s ");
            cmd.append(mSleepTime);
        }
        standardListener.testRunStarted(mRuKey, 1);
        long start = System.currentTimeMillis();
        CollectingOutputReceiver receiver = new CollectingOutputReceiver();
        mTestDevice.executeShellCommand(AbiFormatter.formatCmdForAbi(cmd.toString(), mForceAbi),
                receiver, SHELL_TIMEOUT, TimeUnit.MILLISECONDS, 2);
        String result = receiver.getOutput();
        if (result == null) {
            CLog.v("no test results returned. Test failed?");
            return;
        }
        // parse results and report metrics
        parseResult(result);
        standardListener.testRunEnded(
                (System.currentTimeMillis() - start), TfMetricProtoUtil.upgradeConvert(mResultMap));
    }

    /**
     * Parse results returned from running the benchmark
     */
    public void parseResult(String result) {
        String[] lines = result.split(System.getProperty("line.separator"));
        if (lines.length <= 0) {
            return;
        }
        for (int i = 0; i < lines.length; i++) {
            if (!lines[i].contains(FIRST_LINE) && !(lines[i].contains(TITLE))) {
                // skip the first two lines
                String[] items = lines[i].trim().split("\\|");
                if (items.length == 3) {
                    String schemaKey = String.format("%s %s", items[0].trim(), items[1].trim());
                    if (mSchemaMap.get(schemaKey) != null) {
                        // get the mapped schema key if there is any
                        schemaKey = mSchemaMap.get(schemaKey);
                    }
                    String renderTime = items[2].trim();
                    if (renderTime != null) {
                        if (renderTime.equals(RESULT_FAST)) {
                            mResultMap.put(schemaKey, "0");
                        } else if (renderTime.equals(RESULT_SLOW)) {
                            mResultMap.put(schemaKey, "1000");
                        } else if (renderTime.equals(RESULT_VARIES)){
                            mResultMap.put(schemaKey, "-1");
                        } else {
                            mResultMap.put(schemaKey, renderTime);
                        }
                    }
                }
            }
        }
    }
}
