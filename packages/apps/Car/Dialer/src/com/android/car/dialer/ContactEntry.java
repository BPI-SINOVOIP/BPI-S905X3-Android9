/*
 * Copyright (C) 2015 The Android Open Source Project
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
package com.android.car.dialer;

import android.content.Context;
import android.database.Cursor;
import android.provider.ContactsContract;
import android.support.annotation.Nullable;
import android.text.TextUtils;
import android.util.Log;

import com.android.car.dialer.telecom.PhoneLoader;
import com.android.car.dialer.telecom.TelecomUtils;

/**
 * Encapsulates data about a phone Contact entry. Typically loaded from the local Contact store.
 */
// TODO: Refactor to use Builder design pattern.
public class ContactEntry implements Comparable<ContactEntry> {
    private final Context mContext;

    /**
     * An unique primary key for searching an entry.
     */
    private int mId;

    /**
     * Whether this contact entry is starred by user.
     */
    private boolean mIsStarred;

    /**
     * Contact-specific information about whether or not a contact has been pinned by the user at
     * a particular position within the system contact application's user interface.
     */
    private int mPinnedPosition;

    /**
     * Phone number.
     */
    private String mNumber;

    /**
     * The display name.
     */
    @Nullable
    private String mDisplayName;

    /**
     * A URI that can be used to retrieve a thumbnail of the contact's photo.
     */
    private String mAvatarThumbnailUri;

    /**
     * A URI that can be used to retrieve the contact's full-size photo.
     */
    private String mAvatarUri;

    /**
     * An opaque value that contains hints on how to find the contact if its row id changed
     * as a result of a sync or aggregation
     */
    private String mLookupKey;

    /**
     * The type of data, for example Home or Work.
     */
    private int mType;

    /**
     * The user defined label for the the contact method.
     */
    private String mLabel;

    /**
     * Parses a Contact entry for a Cursor loaded from the OS Strequents DB.
     */
    public static ContactEntry fromCursor(Cursor cursor, Context context) {
        int idColumnIndex = PhoneLoader.getIdColumnIndex(cursor);
        int starredColumn = cursor.getColumnIndex(ContactsContract.Contacts.STARRED);
        int pinnedColumn = cursor.getColumnIndex("pinned");
        int displayNameColumnIndex = cursor.getColumnIndex(ContactsContract.Contacts.DISPLAY_NAME);
        int avatarUriColumnIndex = cursor.getColumnIndex(ContactsContract.Contacts.PHOTO_URI);
        int avatarThumbnailColumnIndex = cursor.getColumnIndex(
                ContactsContract.Contacts.PHOTO_THUMBNAIL_URI);
        int lookupKeyColumnIndex = cursor.getColumnIndex(ContactsContract.Contacts.LOOKUP_KEY);
        int typeColumnIndex = cursor.getColumnIndex(ContactsContract.CommonDataKinds.Phone.DATA2);
        int labelColumnIndex = cursor.getColumnIndex(ContactsContract.CommonDataKinds.Phone.DATA3);

        String name = cursor.getString(displayNameColumnIndex);
        String number = PhoneLoader.getPhoneNumber(cursor, context.getContentResolver());
        int starred = cursor.getInt(starredColumn);
        int pinnedPosition = cursor.getInt(pinnedColumn);
        ContactEntry contactEntry = new ContactEntry(context, name, number, starred > 0,
                pinnedPosition);
        contactEntry.setId(cursor.getInt(idColumnIndex));
        contactEntry.setAvatarUri(cursor.getString(avatarUriColumnIndex));
        contactEntry.setAvatarThumbnailUri(cursor.getString(avatarThumbnailColumnIndex));
        contactEntry.setLookupKey(cursor.getString(lookupKeyColumnIndex));
        contactEntry.setType(cursor.getInt(typeColumnIndex));
        contactEntry.setLabel(cursor.getString(labelColumnIndex));
        return contactEntry;
    }

    public ContactEntry(
            Context context, String name, String number, boolean isStarred, int pinnedPosition) {
        mContext = context;
        this.mDisplayName = name;
        this.mNumber = number;
        this.mIsStarred = isStarred;
        this.mPinnedPosition = pinnedPosition;
    }

    /**
     * Retrieves a best-effort contact name ready for display to the user.
     * It takes into account the number associated with a name for fail cases.
     */
    public String getDisplayName() {
        if (!TextUtils.isEmpty(mDisplayName)) {
            return mDisplayName;
        }
        if (isVoicemail()) {
            return mContext.getResources().getString(R.string.voicemail);
        } else {
            String displayName = TelecomUtils.getFormattedNumber(mContext, mNumber);
            if (TextUtils.isEmpty(displayName)) {
                displayName = mContext.getString(R.string.unknown);
            }
            return displayName;
        }
    }

    public boolean isVoicemail() {
        return mNumber.equals(TelecomUtils.getVoicemailNumber(mContext));
    }

    @Override
    public int compareTo(ContactEntry strequentContactEntry) {
        if (mIsStarred == strequentContactEntry.mIsStarred) {
            if (mPinnedPosition == strequentContactEntry.mPinnedPosition) {
                if (mDisplayName == strequentContactEntry.mDisplayName) {
                    return compare(mNumber, strequentContactEntry.mNumber);
                }
                return compare(mDisplayName, strequentContactEntry.mDisplayName);
            } else {
                if (mPinnedPosition > 0 && strequentContactEntry.mPinnedPosition > 0) {
                    return mPinnedPosition - strequentContactEntry.mPinnedPosition;
                }

                if (mPinnedPosition > 0) {
                    return -1;
                }

                return 1;
            }
        }

        if (mIsStarred) {
            return -1;
        }

        return 1;
    }

    @Override
    public boolean equals(Object obj) {
        if (obj instanceof ContactEntry) {
            ContactEntry other = (ContactEntry) obj;
            if (compare(mDisplayName, other.mDisplayName) == 0
                    && compare(mNumber, other.mNumber) == 0
                    && mIsStarred == other.mIsStarred
                    && mPinnedPosition == other.mPinnedPosition) {
                return true;
            }
        }
        return false;
    }

    @Override
    public int hashCode() {
        int result = 17;
        result = 31 * result + (mIsStarred ? 1 : 0);
        result = 31 * result + mPinnedPosition;
        result = 31 * result + (mDisplayName == null ? 0 : mDisplayName.hashCode());
        result = 31 * result + (mNumber == null ? 0 : mNumber.hashCode());
        return result;
    }

    private int compare(final String one, final String two) {
        if (one == null ^ two == null) {
            return (one == null) ? -1 : 1;
        }

        if (one == null && two == null) {
            return 0;
        }

        return one.compareTo(two);
    }

    public int getId() {
        return mId;
    }

    private void setId(int id) {
        mId = id;
    }

    public String getAvatarUri() {
        return mAvatarUri;
    }

    private void setAvatarUri(String avatarUri) {
        mAvatarUri = avatarUri;
    }

    public String getLookupKey() {
        return mLookupKey;
    }

    private void setLookupKey(String lookupKey) {
        mLookupKey = lookupKey;
    }

    public int getType() {
        return mType;
    }

    private void setType(int type) {
        mType = type;
    }

    public String getLabel() {
        return mLabel;
    }

    private void setLabel(String label) {
        mLabel = label;
    }

    public String getAvatarThumbnailUri() {
        return mAvatarThumbnailUri;
    }

    private void setAvatarThumbnailUri(String avatarThumbnailUri) {
        mAvatarThumbnailUri = avatarThumbnailUri;
    }

    public String getNumber() {
        return mNumber;
    }

    public boolean isStarred() {
        return mIsStarred;
    }

    public int getPinnedPosition() {
        return mPinnedPosition;
    }

}
