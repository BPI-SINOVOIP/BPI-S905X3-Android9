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

package com.android.car.settings.security;

import android.app.KeyguardManager;
import android.os.Bundle;
import android.support.v4.app.Fragment;
import android.support.v7.widget.Toolbar;

import com.android.car.settings.R;
import com.android.car.settings.common.BaseFragment;
import com.android.car.settings.common.CarSettingActivity;
import com.android.car.settingslib.util.ResultCodes;

/**
 * Entry point Activity for Setup Wizard to set screen lock.
 */
public class SetupWizardScreenLockActivity extends CarSettingActivity implements
        LockTypeDialogFragment.OnLockSelectListener {

    @Override
    public void launchFragment(BaseFragment fragment) {
    }

    @Override
    public void goBack() {
        setResult(RESULT_CANCELED);
        finish();
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        // This activity is meant for Setup Wizard therefore doesn't ask the user for credentials.
        // It's pointless to launch this activity as the lock can't be changed without current
        // credential.
        if (getSystemService(KeyguardManager.class).isKeyguardSecure()) {
            setResult(RESULT_CANCELED);
            finish();
            return;
        }

        setContentView(R.layout.suw_activity);
        Toolbar toolbar = (Toolbar) findViewById(R.id.toolbar);
        setSupportActionBar(toolbar);

        BaseFragment pinFragment = ChooseLockPinPasswordFragment.newPinInstance();
        setFragmentArgs(pinFragment);

        getSupportFragmentManager()
                .beginTransaction()
                .add(R.id.fragment_container, pinFragment)
                .commit();
    }

    /**
     * Handler that will be invoked when Cancel button is clicked in the fragment.
     */
    public void onCancel() {
        setResult(ResultCodes.RESULT_SKIP);
        finish();
    }

    /**
     * Handler that will be invoked when lock save is completed.
     */
    public void onComplete() {
        setResult(RESULT_OK);
        finish();
    }

    @Override
    public void onLockTypeSelected(int position) {
        Fragment fragment = null;

        switch(position) {
            case LockTypeDialogFragment.POSITION_NONE:
                setResult(ResultCodes.RESULT_NONE);
                finish();
                break;
            case LockTypeDialogFragment.POSITION_PIN:
                fragment = ChooseLockPinPasswordFragment.newPinInstance();
                break;
            case LockTypeDialogFragment.POSITION_PATTERN:
                fragment = ChooseLockPatternFragment.newInstance();
                break;
            case LockTypeDialogFragment.POSITION_PASSWORD:
                fragment = ChooseLockPinPasswordFragment.newPasswordInstance();
                break;
            default:
                // Do nothing
        }
        if (fragment != null) {
            setFragmentArgs(fragment);

            getSupportFragmentManager()
                    .beginTransaction()
                    .replace(R.id.fragment_container, fragment)
                    .commitNow();
        }
    }

    private void setFragmentArgs(Fragment fragment) {
        Bundle args = fragment.getArguments();
        if (args == null) {
            args = new Bundle();
        }
        args.putBoolean(BaseFragment.EXTRA_RUNNING_IN_SETUP_WIZARD, true);
        fragment.setArguments(args);
    }
}
