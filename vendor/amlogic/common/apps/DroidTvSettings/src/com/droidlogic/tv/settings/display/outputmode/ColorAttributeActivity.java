/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 *     AMLOGIC ColorAttributeActivity
 */

package com.droidlogic.tv.settings.display.outputmode;

import com.droidlogic.tv.settings.BaseSettingsFragment;
import com.droidlogic.tv.settings.TvSettingsActivity;

import android.app.Fragment;

/**
 * Activity to control Color space settings.
 */
public class ColorAttributeActivity extends TvSettingsActivity {

    @Override
    protected Fragment createSettingsFragment() {
        return SettingsFragment.newInstance();
    }

    public static class SettingsFragment extends BaseSettingsFragment {

        public static SettingsFragment newInstance() {
            return new SettingsFragment();
        }

        @Override
        public void onPreferenceStartInitialScreen() {
            final ColorAttributeFragment fragment = ColorAttributeFragment.newInstance();
            startPreferenceFragment(fragment);
        }
    }
}


