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

package android.view.accessibility.cts;

import android.graphics.Rect;
import android.os.Bundle;
import android.os.Parcel;
import android.platform.test.annotations.Presubmit;
import android.test.AndroidTestCase;
import android.test.suitebuilder.annotation.SmallTest;
import android.text.InputType;
import android.text.TextUtils;
import android.view.View;
import android.view.accessibility.AccessibilityEvent;
import android.view.accessibility.AccessibilityNodeInfo;
import android.view.accessibility.AccessibilityNodeInfo.AccessibilityAction;
import android.view.accessibility.AccessibilityNodeInfo.CollectionInfo;
import android.view.accessibility.AccessibilityNodeInfo.CollectionItemInfo;
import android.view.accessibility.AccessibilityNodeInfo.RangeInfo;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

/**
 * Class for testing {@link AccessibilityNodeInfo}.
 */
@Presubmit
public class AccessibilityNodeInfoTest extends AndroidTestCase {

    @SmallTest
    public void testMarshaling() throws Exception {
        // fully populate the node info to marshal
        AccessibilityNodeInfo sentInfo = AccessibilityNodeInfo.obtain(new View(getContext()));
        fullyPopulateAccessibilityNodeInfo(sentInfo);

        // marshal and unmarshal the node info
        Parcel parcel = Parcel.obtain();
        sentInfo.writeToParcelNoRecycle(parcel, 0);
        parcel.setDataPosition(0);
        AccessibilityNodeInfo receivedInfo = AccessibilityNodeInfo.CREATOR.createFromParcel(parcel);

        // make sure all fields properly marshaled
        assertEqualsAccessibilityNodeInfo(sentInfo, receivedInfo);

        parcel.recycle();
    }

    /**
     * Tests if {@link AccessibilityNodeInfo}s are properly reused.
     */
    @SmallTest
    public void testReuse() {
        AccessibilityEvent firstInfo = AccessibilityEvent.obtain();
        firstInfo.recycle();
        AccessibilityEvent secondInfo = AccessibilityEvent.obtain();
        assertSame("AccessibilityNodeInfo not properly reused", firstInfo, secondInfo);
    }

    /**
     * Tests if {@link AccessibilityNodeInfo} are properly recycled.
     */
    @SmallTest
    public void testRecycle() {
        // obtain and populate an node info
        AccessibilityNodeInfo populatedInfo = AccessibilityNodeInfo.obtain();
        fullyPopulateAccessibilityNodeInfo(populatedInfo);

        // recycle and obtain the same recycled instance
        populatedInfo.recycle();
        AccessibilityNodeInfo recycledInfo = AccessibilityNodeInfo.obtain();

        // check expectations
        assertAccessibilityNodeInfoCleared(recycledInfo);
    }

    /**
     * Tests whether the event describes its contents consistently.
     */
    @SmallTest
    public void testDescribeContents() {
        AccessibilityNodeInfo info = AccessibilityNodeInfo.obtain();
        assertSame("Accessibility node infos always return 0 for this method.", 0,
                info.describeContents());
        fullyPopulateAccessibilityNodeInfo(info);
        assertSame("Accessibility node infos always return 0 for this method.", 0,
                info.describeContents());
    }

    /**
     * Tests whether accessibility actions are properly added.
     */
    @SmallTest
    public void testAddActions() {
        List<AccessibilityAction> customActions = new ArrayList<AccessibilityAction>();
        customActions.add(new AccessibilityAction(AccessibilityNodeInfo.ACTION_FOCUS, "Foo"));
        customActions.add(new AccessibilityAction(R.id.foo_custom_action, "Foo"));

        AccessibilityNodeInfo info = AccessibilityNodeInfo.obtain();
        info.addAction(AccessibilityNodeInfo.ACTION_FOCUS);
        info.addAction(AccessibilityNodeInfo.ACTION_CLEAR_FOCUS);
        for (AccessibilityAction customAction : customActions) {
            info.addAction(customAction);
        }

        assertSame(info.getActions(), (AccessibilityNodeInfo.ACTION_FOCUS
                | AccessibilityNodeInfo.ACTION_CLEAR_FOCUS));

        List<AccessibilityAction> allActions = new ArrayList<AccessibilityAction>();
        allActions.add(AccessibilityAction.ACTION_CLEAR_FOCUS);
        allActions.addAll(customActions);
        assertEquals(info.getActionList(), allActions);
    }

    /**
     * Tests whether we catch addition of an action with invalid id.
     */
    @SmallTest
    public void testCreateInvalidActionId() {
        try {
            new AccessibilityAction(3, null);
        } catch (IllegalArgumentException iae) {
            /* expected */
        }
    }

    /**
     * Tests whether accessibility actions are properly removed.
     */
    @SmallTest
    public void testRemoveActions() {
        AccessibilityNodeInfo info = AccessibilityNodeInfo.obtain();

        info.addAction(AccessibilityNodeInfo.ACTION_FOCUS);
        assertSame(info.getActions(), AccessibilityNodeInfo.ACTION_FOCUS);

        info.removeAction(AccessibilityNodeInfo.ACTION_FOCUS);
        assertSame(info.getActions(), 0);
        assertTrue(info.getActionList().isEmpty());

        AccessibilityAction customFocus = new AccessibilityAction(
                AccessibilityNodeInfo.ACTION_FOCUS, "Foo");
        info.addAction(AccessibilityNodeInfo.ACTION_FOCUS);
        info.addAction(customFocus);
        assertSame(info.getActionList().size(), 1);
        assertEquals(info.getActionList().get(0), customFocus);
        assertSame(info.getActions(), AccessibilityNodeInfo.ACTION_FOCUS);

        info.removeAction(customFocus);
        assertSame(info.getActions(), 0);
        assertTrue(info.getActionList().isEmpty());
    }

    /**
     * While CharSequence is immutable, some classes implementing it are mutable. Make sure they
     * can't change the object by changing the objects backing CharSequence
     */
    @SmallTest
    public void testChangeTextAfterSetting_shouldNotAffectInfo() {
        final String originalText = "Cassowaries";
        final String newText = "Hornbill";
        AccessibilityNodeInfo info = AccessibilityNodeInfo.obtain();
        StringBuffer updatingString = new StringBuffer(originalText);
        info.setText(updatingString);
        info.setError(updatingString);
        info.setContentDescription(updatingString);

        updatingString.delete(0, updatingString.length());
        updatingString.append(newText);

        assertTrue(TextUtils.equals(originalText, info.getText()));
        assertTrue(TextUtils.equals(originalText, info.getError()));
        assertTrue(TextUtils.equals(originalText, info.getContentDescription()));
    }

    @SmallTest
    public void testIsHeadingTakesOldApiIntoAccount() {
        final AccessibilityNodeInfo info = AccessibilityNodeInfo.obtain();
        assertFalse(info.isHeading());
        final CollectionItemInfo headingItemInfo = CollectionItemInfo.obtain(0, 1, 0, 1, true);
        info.setCollectionItemInfo(headingItemInfo);
        assertTrue(info.isHeading());
        final CollectionItemInfo nonHeadingItemInfo = CollectionItemInfo.obtain(0, 1, 0, 1, false);
        info.setCollectionItemInfo(nonHeadingItemInfo);
        assertFalse(info.isHeading());
    }

    /**
     * Fully populates the {@link AccessibilityNodeInfo} to marshal.
     *
     * @param info The node info to populate.
     */
    private void fullyPopulateAccessibilityNodeInfo(AccessibilityNodeInfo info) {
        // Populate 10 fields
        info.setBoundsInParent(new Rect(1,1,1,1));
        info.setBoundsInScreen(new Rect(2,2,2,2));
        info.setClassName("foo.bar.baz.Class");
        info.setContentDescription("content description");
        info.setTooltipText("tooltip");
        info.setPackageName("foo.bar.baz");
        info.setText("text");
        info.setHintText("hint");
        // 2 children = 1 field
        info.addChild(new View(getContext()));
        info.addChild(new View(getContext()), 1);
        // 2 built-in actions and 2 custom actions, but all are in 1 field
        info.addAction(AccessibilityNodeInfo.ACTION_FOCUS);
        info.addAction(AccessibilityNodeInfo.ACTION_CLEAR_FOCUS);
        info.addAction(new AccessibilityAction(AccessibilityNodeInfo.ACTION_FOCUS, "Foo"));
        info.addAction(new AccessibilityAction(R.id.foo_custom_action, "Foo"));

        // Populate 10 fields
        info.setMovementGranularities(AccessibilityNodeInfo.MOVEMENT_GRANULARITY_LINE);
        info.setViewIdResourceName("foo.bar:id/baz");
        info.setDrawingOrder(5);
        info.setAvailableExtraData(
                Arrays.asList(AccessibilityNodeInfo.EXTRA_DATA_TEXT_CHARACTER_LOCATION_KEY));
        info.setPaneTitle("Pane title");
        info.setError("Error text");
        info.setMaxTextLength(42);
        // Text selection is 2 fields
        info.setTextSelection(3, 7);
        info.setLiveRegion(View.ACCESSIBILITY_LIVE_REGION_POLITE);

        // Populate 10 fields
        Bundle extras = info.getExtras();
        extras.putBoolean("areCassowariesAwesome", true);
        info.setInputType(InputType.TYPE_CLASS_TEXT);
        info.setRangeInfo(RangeInfo.obtain(RangeInfo.RANGE_TYPE_FLOAT, 0.05f, 1.0f, 0.01f));
        info.setCollectionInfo(
                CollectionInfo.obtain(2, 2, true, CollectionInfo.SELECTION_MODE_MULTIPLE));
        info.setCollectionItemInfo(CollectionItemInfo.obtain(1, 2, 3, 4, true, true));
        info.setParent(new View(getContext()));
        info.setSource(new View(getContext())); // Populates 2 fields: source and window id
        info.setTraversalBefore(new View(getContext()));
        info.setTraversalAfter(new View(getContext()));

        // Populate 2 fields
        info.setLabeledBy(new View(getContext()));
        info.setLabelFor(new View(getContext()));

        // And Boolean properties are another field. Total is 33

        // 10 Boolean properties
        info.setCheckable(true);
        info.setChecked(true);
        info.setFocusable(true);
        info.setFocused(true);
        info.setSelected(true);
        info.setClickable(true);
        info.setLongClickable(true);
        info.setEnabled(true);
        info.setPassword(true);
        info.setScrollable(true);

        // 10 Boolean properties
        info.setAccessibilityFocused(true);
        info.setVisibleToUser(true);
        info.setEditable(true);
        info.setCanOpenPopup(true);
        info.setDismissable(true);
        info.setMultiLine(true);
        info.setContentInvalid(true);
        info.setContextClickable(true);
        info.setImportantForAccessibility(true);
        info.setScreenReaderFocusable(true);

        // 2 Boolean properties, for a total of 22
        info.setShowingHintText(true);
        info.setHeading(true);
    }

    /**
     * Compares all properties of the <code>expectedInfo</code> and the
     * <code>receivedInfo</code> to verify that the received node info is
     * the one that is expected.
     */
    public static void assertEqualsAccessibilityNodeInfo(AccessibilityNodeInfo expectedInfo,
            AccessibilityNodeInfo receivedInfo) {
        // Check 10 fields
        Rect expectedBounds = new Rect();
        Rect receivedBounds = new Rect();
        expectedInfo.getBoundsInParent(expectedBounds);
        receivedInfo.getBoundsInParent(receivedBounds);
        assertEquals("boundsInParent has incorrect value", expectedBounds, receivedBounds);
        expectedInfo.getBoundsInScreen(expectedBounds);
        receivedInfo.getBoundsInScreen(receivedBounds);
        assertEquals("boundsInScreen has incorrect value", expectedBounds, receivedBounds);
        assertEquals("className has incorrect value", expectedInfo.getClassName(),
                receivedInfo.getClassName());
        assertEquals("contentDescription has incorrect value", expectedInfo.getContentDescription(),
                receivedInfo.getContentDescription());
        assertEquals("tooltip text has incorrect value", expectedInfo.getTooltipText(),
                receivedInfo.getTooltipText());
        assertEquals("packageName has incorrect value", expectedInfo.getPackageName(),
                receivedInfo.getPackageName());
        assertEquals("text has incorrect value", expectedInfo.getText(), receivedInfo.getText());
        assertEquals("Hint text has incorrect value",
                expectedInfo.getHintText(), receivedInfo.getHintText());
        assertSame("childCount has incorrect value", expectedInfo.getChildCount(),
                receivedInfo.getChildCount());
        // Actions is just 1 field, but we check it 2 ways
        assertSame("actions has incorrect value", expectedInfo.getActions(),
                receivedInfo.getActions());
        assertEquals("actionsSet has incorrect value", expectedInfo.getActionList(),
                receivedInfo.getActionList());

        // Check 10 fields
        assertSame("movementGranularities has incorrect value",
                expectedInfo.getMovementGranularities(),
                receivedInfo.getMovementGranularities());
        assertEquals("viewId has incorrect value", expectedInfo.getViewIdResourceName(),
                receivedInfo.getViewIdResourceName());
        assertEquals("drawing order has incorrect value", expectedInfo.getDrawingOrder(),
                receivedInfo.getDrawingOrder());
        assertEquals("Extra data flags have incorrect value", expectedInfo.getAvailableExtraData(),
                receivedInfo.getAvailableExtraData());
        assertEquals("Pane title has incorrect value", expectedInfo.getPaneTitle(),
                receivedInfo.getPaneTitle());
        assertEquals("Error text has incorrect value", expectedInfo.getError(),
                receivedInfo.getError());
        assertEquals("Max text length has incorrect value", expectedInfo.getMaxTextLength(),
                receivedInfo.getMaxTextLength());
        assertEquals("Text selection start has incorrect value",
                expectedInfo.getTextSelectionStart(), receivedInfo.getTextSelectionStart());
        assertEquals("Text selection end has incorrect value",
                expectedInfo.getTextSelectionEnd(), receivedInfo.getTextSelectionEnd());
        assertEquals("Live region has incorrect value", expectedInfo.getLiveRegion(),
                receivedInfo.getLiveRegion());

        // Check 2 fields
        // Extras is a Bundle, and Bundle doesn't declare equals. Comparing the keyset gets us
        // what we need, though
        assertEquals("Extras have incorrect value", expectedInfo.getExtras().keySet(),
                receivedInfo.getExtras().keySet());
        assertEquals("InputType has incorrect value", expectedInfo.getInputType(),
                receivedInfo.getInputType());

        // Check 3 fields with sub-objects
        RangeInfo expectedRange = expectedInfo.getRangeInfo();
        RangeInfo receivedRange = receivedInfo.getRangeInfo();
        assertEquals("Range info has incorrect value", (expectedRange != null),
                (receivedRange != null));
        if (expectedRange != null) {
            assertEquals("RangeInfo#getCurrent has incorrect value", expectedRange.getCurrent(),
                    receivedRange.getCurrent());
            assertEquals("RangeInfo#getMin has incorrect value", expectedRange.getMin(),
                    receivedRange.getMin());
            assertEquals("RangeInfo#getMax has incorrect value", expectedRange.getMax(),
                    receivedRange.getMax());
            assertEquals("RangeInfo#getType has incorrect value", expectedRange.getType(),
                    receivedRange.getType());
        }

        CollectionInfo expectedCollectionInfo = expectedInfo.getCollectionInfo();
        CollectionInfo receivedCollectionInfo = receivedInfo.getCollectionInfo();
        assertEquals("Collection info has incorrect value", (expectedCollectionInfo != null),
                (receivedCollectionInfo != null));
        if (expectedCollectionInfo != null) {
            assertEquals("CollectionInfo#getColumnCount has incorrect value",
                    expectedCollectionInfo.getColumnCount(),
                    receivedCollectionInfo.getColumnCount());
            assertEquals("CollectionInfo#getRowCount has incorrect value",
                    expectedCollectionInfo.getRowCount(), receivedCollectionInfo.getRowCount());
            assertEquals("CollectionInfo#getSelectionMode has incorrect value",
                    expectedCollectionInfo.getSelectionMode(),
                    receivedCollectionInfo.getSelectionMode());
        }

        CollectionItemInfo expectedItemInfo = expectedInfo.getCollectionItemInfo();
        CollectionItemInfo receivedItemInfo = receivedInfo.getCollectionItemInfo();
        assertEquals("Collection item info has incorrect value", (expectedItemInfo != null),
                (receivedItemInfo != null));
        if (expectedItemInfo != null) {
            assertEquals("CollectionItemInfo#getColumnIndex has incorrect value",
                    expectedItemInfo.getColumnIndex(),
                    receivedItemInfo.getColumnIndex());
            assertEquals("CollectionItemInfo#getColumnSpan has incorrect value",
                    expectedItemInfo.getColumnSpan(),
                    receivedItemInfo.getColumnSpan());
            assertEquals("CollectionItemInfo#getRowIndex has incorrect value",
                    expectedItemInfo.getRowIndex(),
                    receivedItemInfo.getRowIndex());
            assertEquals("CollectionItemInfo#getRowSpan has incorrect value",
                    expectedItemInfo.getRowSpan(),
                    receivedItemInfo.getRowSpan());
        }

        // And the boolean properties are another field, for a total of 26
        // Missing parent: Tested end-to-end in AccessibilityWindowTraversalTest#testObjectContract
        //                 (getting a child is also checked there)
        //         traversalbefore: AccessibilityEndToEndTest
        //                          #testTraversalBeforeReportedToAccessibility
        //         traversalafter: AccessibilityEndToEndTest
        //                         #testTraversalAfterReportedToAccessibility
        //         labelfor: AccessibilityEndToEndTest#testLabelForReportedToAccessibility
        //         labeledby: AccessibilityEndToEndTest#testLabelForReportedToAccessibility
        //         windowid: Not directly observable
        //         sourceid: Not directly observable
        // Which brings us to a total of 33

        // 10 Boolean properties
        assertSame("checkable has incorrect value", expectedInfo.isCheckable(),
                receivedInfo.isCheckable());
        assertSame("checked has incorrect value", expectedInfo.isChecked(),
                receivedInfo.isChecked());
        assertSame("focusable has incorrect value", expectedInfo.isFocusable(),
                receivedInfo.isFocusable());
        assertSame("focused has incorrect value", expectedInfo.isFocused(),
                receivedInfo.isFocused());
        assertSame("selected has incorrect value", expectedInfo.isSelected(),
                receivedInfo.isSelected());
        assertSame("clickable has incorrect value", expectedInfo.isClickable(),
                receivedInfo.isClickable());
        assertSame("longClickable has incorrect value", expectedInfo.isLongClickable(),
                receivedInfo.isLongClickable());
        assertSame("enabled has incorrect value", expectedInfo.isEnabled(),
                receivedInfo.isEnabled());
        assertSame("password has incorrect value", expectedInfo.isPassword(),
                receivedInfo.isPassword());
        assertSame("scrollable has incorrect value", expectedInfo.isScrollable(),
                receivedInfo.isScrollable());

        // 10 Boolean properties
        assertSame("AccessibilityFocused has incorrect value",
                expectedInfo.isAccessibilityFocused(),
                receivedInfo.isAccessibilityFocused());
        assertSame("VisibleToUser has incorrect value", expectedInfo.isVisibleToUser(),
                receivedInfo.isVisibleToUser());
        assertSame("Editable has incorrect value", expectedInfo.isEditable(),
                receivedInfo.isEditable());
        assertSame("canOpenPopup has incorrect value", expectedInfo.canOpenPopup(),
                receivedInfo.canOpenPopup());
        assertSame("Dismissable has incorrect value", expectedInfo.isDismissable(),
                receivedInfo.isDismissable());
        assertSame("Multiline has incorrect value", expectedInfo.isMultiLine(),
                receivedInfo.isMultiLine());
        assertSame("ContentInvalid has incorrect value", expectedInfo.isContentInvalid(),
                receivedInfo.isContentInvalid());
        assertSame("contextClickable has incorrect value", expectedInfo.isContextClickable(),
                receivedInfo.isContextClickable());
        assertSame("importantForAccessibility has incorrect value",
                expectedInfo.isImportantForAccessibility(),
                receivedInfo.isImportantForAccessibility());
        assertSame("isScreenReaderFocusable has incorrect value",
                expectedInfo.isScreenReaderFocusable(), receivedInfo.isScreenReaderFocusable());

        // 2 Boolean properties, for a total of 22
        assertSame("isShowingHint has incorrect value",
                expectedInfo.isShowingHintText(), receivedInfo.isShowingHintText());
        assertSame("isHeading has incorrect value",
                expectedInfo.isHeading(), receivedInfo.isHeading());
    }

    /**
     * Asserts that an {@link AccessibilityNodeInfo} is cleared.
     *
     * @param info The node info to check.
     */
    public static void assertAccessibilityNodeInfoCleared(AccessibilityNodeInfo info) {
        // Check 10 fields
        Rect bounds = new Rect();
        info.getBoundsInParent(bounds);
        assertTrue("boundsInParent not properly recycled", bounds.isEmpty());
        info.getBoundsInScreen(bounds);
        assertTrue("boundsInScreen not properly recycled", bounds.isEmpty());
        assertNull("className not properly recycled", info.getClassName());
        assertNull("contentDescription not properly recycled", info.getContentDescription());
        assertNull("tooltiptext not properly recycled", info.getTooltipText());
        assertNull("packageName not properly recycled", info.getPackageName());
        assertNull("text not properly recycled", info.getText());
        assertNull("Hint text not properly recycled", info.getHintText());
        assertEquals("Children list not properly recycled", 0, info.getChildCount());
        // Actions are in one field
        assertSame("actions not properly recycled", 0, info.getActions());
        assertTrue("action list not properly recycled", info.getActionList().isEmpty());

        // Check 10 fields
        assertSame("movementGranularities not properly recycled", 0,
                info.getMovementGranularities());
        assertNull("viewId not properly recycled", info.getViewIdResourceName());
        assertEquals(0, info.getDrawingOrder());
        assertTrue(info.getAvailableExtraData().isEmpty());
        assertNull("Pane title not properly recycled", info.getPaneTitle());
        assertNull("Error text not properly recycled", info.getError());
        assertEquals("Max text length not properly recycled", -1, info.getMaxTextLength());
        assertEquals("Text selection start not properly recycled",
                -1, info.getTextSelectionStart());
        assertEquals("Text selection end not properly recycled", -1, info.getTextSelectionEnd());
        assertEquals("Live region not properly recycled", 0, info.getLiveRegion());

        // Check 5 fields
        assertEquals("Extras not properly recycled", 0, info.getExtras().keySet().size());
        assertEquals("Input type not properly recycled", 0, info.getInputType());
        assertNull("Range info not properly recycled", info.getRangeInfo());
        assertNull("Collection info not properly recycled", info.getCollectionInfo());
        assertNull("Collection item info not properly recycled", info.getCollectionItemInfo());

        // And Boolean properties brings up to 26 fields
        // Missing:
        //  parent (can't be performed on sealed instance, even if null)
        //  traversalbefore (can't be performed on sealed instance, even if null)
        //  traversalafter (can't be performed on sealed instance, even if null)
        //  labelfor (can't be performed on sealed instance, even if null)
        //  labeledby (can't be performed on sealed instance, even if null)
        //  sourceId (not directly observable)
        //  windowId (not directly observable)
        // a total of 33

        // 10 Boolean properties
        assertFalse("checkable not properly recycled", info.isCheckable());
        assertFalse("checked not properly recycled", info.isChecked());
        assertFalse("focusable not properly recycled", info.isFocusable());
        assertFalse("focused not properly recycled", info.isFocused());
        assertFalse("selected not properly recycled", info.isSelected());
        assertFalse("clickable not properly recycled", info.isClickable());
        assertFalse("longClickable not properly recycled", info.isLongClickable());
        assertFalse("enabled not properly recycled", info.isEnabled());
        assertFalse("password not properly recycled", info.isPassword());
        assertFalse("scrollable not properly recycled", info.isScrollable());

        // 10 Boolean properties
        assertFalse("accessibility focused not properly recycled", info.isAccessibilityFocused());
        assertFalse("VisibleToUser not properly recycled", info.isVisibleToUser());
        assertFalse("Editable not properly recycled", info.isEditable());
        assertFalse("canOpenPopup not properly recycled", info.canOpenPopup());
        assertFalse("Dismissable not properly recycled", info.isDismissable());
        assertFalse("Multiline not properly recycled", info.isMultiLine());
        assertFalse("ContentInvalid not properly recycled", info.isContentInvalid());
        assertFalse("contextClickable not properly recycled", info.isContextClickable());
        assertFalse("importantForAccessibility not properly recycled",
                info.isImportantForAccessibility());
        assertFalse("ScreenReaderFocusable not properly recycled", info.isScreenReaderFocusable());

        // 2 Boolean properties
        assertFalse("isShowingHint not properly reset", info.isShowingHintText());
        assertFalse("isHeading not properly reset", info.isHeading());
    }
}
