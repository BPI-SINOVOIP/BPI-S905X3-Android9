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

package com.android.car.settingslib.loader;

import android.annotation.Nullable;
import android.content.Context;
import android.support.v4.content.AsyncTaskLoader;

/**
 * This class fills in some boilerplate for AsyncTaskLoader to actually load things.
 * Classes the extend {@link AsyncLoader} need to properly implement required methods expressed in
 * {@link AsyncTaskLoader}
 *
 * <p>Taken from {@link com.android.settingslib.utils.AsyncLoader}. Only change to extend from
 * support library {@link AsyncTaskLoader}
 *
 * @param <T> the data type to be loaded.
 */
public abstract class AsyncLoader<T> extends AsyncTaskLoader<T> {
    @Nullable
    private T mResult;

    public AsyncLoader(Context context) {
        super(context);
    }

    @Override
    protected void onStartLoading() {
        if (mResult != null) {
            deliverResult(mResult);
        }

        if (takeContentChanged() || mResult == null) {
            forceLoad();
        }
    }

    @Override
    protected void onStopLoading() {
        cancelLoad();
    }

    @Override
    public void deliverResult(T data) {
        if (isReset()) {
            return;
        }
        mResult = data;
        if (isStarted()) {
            super.deliverResult(data);
        }
    }

    @Override
    protected void onReset() {
        super.onReset();
        onStopLoading();
        mResult = null;
    }
}
