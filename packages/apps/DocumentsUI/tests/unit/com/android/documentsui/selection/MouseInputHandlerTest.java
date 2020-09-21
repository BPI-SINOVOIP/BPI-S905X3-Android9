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

package com.android.documentsui.selection;

import static com.android.documentsui.testing.TestEvents.Mouse.ALT_CLICK;
import static com.android.documentsui.testing.TestEvents.Mouse.CLICK;
import static com.android.documentsui.testing.TestEvents.Mouse.CTRL_CLICK;
import static com.android.documentsui.testing.TestEvents.Mouse.SECONDARY_CLICK;
import static com.android.documentsui.testing.TestEvents.Mouse.SHIFT_CLICK;
import static com.android.documentsui.testing.TestEvents.Mouse.TERTIARY_CLICK;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;

import android.support.test.filters.SmallTest;
import android.support.test.runner.AndroidJUnit4;
import android.support.v7.widget.RecyclerView;
import android.view.MotionEvent;

import com.android.documentsui.selection.SelectionHelper;
import com.android.documentsui.selection.testing.SelectionHelpers;
import com.android.documentsui.selection.testing.SelectionProbe;
import com.android.documentsui.selection.testing.TestData;
import com.android.documentsui.selection.testing.TestEvents;
import com.android.documentsui.selection.testing.TestMouseCallbacks;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;

import java.util.List;

@RunWith(AndroidJUnit4.class)
@SmallTest
public final class MouseInputHandlerTest {

    private static final List<String> ITEMS = TestData.create(100);

    private MouseInputHandler mInputDelegate;
    private TestMouseCallbacks mCallbacks;
    private TestItemDetailsLookup mDetailsLookup;
    private SelectionProbe mSelection;
    private SelectionHelper mSelectionMgr;

    private TestEvents.Builder mEvent;

    @Before
    public void setUp() {

        mSelectionMgr = SelectionHelpers.createTestInstance(ITEMS);
        mDetailsLookup = new TestItemDetailsLookup();
        mSelection = new SelectionProbe(mSelectionMgr);

        mCallbacks = new TestMouseCallbacks();
        mInputDelegate = new MouseInputHandler(mSelectionMgr, mDetailsLookup, mCallbacks);

        mEvent = TestEvents.builder().mouse();
        mDetailsLookup.initAt(RecyclerView.NO_POSITION);
    }

    @Test
    public void testConfirmedClick_StartsSelection() {
        mDetailsLookup.initAt(11).setInItemSelectRegion(true);
        mInputDelegate.onSingleTapConfirmed(CLICK);

        mSelection.assertSelection(11);
    }

    @Test
    public void testClickOnSelectRegion_AddsToSelection() {
        mDetailsLookup.initAt(11).setInItemSelectRegion(true);
        mInputDelegate.onSingleTapConfirmed(CLICK);

        mDetailsLookup.initAt(10).setInItemSelectRegion(true);
        mInputDelegate.onSingleTapUp(CLICK);

        mSelection.assertSelected(10, 11);
    }

    @Test
    public void testClickOnIconOfSelectedItem_RemovesFromSelection() {
        mDetailsLookup.initAt(8).setInItemSelectRegion(true);
        mInputDelegate.onSingleTapConfirmed(CLICK);

        mDetailsLookup.initAt(11);
        mInputDelegate.onSingleTapUp(SHIFT_CLICK);
        mSelection.assertSelected(8, 9, 10, 11);

        mDetailsLookup.initAt(9);
        mInputDelegate.onSingleTapUp(CLICK);
        mSelection.assertSelected(8, 10, 11);
    }

    @Test
    public void testRightClickDown_StartsContextMenu() {
        mInputDelegate.onDown(SECONDARY_CLICK);

        mCallbacks.assertLastEvent(SECONDARY_CLICK);
    }

    @Test
    public void testAltClickDown_StartsContextMenu() {
        mInputDelegate.onDown(ALT_CLICK);

        mCallbacks.assertLastEvent(ALT_CLICK);
    }

    @Test
    public void testScroll_shouldTrap() {
        mDetailsLookup.initAt(0);
        assertTrue(mInputDelegate.onScroll(
                null,
                mEvent.action(MotionEvent.ACTION_MOVE).primary().build(),
                -1,
                -1));
    }

    @Test
    public void testScroll_NoTrapForTwoFinger() {
        mDetailsLookup.initAt(0);
        assertFalse(mInputDelegate.onScroll(
                null,
                mEvent.action(MotionEvent.ACTION_MOVE).build(),
                -1,
                -1));
    }

    @Test
    public void testUnconfirmedCtrlClick_AddsToExistingSelection() {
        mDetailsLookup.initAt(7).setInItemSelectRegion(true);
        mInputDelegate.onSingleTapConfirmed(CLICK);

        mDetailsLookup.initAt(11);
        mInputDelegate.onSingleTapUp(CTRL_CLICK);

        mSelection.assertSelection(7, 11);
    }

    @Test
    public void testUnconfirmedShiftClick_ExtendsSelection() {
        mDetailsLookup.initAt(7).setInItemSelectRegion(true);
        mInputDelegate.onSingleTapConfirmed(CLICK);

        mDetailsLookup.initAt(11);
        mInputDelegate.onSingleTapUp(SHIFT_CLICK);

        mSelection.assertSelection(7, 8, 9, 10, 11);
    }

    @Test
    public void testConfirmedShiftClick_ExtendsSelectionFromOriginFocus() {
        TestItemDetails item = mDetailsLookup.initAt(7);
        mCallbacks.focusItem(item);

        // This is a hack-y test, since the real FocusManager would've set range begin itself.
        mSelectionMgr.anchorRange(7);
        mSelection.assertNoSelection();

        mDetailsLookup.initAt(11);
        mInputDelegate.onSingleTapConfirmed(SHIFT_CLICK);
        mSelection.assertSelection(7, 8, 9, 10, 11);
    }

    @Test
    public void testUnconfirmedShiftClick_RotatesAroundOrigin() {
        mDetailsLookup.initAt(7).setInItemSelectRegion(true);
        mInputDelegate.onSingleTapConfirmed(CLICK);

        mDetailsLookup.initAt(11);
        mInputDelegate.onSingleTapUp(SHIFT_CLICK);
        mSelection.assertSelection(7, 8, 9, 10, 11);

        mDetailsLookup.initAt(5);
        mInputDelegate.onSingleTapUp(SHIFT_CLICK);

        mSelection.assertSelection(5, 6, 7);
        mSelection.assertNotSelected(8, 9, 10, 11);
    }

    @Test
    public void testUnconfirmedShiftCtrlClick_Combination() {
        mDetailsLookup.initAt(7).setInItemSelectRegion(true);
        mInputDelegate.onSingleTapConfirmed(CLICK);

        mDetailsLookup.initAt(11);
        mInputDelegate.onSingleTapUp(SHIFT_CLICK);
        mSelection.assertSelection(7, 8, 9, 10, 11);

        mDetailsLookup.initAt(5);
        mInputDelegate.onSingleTapUp(CTRL_CLICK);

        mSelection.assertSelection(5, 7, 8, 9, 10, 11);
    }

    @Test
    public void testUnconfirmedShiftCtrlClick_ShiftTakesPriority() {
        mDetailsLookup.initAt(7).setInItemSelectRegion(true);
        mInputDelegate.onSingleTapConfirmed(CLICK);

        mDetailsLookup.initAt(11);
        mInputDelegate.onSingleTapUp(mEvent.ctrl().shift().build());

        mSelection.assertSelection(7, 8, 9, 10, 11);
    }

    // TODO: Add testSpaceBar_Previews, but we need to set a system property
    // to have a deterministic state.

    @Test
    public void testDoubleClick_Opens() {
        TestItemDetails doc = mDetailsLookup.initAt(11);
        mInputDelegate.onDoubleTap(CLICK);

        mCallbacks.assertActivated(doc);
    }

    @Test
    public void testMiddleClick_DoesNothing() {
        mDetailsLookup.initAt(11).setInItemSelectRegion(true);
        mInputDelegate.onSingleTapConfirmed(TERTIARY_CLICK);

        mSelection.assertNoSelection();
    }

    @Test
    public void testClickOff_ClearsSelection() {
        mDetailsLookup.initAt(11).setInItemSelectRegion(true);
        mInputDelegate.onSingleTapConfirmed(CLICK);

        mDetailsLookup.initAt(RecyclerView.NO_POSITION);
        mInputDelegate.onSingleTapUp(CLICK);

        mSelection.assertNoSelection();
    }

    @Test
    public void testClick_Focuses() {
        mDetailsLookup.initAt(11).setInItemSelectRegion(false);
        mInputDelegate.onSingleTapConfirmed(CLICK);

        mCallbacks.assertHasFocus(true);
        mCallbacks.assertFocused("11");
    }

    @Test
    public void testClickOff_ClearsFocus() {
        mDetailsLookup.initAt(11).setInItemSelectRegion(false);
        mInputDelegate.onSingleTapConfirmed(CLICK);
        mCallbacks.assertHasFocus(true);

        mDetailsLookup.initAt(RecyclerView.NO_POSITION);
        mInputDelegate.onSingleTapUp(CLICK);
        mCallbacks.assertHasFocus(false);
    }

    @Test
    public void testClickOffSelection_RemovesSelectionAndFocuses() {
        mDetailsLookup.initAt(1).setInItemSelectRegion(true);
        mInputDelegate.onSingleTapConfirmed(CLICK);

        mDetailsLookup.initAt(5);
        mInputDelegate.onSingleTapUp(SHIFT_CLICK);

        mSelection.assertSelection(1, 2, 3, 4, 5);

        mDetailsLookup.initAt(11);
        mInputDelegate.onSingleTapUp(CLICK);

        mCallbacks.assertFocused("11");
        mSelection.assertNoSelection();
    }
}
