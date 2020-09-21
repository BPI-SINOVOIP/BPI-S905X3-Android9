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

package com.android.settingslib.drawable;

import static com.google.common.truth.Truth.assertThat;

import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.support.test.InstrumentationRegistry;
import android.support.test.runner.AndroidJUnit4;

import com.android.settingslib.R;

import org.junit.Test;
import org.junit.runner.RunWith;

@RunWith(AndroidJUnit4.class)
public class UserIconDrawableTest {

    private UserIconDrawable mDrawable;

    @Test
    public void getConstantState_shouldNotBeNull() {
        final Bitmap b = BitmapFactory.decodeResource(
                InstrumentationRegistry.getTargetContext().getResources(),
                R.drawable.ic_mode_edit);
        mDrawable = new UserIconDrawable(100 /* size */).setIcon(b).bake();

        assertThat(mDrawable.getConstantState()).isNotNull();
    }
}
