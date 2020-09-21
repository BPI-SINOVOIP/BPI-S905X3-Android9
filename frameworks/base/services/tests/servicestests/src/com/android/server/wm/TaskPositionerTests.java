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
 * limitations under the License
 */

package com.android.server.wm;

import org.junit.Before;
import org.junit.Ignore;
import org.junit.Test;
import org.junit.runner.RunWith;

import android.graphics.Rect;
import android.support.test.filters.SmallTest;
import android.support.test.runner.AndroidJUnit4;
import android.util.DisplayMetrics;
import android.util.Log;
import android.view.Display;

import static com.android.server.wm.TaskPositioner.MIN_ASPECT;
import static com.android.server.wm.WindowManagerService.dipToPixel;
import static com.android.server.wm.WindowState.MINIMUM_VISIBLE_HEIGHT_IN_DP;
import static com.android.server.wm.WindowState.MINIMUM_VISIBLE_WIDTH_IN_DP;
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNull;
import static org.junit.Assert.assertTrue;

/**
 * Tests for the {@link TaskPositioner} class.
 *
 * runtest frameworks-services -c com.android.server.wm.TaskPositionerTests
 */
@SmallTest
@RunWith(AndroidJUnit4.class)
public class TaskPositionerTests extends WindowTestsBase {

    private final boolean DEBUGGING = false;
    private final String TAG = "TaskPositionerTest";

    private final static int MOUSE_DELTA_X = 5;
    private final static int MOUSE_DELTA_Y = 5;

    private int mMinVisibleWidth;
    private int mMinVisibleHeight;
    private TaskPositioner mPositioner;

    @Before
    public void setUp() throws Exception {
        super.setUp();

        TaskPositioner.setFactory(null);

        final Display display = mDisplayContent.getDisplay();
        final DisplayMetrics dm = new DisplayMetrics();
        display.getMetrics(dm);

        // This should be the same calculation as the TaskPositioner uses.
        mMinVisibleWidth = dipToPixel(MINIMUM_VISIBLE_WIDTH_IN_DP, dm);
        mMinVisibleHeight = dipToPixel(MINIMUM_VISIBLE_HEIGHT_IN_DP, dm);

        mPositioner = TaskPositioner.create(sWm);
        mPositioner.register(mDisplayContent);
    }

    @Test
    public void testOverrideFactory() throws Exception {
        final boolean[] created = new boolean[1];
        created[0] = false;
        TaskPositioner.setFactory(new TaskPositioner.Factory() {
            @Override
            public TaskPositioner create(WindowManagerService service) {
                created[0] = true;
                return null;
            }
        });

        assertNull(TaskPositioner.create(sWm));
        assertTrue(created[0]);
    }

    /**
     * This tests that free resizing will allow to change the orientation as well
     * as does some basic tests (e.g. dragging in Y only will keep X stable).
     */
    @Test
    @Ignore
    public void testBasicFreeWindowResizing() throws Exception {
        final Rect r = new Rect(100, 220, 700, 520);
        final int midY = (r.top + r.bottom) / 2;

        // Start a drag resize starting upper left.
        mPositioner.startDrag(true /*resizing*/,
                false /*preserveOrientation*/, r.left - MOUSE_DELTA_X, r.top - MOUSE_DELTA_Y, r);
        assertBoundsEquals(r, mPositioner.getWindowDragBounds());

        // Drag to a good landscape size.
        mPositioner.resizeDrag(0.0f, 0.0f);
        assertBoundsEquals(new Rect(MOUSE_DELTA_X, MOUSE_DELTA_Y, r.right, r.bottom),
                mPositioner.getWindowDragBounds());

        // Drag to a good portrait size.
        mPositioner.resizeDrag(400.0f, 0.0f);
        assertBoundsEquals(new Rect(400 + MOUSE_DELTA_X, MOUSE_DELTA_Y, r.right, r.bottom),
                mPositioner.getWindowDragBounds());

        // Drag to a too small size for the width.
        mPositioner.resizeDrag(2000.0f, r.top);
        assertBoundsEquals(
                new Rect(r.right - mMinVisibleWidth, r.top + MOUSE_DELTA_Y, r.right, r.bottom),
                mPositioner.getWindowDragBounds());

        // Drag to a too small size for the height.
        mPositioner.resizeDrag(r.left, 2000.0f);
        assertBoundsEquals(
                new Rect(r.left + MOUSE_DELTA_X, r.bottom - mMinVisibleHeight, r.right, r.bottom),
                mPositioner.getWindowDragBounds());

        // Start a drag resize left and see that only the left coord changes..
        mPositioner.startDrag(true /*resizing*/,
                false /*preserveOrientation*/, r.left - MOUSE_DELTA_X, midY, r);

        // Drag to the left.
        mPositioner.resizeDrag(0.0f, midY);
        assertBoundsEquals(new Rect(MOUSE_DELTA_X, r.top, r.right, r.bottom),
                mPositioner.getWindowDragBounds());

        // Drag to the right.
        mPositioner.resizeDrag(200.0f, midY);
        assertBoundsEquals(new Rect(200 + MOUSE_DELTA_X, r.top, r.right, r.bottom),
                mPositioner.getWindowDragBounds());

        // Drag to the top
        mPositioner.resizeDrag(r.left, 0.0f);
        assertBoundsEquals(new Rect(r.left + MOUSE_DELTA_X, r.top, r.right, r.bottom),
                mPositioner.getWindowDragBounds());

        // Drag to the bottom
        mPositioner.resizeDrag(r.left, 1000.0f);
        assertBoundsEquals(new Rect(r.left + MOUSE_DELTA_X, r.top, r.right, r.bottom),
                mPositioner.getWindowDragBounds());
    }

    /**
     * This tests that by dragging any edge, the fixed / opposite edge(s) remains anchored.
     */
    @Test
    @Ignore
    public void testFreeWindowResizingTestAllEdges() throws Exception {
        final Rect r = new Rect(100, 220, 700, 520);
        final int midX = (r.left + r.right) / 2;
        final int midY = (r.top + r.bottom) / 2;

        // Drag upper left.
        mPositioner.startDrag(true /*resizing*/,
                false /*preserveOrientation*/, r.left - MOUSE_DELTA_X, r.top - MOUSE_DELTA_Y, r);
        mPositioner.resizeDrag(0.0f, 0.0f);
        assertTrue(r.left != mPositioner.getWindowDragBounds().left);
        assertEquals(r.right, mPositioner.getWindowDragBounds().right);
        assertTrue(r.top != mPositioner.getWindowDragBounds().top);
        assertEquals(r.bottom, mPositioner.getWindowDragBounds().bottom);

        // Drag upper.
        mPositioner.startDrag(true /*resizing*/,
                false /*preserveOrientation*/, midX, r.top - MOUSE_DELTA_Y, r);
        mPositioner.resizeDrag(0.0f, 0.0f);
        assertEquals(r.left, mPositioner.getWindowDragBounds().left);
        assertEquals(r.right, mPositioner.getWindowDragBounds().right);
        assertTrue(r.top != mPositioner.getWindowDragBounds().top);
        assertEquals(r.bottom, mPositioner.getWindowDragBounds().bottom);

        // Drag upper right.
        mPositioner.startDrag(true /*resizing*/,
                false /*preserveOrientation*/, r.right + MOUSE_DELTA_X, r.top - MOUSE_DELTA_Y, r);
        mPositioner.resizeDrag(r.right + 100, 0.0f);
        assertEquals(r.left, mPositioner.getWindowDragBounds().left);
        assertTrue(r.right != mPositioner.getWindowDragBounds().right);
        assertTrue(r.top != mPositioner.getWindowDragBounds().top);
        assertEquals(r.bottom, mPositioner.getWindowDragBounds().bottom);

        // Drag right.
        mPositioner.startDrag(true /*resizing*/,
                false /*preserveOrientation*/, r.right + MOUSE_DELTA_X, midY, r);
        mPositioner.resizeDrag(r.right + 100, 0.0f);
        assertEquals(r.left, mPositioner.getWindowDragBounds().left);
        assertTrue(r.right != mPositioner.getWindowDragBounds().right);
        assertEquals(r.top, mPositioner.getWindowDragBounds().top);
        assertEquals(r.bottom, mPositioner.getWindowDragBounds().bottom);

        // Drag bottom right.
        mPositioner.startDrag(true /*resizing*/,
                false /*preserveOrientation*/,
                r.right + MOUSE_DELTA_X, r.bottom + MOUSE_DELTA_Y, r);
        mPositioner.resizeDrag(r.right + 100, r.bottom + 100);
        assertEquals(r.left, mPositioner.getWindowDragBounds().left);
        assertTrue(r.right != mPositioner.getWindowDragBounds().right);
        assertEquals(r.top, mPositioner.getWindowDragBounds().top);
        assertTrue(r.bottom != mPositioner.getWindowDragBounds().bottom);

        // Drag bottom.
        mPositioner.startDrag(true /*resizing*/,
                false /*preserveOrientation*/, midX, r.bottom + MOUSE_DELTA_Y, r);
        mPositioner.resizeDrag(r.right + 100, r.bottom + 100);
        assertEquals(r.left, mPositioner.getWindowDragBounds().left);
        assertEquals(r.right, mPositioner.getWindowDragBounds().right);
        assertEquals(r.top, mPositioner.getWindowDragBounds().top);
        assertTrue(r.bottom != mPositioner.getWindowDragBounds().bottom);

        // Drag bottom left.
        mPositioner.startDrag(true /*resizing*/,
                false /*preserveOrientation*/, r.left - MOUSE_DELTA_X, r.bottom + MOUSE_DELTA_Y, r);
        mPositioner.resizeDrag(0.0f, r.bottom + 100);
        assertTrue(r.left != mPositioner.getWindowDragBounds().left);
        assertEquals(r.right, mPositioner.getWindowDragBounds().right);
        assertEquals(r.top, mPositioner.getWindowDragBounds().top);
        assertTrue(r.bottom != mPositioner.getWindowDragBounds().bottom);

        // Drag left.
        mPositioner.startDrag(true /*resizing*/,
                false /*preserveOrientation*/, r.left - MOUSE_DELTA_X, midX, r);
        mPositioner.resizeDrag(0.0f, r.bottom + 100);
        assertTrue(r.left != mPositioner.getWindowDragBounds().left);
        assertEquals(r.right, mPositioner.getWindowDragBounds().right);
        assertEquals(r.top, mPositioner.getWindowDragBounds().top);
        assertEquals(r.bottom, mPositioner.getWindowDragBounds().bottom);
    }

    /**
     * This tests that a constrained landscape window will keep the aspect and do the
     * right things upon resizing when dragged from the top left corner.
     */
    @Test
    @Ignore
    public void testLandscapePreservedWindowResizingDragTopLeft() throws Exception {
        final Rect r = new Rect(100, 220, 700, 520);

        mPositioner.startDrag(true /*resizing*/,
                true /*preserveOrientation*/, r.left - MOUSE_DELTA_X, r.top - MOUSE_DELTA_Y, r);
        assertBoundsEquals(r, mPositioner.getWindowDragBounds());

        // Drag to a good landscape size.
        mPositioner.resizeDrag(0.0f, 0.0f);
        assertBoundsEquals(new Rect(MOUSE_DELTA_X, MOUSE_DELTA_Y, r.right, r.bottom),
                mPositioner.getWindowDragBounds());

        // Drag to a good portrait size.
        mPositioner.resizeDrag(400.0f, 0.0f);
        int width = Math.round((float) (r.bottom - MOUSE_DELTA_Y) * MIN_ASPECT);
        assertBoundsEquals(new Rect(r.right - width, MOUSE_DELTA_Y, r.right, r.bottom),
                mPositioner.getWindowDragBounds());

        // Drag to a too small size for the width.
        mPositioner.resizeDrag(2000.0f, r.top);
        final int w = mMinVisibleWidth;
        final int h = Math.round(w / MIN_ASPECT);
        assertBoundsEquals(new Rect(r.right - w, r.bottom - h, r.right, r.bottom),
                mPositioner.getWindowDragBounds());

        // Drag to a too small size for the height.
        mPositioner.resizeDrag(r.left, 2000.0f);
        assertBoundsEquals(
                new Rect(r.left + MOUSE_DELTA_X, r.bottom - mMinVisibleHeight, r.right, r.bottom),
                mPositioner.getWindowDragBounds());
    }

    /**
     * This tests that a constrained landscape window will keep the aspect and do the
     * right things upon resizing when dragged from the left corner.
     */
    @Test
    @Ignore
    public void testLandscapePreservedWindowResizingDragLeft() throws Exception {
        final Rect r = new Rect(100, 220, 700, 520);
        final int midY = (r.top + r.bottom) / 2;

        mPositioner.startDrag(true /*resizing*/,
                true /*preserveOrientation*/, r.left - MOUSE_DELTA_X, midY, r);

        // Drag to the left.
        mPositioner.resizeDrag(0.0f, midY);
        assertBoundsEquals(new Rect(MOUSE_DELTA_X, r.top, r.right, r.bottom),
                mPositioner.getWindowDragBounds());

        // Drag to the right.
        mPositioner.resizeDrag(200.0f, midY);
        assertBoundsEquals(new Rect(200 + MOUSE_DELTA_X, r.top, r.right, r.bottom),
                mPositioner.getWindowDragBounds());

        // Drag all the way to the right and see the height also shrinking.
        mPositioner.resizeDrag(2000.0f, midY);
        final int w = mMinVisibleWidth;
        final int h = Math.round((float)w / MIN_ASPECT);
        assertBoundsEquals(new Rect(r.right - w, r.top, r.right, r.top + h),
                mPositioner.getWindowDragBounds());

        // Drag to the top.
        mPositioner.resizeDrag(r.left, 0.0f);
        assertBoundsEquals(new Rect(r.left + MOUSE_DELTA_X, r.top, r.right, r.bottom),
                mPositioner.getWindowDragBounds());

        // Drag to the bottom.
        mPositioner.resizeDrag(r.left, 1000.0f);
        assertBoundsEquals(new Rect(r.left + MOUSE_DELTA_X, r.top, r.right, r.bottom),
                mPositioner.getWindowDragBounds());
    }

    /**
     * This tests that a constrained landscape window will keep the aspect and do the
     * right things upon resizing when dragged from the top corner.
     */
    @Test
    @Ignore
    public void testLandscapePreservedWindowResizingDragTop() throws Exception {
        final Rect r = new Rect(100, 220, 700, 520);
        final int midX = (r.left + r.right) / 2;

        mPositioner.startDrag(true /*resizing*/,
                true /*preserveOrientation*/, midX, r.top - MOUSE_DELTA_Y, r);

        // Drag to the left (no change).
        mPositioner.resizeDrag(0.0f, r.top);
        assertBoundsEquals(new Rect(r.left, r.top + MOUSE_DELTA_Y, r.right, r.bottom),
                mPositioner.getWindowDragBounds());

        // Drag to the right (no change).
        mPositioner.resizeDrag(2000.0f, r.top);
        assertBoundsEquals(new Rect(r.left , r.top + MOUSE_DELTA_Y, r.right, r.bottom),
                mPositioner.getWindowDragBounds());

        // Drag to the top.
        mPositioner.resizeDrag(300.0f, 0.0f);
        int h = r.bottom - MOUSE_DELTA_Y;
        int w = Math.max(r.right - r.left, Math.round(h * MIN_ASPECT));
        assertBoundsEquals(new Rect(r.left, MOUSE_DELTA_Y, r.left + w, r.bottom),
                mPositioner.getWindowDragBounds());

        // Drag to the bottom.
        mPositioner.resizeDrag(r.left, 1000.0f);
        h = mMinVisibleHeight;
        assertBoundsEquals(new Rect(r.left, r.bottom - h, r.right, r.bottom),
                mPositioner.getWindowDragBounds());
    }

    /**
     * This tests that a constrained portrait window will keep the aspect and do the
     * right things upon resizing when dragged from the top left corner.
     */
    @Test
    @Ignore
    public void testPortraitPreservedWindowResizingDragTopLeft() throws Exception {
        final Rect r = new Rect(330, 100, 630, 600);

        mPositioner.startDrag(true /*resizing*/,
                true /*preserveOrientation*/, r.left - MOUSE_DELTA_X, r.top - MOUSE_DELTA_Y, r);
        assertBoundsEquals(r, mPositioner.getWindowDragBounds());

        // Drag to a good landscape size.
        mPositioner.resizeDrag(0.0f, 0.0f);
        int height = Math.round((float) (r.right - MOUSE_DELTA_X) * MIN_ASPECT);
        assertBoundsEquals(new Rect(MOUSE_DELTA_X, r.bottom - height, r.right, r.bottom),
                mPositioner.getWindowDragBounds());

        // Drag to a good portrait size.
        mPositioner.resizeDrag(500.0f, 0.0f);
        assertBoundsEquals(new Rect(500 + MOUSE_DELTA_X, MOUSE_DELTA_Y, r.right, r.bottom),
                mPositioner.getWindowDragBounds());

        // Drag to a too small size for the height and the the width shrinking.
        mPositioner.resizeDrag(r.left + MOUSE_DELTA_X, 2000.0f);
        final int w = Math.max(mMinVisibleWidth, Math.round(mMinVisibleHeight / MIN_ASPECT));
        final int h = Math.max(mMinVisibleHeight, Math.round(w * MIN_ASPECT));
        assertBoundsEquals(
                new Rect(r.right - w, r.bottom - h, r.right, r.bottom),
                mPositioner.getWindowDragBounds());
    }

    /**
     * This tests that a constrained portrait window will keep the aspect and do the
     * right things upon resizing when dragged from the left corner.
     */
    @Test
    @Ignore
    public void testPortraitPreservedWindowResizingDragLeft() throws Exception {
        final Rect r = new Rect(330, 100, 630, 600);
        final int midY = (r.top + r.bottom) / 2;

        mPositioner.startDrag(true /*resizing*/,
                true /*preserveOrientation*/, r.left - MOUSE_DELTA_X, midY, r);

        // Drag to the left.
        mPositioner.resizeDrag(0.0f, midY);
        int w = r.right - MOUSE_DELTA_X;
        int h = Math.round(w * MIN_ASPECT);
        assertBoundsEquals(new Rect(MOUSE_DELTA_X, r.top, r.right, r.top + h),
                mPositioner.getWindowDragBounds());

        // Drag to the right.
        mPositioner.resizeDrag(450.0f, midY);
        assertBoundsEquals(new Rect(450 + MOUSE_DELTA_X, r.top, r.right, r.bottom),
                mPositioner.getWindowDragBounds());

        // Drag all the way to the right.
        mPositioner.resizeDrag(2000.0f, midY);
        w = mMinVisibleWidth;
        h = Math.max(Math.round((float)w * MIN_ASPECT), r.height());
        assertBoundsEquals(new Rect(r.right - w, r.top, r.right, r.top + h),
                mPositioner.getWindowDragBounds());

        // Drag to the top.
        mPositioner.resizeDrag(r.left, 0.0f);
        assertBoundsEquals(new Rect(r.left + MOUSE_DELTA_X, r.top, r.right, r.bottom),
                mPositioner.getWindowDragBounds());

        // Drag to the bottom.
        mPositioner.resizeDrag(r.left, 1000.0f);
        assertBoundsEquals(new Rect(r.left + MOUSE_DELTA_X, r.top, r.right, r.bottom),
                mPositioner.getWindowDragBounds());
    }

    /**
     * This tests that a constrained portrait window will keep the aspect and do the
     * right things upon resizing when dragged from the top corner.
     */
    @Test
    @Ignore
    public void testPortraitPreservedWindowResizingDragTop() throws Exception {
        final Rect r = new Rect(330, 100, 630, 600);
        final int midX = (r.left + r.right) / 2;

        mPositioner.startDrag(true /*resizing*/,
                true /*preserveOrientation*/, midX, r.top - MOUSE_DELTA_Y, r);

        // Drag to the left (no change).
        mPositioner.resizeDrag(0.0f, r.top);
        assertBoundsEquals(new Rect(r.left, r.top + MOUSE_DELTA_Y, r.right, r.bottom),
                mPositioner.getWindowDragBounds());

        // Drag to the right (no change).
        mPositioner.resizeDrag(2000.0f, r.top);
        assertBoundsEquals(new Rect(r.left , r.top + MOUSE_DELTA_Y, r.right, r.bottom),
                mPositioner.getWindowDragBounds());

        // Drag to the top.
        mPositioner.resizeDrag(300.0f, 0.0f);
        int h = r.bottom - MOUSE_DELTA_Y;
        int w = Math.min(r.width(), Math.round(h / MIN_ASPECT));
        assertBoundsEquals(new Rect(r.left, MOUSE_DELTA_Y, r.left + w, r.bottom),
                mPositioner.getWindowDragBounds());

        // Drag to the bottom.
        mPositioner.resizeDrag(r.left, 1000.0f);
        h = Math.max(mMinVisibleHeight, Math.round(mMinVisibleWidth * MIN_ASPECT));
        w = Math.round(h / MIN_ASPECT);
        assertBoundsEquals(new Rect(r.left, r.bottom - h, r.left + w, r.bottom),
                mPositioner.getWindowDragBounds());
    }

    private void assertBoundsEquals(Rect expected, Rect actual) {
        if (DEBUGGING) {
            if (!expected.equals(actual)) {
                Log.e(TAG, "rect(" + actual.toString() + ") != isRect(" + actual.toString()
                        + ") " + Log.getStackTraceString(new Throwable()));
            }
        }
        assertEquals(expected.left, actual.left);
        assertEquals(expected.right, actual.right);
        assertEquals(expected.top, actual.top);
        assertEquals(expected.bottom, actual.bottom);
    }
}
