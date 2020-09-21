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
 * limitations under the License.
 */

package com.android.documentsui.selection;

import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;

import android.graphics.Point;
import android.graphics.Rect;
import android.support.test.filters.SmallTest;
import android.support.test.runner.AndroidJUnit4;
import android.support.v7.widget.RecyclerView.OnScrollListener;
import android.view.MotionEvent;

import com.android.documentsui.selection.BandSelectionHelper.BandHost;
import com.android.documentsui.selection.testing.SelectionPredicates;
import com.android.documentsui.selection.testing.TestAdapter;
import com.android.documentsui.selection.testing.TestBandPredicate;
import com.android.documentsui.selection.testing.TestEvents.Builder;
import com.android.documentsui.selection.testing.TestStableIdProvider;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;

import java.util.Collections;
import java.util.List;

@RunWith(AndroidJUnit4.class)
@SmallTest
public class BandSelectionHelperTest {

    private List<String> mItems;
    private BandSelectionHelper mBandController;
    private boolean mIsActive;
    private Builder mStartBuilder;
    private Builder mStopBuilder;
    private MotionEvent mStartEvent;
    private MotionEvent mStopEvent;
    private TestBandHost mBandHost;
    private TestBandPredicate mBandPredicate;

    @Before
    public void setup() throws Exception {
        mItems = TestAdapter.createItemList(10);
        mIsActive = false;
        TestAdapter adapter = new TestAdapter();
        adapter.updateTestModelIds(mItems);
        mBandHost = new TestBandHost();
        mBandPredicate = new TestBandPredicate();

        SelectionHelper helper = new DefaultSelectionHelper(
                DefaultSelectionHelper.MODE_SINGLE,
                adapter,
                new TestStableIdProvider(adapter),
                SelectionPredicates.CAN_SET_ANYTHING);

        mBandController = new BandSelectionHelper(
                mBandHost,
                adapter,
                new TestStableIdProvider(adapter),
                helper,
                SelectionPredicates.CAN_SET_ANYTHING,
                mBandPredicate,
                new ContentLock()) {
                    @Override
                    public boolean isActive() {
                        return mIsActive;
                    }
                };

        mStartBuilder = new Builder().mouse().primary().action(MotionEvent.ACTION_MOVE);
        mStopBuilder = new Builder().mouse().action(MotionEvent.ACTION_UP);
        mStartEvent = mStartBuilder.build();
        mStopEvent = mStopBuilder.build();
    }

    @Test
    public void testGoodStart() {
        assertTrue(mBandController.shouldStart(mStartEvent));
    }

    @Test
    public void testBadStart_NoButtons() {
        assertFalse(mBandController.shouldStart(
                mStartBuilder.releaseButton(MotionEvent.BUTTON_PRIMARY).build()));
    }

    @Test
    public void testBadStart_SecondaryButton() {
        assertFalse(
                mBandController.shouldStart(mStartBuilder.secondary().build()));
    }

    @Test
    public void testBadStart_TertiaryButton() {
        assertFalse(
                mBandController.shouldStart(mStartBuilder.tertiary().build()));
    }

    @Test
    public void testBadStart_Touch() {
        assertFalse(mBandController.shouldStart(
                mStartBuilder.touch().releaseButton(MotionEvent.BUTTON_PRIMARY).build()));
    }

    @Test
    public void testBadStart_RespectsCanInitiateBand() {
        mBandPredicate.setCanInitiate(false);
        assertFalse(mBandController.shouldStart(mStartEvent));
    }

    @Test
    public void testBadStart_ActionDown() {
        assertFalse(mBandController
                .shouldStart(mStartBuilder.action(MotionEvent.ACTION_DOWN).build()));
    }

    @Test
    public void testBadStart_ActionUp() {
        assertFalse(mBandController
                .shouldStart(mStartBuilder.action(MotionEvent.ACTION_UP).build()));
    }

    @Test
    public void testBadStart_ActionPointerDown() {
        assertFalse(mBandController.shouldStart(
                mStartBuilder.action(MotionEvent.ACTION_POINTER_DOWN).build()));
    }

    @Test
    public void testBadStart_ActionPointerUp() {
        assertFalse(mBandController.shouldStart(
                mStartBuilder.action(MotionEvent.ACTION_POINTER_UP).build()));
    }

    @Test
    public void testBadStart_NoItems() {
        TestAdapter emptyAdapter = new TestAdapter();
        emptyAdapter.updateTestModelIds(Collections.EMPTY_LIST);

        SelectionHelper helper = new DefaultSelectionHelper(
                DefaultSelectionHelper.MODE_SINGLE,
                emptyAdapter,
                new TestStableIdProvider(emptyAdapter),
                SelectionPredicates.CAN_SET_ANYTHING);

        mBandController = new BandSelectionHelper(
                new TestBandHost(),
                emptyAdapter,
                new TestStableIdProvider(emptyAdapter),
                helper,
                SelectionPredicates.CAN_SET_ANYTHING,
                mBandPredicate,
                new ContentLock());

        assertFalse(mBandController.shouldStart(mStartEvent));
    }

    @Test
    public void testBadStart_alreadyActive() {
        mIsActive = true;
        assertFalse(mBandController.shouldStart(mStartEvent));
    }

    @Test
    public void testGoodStop() {
        mIsActive = true;
        assertTrue(mBandController.shouldStop(mStopEvent));
    }

    @Test
    public void testGoodStop_PointerUp() {
        mIsActive = true;
        assertTrue(mBandController
                .shouldStop(mStopBuilder.action(MotionEvent.ACTION_POINTER_UP).build()));
    }

    @Test
    public void testGoodStop_Cancel() {
        mIsActive = true;
        assertTrue(mBandController
                .shouldStop(mStopBuilder.action(MotionEvent.ACTION_CANCEL).build()));
    }

    @Test
    public void testBadStop_NotActive() {
        assertFalse(mBandController.shouldStop(mStopEvent));
    }

    @Test
    public void testBadStop_NonMouse() {
        mIsActive = true;
        assertFalse(mBandController.shouldStop(mStopBuilder.touch().build()));
    }

    @Test
    public void testBadStop_Move() {
        mIsActive = true;
        assertFalse(mBandController.shouldStop(
                mStopBuilder.action(MotionEvent.ACTION_MOVE).touch().build()));
    }

    @Test
    public void testBadStop_Down() {
        mIsActive = true;
        assertFalse(mBandController.shouldStop(
                mStopBuilder.action(MotionEvent.ACTION_DOWN).touch().build()));
    }

    private final class TestBandHost extends BandHost {
        @Override
        public void scrollBy(int dy) {
        }

        @Override
        public void runAtNextFrame(Runnable r) {
        }

        @Override
        public void removeCallback(Runnable r) {
        }

        @Override
        public void showBand(Rect rect) {
        }

        @Override
        public void hideBand() {
        }

        @Override
        public void addOnScrollListener(OnScrollListener listener) {
        }

        @Override
        public void removeOnScrollListener(OnScrollListener listener) {
        }

        @Override
        public int getHeight() {
            return 0;
        }

        @Override
        public void invalidateView() {
        }

        @Override
        public Point createAbsolutePoint(Point relativePoint) {
            return null;
        }

        @Override
        public Rect getAbsoluteRectForChildViewAt(int index) {
            return null;
        }

        @Override
        public int getAdapterPositionAt(int index) {
            return 0;
        }

        @Override
        public int getColumnCount() {
            return 0;
        }

        @Override
        public int getChildCount() {
            return 0;
        }

        @Override
        public int getVisibleChildCount() {
            return 0;
        }

        @Override
        public boolean hasView(int adapterPosition) {
            return false;
        }
    }
}
