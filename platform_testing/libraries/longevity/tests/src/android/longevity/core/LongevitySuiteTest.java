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
package android.longevity.core;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.fail;

import android.longevity.core.scheduler.Iterate;

import java.util.HashMap;
import java.util.Map;

import org.junit.Test;
import org.junit.internal.builders.AllDefaultPossibilitiesBuilder;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;
import org.junit.runners.model.InitializationError;
import org.junit.runners.Suite.SuiteClasses;

/**
 * Unit tests for the {@link LongevitySuite} runner.
 */
@RunWith(JUnit4.class)
public class LongevitySuiteTest {
    private static final int TEST_ITERATIONS = 10000;

    /**
     * Unit test that the {@link SuiteClasses} annotation is required.
     */
    @Test
    public void testAnnotationRequired() {
        try {
            new LongevitySuite(NoSuiteClassesSuite.class, new AllDefaultPossibilitiesBuilder(true));
            fail("This suite should not be possible to construct.");
        } catch (InitializationError e) {
            // ignore and pass.
        }
    }

    @RunWith(LongevitySuite.class)
    private static class NoSuiteClassesSuite { }

    /**
     * Tests that the {@link LongevitySuite} properly accounts for the number of tests in children.
     */
    @Test
    public void testChildAccounting() throws InitializationError {
        Map<String, String> args = new HashMap();
        args.put(Iterate.OPTION_NAME, String.valueOf(TEST_ITERATIONS));
        LongevitySuite suite =
                new LongevitySuite(TestSuite.class, new AllDefaultPossibilitiesBuilder(true), args);
        assertEquals(suite.testCount(), TEST_ITERATIONS * 3);
    }


    @RunWith(LongevitySuite.class)
    @SuiteClasses({
        TestSuite.OneTest.class,
        TestSuite.TwoTests.class
    })
    /**
     * Sample device-side test cases.
     */
    public static class TestSuite {
        // no local test cases.

        public static class OneTest {
            @Test
            public void testNothing1() { }
        }

        public static class TwoTests {
            @Test
            public void testNothing1() { }

            @Test
            public void testNothing2() { }
        }
    }
}
