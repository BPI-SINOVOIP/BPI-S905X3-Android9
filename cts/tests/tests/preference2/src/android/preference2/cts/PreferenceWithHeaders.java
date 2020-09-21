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

import android.os.Bundle;
import android.preference.PreferenceActivity;
import android.preference.PreferenceFragment;
import android.preference2.cts.R;
import android.widget.Button;

import java.util.List;

/**
 * Demonstration of PreferenceActivity to make a top-level preference
 * panel with headers.
 */

public class PreferenceWithHeaders extends PreferenceActivity {

    // For tests to verify if the headers were loaded.
    List<Header> loadedHeaders;

    boolean onCreateFinished;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        // Add a button to the header list.
        if (hasHeaders()) {
            Button button = new Button(this);
            button.setText("Some action");
            setListFooter(button);
        }
        onCreateFinished = true;
    }

    /*
     * Validate the fragment loaded into this activity. Required for apps built for API 19 and
     * above.
     */
    protected boolean isValidFragment(String fragment) {
        return PrefsOneFragment.class.getName().equals(fragment)
                || PrefsTwoFragment.class.getName().equals(fragment)
                || PrefsOneFragmentInner.class.getName().equals(fragment);
    }

    /**
     * Populate the activity with the top-level headers.
     */
    @Override
    public void onBuildHeaders(List<Header> target) {
        loadedHeaders = target;
        loadHeadersFromResource(R.xml.preference_headers, target);
    }

    public static class PrefsOneFragment extends PreferenceFragment {
        @Override
        public void onCreate(Bundle savedInstanceState) {
            super.onCreate(savedInstanceState);
            addPreferencesFromResource(R.xml.preferences);
        }
    }

    public static class PrefsTwoFragment extends PreferenceFragment {
        @Override
        public void onCreate(Bundle savedInstanceState) {
            super.onCreate(savedInstanceState);
            addPreferencesFromResource(R.xml.preferences2);
        }
    }

    public static class PrefsOneFragmentInner extends PreferenceFragment {
        @Override
        public void onCreate(Bundle savedInstanceState) {
            super.onCreate(savedInstanceState);
            addPreferencesFromResource(R.xml.fragmented_preferences_inner);
        }
    }
}

