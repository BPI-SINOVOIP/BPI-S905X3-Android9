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
 * limitations under the License
 */

package com.google.android.tv.partner.support;

import android.content.ContentValues;
import android.support.annotation.Nullable;
import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

/** Value class for {@link com.google.android.tv.partner.support.EpgContract.Lineups} */
// TODO(b/72052568): Get autovalue to work in aosp master
public abstract class Lineup {
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

    public static Lineup createLineup(String id, int type, String name, List<String> channels) {
        return new AutoValue_Lineup(
                id, type, name, Collections.unmodifiableList(new ArrayList<>(channels)));
    }

    public static Lineup createLineup(ContentValues contentValues) {
        String id = contentValues.getAsString(EpgContract.Lineups.COLUMN_ID);
        int type = contentValues.getAsInteger(EpgContract.Lineups.COLUMN_TYPE);
        String name = contentValues.getAsString(EpgContract.Lineups.COLUMN_NAME);
        String channels = contentValues.getAsString(EpgContract.Lineups.COLUMN_CHANNELS);
        List<String> channelList = EpgContract.toChannelList(channels);
        return new AutoValue_Lineup(id, type, name, Collections.unmodifiableList(channelList));
    }

    /** The ID of this lineup. */
    public abstract String getId();

    /** The type associated with this lineup. */
    public abstract int getType();

    /** The human readable name associated with this lineup. */
    @Nullable
    public abstract String getName();

    /** An unmodifiable list of channel numbers that this lineup has. */
    public abstract List<String> getChannels();
}
