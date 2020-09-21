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

public final class ImeCommand {

    private static final String NAME_KEY = "name";
    private static final String ID_KEY = "id";
    private static final String DISPATCH_TO_MAIN_THREAD_KEY = "dispatchToMainThread";
    private static final String EXTRA_KEY = "extra";

    @NonNull
    private final String mName;
    private final long mId;
    private final boolean mDispatchToMainThread;
    @NonNull
    private final Bundle mExtras;

    ImeCommand(@NonNull String name, long id, boolean dispatchToMainThread,
            @NonNull Bundle extras) {
        mName = name;
        mId = id;
        mDispatchToMainThread = dispatchToMainThread;
        mExtras = extras;
    }

    private ImeCommand(@NonNull Bundle bundle) {
        mName = bundle.getString(NAME_KEY);
        mId = bundle.getLong(ID_KEY);
        mDispatchToMainThread = bundle.getBoolean(DISPATCH_TO_MAIN_THREAD_KEY);
        mExtras = bundle.getParcelable(EXTRA_KEY);
    }

    static ImeCommand fromBundle(@NonNull Bundle bundle) {
        return new ImeCommand(bundle);
    }

    Bundle toBundle() {
        final Bundle bundle = new Bundle();
        bundle.putString(NAME_KEY, mName);
        bundle.putLong(ID_KEY, mId);
        bundle.putBoolean(DISPATCH_TO_MAIN_THREAD_KEY, mDispatchToMainThread);
        bundle.putParcelable(EXTRA_KEY, mExtras);
        return bundle;
    }

    @NonNull
    public String getName() {
        return mName;
    }

    public long getId() {
        return mId;
    }

    public boolean shouldDispatchToMainThread() {
        return mDispatchToMainThread;
    }

    @NonNull
    public Bundle getExtras() {
        return mExtras;
    }
}
