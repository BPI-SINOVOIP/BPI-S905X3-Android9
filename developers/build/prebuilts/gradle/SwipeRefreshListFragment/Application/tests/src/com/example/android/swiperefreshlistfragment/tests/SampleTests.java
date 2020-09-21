/*
 * Copyright 2014 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.example.android.swiperefreshlistfragment.tests;

import com.example.android.swiperefreshlistfragment.MainActivity;
import com.example.android.swiperefreshlistfragment.R;
import com.example.android.swiperefreshlistfragment.SwipeRefreshListFragmentFragment;

import android.support.v4.widget.SwipeRefreshLayout;
import android.test.ActivityInstrumentationTestCase2;
import android.test.TouchUtils;
import android.view.Gravity;

/**
 * Tests for SwipeRefreshListFragment sample.
 */
public class SampleTests extends ActivityInstrumentationTestCase2<MainActivity> {

    private MainActivity mTestActivity;
    private SwipeRefreshListFragmentFragment mTestFragment;

    private SwipeRefreshLayout mSwipeRefreshLayout;

    public SampleTests() {
        super(MainActivity.class);
    }

    @Override
    protected void setUp() throws Exception {
        super.setUp();

        // Starts the activity under test using the default Intent with:
        // action = {@link Intent#ACTION_MAIN}
        // flags = {@link Intent#FLAG_ACTIVITY_NEW_TASK}
        // All other fields are null or empty.
        mTestActivity = getActivity();
        mTestFragment = (SwipeRefreshListFragmentFragment)
                mTestActivity.getSupportFragmentManager().getFragments().get(1);
        mSwipeRefreshLayout = mTestFragment.getSwipeRefreshLayout();
    }

    /**
     * Test if the test fixture has been set up correctly.
     */
    public void testPreconditions() {
        //Try to add a message to add context to your assertions. These messages will be shown if
        //a tests fails and make it easy to understand why a test failed
        assertNotNull("mTestActivity is null", mTestActivity);
        assertNotNull("mTestFragment is null", mTestFragment);
        assertNotNull("mSwipeRefreshLayout is null", mSwipeRefreshLayout);
    }

    /**
     * Test that swiping on the populated list triggers a refresh.
     */
    public void testSwipingListView() {
        // Given a SwipeRefreshLayout which is displaying a populated list

        // When the swipe refresh layout is dragged
        TouchUtils.dragViewBy(this,
                mSwipeRefreshLayout,
                Gravity.TOP | Gravity.CENTER_HORIZONTAL,
                0,
                Math.round(mSwipeRefreshLayout.getHeight() * 0.4f));

        // Then the SwipeRefreshLayout should be refreshing
        // We need to use runOnMainSync here as fake dragging uses waitForIdleSync()
        getInstrumentation().runOnMainSync(new Runnable() {
            @Override
            public void run() {
                assertTrue(mSwipeRefreshLayout.isRefreshing());
            }
        });
    }

    /**
     * Test that selecting the refresh menu item triggers a refresh.
     */
    public void testRefreshMenuItem() {
        // When the refresh menu item is selected
        getInstrumentation().invokeMenuActionSync(mTestActivity, R.id.menu_refresh, 0);

        // Then the SwipeRefreshLayout should be refreshing
        assertTrue(mSwipeRefreshLayout.isRefreshing());
    }

}