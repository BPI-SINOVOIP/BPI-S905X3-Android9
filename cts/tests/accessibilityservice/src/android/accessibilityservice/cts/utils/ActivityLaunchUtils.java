/**
 * Copyright (C) 2017 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except
 * in compliance with the License. You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software distributed under the
 * License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
 * express or implied. See the License for the specific language governing permissions and
 * limitations under the License.
 */

package android.accessibilityservice.cts.utils;

import static android.accessibilityservice.cts.AccessibilityActivityTestCase
        .TIMEOUT_ASYNC_PROCESSING;

import static org.junit.Assert.assertNotNull;

import android.accessibilityservice.AccessibilityServiceInfo;
import android.app.Activity;
import android.app.Instrumentation;
import android.app.UiAutomation;
import android.graphics.Rect;
import android.support.test.rule.ActivityTestRule;
import android.text.TextUtils;
import android.view.accessibility.AccessibilityEvent;
import android.view.accessibility.AccessibilityWindowInfo;

import java.util.List;

/**
 * Utilities useful when launching an activity to make sure it's all the way on the screen
 * before we start testing it.
 */
public class ActivityLaunchUtils {
    // Using a static variable so it can be used in lambdas. Not preserving state in it.
    private static Activity mTempActivity;

    public static Activity launchActivityAndWaitForItToBeOnscreen(Instrumentation instrumentation,
            UiAutomation uiAutomation, ActivityTestRule<? extends Activity> rule) throws Exception {
        final int[] location = new int[2];
        final StringBuilder activityPackage = new StringBuilder();
        final Rect bounds = new Rect();
        final StringBuilder activityTitle = new StringBuilder();
        // Make sure we get window events, so we'll know when the window appears
        AccessibilityServiceInfo info = uiAutomation.getServiceInfo();
        info.flags |= AccessibilityServiceInfo.FLAG_RETRIEVE_INTERACTIVE_WINDOWS;
        uiAutomation.setServiceInfo(info);
        final AccessibilityEvent awaitedEvent = uiAutomation.executeAndWaitForEvent(
                () -> {
                    mTempActivity = rule.launchActivity(null);
                    final StringBuilder builder = new StringBuilder();
                    instrumentation.runOnMainSync(() -> {
                        mTempActivity.getWindow().getDecorView().getLocationOnScreen(location);
                        activityPackage.append(mTempActivity.getPackageName());
                    });
                    instrumentation.waitForIdleSync();
                    activityTitle.append(getActivityTitle(instrumentation, mTempActivity));
                },
                (event) -> {
                    final AccessibilityWindowInfo window =
                            findWindowByTitle(uiAutomation, activityTitle);
                    if (window == null) return false;
                    window.getBoundsInScreen(bounds);
                    mTempActivity.getWindow().getDecorView().getLocationOnScreen(location);
                    if (bounds.isEmpty()) {
                        return false;
                    }
                    return (!bounds.isEmpty())
                            && (bounds.left == location[0]) && (bounds.top == location[1]);
                }, TIMEOUT_ASYNC_PROCESSING);
        assertNotNull(awaitedEvent);
        return mTempActivity;
    }

    public static CharSequence getActivityTitle(
            Instrumentation instrumentation, Activity activity) {
        final StringBuilder titleBuilder = new StringBuilder();
        instrumentation.runOnMainSync(() -> titleBuilder.append(activity.getTitle()));
        return titleBuilder;
    }

    public static AccessibilityWindowInfo findWindowByTitle(
            UiAutomation uiAutomation, CharSequence title) {
        final List<AccessibilityWindowInfo> windows = uiAutomation.getWindows();
        AccessibilityWindowInfo returnValue = null;
        for (int i = 0; i < windows.size(); i++) {
            final AccessibilityWindowInfo window = windows.get(i);
            if (TextUtils.equals(title, window.getTitle())) {
                returnValue = window;
            } else {
                window.recycle();
            }
        }
        return returnValue;
    }
}
