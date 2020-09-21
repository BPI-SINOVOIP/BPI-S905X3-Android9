/**
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

package com.android.car.radio.platform;

import android.annotation.NonNull;
import android.annotation.Nullable;
import android.graphics.Bitmap;

import com.android.car.broadcastradio.support.platform.ImageResolver;

import java.util.LinkedHashMap;
import java.util.Map;
import java.util.Objects;

public class ImageMemoryCache implements ImageResolver {
    private final RadioManagerExt mRadioManager;
    private final Map<Long, Bitmap> mCache;

    public ImageMemoryCache(@NonNull RadioManagerExt radioManager, int cacheSize) {
        mRadioManager = Objects.requireNonNull(radioManager);
        mCache = new CacheMap<>(cacheSize);
    }

    public @Nullable Bitmap resolve(long globalId) {
        synchronized (mCache) {
            if (mCache.containsKey(globalId)) return mCache.get(globalId);

            Bitmap bm = mRadioManager.getMetadataImage(globalId);
            mCache.put(globalId, bm);
            return bm;
        }
    }

    private static class CacheMap<K, V> extends LinkedHashMap<K, V> {
        private final int mMaxSize;

        public CacheMap(int maxSize) {
            if (maxSize < 0) throw new IllegalArgumentException("maxSize must not be negative");
            mMaxSize = maxSize;
        }

        @Override
        protected boolean removeEldestEntry(Map.Entry<K, V> eldest) {
            return size() > mMaxSize;
        }
    }
}
