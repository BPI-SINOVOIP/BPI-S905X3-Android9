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

package com.android.tradefed.config;

import java.io.InputStream;

/**
 * An interface for config server.
 *
 * <p>Instead of starting with a host config file, a Tradefed instnace can start with an {@link
 * IConfigurationServer}. A tradefed starts with {@link IConfigurationServer} will load current
 * host's config from the remote storage and load all dependent configs as needed. Tradefed can
 * either start with {@link IConfigurationServer} or config file, but not both.
 */
public interface IConfigurationServer {

    /**
     * Get config content by its name.
     *
     * @param name config's name
     * @return an {@link InputStream} is the config file content.
     * @throws ConfigurationException
     */
    public InputStream getConfig(String name) throws ConfigurationException;

    /**
     * Get current host's config file name for the current Tradefed session. Instead of reading the
     * host config file from local files, Tradefed start with an {@link IConfigurationServer} will
     * get the host config from the server.
     *
     * @return a host config file name.
     * @throws ConfigurationException
     */
    public String getCurrentHostConfig() throws ConfigurationException;
}
