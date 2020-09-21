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

import android.content.Context;
import android.content.ContentResolver;
import android.content.ContentUris;
import android.content.ContentValues;
import android.content.Context;
import android.database.Cursor;
import android.database.CursorWrapper;
import android.database.Cursor;
import android.database.DatabaseUtils;
import android.net.Uri;
import android.text.format.Time;
import android.text.TextUtils;

import com.android.ims.internal.EABContract;
import com.android.ims.internal.ContactNumberUtils;
import com.android.ims.RcsPresenceInfo;
import com.android.ims.internal.Logger;

import java.util.ArrayList;
import java.util.List;

public class EABContactManager {
    private Logger logger = Logger.getLogger(this.getClass().getName());

    /**
     * An identifier for a particular EAB contact number, unique across the system.
     * Clients use this ID to make subsequent calls related to the contact.
     */
    public final static String COLUMN_ID = Contacts.Impl._ID;

    /**
     * Timestamp when the presence was last updated, in {@link System#currentTimeMillis
     * System.currentTimeMillis()} (wall clock time in UTC).
     */
    public final static String COLUMN_LAST_UPDATED_TIMESTAMP =
            Contacts.Impl.CONTACT_LAST_UPDATED_TIMESTAMP;

    /**
     * columns to request from EABProvider.
     * @hide
     */
    public static final String[] CONTACT_COLUMNS = new String[] {
        Contacts.Impl._ID,
        Contacts.Impl.CONTACT_NUMBER,
        Contacts.Impl.CONTACT_NAME,
        Contacts.Impl.CONTACT_LAST_UPDATED_TIMESTAMP,
        Contacts.Impl.VOLTE_CALL_SERVICE_CONTACT_ADDRESS,
        Contacts.Impl.VOLTE_CALL_CAPABILITY,
        Contacts.Impl.VOLTE_CALL_CAPABILITY_TIMESTAMP,
        Contacts.Impl.VOLTE_CALL_AVAILABILITY,
        Contacts.Impl.VOLTE_CALL_AVAILABILITY_TIMESTAMP,
        Contacts.Impl.VIDEO_CALL_SERVICE_CONTACT_ADDRESS,
        Contacts.Impl.VIDEO_CALL_CAPABILITY,
        Contacts.Impl.VIDEO_CALL_CAPABILITY_TIMESTAMP,
        Contacts.Impl.VIDEO_CALL_AVAILABILITY,
        Contacts.Impl.VIDEO_CALL_AVAILABILITY_TIMESTAMP,
        Contacts.Impl.VOLTE_STATUS
    };

    /**
     * Look up the formatted number and Data ID
     */
    private static final String[] DATA_QUERY_PROJECTION = new String[] {
        Contacts.Impl._ID,
        Contacts.Impl.FORMATTED_NUMBER,
        EABContract.EABColumns.DATA_ID
    };
    // Data Query Columns, which match the DATA_QUERY_PROJECTION
    private static final int DATA_QUERY_ID = 0;
    private static final int DATA_QUERY_FORMATTED_NUMBER = 1;
    private static final int DATA_QUERY_DATA_ID = 2;


    /**
     * This class contains all the information necessary to request a new contact.
     */
    public static class Request {
        private long mId = -1;
        private String mContactNumber = null;
        private String mContactName = null;

        private int mVolteCallCapability = -1;
        private long mVolteCallCapabilityTimeStamp = -1;
        private int mVolteCallAvailability = -1;
        private long mVolteCallAvailabilityTimeStamp = -1;
        private String mVolteCallServiceContactAddress = null;

        private int mVideoCallCapability = -1;
        private long mVideoCallCapabilityTimeStamp = -1;
        private int mVideoCallAvailability = -1;
        private long mVideoCallAvailabilityTimeStamp = -1;
        private String mVideoCallServiceContactAddress = null;

        private long mContactLastUpdatedTimeStamp = -1;
        private int mFieldUpdatedFlags = 0;

        private static int sVolteCallCapabilityFlag = 0x0001;
        private static int sVolteCallCapabilityTimeStampFlag = 0x0002;
        private static int sVolteCallAvailabilityFlag = 0x0004;
        private static int sVolteCallAvailabilityTimeStampFlag = 0x0008;
        private static int sVolteCallServiceContactAddressFlag = 0x0010;
        private static int sVideoCallCapabilityFlag = 0x0020;
        private static int sVideoCallCapabilityTimeStampFlag = 0x0040;
        private static int sVideoCallAvailabilityFlag = 0x0080;
        private static int sVideoCallAvailabilityTimeStampFlag = 0x0100;
        private static int sVideoCallServiceContactAddressFlag = 0x0200;
        private static int sContactLastUpdatedTimeStampFlag = 0x0400;

        /**
         * @param id the contact id.
         */
        public Request(long id) {
            if (id < 0) {
                throw new IllegalArgumentException(
                        "Can't update EAB presence item with id: " + id);
            }

            mId = id;
        }

        public Request(String number) {
            if (TextUtils.isEmpty(number)) {
                throw new IllegalArgumentException(
                        "Can't update EAB presence item with number: " + number);
            }

            mContactNumber = number;
        }

        public long getContactId() {
            return mId;
        }

        public String getContactNumber() {
            return mContactNumber;
        }

        /**
         * Set Volte call service contact address.
         * @param address contact from NOTIFY
         * @return this object
         */
        public Request setVolteCallServiceContactAddress(String address) {
            mVolteCallServiceContactAddress = address;
            mFieldUpdatedFlags |= sVolteCallServiceContactAddressFlag;
            return this;
        }

        /**
         * Set Volte call capability.
         * @param b wheter volte call is supported or not
         * @return this object
         */
        public Request setVolteCallCapability(boolean b) {
            mVolteCallCapability = b ? 1 : 0;
            mFieldUpdatedFlags |= sVolteCallCapabilityFlag;
            return this;
        }

        public Request setVolteCallCapability(int val) {
            mVolteCallCapability = val;
            mFieldUpdatedFlags |= sVolteCallCapabilityFlag;
            return this;
        }

        /**
         * Set Volte call availability.
         * @param b wheter volte call is available or not
         * @return this object
         */
        public Request setVolteCallAvailability(boolean b) {
            mVolteCallAvailability = b ? 1 : 0;
            mFieldUpdatedFlags |= sVolteCallAvailabilityFlag;
            return this;
        }

        public Request setVolteCallAvailability(int val) {
            mVolteCallAvailability = val;
            mFieldUpdatedFlags |= sVolteCallAvailabilityFlag;
            return this;
        }

        /**
         * Set Video call service contact address.
         * @param address contact from NOTIFY.
         * @return this object
         */
        public Request setVideoCallServiceContactAddress(String address) {
            mVideoCallServiceContactAddress = address;
            mFieldUpdatedFlags |= sVideoCallServiceContactAddressFlag;
            return this;
        }

        /**
         * Set Video call capability.
         * @param b wheter volte call is supported or not
         * @return this object
         */
        public Request setVideoCallCapability(boolean b) {
            mVideoCallCapability = b ? 1 : 0;
            mFieldUpdatedFlags |= sVideoCallCapabilityFlag;
            return this;
        }

        public Request setVideoCallCapability(int val) {
            mVideoCallCapability = val;
            mFieldUpdatedFlags |= sVideoCallCapabilityFlag;
            return this;
        }

        /**
         * Set Video call availability.
         * @param b wheter volte call is available or not
         * @return this object
         */
        public Request setVideoCallAvailability(boolean b) {
            mVideoCallAvailability = b ? 1 : 0;
            mFieldUpdatedFlags |= sVideoCallAvailabilityFlag;
            return this;
        }

        public Request setVideoCallAvailability(int val) {
            mVideoCallAvailability = val;
            mFieldUpdatedFlags |= sVideoCallAvailabilityFlag;
            return this;
        }

        /**
         * Set the update timestamp.
         * @param long timestamp the last update timestamp
         * @return this object
         */
        public Request setLastUpdatedTimeStamp(long timestamp) {
            mContactLastUpdatedTimeStamp = timestamp;
            mFieldUpdatedFlags |= sContactLastUpdatedTimeStampFlag;
            return this;
        }

        public Request setVolteCallCapabilityTimeStamp(long timestamp) {
            mVolteCallCapabilityTimeStamp = timestamp;
            mFieldUpdatedFlags |= sVolteCallCapabilityTimeStampFlag;
            return this;
        }

        public Request setVolteCallAvailabilityTimeStamp(long timestamp) {
            mVolteCallAvailabilityTimeStamp = timestamp;
            mFieldUpdatedFlags |= sVolteCallAvailabilityTimeStampFlag;
            return this;
        }

        public Request setVideoCallCapabilityTimeStamp(long timestamp) {
            mVideoCallCapabilityTimeStamp = timestamp;
            mFieldUpdatedFlags |= sVideoCallCapabilityTimeStampFlag;
            return this;
        }

        public Request setVideoCallAvailabilityTimeStamp(long timestamp) {
            mVideoCallAvailabilityTimeStamp = timestamp;
            mFieldUpdatedFlags |= sVideoCallAvailabilityTimeStampFlag;
            return this;
        }

        public Request reset() {
            mVolteCallCapability = -1;
            mVolteCallCapabilityTimeStamp = -1;
            mVolteCallAvailability = -1;
            mVolteCallAvailabilityTimeStamp = -1;
            mVolteCallServiceContactAddress = null;

            mVideoCallCapability = -1;
            mVideoCallCapabilityTimeStamp = -1;
            mVideoCallAvailability = -1;
            mVideoCallAvailabilityTimeStamp = -1;
            mVideoCallServiceContactAddress = null;

            mContactLastUpdatedTimeStamp = -1;
            mFieldUpdatedFlags = 0;
            return this;
        }

        /**
         * @return ContentValues to be passed to EABProvider.update()
         */
        ContentValues toContentValues() {
            ContentValues values = new ContentValues();

            if ((mFieldUpdatedFlags & sVolteCallCapabilityFlag) > 0) {
                values.put(Contacts.Impl.VOLTE_CALL_CAPABILITY,
                        mVolteCallCapability);
            }
            if ((mFieldUpdatedFlags & sVolteCallCapabilityTimeStampFlag) > 0) {
                values.put(Contacts.Impl.VOLTE_CALL_CAPABILITY_TIMESTAMP,
                        mVolteCallCapabilityTimeStamp);
            }
            if ((mFieldUpdatedFlags & sVolteCallAvailabilityFlag) > 0) {
                values.put(Contacts.Impl.VOLTE_CALL_AVAILABILITY,
                        mVolteCallAvailability);
            }
            if ((mFieldUpdatedFlags & sVolteCallAvailabilityTimeStampFlag) > 0) {
                values.put(Contacts.Impl.VOLTE_CALL_AVAILABILITY_TIMESTAMP,
                        mVolteCallAvailabilityTimeStamp);
            }
            if ((mFieldUpdatedFlags & sVolteCallServiceContactAddressFlag) > 0) {
                values.put(Contacts.Impl.VOLTE_CALL_SERVICE_CONTACT_ADDRESS,
                        mVolteCallServiceContactAddress);
            }

            if ((mFieldUpdatedFlags & sVideoCallCapabilityFlag) > 0) {
                values.put(Contacts.Impl.VIDEO_CALL_CAPABILITY,
                        mVideoCallCapability);
            }
            if ((mFieldUpdatedFlags & sVideoCallCapabilityTimeStampFlag) > 0) {
                values.put(Contacts.Impl.VIDEO_CALL_CAPABILITY_TIMESTAMP,
                        mVideoCallCapabilityTimeStamp);
            }
            if ((mFieldUpdatedFlags & sVideoCallAvailabilityFlag) > 0) {
                values.put(Contacts.Impl.VIDEO_CALL_AVAILABILITY,
                        mVideoCallAvailability);
            }
            if ((mFieldUpdatedFlags & sVideoCallAvailabilityTimeStampFlag) > 0) {
                values.put(Contacts.Impl.VIDEO_CALL_AVAILABILITY_TIMESTAMP,
                        mVideoCallAvailabilityTimeStamp);
            }
            if ((mFieldUpdatedFlags & sVideoCallServiceContactAddressFlag) > 0) {
                values.put(Contacts.Impl.VIDEO_CALL_SERVICE_CONTACT_ADDRESS,
                        mVideoCallServiceContactAddress);
            }

            if ((mFieldUpdatedFlags & sContactLastUpdatedTimeStampFlag) > 0 ) {
                values.put(Contacts.Impl.CONTACT_LAST_UPDATED_TIMESTAMP,
                        mContactLastUpdatedTimeStamp);
            }

            return values;
        }

        @Override
        public String toString() {
            StringBuilder sb = new StringBuilder(512);
            sb.append("EABContactManager.Request { ");
            if (mId != -1) {
                sb.append("\nId: " + mId);
            }
            if (!TextUtils.isEmpty(mContactNumber)) {
                sb.append("\nContact Number: " + mContactNumber);
            }
            if (!TextUtils.isEmpty(mContactName)) {
                sb.append("\nContact Name: " + mContactName);
            }

            if ((mFieldUpdatedFlags & sVolteCallCapabilityFlag) > 0) {
                sb.append("\nVolte call capability: " + mVolteCallCapability);
            }
            if ((mFieldUpdatedFlags & sVolteCallCapabilityTimeStampFlag) > 0) {
                sb.append("\nVolte call capability timestamp: " + mVolteCallCapabilityTimeStamp
                        + "(" + getTimeString(mVolteCallCapabilityTimeStamp) + ")");
            }
            if ((mFieldUpdatedFlags & sVolteCallAvailabilityFlag) > 0) {
                sb.append("\nVolte call availability: " + mVolteCallAvailability);
            }
            if ((mFieldUpdatedFlags & sVolteCallAvailabilityTimeStampFlag) > 0) {
                sb.append("\nVolte call availablity timestamp: " + mVolteCallAvailabilityTimeStamp
                        + "(" + getTimeString(mVolteCallAvailabilityTimeStamp) + ")");
            }
            if ((mFieldUpdatedFlags & sVolteCallServiceContactAddressFlag) > 0) {
                sb.append("\nVolte Call Service address: " + mVolteCallServiceContactAddress);
            }

            if ((mFieldUpdatedFlags & sVideoCallCapabilityFlag) > 0) {
                sb.append("\nVideo call capability: " + mVideoCallCapability);
            }
            if ((mFieldUpdatedFlags & sVideoCallCapabilityTimeStampFlag) > 0) {
                sb.append("\nVideo call capability timestamp: " + mVideoCallCapabilityTimeStamp
                        + "(" + getTimeString(mVideoCallCapabilityTimeStamp) + ")");
            }
            if ((mFieldUpdatedFlags & sVideoCallAvailabilityFlag) > 0) {
                sb.append("\nVideo call availability: " + mVideoCallAvailability);
            }
            if ((mFieldUpdatedFlags & sVideoCallAvailabilityTimeStampFlag) > 0) {
                sb.append("\nVideo call availablity timestamp: " + mVideoCallAvailabilityTimeStamp
                        + "(" + getTimeString(mVideoCallAvailabilityTimeStamp) + ")");
            }
            if ((mFieldUpdatedFlags & sVideoCallServiceContactAddressFlag) > 0) {
                sb.append("\nVideo Call Service address: " + mVideoCallServiceContactAddress);
            }

            if ((mFieldUpdatedFlags & sContactLastUpdatedTimeStampFlag) > 0 ) {
                sb.append("\nContact last update time: " + mContactLastUpdatedTimeStamp
                        + "(" + getTimeString(mContactLastUpdatedTimeStamp) + ")");
            }

            sb.append(" }");
            return sb.toString();
        }
    }

    /**
     * This class may be used to filter EABProvider queries.
     */
    public static class Query {
        /**
         * Constant for use with {@link #orderBy}
         * @hide
         */
        public static final int ORDER_ASCENDING = 1;

        /**
         * Constant for use with {@link #orderBy}
         * @hide
         */
        public static final int ORDER_DESCENDING = 2;

        private long[] mIds = null;
        private String mContactNumber = null;
        private List<String> mTimeFilters = null;
        private String mOrderByColumn = COLUMN_LAST_UPDATED_TIMESTAMP;
        private int mOrderDirection = ORDER_ASCENDING;

        /**
         * Include only the contacts with the given IDs.
         * @return this object
         */
        public Query setFilterById(long... ids) {
            mIds = ids;
            return this;
        }

        /**
         * Include only the contacts with the given number.
         * @return this object
         */
        public Query setFilterByNumber(String number) {
            mContactNumber = number;
            return this;
        }

        /**
         * Include the contacts that meet the specified time condition.
         * @return this object
         */
        public Query setFilterByTime(String selection) {
            if (mTimeFilters == null) {
                mTimeFilters = new ArrayList<String>();
            }

            mTimeFilters.add(selection);
            return this;
        }

        /**
         * Include only the contacts that has not been updated before the last time.
         * @return this object
         */
        public Query setFilterByTime(String column, long last) {
            if (mTimeFilters == null) {
                mTimeFilters = new ArrayList<String>();
            }

            mTimeFilters.add(column + "<='" + last + "'");
            return this;
        }

        /**
         * Include only the contacts that has not been updated after the eariest time.
         * @return this object
         */
        public Query setFilterByEarliestTime(String column, long earliest) {
            if (mTimeFilters == null) {
                mTimeFilters = new ArrayList<String>();
            }

            mTimeFilters.add(column + ">='" + earliest + "'");
            return this;
        }

        /**
         * Change the sort order of the returned Cursor.
         *
         * @param column one of the COLUMN_* constants; currently, only
         *         {@link #COLUMN_LAST_MODIFIED_TIMESTAMP} and {@link #COLUMN_TOTAL_SIZE_BYTES} are
         *         supported.
         * @param direction either {@link #ORDER_ASCENDING} or {@link #ORDER_DESCENDING}
         * @return this object
         * @hide
         */
        public Query orderBy(String column, int direction) {
            if (direction != ORDER_ASCENDING && direction != ORDER_DESCENDING) {
                throw new IllegalArgumentException("Invalid direction: " + direction);
            }

            if (column.equals(COLUMN_ID)) {
                mOrderByColumn = Contacts.Impl._ID;
            } else if (column.equals(COLUMN_LAST_UPDATED_TIMESTAMP)) {
                mOrderByColumn = Contacts.Impl.CONTACT_LAST_UPDATED_TIMESTAMP;
            } else {
                throw new IllegalArgumentException("Cannot order by " + column);
            }
            mOrderDirection = direction;
            return this;
        }

        /**
         * Run this query using the given ContentResolver.
         * @param projection the projection to pass to ContentResolver.query()
         * @return the Cursor returned by ContentResolver.query()
         */
        Cursor runQuery(ContentResolver resolver, String[] projection, Uri baseUri) {
            Uri uri = baseUri;
            List<String> selectionParts = new ArrayList<String>();
            String[] selectionArgs = null;

            if (mIds != null) {
                selectionParts.add(getWhereClauseForIds(mIds));
                selectionArgs = getWhereArgsForIds(mIds);
            }

            if (!TextUtils.isEmpty(mContactNumber)) {
                String number = mContactNumber;
                if (number.startsWith("tel:")) {
                    number = number.substring(4);
                }
                String escapedPhoneNumber = DatabaseUtils.sqlEscapeString(number);
                String cselection = "(" + Contacts.Impl.CONTACT_NUMBER + "=" + escapedPhoneNumber;
                cselection += " OR PHONE_NUMBERS_EQUAL(" + Contacts.Impl.CONTACT_NUMBER + ", ";
                cselection += escapedPhoneNumber + ", 0))";

                selectionParts.add(cselection);
            }

            if (mTimeFilters != null) {
                String cselection = joinStrings(" OR ", mTimeFilters);
                int size = mTimeFilters.size();
                if (size > 1) {
                    cselection = "(" + cselection + ")";
                }
                selectionParts.add(cselection);
            }

            String selection = joinStrings(" AND ", selectionParts);
            String orderDirection = (mOrderDirection == ORDER_ASCENDING ? "ASC" : "DESC");
            String orderBy = mOrderByColumn + " " + orderDirection;

            return resolver.query(uri, projection, selection, selectionArgs, orderBy);
        }

        private String joinStrings(String joiner, Iterable<String> parts) {
            StringBuilder builder = new StringBuilder();
            boolean first = true;
            for (String part : parts) {
                if (!first) {
                    builder.append(joiner);
                }
                builder.append(part);
                first = false;
            }
            return builder.toString();
        }

        @Override
        public String toString() {
            List<String> selectionParts = new ArrayList<String>();
            String[] selectionArgs = null;

            if (mIds != null) {
                selectionParts.add(getWhereClauseForIds(mIds));
                selectionArgs = getWhereArgsForIds(mIds);
            }

            if (!TextUtils.isEmpty(mContactNumber)) {
                String number = mContactNumber;
                if (number.startsWith("tel:")) {
                    number = number.substring(4);
                }
                String escapedPhoneNumber = DatabaseUtils.sqlEscapeString(number);
                String cselection = "(" + Contacts.Impl.CONTACT_NUMBER + "=" + escapedPhoneNumber;
                cselection += " OR PHONE_NUMBERS_EQUAL(" + Contacts.Impl.CONTACT_NUMBER + ", ";
                cselection += escapedPhoneNumber + ", 0))";

                selectionParts.add(cselection);
            }

            if (mTimeFilters != null) {
                String cselection = joinStrings(" OR ", mTimeFilters);
                int size = mTimeFilters.size();
                if (size > 1) {
                    cselection = "(" + cselection + ")";
                }
                selectionParts.add(cselection);
            }

            String selection = joinStrings(" AND ", selectionParts);
            String orderDirection = (mOrderDirection == ORDER_ASCENDING ? "ASC" : "DESC");
            String orderBy = mOrderByColumn + " " + orderDirection;

            StringBuilder sb = new StringBuilder(512);
            sb.append("EABContactManager.Query { ");
            sb.append("\nSelection: " + selection);
            sb.append("\nSelectionArgs: " + selectionArgs);
            sb.append("\nOrderBy: " + orderBy);
            sb.append(" }");
            return sb.toString();
        }
    }

    private ContentResolver mResolver;
    private String mPackageName;
    private Uri mBaseUri = Contacts.Impl.CONTENT_URI;

    /**
     * @hide
     */
    public EABContactManager(ContentResolver resolver, String packageName) {
        mResolver = resolver;
        mPackageName = packageName;
    }

    /**
     * Query the presence manager about contacts that have been requested.
     * @param query parameters specifying filters for this query
     * @return a Cursor over the result set of contacts, with columns consisting of all the
     * COLUMN_* constants.
     */
    public Cursor query(Query query) {
        Cursor underlyingCursor = query.runQuery(mResolver, CONTACT_COLUMNS, mBaseUri);
        if (underlyingCursor == null) {
            return null;
        }

        return new CursorTranslator(underlyingCursor, mBaseUri);
    }

    /**
     * Update a contact presence status.
     *
     * @param request the parameters specifying this contact presence
     * @return an ID for the contact, unique across the system.  This ID is used to make future
     * calls related to this contact.
     */
    public int update(Request request) {
        if (request == null) {
            return 0;
        }

        long id = request.getContactId();
        String number = request.getContactNumber();
        if ((id == -1) && TextUtils.isEmpty(number)) {
            throw new IllegalArgumentException("invalid request for contact update.");
        }

        ContentValues values = request.toContentValues();
        if (id != -1) {
            logger.debug("Update contact " + id + " with request: " + values);
            return mResolver.update(ContentUris.withAppendedId(mBaseUri, id), values,
                    null, null);
        } else {
            Query query = new Query().setFilterByNumber(number)
                                     .orderBy(COLUMN_ID, Query.ORDER_ASCENDING);
            long[] ids = null;
            Cursor cursor = null;
            try {
                cursor = query(query);
                if (cursor == null) {
                    return 0;
                }
                int count = cursor.getCount();
                if (count == 0) {
                    return 0;
                }

                ids = new long[count];
                int idx = 0;
                for (cursor.moveToFirst(); !cursor.isAfterLast(); cursor.moveToNext()) {
                    id = cursor.getLong(cursor.getColumnIndex(Contacts.Impl._ID));
                    ids[idx++] = id;
                    if (idx >= count) {
                        break;
                    }
                }
            } finally {
                if (cursor != null) {
                    cursor.close();
                }
            }

            if ((ids == null) || (ids.length == 0)) {
                return 0;
            }

            if (ids.length == 1) {
                logger.debug("Update contact " + ids[0] + " with request: " + values);
                return mResolver.update(ContentUris.withAppendedId(mBaseUri, ids[0]), values,
                        null, null);
            }

            logger.debug("Update contact " + number + " with request: " + values);
            return mResolver.update(mBaseUri, values, getWhereClauseForIds(ids),
                    getWhereArgsForIds(ids));
        }
    }

    /**
     * Get the EABProvider URI for the contact with the given ID.
     *
     * @hide
     */
    public Uri getContactUri(long id) {
        return ContentUris.withAppendedId(mBaseUri, id);
    }

    /**
     * Get a parameterized SQL WHERE clause to select a bunch of IDs.
     */
    static String getWhereClauseForIds(long[] ids) {
        StringBuilder whereClause = new StringBuilder();
        whereClause.append("(");
        for (int i = 0; i < ids.length; i++) {
            if (i > 0) {
                whereClause.append("OR ");
            }
            whereClause.append(COLUMN_ID);
            whereClause.append(" = ? ");
        }
        whereClause.append(")");
        return whereClause.toString();
    }

    /**
     * Get the selection args for a clause returned by {@link #getWhereClauseForIds(long[])}.
     */
    static String[] getWhereArgsForIds(long[] ids) {
        String[] whereArgs = new String[ids.length];
        for (int i = 0; i < ids.length; i++) {
            whereArgs[i] = Long.toString(ids[i]);
        }
        return whereArgs;
    }

    static String getTimeString(long time) {
        if (time <= 0) {
            time = System.currentTimeMillis();
        }

        Time tobj = new Time();
        tobj.set(time);
        return String.format("%s.%s", tobj.format("%m-%d %H:%M:%S"), time % 1000);
    }

    /**
     * This class wraps a cursor returned by EABProvider -- the "underlying cursor" -- and
     * presents a different set of columns, those defined in the COLUMN_* constants.
     * Some columns correspond directly to underlying values while others are computed from
     * underlying data.
     */
    private static class CursorTranslator extends CursorWrapper {
        private Uri mBaseUri;

        public CursorTranslator(Cursor cursor, Uri baseUri) {
            super(cursor);
            mBaseUri = baseUri;
        }

        @Override
        public int getInt(int columnIndex) {
            return (int) getLong(columnIndex);
        }

        @Override
        public long getLong(int columnIndex) {
            return super.getLong(columnIndex);
        }

        @Override
        public String getString(int columnIndex) {
            return super.getString(columnIndex);
        }
    }

    public void updateAllCapabilityToUnknown() {
        if (mResolver == null) {
            logger.error("updateAllCapabilityToUnknown, mResolver=null");
            return;
        }

        ContentValues values = new ContentValues();
        values.put(Contacts.Impl.CONTACT_LAST_UPDATED_TIMESTAMP, (String)null);
        values.put(Contacts.Impl.VOLTE_CALL_SERVICE_CONTACT_ADDRESS, (String)null);
        values.put(Contacts.Impl.VOLTE_CALL_CAPABILITY, (String)null);
        values.put(Contacts.Impl.VOLTE_CALL_CAPABILITY_TIMESTAMP, (String)null);
        values.put(Contacts.Impl.VOLTE_CALL_AVAILABILITY, (String)null);
        values.put(Contacts.Impl.VOLTE_CALL_AVAILABILITY_TIMESTAMP, (String)null);

        values.put(Contacts.Impl.VIDEO_CALL_SERVICE_CONTACT_ADDRESS, (String)null);
        values.put(Contacts.Impl.VIDEO_CALL_CAPABILITY, (String)null);
        values.put(Contacts.Impl.VIDEO_CALL_CAPABILITY_TIMESTAMP, (String)null);
        values.put(Contacts.Impl.VIDEO_CALL_AVAILABILITY, (String)null);
        values.put(Contacts.Impl.VIDEO_CALL_AVAILABILITY_TIMESTAMP, (String)null);

        try {
            int count = ContactDbUtil.resetVtCapability(mResolver);
            logger.print("update Contact DB: updateAllCapabilityToUnknown count=" + count);

            count = mResolver.update(Contacts.Impl.CONTENT_URI,
                    values, null, null);
            logger.print("update EAB DB: updateAllCapabilityToUnknown count=" + count);
        } catch (Exception ex) {
            logger.error("updateAllCapabilityToUnknown exception: " + ex);
        }
    }

    public void updateAllVtCapabilityToUnknown() {
        if (mResolver == null) {
            logger.error("updateAllVtCapabilityToUnknown mResolver=null");
            return;
        }

        ContentValues values = new ContentValues();
        values.put(Contacts.Impl.CONTACT_LAST_UPDATED_TIMESTAMP, (String)null);
        values.put(Contacts.Impl.VIDEO_CALL_SERVICE_CONTACT_ADDRESS, (String)null);
        values.put(Contacts.Impl.VIDEO_CALL_CAPABILITY, (String)null);
        values.put(Contacts.Impl.VIDEO_CALL_CAPABILITY_TIMESTAMP, (String)null);
        values.put(Contacts.Impl.VIDEO_CALL_AVAILABILITY, (String)null);
        values.put(Contacts.Impl.VIDEO_CALL_AVAILABILITY_TIMESTAMP, (String)null);

        try {
            int count = ContactDbUtil.resetVtCapability(mResolver);
            logger.print("update Contact DB: updateAllVtCapabilityToUnknown count=" + count);

            count = mResolver.update(Contacts.Impl.CONTENT_URI,
                    values, null, null);
            logger.print("update EAB DB: updateAllVtCapabilityToUnknown count=" + count);
        } catch (Exception ex) {
            logger.error("updateAllVtCapabilityToUnknown exception: " + ex);
        }
    }

    // if updateLastTimestamp is true, the rcsPresenceInfo is from network.
    // if the updateLastTimestamp is false, It is used to update the availabilty to unknown only.
    // And the availability will be updated only when it has expired, so we don't update the
    // timestamp to make sure the availablity still in expired status and will be subscribed from
    // network afterwards.
    public int update(RcsPresenceInfo rcsPresenceInfo, boolean updateLastTimestamp) {
        if (rcsPresenceInfo == null) {
            return 0;
        }

        String number = rcsPresenceInfo.getContactNumber();
        if (TextUtils.isEmpty(number)) {
            logger.error("Failed to update for the contact number is empty.");
            return 0;
        }

        ContentValues values = new ContentValues();

        int volteStatus = rcsPresenceInfo.getVolteStatus();
        if(volteStatus != RcsPresenceInfo.VolteStatus.VOLTE_UNKNOWN) {
            values.put(Contacts.Impl.VOLTE_STATUS, volteStatus);
        }

        if(updateLastTimestamp){
            values.put(Contacts.Impl.CONTACT_LAST_UPDATED_TIMESTAMP,
                    (long)System.currentTimeMillis());
        }

        int lteCallCapability = rcsPresenceInfo.getServiceState(
                RcsPresenceInfo.ServiceType.VOLTE_CALL);
        long lteCallTimestamp = rcsPresenceInfo.getTimeStamp(
                RcsPresenceInfo.ServiceType.VOLTE_CALL);
        values.put(Contacts.Impl.VOLTE_CALL_AVAILABILITY, lteCallCapability);
        values.put(Contacts.Impl.VOLTE_CALL_AVAILABILITY_TIMESTAMP, (long)lteCallTimestamp);
        if(rcsPresenceInfo.getServiceState(RcsPresenceInfo.ServiceType.VOLTE_CALL)
                != RcsPresenceInfo.ServiceState.UNKNOWN){
            String lteCallContactAddress =
                    rcsPresenceInfo.getServiceContact(RcsPresenceInfo.ServiceType.VOLTE_CALL);
            if (!TextUtils.isEmpty(lteCallContactAddress)) {
                values.put(Contacts.Impl.VOLTE_CALL_SERVICE_CONTACT_ADDRESS, lteCallContactAddress);
            }

            values.put(Contacts.Impl.VOLTE_CALL_CAPABILITY, lteCallCapability);
            values.put(Contacts.Impl.VOLTE_CALL_CAPABILITY_TIMESTAMP, lteCallTimestamp);
        }

        int videoCallCapability = rcsPresenceInfo.getServiceState(
                RcsPresenceInfo.ServiceType.VT_CALL);
        long videoCallTimestamp = rcsPresenceInfo.getTimeStamp(
                RcsPresenceInfo.ServiceType.VT_CALL);
        values.put(Contacts.Impl.VIDEO_CALL_AVAILABILITY, videoCallCapability);
        values.put(Contacts.Impl.VIDEO_CALL_AVAILABILITY_TIMESTAMP, (long)videoCallTimestamp);
        if(rcsPresenceInfo.getServiceState(RcsPresenceInfo.ServiceType.VT_CALL)
                != RcsPresenceInfo.ServiceState.UNKNOWN){
            String videoCallContactAddress =
                    rcsPresenceInfo.getServiceContact(RcsPresenceInfo.ServiceType.VT_CALL);
            if (!TextUtils.isEmpty(videoCallContactAddress)) {
                values.put(Contacts.Impl.VIDEO_CALL_SERVICE_CONTACT_ADDRESS,
                        videoCallContactAddress);
            }

            values.put(Contacts.Impl.VIDEO_CALL_CAPABILITY, videoCallCapability);
            values.put(Contacts.Impl.VIDEO_CALL_CAPABILITY_TIMESTAMP, videoCallTimestamp);
        }

        int count = 0;
        Cursor cursor = null;
        try{
            cursor = mResolver.query(Contacts.Impl.CONTENT_URI, DATA_QUERY_PROJECTION,
                    "PHONE_NUMBERS_EQUAL(" + Contacts.Impl.FORMATTED_NUMBER + ", ?, 1)",
                    new String[] {number}, null);
            if(cursor == null) {
                logger.print("update rcsPresenceInfo to DB: update count=" + count);
                return count;
            }

            ContactNumberUtils contactNumberUtils = ContactNumberUtils.getDefault();
            for (cursor.moveToFirst(); !cursor.isAfterLast(); cursor.moveToNext()) {
                String numberInDB = cursor.getString(DATA_QUERY_FORMATTED_NUMBER);
                logger.debug("number=" + number + " numberInDB=" + numberInDB +
                        " formatedNumber in DB=" + contactNumberUtils.format(numberInDB));
                if(number.equals(contactNumberUtils.format(numberInDB))) {
                    count = ContactDbUtil.updateVtCapability(mResolver,
                            cursor.getLong(DATA_QUERY_DATA_ID),
                            (videoCallCapability == RcsPresenceInfo.ServiceState.ONLINE));
                    logger.print("update rcsPresenceInfo to Contact DB, count=" + count);

                    int id = cursor.getInt(DATA_QUERY_ID);
                    count += mResolver.update(Contacts.Impl.CONTENT_URI, values,
                            Contacts.Impl._ID + "=" + id, null);
                    logger.debug("count=" + count);
                }
            }

            logger.print("update rcsPresenceInfo to DB: update count=" + count +
                    " rcsPresenceInfo=" + rcsPresenceInfo);
        } catch(Exception e){
            logger.error("updateCapability exception");
        } finally {
            if(cursor != null) {
                cursor.close();
                cursor = null;
            }
        }

        return count;
    }
}

