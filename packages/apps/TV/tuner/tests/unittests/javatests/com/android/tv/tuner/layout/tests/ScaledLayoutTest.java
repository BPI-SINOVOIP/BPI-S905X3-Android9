/*
 * Copyright (C) 2015 The Android Open Source Project
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

package com.android.tv.tuner.layout.tests;

import static junit.framework.Assert.assertEquals;
import static junit.framework.Assert.assertNotNull;

import android.content.Intent;
import android.support.test.filters.SmallTest;
import android.support.test.rule.ActivityTestRule;
import android.support.test.runner.AndroidJUnit4;
import android.view.View;
import android.widget.FrameLayout;
import com.android.tv.tuner.layout.ScaledLayout;
import org.junit.After;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;

@SmallTest
@RunWith(AndroidJUnit4.class)
public class ScaledLayoutTest {

    @Rule public ScaledActivityTestRule mActivityRule = new ScaledActivityTestRule();

    @Before
    public void setup() {
        mActivityRule.launchActivity(new Intent());
    }

    @After
    public void tearDown() {
        mActivityRule.finishActivity();
    }

    @Test
    public void testScaledLayout_layoutInXml() {
        ScaledLayoutActivity activity = mActivityRule.getActivity();
        assertNotNull(activity);
        FrameLayout rootLayout = mActivityRule.getActivity().findViewById(R.id.root_layout);
        assertNotNull(rootLayout);
        ScaledLayout scaledLayout = (ScaledLayout) rootLayout.getChildAt(0);
        assertNotNull(scaledLayout);
        View view1 = scaledLayout.findViewById(R.id.view1);
        assertNotNull(view1);
        View view2 = scaledLayout.findViewById(R.id.view2);
        assertNotNull(view2);
        View view3 = scaledLayout.findViewById(R.id.view3);
        assertNotNull(view3);
        View view4 = scaledLayout.findViewById(R.id.view4);
        assertNotNull(view4);
        assertEquals((int) (400 * 0.1), view1.getWidth());
        assertEquals((int) (300 * 0.2), view1.getHeight());
        assertEquals((int) (400 * 0.8), view1.getLeft());
        assertEquals((int) (300 * 0.1), view1.getTop());
        assertEquals((int) (400 * 0.1), view2.getWidth());
        assertEquals(300, view2.getHeight());
        assertEquals((int) (400 * 0.2), view2.getLeft());
        assertEquals(0, view2.getTop());
        assertEquals((int) (400 * 0.2), view3.getWidth());
        assertEquals((int) (300 * 0.1), view3.getHeight());
        assertEquals((int) (400 * 0.3), view3.getLeft());
        assertEquals((int) (300 * 0.4), view3.getTop());
        assertEquals((int) (400 * 0.1), view4.getWidth());
        assertEquals((int) (300 * 0.8), view4.getHeight());
        assertEquals((int) (400 * 0.05), view4.getLeft());
        assertEquals((int) (300 * 0.15), view4.getTop());
    }

    @Test
    public void testScaledLayout_layoutThroughCode() {
        ScaledLayoutActivity activity = mActivityRule.getActivity();
        assertNotNull(activity);
        FrameLayout rootLayout = mActivityRule.getActivity().findViewById(R.id.root_layout);
        assertNotNull(rootLayout);
        ScaledLayout scaledLayout = (ScaledLayout) rootLayout.getChildAt(1);
        assertNotNull(scaledLayout);
        View view1 = scaledLayout.findViewById(R.id.view1);
        assertNotNull(view1);
        View view2 = scaledLayout.findViewById(R.id.view2);
        assertNotNull(view2);
        View view3 = scaledLayout.findViewById(R.id.view3);
        assertNotNull(view3);
        View view4 = scaledLayout.findViewById(R.id.view4);
        assertNotNull(view4);
        assertEquals(50, view1.getWidth());
        assertEquals(50, view1.getHeight());
        assertEquals(0, view1.getLeft());
        assertEquals(0, view1.getTop());
        assertEquals(50, view2.getWidth());
        assertEquals(50, view2.getHeight());
        assertEquals(50, view2.getLeft());
        assertEquals(0, view2.getTop());
        assertEquals(50, view3.getWidth());
        assertEquals(50, view3.getHeight());
        assertEquals(0, view3.getLeft());
        assertEquals(50, view3.getTop());
        assertEquals(50, view4.getWidth());
        assertEquals(50, view4.getHeight());
        assertEquals(50, view4.getLeft());
        assertEquals(50, view4.getTop());
    }

    @Test
    public void testScaledLayout_bounceY() {
        ScaledLayoutActivity activity = mActivityRule.getActivity();
        assertNotNull(activity);
        FrameLayout rootLayout = mActivityRule.getActivity().findViewById(R.id.root_layout);
        assertNotNull(rootLayout);
        ScaledLayout scaledLayout = (ScaledLayout) rootLayout.getChildAt(2);
        assertNotNull(scaledLayout);
        View view1 = scaledLayout.findViewById(R.id.view1);
        assertNotNull(view1);
        View view2 = scaledLayout.findViewById(R.id.view2);
        assertNotNull(view2);
        assertEquals(100, view1.getWidth());
        assertEquals(20, view1.getHeight());
        assertEquals(0, view1.getLeft());
        assertEquals(60, view1.getTop());
        assertEquals(100, view2.getWidth());
        assertEquals(20, view2.getHeight());
        assertEquals(0, view2.getLeft());
        assertEquals(80, view2.getTop());
    }

    private static class ScaledActivityTestRule extends ActivityTestRule<ScaledLayoutActivity> {

        public ScaledActivityTestRule() {
            super(ScaledLayoutActivity.class);
        }
    }
}
