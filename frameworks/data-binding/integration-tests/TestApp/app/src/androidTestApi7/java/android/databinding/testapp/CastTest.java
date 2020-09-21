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
package android.databinding.testapp;

import android.databinding.testapp.databinding.CastTestBinding;

import android.support.v4.util.ArrayMap;
import android.test.UiThreadTest;

import java.util.ArrayList;

public class CastTest extends BaseDataBinderTest<CastTestBinding> {
    ArrayList<String> mValues = new ArrayList<>();
    ArrayMap<String, String> mMap = new ArrayMap<>();

    public CastTest() {
        super(CastTestBinding.class);
    }

    @UiThreadTest
    public void testCast() throws Throwable {
        initBinder();
        mValues.clear();
        mValues.add("hello");
        mValues.add("world");
        mValues.add("not seen");
        mMap.clear();
        mMap.put("hello", "world");
        mMap.put("world", "hello");
        mBinder.setList(mValues);
        mBinder.setMap(mMap);
        mBinder.executePendingBindings();
        assertEquals("hello", mBinder.textView0.getText().toString());
        assertEquals("world", mBinder.textView1.getText().toString());
    }
}
