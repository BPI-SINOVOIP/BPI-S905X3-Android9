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

import android.databinding.testapp.databinding.BasicDependantBindingBinding;
import android.databinding.testapp.vo.NotBindableVo;

import android.test.UiThreadTest;

import java.util.ArrayList;
import java.util.List;

public class BasicDependantBindingTest extends BaseDataBinderTest<BasicDependantBindingBinding> {

    public BasicDependantBindingTest() {
        super(BasicDependantBindingBinding.class);
    }

    @Override
    protected void setUp() throws Exception {
        super.setUp();
        initBinder();
    }

    public List<NotBindableVo> permutations(String value) {
        List<NotBindableVo> result = new ArrayList<>();
        result.add(null);
        result.add(new NotBindableVo(null));
        result.add(new NotBindableVo(value));
        return result;
    }

    @UiThreadTest
    public void testAllPermutations() {
        List<NotBindableVo> obj1s = permutations("a");
        List<NotBindableVo> obj2s = permutations("b");
        for (NotBindableVo obj1 : obj1s) {
            for (NotBindableVo obj2 : obj2s) {
                reCreateBinder(null); //get a new one
                testWith(obj1, obj2);
                reCreateBinder(null);
                mBinder.executePendingBindings();
                testWith(obj1, obj2);
            }
        }
    }

    private void testWith(NotBindableVo obj1, NotBindableVo obj2) {
        mBinder.setObj1(obj1);
        mBinder.setObj2(obj2);
        mBinder.executePendingBindings();
        assertValues(safeGet(obj1), safeGet(obj2),
                obj1 == null ? "" : obj1.mergeStringFields(obj2),
                obj2 == null ? "" : obj2.mergeStringFields(obj1),
                (obj1 == null ? null : obj1.getStringValue())
                        + (obj2 == null ? null : obj2.getStringValue())
        );
    }

    private String safeGet(NotBindableVo vo) {
        if (vo == null || vo.getStringValue() == null) {
            return "";
        }
        return vo.getStringValue();
    }

    private void assertValues(String textView1, String textView2,
            String mergedView1, String mergedView2, String rawMerge) {
        assertEquals(textView1, mBinder.textView1.getText().toString());
        assertEquals(textView2, mBinder.textView2.getText().toString());
        assertEquals(mergedView1, mBinder.mergedTextView1.getText().toString());
        assertEquals(mergedView2, mBinder.mergedTextView2.getText().toString());
        assertEquals(rawMerge, mBinder.rawStringMerge.getText().toString());
    }
}
