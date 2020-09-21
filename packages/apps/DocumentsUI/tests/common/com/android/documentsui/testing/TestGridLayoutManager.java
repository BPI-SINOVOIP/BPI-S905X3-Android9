/*
 * Copyright (C) 2016 The Android Open Source Project
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

package com.android.documentsui.testing;

import android.content.Context;
import android.support.v7.widget.GridLayoutManager;

import org.mockito.Mockito;

public class TestGridLayoutManager extends GridLayoutManager {

    private int mFirstVisibleItemPosition;

    public static TestGridLayoutManager create() {
        final TestGridLayoutManager manager = Mockito.mock(TestGridLayoutManager.class,
                Mockito.withSettings().defaultAnswer(Mockito.CALLS_REAL_METHODS));
        return manager;
    }

    @Override
    public int findFirstVisibleItemPosition() { return mFirstVisibleItemPosition; }

    public void setFirstVisibleItemPosition(int position) { mFirstVisibleItemPosition = position; }

    private TestGridLayoutManager(Context context, int spanCount) {
        super(context, spanCount);
    }
}
