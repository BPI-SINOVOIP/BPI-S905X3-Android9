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

package android.autofillservice.cts;

import static android.autofillservice.cts.common.ShellHelper.runShellCommand;

import androidx.annotation.NonNull;
import android.util.Log;

import org.junit.AssumptionViolatedException;
import org.junit.rules.TestRule;
import org.junit.runner.Description;
import org.junit.runners.model.Statement;

/**
 * Custom JUnit4 rule that improves autofill-related logging by:
 *
 * <ol>
 *   <li>Setting logging level to verbose before test start.
 *   <li>Call {@code dumpsys autofill} in case of failure.
 * </ol>
 */
public class AutofillLoggingTestRule implements TestRule, SafeCleanerRule.Dumper {

    private static final String TAG = "AutofillLoggingTestRule";

    private final String mTag;
    private boolean mDumped;

    public AutofillLoggingTestRule(String tag) {
        mTag = tag;
    }

    @Override
    public Statement apply(Statement base, Description description) {
        return new Statement() {

            @Override
            public void evaluate() throws Throwable {
                final String testName = description.getDisplayName();
                Log.v(TAG, "@Before " + testName);
                final String levelBefore = runShellCommand("cmd autofill get log_level");
                if (!levelBefore.equals("verbose")) {
                    runShellCommand("cmd autofill set log_level verbose");
                }
                try {
                    base.evaluate();
                } catch (Throwable t) {
                    dump(testName, t);
                    throw t;
                } finally {
                    try {
                        if (!levelBefore.equals("verbose")) {
                            runShellCommand("cmd autofill set log_level %s", levelBefore);
                        }
                    } finally {
                        Log.v(TAG, "@After " + testName);
                    }
                }
            }
        };
    }

    @Override
    public void dump(@NonNull String testName, @NonNull Throwable t) {
        if (mDumped) {
            Log.e(mTag, "dump(" + testName + "): already dumped");
            return;
        }
        if ((t instanceof AssumptionViolatedException)) {
            // This exception is used to indicate a test should be skipped and is
            // ignored by JUnit runners - we don't need to dump it...
            Log.w(TAG, "ignoring exception: " + t);
            return;
        }
        Log.e(mTag, "Dumping after exception on " + testName, t);
        final String autofillDump = runShellCommand("dumpsys autofill");
        Log.e(mTag, "autofill dump: \n" + autofillDump);
        final String activityDump = runShellCommand("dumpsys activity top");
        Log.e(mTag, "top activity dump: \n" + activityDump);
        mDumped = true;
    }
}
