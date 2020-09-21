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

package com.android.tv.settings.autofill;

import android.app.Fragment;
import android.content.Intent;
import android.os.Bundle;

import com.android.tv.settings.BaseSettingsFragment;
import com.android.tv.settings.TvSettingsActivity;

/**
 * Activity pick current autofill service
 */
public class AutofillPickerActivity extends TvSettingsActivity {

    /**
     * Extra set when the fragment is implementing ACTION_REQUEST_SET_AUTOFILL_SERVICE.
     */
    public static final String EXTRA_PACKAGE_NAME = "package_name";

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        final Intent intent = getIntent();
        final String packageName = intent.getData().getSchemeSpecificPart();
        intent.putExtra(EXTRA_PACKAGE_NAME, packageName);
        super.onCreate(savedInstanceState);
    }

    @Override
    protected Fragment createSettingsFragment() {
        return SettingsFragment.newInstance();
    }

    /**
     * SettingsFragment for AutofillPickerActivity
     */
    public static class SettingsFragment extends BaseSettingsFragment {

        /**
         * @return new SettingsFragment instance
         */
        public static SettingsFragment newInstance() {
            return new SettingsFragment();
        }

        @Override
        public void onPreferenceStartInitialScreen() {
            final AutofillPickerFragment fragment = AutofillPickerFragment.newInstance();
            startPreferenceFragment(fragment);
        }
    }
}
