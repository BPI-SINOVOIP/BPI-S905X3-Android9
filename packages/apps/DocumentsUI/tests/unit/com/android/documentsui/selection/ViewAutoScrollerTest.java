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

import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;

import android.graphics.Point;
import android.support.test.filters.SmallTest;
import android.support.test.runner.AndroidJUnit4;

import com.android.documentsui.selection.ViewAutoScroller.ScrollHost;
import com.android.documentsui.selection.ViewAutoScroller.ScrollerCallbacks;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;

import java.util.function.IntConsumer;

@RunWith(AndroidJUnit4.class)
@SmallTest
public final class ViewAutoScrollerTest {

    private static final int VIEW_HEIGHT = 100;
    private static final int TOP_Y_POINT = (int) (VIEW_HEIGHT
            * ViewAutoScroller.TOP_BOTTOM_THRESHOLD_RATIO) - 1;
    private static final int BOTTOM_Y_POINT = VIEW_HEIGHT
            - (int) (VIEW_HEIGHT * ViewAutoScroller.TOP_BOTTOM_THRESHOLD_RATIO)
            + 1;
    private static final int EDGE_HEIGHT = 5;

    private ViewAutoScroller mAutoScroller;
    private Point mPoint;
    private boolean mActive;
    private ScrollHost mHost;
    private ScrollerCallbacks mCallbacks;
    private IntConsumer mScrollAssert;

    @Before
    public void setUp() {
        mActive = false;
        mPoint = new Point();

        mHost = new ScrollHost() {
            @Override
            public boolean isActive() {
                return mActive;
            }

            @Override
            public int getViewHeight() {
                return VIEW_HEIGHT;
            }

            @Override
            public Point getCurrentPosition() {
                return mPoint;
            }
        };

        mCallbacks = new ScrollerCallbacks() {
            @Override
            public void scrollBy(int dy) {
                mScrollAssert.accept(dy);
            }

            @Override
            public void runAtNextFrame(Runnable r) {
            }

            @Override
            public void removeCallback(Runnable r) {
            }
        };

        mAutoScroller = new ViewAutoScroller(mHost, mCallbacks);
    }

    @Test
    public void testCursorNotInScrollZone() {
        mPoint = new Point(0, VIEW_HEIGHT/2);
        mScrollAssert = (int dy) -> {
            // Should not have called this method
            fail("Received unexpected scroll event");
            assertTrue(false);
        };
        mAutoScroller.run();
    }

    @Test
    public void testCursorInScrollZone_notActive() {
        mActive = false;
        mPoint = new Point(0, TOP_Y_POINT);
        mScrollAssert = (int dy) -> {
            // Should not have called this method
            fail("Received unexpected scroll event");
            assertTrue(false);
        };
        mAutoScroller.run();
    }

    @Test
    public void testCursorInScrollZone_top() {
        mActive = true;
        mPoint = new Point(0, TOP_Y_POINT);
        int expectedScrollDistance = mAutoScroller.computeScrollDistance(-1);
        mScrollAssert = (int dy) -> {
            assertTrue(dy == expectedScrollDistance);
        };
        mAutoScroller.run();
    }

    @Test
    public void testCursorInScrollZone_bottom() {
        mActive = true;
        mPoint = new Point(0, BOTTOM_Y_POINT);
        int expectedScrollDistance = mAutoScroller.computeScrollDistance(1);
        mScrollAssert = (int dy) -> {
            assertTrue(dy == expectedScrollDistance);
        };
        mAutoScroller.run();
    }
}
