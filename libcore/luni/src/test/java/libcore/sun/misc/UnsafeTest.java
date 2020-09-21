/*
 * Copyright (C) 2009 The Android Open Source Project
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

package libcore.sun.misc;

import junit.framework.TestCase;

import java.lang.reflect.Field;
import java.util.concurrent.Callable;
import java.util.concurrent.Executors;

import sun.misc.Unsafe;

public class UnsafeTest extends TestCase {

    public void test_getUnsafeForbidden() {
        try {
            Unsafe.getUnsafe();
            fail();
        } catch (SecurityException expected) {
        }
    }

    /**
     * Regression for 2053217. We used to look one level higher than necessary
     * on the stack.
     */
    public void test_getUnsafeForbiddenWithSystemCaller() throws Exception {
        Callable<Object> callable = Executors.callable(new Runnable() {
            public void run() {
                Unsafe.getUnsafe();
            }
        });

        try {
            callable.call();
            fail();
        } catch (SecurityException expected) {
        }
    }

    private class AllocateInstanceTestClass {
        public int i = 123;
        public String s = "hello";
        public Object getThis() { return AllocateInstanceTestClass.this; }
    }

    private static Unsafe getUnsafe() throws Exception {
        Class<?> unsafeClass = Class.forName("sun.misc.Unsafe");
        Field f = unsafeClass.getDeclaredField("theUnsafe");
        f.setAccessible(true);
        return (Unsafe) f.get(null);
    }

    public void test_allocateInstance() throws Exception {
        AllocateInstanceTestClass i = (AllocateInstanceTestClass)
                getUnsafe().allocateInstance(AllocateInstanceTestClass.class);
        assertEquals(0, i.i);
        assertEquals(null, i.s);
        assertEquals(i, i.getThis());
    }
}
