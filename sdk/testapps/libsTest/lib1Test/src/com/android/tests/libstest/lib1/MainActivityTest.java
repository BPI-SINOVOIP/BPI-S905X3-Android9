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

package com.android.tests.libstest.lib1;

import android.test.ActivityInstrumentationTestCase2;
import android.test.suitebuilder.annotation.MediumTest;
import android.widget.TextView;

/**
 * An example of an {@link ActivityInstrumentationTestCase2} of a specific activity {@link Focus2}.
 * By virtue of extending {@link ActivityInstrumentationTestCase2}, the target activity is automatically
 * launched and finished before and after each test.  This also extends
 * {@link android.test.InstrumentationTestCase}, which provides
 * access to methods for sending events to the target activity, such as key and
 * touch events.  See {@link #sendKeys}.
 *
 * In general, {@link android.test.InstrumentationTestCase}s and {@link ActivityInstrumentationTestCase2}s
 * are heavier weight functional tests available for end to end testing of your
 * user interface.  When run via a {@link android.test.InstrumentationTestRunner},
 * the necessary {@link android.app.Instrumentation} will be injected for you to
 * user via {@link #getInstrumentation} in your tests.
 *
 * See {@link com.example.android.apis.AllTests} for documentation on running
 * all tests and individual tests in this application.
 */
public class MainActivityTest extends ActivityInstrumentationTestCase2<MainActivity> {

    private TextView mLib1TextView1;
    private TextView mLib1TextView2;
    private TextView mLib2TextView1;
    private TextView mLib2TextView2;

    /**
     * Creates an {@link ActivityInstrumentationTestCase2} that tests the {@link Focus2} activity.
     */
    public MainActivityTest() {
        super(MainActivity.class);
    }

    @Override
    protected void setUp() throws Exception {
        super.setUp();
        final MainActivity a = getActivity();
        // ensure a valid handle to the activity has been returned
        assertNotNull(a);
        
        mLib1TextView1 = (TextView) a.findViewById(R.id.lib1_text1);
        mLib1TextView2 = (TextView) a.findViewById(R.id.lib1_text2);
        mLib2TextView1 = (TextView) a.findViewById(R.id.lib2_text1);
        mLib2TextView2 = (TextView) a.findViewById(R.id.lib2_text2);
    }

    /**
     * The name 'test preconditions' is a convention to signal that if this
     * test doesn't pass, the test case was not set up properly and it might
     * explain any and all failures in other tests.  This is not guaranteed
     * to run before other tests, as junit uses reflection to find the tests.
     */
    @MediumTest
    public void testPreconditions() {
        assertNotNull(mLib1TextView1);
        assertNotNull(mLib1TextView2);
        assertNotNull(mLib2TextView1);
        assertNotNull(mLib2TextView2);
    }

    @MediumTest
    public void testAndroidStrings() {
        assertEquals("SUCCESS-LIB1", mLib1TextView1.getText());
        assertEquals("SUCCESS-LIB2", mLib2TextView1.getText());
    }

    @MediumTest
    public void testJavaStrings() {
        assertEquals("SUCCESS-LIB1", mLib1TextView2.getText());
        assertEquals("SUCCESS-LIB2", mLib2TextView2.getText());
    }
}
