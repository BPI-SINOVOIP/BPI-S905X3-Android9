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
package com.android.car;

import android.car.Car;
import android.car.media.CarAudioManager;
import android.support.test.filters.MediumTest;
import android.support.test.runner.AndroidJUnit4;
import android.util.Log;

import org.junit.Test;
import org.junit.runner.RunWith;

@RunWith(AndroidJUnit4.class)
@MediumTest
public class CarAudioManagerTest extends MockedCarTestBase {

    private static final String TAG = CarAudioManagerTest.class.getSimpleName();

    private CarAudioManager mCarAudioManager;


    @Override
    public void setUp() throws Exception {
        super.setUp();
        mCarAudioManager = (CarAudioManager) getCar().getCarManager(Car.AUDIO_SERVICE);
    }

    @Test
    public void testGetExternalSources() throws Exception {
        String[] sources = mCarAudioManager.getExternalSources();
        // Log the sources we found.  No other real error check as long as the query doesn't crash
        // unless/until we have a known set of audio devices available.
        for (String s: sources) {
            Log.i(TAG, "  Available ext source: " + s);
        }
    }
}
