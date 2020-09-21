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
package com.android.tradefed.util;

import org.junit.Assert;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

import java.util.ArrayList;
import java.util.List;

/** Unit tests for {@link KeyguardControllerState} */
@RunWith(JUnit4.class)
public class KeyguardControllerStateTest {

    /** Test that {@link KeyguardControllerState#create(List)} returns null if input is invalid. */
    @Test
    public void testCreate_invalidOutput() {
        List<String> testOutput = new ArrayList<String>();
        testOutput.add("mSleepTimeout=false");
        testOutput.add("  mCurTaskIdForUser={0=2}");
        testOutput.add("  mUserStackInFront={}");
        KeyguardControllerState state = KeyguardControllerState.create(testOutput);
        Assert.assertNull(state);
    }

    /**
     * Test that {@link KeyguardControllerState#create(List)} returns proper values based on input.
     */
    @Test
    public void testCreate() {
        List<String> testOutput = new ArrayList<String>();
        testOutput.add("KeyguardController:");
        testOutput.add("  mKeyguardShowing=true");
        testOutput.add("  mKeyguardGoingAway=false");
        testOutput.add("  mOccluded=false");
        KeyguardControllerState state = KeyguardControllerState.create(testOutput);
        Assert.assertTrue(state.isKeyguardShowing());
        Assert.assertFalse(state.isKeyguardOccluded());
    }
}
