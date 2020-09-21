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
package android.longevity.platform;

import static org.mockito.Mockito.any;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import android.app.Instrumentation;
import android.content.Context;
import android.content.Intent;
import android.os.BatteryManager;
import android.os.Bundle;


import org.junit.Before;
import org.junit.Test;
import org.junit.internal.builders.AllDefaultPossibilitiesBuilder;
import org.junit.runner.RunWith;
import org.junit.runner.notification.RunNotifier;
import org.junit.runners.JUnit4;
import org.junit.runners.model.InitializationError;
import org.junit.runners.Suite.SuiteClasses;
import org.mockito.Mockito;

/**
 * Unit tests for the {@link LongevitySuite} runner.
 */
@RunWith(JUnit4.class)
public class LongevitySuiteTest {
    private final Instrumentation mInstrumentation = Mockito.mock(Instrumentation.class);
    private final Context mContext = Mockito.mock(Context.class);

    private final RunNotifier mRunNotifier = Mockito.spy(new RunNotifier());
    private final Intent mBatteryIntent = new Intent();

    private LongevitySuite mSuite;

    @Before
    public void setUpSuite() throws InitializationError {
        // Android context mocking.
        when(mContext.registerReceiver(any(), any())).thenReturn(mBatteryIntent);
        // Create the core suite to test.
        mSuite = new LongevitySuite(TestSuite.class, new AllDefaultPossibilitiesBuilder(true),
                mInstrumentation, mContext, new Bundle());
    }

    /**
     * Tests that devices with batteries terminate on low battery.
     */
    @Test
    public void testDeviceWithBattery_registersReceiver() {
        mBatteryIntent.putExtra(BatteryManager.EXTRA_PRESENT, true);
        mBatteryIntent.putExtra(BatteryManager.EXTRA_LEVEL, 1);
        mBatteryIntent.putExtra(BatteryManager.EXTRA_SCALE, 100);
        mSuite.run(mRunNotifier);
        verify(mRunNotifier).pleaseStop();
    }

    /**
     * Tests that devices without batteries do not terminate on low battery.
     */
    @Test
    public void testDeviceWithoutBattery_doesNotRegisterReceiver() {
        mBatteryIntent.putExtra(BatteryManager.EXTRA_PRESENT, false);
        mBatteryIntent.putExtra(BatteryManager.EXTRA_LEVEL, 1);
        mBatteryIntent.putExtra(BatteryManager.EXTRA_SCALE, 100);
        mSuite.run(mRunNotifier);
        verify(mRunNotifier, never()).pleaseStop();
    }


    @RunWith(LongevitySuite.class)
    @SuiteClasses({
        TestSuite.BasicTest.class,
    })
    /**
     * Sample device-side test cases.
     */
    public static class TestSuite {
        // no local test cases.

        public static class BasicTest {
            @Test
            public void testNothing() { }
        }
    }
}
