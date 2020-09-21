/*
 * Copyright (C) 2018 The Android Open Source Project
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
package android.animation.cts;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;

import android.support.test.InstrumentationRegistry;
import android.support.test.filters.MediumTest;
import android.support.test.rule.ActivityTestRule;
import android.support.test.runner.AndroidJUnit4;
import android.view.animation.AnimationUtils;
import android.view.animation.Interpolator;

import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;

@MediumTest
@RunWith(AndroidJUnit4.class)
public class InterpolatorTest {
    private static final float EPSILON = 0.00001f;
    private AnimationActivity mActivity;

    @Rule
    public ActivityTestRule<AnimationActivity> mActivityRule =
            new ActivityTestRule<>(AnimationActivity.class);

    @Before
    public void setup() {
        InstrumentationRegistry.getInstrumentation().setInTouchMode(false);
        mActivity = mActivityRule.getActivity();
    }

    @Test
    public void testFastOutExtraSlowIn() {
        Interpolator interpolator = AnimationUtils.loadInterpolator(mActivity,
                android.R.interpolator.fast_out_extra_slow_in);
        float turningPointX = 0.166666f;
        float turningPointY = 0.4f;

        // Test that the interpolator starts at (0, 0) and ends at (1, 1)
        assertEquals(0f, interpolator.getInterpolation(0), EPSILON);
        assertEquals(1f, interpolator.getInterpolation(1), EPSILON);

        // Test that when duration is at 1/6 of the total duration, the interpolation is 0.4, per
        // interpolator spec.
        assertEquals(turningPointY, interpolator.getInterpolation(turningPointX), EPSILON);

        // Test that the first segment of the curve (i.e. fraction in (0, 0.166666)) is below the
        // line formed with point (0, 0) and (0.166666, 0.4)
        for (float fraction = EPSILON; fraction < turningPointX; fraction += 0.05f) {
            assertTrue(interpolator.getInterpolation(fraction)
                    < fraction / turningPointX * turningPointY);
        }

        // Test that the second segment of the curve (i.e. fraction in (0.166666, 1)) is above
        // the line formed with point (0.166666, 0.4) and (1, 1)
        for (float fraction = turningPointX + EPSILON; fraction < 1f; fraction += 0.05f) {
            float value = interpolator.getInterpolation(fraction);
            assertTrue((value - turningPointY) / (fraction - turningPointX)
                    > (1f - turningPointY) / (1f - turningPointX));
        }
        // The curve needs to pass the turning point.
        assertEquals(turningPointY, interpolator.getInterpolation(turningPointX), EPSILON);
    }

}
