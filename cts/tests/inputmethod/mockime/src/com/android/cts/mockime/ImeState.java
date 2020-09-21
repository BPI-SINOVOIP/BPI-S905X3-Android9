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

package com.android.cts.mockime;

import android.os.Bundle;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

/**
 * An immutable object that stores several runtime state of {@link MockIme}.
 */
public final class ImeState {
    private final boolean mHasInputBinding;
    private final boolean mHasDummyInputConnection;

    /**
     * @return {@code true} if {@link MockIme#getCurrentInputBinding()} returned non-null
     *         {@link android.view.inputmethod.InputBinding} when this snapshot was taken.
     */
    public boolean hasInputBinding() {
        return mHasInputBinding;
    }

    /**
     * @return {@code true} if {@link MockIme#getCurrentInputConnection()} returned non-dummy
     *         {@link android.view.inputmethod.InputConnection} when this snapshot was taken.
     */
    public boolean hasDummyInputConnection() {
        return mHasDummyInputConnection;
    }

    ImeState(boolean hasInputBinding, boolean hasDummyInputConnection) {
        mHasInputBinding = hasInputBinding;
        mHasDummyInputConnection = hasDummyInputConnection;
    }

    @NonNull
    Bundle toBundle() {
        final Bundle bundle = new Bundle();
        bundle.putBoolean("mHasInputBinding", mHasInputBinding);
        bundle.putBoolean("mHasDummyInputConnection", mHasDummyInputConnection);
        return bundle;
    }

    @Nullable
    static ImeState fromBundle(@Nullable Bundle bundle) {
        if (bundle == null) {
            return null;
        }
        final boolean hasInputBinding = bundle.getBoolean("mHasInputBinding");
        final boolean hasDummyInputConnection = bundle.getBoolean("mHasDummyInputConnection");
        return new ImeState(hasInputBinding, hasDummyInputConnection);
    }
}
