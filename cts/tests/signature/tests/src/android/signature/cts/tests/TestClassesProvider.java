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
package android.signature.cts.tests;

import java.util.stream.Stream;

import android.signature.cts.ClassProvider;
import android.signature.cts.DexMember;
import android.signature.cts.tests.data.AbstractClass;
import android.signature.cts.tests.data.SystemApiClass;
import android.signature.cts.tests.data.FinalClass;
import android.signature.cts.tests.data.PrivateClass;
import android.signature.cts.tests.data.PublicApiClass;
import android.signature.cts.tests.data.ForciblyPublicizedPrivateClass;
import android.signature.cts.tests.data.NormalClass;
import android.signature.cts.tests.data.NormalException;
import android.signature.cts.tests.data.NormalInterface;
import android.signature.cts.tests.data.ApiAnnotation;

public class TestClassesProvider extends ClassProvider {
    @Override
    public Stream<Class<?>> getAllClasses() {
        Stream.Builder<Class<?>> builder = Stream.builder();
        builder.add(AbstractClass.class);
        builder.add(FinalClass.class);
        builder.add(NormalClass.class);
        builder.add(NormalException.class);
        builder.add(NormalInterface.class);
        builder.add(ApiAnnotation.class);
        builder.add(PublicApiClass.class);
        builder.add(SystemApiClass.class);
        builder.add(PrivateClass.class);
        builder.add(ForciblyPublicizedPrivateClass.class);
        return builder.build();
    }

    @Override
    public Stream<DexMember> getAllMembers(Class<?> klass) {
        return null;
    }
}
