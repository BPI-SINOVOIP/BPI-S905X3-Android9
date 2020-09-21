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

package com.android.settings.accessibility;

import android.os.Bundle;

import com.android.internal.logging.MetricsLogger;
import com.android.internal.logging.nano.MetricsProto.MetricsEvent;

public class ToggleScreenMagnificationPreferenceFragmentForSetupWizard
        extends ToggleScreenMagnificationPreferenceFragment {

    @Override
    public int getMetricsCategory() {
        return MetricsEvent.SUW_ACCESSIBILITY_TOGGLE_SCREEN_MAGNIFICATION;
    }

    @Override
    public void onStop() {
        // Log the final choice in value if it's different from the previous value.
        Bundle args = getArguments();
        if ((args != null) && args.containsKey(AccessibilitySettings.EXTRA_CHECKED)) {
            if (mToggleSwitch.isChecked() != args.getBoolean(AccessibilitySettings.EXTRA_CHECKED)) {
                // TODO: Distinguish between magnification modes
                mMetricsFeatureProvider.action(getContext(),
                        MetricsEvent.SUW_ACCESSIBILITY_TOGGLE_SCREEN_MAGNIFICATION,
                        mToggleSwitch.isChecked());
            }
        }
        super.onStop();
    }
}
