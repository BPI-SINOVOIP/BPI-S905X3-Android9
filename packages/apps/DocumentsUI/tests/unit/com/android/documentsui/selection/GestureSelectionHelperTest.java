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

import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;

import android.support.test.filters.SmallTest;
import android.support.test.runner.AndroidJUnit4;
import android.support.v7.widget.RecyclerView;
import android.view.MotionEvent;

import com.android.documentsui.selection.testing.SelectionProbe;
import com.android.documentsui.selection.testing.TestData;
import com.android.documentsui.testing.SelectionHelpers;
import com.android.documentsui.testing.TestEvents;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;

import java.util.List;

@RunWith(AndroidJUnit4.class)
@SmallTest
public class GestureSelectionHelperTest {

    private static final List<String> ITEMS = TestData.create(100);
    private static final MotionEvent DOWN = TestEvents.builder()
            .action(MotionEvent.ACTION_DOWN)
            .location(1, 1)
            .build();

    private static final MotionEvent MOVE = TestEvents.builder()
            .action(MotionEvent.ACTION_MOVE)
            .location(1, 1)
            .build();

    private static final MotionEvent UP = TestEvents.builder()
            .action(MotionEvent.ACTION_UP)
            .location(1, 1)
            .build();

    private GestureSelectionHelper mHelper;
    private SelectionHelper mSelectionHelper;
    private SelectionProbe mSelection;
    private ContentLock mLock;
    private TestItemDetailsLookup mItemLookup;
    private TestViewDelegate mView;

    @Before
    public void setUp() {
        mSelectionHelper = SelectionHelpers.createTestInstance(ITEMS);
        mSelection = new SelectionProbe(mSelectionHelper);
        mLock = new ContentLock();
        mItemLookup = new TestItemDetailsLookup();
        mItemLookup.initAt(3);
        mView = new TestViewDelegate();
        mHelper = new GestureSelectionHelper(mSelectionHelper, mView, mLock, mItemLookup);
    }

    @Test
    public void testIgnoresDown_NoPosition() {
        mView.mNextPosition = RecyclerView.NO_POSITION;
        assertFalse(mHelper.onInterceptTouchEvent(null, DOWN));
    }

    @Test
    public void testIgnoresDown_NoItemDetails() {
        mItemLookup.reset();
        assertFalse(mHelper.onInterceptTouchEvent(null, DOWN));
    }

    @Test
    public void testNoStartOnIllegalPosition() {
        mView.mNextPosition = -1;
        mHelper.onInterceptTouchEvent(null, DOWN);
        mHelper.start();
        assertFalse(mLock.isLocked());
    }

    @Test
    public void testClaimsDownOnItem() {
        mView.mNextPosition = 0;
        assertTrue(mHelper.onInterceptTouchEvent(null, DOWN));
    }

    @Test
    public void testClaimsMoveIfStarted() {
        mView.mNextPosition = 0;
        assertTrue(mHelper.onInterceptTouchEvent(null, DOWN));

        // Normally, this is controller by the TouchSelectionHelper via a a long press gesture.
        mSelectionHelper.select("1");
        mSelectionHelper.anchorRange(1);
        mHelper.start();
        assertTrue(mHelper.onInterceptTouchEvent(null, MOVE));
    }

    @Test
    public void testCreatesRangeSelection() {
        mView.mNextPosition = 1;
        mHelper.onInterceptTouchEvent(null, DOWN);
        // Another way we are implicitly coupled to TouchInputHandler, is that we depend on
        // long press to establish the initial anchor point. Without that we'll get an
        // error when we try to extend the range.

        mSelectionHelper.select("1");
        mSelectionHelper.anchorRange(1);
        mHelper.start();

        mHelper.onTouchEvent(null, MOVE);

        mView.mNextPosition = 9;
        mHelper.onTouchEvent(null, MOVE);
        mHelper.onTouchEvent(null, UP);

        mSelection.assertRangeSelected(1,  9);
    }

    private static final class TestViewDelegate extends GestureSelectionHelper.ViewDelegate {

        private int mNextPosition = RecyclerView.NO_POSITION;

        @Override
        int getHeight() {
            return 1000;
        }

        @Override
        int getItemUnder(MotionEvent e) {
            return mNextPosition;
        }

        @Override
        int getLastGlidedItemPosition(MotionEvent e) {
            return mNextPosition;
        }
    }
}
