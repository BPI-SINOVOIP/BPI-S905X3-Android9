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
package com.example.android.directboot.test;

import com.example.android.directboot.*;

import android.test.ActivityInstrumentationTestCase2;

/**
* Tests for Direct Boot sample.
*/
public class SampleTests extends ActivityInstrumentationTestCase2<MainActivity> {

    private MainActivity mTestActivity;
    private SchedulerFragment mTestFragment;

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
        mTestFragment = (SchedulerFragment)
            mTestActivity.getSupportFragmentManager().getFragments().get(1);
    }

    /**
    * Test if the test fixture has been set up correctly.
    */
    public void testPreconditions() {
        //Try to add a message to add context to your assertions. These messages will be shown if
        //a tests fails and make it easy to understand why a test failed
        assertNotNull("mTestActivity is null", mTestActivity);
        assertNotNull("mTestFragment is null", mTestFragment);
    }
}
