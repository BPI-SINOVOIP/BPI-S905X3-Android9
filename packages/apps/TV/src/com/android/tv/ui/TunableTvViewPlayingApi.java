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
 * limitations under the License
 */

package com.android.tv.ui;

/** API to play pause and set the volume of a TunableTvView */
public interface TunableTvViewPlayingApi {

    boolean isPlaying();

    void setStreamVolume(float volume);

    void setTimeShiftListener(TimeShiftListener listener);

    boolean isTimeShiftAvailable();

    void timeshiftPlay();

    void timeshiftPause();

    void timeshiftRewind(int speed);

    void timeshiftFastForward(int speed);

    void timeshiftSeekTo(long timeMs);

    long timeshiftGetCurrentPositionMs();

    /** Used to receive the time-shift events. */
    abstract class TimeShiftListener {
        /**
         * Called when the availability of the time-shift for the current channel has been changed.
         * It should be guaranteed that this is called only when the availability is really changed.
         */
        public abstract void onAvailabilityChanged();

        /**
         * Called when the record start time has been changed. This is not called when the recorded
         * programs is played.
         */
        public abstract void onRecordStartTimeChanged(long recordStartTimeMs);
    }
}
