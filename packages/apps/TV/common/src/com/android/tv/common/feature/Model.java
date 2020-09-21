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
 * limitations under the License
 */

package com.android.tv.common.feature;

import android.content.Context;

/** Holder for {@link android.os.Build#MODEL} features. */
public interface Model {

    ModelFeature NEXUS_PLAYER = new ModelFeature("Nexus Player");

    /** True when the {@link android.os.Build#MODEL} equals the {@code model} given. */
    public static final class ModelFeature implements Feature {
        private final String mModel;

        private ModelFeature(String model) {
            mModel = model;
        }

        @Override
        public boolean isEnabled(Context context) {
            return isEnabled();
        }

        public boolean isEnabled() {
            return android.os.Build.MODEL.equals(mModel);
        }

        @Override
        public String toString() {
            return "ModelFeature(" + mModel + ")=" + isEnabled();
        }
    }
}
