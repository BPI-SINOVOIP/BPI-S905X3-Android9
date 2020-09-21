/*
 * Copyright 2018 The Android Open Source Project
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

package com.android.bluetooth.avrcp;

import android.media.session.PlaybackState;

/**
 * Carries the playback status information in a custom object.
 */
// TODO(apanicke): Send the current active song ID along with this object so that all information
// is carried by our custom types.
class PlayStatus {
    static final byte STOPPED = 0;
    static final byte PLAYING = 1;
    static final byte PAUSED = 2;
    static final byte FWD_SEEK = 3;
    static final byte REV_SEEK = 4;
    static final byte ERROR = -1;

    public long position = 0xFFFFFFFFFFFFFFFFL;
    public long duration = 0x00L;
    public byte state = STOPPED;

    // Duration info isn't contained in the PlaybackState so the service must supply it.
    static PlayStatus fromPlaybackState(PlaybackState state, long duration) {
        PlayStatus ret = new PlayStatus();
        if (state == null) return ret;

        ret.state = playbackStateToAvrcpState(state.getState());
        ret.position = state.getPosition();
        ret.duration = duration;
        return ret;
    }

    static byte playbackStateToAvrcpState(int playbackState) {
        switch (playbackState) {
            case PlaybackState.STATE_STOPPED:
            case PlaybackState.STATE_NONE:
            case PlaybackState.STATE_CONNECTING:
                return PlayStatus.STOPPED;

            case PlaybackState.STATE_BUFFERING:
            case PlaybackState.STATE_PLAYING:
                return PlayStatus.PLAYING;

            case PlaybackState.STATE_PAUSED:
                return PlayStatus.PAUSED;

            case PlaybackState.STATE_FAST_FORWARDING:
            case PlaybackState.STATE_SKIPPING_TO_NEXT:
            case PlaybackState.STATE_SKIPPING_TO_QUEUE_ITEM:
                return PlayStatus.FWD_SEEK;

            case PlaybackState.STATE_REWINDING:
            case PlaybackState.STATE_SKIPPING_TO_PREVIOUS:
                return PlayStatus.REV_SEEK;

            case PlaybackState.STATE_ERROR:
            default:
                return PlayStatus.ERROR;
        }
    }

    @Override
    public String toString() {
        return "{ state=" + state + " position=" + position + " duration=" + duration + " }";
    }
}
