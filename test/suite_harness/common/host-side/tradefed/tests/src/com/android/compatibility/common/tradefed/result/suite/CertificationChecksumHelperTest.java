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
package com.android.compatibility.common.tradefed.result.suite;


import static org.junit.Assert.assertTrue;

import com.android.compatibility.common.util.ChecksumReporter.ChecksumValidationException;
import com.android.tradefed.metrics.proto.MetricMeasurement.Metric;
import com.android.tradefed.result.TestDescription;
import com.android.tradefed.result.TestResult;
import com.android.tradefed.result.TestRunResult;
import com.android.tradefed.util.FileUtil;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

import java.io.File;
import java.util.ArrayList;
import java.util.Collection;
import java.util.HashMap;
import java.util.Map.Entry;

/**
 * Unit tests for {@link CertificationChecksumHelper}.
 */
@RunWith(JUnit4.class)
public class CertificationChecksumHelperTest {
    private final static String FINGERPRINT = "thisismyfingerprint";
    private File mWorkingDir;
    private File mFakeLogFile;

    @Before
    public void setUp() throws Exception {
        mWorkingDir = FileUtil.createTempDir("certification-tests");
        mFakeLogFile = FileUtil.createTempFile("fake-log-file", ".xml", mWorkingDir);
        FileUtil.writeToFile("Bunch of data to make the file unique", mFakeLogFile);
    }

    @After
    public void tearDown() {
        FileUtil.recursiveDelete(mWorkingDir);
    }

    /**
     * Validation that the checksum generated can be properly checked when re-reading the file.
     */
    @Test
    public void testCreateChecksum() throws ChecksumValidationException {
        Collection<TestRunResult> results = new ArrayList<>();
        TestRunResult run1 = createFakeResults("run1", 2);
        results.add(run1);
        TestRunResult run2 = createFakeResults("run2", 3);
        results.add(run2);
        boolean res = CertificationChecksumHelper.tryCreateChecksum(
                mWorkingDir, results, FINGERPRINT);
        assertTrue(res);
        // Attempt to parse the results back
        File checksum = new File(mWorkingDir, CertificationChecksumHelper.NAME);
        assertTrue(checksum.exists());
        CertificationChecksumHelper parser = new CertificationChecksumHelper(
                mWorkingDir, FINGERPRINT);
        // Assert that the info are checkable
        assertTrue(parser.containsFile(mFakeLogFile, mWorkingDir.getName()));
        // Check run1
        for (Entry<TestDescription, TestResult> entry : run1.getTestResults().entrySet()) {
            assertTrue(parser.containsTestResult(entry, run1, FINGERPRINT));
        }
        // Check run2
        for (Entry<TestDescription, TestResult> entry : run2.getTestResults().entrySet()) {
            assertTrue(parser.containsTestResult(entry, run2, FINGERPRINT));
        }
    }

    private TestRunResult createFakeResults(String runName, int testCount) {
        TestRunResult results = new TestRunResult();
        results.testRunStarted(runName, testCount);
        for (int i = 0; i < testCount; i++) {
            TestDescription test = new TestDescription("com.class.path", "testMethod" + i);
            results.testStarted(test);
            results.testEnded(test, new HashMap<String, Metric>());
        }
        results.testRunEnded(500L, new HashMap<String, Metric>());
        return results;
    }
}
