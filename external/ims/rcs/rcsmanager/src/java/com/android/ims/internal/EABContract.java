/*
 * Copyright (c) 2015, Motorola Mobility LLC
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     - Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     - Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     - Neither the name of Motorola Mobility nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL MOTOROLA MOBILITY LLC BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 */

package com.android.ims.internal;

import android.net.Uri;
import android.provider.BaseColumns;

/**
 * <p>
 * The contract between the EAB provider and applications. Contains
 * definitions for the supported URIs and data columns.
 * </p>
 * <h3>Overview</h3>
 * <p>
 * EABContract defines the data model of EAB related information.
 * This data is stored in a table EABPresence.
 * </p>
 *
 * @hide
 *
 */
public final class EABContract {
    /**
     * This authority is used for writing to or querying from the EAB provider.
     */
    public static final String AUTHORITY = "com.android.rcs.eab";

    /**
     * The content:// style URL for the top-level EAB authority
     */
    public static final Uri CONTENT_URI = Uri.parse("content://" + AUTHORITY);

    /**
     *  Class for EABProviderItem
     */
    public static class EABColumns implements BaseColumns {
        public static final String TABLE_NAME = "EABPresence";
        public static final String GROUPITEMS_NAME = "EABGroupDetails";

        /**
         * CONTENT_URI
         * <P>
         * "content://com.android.vt.eab/EABPresence"
         * </P>
         */
        public static final Uri CONTENT_URI = Uri.withAppendedPath(EABContract.CONTENT_URI,
                TABLE_NAME);

        public static final String CONTENT_TYPE =
                "vnd.android.cursor.dir/vnd.android.rcs.eab.provider.eabprovider";

        public static final String CONTENT_ITEM_TYPE =
                "vnd.android.cursor.item/vnd.android.rcs.eab.provider.eabprovider";

        /**
         * Key defining the contact number.
         * <P>
         * Type: TEXT
         * </P>
         */
        public static final String CONTACT_NUMBER = "contact_number";

        /**
         * Key defining the contact name.
         * <P>
         * Type: TEXT
         * </P>
         */
        public static final String CONTACT_NAME = "contact_name";

        /**
         * Key defining the reference to ContactContract raw_contact_id of the number.
         * <P>
         * Type: TEXT
         * </P>
         */
        public static final String RAW_CONTACT_ID = "raw_contact_id";

        /**
         * Key defining the reference to ContactContract contact_id of the number.
         * <P>
         * Type: TEXT
         * </P>
         */
        public static final String CONTACT_ID = "contact_id";

        /**
         * Key defining the reference to ContactContract data_id of the number.
         * <P>
         * Type: TEXT
         * </P>
         */
        public static final String DATA_ID = "data_id";

        /**
         * Key defining the account type.
         * <P>
         * Type: TEXT
         * </P>
         */
        public static final String ACCOUNT_TYPE = "account_type";

        /**
         * Key defining the VoLTE call service contact address.
         * <P>
         * Type: TEXT
         * </P>
         */
        public static final String VOLTE_CALL_SERVICE_CONTACT_ADDRESS = "volte_contact_address";

        /**
         * Key defining the VoLTE call capability.
         * <P>
         * Type: TEXT
         * </P>
         */
        public static final String VOLTE_CALL_CAPABILITY = "volte_call_capability";

        /**
         * Key defining the VoLTE call capability timestamp.
         * <P>
         * Type: TEXT
         * </P>
         */
        public static final String VOLTE_CALL_CAPABILITY_TIMESTAMP = "volte_capability_timestamp";

        /**
         * Key defining the VoLTE call availability.
         * <P>
         * Type: LONG
         * </P>
         */
        public static final String VOLTE_CALL_AVAILABILITY = "volte_call_avalibility";

        /**
         * Key defining the VoLTE call availability timestamp.
         * <P>
         * Type: TEXT
         * </P>
         */
        public static final String VOLTE_CALL_AVAILABILITY_TIMESTAMP =
                "volte_availability_timestamp";

        /**
         * Key defining the Video call service contact address.
         * <P>
         * Type: LONG
         * </P>
         */
        public static final String VIDEO_CALL_SERVICE_CONTACT_ADDRESS = "video_contact_address";

        /**
         * Key defining the Video call capability.
         * <P>
         * Type: TEXT
         * </P>
         */
        public static final String VIDEO_CALL_CAPABILITY = "video_call_capability";

        /**
         * Key defining the Video call capability timestamp.
         * <P>
         * Type: TEXT
         * </P>
         */
        public static final String VIDEO_CALL_CAPABILITY_TIMESTAMP = "video_capability_timestamp";

        /**
         * Key defining the Video call availability.
         * <P>
         * Type: LONG
         * </P>
         */
        public static final String VIDEO_CALL_AVAILABILITY = "video_call_availability";

        /**
         * Key defining the Video call availability timestamp.
         * <P>
         * Type: TEXT
         * </P>
         */
        public static final String VIDEO_CALL_AVAILABILITY_TIMESTAMP =
                "video_availability_timestamp";

        /**
         * Key defining the Video call availability timestamp.
         * <P>
         * Type: LONG
         * </P>
         */
        public static final String CONTACT_LAST_UPDATED_TIMESTAMP =
                "contact_last_updated_timestamp";

        /**
         * Key defining the volte status.
         * <p>
         * Type: INT
         * </p>
         */
        public static final String VOLTE_STATUS = "volte_status";

        /**
         * Key defining the formatted number.
         * <p>
         * Type: TEXT
         * </p>
         */
        public static final String FORMATTED_NUMBER = "formatted_number";

        /**
         * @hide
         */
        public static final String[] PROJECTION = new String[] {
            _ID,
            CONTACT_NAME,
            CONTACT_NUMBER,
            RAW_CONTACT_ID,
            CONTACT_ID,
            DATA_ID,
            ACCOUNT_TYPE,
            VOLTE_CALL_SERVICE_CONTACT_ADDRESS,
            VOLTE_CALL_CAPABILITY,
            VOLTE_CALL_AVAILABILITY_TIMESTAMP,
            VOLTE_CALL_AVAILABILITY,
            VOLTE_CALL_CAPABILITY_TIMESTAMP,
            VIDEO_CALL_SERVICE_CONTACT_ADDRESS,
            VIDEO_CALL_CAPABILITY,
            VIDEO_CALL_CAPABILITY_TIMESTAMP,
            VIDEO_CALL_AVAILABILITY,
            VIDEO_CALL_AVAILABILITY_TIMESTAMP,
            CONTACT_LAST_UPDATED_TIMESTAMP,
            VOLTE_STATUS
        };
    }
}

