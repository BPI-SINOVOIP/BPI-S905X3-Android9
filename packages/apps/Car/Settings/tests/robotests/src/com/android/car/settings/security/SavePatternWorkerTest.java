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

import static org.mockito.Mockito.doNothing;
import static org.mockito.Mockito.doThrow;
import static org.mockito.Mockito.spy;

import com.android.car.settings.CarSettingsRobolectricTestRunner;

import org.junit.Test;
import org.junit.runner.RunWith;

/**
 * Tests for SavePatternWorker class.
 */
@RunWith(CarSettingsRobolectricTestRunner.class)
public class SavePatternWorkerTest {
    /**
     * A test to check return value when save worker succeeds
     */
    @Test
    public void testSaveLockSuccessReturnsTrue() {
        SavePatternWorker worker = spy(new SavePatternWorker());

        doNothing().when(worker).saveLock();

        assertThat(worker.saveAndVerifyInBackground()).isTrue();
    }

    /**
     * A test to check return value when save worker fails
     */
    @Test
    public void testSaveLockFailureReturnsFalse() {
        SavePatternWorker worker = spy(new SavePatternWorker());

        doThrow(new RuntimeException()).when(worker).saveLock();

        assertThat(worker.saveAndVerifyInBackground()).isFalse();
    }
}
