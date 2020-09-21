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

import java.lang.reflect.Constructor;
import java.lang.reflect.Method;

public class Main {
    static final String DEX_FILE = System.getenv("DEX_LOCATION") + "/636-wrong-static-access-ex.jar";

    public static void main(String[] args) throws Exception {
        System.loadLibrary(args[0]);
        Class<?> pathClassLoader = Class.forName("dalvik.system.PathClassLoader");
        if (pathClassLoader == null) {
            throw new AssertionError("Couldn't find path class loader class");
        }
        Constructor<?> constructor =
            pathClassLoader.getDeclaredConstructor(String.class, ClassLoader.class);
        ClassLoader loader = (ClassLoader) constructor.newInstance(
            DEX_FILE, ClassLoader.getSystemClassLoader());
        Class<?> foo = loader.loadClass("Foo");
        Method doTest = foo.getDeclaredMethod("doTest");
        doTest.invoke(null);
    }

    public static native void ensureJitCompiled(Class<?> cls, String methodName);
}
