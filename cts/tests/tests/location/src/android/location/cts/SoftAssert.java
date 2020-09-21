/*
 * Copyright (C) 2015 Google Inc.
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

package android.location.cts;

import junit.framework.Assert;

import android.util.Log;

import java.util.ArrayList;
import java.util.List;

/**
 * Custom Assertion class. This is useful for doing multiple validations together
 * without failing the test. Tests don’t stop running even if an assertion condition fails,
 * but the test itself is marked as a failed test to indicate the right result
 * at the end of the test.
 */
public class SoftAssert {

    List<String> mErrorList;
    private String mTag;

    SoftAssert(String source) {
        mErrorList = new ArrayList<>();
        mTag = source;
    }

    /**
     * Check if condition is true
     *
     * @param message        test message
     * @param eventTimeInNs  the time at which the condition occurred
     * @param expectedResult expected value
     * @param actualResult   actual value
     * @param condition      condition for test
     */
    public void assertTrue(String message, Long eventTimeInNs, String expectedResult,
                           String actualResult, boolean condition) {
        if (condition) {
            Log.i(mTag, message + ", (Test: PASS, actual : " +
                    actualResult + ", expected: " + expectedResult + ")");
        } else {
            String errorMessage = "";
            if (eventTimeInNs != null) {
                errorMessage = "At time = " + eventTimeInNs + " ns, ";
            }
            errorMessage += message +
                    " (Test: FAIL, actual :" + actualResult + ", " +
                    "expected: " + expectedResult + ")";
            Log.e(mTag, errorMessage);
            mErrorList.add(errorMessage);
        }
    }

    /**
     * assertTrue without eventTime
     *
     * @param message        test message
     * @param expectedResult expected value
     * @param actualResult   actual value
     * @param condition      condition for test
     */
    public void assertTrue(String message, String expectedResult,
        String actualResult, boolean condition) {
        assertTrue(message, null, expectedResult, actualResult, condition);
    }

    /**
     * Check if a condition is true.
     * NOTE: A failure is downgraded to a warning.
     *
     * @param message        the message associated with the condition
     * @param eventTimeInNs  the time at which the condition occurred
     * @param expectedResult the expected result of the condition
     * @param actualResult   the actual result of the condition
     * @param condition      the condition status
     */
    public void assertTrueAsWarning(
            String message,
            long eventTimeInNs,
            String expectedResult,
            String actualResult,
            boolean condition) {
        if (condition) {
            String formattedMessage = String.format(
                    "%s, (Test: PASS, actual : %s, expected : %s)",
                    message,
                    actualResult,
                    expectedResult);
            Log.i(mTag, formattedMessage);
        } else {
            String formattedMessage = String.format(
                    "At time = %d ns, %s (Test: WARNING, actual : %s, expected : %s).",
                    eventTimeInNs,
                    message,
                    actualResult,
                    expectedResult);
            failAsWarning(mTag, formattedMessage);
        }
    }

    /**
     * Check if condition is true
     *
     * @param message   test message
     * @param condition condition for test
     */
    public void assertTrue(String message, boolean condition) {
        assertOrWarnTrue(true, message, condition);
    }

    /**
     * Check if condition is true
     *
     * @param strict      if true, add this to the failure list, else, log a warning message
     * @param message     message to describe the test - output on pass or fail
     * @param condition   condition for test
     */
    public void assertOrWarnTrue(boolean strict, String message, boolean condition) {
        if (condition) {
            Log.i(mTag, "(Test: PASS) " + message);
        } else {
            String errorMessage = "(Test: FAIL) " + message;
            Log.i(mTag, errorMessage);
            if (strict) {
                mErrorList.add(errorMessage);
            } else {
                failAsWarning(mTag, errorMessage);
            }
        }
    }

    /**
     * Assert all conditions together. This method collates all the failures and decides
     * whether to fail the test or not at the end. This must be called at the end of the test.
     */
    public void assertAll() {
        if (mErrorList.isEmpty()) {
            Log.i(mTag, "All test pass.");
            // Test pass if there are no error message in errorMessageSet
            Assert.assertTrue(true);
        } else {
            StringBuilder message = new StringBuilder();
            for (String msg : mErrorList) {
                message.append(msg + "\n");
            }
            Log.e(mTag, "Failing tests are: \n" + message);
            Assert.fail("Failing tests are: \n" + message);
        }
    }

    /**
     * A hard or soft failure, depending on the setting.
     * TODO - make this cleaner - e.g. refactor this out completely: get rid of static methods,
     * and make a class (say TestVerification) the only entry point for verifications,
     * so strict vs not can be abstracted away test implementations.
     */
    public static void failOrWarning(boolean testIsStrict, String message, boolean condition) {
        if (testIsStrict) {
            Assert.assertTrue(message, condition);
        } else {
            if (!condition) {
                failAsWarning("", message);
            }
        }
    }

    /**
     * A soft failure. In the current state of the tests, it will only log a warning and let the
     * test be reported as 'pass'.
     */
    public static void failAsWarning(String tag, String message) {
        Log.w(tag, message + " [NOTE: in a future release this feature might become mandatory, and"
                + " this warning will fail the test].");
    }
}
