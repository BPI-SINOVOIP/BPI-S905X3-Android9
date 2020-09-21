/*
 * Copyright (C) 2016 The Android Open Source Project
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
package com.android.tradefed.targetprep;

import com.android.tradefed.build.IBuildInfo;
import com.android.tradefed.config.Option;
import com.android.tradefed.config.OptionClass;
import com.android.tradefed.device.CollectingOutputReceiver;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.log.ITestLogger;
import com.android.tradefed.log.LogUtil.CLog;
import com.android.tradefed.result.ByteArrayInputStreamSource;
import com.android.tradefed.result.ITestLoggerReceiver;
import com.android.tradefed.result.LogDataType;
import com.android.tradefed.util.StreamUtil;

import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.TimeUnit;

/**
 * A {@link ITargetCleaner} that runs 'dumpsys meminfo --unreachable -a' to identify the unreachable
 * native memory currently held by each process.
 *
 * <p>Note: this preparer requires N platform or newer.
 */
@OptionClass(alias = "native-leak-collector")
public class NativeLeakCollector extends BaseTargetPreparer
        implements ITestLoggerReceiver, ITargetCleaner {
    private static final String UNREACHABLE_MEMINFO_CMD = "dumpsys -t %d meminfo --unreachable -a";
    private static final String DIRECT_UNREACHABLE_CMD = "dumpsys -t %d %s --unreachable";
    private static final String OUTPUT_HEADER = "\nExecuted command: %s\n";

    private ITestLogger mTestLogger;

    @Option(name = "dump-timeout", description = "Timeout limit for dumping unreachable native "
            + "memory allocation information. Can be in any valid duration format, e.g. 5m, 1h.",
            isTimeVal = true)
    private long mDumpTimeout = 5 * 60 * 1000; // defaults to 5m

    @Option(name = "log-filename", description = "The filename to give this log.")
    private String mLogFilename = "unreachable-meminfo";

    @Option(name = "additional-proc", description = "A list indicating any additional names to "
            + "query for unreachable native memory.")
    private List<String> mAdditionalProc = new ArrayList<String>();

    @Option(name = "additional-dump-timeout", description = "An additional timeout limit for any "
            + "direct unreachable memory dump commands specified by the 'additional-procs' option. "
            + "Can be in any valid duration format, e.g. 5m, 1h.",
            isTimeVal = true)
    private long mAdditionalDumpTimeout = 1 * 60 * 1000; // defaults to 1m

    /**
     * {@inheritDoc}
     */
    @Override
    public void setUp(ITestDevice device, IBuildInfo buildInfo)
            throws TargetSetupError, BuildError, DeviceNotAvailableException {
        // No-op
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void tearDown(ITestDevice device, IBuildInfo buildInfo, Throwable e)
            throws DeviceNotAvailableException {
        if (isDisabled() || (e instanceof DeviceNotAvailableException)) {
            return;
        }

        CollectingOutputReceiver receiver = new CollectingOutputReceiver();
        String allCommand = String.format(UNREACHABLE_MEMINFO_CMD, mDumpTimeout / 1000);
        writeToReceiver(String.format(OUTPUT_HEADER, allCommand), receiver);
        device.executeShellCommand(allCommand, receiver, mDumpTimeout, TimeUnit.MILLISECONDS, 1);

        for (String proc : mAdditionalProc) {
            String procCommand = String.format(DIRECT_UNREACHABLE_CMD,
                    mAdditionalDumpTimeout / 1000, proc);
            writeToReceiver(String.format(OUTPUT_HEADER, procCommand), receiver);
            device.executeShellCommand(procCommand, receiver, mAdditionalDumpTimeout,
                    TimeUnit.MILLISECONDS, 1);
        }

        if (receiver.getOutput() != null && !receiver.getOutput().isEmpty()) {
            if (mTestLogger != null) {
                ByteArrayInputStreamSource byteOutput =
                        new ByteArrayInputStreamSource(receiver.getOutput().getBytes());
                mTestLogger.testLog(mLogFilename, LogDataType.TEXT, byteOutput);
                StreamUtil.cancel(byteOutput);
            } else {
                CLog.w("No test logger available, printing output here:\n%s", receiver.getOutput());
            }
        }
    }

    private void writeToReceiver (String msg, CollectingOutputReceiver receiver) {
        byte[] msgBytes = msg.getBytes();
        int byteCount = msgBytes.length;
        receiver.addOutput(msgBytes, 0, byteCount);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void setTestLogger(ITestLogger testLogger) {
        mTestLogger = testLogger;
    }
}
