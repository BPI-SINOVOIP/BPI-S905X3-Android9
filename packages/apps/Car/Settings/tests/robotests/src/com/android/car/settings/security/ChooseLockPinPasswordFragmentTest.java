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

import static org.mockito.Mockito.doNothing;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.spy;
import static org.mockito.Mockito.verify;

import com.android.car.settings.CarSettingsRobolectricTestRunner;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;

/**
 * Tests for ChooseLockPinPasswordFragment class.
 */
@RunWith(CarSettingsRobolectricTestRunner.class)
public class ChooseLockPinPasswordFragmentTest {
    private ChooseLockPinPasswordFragment mFragment;

    @Before
    public void initFragment() {
        mFragment = ChooseLockPinPasswordFragment.newPasswordInstance();
    }

    /**
     * A test to verify that onComplete is called is finished when save worker succeeds
     */
    @Test
    public void testOnCompleteIsCalledWhenSaveWorkerSucceeds() {
        ChooseLockPinPasswordFragment spyFragment = spy(mFragment);
        doNothing().when(spyFragment).onComplete();

        spyFragment.onChosenLockSaveFinished(true);

        verify(spyFragment).onComplete();
    }

    /**
     * A test to verify that the UI stage is updated when save worker fails
     */
    @Test
    public void testStageIsUpdatedWhenSaveWorkerFails() {
        ChooseLockPinPasswordFragment spyFragment = spy(mFragment);
        doNothing().when(spyFragment).updateStage(ChooseLockPinPasswordFragment.Stage.SaveFailure);

        spyFragment.onChosenLockSaveFinished(false);

        verify(spyFragment, never()).onComplete();
        verify(spyFragment).updateStage(ChooseLockPinPasswordFragment.Stage.SaveFailure);
    }
}
