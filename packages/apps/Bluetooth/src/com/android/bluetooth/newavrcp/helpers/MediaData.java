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

package com.android.bluetooth.avrcp;

import android.media.session.PlaybackState;

import java.util.List;
import java.util.Objects;

/*
 * Helper class to transport metadata around AVRCP
 */
class MediaData {
    public List<Metadata> queue;
    public PlaybackState state;
    public Metadata metadata;

    MediaData(Metadata m, PlaybackState s, List<Metadata> q) {
        metadata = m;
        state = s;
        queue = q;
    }

    @Override
    public boolean equals(Object o) {
        if (o == null) return false;
        if (!(o instanceof MediaData)) return false;

        final MediaData u = (MediaData) o;

        if (!MediaPlayerWrapper.playstateEquals(state, u.state)) {
            return false;
        }

        if (!Objects.equals(metadata, u.metadata)) {
            return false;
        }

        if (!Objects.equals(queue, u.queue)) {
            return false;
        }

        return true;
    }
}
