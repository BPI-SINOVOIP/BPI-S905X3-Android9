/*
 * Copyright (C) 2009 The Android Open Source Project
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

package com.android.loaderapp;

import android.Manifest;
import android.content.AsyncQueryHandler;
import android.content.ContentResolver;
import android.content.ContentUris;
import android.content.ContentValues;
import android.content.Context;
import android.content.pm.PackageManager;
import android.content.pm.PackageManager.NameNotFoundException;
import android.content.res.Resources;
import android.content.res.Resources.NotFoundException;
import android.database.Cursor;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.net.Uri;
import android.os.SystemClock;
import android.provider.ContactsContract.Contacts;
import android.provider.ContactsContract.Data;
import android.provider.ContactsContract.PhoneLookup;
import android.provider.ContactsContract.RawContacts;
import android.provider.ContactsContract.StatusUpdates;
import android.provider.ContactsContract.CommonDataKinds.Email;
import android.provider.ContactsContract.CommonDataKinds.Photo;
import android.text.TextUtils;
import android.text.format.DateUtils;
import android.util.AttributeSet;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.widget.CheckBox;
import android.widget.QuickContactBadge;
import android.widget.FrameLayout;
import android.widget.ImageView;
import android.widget.TextView;

/**
 * Header used across system for displaying a title bar with contact info. You
 * can bind specific values on the header, or use helper methods like
 * {@link #bindFromContactLookupUri(Uri)} to populate asynchronously.
 * <p>
 * The parent must request the {@link Manifest.permission#READ_CONTACTS}
 * permission to access contact data.
 */
public class ContactHeaderWidget extends FrameLayout implements View.OnClickListener {

    private static final String TAG = "ContactHeaderWidget";

    private TextView mDisplayNameView;
    private View mAggregateBadge;
    private TextView mPhoneticNameView;
    private CheckBox mStarredView;
    private QuickContactBadge mPhotoView;
    private ImageView mPresenceView;
    private TextView mStatusView;
    private TextView mStatusAttributionView;
    private int mNoPhotoResource;
    private QueryHandler mQueryHandler;

    protected Uri mContactUri;

    protected String[] mExcludeMimes = null;

    protected ContentResolver mContentResolver;

    /**
     * Interface for callbacks invoked when the user interacts with a header.
     */
    public interface ContactHeaderListener {
        public void onPhotoClick(View view);
        public void onDisplayNameClick(View view);
    }

    private ContactHeaderListener mListener;


    private interface ContactQuery {
        //Projection used for the summary info in the header.
        String[] COLUMNS = new String[] {
            Contacts._ID,
            Contacts.LOOKUP_KEY,
            Contacts.PHOTO_ID,
            Contacts.DISPLAY_NAME,
            Contacts.PHONETIC_NAME,
            Contacts.STARRED,
            Contacts.CONTACT_PRESENCE,
            Contacts.CONTACT_STATUS,
            Contacts.CONTACT_STATUS_TIMESTAMP,
            Contacts.CONTACT_STATUS_RES_PACKAGE,
            Contacts.CONTACT_STATUS_LABEL,
        };
        int _ID = 0;
        int LOOKUP_KEY = 1;
        int PHOTO_ID = 2;
        int DISPLAY_NAME = 3;
        int PHONETIC_NAME = 4;
        //TODO: We need to figure out how we're going to get the phonetic name.
        //static final int HEADER_PHONETIC_NAME_COLUMN_INDEX
        int STARRED = 5;
        int CONTACT_PRESENCE_STATUS = 6;
        int CONTACT_STATUS = 7;
        int CONTACT_STATUS_TIMESTAMP = 8;
        int CONTACT_STATUS_RES_PACKAGE = 9;
        int CONTACT_STATUS_LABEL = 10;
    }

    private interface PhotoQuery {
        String[] COLUMNS = new String[] {
            Photo.PHOTO
        };

        int PHOTO = 0;
    }

    //Projection used for looking up contact id from phone number
    protected static final String[] PHONE_LOOKUP_PROJECTION = new String[] {
        PhoneLookup._ID,
        PhoneLookup.LOOKUP_KEY,
    };
    protected static final int PHONE_LOOKUP_CONTACT_ID_COLUMN_INDEX = 0;
    protected static final int PHONE_LOOKUP_CONTACT_LOOKUP_KEY_COLUMN_INDEX = 1;

    //Projection used for looking up contact id from email address
    protected static final String[] EMAIL_LOOKUP_PROJECTION = new String[] {
        RawContacts.CONTACT_ID,
        Contacts.LOOKUP_KEY,
    };
    protected static final int EMAIL_LOOKUP_CONTACT_ID_COLUMN_INDEX = 0;
    protected static final int EMAIL_LOOKUP_CONTACT_LOOKUP_KEY_COLUMN_INDEX = 1;

    protected static final String[] CONTACT_LOOKUP_PROJECTION = new String[] {
        Contacts._ID,
    };
    protected static final int CONTACT_LOOKUP_ID_COLUMN_INDEX = 0;

    private static final int TOKEN_CONTACT_INFO = 0;
    private static final int TOKEN_PHONE_LOOKUP = 1;
    private static final int TOKEN_EMAIL_LOOKUP = 2;
    private static final int TOKEN_PHOTO_QUERY = 3;

    public ContactHeaderWidget(Context context) {
        this(context, null);
    }

    public ContactHeaderWidget(Context context, AttributeSet attrs) {
        this(context, attrs, 0);
    }

    public ContactHeaderWidget(Context context, AttributeSet attrs, int defStyle) {
        super(context, attrs, defStyle);

        mContentResolver = mContext.getContentResolver();

        LayoutInflater inflater =
            (LayoutInflater) mContext.getSystemService(Context.LAYOUT_INFLATER_SERVICE);
        inflater.inflate(R.layout.contact_header, this);

        mDisplayNameView = (TextView) findViewById(R.id.name);

        mPhoneticNameView = (TextView) findViewById(R.id.phonetic_name);

        mPhotoView = (QuickContactBadge) findViewById(R.id.photo);

        mPresenceView = (ImageView) findViewById(R.id.presence);

        mStatusView = (TextView)findViewById(R.id.status);
        mStatusAttributionView = (TextView)findViewById(R.id.status_date);

        // Set the photo with a random "no contact" image
        long now = SystemClock.elapsedRealtime();
        int num = (int) now & 0xf;
        if (num < 9) {
            // Leaning in from right, common
            mNoPhotoResource = R.drawable.ic_contact_picture;
        } else if (num < 14) {
            // Leaning in from left uncommon
            mNoPhotoResource = R.drawable.ic_contact_picture_2;
        } else {
            // Coming in from the top, rare
            mNoPhotoResource = R.drawable.ic_contact_picture_3;
        }

        resetAsyncQueryHandler();
    }

    public void enableClickListeners() {
        mDisplayNameView.setOnClickListener(this);
        mPhotoView.setOnClickListener(this);
    }

    /**
     * Set the given {@link ContactHeaderListener} to handle header events.
     */
    public void setContactHeaderListener(ContactHeaderListener listener) {
        mListener = listener;
    }

    private void performPhotoClick() {
        if (mListener != null) {
            mListener.onPhotoClick(mPhotoView);
        }
    }

    private void performDisplayNameClick() {
        if (mListener != null) {
            mListener.onDisplayNameClick(mDisplayNameView);
        }
    }

    private class QueryHandler extends AsyncQueryHandler {

        public QueryHandler(ContentResolver cr) {
            super(cr);
        }

        @Override
        protected void onQueryComplete(int token, Object cookie, Cursor cursor) {
            try{
                if (this != mQueryHandler) {
                    Log.d(TAG, "onQueryComplete: discard result, the query handler is reset!");
                    return;
                }

                switch (token) {
                    case TOKEN_PHOTO_QUERY: {
                        //Set the photo
                        Bitmap photoBitmap = null;
                        if (cursor != null && cursor.moveToFirst()
                                && !cursor.isNull(PhotoQuery.PHOTO)) {
                            byte[] photoData = cursor.getBlob(PhotoQuery.PHOTO);
                            photoBitmap = BitmapFactory.decodeByteArray(photoData, 0,
                                    photoData.length, null);
                        }

                        if (photoBitmap == null) {
                            photoBitmap = loadPlaceholderPhoto(null);
                        }
                        setPhoto(photoBitmap);
                        if (cookie != null && cookie instanceof Uri) {
                            mPhotoView.assignContactUri((Uri) cookie);
                        }
                        invalidate();
                        break;
                    }
                    case TOKEN_CONTACT_INFO: {
                        if (cursor != null && cursor.moveToFirst()) {
                            bindContactInfo(cursor);
                            final Uri lookupUri = Contacts.getLookupUri(
                                    cursor.getLong(ContactQuery._ID),
                                    cursor.getString(ContactQuery.LOOKUP_KEY));

                            final long photoId = cursor.getLong(ContactQuery.PHOTO_ID);

                            setPhotoId(photoId, lookupUri);
                        } else {
                            // shouldn't really happen
                            setDisplayName(null, null);
                            setSocialSnippet(null);
                            setPhoto(loadPlaceholderPhoto(null));
                        }
                        break;
                    }
                    case TOKEN_PHONE_LOOKUP: {
                        if (cursor != null && cursor.moveToFirst()) {
                            long contactId = cursor.getLong(PHONE_LOOKUP_CONTACT_ID_COLUMN_INDEX);
                            String lookupKey = cursor.getString(
                                    PHONE_LOOKUP_CONTACT_LOOKUP_KEY_COLUMN_INDEX);
                            bindFromContactUriInternal(Contacts.getLookupUri(contactId, lookupKey),
                                    false /* don't reset query handler */);
                        } else {
                            String phoneNumber = (String) cookie;
                            setDisplayName(phoneNumber, null);
                            setSocialSnippet(null);
                            setPhoto(loadPlaceholderPhoto(null));
                            mPhotoView.assignContactFromPhone(phoneNumber, true);
                        }
                        break;
                    }
                    case TOKEN_EMAIL_LOOKUP: {
                        if (cursor != null && cursor.moveToFirst()) {
                            long contactId = cursor.getLong(EMAIL_LOOKUP_CONTACT_ID_COLUMN_INDEX);
                            String lookupKey = cursor.getString(
                                    EMAIL_LOOKUP_CONTACT_LOOKUP_KEY_COLUMN_INDEX);
                            bindFromContactUriInternal(Contacts.getLookupUri(contactId, lookupKey),
                                    false /* don't reset query handler */);
                        } else {
                            String emailAddress = (String) cookie;
                            setDisplayName(emailAddress, null);
                            setSocialSnippet(null);
                            setPhoto(loadPlaceholderPhoto(null));
                            mPhotoView.assignContactFromEmail(emailAddress, true);
                        }
                        break;
                    }
                }
            } finally {
                if (cursor != null) {
                    cursor.close();
                }
            }
        }
    }

    /**
     * Manually set the presence.
     */
    public void setPresence(int presence) {
        mPresenceView.setImageResource(StatusUpdates.getPresenceIconResourceId(presence));
    }

    /**
     * Manually set the presence. If presence is null, it is hidden.
     * This doesn't change the underlying {@link Contacts} value, only the UI state.
     * @hide
     */
    public void setPresence(Integer presence) {
        if (presence == null) {
            showPresence(false);
        } else {
            showPresence(true);
            setPresence(presence.intValue());
        }
    }

    /**
     * Turn on/off showing the presence.
     * @hide this is here for consistency with setStared/showStar and should be public
     */
    public void showPresence(boolean showPresence) {
        mPresenceView.setVisibility(showPresence ? View.VISIBLE : View.GONE);
    }

    /**
     * Manually set the contact uri without loading any data
     */
    public void setContactUri(Uri uri) {
        setContactUri(uri, true);
    }

    /**
     * Manually set the contact uri without loading any data
     */
    public void setContactUri(Uri uri, boolean sendToQuickContact) {
        mContactUri = uri;
        if (sendToQuickContact) {
            mPhotoView.assignContactUri(uri);
        }
    }

    /**
     * Manually set the photo to display in the header. This doesn't change the
     * underlying {@link Contacts}, only the UI state.
     */
    public void setPhoto(Bitmap bitmap) {
        mPhotoView.setImageBitmap(bitmap);
    }

    /**
     * Manually set the photo given its id. If the id is 0, a placeholder picture will
     * be loaded. For any other Id, an async query is started
     * @hide
     */
    public void setPhotoId(final long photoId, final Uri lookupUri) {
        if (photoId == 0) {
            setPhoto(loadPlaceholderPhoto(null));
            mPhotoView.assignContactUri(lookupUri);
            invalidate();
        } else {
            startPhotoQuery(photoId, lookupUri,
                    false /* don't reset query handler */);
        }
    }

    /**
     * Manually set the display name and phonetic name to show in the header.
     * This doesn't change the underlying {@link Contacts}, only the UI state.
     */
    public void setDisplayName(CharSequence displayName, CharSequence phoneticName) {
        mDisplayNameView.setText(displayName);
        if (!TextUtils.isEmpty(phoneticName)) {
            mPhoneticNameView.setText(phoneticName);
            mPhoneticNameView.setVisibility(View.VISIBLE);
        } else {
            mPhoneticNameView.setVisibility(View.GONE);
        }
    }

    /**
     * Manually set the social snippet text to display in the header. This doesn't change the
     * underlying {@link Contacts}, only the UI state.
     */
    public void setSocialSnippet(CharSequence snippet) {
        if (snippet == null) {
            mStatusView.setVisibility(View.GONE);
            mStatusAttributionView.setVisibility(View.GONE);
        } else {
            mStatusView.setText(snippet);
            mStatusView.setVisibility(View.VISIBLE);
        }
    }

    /**
     * Manually set the status attribution text to display in the header.
     * This doesn't change the underlying {@link Contacts}, only the UI state.
     * @hide
     */
    public void setStatusAttribution(CharSequence attribution) {
        if (attribution != null) {
            mStatusAttributionView.setText(attribution);
            mStatusAttributionView.setVisibility(View.VISIBLE);
        } else {
            mStatusAttributionView.setVisibility(View.GONE);
        }
    }

    /**
     * Set a list of specific MIME-types to exclude and not display. For
     * example, this can be used to hide the {@link Contacts#CONTENT_ITEM_TYPE}
     * profile icon.
     */
    public void setExcludeMimes(String[] excludeMimes) {
        mExcludeMimes = excludeMimes;
        mPhotoView.setExcludeMimes(excludeMimes);
    }

    /**
     * Manually set all the status values to display in the header.
     * This doesn't change the underlying {@link Contacts}, only the UI state.
     * @hide
     * @param status             The status of the contact. If this is either null or empty,
     *                           the status is cleared and the other parameters are ignored.
     * @param statusTimestamp    The timestamp (retrieved via a call to
     *                           {@link System#currentTimeMillis()}) of the last status update.
     *                           This value can be null if it is not known.
     * @param statusLabel        The id of a resource string that specifies the current
     *                           status. This value can be null if no Label should be used.
     * @param statusResPackage   The name of the resource package containing the resource string
     *                           referenced in the parameter statusLabel.
     */
    public void setStatus(final String status, final Long statusTimestamp,
            final Integer statusLabel, final String statusResPackage) {
        if (TextUtils.isEmpty(status)) {
            setSocialSnippet(null);
            return;
        }

        setSocialSnippet(status);

        final CharSequence timestampDisplayValue;

        if (statusTimestamp != null) {
            // Set the date/time field by mixing relative and absolute
            // times.
            int flags = DateUtils.FORMAT_ABBREV_RELATIVE;

            timestampDisplayValue = DateUtils.getRelativeTimeSpanString(
                    statusTimestamp.longValue(), System.currentTimeMillis(),
                    DateUtils.MINUTE_IN_MILLIS, flags);
        } else {
            timestampDisplayValue = null;
        }


        String labelDisplayValue = null;

        if (statusLabel != null) {
            Resources resources;
            if (TextUtils.isEmpty(statusResPackage)) {
                resources = getResources();
            } else {
                PackageManager pm = getContext().getPackageManager();
                try {
                    resources = pm.getResourcesForApplication(statusResPackage);
                } catch (NameNotFoundException e) {
                    Log.w(TAG, "Contact status update resource package not found: "
                            + statusResPackage);
                    resources = null;
                }
            }

            if (resources != null) {
                try {
                    labelDisplayValue = resources.getString(statusLabel.intValue());
                } catch (NotFoundException e) {
                    Log.w(TAG, "Contact status update resource not found: " + statusResPackage + "@"
                            + statusLabel.intValue());
                }
            }
        }

        final CharSequence attribution;
        if (timestampDisplayValue != null && labelDisplayValue != null) {
            attribution = getContext().getString(
                    R.string.contact_status_update_attribution_with_date,
                    timestampDisplayValue, labelDisplayValue);
        } else if (timestampDisplayValue == null && labelDisplayValue != null) {
            attribution = getContext().getString(
                    R.string.contact_status_update_attribution,
                    labelDisplayValue);
        } else if (timestampDisplayValue != null) {
            attribution = timestampDisplayValue;
        } else {
            attribution = null;
        }
        setStatusAttribution(attribution);
    }

    /**
     * Convenience method for binding all available data from an existing
     * contact.
     *
     * @param contactLookupUri a {Contacts.CONTENT_LOOKUP_URI} style URI.
     */
    public void bindFromContactLookupUri(Uri contactLookupUri) {
        bindFromContactUriInternal(contactLookupUri, true /* reset query handler */);
    }

    /**
     * Convenience method for binding all available data from an existing
     * contact.
     *
     * @param contactUri a {Contacts.CONTENT_URI} style URI.
     * @param resetQueryHandler whether to use a new AsyncQueryHandler or not.
     */
    private void bindFromContactUriInternal(Uri contactUri, boolean resetQueryHandler) {
        mContactUri = contactUri;
        startContactQuery(contactUri, resetQueryHandler);
    }

    /**
     * Convenience method for binding all available data from an existing
     * contact.
     *
     * @param emailAddress The email address used to do a reverse lookup in
     * the contacts database. If more than one contact contains this email
     * address, one of them will be chosen to bind to.
     */
    public void bindFromEmail(String emailAddress) {
        resetAsyncQueryHandler();

        mQueryHandler.startQuery(TOKEN_EMAIL_LOOKUP, emailAddress,
                Uri.withAppendedPath(Email.CONTENT_LOOKUP_URI, Uri.encode(emailAddress)),
                EMAIL_LOOKUP_PROJECTION, null, null, null);
    }

    /**
     * Convenience method for binding all available data from an existing
     * contact.
     *
     * @param number The phone number used to do a reverse lookup in
     * the contacts database. If more than one contact contains this phone
     * number, one of them will be chosen to bind to.
     */
    public void bindFromPhoneNumber(String number) {
        resetAsyncQueryHandler();

        mQueryHandler.startQuery(TOKEN_PHONE_LOOKUP, number,
                Uri.withAppendedPath(PhoneLookup.CONTENT_FILTER_URI, Uri.encode(number)),
                PHONE_LOOKUP_PROJECTION, null, null, null);
    }

    /**
     * startContactQuery
     *
     * internal method to query contact by Uri.
     *
     * @param contactUri the contact uri
     * @param resetQueryHandler whether to use a new AsyncQueryHandler or not
     */
    private void startContactQuery(Uri contactUri, boolean resetQueryHandler) {
        if (resetQueryHandler) {
            resetAsyncQueryHandler();
        }

        mQueryHandler.startQuery(TOKEN_CONTACT_INFO, contactUri, contactUri, ContactQuery.COLUMNS,
                null, null, null);
    }

    /**
     * startPhotoQuery
     *
     * internal method to query contact photo by photo id and uri.
     *
     * @param photoId the photo id.
     * @param lookupKey the lookup uri.
     * @param resetQueryHandler whether to use a new AsyncQueryHandler or not.
     */
    protected void startPhotoQuery(long photoId, Uri lookupKey, boolean resetQueryHandler) {
        if (resetQueryHandler) {
            resetAsyncQueryHandler();
        }

        mQueryHandler.startQuery(TOKEN_PHOTO_QUERY, lookupKey,
                ContentUris.withAppendedId(Data.CONTENT_URI, photoId), PhotoQuery.COLUMNS,
                null, null, null);
    }

    /**
     * Method to force this widget to forget everything it knows about the contact.
     * We need to stop any existing async queries for phone, email, contact, and photos.
     */
    public void wipeClean() {
        resetAsyncQueryHandler();

        setDisplayName(null, null);
        setPhoto(loadPlaceholderPhoto(null));
        setSocialSnippet(null);
        setPresence(0);
        mContactUri = null;
        mExcludeMimes = null;
    }


    private void resetAsyncQueryHandler() {
        // the api AsyncQueryHandler.cancelOperation() doesn't really work. Since we really
        // need the old async queries to be cancelled, let's do it the hard way.
        mQueryHandler = new QueryHandler(mContentResolver);
    }

    /**
     * Bind the contact details provided by the given {@link Cursor}.
     */
    protected void bindContactInfo(Cursor c) {
        final String displayName = c.getString(ContactQuery.DISPLAY_NAME);
        final String phoneticName = c.getString(ContactQuery.PHONETIC_NAME);
        this.setDisplayName(displayName, phoneticName);

        //Set the presence status
        if (!c.isNull(ContactQuery.CONTACT_PRESENCE_STATUS)) {
            int presence = c.getInt(ContactQuery.CONTACT_PRESENCE_STATUS);
            setPresence(presence);
            showPresence(true);
        } else {
            showPresence(false);
        }

        //Set the status update
        final String status = c.getString(ContactQuery.CONTACT_STATUS);
        final Long statusTimestamp = c.isNull(ContactQuery.CONTACT_STATUS_TIMESTAMP)
                ? null
                : c.getLong(ContactQuery.CONTACT_STATUS_TIMESTAMP);
        final Integer statusLabel = c.isNull(ContactQuery.CONTACT_STATUS_LABEL)
                ? null
                : c.getInt(ContactQuery.CONTACT_STATUS_LABEL);
        final String statusResPackage = c.getString(ContactQuery.CONTACT_STATUS_RES_PACKAGE);

        setStatus(status, statusTimestamp, statusLabel, statusResPackage);
    }

    public void onClick(View view) {
        switch (view.getId()) {
            case R.id.photo: {
                performPhotoClick();
                break;
            }
            case R.id.name: {
                performDisplayNameClick();
                break;
            }
        }
    }

    private Bitmap loadPlaceholderPhoto(BitmapFactory.Options options) {
        if (mNoPhotoResource == 0) {
            return null;
        }
        return BitmapFactory.decodeResource(mContext.getResources(),
                mNoPhotoResource, options);
    }
}
