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
 * Simple interface to represent object that accepts an {@link IConfiguration}.
 * <p/>
 * Tests or other configuration objects should implement this interface if they need access to a
 * {@link IConfiguration} they are included in.
 */
public interface IConfigurationReceiver {

    // TODO: look into using @Options to implement this behavior instead

    /**
     * Injects the {@link IConfiguration} in use.
     */
    public void setConfiguration(IConfiguration configuration);
}
