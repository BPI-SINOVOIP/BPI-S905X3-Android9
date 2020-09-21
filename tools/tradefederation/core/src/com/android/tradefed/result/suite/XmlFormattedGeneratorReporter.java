/*
 * Copyright (C) 2018 The Android Open Source Project
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
package com.android.tradefed.result.suite;

import com.android.tradefed.log.LogUtil.CLog;
import com.android.tradefed.util.FileUtil;

import java.io.File;
import java.io.IOException;

/**
 * Implementation of the {@link FormattedGeneratorReporter} which format the suite results in an xml
 * format.
 */
public class XmlFormattedGeneratorReporter extends FormattedGeneratorReporter {

    @Override
    public final void finalizeResults(
            IFormatterGenerator generator, SuiteResultHolder resultHolder) {
        File resultDir = null;
        try {
            resultDir = createResultDir();
        } catch (IOException e) {
            CLog.e("Failed to create the result directory:");
            CLog.e(e);
            return;
        }
        // Pre-formatting step that possibly add more information to the generator.
        preFormattingSetup(generator);

        File resultReportFile = null;
        try {
            resultReportFile = generator.writeResults(resultHolder, resultDir);
        } catch (IOException e) {
            CLog.e("Failed to generate the formatted report file:");
            CLog.e(e);
            return;
        }
        // Post-formatting step if something in particular needs to be done with the results.
        postFormattingStep(resultDir, resultReportFile);
    }

    /**
     * Pre formatting step that allows to take action before the report is generated.
     *
     * @param formater The {@link IFormatterGenerator} that will be used for the generation.
     */
    public void preFormattingSetup(IFormatterGenerator formater) {
        // Default implementation does nothing.
    }

    /**
     * Post formatting step that allows to take action after the report is generated.
     *
     * @param resultDir The directory containing the results.
     * @param reportFile The generated report file.
     */
    public void postFormattingStep(File resultDir, File reportFile) {
        // Default implementation does nothing.
    }

    /** Returns the result directory that should be used to store results. */
    public File createResultDir() throws IOException {
        return FileUtil.createTempDir("XmlFormattedGeneratorReporter");
    }

    /** Create the {@link IFormatterGenerator} to be used. Can be overriden to change the format. */
    @Override
    public IFormatterGenerator createFormatter() {
        return new XmlSuiteResultFormatter();
    }
}
