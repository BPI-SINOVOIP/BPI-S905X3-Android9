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

import static com.google.common.base.Preconditions.checkState;

import com.android.tradefed.config.IConfiguration;

import com.google.common.collect.Maps;
import com.google.inject.Guice;
import com.google.inject.Injector;
import com.google.inject.Key;
import com.google.inject.OutOfScopeException;
import com.google.inject.Provider;
import com.google.inject.Scope;
import com.google.inject.Scopes;

import java.util.Map;

/**
 * Scopes a single Tradefed invocation.
 *
 * <p>The scope can be initialized with one or more seed values by calling <code>seed(key, value)
 * </code> before the injector will be called upon to provide for this key. A typical use is for a
 * test invocation to enter/exit the scope, representing an invocation Scope, and seed configuration
 * objects. For each key inserted with seed(), you must include a corresponding binding:
 *
 * <pre><code>
 *   bind(key)
 *       .toProvider(SimpleScope.<key.class>seededKeyProvider())
 *       .in(InvocationScoped.class);
 * </code></pre>
 *
 * FIXME: Possibly handle multi objects (like lists).
 */
public class InvocationScope implements Scope {

    public InvocationScope() {}

    private static final Provider<Object> SEEDED_KEY_PROVIDER =
            new Provider<Object>() {
                @Override
                public Object get() {
                    throw new IllegalStateException(
                            "If you got here then it means that"
                                    + " your code asked for scoped object which should have been"
                                    + " explicitly seeded in this scope by calling"
                                    + " SimpleScope.seed(), but was not.");
                }
            };

    private static InvocationScope sDefaultInstance = null;

    public static InvocationScope getDefault() {
        if (sDefaultInstance == null) {
            sDefaultInstance = new InvocationScope();
        }
        return sDefaultInstance;
    }

    private final ThreadLocal<Map<Key<?>, Object>> values = new ThreadLocal<Map<Key<?>, Object>>();

    /** Start marking the scope of the Tradefed Invocation. */
    public void enter() {
        checkState(values.get() == null, "A scoping block is already in progress");
        values.set(Maps.<Key<?>, Object>newHashMap());
    }

    /** Mark the end of the scope for the Tradefed Invocation. */
    public void exit() {
        checkState(values.get() != null, "No scoping block in progress");
        values.remove();
    }

    /**
     * Interface init between Tradefed and Guice: This is the place where TF object are seeded to
     * the invocation scope to be used.
     *
     * @param config The Tradefed configuration.
     */
    public void seedConfiguration(IConfiguration config) {
        // First seed the configuration itself
        seed(IConfiguration.class, config);
        // Then inject the seeded objects to the configuration.
        injectToConfig(config);
    }

    private void injectToConfig(IConfiguration config) {
        Injector injector = Guice.createInjector(new InvocationScopeModule(this));

        // TODO: inject to TF objects that could require it.
        // Do injection against current test objects: This allows to pass the injector
        for (Object obj : config.getTests()) {
            injector.injectMembers(obj);
        }
    }

    /**
     * Seed a key/value that will be available during the TF invocation scope to be used.
     *
     * @param key the key used to represent the object.
     * @param value The actual object that will be available during the invocation.
     */
    public <T> void seed(Key<T> key, T value) {
        Map<Key<?>, Object> scopedObjects = getScopedObjectMap(key);
        checkState(
                !scopedObjects.containsKey(key),
                "A value for the key %s was "
                        + "already seeded in this scope. Old value: %s New value: %s",
                key,
                scopedObjects.get(key),
                value);
        scopedObjects.put(key, value);
    }

    /**
     * Seed a key/value that will be available during the TF invocation scope to be used.
     *
     * @param clazz the Class used to represent the object.
     * @param value The actual object that will be available during the invocation.
     */
    public <T> void seed(Class<T> clazz, T value) {
        seed(Key.get(clazz), value);
    }

    @Override
    public <T> Provider<T> scope(final Key<T> key, final Provider<T> unscoped) {
        return new Provider<T>() {
            @Override
            public T get() {
                Map<Key<?>, Object> scopedObjects = getScopedObjectMap(key);

                @SuppressWarnings("unchecked")
                T current = (T) scopedObjects.get(key);
                if (current == null && !scopedObjects.containsKey(key)) {
                    current = unscoped.get();

                    // don't remember proxies; these exist only to serve circular dependencies
                    if (Scopes.isCircularProxy(current)) {
                        return current;
                    }

                    scopedObjects.put(key, current);
                }
                return current;
            }
        };
    }

    private <T> Map<Key<?>, Object> getScopedObjectMap(Key<T> key) {
        Map<Key<?>, Object> scopedObjects = values.get();
        if (scopedObjects == null) {
            throw new OutOfScopeException("Cannot access " + key + " outside of a scoping block");
        }
        return scopedObjects;
    }

    /**
     * Returns a provider that always throws exception complaining that the object in question must
     * be seeded before it can be injected.
     *
     * @return typed provider
     */
    @SuppressWarnings({"unchecked"})
    public static <T> Provider<T> seededKeyProvider() {
        return (Provider<T>) SEEDED_KEY_PROVIDER;
    }
}
