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

package com.android.tradefed.log;

/**
 * An interface to handle terrible failures from
 * {@link com.android.tradefed.log.LogUtil.CLog#wtf(String, Throwable)}
 * <br>
 * @see TerribleFailureEmailHandler
 */
public interface ITerribleFailureHandler {

    /**
     * Triggered when a terrible failure occurs in
     * {@link com.android.tradefed.log.LogUtil.CLog#wtf(String, Throwable)}
     *
     * @param description a summary of the terrible failure that occurred
     * @param cause (Optional) contains the stack trace of the terrible failure
     * @return true on being handled successfully, false otherwise
     */
    public boolean onTerribleFailure(String description, Throwable cause);
}
