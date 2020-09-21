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
 * limitations under the License
 */

package android.host.multiuser;

import android.platform.test.annotations.Presubmit;

import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.log.LogUtil.CLog;
import com.android.tradefed.testtype.DeviceJUnit4ClassRunner;

import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.rules.TestRule;
import org.junit.runner.Description;
import org.junit.runner.RunWith;
import org.junit.runners.model.Statement;

import java.util.HashSet;
import java.util.LinkedHashSet;
import java.util.Scanner;
import java.util.Set;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

import static org.junit.Assert.assertTrue;

/**
 * Test verifies that users can be created/switched to without error dialogs shown to the user
 * Run: atest CreateUsersNoAppCrashesTest
 */
@RunWith(DeviceJUnit4ClassRunner.class)
public class CreateUsersNoAppCrashesTest extends BaseMultiUserTest {
    private int mInitialUserId;
    private static final long LOGCAT_POLL_INTERVAL_MS = 5000;
    private static final long USER_SWITCH_COMPLETE_TIMEOUT_MS = 180000;

    @Rule public AppCrashRetryRule appCrashRetryRule = new AppCrashRetryRule();

    @Before
    public void setUp() throws Exception {
        CLog.e("setup_CreateUsersNoAppCrashesTest");
        super.setUp();
        mInitialUserId = getDevice().getCurrentUser();
    }

    @Presubmit
    @Test
    public void testCanCreateGuestUser() throws Exception {
        if (!mSupportsMultiUser) {
            return;
        }
        int userId = getDevice().createUser(
                "TestUser_" + System.currentTimeMillis() /* name */,
                true /* guest */,
                false /* ephemeral */);
        assertSwitchToNewUser(userId);
        assertSwitchToUser(userId, mInitialUserId);

    }

    @Presubmit
    @Test
    public void testCanCreateSecondaryUser() throws Exception {
        if (!mSupportsMultiUser) {
            return;
        }
        int userId = getDevice().createUser(
                "TestUser_" + System.currentTimeMillis() /* name */,
                false /* guest */,
                false /* ephemeral */);
        assertSwitchToNewUser(userId);
        assertSwitchToUser(userId, mInitialUserId);
    }

    private void assertSwitchToNewUser(int toUserId) throws Exception {
        final String exitString = "Finished processing BOOT_COMPLETED for u" + toUserId;
        final Set<String> appErrors = new LinkedHashSet<>();
        getDevice().executeAdbCommand("logcat", "-c"); // Reset log
        assertTrue("Couldn't switch to user " + toUserId, getDevice().switchUser(toUserId));
        final boolean result = waitForUserSwitchComplete(appErrors, toUserId, exitString);
        assertTrue("Didn't receive BOOT_COMPLETED delivered notification. appErrors="
                + appErrors, result);
        if (!appErrors.isEmpty()) {
            throw new AppCrashOnBootError(appErrors);
        }
    }

    private void assertSwitchToUser(int fromUserId, int toUserId) throws Exception {
        final String exitString = "Continue user switch oldUser #" + fromUserId + ", newUser #"
                + toUserId;
        final Set<String> appErrors = new LinkedHashSet<>();
        getDevice().executeAdbCommand("logcat", "-c"); // Reset log
        assertTrue("Couldn't switch to user " + toUserId, getDevice().switchUser(toUserId));
        final boolean result = waitForUserSwitchComplete(appErrors, toUserId, exitString);
        assertTrue("Didn't reach \"Continue user switch\" stage. appErrors=" + appErrors, result);
        if (!appErrors.isEmpty()) {
            throw new AppCrashOnBootError(appErrors);
        }
    }

    private boolean waitForUserSwitchComplete(Set<String> appErrors, int targetUserId,
            String exitString) throws DeviceNotAvailableException, InterruptedException {
        boolean mExitFound = false;
        long ti = System.currentTimeMillis();
        while (System.currentTimeMillis() - ti < USER_SWITCH_COMPLETE_TIMEOUT_MS) {
            String logs = getDevice().executeAdbCommand("logcat", "-v", "brief", "-d",
                    "ActivityManager:D", "AndroidRuntime:E", "*:S");
            Scanner in = new Scanner(logs);
            while (in.hasNextLine()) {
                String line = in.nextLine();
                if (line.contains("Showing crash dialog for package")) {
                    appErrors.add(line);
                } else if (line.contains(exitString)) {
                    // Parse all logs in case crashes occur as a result of onUserChange callbacks
                    mExitFound = true;
                } else if (line.contains("FATAL EXCEPTION IN SYSTEM PROCESS")) {
                    throw new IllegalStateException("System process crashed - " + line);
                }
            }
            in.close();
            if (mExitFound) {
                if (!appErrors.isEmpty()) {
                    CLog.w("App crash dialogs found: " + appErrors);
                }
                return true;
            }
            Thread.sleep(LOGCAT_POLL_INTERVAL_MS);
        }
        return false;
    }

    static class AppCrashOnBootError extends AssertionError {
        private static final Pattern PACKAGE_NAME_PATTERN = Pattern.compile("package ([^\\s]+)");
        private Set<String> errorPackages;

        AppCrashOnBootError(Set<String> errorLogs) {
            super("App error dialog(s) are present: " + errorLogs);
            this.errorPackages = errorLogsToPackageNames(errorLogs);
        }

        private static Set<String> errorLogsToPackageNames(Set<String> errorLogs) {
            Set<String> result = new HashSet<>();
            for (String line : errorLogs) {
                Matcher matcher = PACKAGE_NAME_PATTERN.matcher(line);
                if (matcher.find()) {
                    result.add(matcher.group(1));
                } else {
                    throw new IllegalStateException("Unrecognized line " + line);
                }
            }
            return result;
        }
    }

    /**
     * Rule that retries the test if it failed due to {@link AppCrashOnBootError}
     */
    public static class AppCrashRetryRule implements TestRule {

        @Override
        public Statement apply(Statement base, Description description) {
            return new Statement() {
                @Override
                public void evaluate() throws Throwable {
                    Set<String> errors = evaluateAndReturnAppCrashes(base);
                    if (errors.isEmpty()) {
                        return;
                    }
                    CLog.e("Retrying due to app crashes: " + errors);
                    // Fail only if same apps are crashing in both runs
                    errors.retainAll(evaluateAndReturnAppCrashes(base));
                    assertTrue("App error dialog(s) are present after 2 attempts: " + errors,
                            errors.isEmpty());
                }
            };
        }

        private static Set<String> evaluateAndReturnAppCrashes(Statement base) throws Throwable {
            try {
                base.evaluate();
            } catch (AppCrashOnBootError e) {
                return e.errorPackages;
            }
            return new HashSet<>();
        }
    }
}
