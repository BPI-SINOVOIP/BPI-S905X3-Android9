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

import java.util.Objects;

class Metadata implements Cloneable {
    public String mediaId;
    public String title;
    public String artist;
    public String album;
    public String trackNum;
    public String numTracks;
    public String genre;
    public String duration;

    @Override
    public Metadata clone() {
        Metadata data = new Metadata();
        data.mediaId = mediaId;
        data.title = title;
        data.artist = artist;
        data.album = album;
        data.trackNum = trackNum;
        data.numTracks = numTracks;
        data.genre = genre;
        data.duration = duration;
        return data;
    }

    @Override
    public boolean equals(Object o) {
        if (o == null) return false;
        if (!(o instanceof Metadata)) return false;

        final Metadata m = (Metadata) o;
        if (!Objects.equals(title, m.title)) return false;
        if (!Objects.equals(artist, m.artist)) return false;
        if (!Objects.equals(album, m.album)) return false;
        return true;
    }

    @Override
    public String toString() {
        return "{ mediaId=\"" + mediaId + "\" title=\"" + title + "\" artist=\"" + artist
                + "\" album=\"" + album + "\" duration=" + duration
                + " trackPosition=" + trackNum + "/" + numTracks + " }";

    }
}
