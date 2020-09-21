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
package android.device.loggers;

import android.device.collectors.BaseMetricListener;
import android.device.collectors.DataRecord;
import android.device.collectors.util.SendToInstrumentation;

import org.junit.runner.Description;

import java.lang.annotation.Annotation;

/**
 * Logger that read information from the {@link TestLogData} rule and send the logged file
 * to the instrumentation.
 */
public class LogFileLogger extends BaseMetricListener {

    @Override
    public void onTestEnd(DataRecord testData, Description description) {
        for (Description child : description.getChildren()) {
            for (Annotation a : child.getAnnotations()) {
                if (a instanceof TestLogData.LogAnnotation) {
                    // Log all the logs found.
                    for (TestLogData.LogHolder log : ((TestLogData.LogAnnotation) a).mLogs) {
                        SendToInstrumentation.sendFile(
                                getInstrumentation(), log.mDataName, log.mLogFile);
                    }
                    ((TestLogData.LogAnnotation) a).mLogs.clear();
                }
            }
        }
    }
}
