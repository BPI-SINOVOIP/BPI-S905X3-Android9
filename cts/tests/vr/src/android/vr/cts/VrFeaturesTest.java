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
package android.vr.cts;

import android.content.Context;
import android.content.pm.PackageManager;
import android.os.PowerManager;
import android.os.Process;
import android.test.ActivityInstrumentationTestCase2;

public class VrFeaturesTest extends ActivityInstrumentationTestCase2<CtsActivity> {
    private CtsActivity mActivity;

    public VrFeaturesTest() {
        super(CtsActivity.class);
    }

    public void testLacksDeprecatedVrModeFeature() {
        mActivity = getActivity();
        boolean hasVrMode = mActivity.getPackageManager().hasSystemFeature(
                PackageManager.FEATURE_VR_MODE);
        boolean hasVrModeHighPerf = mActivity.getPackageManager().hasSystemFeature(
                PackageManager.FEATURE_VR_MODE_HIGH_PERFORMANCE);
        if (hasVrMode || hasVrModeHighPerf) {
            assertTrue("FEATURE_VR_MODE and FEATURE_VR_MODE_HIGH_PERFORMANCE must be used together",
                    hasVrMode && hasVrModeHighPerf);
        }
    }

    public void testSustainedPerformanceModeSupported() {
        mActivity = getActivity();
        PowerManager pm = (PowerManager) mActivity.getSystemService(Context.POWER_SERVICE);
        boolean hasVrModeHighPerf = mActivity.getPackageManager().hasSystemFeature(
                PackageManager.FEATURE_VR_MODE_HIGH_PERFORMANCE);
        boolean hasSustainedPerformanceMode = pm.isSustainedPerformanceModeSupported();
        if (hasVrModeHighPerf) {
            assertTrue("FEATURE_VR_MODE_HIGH_PERFORMANCE requires support for sustained performance mode",
                    hasSustainedPerformanceMode);
        }
    }
}
