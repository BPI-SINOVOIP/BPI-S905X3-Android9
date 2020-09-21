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

package com.android.tradefed.targetprep;

/**
 * An enum to define alternative directory behaviors for various test artifact installers/pushers
 * <p>
 * @see TestAppInstallSetup
 * @see TestFilePushSetup
 */
public enum AltDirBehavior {
    /**
     * The alternative directories should be used as a fallback to look up the build artifacts. That
     * is, if build artifacts cannot be found at the regularly configured location.
     */
    FALLBACK,

    /**
     * The alternative directories should be used as an override to look up the build artifacts.
     * That is, alternative directories should be searched first for build artifacts.
     */
    OVERRIDE,
}
