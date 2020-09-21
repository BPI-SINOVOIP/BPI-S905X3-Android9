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
package com.android.loganalysis.util.config;

import java.lang.annotation.ElementType;
import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import java.lang.annotation.Target;

/**
 * Annotates a class as representing a {@link IConfiguration} object.
 */
//TODO: Use libTF once this is copied over.
@Retention(RetentionPolicy.RUNTIME)
@Target(ElementType.TYPE)
public @interface OptionClass {

    /**
     * An optional descriptive alias for this configuration object.
     * <p/>
     * This alias will currently be used for two purposes:
     * <ul>
     * <li>displayed in help output, to help classify options</li>
     * <li>
     * can be used to namespace {@link Option} command line arguments, in cases where a given
     * {@link Option#name()} is not unique amongst configuration objects. To provide a namespace
     * with an {@link Option} command line argument, use this format: <p/>
     * '--[OptionClass alias]:[Option name]'.
     * </li>
     * </ul>
     */
    String alias();

    /**
     * Whether or not to add this {@link Option} to the global Option namespace.
     * <p />
     * If <code>true</code> (the default), then it will be possible to specify this option simply
     * by its name -- <code>--[Option name]</code>.  If <code>false</code>, then the alias or
     * another specific namespace (such as the full classname) must be specified in order to use
     * {@link Option}s for this class --
     * <code>--[OptionClass alias]:[Option name]</code> will work, but <code>--[Option name]</code>
     * won't resolve to the {@link Option}.
     * <p />
     * FIXME: update documentation methods to distinguish classes/fields that aren't in the global
     * FIXME: namespace
     */
    boolean global_namespace() default true;
}
