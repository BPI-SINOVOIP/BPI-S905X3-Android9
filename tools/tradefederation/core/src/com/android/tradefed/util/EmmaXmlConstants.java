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
package com.android.tradefed.util;

/**
 * Constants used when parsing emma xml report.
 */
public class EmmaXmlConstants {
    public static final String PACKAGE_TAG = "package";
    public static final String CLASS_TAG = "class";
    public static final String METHOD_TAG = "method";
    public static final String BLOCK_TAG = "block";
    public static final String LINE_TAG = "line";
    public static final String NAME_ATTR = "name";
    public static final String COVERAGE_TAG = "coverage";
}
