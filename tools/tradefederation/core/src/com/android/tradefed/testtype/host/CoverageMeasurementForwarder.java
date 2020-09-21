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
package com.android.tradefed.testtype.host;

import static com.google.common.base.Preconditions.checkNotNull;

import com.android.tradefed.build.IBuildInfo;
import com.android.tradefed.config.Option;
import com.android.tradefed.config.Option.Importance;
import com.android.tradefed.metrics.proto.MetricMeasurement.Metric;
import com.android.tradefed.result.FileInputStreamSource;
import com.android.tradefed.result.ITestInvocationListener;
import com.android.tradefed.result.InputStreamSource;
import com.android.tradefed.result.LogDataType;
import com.android.tradefed.testtype.IBuildReceiver;
import com.android.tradefed.testtype.IRemoteTest;
import com.android.tradefed.util.FileUtil;

import com.google.common.annotations.VisibleForTesting;

import java.io.File;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;

/** A dummy test that fowards coverage measurements from the build provider to the logger. */
public final class CoverageMeasurementForwarder implements IRemoteTest, IBuildReceiver {

    @Option(
        name = "coverage-measurement",
        description =
                "The name of the build artifact to forward. The artifact should be a "
                        + "coverage measurement (either an .ec or .exec file) that to save as a "
                        + "test result. This option may be repeated.",
        importance = Importance.IF_UNSET,
        mandatory = false
    )
    private List<String> mCoverageMeasurements = new ArrayList<>();

    private IBuildInfo mBuild;

    /** Sets the --coverage-measurement option for testing. */
    @VisibleForTesting
    void setCoverageMeasurements(List<String> coverageMeasurements) {
        mCoverageMeasurements = coverageMeasurements;
    }

    @Override
    public void setBuild(IBuildInfo buildInfo) {
        mBuild = buildInfo;
    }

    /** Returns the {@link IBuildInfo} for this invocation. */
    private IBuildInfo getBuild() {
        return mBuild;
    }

    @Override
    public void run(ITestInvocationListener listener) {
        if (mCoverageMeasurements.isEmpty()) {
            return;
        }

        listener.testRunStarted("CoverageMeasurementedForwarder", 0);
        for (String artifactName : mCoverageMeasurements) {
            File coverageMeasurement =
                    checkNotNull(
                            getBuild().getFile(artifactName),
                            "Failed to get artifact '%s' from the build.",
                            artifactName);
            try (InputStreamSource stream = new FileInputStreamSource(coverageMeasurement)) {
                listener.testLog(artifactName, LogDataType.COVERAGE, stream);
            } finally {
                FileUtil.deleteFile(coverageMeasurement);
            }
        }
        listener.testRunEnded(0, new HashMap<String, Metric>());
    }
}
