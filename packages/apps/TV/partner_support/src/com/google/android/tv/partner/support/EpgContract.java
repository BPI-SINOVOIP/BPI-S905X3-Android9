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

import android.content.ContentUris;
import android.net.Uri;
import java.util.Arrays;
import java.util.Collections;
import java.util.List;

/**
 * The contract between the EPG provider and applications. Contains definitions for the supported
 * URIs and columns.
 *
 * <h3>Overview</h3>
 *
 * <p>
 *
 * <p>EpgContract defines a basic database of EPG data for third party inputs. The information is
 * stored in {@link Lineups} and {@link EpgInputs} tables.
 *
 * <p>
 *
 * <ul>
 *   <li>A row in the {@link Lineups} table represents information TV Lineups for postal codes.
 *   <li>A row in the {@link EpgInputs} table represents partner or 3rd party inputs with the lineup
 *       that input uses.
 * </ul>
 */
public final class EpgContract {

    /** The authority for the EPG provider. */
    public static final String AUTHORITY = "com.google.android.tv.data.epg";

    /** The delimiter between channel numbers. */
    public static final String CHANNEL_NUMBER_DELIMITER = ", ";

    /**
     * A constant of the key for intent to indicate whether cloud EPG is used.
     *
     * <p>Value type: Boolean
     */
    public static final String EXTRA_USE_CLOUD_EPG = "com.google.android.tv.extra.USE_CLOUD_EPG";

    /**
     * Returns the list of channels as a CSV string.
     *
     * <p>Any commas in the original channels are converted to periods.
     */
    public static String toChannelString(List<String> channels) {
        // TODO use a StringJoiner if we set the min SDK to N
        StringBuilder result = new StringBuilder();
        for (String channel : channels) {
            channel = channel.replace(",", ".");
            if (result.length() > 0) {
                result.append(CHANNEL_NUMBER_DELIMITER);
            }
            result.append(channel);
        }
        return result.toString();
    }

    /** Returns a list of channels. */
    public static List<String> toChannelList(String channelString) {
        return channelString == null
                ? Collections.EMPTY_LIST
                : Arrays.asList(channelString.split(CHANNEL_NUMBER_DELIMITER));
    }

    /** Column definitions for the EPG lineups table. */
    public static final class Lineups {

        /** Base content:// style URI for all lines in a postal code. */
        private static final Uri LINEUPS_IN_POSTAL_CODE_URI =
                Uri.parse("content://" + AUTHORITY + "/lineups/postal_code");

        /** The MIME type of a directory of TV channels. */
        public static final String CONTENT_TYPE = "vnd.android.cursor.dir/lineup";

        /** The MIME type of a single TV channel. */
        public static final String CONTENT_ITEM_TYPE = "vnd.android.cursor.item/lineup";

        /** The content:// style URI all lineups in a postal code. */
        public static Uri uriForPostalCode(String postalCode) {
            return LINEUPS_IN_POSTAL_CODE_URI.buildUpon().appendPath(postalCode).build();
        }

        /**
         * The line up id.
         *
         * <p>This is a opaque string that should not be parsed.
         *
         * <p>Type: TEXT
         */
        public static final String COLUMN_ID = "_ID";

        /**
         * The line up name that is displayed to the user.
         *
         * <p>Type: TEXT
         */
        public static final String COLUMN_NAME = "NAME";

        /**
         * The channel numbers that are supported by this lineup that is displayed to the user.
         *
         * <p>Comma separated value of channel numbers
         *
         * <p>Type: TEXT
         */
        public static final String COLUMN_CHANNELS = "CHANNELS";

        /**
         * The line up type.
         *
         * <p>Type: TEXT
         */
        public static final String COLUMN_TYPE = "TYPE";

        private Lineups() {}
    }

    /** Column definitions for the EPG inputs table. */
    public static final class EpgInputs {

        /** The content:// style URI for this table. */
        public static final Uri CONTENT_URI = Uri.parse("content://" + AUTHORITY + "/epg_input");

        /** The MIME type of a directory of TV channels. */
        public static final String CONTENT_TYPE = "vnd.android.cursor.dir/epg_input";

        /** The MIME type of a single TV channel. */
        public static final String CONTENT_ITEM_TYPE = "vnd.android.cursor.item/epg_input";

        /**
         * Builds a URI that points to a specific input.
         *
         * @param rowId The ID of the input to point to.
         */
        public static final Uri buildUri(long rowId) {
            return ContentUris.withAppendedId(CONTENT_URI, rowId);
        }

        /**
         * The unique ID for a row.
         *
         * <p>Type: INTEGER (long)
         */
        public static final String COLUMN_ID = "_ID";

        /**
         * The name of the package that owns the current row.
         *
         * <p>The EPG provider fills in this column with the name of the package that provides the
         * initial data of the row. If the package is later uninstalled, the rows it owns are
         * automatically removed from the tables.
         *
         * <p>Type: TEXT
         */
        public static final String COLUMN_PACKAGE_NAME = "PACKAGE_NAME";

        /**
         * The ID of the TV input service that provides this TV channel.
         *
         * <p>Use {@link android.media.tv.TvContract#buildInputId} to build the ID.
         *
         * <p>This is a required field.
         *
         * <p>Type: TEXT
         */
        public static final String COLUMN_INPUT_ID = "INPUT_ID";
        /**
         * The line up id.
         *
         * <p>This is a opaque string that should not be parsed.
         *
         * <p>This is a required field.
         *
         * <p>Type: TEXT
         */
        public static final String COLUMN_LINEUP_ID = "LINEUP_ID";

        private EpgInputs() {}
    }

    private EpgContract() {}
}
