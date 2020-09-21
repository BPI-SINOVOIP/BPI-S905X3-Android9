/*
 * Copyright (C) 2015 The Android Open Source Project
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

import com.android.tradefed.util.MultiMap;

/**
 * A simple class to accept settings for the ConfigurationXmlParser
 * <p />
 * To pass settings to this class, the alias is mandatory.  So something like
 * {@code --template:map name filename.xml} will work, but this would NOT work:
 * {@code --map name filename.xml}.
 */
@OptionClass(alias = "template", global_namespace = false)
class ConfigurationXmlParserSettings {
    @Option(
        name = "map",
        description =
                "Map the <template-include> tag with the specified "
                        + "name to the specified actual configuration file.  Configuration file "
                        + "resolution will happen as with a standard <include> tag."
    )
    public MultiMap<String, String> templateMap = new MultiMap<>();
}
