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
package com.android.car;

import android.util.Pair;
import android.util.SparseArray;
import java.util.stream.IntStream;
import java.util.stream.Stream;

/**
 * Helper class that provides Stream abstractions for android.util.SparseArray
 */
public class SparseArrayStream {
    public static <E> IntStream keyStream(SparseArray<E> array) {
        return IntStream.range(0, array.size()).map(array::keyAt);
    }

    public static <E> Stream<E> valueStream(SparseArray<E> array) {
        return IntStream.range(0, array.size()).mapToObj(array::valueAt);
    }

    public static <E> Stream<Pair<Integer, E>> pairStream(SparseArray<E> array) {
        return IntStream.range(0, array.size()).mapToObj(
            i -> new Pair<>(array.keyAt(i), array.valueAt(i)));
    }
}
