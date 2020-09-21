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

import android.content.Context;
import android.hardware.SensorManager;
import android.support.test.InstrumentationRegistry;
import android.support.test.filters.SmallTest;
import android.support.test.runner.AndroidJUnit4;
import android.view.OrientationListener;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;

/**
 * Test {@link OrientationListener}.
 */
@SmallTest
@RunWith(AndroidJUnit4.class)
public class OrientationListenerTest {
    private Context mContext;

    @Before
    public void setup() {
        mContext = InstrumentationRegistry.getTargetContext();
    }

    @Test
    public void testConstructor() {
        new MockOrientationListener(mContext);

        new MockOrientationListener(mContext, SensorManager.SENSOR_DELAY_UI);
    }

    @Test
    public void testRegisterationOfOrientationListener() {
        // these methods are called to assure that no exception is thrown
        MockOrientationListener listener = new MockOrientationListener(mContext);
        listener.disable();
        listener.enable();
    }

    @Test
    public void testOnAccuracyChanged() {
        // this method is called to assure that no exception is thrown
        new MockOrientationListener(mContext).onAccuracyChanged(SensorManager.SENSOR_ACCELEROMETER,
                SensorManager.SENSOR_STATUS_ACCURACY_HIGH);

        new MockOrientationListener(mContext).onAccuracyChanged(SensorManager.SENSOR_ORIENTATION,
                SensorManager.SENSOR_STATUS_ACCURACY_MEDIUM);
    }

    @Test
    public void testOnSensorChanged() {
        // this method is called to assure that no exception is thrown
        MockOrientationListener listener = new MockOrientationListener(mContext);
        float[] mockData = new float[SensorManager.RAW_DATA_Z + 1];
        mockData[SensorManager.RAW_DATA_X] = 3.0f;
        mockData[SensorManager.RAW_DATA_Y] = 4.0f;
        mockData[SensorManager.RAW_DATA_Z] = 5.0f * 2.0f + 0.1f;
        new MockOrientationListener(mContext).onSensorChanged(SensorManager.SENSOR_ACCELEROMETER,
                mockData);

        mockData[SensorManager.RAW_DATA_X] = 4.0f;
        mockData[SensorManager.RAW_DATA_Y] = 4.0f;
        mockData[SensorManager.RAW_DATA_Z] = 5.0f * 2.0f;
        new MockOrientationListener(mContext).onSensorChanged(SensorManager.SENSOR_MAGNETIC_FIELD,
                mockData);
    }

    @Test
    public void testOnOrientationChanged() {
        MockOrientationListener listener = new MockOrientationListener(mContext);
        listener.enable();
        // TODO can not simulate sensor events on the emulator.
    }

    private class MockOrientationListener extends OrientationListener {
        public MockOrientationListener(Context context) {
            super(context);
        }

        public MockOrientationListener(Context context, int rate) {
            super(context, rate);
        }

        @Override
        public void onOrientationChanged(int orientation) {
        }
    }
}
