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
package android.device.collectors;

import org.junit.runner.Description;
import org.junit.runner.Result;
import org.junit.runner.notification.Failure;

/**
 * A Stub class for testing.
 */
public class StubTestMetricListener extends BaseMetricListener {

    @Override
    public void onTestRunStart(DataRecord runData, Description description) {
        runData.addStringMetric("run_start", "run_Start" + description.getClassName());
    }

    @Override
    public void onTestRunEnd(DataRecord runData, Result result) {
        runData.addStringMetric("run_end", "run_end");
    }

    @Override
    public void onTestStart(DataRecord testData, Description description) {
        testData.addStringMetric("test_start", "test_start" + description.getMethodName());
    }

    @Override
    public void onTestFail(DataRecord testData, Description description, Failure failure) {
        testData.addStringMetric("test_fail", "test_fail" + description.getMethodName());
    }

    @Override
    public void onTestEnd(DataRecord testData, Description description) {
        testData.addStringMetric("test_end", "test_end" + description.getMethodName());
    }
}