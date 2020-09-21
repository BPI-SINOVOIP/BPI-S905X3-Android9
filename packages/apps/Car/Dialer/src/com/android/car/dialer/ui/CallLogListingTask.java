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
package com.android.car.dialer.ui;

import static android.provider.ContactsContract.CommonDataKinds.Phone.getTypeLabel;

import android.content.Context;
import android.database.Cursor;
import android.graphics.Bitmap;
import android.os.AsyncTask;
import android.provider.CallLog;
import android.support.annotation.NonNull;
import android.telephony.PhoneNumberUtils;
import android.text.TextUtils;
import android.text.format.DateUtils;

import com.android.car.dialer.ContactEntry;
import com.android.car.dialer.R;
import com.android.car.dialer.telecom.InMemoryPhoneBook;
import com.android.car.dialer.telecom.PhoneLoader;
import com.android.car.dialer.telecom.TelecomUtils;

import java.util.ArrayList;
import java.util.List;

/**
 * Async task which loads call history.
 */
public class CallLogListingTask extends AsyncTask<Void, Void, Void> {
    public static class CallLogItem {
        public final String mTitle;
        public final String mText;
        public final String mNumber;
        public final Bitmap mIcon;

        public CallLogItem(String title, String text, String number, Bitmap icon) {
            mTitle = title;
            mText = text;
            mNumber = number;
            mIcon = icon;
        }
    }

    public interface LoadCompleteListener {
        void onLoadComplete(List<CallLogItem> items);
    }

    private Context mContext;
    private Cursor mCursor;
    private List<CallLogItem> mItems;
    private LoadCompleteListener mListener;

    public CallLogListingTask(Context context, Cursor cursor,
            @NonNull LoadCompleteListener listener) {
        mContext = context;
        mCursor = cursor;
        mItems = new ArrayList<>(mCursor.getCount());
        mListener = listener;
    }

    private String maybeAppendCount(StringBuilder sb, int count) {
        if (count > 1) {
            sb.append(" (").append(count).append(")");
        }
        return sb.toString();
    }

    private String getContactName(String cachedName, String number, int count,
            boolean isVoicemail) {
        StringBuilder sb = new StringBuilder();
        if (isVoicemail) {
            sb.append(mContext.getString(R.string.voicemail));
        } else if (cachedName != null) {
            sb.append(cachedName);
        } else if (!TextUtils.isEmpty(number)) {
            sb.append(TelecomUtils.getFormattedNumber(mContext, number));
        } else {
            sb.append(mContext.getString(R.string.unknown));
        }
        return maybeAppendCount(sb, count);
    }

    private static CharSequence getRelativeTime(long millis) {
        boolean validTimestamp = millis > 0;

        return validTimestamp ? DateUtils.getRelativeTimeSpanString(
                millis, System.currentTimeMillis(), DateUtils.MINUTE_IN_MILLIS,
                DateUtils.FORMAT_ABBREV_RELATIVE) : null;
    }

    @Override
    protected Void doInBackground(Void... voids) {
        if (mCursor != null) {
            try {
                int numberColumn = PhoneLoader.getNumberColumnIndex(mCursor);
                int dateColumn = mCursor.getColumnIndex(CallLog.Calls.DATE);

                while (mCursor.moveToNext()) {
                    int count = 1;
                    String number = mCursor.getString(numberColumn);
                    // We want to group calls to the same number into one so seek
                    // forward as many
                    // entries as possible as long as the number is the same.
                    int position = mCursor.getPosition();
                    while (mCursor.moveToNext()) {
                        String nextNumber = mCursor.getString(numberColumn);
                        if (PhoneNumberUtils.compare(mContext, number, nextNumber)) {
                            count++;
                        } else {
                            break;
                        }
                    }

                    mCursor.moveToPosition(position);

                    boolean isVoicemail = PhoneNumberUtils.isVoiceMailNumber(number);
                    ContactEntry contactEntry = InMemoryPhoneBook.get().lookupContactEntry(number);
                    String nameWithCount = getContactName(
                            contactEntry != null ? contactEntry.getDisplayName() : null,
                            number,
                            count,
                            isVoicemail);

                    // Not sure why this is the only column checked here but I'm
                    // assuming this was to work around some bug on some device.
                    long millis = dateColumn == -1 ? 0 : mCursor.getLong(dateColumn);

                    StringBuffer secondaryTextStringBuilder = new StringBuffer();
                    CharSequence relativeDate = getRelativeTime(millis);

                    // Append the type (work, mobile etc.) if it isn't voicemail.
                    if (!isVoicemail) {
                        CharSequence type = contactEntry != null
                                ? getTypeLabel(mContext.getResources(), contactEntry.getType(),
                                contactEntry.getLabel())
                                : "";
                        secondaryTextStringBuilder.append(type);
                        if (!TextUtils.isEmpty(type) && !TextUtils.isEmpty(relativeDate)) {
                            secondaryTextStringBuilder.append(", ");
                        }
                    }
                    // Add in the timestamp.
                    if (relativeDate != null) {
                        secondaryTextStringBuilder.append(relativeDate);
                    }

                    CallLogItem item = new CallLogItem(nameWithCount,
                            secondaryTextStringBuilder.toString(),
                            number, null);
                    mItems.add(item);
                    // Since we deduplicated count rows, we can move all the way to that row so the
                    // next iteration takes us to the row following the last duplicate row.
                    if (count > 1) {
                        mCursor.moveToPosition(position + count - 1);
                    }
                }
            } finally {
                mCursor.close();
            }
        }
        return null;
    }

    @Override
    protected void onPostExecute(Void aVoid) {
        mListener.onLoadComplete(mItems);
    }
}
