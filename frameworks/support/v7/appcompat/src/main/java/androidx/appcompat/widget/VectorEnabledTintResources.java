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

import static androidx.annotation.RestrictTo.Scope.LIBRARY_GROUP;

import android.content.Context;
import android.content.res.Resources;
import android.graphics.drawable.Drawable;
import android.os.Build;

import androidx.annotation.NonNull;
import androidx.annotation.RestrictTo;

import java.lang.ref.WeakReference;

/**
 * This class allows us to intercept calls so that we can tint resources (if applicable), and
 * inflate vector resources from within drawable containers pre-L.
 *
 * @hide
 */
@RestrictTo(LIBRARY_GROUP)
public class VectorEnabledTintResources extends Resources {
    private static boolean sCompatVectorFromResourcesEnabled = false;

    public static boolean shouldBeUsed() {
        return isCompatVectorFromResourcesEnabled()
                && Build.VERSION.SDK_INT <= MAX_SDK_WHERE_REQUIRED;
    }

    /**
     * The maximum API level where this class is needed.
     */
    public static final int MAX_SDK_WHERE_REQUIRED = 20;

    private final WeakReference<Context> mContextRef;

    public VectorEnabledTintResources(@NonNull final Context context,
            @NonNull final Resources res) {
        super(res.getAssets(), res.getDisplayMetrics(), res.getConfiguration());
        mContextRef = new WeakReference<>(context);
    }

    /**
     * We intercept this call so that we tint the result (if applicable). This is needed for
     * things like {@link android.graphics.drawable.DrawableContainer}s which can retrieve
     * their children via this method.
     */
    @Override
    public Drawable getDrawable(int id) throws NotFoundException {
        final Context context = mContextRef.get();
        if (context != null) {
            return AppCompatDrawableManager.get().onDrawableLoadedFromResources(context, this, id);
        } else {
            return super.getDrawable(id);
        }
    }

    final Drawable superGetDrawable(int id) {
        return super.getDrawable(id);
    }

    /**
     * Sets whether vector drawables on older platforms (< API 21) can be used within
     * {@link android.graphics.drawable.DrawableContainer} resources.
     */
    public static void setCompatVectorFromResourcesEnabled(boolean enabled) {
        sCompatVectorFromResourcesEnabled = enabled;
    }

    /**
     * Returns whether vector drawables on older platforms (< API 21) can be accessed from within
     * resources.
     *
     * @see #setCompatVectorFromResourcesEnabled(boolean)
     */
    public static boolean isCompatVectorFromResourcesEnabled() {
        return sCompatVectorFromResourcesEnabled;
    }
}