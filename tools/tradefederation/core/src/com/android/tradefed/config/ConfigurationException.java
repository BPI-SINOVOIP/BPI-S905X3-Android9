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
package com.android.tradefed.config;

/**
 * Thrown if configuration could not be loaded.
 */
public class ConfigurationException extends Exception {
    private static final long serialVersionUID = 7742154448569011969L;

    /**
     * Creates a {@link ConfigurationException}.
     *
     * @param msg a meaningful error message
     */
    public ConfigurationException(String msg) {
        super(msg);
    }

    /**
     * Creates a {@link ConfigurationException}.
     *
     * @param msg a meaningful error message
     * @param cause the {@link Throwable} that represents the original cause of the error
     */
    public ConfigurationException(String msg, Throwable cause) {
        super(msg, cause);
    }
}

