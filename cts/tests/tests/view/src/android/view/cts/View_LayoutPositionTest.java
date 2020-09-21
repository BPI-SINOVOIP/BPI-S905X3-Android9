/*
 * Copyright (C) 2008 The Android Open Source Project
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

package android.view.cts;

import static org.junit.Assert.assertEquals;

import android.app.Activity;
import android.graphics.Rect;
import android.support.test.annotation.UiThreadTest;
import android.support.test.filters.MediumTest;
import android.support.test.rule.ActivityTestRule;
import android.support.test.runner.AndroidJUnit4;
import android.view.View;

import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;

/**
 * For the view test is too big, we divide the test cases into several parts.
 * This part contains size, padding, margin, layout and drawing
 */
@MediumTest
@RunWith(AndroidJUnit4.class)
public class View_LayoutPositionTest {
    private Activity mActivity;

    @Rule
    public ActivityTestRule<ViewLayoutPositionTestCtsActivity> mActivityRule =
            new ActivityTestRule<>(ViewLayoutPositionTestCtsActivity.class);

    @Before
    public void setup() {
        mActivity = mActivityRule.getActivity();
    }

    @UiThreadTest
    @Test
    public void testPositionInParent() {
        View parent = mActivity.findViewById(R.id.testparent);
        View view = mActivity.findViewById(R.id.testview);
        int [] pLocation = new int[2];
        int [] vLocation = new int[2];
        Rect pRect = new Rect();
        Rect vRect = new Rect();

        // baseline not support in view
        assertEquals(-1, view.getBaseline());

        parent.getLocationOnScreen(pLocation);
        view.getLocationOnScreen(vLocation);

        parent.getDrawingRect(pRect);
        view.getDrawingRect(vRect);

        int left = vLocation[0] - pLocation[0];
        int top = vLocation[1] - pLocation[1];
        int right = left + vRect.width();
        int bottom = top + vRect.height();

        assertEquals(left, view.getLeft());
        assertEquals(top, view.getTop());
        assertEquals(right, view.getRight());
        assertEquals(bottom, view.getBottom());

        assertEquals(vRect.width(), view.getWidth());
        assertEquals(vRect.height(), view.getHeight());

        assertEquals(vRect.width(), view.getMeasuredWidth());
        assertEquals(vRect.height(), view.getMeasuredHeight());

        int v_offset = 10;
        int h_offset = 20;
        view.offsetTopAndBottom(v_offset);
        view.offsetLeftAndRight(h_offset);

        // update the position
        parent.getLocationOnScreen(pLocation);
        view.getLocationOnScreen(vLocation);
        parent.getDrawingRect(pRect);
        view.getDrawingRect(vRect);

        int nleft = vLocation[0] - pLocation[0];
        int ntop = vLocation[1] - pLocation[1];
        int nright = nleft + vRect.width();
        int nbottom = ntop + vRect.height();

        assertEquals(left + h_offset , nleft);
        assertEquals(top + v_offset, ntop);
        assertEquals(right + h_offset, nright);
        assertEquals(bottom + v_offset, nbottom);
    }

    @Test
    public void testPadding() {
        View view = new View(mActivity);

        // default is 0
        assertEquals(0, view.getPaddingLeft());
        assertEquals(0, view.getPaddingTop());
        assertEquals(0, view.getPaddingRight());
        assertEquals(0, view.getPaddingBottom());

        view.setPadding(-10, 0, 5, 1000);
        assertEquals(-10, view.getPaddingLeft());
        assertEquals(0, view.getPaddingTop());
        assertEquals(5, view.getPaddingRight());
        assertEquals(1000, view.getPaddingBottom());
    }
}
