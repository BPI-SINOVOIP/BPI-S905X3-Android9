/*
 * Copyright 2018 The Android Open Source Project
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

package androidx.legacy.view;

import android.view.View;

/**
 * Helper for accessing features in {@link View} introduced after API
 * level 13 in a backwards compatible fashion.
 *
 * @deprecated Use {@link androidx.core.view.ViewCompat} instead.
 */
@Deprecated
public class ViewCompat extends androidx.core.view.ViewCompat {
    private ViewCompat() {
    }
}
