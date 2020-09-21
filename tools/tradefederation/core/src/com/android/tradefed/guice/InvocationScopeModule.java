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
package com.android.tradefed.guice;

import com.android.tradefed.config.IConfiguration;

import com.google.inject.AbstractModule;

/**
 * Guice module that can be used anywhere in a TF invocation to requests the Guice-Tradefed
 * supported objects.
 */
public class InvocationScopeModule extends AbstractModule {

    private InvocationScope mScope;

    public InvocationScopeModule() {
        this(InvocationScope.getDefault());
    }

    public InvocationScopeModule(InvocationScope scope) {
        mScope = scope;
    }

    @Override
    public void configure() {
        // Tell Guice about the scope
        bindScope(InvocationScoped.class, mScope);

        // Make our scope instance injectable
        bind(InvocationScope.class).toInstance(mScope);

        // IConfiguration is a supported Guice-Tradefed object.
        bind(IConfiguration.class)
                .toProvider(InvocationScope.<IConfiguration>seededKeyProvider())
                .in(InvocationScoped.class);
    }
}
