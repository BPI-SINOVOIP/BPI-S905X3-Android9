/*
 * Copyright (C) 2016 The Android Open Source Project
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

package android.server.wm;

import static android.app.WindowConfiguration.WINDOWING_MODE_SPLIT_SCREEN_PRIMARY;
import static android.server.am.StateLogger.log;
import static android.server.wm.DialogFrameTestActivity.EXTRA_TEST_CASE;

import android.app.Activity;
import android.content.ComponentName;
import android.content.Intent;
import android.server.am.ActivityManagerTestBase;
import android.server.am.WindowManagerState.WindowState;
import android.support.test.rule.ActivityTestRule;

abstract class ParentChildTestBase<T extends Activity> extends ActivityManagerTestBase {

    interface ParentChildTest {
        void doTest(WindowState parent, WindowState child);
    }

    private void startTestCase(String testCase) throws Exception {
        final Intent intent = new Intent()
                .putExtra(EXTRA_TEST_CASE, testCase);
        activityRule().launchActivity(intent);
    }

    private void startTestCaseDocked(String testCase) throws Exception {
        startTestCase(testCase);
        setActivityTaskWindowingMode(activityName(), WINDOWING_MODE_SPLIT_SCREEN_PRIMARY);
    }

    abstract ComponentName activityName();
    abstract ActivityTestRule<T> activityRule();

    abstract void doSingleTest(ParentChildTest t) throws Exception;

    void doFullscreenTest(String testCase, ParentChildTest t) throws Exception {
        log("Running test fullscreen");
        startTestCase(testCase);
        doSingleTest(t);
        activityRule().finishActivity();
    }

    private void doDockedTest(String testCase, ParentChildTest t) throws Exception {
        log("Running test docked");
        if (!supportsSplitScreenMultiWindow()) {
            log("Skipping test: no split multi-window support");
            return;
        }
        startTestCaseDocked(testCase);
        doSingleTest(t);
        activityRule().finishActivity();
    }

    void doParentChildTest(String testCase, ParentChildTest t) throws Exception {
        doFullscreenTest(testCase, t);
        doDockedTest(testCase, t);
    }
}
