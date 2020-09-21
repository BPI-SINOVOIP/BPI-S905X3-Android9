/*
 * Copyright (C) 2016 The Android Open Source Project
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

package com.android.tradefed.util.keystore;

/**
 * Interface to access a key store for password or sensitive data.
 */
public interface IKeyStoreClient {

    /**
     * A method to check whether or not we have a valid key store to talk to.
     *
     * @return true if we have a valid key store, false otherwise.
     */
    public boolean isAvailable();

    /**
     * A method to check if the key store contains a given key.
     *
     * @param key to check existence for.
     * @return true if the given key exists.
     */
    public boolean containsKey(String key);

    /**
     * A method to fetch a given key inside the key store.
     *
     * @param key to fetch inside the key store.
     * @return the {@link String} value of the key. It will return null if key
     *         is not found.
     */
    public String fetchKey(String key);

}
