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

package com.android.tradefed.testtype;

import com.android.annotations.VisibleForTesting;
import com.android.tradefed.command.CommandFileParser;
import com.android.tradefed.command.CommandFileParser.CommandLine;
import com.android.tradefed.config.ConfigurationException;
import com.android.tradefed.config.ConfigurationFactory;
import com.android.tradefed.config.IConfiguration;
import com.android.tradefed.config.Option;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.log.LogUtil.CLog;
import com.android.tradefed.metrics.proto.MetricMeasurement.Metric;
import com.android.tradefed.result.ITestInvocationListener;
import com.android.tradefed.result.TestDescription;
import com.android.tradefed.util.QuotationAwareTokenizer;
import com.android.tradefed.util.RunUtil;
import com.android.tradefed.util.StreamUtil;
import com.android.tradefed.util.keystore.DryRunKeyStore;

import java.io.File;
import java.io.IOException;
import java.util.HashMap;
import java.util.List;

/**
 * Run noisy dry run on a command file.
 */
public class NoisyDryRunTest implements IRemoteTest {

    private static final long SLEEP_INTERVAL_MILLI_SEC = 5 * 1000;

    @Option(name = "cmdfile", description = "The cmdfile to run noisy dry run on.")
    private String mCmdfile = null;

    @Option(name = "timeout",
            description = "The timeout to wait cmd file be ready.",
            isTimeVal = true)
    private long mTimeoutMilliSec = 0;

    @Override
    public void run(ITestInvocationListener listener) throws DeviceNotAvailableException {
        List<CommandLine> commands = testCommandFile(listener, mCmdfile);
        if (commands != null) {
            testCommandLines(listener, commands);
        }
    }

    private List<CommandLine> testCommandFile(ITestInvocationListener listener, String filename) {
        listener.testRunStarted(NoisyDryRunTest.class.getCanonicalName() + "_parseFile", 1);
        TestDescription parseFileTest =
                new TestDescription(NoisyDryRunTest.class.getCanonicalName(), "parseFile");
        listener.testStarted(parseFileTest);
        CommandFileParser parser = new CommandFileParser();
        try {
            File file = new File(filename);
            checkFileWithTimeout(file);
            return parser.parseFile(file);
        } catch (IOException | ConfigurationException e) {
            listener.testFailed(parseFileTest, StreamUtil.getStackTrace(e));
            return null;
        } finally {
            listener.testEnded(parseFileTest, new HashMap<String, Metric>());
            listener.testRunEnded(0, new HashMap<String, Metric>());
        }
    }

    /**
     * If the file doesn't exist, we want to wait a while and check.
     *
     * @param file
     * @throws IOException
     */
    @VisibleForTesting
    void checkFileWithTimeout(File file) throws IOException {
        long timeout = currentTimeMillis() + mTimeoutMilliSec;
        while (!file.exists() && currentTimeMillis() < timeout) {
            CLog.w("%s doesn't exist, wait and recheck.", file.getAbsoluteFile());
            sleep();
        }
        if (!file.exists()) {
            throw new IOException(
                    String.format("%s doesn't exist.", file.getAbsoluteFile()));
        }
    }

    @VisibleForTesting
    long currentTimeMillis() {
        return System.currentTimeMillis();
    }

    @VisibleForTesting
    void sleep() {
        RunUtil.getDefault().sleep(SLEEP_INTERVAL_MILLI_SEC);
    }

    private void testCommandLines(ITestInvocationListener listener, List<CommandLine> commands) {
        listener.testRunStarted(NoisyDryRunTest.class.getCanonicalName() + "_parseCommands",
                commands.size());
        for (int i = 0; i < commands.size(); ++i) {
            TestDescription parseCmdTest =
                    new TestDescription(
                            NoisyDryRunTest.class.getCanonicalName(), "parseCommand" + i);
            listener.testStarted(parseCmdTest);

            String[] args = commands.get(i).asArray();
            String cmdLine = QuotationAwareTokenizer.combineTokens(args);
            try {
                // Use dry run keystore to always work for any keystore.
                // FIXME: the DryRunKeyStore is a temporary fixed until each config can be validated
                // against its own keystore.
                IConfiguration config =
                        ConfigurationFactory.getInstance()
                                .createConfigurationFromArgs(args, null, new DryRunKeyStore());
                config.validateOptions();
            } catch (ConfigurationException e) {
                CLog.e("Failed to parse comand line: %s.", cmdLine);
                CLog.e(e);
                listener.testFailed(parseCmdTest, StreamUtil.getStackTrace(e));
            } finally {
                listener.testEnded(parseCmdTest, new HashMap<String, Metric>());
            }
        }
        listener.testRunEnded(0, new HashMap<String, Metric>());
    }
}
