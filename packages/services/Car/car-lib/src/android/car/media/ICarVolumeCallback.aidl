/*
 * Copyright (C) 2018 The Android Open Source Project
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

package android.car.media;

/**
 * Binder interface to callback the volume key events.
 *
 * @hide
 */
oneway interface ICarVolumeCallback {
    /**
     * This is called whenever a group volume is changed.
     * The changed-to volume index is not included, the caller is encouraged to
     * get the current group volume index via CarAudioManager.
     */
    void onGroupVolumeChanged(int groupId, int flags);

    /**
     * This is called whenever the master mute state is changed.
     * The changed-to master mute state is not included, the caller is encouraged to
     * get the current master mute state via AudioManager.
     */
    void onMasterMuteChanged(int flags);
}
