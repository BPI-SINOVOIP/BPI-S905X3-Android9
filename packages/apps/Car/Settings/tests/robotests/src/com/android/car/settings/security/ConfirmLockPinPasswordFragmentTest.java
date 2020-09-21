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

import static com.google.common.truth.Truth.assertThat;

import android.view.View;

import com.android.car.settings.CarSettingsRobolectricTestRunner;
import com.android.car.settings.R;
import com.android.car.settings.common.BaseFragment;
import com.android.car.settings.testutils.BaseTestActivity;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.robolectric.Robolectric;

/**
 * Tests for ConfirmLockPinPasswordFragment class.
 */
@RunWith(CarSettingsRobolectricTestRunner.class)
public class ConfirmLockPinPasswordFragmentTest {

    private TestSettingsScreenLockActivity mTestActivity;
    private ConfirmLockPinPasswordFragment mPinFragment;

    @Before
    public void initFragment() {
        mTestActivity = Robolectric.buildActivity(TestSettingsScreenLockActivity.class)
                .create()
                .start()
                .resume()
                .get();

        mPinFragment = ConfirmLockPinPasswordFragment.newPinInstance();
        mTestActivity.launchFragment(mPinFragment);
    }

    /**
     * A test to verify that the Enter key is re-enabled when verification fails.
     */
    @Test
    public void testEnterKeyIsEnabledWhenCheckFails() {
        View enterKey = mPinFragment.getView().findViewById(R.id.key_enter);
        enterKey.setEnabled(false);

        mPinFragment.onCheckCompleted(false);

        assertThat(enterKey.isEnabled()).isTrue();
    }

    /**
     * The containing activity of ConfirmLockPinPasswordFragment must implement two interfaces
     */
    private static class TestSettingsScreenLockActivity extends BaseTestActivity implements
            CheckLockListener, BaseFragment.FragmentController {

        @Override
        public void onLockVerified(String lock) {}

        @Override
        public void goBack() {}

        @Override
        public void showDOBlockingMessage() {}
    }
}
