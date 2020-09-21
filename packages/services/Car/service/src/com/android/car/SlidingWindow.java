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

import java.util.ArrayDeque;
import java.util.Iterator;
import java.util.function.Predicate;
import java.util.stream.Stream;

/**
 * This class keeps track of a limited fixed number of sample data points, correctly removing
 * older samples as new ones are added, and it allows inspecting the samples, as well as
 * easily answering N out of M questions.
 */
class SlidingWindow<T> implements Iterable<T> {
    private final ArrayDeque<T> mElements;
    private final int mMaxSize;

    public SlidingWindow(int size) {
        mMaxSize = size;
        mElements = new ArrayDeque<>(mMaxSize);
    }

    public void add(T sample) {
        if (mElements.size() == mMaxSize) {
            mElements.removeFirst();
        }
        mElements.addLast(sample);
    }

    public void addAll(Iterable<T> elements) {
        elements.forEach(this::add);
    }

    @Override
    public Iterator<T> iterator() {
        return mElements.iterator();
    }

    public Stream<T> stream() {
        return mElements.stream();
    }

    public int size() {
        return mElements.size();
    }

    public int count(Predicate<T> predicate) {
        return (int)stream().filter(predicate).count();
    }
}
