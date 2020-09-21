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

package com.android.tv.data;

import android.support.annotation.IntDef;
import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import java.util.Collections;
import java.util.List;

/** A class that represents a lineup. */
public class Lineup {
    /** The ID of this lineup. */
    public String getId() {
        return id;
    }

    /** The type associated with this lineup. */
    public int getType() {
        return type;
    }

    /** The human readable name associated with this lineup. */
    public String getName() {
        return name;
    }

    /** The human readable name associated with this lineup. */
    public String getLocation() {
        return location;
    }

    /** An unmodifiable list of channel numbers that this lineup has. */
    public List<String> getChannels() {
        return channels;
    }

    private final String id;

    private final int type;

    private final String name;

    private final String location;

    private final List<String> channels;

    @Retention(RetentionPolicy.SOURCE)
    @IntDef({
        LINEUP_CABLE,
        LINEUP_SATELLITE,
        LINEUP_BROADCAST_DIGITAL,
        LINEUP_BROADCAST_ANALOG,
        LINEUP_IPTV,
        LINEUP_MVPD,
        LINEUP_INTERNET,
        LINEUP_OTHER
    })
    public @interface LineupType {}

    /** Lineup type for cable. */
    public static final int LINEUP_CABLE = 0;

    /** Lineup type for satelite. */
    public static final int LINEUP_SATELLITE = 1;

    /** Lineup type for broadcast digital. */
    public static final int LINEUP_BROADCAST_DIGITAL = 2;

    /** Lineup type for broadcast analog. */
    public static final int LINEUP_BROADCAST_ANALOG = 3;

    /** Lineup type for IPTV. */
    public static final int LINEUP_IPTV = 4;

    /**
     * Indicates the lineup is either satellite, cable or IPTV but we are not sure which specific
     * type.
     */
    public static final int LINEUP_MVPD = 5;

    /** Lineup type for Internet. */
    public static final int LINEUP_INTERNET = 6;

    /** Lineup type for other. */
    public static final int LINEUP_OTHER = 7;

    /** Creates a lineup. */
    public Lineup(String id, int type, String name, String location, List<String> channels) {
        this.id = id;
        this.type = type;
        this.name = name;
        this.location = location;
        this.channels = Collections.unmodifiableList(channels);
    }
}
