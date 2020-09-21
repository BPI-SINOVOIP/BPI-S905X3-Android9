/*
 * Copyright (C) 2015 The Android Open Source Project
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

package androidx.appcompat.widget;

import android.content.Context;
import android.content.res.Resources;
import android.graphics.drawable.Drawable;

import androidx.annotation.NonNull;

import java.lang.ref.WeakReference;

/**
 * This class allows us to intercept calls so that we can tint resources (if applicable).
 */
class TintResources extends ResourcesWrapper {

    private final WeakReference<Context> mContextRef;

    public TintResources(@NonNull Context context, @NonNull final Resources res) {
        super(res);
        mContextRef = new WeakReference<>(context);
    }

    /**
     * We intercept this call so that we tint the result (if applicable). This is needed for
     * things like {@link android.graphics.drawable.DrawableContainer}s which can retrieve
     * their children via this method.
     */
    @Override
    public Drawable getDrawable(int id) throws NotFoundException {
        Drawable d = super.getDrawable(id);
        Context context = mContextRef.get();
        if (d != null && context != null) {
            AppCompatDrawableManager.get().tintDrawableUsingColorFilter(context, id, d);
        }
        return d;
    }
}