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

package android.accessibilityservice.cts;

import static android.accessibilityservice.cts.utils.AccessibilityEventFilterUtils.filterWindowsChangedWithChangeTypes;
import static android.accessibilityservice.cts.utils.ActivityLaunchUtils.findWindowByTitle;
import static android.accessibilityservice.cts.utils.ActivityLaunchUtils.getActivityTitle;
import static android.accessibilityservice.cts.utils.ActivityLaunchUtils.launchActivityAndWaitForItToBeOnscreen;
import static android.accessibilityservice.cts.utils.DisplayUtils.getStatusBarHeight;
import static android.content.pm.PackageManager.FEATURE_PICTURE_IN_PICTURE;
import static android.view.accessibility.AccessibilityEvent.TYPE_WINDOWS_CHANGED;
import static android.view.accessibility.AccessibilityEvent.WINDOWS_CHANGE_ACCESSIBILITY_FOCUSED;
import static android.view.accessibility.AccessibilityEvent.WINDOWS_CHANGE_ACTIVE;
import static android.view.accessibility.AccessibilityEvent.WINDOWS_CHANGE_ADDED;
import static android.view.accessibility.AccessibilityEvent.WINDOWS_CHANGE_BOUNDS;
import static android.view.accessibility.AccessibilityEvent.WINDOWS_CHANGE_CHILDREN;
import static android.view.accessibility.AccessibilityEvent.WINDOWS_CHANGE_FOCUSED;
import static android.view.accessibility.AccessibilityEvent.WINDOWS_CHANGE_REMOVED;
import static android.view.accessibility.AccessibilityEvent.WINDOWS_CHANGE_TITLE;

import static junit.framework.TestCase.assertTrue;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNotNull;

import android.accessibilityservice.AccessibilityServiceInfo;
import android.accessibilityservice.cts.activities.AccessibilityWindowReportingActivity;
import android.app.Activity;
import android.app.Instrumentation;
import android.app.UiAutomation;
import android.support.test.InstrumentationRegistry;
import android.support.test.rule.ActivityTestRule;
import android.support.test.runner.AndroidJUnit4;
import android.view.Gravity;
import android.view.View;
import android.view.WindowManager;
import android.view.accessibility.AccessibilityNodeInfo;
import android.view.accessibility.AccessibilityWindowInfo;
import android.widget.ArrayAdapter;
import android.widget.AutoCompleteTextView;
import android.widget.Button;

import org.junit.AfterClass;
import org.junit.Before;
import org.junit.BeforeClass;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;

import java.util.List;
import java.util.concurrent.TimeoutException;

/**
 * Tests that window changes produce the correct events and that AccessibilityWindowInfos are
 * properly populated
 */
@RunWith(AndroidJUnit4.class)
public class AccessibilityWindowReportingTest {
    private static final int TIMEOUT_ASYNC_PROCESSING = 5000;
    private static final CharSequence TOP_WINDOW_TITLE =
            "android.accessibilityservice.cts.AccessibilityWindowReportingTest.TOP_WINDOW_TITLE";

    private static Instrumentation sInstrumentation;
    private static UiAutomation sUiAutomation;
    private Activity mActivity;
    private CharSequence mActivityTitle;

    @Rule
    public ActivityTestRule<AccessibilityWindowReportingActivity> mActivityRule =
            new ActivityTestRule<>(AccessibilityWindowReportingActivity.class, false, false);

    @BeforeClass
    public static void oneTimeSetup() throws Exception {
        sInstrumentation = InstrumentationRegistry.getInstrumentation();
        sUiAutomation = sInstrumentation.getUiAutomation();
        AccessibilityServiceInfo info = sUiAutomation.getServiceInfo();
        info.flags |= AccessibilityServiceInfo.FLAG_RETRIEVE_INTERACTIVE_WINDOWS;
        sUiAutomation.setServiceInfo(info);
    }

    @AfterClass
    public static void finalTearDown() throws Exception {
        sUiAutomation.destroy();
    }

    @Before
    public void setUp() throws Exception {
        mActivity = launchActivityAndWaitForItToBeOnscreen(
                sInstrumentation, sUiAutomation, mActivityRule);
        mActivityTitle = getActivityTitle(sInstrumentation, mActivity);
    }

    @Test
    public void testUpdatedWindowTitle_generatesEventAndIsReturnedByGetTitle() {
        final String updatedTitle = "Updated Title";
        try {
            sUiAutomation.executeAndWaitForEvent(() -> sInstrumentation.runOnMainSync(
                    () -> mActivity.setTitle(updatedTitle)),
                    filterWindowsChangedWithChangeTypes(WINDOWS_CHANGE_TITLE),
                    TIMEOUT_ASYNC_PROCESSING);
        } catch (TimeoutException exception) {
            throw new RuntimeException(
                    "Failed to get windows changed event for title update", exception);
        }
        final AccessibilityWindowInfo window = findWindowByTitle(sUiAutomation, updatedTitle);
        assertNotNull("Updated window title not reported to accessibility", window);
        window.recycle();
    }

    @Test
    public void testWindowAddedMovedAndRemoved_generatesEventsForAllThree() throws Exception {
        final WindowManager.LayoutParams paramsForTop = layoutParmsForWindowOnTop();
        final WindowManager.LayoutParams paramsForBottom = layoutParmsForWindowOnBottom();
        final Button button = new Button(mActivity);
        button.setText(R.string.button1);
        sUiAutomation.executeAndWaitForEvent(() -> sInstrumentation.runOnMainSync(
                () -> mActivity.getWindowManager().addView(button, paramsForTop)),
                filterWindowsChangedWithChangeTypes(WINDOWS_CHANGE_ADDED),
                TIMEOUT_ASYNC_PROCESSING);

        // Move window from top to bottom
        sUiAutomation.executeAndWaitForEvent(() -> sInstrumentation.runOnMainSync(
                () -> mActivity.getWindowManager().updateViewLayout(button, paramsForBottom)),
                filterWindowsChangedWithChangeTypes(WINDOWS_CHANGE_BOUNDS),
                TIMEOUT_ASYNC_PROCESSING);
        // Remove the view
        sUiAutomation.executeAndWaitForEvent(() -> sInstrumentation.runOnMainSync(
                () -> mActivity.getWindowManager().removeView(button)),
                filterWindowsChangedWithChangeTypes(WINDOWS_CHANGE_REMOVED),
                TIMEOUT_ASYNC_PROCESSING);
    }

    @Test
    public void putWindowInPictureInPicture_generatesEventAndReportsProperty() throws Exception {
        if (!sInstrumentation.getContext().getPackageManager()
                .hasSystemFeature(FEATURE_PICTURE_IN_PICTURE)) {
            return;
        }
        sUiAutomation.executeAndWaitForEvent(
                () -> sInstrumentation.runOnMainSync(() -> mActivity.enterPictureInPictureMode()),
                (event) -> {
                    if (event.getEventType() != TYPE_WINDOWS_CHANGED) return false;
                    // Look for a picture-in-picture window
                    final List<AccessibilityWindowInfo> windows = sUiAutomation.getWindows();
                    final int windowCount = windows.size();
                    for (int i = 0; i < windowCount; i++) {
                        if (windows.get(i).isInPictureInPictureMode()) {
                            return true;
                        }
                    }
                    return false;
                }, TIMEOUT_ASYNC_PROCESSING);

        // There should be exactly one picture-in-picture window now
        int numPictureInPictureWindows = 0;
        final List<AccessibilityWindowInfo> windows = sUiAutomation.getWindows();
        final int windowCount = windows.size();
        for (int i = 0; i < windowCount; i++) {
            final AccessibilityWindowInfo window = windows.get(i);
            if (window.isInPictureInPictureMode()) {
                numPictureInPictureWindows++;
            }
        }
        assertTrue(numPictureInPictureWindows >= 1);
    }

    @Test
    public void moveFocusToAnotherWindow_generatesEventsAndMovesActiveAndFocus() throws Exception {
        final View topWindowView = showTopWindowAndWaitForItToShowUp();
        final AccessibilityWindowInfo topWindow =
                findWindowByTitle(sUiAutomation, TOP_WINDOW_TITLE);

        AccessibilityWindowInfo activityWindow = findWindowByTitle(sUiAutomation, mActivityTitle);
        final AccessibilityNodeInfo buttonNode =
                topWindow.getRoot().findAccessibilityNodeInfosByText(
                        sInstrumentation.getContext().getString(R.string.button1)).get(0);

        // Make sure activityWindow is not focused
        if (activityWindow.isFocused()) {
            sUiAutomation.executeAndWaitForEvent(
                    () -> buttonNode.performAction(AccessibilityNodeInfo.ACTION_FOCUS),
                    filterWindowsChangedWithChangeTypes(WINDOWS_CHANGE_FOCUSED),
                    TIMEOUT_ASYNC_PROCESSING);
        }

        // Windows may have changed - refresh
        activityWindow = findWindowByTitle(sUiAutomation, mActivityTitle);
        assertFalse(activityWindow.isActive());
        assertFalse(activityWindow.isFocused());

        // Find a focusable view in the main activity menu
        final AccessibilityNodeInfo autoCompleteTextInfo = activityWindow.getRoot()
                .findAccessibilityNodeInfosByViewId(
                        "android.accessibilityservice.cts:id/autoCompleteLayout")
                .get(0);

        // Remove the top window and focus on the main activity
        sUiAutomation.executeAndWaitForEvent(
                () -> {
                    sInstrumentation.runOnMainSync(
                            () -> mActivity.getWindowManager().removeView(topWindowView));
                    buttonNode.performAction(AccessibilityNodeInfo.ACTION_FOCUS);
                },
                filterWindowsChangedWithChangeTypes(WINDOWS_CHANGE_FOCUSED | WINDOWS_CHANGE_ACTIVE),
                TIMEOUT_ASYNC_PROCESSING);
    }

    @Test
    public void testChangeAccessibilityFocusWindow_getEvent() throws Exception {
        final AccessibilityServiceInfo info = sUiAutomation.getServiceInfo();
        info.flags |= AccessibilityServiceInfo.FLAG_REQUEST_TOUCH_EXPLORATION_MODE;
        sUiAutomation.setServiceInfo(info);

        try {
            showTopWindowAndWaitForItToShowUp();

            final AccessibilityWindowInfo activityWindow =
                    findWindowByTitle(sUiAutomation, mActivityTitle);
            final AccessibilityWindowInfo topWindow =
                    findWindowByTitle(sUiAutomation, TOP_WINDOW_TITLE);
            final AccessibilityNodeInfo win2Node =
                    topWindow.getRoot().findAccessibilityNodeInfosByText(
                            sInstrumentation.getContext().getString(R.string.button1)).get(0);
            final AccessibilityNodeInfo win1Node = activityWindow.getRoot()
                    .findAccessibilityNodeInfosByViewId(
                            "android.accessibilityservice.cts:id/autoCompleteLayout")
                    .get(0);

            sUiAutomation.executeAndWaitForEvent(
                    () -> {
                        win2Node.performAction(AccessibilityNodeInfo.ACTION_ACCESSIBILITY_FOCUS);
                        win1Node.performAction(AccessibilityNodeInfo.ACTION_ACCESSIBILITY_FOCUS);
                    },
                    filterWindowsChangedWithChangeTypes(WINDOWS_CHANGE_ACCESSIBILITY_FOCUSED),
                    TIMEOUT_ASYNC_PROCESSING);
        } finally {
            info.flags &= ~AccessibilityServiceInfo.FLAG_REQUEST_TOUCH_EXPLORATION_MODE;
            sUiAutomation.setServiceInfo(info);
        }
    }

    @Test
    public void testGetAnchorForDropDownForAutoCompleteTextView_returnsTextViewNode() {
        final AutoCompleteTextView autoCompleteTextView =
                (AutoCompleteTextView) mActivity.findViewById(R.id.autoCompleteLayout);
        final AccessibilityNodeInfo autoCompleteTextInfo = sUiAutomation.getRootInActiveWindow()
                .findAccessibilityNodeInfosByViewId(
                        "android.accessibilityservice.cts:id/autoCompleteLayout")
                .get(0);

        // For the drop-down
        final String[] COUNTRIES = new String[] {"Belgium", "France", "Italy", "Germany", "Spain"};

        try {
            sUiAutomation.executeAndWaitForEvent(() -> sInstrumentation.runOnMainSync(
                    () -> {
                        final ArrayAdapter<String> adapter = new ArrayAdapter<>(
                                mActivity, android.R.layout.simple_dropdown_item_1line, COUNTRIES);
                        autoCompleteTextView.setAdapter(adapter);
                        autoCompleteTextView.showDropDown();
                    }),
                    filterWindowsChangedWithChangeTypes(WINDOWS_CHANGE_CHILDREN),
                    TIMEOUT_ASYNC_PROCESSING);
        } catch (TimeoutException exception) {
            throw new RuntimeException(
                    "Failed to get window changed event when showing dropdown", exception);
        }

        // Find the pop-up window
        boolean foundPopup = false;
        final List<AccessibilityWindowInfo> windows = sUiAutomation.getWindows();
        for (int i = 0; i < windows.size(); i++) {
            final AccessibilityWindowInfo window = windows.get(i);
            if (window.getAnchor() == null) {
                continue;
            }
            assertEquals(autoCompleteTextInfo, window.getAnchor());
            assertFalse("Found multiple pop-ups anchored to one text view", foundPopup);
            foundPopup = true;
        }
        assertTrue("Failed to find accessibility window for auto-complete pop-up", foundPopup);
    }

    private View showTopWindowAndWaitForItToShowUp() throws TimeoutException {
        final WindowManager.LayoutParams paramsForTop = layoutParmsForWindowOnTop();
        final Button button = new Button(mActivity);
        button.setText(R.string.button1);
        sUiAutomation.executeAndWaitForEvent(() -> sInstrumentation.runOnMainSync(
                () -> mActivity.getWindowManager().addView(button, paramsForTop)),
                (event) -> {
                    return (event.getEventType() == TYPE_WINDOWS_CHANGED)
                            && (findWindowByTitle(sUiAutomation, mActivityTitle) != null)
                            && (findWindowByTitle(sUiAutomation, TOP_WINDOW_TITLE) != null);
                },
                TIMEOUT_ASYNC_PROCESSING);
        return button;
    }

    private WindowManager.LayoutParams layoutParmsForWindowOnTop() {
        final WindowManager.LayoutParams params = layoutParmsForTestWindow();
        params.gravity = Gravity.TOP;
        params.setTitle(TOP_WINDOW_TITLE);
        sInstrumentation.runOnMainSync(() -> {
            params.y = getStatusBarHeight(mActivity);
        });
        return params;
    }

    private WindowManager.LayoutParams layoutParmsForWindowOnBottom() {
        final WindowManager.LayoutParams params = layoutParmsForTestWindow();
        params.gravity = Gravity.BOTTOM;
        return params;
    }

    private WindowManager.LayoutParams layoutParmsForTestWindow() {
        final WindowManager.LayoutParams params = new WindowManager.LayoutParams();
        params.width = WindowManager.LayoutParams.MATCH_PARENT;
        params.height = WindowManager.LayoutParams.WRAP_CONTENT;
        params.flags = WindowManager.LayoutParams.FLAG_LAYOUT_IN_SCREEN
                | WindowManager.LayoutParams.FLAG_NOT_TOUCH_MODAL
                | WindowManager.LayoutParams.FLAG_LAYOUT_INSET_DECOR;
        params.type = WindowManager.LayoutParams.TYPE_APPLICATION_PANEL;
        sInstrumentation.runOnMainSync(() -> {
            params.token = mActivity.getWindow().getDecorView().getWindowToken();
        });
        return params;
    }
}