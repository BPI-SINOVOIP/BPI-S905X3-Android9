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

package android.dumpsys.cts;

import com.android.compatibility.common.tradefed.build.CompatibilityBuildHelper;
import com.android.ddmlib.testrunner.RemoteAndroidTestRunner;
import com.android.ddmlib.testrunner.TestResult.TestStatus;
import com.android.tradefed.build.IBuildInfo;
import com.android.tradefed.device.CollectingOutputReceiver;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.log.LogUtil.CLog;
import com.android.tradefed.result.CollectingTestListener;
import com.android.tradefed.result.TestDescription;
import com.android.tradefed.result.TestResult;
import com.android.tradefed.result.TestRunResult;
import com.android.tradefed.testtype.DeviceTestCase;
import com.android.tradefed.testtype.IBuildReceiver;

import java.io.FileNotFoundException;
import java.util.Map;
import java.util.Set;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

import javax.annotation.Nonnull;
import javax.annotation.Nullable;

public class BaseDumpsysTest extends DeviceTestCase implements IBuildReceiver {
    protected static final String TAG = "DumpsysHostTest";

    private static final String TEST_RUNNER = "android.support.test.runner.AndroidJUnitRunner";

    /**
     * A reference to the device under test.
     */
    protected ITestDevice mDevice;

    protected IBuildInfo mCtsBuild;

    @Override
    protected void setUp() throws Exception {
        super.setUp();
        mDevice = getDevice();
    }

    @Override
    public void setBuild(IBuildInfo buildInfo) {
        mCtsBuild = buildInfo;
    }

    protected static long assertInteger(String input) {
        try {
            return Long.parseLong(input);
        } catch (NumberFormatException e) {
            fail("Expected an integer but found \"" + input + "\"");
            // Won't be hit, above throws AssertException
            return -1;
        }
    }

    protected static long assertNonNegativeInteger(String input) {
        try {
            final long result = Long.parseLong(input);
            assertTrue("Expected non-negative, but was: " + result, result >= 0);

            return result;
        } catch (NumberFormatException e) {
            fail("Expected an integer but found \"" + input + "\"");
            // Won't be hit, above throws AssertException
            return -1;
        }
    }

    protected static long assertPositiveInteger(String input) {
        try {
            final long result = Long.parseLong(input);
            assertTrue("Expected positive, but was: " + result, result > 0);

            return result;
        } catch (NumberFormatException e) {
            fail("Expected an integer but found \"" + input + "\"");
            // Won't be hit, above throws AssertException
            return -1;
        }
    }

    protected static void assertMinAvgMax(String min, String avg, String max, boolean checkAvg) {
        final long lMin = assertNonNegativeInteger(min);
        final long lAvg = assertNonNegativeInteger(avg);
        final long lMax = assertNonNegativeInteger(max);

        if (checkAvg) {
            assertTrue("min [" + min + "] <= avg [" + avg + "]", lMin <= lAvg);
            assertTrue("avg [" + avg + "] <= max [" + max + "]", lAvg <= lMax);
        } else {
            // There was a bug in the average calculation, so we can't check the average
            // from the last N hour stats, which may be generated on with the buggy logic.
            assertTrue("min [" + min + "] <= max [" + max + "]", lMin <= lMax);
        }
    }

    protected static void assertLesserOrEqual(String lesser, String greater) {
        final long lLesser = assertNonNegativeInteger(lesser);
        final long lGreater = assertNonNegativeInteger(greater);

        assertTrue("[" + lesser + "] <= [" + greater + "]", lLesser <= lGreater);
    }

    protected static double assertDouble(String input) {
        try {
            return Double.parseDouble(input);
        } catch (NumberFormatException e) {
            fail("Expected a double but found \"" + input + "\"");
            return -1;
        }
    }

    protected static void assertSeenTag(Set<String> seenTags, String tag) {
        assertTrue("No line starting with \"" + tag + ",\"", seenTags.contains(tag));
    }


    /**
     * Install a device side test package.
     *
     * @param appFileName Apk file name, such as "CtsNetStatsApp.apk".
     * @param grantPermissions whether to give runtime permissions.
     */
    protected void installPackage(String appFileName, boolean grantPermissions)
            throws FileNotFoundException, DeviceNotAvailableException {
        CLog.d("Installing app " + appFileName);
        CompatibilityBuildHelper buildHelper = new CompatibilityBuildHelper(mCtsBuild);
        final String result = getDevice().installPackage(
                buildHelper.getTestFile(appFileName), true, grantPermissions);
        assertNull("Failed to install " + appFileName + ": " + result, result);
    }

    /**
     * Run a device side test.
     *
     * @param pkgName Test package name, such as "com.android.server.cts.netstats".
     * @param testClassName Test class name; either a fully qualified name, or "." + a class name.
     * @param testMethodName Test method name.
     * @throws DeviceNotAvailableException
     */
    protected void runDeviceTests(@Nonnull String pkgName,
            @Nullable String testClassName, @Nullable String testMethodName)
            throws DeviceNotAvailableException {
        if (testClassName != null && testClassName.startsWith(".")) {
            testClassName = pkgName + testClassName;
        }

        RemoteAndroidTestRunner testRunner = new RemoteAndroidTestRunner(
                pkgName, TEST_RUNNER, getDevice().getIDevice());
        if (testClassName != null && testMethodName != null) {
            testRunner.setMethodName(testClassName, testMethodName);
        } else if (testClassName != null) {
            testRunner.setClassName(testClassName);
        }

        CollectingTestListener listener = new CollectingTestListener();
        assertTrue(getDevice().runInstrumentationTests(testRunner, listener));

        final TestRunResult result = listener.getCurrentRunResults();
        if (result.isRunFailure()) {
            throw new AssertionError("Failed to successfully run device tests for "
                    + result.getName() + ": " + result.getRunFailureMessage());
        }
        if (result.getNumTests() == 0) {
            throw new AssertionError("No tests were run on the device");
        }

        if (result.hasFailedTests()) {
            // build a meaningful error message
            StringBuilder errorBuilder = new StringBuilder("On-device tests failed:\n");
            for (Map.Entry<TestDescription, TestResult> resultEntry :
                    result.getTestResults().entrySet()) {
                if (!resultEntry.getValue().getStatus().equals(TestStatus.PASSED)) {
                    errorBuilder.append(resultEntry.getKey().toString());
                    errorBuilder.append(":\n");
                    errorBuilder.append(resultEntry.getValue().getStackTrace());
                }
            }
            throw new AssertionError(errorBuilder.toString());
        }
    }

    /**
     * Execute the given command, and find the given pattern and return the resulting
     * {@link Matcher}.
     */
    protected Matcher execCommandAndFind(String command, String pattern) throws Exception {
        final CollectingOutputReceiver receiver = new CollectingOutputReceiver();
        getDevice().executeShellCommand(command, receiver);
        final String output = receiver.getOutput();
        final Matcher matcher = Pattern.compile(pattern).matcher(output);
        assertTrue("Pattern '" + pattern + "' didn't match. Output=\n" + output, matcher.find());
        return matcher;
    }

    /**
     * Execute the given command, find the given pattern, and return the first captured group
     * as a String.
     */
    protected String execCommandAndGetFirstGroup(String command, String pattern) throws Exception {
        final Matcher matcher = execCommandAndFind(command, pattern);
        assertTrue("No group found for pattern '" + pattern + "'", matcher.groupCount() > 0);
        return matcher.group(1);
    }
}
