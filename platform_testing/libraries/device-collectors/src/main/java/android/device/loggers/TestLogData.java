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

import org.junit.rules.ExternalResource;
import org.junit.rules.TestRule;
import org.junit.runner.Description;
import org.junit.runners.model.Statement;

import java.io.File;
import java.lang.annotation.Annotation;
import java.util.ArrayList;
import java.util.List;

/**
 * Implementation of {@link ExternalResource} and {@link TestRule}. This rule allows to log logs
 * during a test case (inside @Test). It guarantees that the log list is cleaned between tests,
 * so the same rule object can be re-used.
 *
 * <pre>Example:
 * &#064;Rule
 * public TestLogData logs = new TestLogData();
 *
 * &#064;Test
 * public void testFoo() {
 *     logs.addTestLog("logcat", LogDataType.LOGCAT, new FileInputStreamSource(logcatFile));
 * }
 *
 * &#064;Test
 * public void testFoo2() {
 *     logs.addTestLog("logcat2", LogDataType.LOGCAT, new FileInputStreamSource(logcatFile2));
 * }
 * </pre>
 *
 * TODO: The naming is in sync with the Tradefed host-side one, but they should be refactored in a
 * single place.
 */
public class TestLogData extends ExternalResource {
    private Description mDescription;
    private List<LogHolder> mLogs = new ArrayList<>();

    @Override
    public Statement apply(Statement base, Description description) {
        mDescription = description;
        return super.apply(base, description);
    }

    public final void addTestLog(String dataName, File logFile) {
        mLogs.add(new LogHolder(dataName, logFile));
    }

    @Override
    protected void after() {
        // we inject a Description with an annotation carrying metrics.
        // Since Description cannot be extended and RunNotifier does not give us a lot of
        // flexibility to find our metrics back, we have to pass the information via a placeholder
        // annotation in the Description.
        mDescription.addChild(
                Description.createTestDescription("LOGS", "LOGS", new LogAnnotation(mLogs)));
    }

    /** Fake annotation meant to carry logs to the reporters. */
    static class LogAnnotation implements Annotation {

        public List<LogHolder> mLogs = new ArrayList<>();

        public LogAnnotation(List<LogHolder> logs) {
            mLogs.addAll(logs);
        }

        @Override
        public Class<? extends Annotation> annotationType() {
            return null;
        }
    }

    /** Structure to hold a log file to be reported. */
    static class LogHolder {
        public final String mDataName;
        public final File mLogFile;

        public LogHolder(String dataName, File logFile) {
            mDataName = dataName;
            mLogFile = logFile;
        }
    }
}
