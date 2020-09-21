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
 * Event types for {@link LogcatUpdaterEventParser}.
 */
public enum UpdaterEventType {
    UPDATE_START,
    DOWNLOAD_COMPLETE,
    PATCH_COMPLETE,
    UPDATE_VERIFIER_COMPLETE,
    D2O_COMPLETE,
    UPDATE_COMPLETE,
    // error found in logcat output
    ERROR,
    // error found in logcat output, but doesn't necessarily indicate OTA failure. Should retry.
    ERROR_FLAKY,
    // TradeFed test timed out waiting for event
    INFRA_TIMEOUT,
}
