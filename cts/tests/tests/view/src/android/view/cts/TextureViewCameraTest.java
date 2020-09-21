/*
 * Copyright (C) 2012 The Android Open Source Project
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

import static org.junit.Assert.assertTrue;

import android.hardware.Camera;
import android.support.test.filters.LargeTest;
import android.support.test.rule.ActivityTestRule;
import android.support.test.runner.AndroidJUnit4;

import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;

@LargeTest
@RunWith(AndroidJUnit4.class)
public class TextureViewCameraTest {
    private static final long WAIT_TIMEOUT_IN_SECS = 30;

    private TextureViewCameraActivity mActivity;
    private int mNumberOfCameras;

    @Rule
    public ActivityTestRule<TextureViewCameraActivity> mActivityRule =
            new ActivityTestRule<>(TextureViewCameraActivity.class);

    @Before
    public void setup() {
        mActivity = mActivityRule.getActivity();
        mNumberOfCameras = Camera.getNumberOfCameras();
    }

    @Test
    public void testTextureViewActivity() throws InterruptedException {
        if (mNumberOfCameras < 1) {
            return;
        }
        assertTrue(mActivity.waitForCompletion(WAIT_TIMEOUT_IN_SECS));
    }
}
