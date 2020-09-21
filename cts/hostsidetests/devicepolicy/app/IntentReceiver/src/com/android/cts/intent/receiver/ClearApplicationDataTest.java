/*
 * Copyright 2017, The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.android.cts.intent.receiver;

import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;

import android.content.Context;
import android.content.SharedPreferences;
import android.support.test.InstrumentationRegistry;
import android.support.test.filters.SmallTest;

import org.junit.Before;
import org.junit.Test;

/**
 * Test class that writes to shared preference and verifies that the shared preference gets cleared
 * after DPM.clearApplicationUserData was called.
 */
@SmallTest
public class ClearApplicationDataTest {
    private static final String SHARED_PREFERENCE_NAME = "test-preference";
    private static final String I_WAS_HERE = "I-Was-Here";

    private Context mContext;
    private SharedPreferences mSharedPrefs;

    @Before
    public void setUp() {
        mContext = InstrumentationRegistry.getTargetContext();
        mSharedPrefs = mContext.getSharedPreferences(SHARED_PREFERENCE_NAME, Context.MODE_PRIVATE);
    }

    @Test
    public void testWriteToSharedPreference() {
        mSharedPrefs.edit().putBoolean(I_WAS_HERE, true).commit();
        assertTrue(mSharedPrefs.contains(I_WAS_HERE));
    }

    @Test
    public void testSharedPreferenceCleared() {
        assertFalse(mSharedPrefs.contains(I_WAS_HERE));
    }
}
