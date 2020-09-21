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

package com.android.tradefed.testtype;

import com.android.tradefed.result.LogDataType;

/** The output formats supported by the jacoco code coverage report tool. */
enum JacocoCodeCoverageReportFormat implements CodeCoverageReportFormat {
    CSV("<csv destfile=\"%s\" />", LogDataType.JACOCO_CSV),
    XML("<xml destfile=\"%s\" />", LogDataType.JACOCO_XML),
    HTML("<html destdir=\"%s\" />", LogDataType.HTML);

    private final String mAntTagFormat;
    private final LogDataType mLogDataType;

    /**
     * Initialize a JacocoCodeCoverageReportFormat.

     * @param antTagFormat The ant tag to be passed to build.xml
     * @param logDataType The {@link LogDataType} of the generated report.
     */
    private JacocoCodeCoverageReportFormat(String antTagFormat, LogDataType logDataType) {
        mAntTagFormat = antTagFormat;
        mLogDataType = logDataType;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public LogDataType getLogDataType() {
        return mLogDataType;
    }

    /**
     * Returns the ant tag format used to configure build.xml
     */
    public String getAntTagFormat() {
        return mAntTagFormat;
    }
}
