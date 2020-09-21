/*
 * Copyright (C) 2017 The Android Open Source Project
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

package com.android.tv.common;

import android.content.Context;
import android.content.Intent;
import com.android.tv.common.config.api.RemoteConfig.HasRemoteConfig;
import com.android.tv.common.recording.RecordingStorageStatusManager;
import com.android.tv.common.util.Clock;

/** Injection point for the base app */
public interface BaseSingletons extends HasRemoteConfig {

    Clock getClock();

    RecordingStorageStatusManager getRecordingStorageStatusManager();

    Intent getTunerSetupIntent(Context context);

    String getEmbeddedTunerInputId();
}
