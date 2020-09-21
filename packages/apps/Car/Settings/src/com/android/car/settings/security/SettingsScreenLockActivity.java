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

import android.app.admin.DevicePolicyManager;
import android.os.Bundle;
import android.os.UserHandle;

import com.android.car.settings.R;
import com.android.car.settings.common.BaseFragment;
import com.android.car.settings.common.CarSettingActivity;
import com.android.car.settings.common.Logger;
import com.android.internal.widget.LockPatternUtils;

/**
 * Activity for setting screen locks
 */
public class SettingsScreenLockActivity extends CarSettingActivity implements CheckLockListener {

    public static final String EXTRA_CURRENT_SCREEN_LOCK = "extra_current_screen_lock";
    private static final Logger LOG = new Logger(SettingsScreenLockActivity.class);

    private int mPasswordQuality;
    private LockPatternUtils mLockPatternUtils;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        mLockPatternUtils = new LockPatternUtils(this);
        mPasswordQuality = mLockPatternUtils.getKeyguardStoredPasswordQuality(
                UserHandle.myUserId());

        if (savedInstanceState == null) {
            BaseFragment fragment;
            switch (mPasswordQuality) {
                case DevicePolicyManager.PASSWORD_QUALITY_UNSPECIFIED:
                    fragment = ChooseLockTypeFragment.newInstance();
                    break;
                case DevicePolicyManager.PASSWORD_QUALITY_SOMETHING:
                    fragment = ConfirmLockPatternFragment.newInstance();
                    break;
                case DevicePolicyManager.PASSWORD_QUALITY_NUMERIC:
                case DevicePolicyManager.PASSWORD_QUALITY_NUMERIC_COMPLEX:
                    fragment = ConfirmLockPinPasswordFragment.newPinInstance();
                    break;
                case DevicePolicyManager.PASSWORD_QUALITY_ALPHABETIC:
                case DevicePolicyManager.PASSWORD_QUALITY_ALPHANUMERIC:
                    fragment = ConfirmLockPinPasswordFragment.newPasswordInstance();
                    break;
                default:
                    LOG.e("Unexpected password quality: " + String.valueOf(mPasswordQuality));
                    fragment = ConfirmLockPinPasswordFragment.newPasswordInstance();
            }

            Bundle bundle = fragment.getArguments();
            if (bundle == null) {
                bundle = new Bundle();
            }
            bundle.putInt(ChooseLockTypeFragment.EXTRA_CURRENT_PASSWORD_QUALITY, mPasswordQuality);
            fragment.setArguments(bundle);

            getSupportFragmentManager()
                    .beginTransaction()
                    .add(R.id.fragment_container, fragment)
                    .addToBackStack(null)
                    .commit();
        }
    }

    @Override
    public void onLockVerified(String lock) {
        BaseFragment fragment = ChooseLockTypeFragment.newInstance();
        Bundle bundle = fragment.getArguments();
        if (bundle == null) {
            bundle = new Bundle();
        }
        bundle.putString(EXTRA_CURRENT_SCREEN_LOCK, lock);
        bundle.putInt(ChooseLockTypeFragment.EXTRA_CURRENT_PASSWORD_QUALITY, mPasswordQuality);
        fragment.setArguments(bundle);

        getSupportFragmentManager()
                .beginTransaction()
                .replace(R.id.fragment_container, fragment)
                .commit();
    }
}
