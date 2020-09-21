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

package com.android.tradefed.testtype;

import com.android.tradefed.config.Option;
import com.android.tradefed.config.OptionClass;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.log.LogUtil.CLog;
import com.android.tradefed.result.CollectingTestListener;
import com.android.tradefed.result.FileInputStreamSource;
import com.android.tradefed.result.ITestInvocationListener;
import com.android.tradefed.result.InputStreamSource;
import com.android.tradefed.result.LogDataType;
import com.android.tradefed.result.ResultForwarder;
import com.android.tradefed.result.TestRunResult;
import com.android.tradefed.util.FileUtil;
import com.android.tradefed.util.RunUtil;

import java.io.File;
import java.util.Map;

/**
 * A Test that runs an instrumentation test package on a given device and
 * generates the code coverage report. Requires an emma instrumented
 * application.
 */
@OptionClass(alias = "code-coverage")
public class CodeCoverageTest extends InstrumentationTest {

    @Option(name = "coverage-file",
            description = "Optional custom emma coverage file path. " +
                    "If unspecified, will use package name.")
    private String mCoverageFile = null;

    public static final String COVERAGE_REMOTE_FILE_LABEL = "coverageFilePath";

    @Override
    public void run(final ITestInvocationListener listener) throws DeviceNotAvailableException {
        // Disable rerun mode, we want to stop the tests as soon as we fail.
        super.setRerunMode(false);
        // Force generation of emma coverage file to true and set up coverage
        // file path.
        super.addInstrumentationArg("coverage", "true");
        if (mCoverageFile != null) {
            super.addInstrumentationArg("coverageFile", mCoverageFile);
        }

        CollectingTestListener testCoverageFile = new CollectingTestListener();

        // Run instrumentation tests.
        super.run(new ResultForwarder(listener, testCoverageFile));

        // If coverage file path was not set explicitly before test run, fetch
        // it from the
        // instrumentation out.
        if (mCoverageFile == null) {
            mCoverageFile = fetchCoverageFilePath(testCoverageFile);
        }
        CLog.d("Coverage file at %s", mCoverageFile);
        File coverageFile = null;
        try {
            if (getDevice().doesFileExist(mCoverageFile)) {
                coverageFile = getDevice().pullFile(mCoverageFile);
                if (coverageFile != null) {
                    CLog.d("coverage file from device: %s", coverageFile.getAbsolutePath());
                    try (FileInputStreamSource source = new FileInputStreamSource(coverageFile)) {
                        listener.testLog(
                                getPackageName() + "_runtime_coverage",
                                LogDataType.COVERAGE,
                                source);
                    }
                }
            } else {
                CLog.w("Missing coverage file %s. Did test crash?", mCoverageFile);
                RunUtil.getDefault().sleep(2000);
                // grab logcat snapshot when this happens
                try (InputStreamSource s = getDevice().getLogcat(500 * 1024)) {
                    listener.testLog(
                            getPackageName() + "_coverage_crash_" + getTestSize(),
                            LogDataType.LOGCAT,
                            s);
                }
            }
        } finally {
            if (coverageFile != null) {
                FileUtil.deleteFile(coverageFile);
            }
        }
    }

    /**
     * Returns the runtime coverage file path from instrumentation test metrics.
     */
    private String fetchCoverageFilePath(CollectingTestListener listener) {
        TestRunResult runResult = listener.getCurrentRunResults();
        Map<String, String> metrics = runResult.getRunMetrics();
        return metrics.get(COVERAGE_REMOTE_FILE_LABEL);
    }
}
