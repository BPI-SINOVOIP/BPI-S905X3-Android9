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

package com.android.ide.eclipse.ddms;

/**
 * Classes which implement this interface provides the location of various SDK tools.
 */
public interface IToolsLocator {

    /**
     * Queries the location of ADB
     * @return A full OS path to the location of adb.
     */
    String getAdbLocation();

    /**
     * Queries the location of Traceview
     * @return A full OS path to the location of traceview
     */
    String getTraceViewLocation();

    /**
     * Queries the location of hprof-conv
     * @return A full OS path to the location of hprof-conv.
     */
    String getHprofConvLocation();
}
