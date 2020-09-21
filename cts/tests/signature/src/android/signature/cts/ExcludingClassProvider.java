/*
 * Copyright (C) 2018 The Android Open Source Project
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
package android.signature.cts;

import java.util.function.Predicate;
import java.util.stream.Stream;

/**
 * A filtered class provider which excludes classes by their canonical names
 */
public class ExcludingClassProvider extends ClassProvider {
    private final ClassProvider base;
    private final Predicate<String> testForExclusion;

    public ExcludingClassProvider(ClassProvider base,
            Predicate<String> testForExclusion) {
        this.base = base;
        this.testForExclusion = testForExclusion;
    }

    @Override
    public Class<?> getClass(String name) throws ClassNotFoundException {
        if (!testForExclusion.test(name)) {
            return base.getClass(name);
        }
        // a filtered-out class is the same as non-existing class
        throw new ClassNotFoundException("Cannot find class " + name);
    }

    @Override
    public Stream<Class<?>> getAllClasses() {
        return base.getAllClasses()
                .filter(clazz -> !testForExclusion.test(clazz.getCanonicalName()));
    }

    @Override
    public Stream<DexMember> getAllMembers(Class<?> klass) {
        return base.getAllMembers(klass);
    }
}
