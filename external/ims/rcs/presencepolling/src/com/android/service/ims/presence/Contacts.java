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

package com.android.service.ims.presence;

import android.net.Uri;
import android.provider.BaseColumns;
import android.text.format.Time;
import com.android.ims.internal.EABContract;

import com.android.ims.internal.ContactNumberUtils;
import com.android.ims.internal.Logger;

public final class Contacts {
    private Contacts() {}

    /**
     * Intent that a new contact is inserted in EAB Provider.
     * This intent will have a extra parameter with key NEW_PHONE_NUMBER.
     */
    public static final String ACTION_NEW_CONTACT_INSERTED =
            "android.provider.rcs.eab.EAB_NEW_CONTACT_INSERTED";

    /**
     * Intent that EAB database is reset.
     */
    public static final String ACTION_EAB_DATABASE_RESET =
            "android.provider.rcs.eab.EAB_DATABASE_RESET";

    /**
     * Key to bundle the new phone number inserted in EAB Provider.
     */
    public static final String NEW_PHONE_NUMBER = "newPhoneNumber";


    public static final String AUTHORITY = EABContract.AUTHORITY;

    public static final Uri CONTENT_URI = Uri.parse("content://" + AUTHORITY);

    public static final class Impl implements BaseColumns {
        private Impl() {}

        public static final String TABLE_NAME =
                EABContract.EABColumns.TABLE_NAME;

        /**
         * CONTENT_URI
         * <P>
         * "content://com.android.vt.eab/EABPresence"
         * </P>
         */
        public static final Uri CONTENT_URI =
                Uri.withAppendedPath(Contacts.CONTENT_URI, TABLE_NAME);

        /**
         * Key defining the contact number.
         * <P>
         * Type: TEXT
         * </P>
         */
        public static final String FORMATTED_NUMBER =
                EABContract.EABColumns.FORMATTED_NUMBER;

        /**
         * Key defining the contact number.
         * <P>
         * Type: TEXT
         * </P>
         */
        public static final String CONTACT_NUMBER =
                EABContract.EABColumns.CONTACT_NUMBER;

        /**
         * Key defining the contact name.
         * <P>
         * Type: TEXT
         * </P>
         */
        public static final String CONTACT_NAME =
                EABContract.EABColumns.CONTACT_NAME;

        /**
         * <p>
         * Type: INT
         * </p>
         */
        public static final String VOLTE_STATUS =
                EABContract.EABColumns.VOLTE_STATUS;

        /**
         * Key defining the last updated timestamp.
         * <P>
         * Type: LONG
         * </P>
         */
        public static final String CONTACT_LAST_UPDATED_TIMESTAMP =
                EABContract.EABColumns.CONTACT_LAST_UPDATED_TIMESTAMP;

        /**
         * Key defining the VoLTE call service contact address.
         * <P>
         * Type: TEXT
         * </P>
         */
        public static final String VOLTE_CALL_SERVICE_CONTACT_ADDRESS =
                EABContract.EABColumns.VOLTE_CALL_SERVICE_CONTACT_ADDRESS;

        /**
         * Key defining the VoLTE call capability.
         * <P>
         * Type: TEXT
         * </P>
         */
        public static final String VOLTE_CALL_CAPABILITY =
                EABContract.EABColumns.VOLTE_CALL_CAPABILITY;

        /**
         * Key defining the VoLTE call capability timestamp.
         * <P>
         * Type: LONG
         * </P>
         */
        public static final String VOLTE_CALL_CAPABILITY_TIMESTAMP =
                EABContract.EABColumns.VOLTE_CALL_CAPABILITY_TIMESTAMP;

        /**
         * Key defining the VoLTE call availability.
         * <P>
         * Type: TEXT
         * </P>
         */
        public static final String VOLTE_CALL_AVAILABILITY =
                EABContract.EABColumns.VOLTE_CALL_AVAILABILITY;

        /**
         * Key defining the VoLTE call availability timestamp.
         * <P>
         * Type: LONG
         * </P>
         */
        public static final String VOLTE_CALL_AVAILABILITY_TIMESTAMP =
                EABContract.EABColumns.VOLTE_CALL_AVAILABILITY_TIMESTAMP;

        /**
         * Key defining the Video call service contact address.
         * <P>
         * Type: TEXT
         * </P>
         */
        public static final String VIDEO_CALL_SERVICE_CONTACT_ADDRESS =
                EABContract.EABColumns.VIDEO_CALL_SERVICE_CONTACT_ADDRESS;

        /**
         * Key defining the Video call capability.
         * <P>
         * Type: TEXT
         * </P>
         */
        public static final String VIDEO_CALL_CAPABILITY =
                EABContract.EABColumns.VIDEO_CALL_CAPABILITY;

        /**
         * Key defining the Video call capability timestamp.
         * <P>
         * Type: LONG
         * </P>
         */
        public static final String VIDEO_CALL_CAPABILITY_TIMESTAMP =
                EABContract.EABColumns.VIDEO_CALL_CAPABILITY_TIMESTAMP;

        /**
         * Key defining the Video call availability.
         * <P>
         * Type: TEXT
         * </P>
         */
        public static final String VIDEO_CALL_AVAILABILITY =
                EABContract.EABColumns.VIDEO_CALL_AVAILABILITY;

        /**
         * Key defining the Video call availability timestamp.
         * <P>
         * Type: LONG
         * </P>
         */
        public static final String VIDEO_CALL_AVAILABILITY_TIMESTAMP =
                EABContract.EABColumns.VIDEO_CALL_AVAILABILITY_TIMESTAMP;
    }

    public static class Item {
        public Item(long id) {
            mId = id;
        }

        public long id() {
            return mId;
        }

        public String number() {
            return mNumber;
        }

        public void setNumber(String number) {
            mNumber = ContactNumberUtils.getDefault().format(number);
        }

        public boolean isValid() {
            int res = ContactNumberUtils.getDefault().validate(mNumber);
            return (res == ContactNumberUtils.NUMBER_VALID);
        }

        public String name() {
            return mName;
        }

        public void setName(String name) {
            mName = name;
        }

        public long lastUpdateTime() {
            return mLastUpdateTime;
        }

        public void setLastUpdateTime(long time) {
            mLastUpdateTime = time;
        }

        public long volteTimestamp() {
            return mVolteTimeStamp;
        }

        public void setVolteTimestamp(long time) {
            mVolteTimeStamp = time;
        }

        public long videoTimestamp() {
            return mVideoTimeStamp;
        }

        public void setVideoTimestamp(long time) {
            mVideoTimeStamp = time;
        }

        @Override
        public boolean equals(Object o) {
            if (this == o) return true;
            if (!(o instanceof Contacts.Item)) return false;

            Contacts.Item that = (Contacts.Item) o;
            return this.number().equalsIgnoreCase(that.number());
        }

        @Override
        public String toString() {
            return new StringBuilder(256)
                .append("Contacts.Item { ")
                .append("\nId: " + mId)
                .append("\nNumber: " + Logger.hidePhoneNumberPii(mNumber))
                .append("\nLast update time: " + mLastUpdateTime + "(" +
                        getTimeString(mLastUpdateTime) + ")")
                .append("\nVolte capability timestamp: " + mVolteTimeStamp + "(" +
                        getTimeString(mVolteTimeStamp) + ")")
                .append("\nVideo capability timestamp: " + mVideoTimeStamp + "(" +
                        getTimeString(mVideoTimeStamp) + ")")
                .append("\nisValid: " + isValid())
                .append(" }")
                .toString();
        }

        private String getTimeString(long time) {
            if (time <= 0) {
                time = System.currentTimeMillis();
            }

            Time tobj = new Time();
            tobj.set(time);
            return String.format("%s.%s", tobj.format("%m-%d %H:%M:%S"), time % 1000);
        }

        private long mId;
        private String mNumber;
        private String mName;
        private long mLastUpdateTime;
        private long mVolteTimeStamp;
        private long mVideoTimeStamp;
    }
}
