/*
 * Copyright (C) 2011 The Android Open Source Project
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
package com.android.tradefed.util;

import junit.framework.Test;
import junit.framework.TestCase;
import junit.framework.TestResult;

import java.io.File;
import java.io.InputStream;
import java.util.Collection;
import java.util.Collections;

/**
 * Test {@link TestLoader}
 */
public class TestLoaderTest extends TestCase {

    /**
     * Basic test for {@link TestLoader#loadTests(File, Collection)}.
     * <p/>
     * Extracts a tests.jar containing one test and one inner test, and ensures that (only) the
     * public test is loaded and run.
     */
    @SuppressWarnings("unchecked")
    public void testLoadTests() throws Exception {
        // extract the tests.jar from classpath into a tmp file
        InputStream testJarStream = getClass().getResourceAsStream("/util/tests.jar");
        File tmpJar = FileUtil.createTempFile("tests", ".jar");
        try {
            FileUtil.writeToFile(testJarStream, tmpJar);
            Test loadedSuite = new TestLoader().loadTests(tmpJar, Collections.EMPTY_LIST);
            assertNotNull(loadedSuite);
            TestResult result = new TestResult();
            loadedSuite.run(result);
            assertEquals(1, loadedSuite.countTestCases());
            assertEquals(1, result.runCount());
            assertTrue(result.wasSuccessful());

        } finally {
            tmpJar.delete();
        }
    }
}
