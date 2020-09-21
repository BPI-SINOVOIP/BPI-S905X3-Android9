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

package com.android.car.settings.testutils;

import static org.robolectric.shadow.api.Shadow.directlyOn;

import android.annotation.ColorRes;
import android.annotation.DimenRes;
import android.annotation.Nullable;
import android.content.res.Resources;

import org.robolectric.annotation.Implementation;
import org.robolectric.annotation.Implements;
import org.robolectric.annotation.RealObject;
import org.robolectric.shadows.ShadowResources;

/**
 * Overrides ShadowResources to return a dummy value if resources are not found.
 *
 * Reason for this is the Design Support Library has moved to prebuilts but robolectric is unable
 * to pull resources from binaries at this time.
 * TODO: Delete this shadow when robolectric supports binary resource loading
 */
@Implements(value = Resources.class, inheritImplementationMethods = true)
public class CarSettingsShadowResources extends ShadowResources {
    @RealObject
    public Resources realResources;

    @Implementation
    public int getDimensionPixelSize(@DimenRes int id) {
        try {
            return directlyOn(realResources, Resources.class).getDimensionPixelSize(id);
        } catch (Resources.NotFoundException e1) {
            return 0;
        }
    }

    @Implementation
    public int getDimensionPixelOffset(@DimenRes int id) {
        try {
            return directlyOn(realResources, Resources.class).getDimensionPixelOffset(id);
        } catch (Resources.NotFoundException e1) {
            return 0;
        }
    }

    @Implementation
    public int getColor(@ColorRes int id, @Nullable Resources.Theme theme) {
        try {
            return directlyOn(realResources, Resources.class).getColor(id, theme);
        } catch (Resources.NotFoundException e1) {
            return 0;
        }
    }
}

