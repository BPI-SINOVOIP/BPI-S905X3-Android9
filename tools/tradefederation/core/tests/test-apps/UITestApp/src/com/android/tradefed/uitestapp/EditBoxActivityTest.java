/*
 * Copyright (C) 2010 The Android Open Source Project
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
package com.android.tradefed.uitestapp;

import android.test.ActivityInstrumentationTestCase2;
import android.test.TouchUtils;
import android.test.UiThreadTest;
import android.widget.EditText;

/**
 * A test that does simple UI events.
 * <p/>
 * Its intended to verify that the UI on device is in a state where UI tests will be successful (ie
 * screen is free from other dialogs, keyguard, etc)
 */
public class EditBoxActivityTest extends ActivityInstrumentationTestCase2<EditBoxActivity> {

    public EditBoxActivityTest() {
        super(EditBoxActivity.class);
    }

    /**
     * Test that the edit box on {@link EditBoxActivity} can focused.
     */
    @UiThreadTest
    public void testEditTextFocus() {
        EditText editText = (EditText)getActivity().findViewById(R.id.EditText);
        assertNotNull(editText);
        assertTrue(editText.requestFocus());
        assertTrue(editText.hasFocus());
    }

    /**
     * Test that the edit box on {@link EditBoxActivity} can modified.
     */
    @UiThreadTest
    public void testEditTextModified() {
        EditText editText = (EditText)getActivity().findViewById(R.id.EditText);
        assertNotNull(editText);
        final String expectedText = "foo2";
        editText.setText(expectedText);
        String actualText = editText.getText().toString();
        assertEquals(expectedText, actualText);
    }

    /**
     * Test that a instrumentation string sync can be sent
     */
    public void testInstrumentationStringSync() {
        EditText editText = (EditText)getActivity().findViewById(R.id.EditText);
        assertNotNull(editText);
        assertTrue(editText.requestFocus());
        getInstrumentation().sendStringSync("foo");
    }

    /**
     * Test that a drag ui event can be sent
     */
    public void testDragEvent() {
        TouchUtils.dragQuarterScreenDown(this, getActivity());
    }

}
