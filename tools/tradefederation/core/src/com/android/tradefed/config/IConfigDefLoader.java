/*
 * Copyright (C) 2011 The Android Open Source Project
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

import java.util.Map;

/**
 * Interface for retrieving a ConfigurationDef.
 */
interface IConfigDefLoader {

    /**
     * Retrieve the {@link ConfigurationDef} for the given name
     *
     * @param name
     * @param templateMap map of template-include names to configuration filenames
     * @return {@link ConfigurationDef}
     * @throws ConfigurationException if an error occurred loading the config
     */
    ConfigurationDef getConfigurationDef(String name, Map<String, String> templateMap)
            throws ConfigurationException;

    boolean isGlobalConfig();

    /**
     * Load a config's data into the given {@link ConfigurationDef}
     *
     * @param def the {@link ConfigurationDef} to load the data into
     * @param parentName the name of the parent config
     * @param name the name of config to include
     * @param deviceTagObject the name of the current deviceTag or null if not inside a device tag.
     * @param templateMap the current map of template to be loaded.
     * @throws ConfigurationException if an error occurred loading the config
     */
    void loadIncludedConfiguration(
            ConfigurationDef def,
            String parentName,
            String name,
            String deviceTagObject,
            Map<String, String> templateMap)
            throws ConfigurationException;
}
