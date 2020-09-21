/*
 * Copyright (C) 2010 The Android Open Source Project
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

import android.os.Message;
import android.os.Parcel;
import android.platform.test.annotations.Presubmit;
import android.test.suitebuilder.annotation.SmallTest;
import android.text.TextUtils;
import android.view.accessibility.AccessibilityEvent;
import android.view.accessibility.AccessibilityNodeInfo;
import android.view.accessibility.AccessibilityRecord;

import junit.framework.TestCase;

/**
 * Class for testing {@link AccessibilityEvent}.
 */
@Presubmit
public class AccessibilityEventTest extends TestCase {
    /**
     * Tests whether accessibility events are correctly written and
     * read from a parcel (version 1).
     */
    @SmallTest
    public void testMarshaling() throws Exception {
        // fully populate the event to marshal
        AccessibilityEvent sentEvent = AccessibilityEvent.obtain();
        fullyPopulateAccessibilityEvent(sentEvent);

        // marshal and unmarshal the event
        Parcel parcel = Parcel.obtain();
        sentEvent.writeToParcel(parcel, 0);
        parcel.setDataPosition(0);
        AccessibilityEvent receivedEvent = AccessibilityEvent.CREATOR.createFromParcel(parcel);

        // make sure all fields properly marshaled
        assertEqualsAccessiblityEvent(sentEvent, receivedEvent);

        parcel.recycle();
    }

    /**
     * Tests if {@link AccessibilityEvent} are properly reused.
     */
    @SmallTest
    public void testReuse() {
        AccessibilityEvent firstEvent = AccessibilityEvent.obtain();
        firstEvent.recycle();
        AccessibilityEvent secondEvent = AccessibilityEvent.obtain();
        assertSame("AccessibilityEvent not properly reused", firstEvent, secondEvent);
    }

    /**
     * Tests if {@link AccessibilityEvent} are properly recycled.
     */
    @SmallTest
    public void testRecycle() {
        // obtain and populate an event
        AccessibilityEvent populatedEvent = AccessibilityEvent.obtain();
        fullyPopulateAccessibilityEvent(populatedEvent);

        // recycle and obtain the same recycled instance
        populatedEvent.recycle();
        AccessibilityEvent recycledEvent = AccessibilityEvent.obtain();

        // check expectations
        assertAccessibilityEventCleared(recycledEvent);
    }

    /**
     * Tests whether the event types are correctly converted to strings.
     */
    @SmallTest
    public void testEventTypeToString() {
        assertEquals("TYPE_NOTIFICATION_STATE_CHANGED", AccessibilityEvent.eventTypeToString(
                AccessibilityEvent.TYPE_NOTIFICATION_STATE_CHANGED));
        assertEquals("TYPE_TOUCH_EXPLORATION_GESTURE_END", AccessibilityEvent.eventTypeToString(
                AccessibilityEvent.TYPE_TOUCH_EXPLORATION_GESTURE_END));
        assertEquals("TYPE_TOUCH_EXPLORATION_GESTURE_START", AccessibilityEvent.eventTypeToString(
                AccessibilityEvent.TYPE_TOUCH_EXPLORATION_GESTURE_START));
        assertEquals("TYPE_VIEW_CLICKED", AccessibilityEvent.eventTypeToString(
                AccessibilityEvent.TYPE_VIEW_CLICKED));
        assertEquals("TYPE_VIEW_FOCUSED", AccessibilityEvent.eventTypeToString(
                AccessibilityEvent.TYPE_VIEW_FOCUSED));
        assertEquals("TYPE_VIEW_HOVER_ENTER",
                AccessibilityEvent.eventTypeToString(AccessibilityEvent.TYPE_VIEW_HOVER_ENTER));
        assertEquals("TYPE_VIEW_HOVER_EXIT", AccessibilityEvent.eventTypeToString(
                AccessibilityEvent.TYPE_VIEW_HOVER_EXIT));
        assertEquals("TYPE_VIEW_LONG_CLICKED", AccessibilityEvent.eventTypeToString(
                AccessibilityEvent.TYPE_VIEW_LONG_CLICKED));
        assertEquals("TYPE_VIEW_CONTEXT_CLICKED", AccessibilityEvent.eventTypeToString(
                AccessibilityEvent.TYPE_VIEW_CONTEXT_CLICKED));
        assertEquals("TYPE_VIEW_SCROLLED", AccessibilityEvent.eventTypeToString(
                AccessibilityEvent.TYPE_VIEW_SCROLLED));
        assertEquals("TYPE_VIEW_SELECTED", AccessibilityEvent.eventTypeToString(
                AccessibilityEvent.TYPE_VIEW_SELECTED));
        assertEquals("TYPE_VIEW_TEXT_CHANGED", AccessibilityEvent.eventTypeToString(
                AccessibilityEvent .TYPE_VIEW_TEXT_CHANGED));
        assertEquals("TYPE_VIEW_TEXT_SELECTION_CHANGED", AccessibilityEvent.eventTypeToString(
                AccessibilityEvent.TYPE_VIEW_TEXT_SELECTION_CHANGED));
        assertEquals("TYPE_WINDOW_CONTENT_CHANGED", AccessibilityEvent.eventTypeToString(
                AccessibilityEvent.TYPE_WINDOW_CONTENT_CHANGED));
        assertEquals("TYPE_WINDOW_STATE_CHANGED", AccessibilityEvent.eventTypeToString(
                AccessibilityEvent.TYPE_WINDOW_STATE_CHANGED));
    }

    /**
     * Tests whether the event describes its contents consistently.
     */
    @SmallTest
    public void testDescribeContents() {
        AccessibilityEvent event = AccessibilityEvent.obtain();
        assertSame("Accessibility events always return 0 for this method.", 0,
                event.describeContents());
        fullyPopulateAccessibilityEvent(event);
        assertSame("Accessibility events always return 0 for this method.", 0,
                event.describeContents());
    }

    /**
     * Tests whether accessibility events are correctly written and
     * read from a parcel (version 2).
     */
    @SmallTest
    public void testMarshaling2() {
        AccessibilityEvent marshaledEvent = AccessibilityEvent.obtain();
        fullyPopulateAccessibilityEvent(marshaledEvent);
        Parcel parcel = Parcel.obtain();
        marshaledEvent.writeToParcel(parcel, 0);
        parcel.setDataPosition(0);
        AccessibilityEvent unmarshaledEvent = AccessibilityEvent.obtain();
        unmarshaledEvent.initFromParcel(parcel);
        assertEqualsAccessiblityEvent(marshaledEvent, unmarshaledEvent);

        parcel.recycle();
    }

    /**
     * While CharSequence is immutable, some classes implementing it are mutable. Make sure they
     * can't change the object by changing the objects backing CharSequence
     */
    @SmallTest
    public void testChangeTextAfterSetting_shouldNotAffectEvent() {
        final String originalText = "Cassowary";
        final String newText = "Hornbill";
        AccessibilityEvent event = AccessibilityEvent.obtain();
        StringBuffer updatingString = new StringBuffer(originalText);
        event.setBeforeText(updatingString);
        event.setContentDescription(updatingString);

        updatingString.delete(0, updatingString.length());
        updatingString.append(newText);

        assertTrue(TextUtils.equals(originalText, event.getBeforeText()));
        assertTrue(TextUtils.equals(originalText, event.getContentDescription()));
    }

    /**
     * Fully populates the {@link AccessibilityEvent} to marshal.
     *
     * @param sentEvent The event to populate.
     */
    private void fullyPopulateAccessibilityEvent(AccessibilityEvent sentEvent) {
        sentEvent.setAddedCount(1);
        sentEvent.setBeforeText("BeforeText");
        sentEvent.setChecked(true);
        sentEvent.setClassName("foo.bar.baz.Class");
        sentEvent.setContentDescription("ContentDescription");
        sentEvent.setCurrentItemIndex(1);
        sentEvent.setEnabled(true);
        sentEvent.setEventType(AccessibilityEvent.TYPE_VIEW_FOCUSED);
        sentEvent.setEventTime(1000);
        sentEvent.setFromIndex(1);
        sentEvent.setFullScreen(true);
        sentEvent.setItemCount(1);
        sentEvent.setPackageName("foo.bar.baz");
        sentEvent.setParcelableData(Message.obtain(null, 1, 2, 3));
        sentEvent.setPassword(true);
        sentEvent.setRemovedCount(1);
        sentEvent.getText().add("Foo");
        sentEvent.setMaxScrollX(1);
        sentEvent.setMaxScrollY(1);
        sentEvent.setScrollX(1);
        sentEvent.setScrollY(1);
        sentEvent.setScrollDeltaX(3);
        sentEvent.setScrollDeltaY(3);
        sentEvent.setToIndex(1);
        sentEvent.setScrollable(true);
        sentEvent.setAction(AccessibilityNodeInfo.ACTION_ACCESSIBILITY_FOCUS);
        sentEvent.setMovementGranularity(AccessibilityNodeInfo.MOVEMENT_GRANULARITY_LINE);

        AccessibilityRecord record = AccessibilityRecord.obtain();
        AccessibilityRecordTest.fullyPopulateAccessibilityRecord(record);
        sentEvent.appendRecord(record);
    }


    /**
     * Compares all properties of the <code>expectedEvent</code> and the
     * <code>receviedEvent</code> to verify that the received event is the one
     * that is expected.
     */
    private static void assertEqualsAccessiblityEvent(AccessibilityEvent expectedEvent,
            AccessibilityEvent receivedEvent) {
        assertEquals("addedCount has incorrect value", expectedEvent.getAddedCount(), receivedEvent
                .getAddedCount());
        assertEquals("beforeText has incorrect value", expectedEvent.getBeforeText(), receivedEvent
                .getBeforeText());
        assertEquals("checked has incorrect value", expectedEvent.isChecked(), receivedEvent
                .isChecked());
        assertEquals("className has incorrect value", expectedEvent.getClassName(), receivedEvent
                .getClassName());
        assertEquals("contentDescription has incorrect value", expectedEvent
                .getContentDescription(), receivedEvent.getContentDescription());
        assertEquals("currentItemIndex has incorrect value", expectedEvent.getCurrentItemIndex(),
                receivedEvent.getCurrentItemIndex());
        assertEquals("enabled has incorrect value", expectedEvent.isEnabled(), receivedEvent
                .isEnabled());
        assertEquals("eventType has incorrect value", expectedEvent.getEventType(), receivedEvent
                .getEventType());
        assertEquals("fromIndex has incorrect value", expectedEvent.getFromIndex(), receivedEvent
                .getFromIndex());
        assertEquals("fullScreen has incorrect value", expectedEvent.isFullScreen(), receivedEvent
                .isFullScreen());
        assertEquals("itemCount has incorrect value", expectedEvent.getItemCount(), receivedEvent
                .getItemCount());
        assertEquals("password has incorrect value", expectedEvent.isPassword(), receivedEvent
                .isPassword());
        assertEquals("removedCount has incorrect value", expectedEvent.getRemovedCount(),
                receivedEvent.getRemovedCount());
        AccessibilityRecordTest.assertEqualsText(expectedEvent.getText(), receivedEvent.getText());
        assertEquals("must have one record", expectedEvent.getRecordCount(),
                receivedEvent.getRecordCount());
        assertSame("maxScrollX has incorect value", expectedEvent.getMaxScrollX(),
                receivedEvent.getMaxScrollX());
        assertSame("maxScrollY has incorect value", expectedEvent.getMaxScrollY(),
                receivedEvent.getMaxScrollY());
        assertSame("scrollX has incorect value", expectedEvent.getScrollX(),
                receivedEvent.getScrollX());
        assertSame("scrollY has incorect value", expectedEvent.getScrollY(),
                receivedEvent.getScrollY());
        assertSame("scrollDeltaX has incorect value", expectedEvent.getScrollDeltaX(),
                receivedEvent.getScrollDeltaX());
        assertSame("scrollDeltaY has incorect value", expectedEvent.getScrollDeltaY(),
                receivedEvent.getScrollDeltaY());
        assertSame("toIndex has incorect value", expectedEvent.getToIndex(),
                receivedEvent.getToIndex());
        assertSame("scrollable has incorect value", expectedEvent.isScrollable(),
                receivedEvent.isScrollable());
        assertSame("granularity has incorect value", expectedEvent.getMovementGranularity(),
                receivedEvent.getMovementGranularity());
        assertSame("action has incorect value", expectedEvent.getAction(),
                receivedEvent.getAction());
        assertSame("windowChangeTypes has incorect value", expectedEvent.getWindowChanges(),
                receivedEvent.getWindowChanges());

        assertSame("parcelableData has incorect value",
                ((Message) expectedEvent.getParcelableData()).what,
                ((Message) receivedEvent.getParcelableData()).what);

        AccessibilityRecord receivedRecord = receivedEvent.getRecord(0);
        AccessibilityRecordTest.assertEqualAccessibilityRecord(expectedEvent, receivedRecord);
    }

    /**
     * Asserts that an {@link AccessibilityEvent} is cleared.
     *
     * @param event The event to check.
     */
    private static void assertAccessibilityEventCleared(AccessibilityEvent event) {
        AccessibilityRecordTest.assertAccessibilityRecordCleared(event);
        TestCase.assertEquals("eventTime not properly recycled", 0, event.getEventTime());
        TestCase.assertEquals("eventType not properly recycled", 0, event.getEventType());
        TestCase.assertNull("packageName not properly recycled", event.getPackageName());
    }
}
