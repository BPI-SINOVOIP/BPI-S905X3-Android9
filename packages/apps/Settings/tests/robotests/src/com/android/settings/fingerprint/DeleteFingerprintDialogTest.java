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

package com.android.settings.fingerprint;

import static org.mockito.Mockito.never;
import static org.mockito.Mockito.spy;
import static org.mockito.Mockito.verify;

import android.app.Dialog;
import android.hardware.fingerprint.Fingerprint;

import com.android.settings.fingerprint.FingerprintSettings.FingerprintSettingsFragment;
import com.android.settings.fingerprint.FingerprintSettings.FingerprintSettingsFragment.DeleteFingerprintDialog;
import com.android.settings.testutils.SettingsRobolectricTestRunner;
import com.android.settings.testutils.shadow.ShadowFragment;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;
import org.robolectric.annotation.Config;
import org.robolectric.util.FragmentTestUtil;

@RunWith(SettingsRobolectricTestRunner.class)
@Config(shadows = ShadowFragment.class)
public class DeleteFingerprintDialogTest {

    @Mock
    private Fingerprint mFingerprint;
    @Mock
    private FingerprintSettingsFragment mTarget;

    private DeleteFingerprintDialog mFragment;

    @Before
    public void setUp() {
        MockitoAnnotations.initMocks(this);
        mFragment = spy(DeleteFingerprintDialog.newInstance(mFingerprint, mTarget));
    }

    @Test
    public void launchDialog_clickPositive_shouldDeleteFingerprint() {
        FragmentTestUtil.startFragment(mFragment);

        mFragment.onClick(mFragment.getDialog(), Dialog.BUTTON_POSITIVE);

        verify(mTarget).deleteFingerPrint(mFingerprint);
    }

    @Test
    public void launchDialog_clickNegative_shouldDoNothing() {
        FragmentTestUtil.startFragment(mFragment);

        mFragment.onClick(mFragment.getDialog(), Dialog.BUTTON_NEGATIVE);

        verify(mTarget, never()).deleteFingerPrint(mFingerprint);
    }
}
