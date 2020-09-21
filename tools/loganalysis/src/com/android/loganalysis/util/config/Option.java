/*
 * Copyright (C) 2010 The Android Open Source Project
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

package com.android.loganalysis.util.config;

import java.lang.annotation.ElementType;
import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import java.lang.annotation.Target;
import java.util.Collection;
import java.util.Map;

/**
 * Annotates a field as representing a {@link IConfiguration} option.
 */
//TODO: Use libTF once this is copied over.
@Retention(RetentionPolicy.RUNTIME)
@Target(ElementType.FIELD)
public @interface Option {

    static final char NO_SHORT_NAME = '0';

    public enum Importance {
        /** the option should never be treated as important */
        NEVER,
        /** the option should be treated as important only if it has no value */
        IF_UNSET,
        /** the option should always be treated as important */
        ALWAYS;
    }

    /**
     * The mandatory unique name for this option.
     * <p/>
     * This will map to a command line argument prefixed with two '-' characters.
     * For example, an {@link Option} with name 'help' would be specified with '--help' on the
     * command line.
     * <p/>
     * Names may not contain a colon eg ':'.
     */
    String name();

    /**
     * Optional abbreviated name for option.
     * This will map to a command line argument prefixed with a single '-'.
     * e.g. "-h" where h = shortName.
     *
     * '0' is reserved to mean the option has no shortName.
     **/
    char shortName() default NO_SHORT_NAME;

    /**
     * User friendly description of the option.
     */
    String description() default "";

    /**
     * The importance of the option.
     * <p/>
     * An option deemed 'important' will be displayed in the abbreviated help output. Help for an
     * unimportant option will only be displayed in the full help text.
     */
    Importance importance() default Importance.NEVER;

    /**
     * Whether the option is mandatory or optional.
     * <p />
     * The configuration framework will throw a {@code ConfigurationException} if either of the
     * following is true of a mandatory field after options have been parsed from all sources:
     * <ul>
     *   <li>The field is {@code null}.</li>
     *   <li>The field is an empty {@link java.util.Collection}.</li>
     * </ul>
     */
     boolean mandatory() default false;

    /**
     * Controls the behavior when an option is specified multiple times.  Note that this rule is
     * ignored completely for options that are {@link Collection}s or {@link Map}s.
     */
    OptionUpdateRule updateRule() default OptionUpdateRule.LAST;
}
