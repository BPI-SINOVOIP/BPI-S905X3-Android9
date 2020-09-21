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
package android.preference2.cts;

import android.app.Activity;
import android.app.FragmentTransaction;
import android.os.Bundle;
import android.preference.Preference;
import android.preference.PreferenceFragment;
import android.preference.PreferenceScreen;

/**
 * Demonstration of PreferenceFragment, showing a single fragment in an
 * activity.
 */
public class PreferenceFragmentActivity extends Activity {

    public PrefFragment prefFragment;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        prefFragment = new PrefFragment();
        getFragmentManager()
                .beginTransaction()
                .replace(android.R.id.content, prefFragment, PrefFragment.TAG)
                .commit();
    }

    public static class PrefFragment extends PreferenceFragment {
        public static final String TAG = "Pref-1";

        public PrefFragment() {

        }
    }
}
