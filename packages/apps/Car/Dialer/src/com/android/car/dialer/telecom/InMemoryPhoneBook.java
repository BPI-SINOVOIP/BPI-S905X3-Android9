package com.android.car.dialer.telecom;

import android.content.Context;
import android.database.Cursor;
import android.provider.ContactsContract;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.support.v4.content.CursorLoader;
import android.support.v4.content.Loader;
import android.telephony.PhoneNumberUtils;

import com.android.car.dialer.ContactEntry;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

/**
 * A singleton statically accessible helper class which pre-loads contacts list into memory so
 * that they can be accessed more easily and quickly.
 */
public class InMemoryPhoneBook implements Loader.OnLoadCompleteListener<Cursor> {
    private static InMemoryPhoneBook sInMemoryPhoneBook;

    private final Context mContext;

    private boolean mIsLoaded = false;
    private List<ContactEntry> mContactEntries = new ArrayList<>();
    Map<Integer, List<ContactEntry>> mIdToContactEntryMap;

    private InMemoryPhoneBook(Context context) {
        mContext = context;
    }

    public static InMemoryPhoneBook init(Context context) {
        if (sInMemoryPhoneBook == null) {
            sInMemoryPhoneBook = new InMemoryPhoneBook(context);
            sInMemoryPhoneBook.onInit();
        } else {
            throw new IllegalStateException("Call teardown before reinitialized PhoneBook");
        }
        return get();
    }

    public static InMemoryPhoneBook get() {
        if (sInMemoryPhoneBook != null) {
            return sInMemoryPhoneBook;
        } else {
            throw new IllegalStateException("Call init before get InMemoryPhoneBook");
        }
    }

    public static void tearDown() {
        sInMemoryPhoneBook = null;
    }

    private void onInit() {
        CursorLoader cursorLoader = createPhoneBookCursorLoader();
        cursorLoader.registerListener(0, this);
        cursorLoader.startLoading();
    }

    public boolean isLoaded() {
        return mIsLoaded;
    }

    /**
     * Returns a alphabetically sorted contact list.
     */
    public List<ContactEntry> getOrderedContactEntries() {
        return mContactEntries;
    }

    @Nullable
    public ContactEntry lookupContactEntry(String phoneNumber) {
        for (ContactEntry contactEntry : mContactEntries) {
            if (PhoneNumberUtils.compare(mContext, phoneNumber, contactEntry.getNumber())) {
                return contactEntry;
            }
        }
        return null;
    }

    public Map<Integer, List<ContactEntry>> getIdToContactEntryMap() {
        if (mIdToContactEntryMap == null) {
            mIdToContactEntryMap = new HashMap<>();
            for (ContactEntry contactEntry : mContactEntries) {
                List<ContactEntry> list;
                if (mIdToContactEntryMap.containsKey(contactEntry.getId())) {
                    list = mIdToContactEntryMap.get(contactEntry.getId());
                } else {
                    list = new ArrayList<>();
                }
                list.add(contactEntry);
            }
        }
        return mIdToContactEntryMap;
    }

    @Override
    public void onLoadComplete(@NonNull Loader<Cursor> loader, @Nullable Cursor cursor) {
        if (cursor != null) {
            while (cursor.moveToNext()) {
                mContactEntries.add(ContactEntry.fromCursor(cursor, mContext));
            }
        }
        mIsLoaded = true;
    }

    private CursorLoader createPhoneBookCursorLoader() {
        return new CursorLoader(mContext,
                ContactsContract.Data.CONTENT_URI,
                null,
                ContactsContract.Data.MIMETYPE + " = '"
                        + ContactsContract.CommonDataKinds.Phone
                        .CONTENT_ITEM_TYPE + "'",
                null,
                ContactsContract.Contacts.DISPLAY_NAME + " ASC ");
    }
}
