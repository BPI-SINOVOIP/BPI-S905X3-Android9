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

package com.android.car.settingslib.util;

import android.content.ContentResolver;
import android.content.ContentUris;
import android.content.ContentValues;
import android.database.Cursor;
import android.net.Uri;
import android.provider.ContactsContract;
import android.text.TextUtils;
import android.util.Log;

/**
 * Saves name as GIVEN_NAME on "Me" Profile. If "Me" contact does not exist, it's created.
 */
public class ProfileNameSaver {
    private static final String TAG = "ProfileNameSaver";
    private static final Uri PROFILE_URI = ContactsContract.Profile.CONTENT_RAW_CONTACTS_URI;
    private static final int INVALID_ID = -1;

    /**
     * The projection into the profiles database that retrieves just the id of a profile.
     */
    private static final String[] CONTACTS_PROJECTION =
            new String[]{ContactsContract.RawContacts._ID};

    /**
     * A projection into the profile data for one particular contact that includes the
     * contact's MIME type.
     */
    private static final String[] CONTACTS_PROFILE_PROJECTION = new String[]{
            ContactsContract.Data._ID,
            ContactsContract.Data.MIMETYPE};

    /**
     * A selection into the profiles database that queries for a MIME type.
     */
    private static final String CONTACT_MIME_QUERY = ContactsContract.Data.MIMETYPE + " = ?";

    /**
     * Updates the "Me" contact profile card with the given name as the first name of the
     * contact. If the "Me" contact does not exist, then one will be created.
     *
     * @param name The first name for the "Me" contact. If this value is {@code null} or
     *             empty, then the contact will not be updated.
     */
    public static void updateMeContactWith(String name, ContentResolver contentResolver) {
        if (Log.isLoggable(TAG, Log.VERBOSE)) {
            Log.v(TAG, "updateMeContactWith name=" + name);
        }

        if (TextUtils.isEmpty(name)) {
            return;
        }

        // Check if there is already an existing "me" contact.
        Cursor cursor = contentResolver.query(
                PROFILE_URI,
                CONTACTS_PROJECTION,
                /* selection= */ null,
                /* selectionArgs= */ null,
                /* sortOrder= */ null);

        if (cursor == null) {
            // Error in querying the content resolver, skip the update flow
            Log.e(TAG, "Received null from query to the \"me\" contact at " + PROFILE_URI);
            return;
        }

        long meRawContactId = -1;  // no ID can be < 0
        boolean newContact = true;
        try {
            if (cursor.moveToFirst()) {
                meRawContactId = cursor.getLong(0);
                newContact = false;  // An entry indicates that the "me" contact exists.
            }
        } finally {
            cursor.close();
        }

        ContentValues values = new ContentValues();

        if (newContact) {
            meRawContactId = ContentUris.parseId(contentResolver.insert(PROFILE_URI, values));
        }

        values.put(ContactsContract.Data.RAW_CONTACT_ID, meRawContactId);
        values.put(ContactsContract.CommonDataKinds.StructuredName.GIVEN_NAME, name);
        values.put(ContactsContract.Data.MIMETYPE,
                ContactsContract.CommonDataKinds.StructuredName.CONTENT_ITEM_TYPE);

        long structuredNameId = getProfileItem(meRawContactId, contentResolver);
        if (newContact || structuredNameId == INVALID_ID) {
            contentResolver.insert(ContactsContract.Data.CONTENT_URI, values);
        } else {
            contentResolver.update(ContentUris.withAppendedId(
                    ContactsContract.Data.CONTENT_URI, structuredNameId), values,
                    /* where= */ null, /* selectionArgs= */ null);
        }
    }

    /**
     * Helper method to search for the first profile item of the type {@link
     * ContactsContract.CommonDataKinds.StructuredName#CONTENT_ITEM_TYPE}.
     *
     * @return The item's ID or {@link #INVALID_ID} if not found.
     */
    private static long getProfileItem(long profileContactId, ContentResolver contentResolver) {
        if (Log.isLoggable(TAG, Log.VERBOSE)) {
            Log.v(TAG, "getProfileItem profileContactId=" + profileContactId);
        }
        Uri dataUri = ContactsContract.Profile.CONTENT_RAW_CONTACTS_URI.buildUpon()
                .appendPath(String.valueOf(profileContactId))
                .appendPath(ContactsContract.RawContacts.Data.CONTENT_DIRECTORY)
                .build();

        Cursor cursor = contentResolver.query(
                dataUri,
                CONTACTS_PROFILE_PROJECTION,
                CONTACT_MIME_QUERY,
                new String[]{
                        ContactsContract.CommonDataKinds.StructuredName.CONTENT_ITEM_TYPE},
                /* sortOrder= */ null);

        // Error in querying the content resolver.
        if (cursor == null) {
            Log.e(TAG, "Received null from query to the first profile item at " + dataUri);
            return INVALID_ID;
        }

        try {
            return cursor.moveToFirst() ? cursor.getLong(0) : INVALID_ID;
        } finally {
            cursor.close();
        }
    }
}
