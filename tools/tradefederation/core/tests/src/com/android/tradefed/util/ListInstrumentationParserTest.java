/*
 * Copyright (C) 2015 The Android Open Source Project
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

import com.android.tradefed.util.ListInstrumentationParser.InstrumentationTarget;
import com.google.common.collect.Lists;

import junit.framework.TestCase;

import java.util.List;
import java.util.Objects;

/** Simple unit test for {@link ListInstrumentationParser}. */
public class ListInstrumentationParserTest extends TestCase {

    // Example instrumentation test values
    private static final String EXAMPLE_TEST_PACKAGE_1 = "com.example.test";
    private static final String EXAMPLE_TARGET_1 = "com.example";
    private static final String EXAMPLE_RUNNER_1 = "android.test.InstrumentationTestRunner";
    private static final String LIST_INSTRUMENTATION_OUTPUT_1 =
            "instrumentation:com.example.test/android.test.InstrumentationTestRunner (target=com.example)";

    private static final String EXAMPLE_TEST_PACKAGE_2 = "com.foobar.test";
    private static final String EXAMPLE_TARGET_2 = "com.example2";
    private static final String EXAMPLE_RUNNER_2 = "android.support.test.runner.AndroidJUnitRunner";
    private static final String LIST_INSTRUMENTATION_OUTPUT_2 =
            "instrumentation:com.foobar.test/android.support.test.runner.AndroidJUnitRunner (target=com.example2)";

    private static final String EXAMPLE_TEST_PACKAGE_3 = "com.example.test";
    private static final String EXAMPLE_TARGET_3 = "com.example";
    private static final String EXAMPLE_RUNNER_3 = "android.support.test.runner.AndroidJUnitRunner";
    private static final String LIST_INSTRUMENTATION_OUTPUT_3 =
            "instrumentation:com.example.test/android.support.test.runner.AndroidJUnitRunner (target=com.example)";


    private ListInstrumentationParser mParser;

    @Override
    public void setUp() {
        mParser = new ListInstrumentationParser();
    }

    /**
     * Test behavior when there are no lines to parse
     */
    public void testEmptyParse() {
        List<InstrumentationTarget> targets = mParser.getInstrumentationTargets();
        assertTrue("getInstrumentationTargets() not empty", targets.isEmpty());
    }

    /**
     * Simple test case for list instrumentation parsing
     */
    public void testSimpleParse() {
        // Build example `pm list instrumentation` output and send it to the parser
        String[] pmListOutput = {
            LIST_INSTRUMENTATION_OUTPUT_1,
            LIST_INSTRUMENTATION_OUTPUT_2,
            LIST_INSTRUMENTATION_OUTPUT_3
        };
        mParser.processNewLines(pmListOutput);

        // Expected targets
        InstrumentationTarget target1 = new InstrumentationTarget(
                EXAMPLE_TEST_PACKAGE_1, EXAMPLE_RUNNER_1, EXAMPLE_TARGET_1);
        InstrumentationTarget target2 = new InstrumentationTarget(
                EXAMPLE_TEST_PACKAGE_2, EXAMPLE_RUNNER_2, EXAMPLE_TARGET_2);
        InstrumentationTarget target3 = new InstrumentationTarget(
                EXAMPLE_TEST_PACKAGE_3, EXAMPLE_RUNNER_3, EXAMPLE_TARGET_3);

        // Targets should be alphabetized by test package name, runner name, target name.
        List<InstrumentationTarget> expectedTargets = Lists.newArrayList(target1, target3, target2);

        // Get the parsed targets and make sure they contain the expected targets
        List<InstrumentationTarget> parsedTargets = mParser.getInstrumentationTargets();
        validateInstrumentationTargets(expectedTargets, parsedTargets);

        assertFalse("Nonshardable targets treated as shardable", target1.isShardable());
        assertTrue("Shardable targets not treated as shardable", target2.isShardable());
    }

    /**
     * Helper function to compare two Lists and assert if they do not contain identical
     * InstrumentationTargets
     */
    private void validateInstrumentationTargets(List<InstrumentationTarget> expectedTargets,
            List<InstrumentationTarget> actualTargets) {

        // Lists must be the same size
        assertEquals("Unexpected number of parsed targets",
                expectedTargets.size(), actualTargets.size());
        // And contain the same elements
        for (InstrumentationTarget actualTarget : actualTargets) {
            // Must equal one of the expected targets
            boolean matched = false;
            for (InstrumentationTarget expectedTarget : expectedTargets) {
                matched = areTargetsEqual(expectedTarget, actualTarget);
                if (matched) {
                    expectedTargets.remove(expectedTarget);
                    break;
                }
            }
            if (!matched) {
                fail(String.format("Unexpected InstrumentationTarget(%s, %s, %s)",
                      actualTarget.packageName, actualTarget.runnerName, actualTarget.targetName));
            }
        }
    }

    /**
     * Helper function to determine if two {@link InstrumentationTarget}s are equal
     */
    private static boolean areTargetsEqual(InstrumentationTarget expected,
            InstrumentationTarget actual) {

        return Objects.equals(expected.packageName, actual.packageName) &&
                Objects.equals(expected.runnerName, actual.runnerName) &&
                Objects.equals(expected.targetName, actual.targetName);
    }
}
