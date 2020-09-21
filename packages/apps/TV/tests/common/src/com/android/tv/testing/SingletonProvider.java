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

package com.android.tv.testing;

import javax.inject.Provider;

/** A Provider that always returns the same instance. */
public class SingletonProvider<T> implements Provider<T> {
    private final T t;

    private SingletonProvider(T t) {
        this.t = t;
    }

    @Override
    public T get() {
        return t;
    }

    public static <S, T extends S> Provider<S> create(T t) {
        return new SingletonProvider<S>(t);
    }
}
