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
 * limitations under the License
 */

package com.android.tv.common.feature;

import android.content.Context;
import com.android.tv.common.experiments.ExperimentFlag;

/** A {@link Feature} base on an {@link ExperimentFlag}. */
public final class ExperimentFeature implements Feature {

    public static Feature from(ExperimentFlag<Boolean> flag) {
        return new ExperimentFeature(flag);
    }

    private final ExperimentFlag<Boolean> mFlag;

    private ExperimentFeature(ExperimentFlag<Boolean> flag) {
        mFlag = flag;
    }

    @Override
    public boolean isEnabled(Context context) {
        return mFlag.get();
    }

    @Override
    public String toString() {
        return "ExperimentFeature for " + mFlag;
    }
}
