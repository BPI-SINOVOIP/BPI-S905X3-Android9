/*
 * Copyright (C) 2015 The Android Open Source Project
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *      http://www.apache.org/licenses/LICENSE-2.0
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package android.databinding.testapp;

import android.databinding.testapp.databinding.BracketTestBinding;

import android.test.UiThreadTest;
import android.util.LongSparseArray;
import android.util.SparseArray;
import android.util.SparseBooleanArray;
import android.util.SparseIntArray;
import android.util.SparseLongArray;

public class BracketTest extends BaseDataBinderTest<BracketTestBinding> {
    private String[] mArray = {
            "Hello World"
    };

    private SparseArray<String> mSparseArray = new SparseArray<>();
    private SparseIntArray mSparseIntArray = new SparseIntArray();
    private SparseBooleanArray mSparseBooleanArray = new SparseBooleanArray();
    private SparseLongArray mSparseLongArray = new SparseLongArray();
    private LongSparseArray<String> mLongSparseArray = new LongSparseArray<>();

    public BracketTest() {
        super(BracketTestBinding.class);
        mSparseArray.put(0, "Hello");
        mLongSparseArray.put(0, "World");
        mSparseIntArray.put(0, 100);
        mSparseBooleanArray.put(0, true);
        mSparseLongArray.put(0, 5);
    }

    @Override
    protected void setUp() throws Exception {
        super.setUp();
        initBinder(new Runnable() {
            @Override
            public void run() {
                mBinder.setArray(mArray);
                mBinder.setSparseArray(mSparseArray);
                mBinder.setSparseIntArray(mSparseIntArray);
                mBinder.setSparseBooleanArray(mSparseBooleanArray);
                mBinder.setSparseLongArray(mSparseLongArray);
                mBinder.setLongSparseArray(mLongSparseArray);
                mBinder.setIndexObj((Integer) 0);

                mBinder.executePendingBindings();
            }
        });
    }

    @UiThreadTest
    public void testBrackets() {
        assertEquals("Hello World", mBinder.arrayText.getText().toString());
        assertEquals("Hello", mBinder.sparseArrayText.getText().toString());
        assertEquals("World", mBinder.longSparseArrayText.getText().toString());
        assertEquals("100", mBinder.sparseIntArrayText.getText().toString());
        assertEquals("true", mBinder.sparseBooleanArrayText.getText().toString());
        assertEquals("5", mBinder.sparseLongArrayText.getText().toString());
    }

    @UiThreadTest
    public void testBracketOutOfBounds() {
        mBinder.setIndex(1);
        mBinder.executePendingBindings();
        assertEquals("", mBinder.arrayText.getText().toString());
        assertEquals("", mBinder.sparseArrayText.getText().toString());
        assertEquals("", mBinder.longSparseArrayText.getText().toString());
        assertEquals("0", mBinder.sparseIntArrayText.getText().toString());
        assertEquals("false", mBinder.sparseBooleanArrayText.getText().toString());
        assertEquals("0", mBinder.sparseLongArrayText.getText().toString());
        mBinder.setIndex(-1);
        mBinder.executePendingBindings();
        assertEquals("", mBinder.arrayText.getText().toString());
        assertEquals("", mBinder.sparseArrayText.getText().toString());
        assertEquals("", mBinder.longSparseArrayText.getText().toString());
        assertEquals("0", mBinder.sparseIntArrayText.getText().toString());
        assertEquals("false", mBinder.sparseBooleanArrayText.getText().toString());
        assertEquals("0", mBinder.sparseLongArrayText.getText().toString());
    }

    @UiThreadTest
    public void testBracketObj() {
        mBinder.executePendingBindings();
        assertEquals("Hello World", mBinder.indexObjView.getText().toString());
        assertEquals("Hello", mBinder.sparseArrayTextObj.getText().toString());
    }

    @UiThreadTest
    public void testBracketMap() throws Throwable {
        assertEquals("", mBinder.bracketMap.getText().toString());
    }
}
