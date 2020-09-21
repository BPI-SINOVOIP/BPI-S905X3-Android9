/**
 * Copyright (C) 2012 The Android Open Source Project
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

package android.accessibilityservice.cts;

import static android.accessibilityservice.cts.utils.ActivityLaunchUtils
        .launchActivityAndWaitForItToBeOnscreen;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertSame;
import static org.junit.Assert.assertTrue;

import android.accessibilityservice.AccessibilityServiceInfo;
import android.accessibilityservice.cts.R;
import android.accessibilityservice.cts.activities.AccessibilityViewTreeReportingActivity;
import android.app.Instrumentation;
import android.app.UiAutomation;
import android.content.Context;
import android.support.test.InstrumentationRegistry;
import android.support.test.rule.ActivityTestRule;
import android.support.test.runner.AndroidJUnit4;
import android.text.TextUtils;
import android.view.View;
import android.view.ViewGroup;
import android.view.accessibility.AccessibilityEvent;
import android.view.accessibility.AccessibilityNodeInfo;
import android.widget.Button;
import android.widget.LinearLayout;

import org.junit.AfterClass;
import org.junit.Before;
import org.junit.BeforeClass;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;

/**
 * Test cases for testing the accessibility focus APIs exposed to accessibility
 * services. This test checks how the view hierarchy is reported to accessibility
 * services.
 */
@RunWith(AndroidJUnit4.class)
public class AccessibilityViewTreeReportingTest {
    private static final int TIMEOUT_ASYNC_PROCESSING = 5000;

    private static Instrumentation sInstrumentation;
    private static UiAutomation sUiAutomation;

    private AccessibilityViewTreeReportingActivity mActivity;

    @Rule
    public ActivityTestRule<AccessibilityViewTreeReportingActivity> mActivityRule =
            new ActivityTestRule<>(AccessibilityViewTreeReportingActivity.class, false, false);

    @BeforeClass
    public static void oneTimeSetup() throws Exception {
        sInstrumentation = InstrumentationRegistry.getInstrumentation();
        sUiAutomation = sInstrumentation.getUiAutomation();
    }

    @AfterClass
    public static void finalTearDown() throws Exception {
        sUiAutomation.destroy();
    }

    @Before
    public void setUp() throws Exception {
        mActivity = (AccessibilityViewTreeReportingActivity) launchActivityAndWaitForItToBeOnscreen(
                sInstrumentation, sUiAutomation, mActivityRule);
        setGetNonImportantViews(false);
    }


    @Test
    public void testDescendantsOfNotImportantViewReportedInOrder1() throws Exception {
        AccessibilityNodeInfo firstFrameLayout = getNodeByText(R.string.firstFrameLayout);
        assertNotNull(firstFrameLayout);
        assertSame(3, firstFrameLayout.getChildCount());

        // Check if the first child is the right one.
        AccessibilityNodeInfo firstTextView = getNodeByText(R.string.firstTextView);
        assertEquals(firstTextView, firstFrameLayout.getChild(0));

        // Check if the second child is the right one.
        AccessibilityNodeInfo firstEditText = getNodeByText(R.string.firstEditText);
        assertEquals(firstEditText, firstFrameLayout.getChild(1));

        // Check if the third child is the right one.
        AccessibilityNodeInfo firstButton = getNodeByText(R.string.firstButton);
        assertEquals(firstButton, firstFrameLayout.getChild(2));
    }

    @Test
    public void testDescendantsOfNotImportantViewReportedInOrder2() throws Exception {
        AccessibilityNodeInfo secondFrameLayout = getNodeByText(R.string.secondFrameLayout);
        assertNotNull(secondFrameLayout);
        assertSame(3, secondFrameLayout.getChildCount());

        // Check if the first child is the right one.
        AccessibilityNodeInfo secondTextView = getNodeByText(R.string.secondTextView);
        assertEquals(secondTextView, secondFrameLayout.getChild(0));

        // Check if the second child is the right one.
        AccessibilityNodeInfo secondEditText = getNodeByText(R.string.secondEditText);
        assertEquals(secondEditText, secondFrameLayout.getChild(1));

        // Check if the third child is the right one.
        AccessibilityNodeInfo secondButton = getNodeByText(R.string.secondButton);
        assertEquals(secondButton, secondFrameLayout.getChild(2));
    }

    @Test
    public void testDescendantsOfNotImportantViewReportedInOrder3() throws Exception {
        AccessibilityNodeInfo rootLinearLayout =
                getNodeByText(R.string.rootLinearLayout);
        assertNotNull(rootLinearLayout);
        assertSame(4, rootLinearLayout.getChildCount());

        // Check if the first child is the right one.
        AccessibilityNodeInfo firstFrameLayout =
                getNodeByText(R.string.firstFrameLayout);
        assertEquals(firstFrameLayout, rootLinearLayout.getChild(0));

        // Check if the second child is the right one.
        AccessibilityNodeInfo secondTextView = getNodeByText(R.string.secondTextView);
        assertEquals(secondTextView, rootLinearLayout.getChild(1));

        // Check if the third child is the right one.
        AccessibilityNodeInfo secondEditText = getNodeByText(R.string.secondEditText);
        assertEquals(secondEditText, rootLinearLayout.getChild(2));

        // Check if the fourth child is the right one.
        AccessibilityNodeInfo secondButton = getNodeByText(R.string.secondButton);
        assertEquals(secondButton, rootLinearLayout.getChild(3));
    }

    @Test
    public void testDrawingOrderInImportantParentFollowsXmlOrder() throws Exception {
        sInstrumentation.runOnMainSync(() -> mActivity.findViewById(R.id.firstLinearLayout)
                .setImportantForAccessibility(View.IMPORTANT_FOR_ACCESSIBILITY_YES));

        AccessibilityNodeInfo firstTextView = getNodeByText(R.string.firstTextView);
        AccessibilityNodeInfo firstEditText = getNodeByText(R.string.firstEditText);
        AccessibilityNodeInfo firstButton = getNodeByText(R.string.firstButton);

        // Drawing order is: firstTextView, firstEditText, firstButton
        assertTrue(firstTextView.getDrawingOrder() < firstEditText.getDrawingOrder());
        assertTrue(firstEditText.getDrawingOrder() < firstButton.getDrawingOrder());

        // Confirm that obtaining copies doesn't change our results
        AccessibilityNodeInfo copyOfFirstEditText = AccessibilityNodeInfo.obtain(firstEditText);
        assertTrue(firstTextView.getDrawingOrder() < copyOfFirstEditText.getDrawingOrder());
        assertTrue(copyOfFirstEditText.getDrawingOrder() < firstButton.getDrawingOrder());
    }

    @Test
    public void testDrawingOrderGettingAllViewsFollowsXmlOrder() throws Exception {
        setGetNonImportantViews(true);
        AccessibilityNodeInfo firstTextView = getNodeByText(R.string.firstTextView);
        AccessibilityNodeInfo firstEditText = getNodeByText(R.string.firstEditText);
        AccessibilityNodeInfo firstButton = getNodeByText(R.string.firstButton);

        // Drawing order is: firstTextView, firstEditText, firstButton
        assertTrue(firstTextView.getDrawingOrder() < firstEditText.getDrawingOrder());
        assertTrue(firstEditText.getDrawingOrder() < firstButton.getDrawingOrder());
    }

    @Test
    public void testDrawingOrderWithZCoordsDrawsHighestZLast() throws Exception {
        setGetNonImportantViews(true);
        sInstrumentation.runOnMainSync(() -> {
            mActivity.findViewById(R.id.firstTextView).setZ(50);
            mActivity.findViewById(R.id.firstEditText).setZ(100);
        });

        AccessibilityNodeInfo firstTextView = getNodeByText(R.string.firstTextView);
        AccessibilityNodeInfo firstEditText = getNodeByText(R.string.firstEditText);
        AccessibilityNodeInfo firstButton = getNodeByText(R.string.firstButton);

        // Drawing order is firstButton (no z), firstTextView (z=50), firstEditText (z=100)
        assertTrue(firstButton.getDrawingOrder() < firstTextView.getDrawingOrder());
        assertTrue(firstTextView.getDrawingOrder() < firstEditText.getDrawingOrder());
    }

    @Test
    public void testDrawingOrderWithCustomDrawingOrder() throws Exception {
        setGetNonImportantViews(true);
        sInstrumentation.runOnMainSync(() -> {
            // Reorganize the hiearchy to replace firstLinearLayout with one that allows us to
            // control the draw order
            LinearLayout rootLinearLayout =
                    (LinearLayout) mActivity.findViewById(R.id.rootLinearLayout);
            LinearLayout firstLinearLayout =
                    (LinearLayout) mActivity.findViewById(R.id.firstLinearLayout);
            View firstTextView = mActivity.findViewById(R.id.firstTextView);
            View firstEditText = mActivity.findViewById(R.id.firstEditText);
            View firstButton = mActivity.findViewById(R.id.firstButton);
            firstLinearLayout.removeAllViews();
            LinearLayoutWithDrawingOrder layoutWithDrawingOrder =
                    new LinearLayoutWithDrawingOrder(mActivity);
            rootLinearLayout.addView(layoutWithDrawingOrder);
            layoutWithDrawingOrder.addView(firstTextView);
            layoutWithDrawingOrder.addView(firstEditText);
            layoutWithDrawingOrder.addView(firstButton);
            layoutWithDrawingOrder.childDrawingOrder = new int[] {2, 0, 1};
        });

        AccessibilityNodeInfo firstTextView = getNodeByText(R.string.firstTextView);
        AccessibilityNodeInfo firstEditText = getNodeByText(R.string.firstEditText);
        AccessibilityNodeInfo firstButton = getNodeByText(R.string.firstButton);

        // Drawing order is firstEditText, firstButton, firstTextView
        assertTrue(firstEditText.getDrawingOrder() < firstButton.getDrawingOrder());
        assertTrue(firstButton.getDrawingOrder() < firstTextView.getDrawingOrder());
    }

    @Test
    public void testDrawingOrderWithNotImportantSiblingConsidersItsChildren() throws Exception {
        // Make the first frame layout a higher Z so it's drawn last
        sInstrumentation.runOnMainSync(
                () -> mActivity.findViewById(R.id.firstFrameLayout).setZ(100));
        AccessibilityNodeInfo secondTextView = getNodeByText(R.string.secondTextView);
        AccessibilityNodeInfo secondEditText = getNodeByText(R.string.secondEditText);
        AccessibilityNodeInfo secondButton = getNodeByText(R.string.secondButton);
        AccessibilityNodeInfo firstFrameLayout = getNodeByText( R.string.firstFrameLayout);
        assertTrue(secondTextView.getDrawingOrder() < firstFrameLayout.getDrawingOrder());
        assertTrue(secondEditText.getDrawingOrder() < firstFrameLayout.getDrawingOrder());
        assertTrue(secondButton.getDrawingOrder() < firstFrameLayout.getDrawingOrder());
    }

    @Test
    public void testDrawingOrderWithNotImportantParentConsidersParentSibling() throws Exception {
        AccessibilityNodeInfo firstFrameLayout = getNodeByText(R.string.firstFrameLayout);
        AccessibilityNodeInfo secondTextView = getNodeByText(R.string.secondTextView);
        AccessibilityNodeInfo secondEditText = getNodeByText(R.string.secondEditText);
        AccessibilityNodeInfo secondButton = getNodeByText(R.string.secondButton);

        assertTrue(secondTextView.getDrawingOrder() > firstFrameLayout.getDrawingOrder());
        assertTrue(secondEditText.getDrawingOrder() > firstFrameLayout.getDrawingOrder());
        assertTrue(secondButton.getDrawingOrder() > firstFrameLayout.getDrawingOrder());
    }

    @Test
    public void testDrawingOrderRootNodeHasIndex0() throws Exception {
        assertEquals(0, sUiAutomation.getRootInActiveWindow().getDrawingOrder());
    }

    @Test
    public void testAccessibilityImportanceReportingForImportantView() throws Exception {
        setGetNonImportantViews(true);
        sInstrumentation.runOnMainSync(() -> {
            // Manually control importance for firstButton
            View firstButton = mActivity.findViewById(R.id.firstButton);
            firstButton.setImportantForAccessibility(View.IMPORTANT_FOR_ACCESSIBILITY_YES);
        });

        AccessibilityNodeInfo firstButtonNode = getNodeByText(R.string.firstButton);
        assertTrue(firstButtonNode.isImportantForAccessibility());
    }

    @Test
    public void testAccessibilityImportanceReportingForUnimportantView() throws Exception {
        setGetNonImportantViews(true);
        sInstrumentation.runOnMainSync(() -> {
            View firstButton = mActivity.findViewById(R.id.firstButton);
            firstButton.setImportantForAccessibility(View.IMPORTANT_FOR_ACCESSIBILITY_NO);
        });

        AccessibilityNodeInfo firstButtonNode = getNodeByText(R.string.firstButton);
        assertFalse(firstButtonNode.isImportantForAccessibility());
    }

    @Test
    public void testAddViewToLayout_receiveSubtreeEvent() throws Throwable {
        final LinearLayout layout =
                (LinearLayout) mActivity.findViewById(R.id.secondLinearLayout);
        final Button newButton = new Button(mActivity);
        newButton.setText("New Button");
        newButton.setWidth(ViewGroup.LayoutParams.WRAP_CONTENT);
        newButton.setHeight(ViewGroup.LayoutParams.WRAP_CONTENT);
        AccessibilityEvent awaitedEvent =
                sUiAutomation.executeAndWaitForEvent(
                        () -> mActivity.runOnUiThread(() -> layout.addView(newButton)),
                        (event) -> {
                            boolean isContentChanged = event.getEventType()
                                    == AccessibilityEvent.TYPE_WINDOW_CONTENT_CHANGED;
                            int isSubTree = (event.getContentChangeTypes()
                                    & AccessibilityEvent.CONTENT_CHANGE_TYPE_SUBTREE);
                            boolean isFromThisPackage = TextUtils.equals(event.getPackageName(),
                                    mActivity.getPackageName());
                            return isContentChanged && (isSubTree != 0) && isFromThisPackage;
                        }, TIMEOUT_ASYNC_PROCESSING);
        // The event should come from a view that's important for accessibility, even though the
        // layout we added it to isn't important. Otherwise services may not find out about the
        // new button.
        assertTrue(awaitedEvent.getSource().isImportantForAccessibility());
    }

    private void setGetNonImportantViews(boolean getNonImportantViews) {
        AccessibilityServiceInfo serviceInfo = sUiAutomation.getServiceInfo();
        serviceInfo.flags &= ~AccessibilityServiceInfo.FLAG_INCLUDE_NOT_IMPORTANT_VIEWS;
        serviceInfo.flags |= getNonImportantViews ?
                AccessibilityServiceInfo.FLAG_INCLUDE_NOT_IMPORTANT_VIEWS : 0;
        sUiAutomation.setServiceInfo(serviceInfo);
    }

    private AccessibilityNodeInfo getNodeByText(int stringId) {
        return sUiAutomation.getRootInActiveWindow().findAccessibilityNodeInfosByText(
                sInstrumentation.getContext().getString(stringId)).get(0);
    }

    class LinearLayoutWithDrawingOrder extends LinearLayout {
        public int[] childDrawingOrder;
        LinearLayoutWithDrawingOrder(Context context) {
            super(context);
            setChildrenDrawingOrderEnabled(true);
        }

        @Override
        protected int getChildDrawingOrder(int childCount, int i) {
            return childDrawingOrder[i];
        }
    }
}
