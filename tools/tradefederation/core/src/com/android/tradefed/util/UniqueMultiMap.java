/*
 * Copyright (C) 2011 The Android Open Source Project
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
package com.android.tradefed.util;

import com.android.tradefed.build.BuildSerializedVersion;

import java.util.Collection;

/**
 * A {@link MultiMap} that ensures unique values for each key.
 * <p/>
 * Attempts to insert a duplicate value will be ignored
 * @param <K>
 */
public class UniqueMultiMap<K, V> extends MultiMap<K, V> {

    private static final long serialVersionUID = BuildSerializedVersion.VERSION;

    @Override
    public V put(K key, V value) {
        Collection<V> values = get(key);
        if (values != null && values.contains(value)) {
            return null;
        }
        return super.put(key, value);
    }
}
