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
package android.databinding.testapp.vo;

import android.databinding.BaseObservable;
import android.databinding.ObservableField;
import android.databinding.testapp.BR;
import android.util.ArrayMap;

import java.util.ArrayList;
import java.util.List;
import java.util.Map;

public class FindMethodBindingObject extends FindMethodBindingObjectBase {
    public String method() { return "no arg"; }

    public String method(int i) { return String.valueOf(i); }

    public String method(float f) { return String.valueOf(f); }

    public String method(String value) { return value; }

    public static String staticMethod() { return "world"; }

    public static Foo foo = new Foo();

    public ObservableClass observableClass = new ObservableClass();

    public static Bar<String> bar = new Bar<>();

    public float confusingParam(int i) { return i; }
    public String confusingParam(Object o) { return o.toString(); }

    public int confusingPrimitive(int i) { return i; }
    public String confusingPrimitive(Integer i) { return i.toString(); }

    public float confusingInheritance(Object o) { return 0; }
    public String confusingInheritance(String s) { return s; }
    public int confusingInheritance(Integer i) { return i; }

    public int confusingTypeArgs(List<String> s) { return 0; }
    public String confusingTypeArgs(Map<String, String> s) { return "yay"; }

    public ArrayMap<String, String> getMap() { return null; }

    public int[] getArray() { return new int[5]; }

    public final ObservableField<String> myField = new ObservableField<String>();

    public List getList() {
        ArrayList<String> vals = new ArrayList<>();
        vals.add("hello");
        return vals;
    }

    public int argsClose(int i, String j) {
        return i;
    }

    public float argsClose(int i, short j) {
        return i;
    }

    public int argsClose(int i, int j) {
        return j;
    }

    public static class Foo {
        public final String bar = "hello world";
        public static final String baz = "hello world";
    }

    public static class Bar<T> {
        public T method(T value) { return value; }
    }

    public static final class ObservableClass extends BaseObservable {
        public String x;

        public String getX() {
            return x;
        }

        public void setX(String x) {
            this.x = x;
            notifyPropertyChanged(BR._all);
        }
    }
}
