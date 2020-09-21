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
package com.android.tradefed.config;

/**
 * Class extending {@link ConfigurationException} for template related error during configuration
 * parsing. Allows this error to be easily detected.
 */
public class TemplateResolutionError extends ConfigurationException {

    private static final long serialVersionUID = 7742154438569011969L;

    private String mTemplateKey = null;
    private String mConfigName = null;

    /**
     * Creates a {@link TemplateResolutionError}.
     *
     * @param configName the configuration name involved
     * @param templateName the key name that is used to replace the template.
     */
    public TemplateResolutionError(String configName, String templateName) {
        super(String.format(
                "Failed to parse config xml '%s'. Reason: " +
                "Couldn't resolve template-include named " +
                "'%s': No 'default' attribute and no matching manual resolution. " +
                "Try using argument --template:map %s (config path)",
                configName, templateName, templateName));
        mTemplateKey = templateName;
        mConfigName = configName;
    }

    /**
     * Return the template key that is used to replace the template.
     */
    public String getTemplateKey() {
        return mTemplateKey;
    }

    /**
     * Return the config name that raised the exception in the first place.
     */
    public String getConfigName() {
        return mConfigName;
    }
}
