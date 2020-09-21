/*
 * Copyright (C) 2014 The Android Open Source Project
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

package com.example.android.basicmanagedprofile;

import android.app.Activity;
import android.app.admin.DevicePolicyManager;
import android.content.Context;
import android.os.Bundle;

public class MainActivity extends Activity {

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main_real);
        if (savedInstanceState == null) {
            DevicePolicyManager manager = (DevicePolicyManager)
                    getSystemService(Context.DEVICE_POLICY_SERVICE);
            if (manager.isProfileOwnerApp(getApplicationContext().getPackageName())) {
                // If the managed profile is already set up, we show the main screen.
                showMainFragment();
            } else {
                // If not, we show the set up screen.
                showSetupProfile();
            }
        }
    }

    private void showSetupProfile() {
        getFragmentManager().beginTransaction()
                .replace(R.id.container, SetupProfileFragment.newInstance())
                .commit();
    }

    private void showMainFragment() {
        getFragmentManager().beginTransaction()
                .add(R.id.container, BasicManagedProfileFragment.newInstance())
                .commit();
    }

}
