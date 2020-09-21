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
package tests.support;

import java.io.File;
import java.net.URL;
import java.net.URLClassLoader;

/**
 * Support class for creating a file-based ClassLoader. Delegates to either
 * Dalvik's PathClassLoader or the RI's URLClassLoader, but does so by-name.
 * This allows us to run corresponding tests in both environments.
 */
public abstract class Support_ClassLoader {

    public abstract ClassLoader getClassLoader(URL url, ClassLoader parent);

    public static ClassLoader getInstance(URL url, ClassLoader parent) {
        try {
            Support_ClassLoader factory;

            String packageName = Support_ClassLoader.class.getPackage().getName();
            if ("Dalvik".equals(System.getProperty("java.vm.name"))) {
                factory = (Support_ClassLoader)Class.forName(
                    packageName + ".Support_ClassLoaderDalvik").newInstance();
            } else {
                factory = (Support_ClassLoader)Class.forName(
                    packageName + ".Support_ClassLoader$RefImpl").newInstance();
            }

            return factory.getClassLoader(url, parent);
        } catch (Exception ex) {
            throw new RuntimeException("Unable to create ClassLoader", ex);
        }
    }

    /**
     * Implementation for the reference implementation. Nothing interesting to
     * see here. Please get along.
     */
    static class RefImpl extends Support_ClassLoader {
        @Override
        public ClassLoader getClassLoader(URL url, ClassLoader parent) {
            return new URLClassLoader(new URL[] { url }, parent);
        }
    }
}
