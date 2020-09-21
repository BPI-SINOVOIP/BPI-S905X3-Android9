/*
 * Copyright (C) 2012 The Android Open Source Project
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

package android.accessibilityservice.cts;

import android.accessibilityservice.AccessibilityServiceInfo;
import android.app.Activity;
import android.app.UiAutomation;
import android.test.ActivityInstrumentationTestCase2;
import android.view.accessibility.AccessibilityEvent;
import android.view.accessibility.AccessibilityNodeInfo;
import android.view.accessibility.AccessibilityWindowInfo;

import java.util.List;
import java.util.concurrent.TimeoutException;

/**
 * Base text case for testing accessibility APIs by instrumenting an Activity.
 */
public abstract class AccessibilityActivityTestCase<T extends Activity>
        extends ActivityInstrumentationTestCase2<T> {
    /**
     * Timeout required for pending Binder calls or event processing to
     * complete.
     */
    public static final long TIMEOUT_ASYNC_PROCESSING = 5000;

    /**
     * The timeout since the last accessibility event to consider the device idle.
     */
    public static final long TIMEOUT_ACCESSIBILITY_STATE_IDLE = 500;

    /**
     * @param activityClass
     */
    public AccessibilityActivityTestCase(Class<T> activityClass) {
        super(activityClass);
    }

    @Override
    public void setUp() throws Exception {
        super.setUp();

        AccessibilityServiceInfo info = getInstrumentation().getUiAutomation().getServiceInfo();
        info.flags |= AccessibilityServiceInfo.FLAG_REQUEST_TOUCH_EXPLORATION_MODE;
        info.flags |= AccessibilityServiceInfo.FLAG_RETRIEVE_INTERACTIVE_WINDOWS;
        info.flags &= ~AccessibilityServiceInfo.FLAG_INCLUDE_NOT_IMPORTANT_VIEWS;
        getInstrumentation().getUiAutomation().setServiceInfo(info);

        startActivityAndWaitForFirstEvent();

        waitForIdle();
    }

    /**
     * Waits for the UI do be idle.
     *
     * @throws TimeoutException if idle cannot be detected.
     */
    public void waitForIdle() throws TimeoutException {
        getInstrumentation().getUiAutomation().waitForIdle(
                TIMEOUT_ACCESSIBILITY_STATE_IDLE,
                TIMEOUT_ASYNC_PROCESSING);
    }

    /**
     * @return The string for a given <code>resId</code>.
     */
    public String getString(int resId) {
        return getInstrumentation().getContext().getString(resId);
    }

    /**
     * Starts the activity under tests and waits for the first accessibility
     * event from that activity.
     */
    private void startActivityAndWaitForFirstEvent() throws Exception {
        AccessibilityEvent awaitedEvent =
            getInstrumentation().getUiAutomation().executeAndWaitForEvent(
                new Runnable() {
            @Override
            public void run() {
                getActivity();
                getInstrumentation().waitForIdleSync();
            }
        },
                new UiAutomation.AccessibilityEventFilter() {
            @Override
            public boolean accept(AccessibilityEvent event) {
                List<AccessibilityWindowInfo> windows = getInstrumentation().getUiAutomation()
                        .getWindows();
                // Wait for a window state changed event with our window showing
                for (int i = 0; i < windows.size(); i++) {
                    AccessibilityNodeInfo root = windows.get(i).getRoot();
                    if ((root != null) &&
                            root.getPackageName().equals(getActivity().getPackageName())) {
                        return (event.getEventType()
                                == AccessibilityEvent.TYPE_WINDOW_STATE_CHANGED);
                    }
                }
                return false;
            }
        },
        TIMEOUT_ASYNC_PROCESSING);
        assertNotNull(awaitedEvent);
    }
}
