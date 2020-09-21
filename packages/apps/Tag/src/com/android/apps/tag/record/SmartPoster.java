/*
 * Copyright (C) 2010 The Android Open Source Project
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

package com.android.apps.tag.record;

import com.android.apps.tag.R;
import com.android.apps.tag.message.NdefMessageParser;
import com.google.common.base.Charsets;
import com.google.common.base.Preconditions;
import com.google.common.collect.ImmutableMap;
import com.google.common.collect.Iterables;

import android.app.Activity;
import android.content.Context;
import android.nfc.FormatException;
import android.nfc.NdefMessage;
import android.nfc.NdefRecord;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.ViewGroup.LayoutParams;
import android.widget.LinearLayout;

import java.util.Arrays;
import java.util.Locale;
import java.util.NoSuchElementException;

import javax.annotation.Nullable;

/**
 * A representation of an NFC Forum "Smart Poster".
 */
public class SmartPoster extends ParsedNdefRecord {

    /**
     * NFC Forum Smart Poster Record Type Definition section 3.2.1.
     *
     * "The Title record for the service (there can be many of these in
     * different languages, but a language MUST NOT be repeated).
     * This record is optional."

     */
    private final TextRecord mTitleRecord;

    /**
     * NFC Forum Smart Poster Record Type Definition section 3.2.1.
     *
     * "The URI record. This is the core of the Smart Poster, and all other
     * records are just metadata about this record. There MUST be one URI
     * record and there MUST NOT be more than one."
     */
    private final UriRecord mUriRecord;

    /**
     * NFC Forum Smart Poster Record Type Definition section 3.2.1.
     *
     * "The Icon record. A Smart Poster may include an icon by including one
     * or many MIME-typed image records within the Smart Poster. If the
     * device supports images, it SHOULD select and display one of these,
     * depending on the device capabilities. The device SHOULD display only
     * one. The Icon record is optional."
     */
    private final ImageRecord mImageRecord;

    /**
     * NFC Forum Smart Poster Record Type Definition section 3.2.1.
     *
     * "The Action record. This record describes how the service should be
     * treated. For example, the action may indicate that the device should
     * save the URI as a bookmark or open a browser. The Action record is
     * optional. If it does not exist, the device may decide what to do with
     * the service. If the action record exists, it should be treated as
     * a strong suggestion; the UI designer may ignore it, but doing so
     * will induce a different user experience from device to device."
     */
    private final RecommendedAction mAction;

    /**
     * NFC Forum Smart Poster Record Type Definition section 3.2.1.
     *
     * "The Type record. If the URI references an external entity (e.g., via
     * a URL), the Type record may be used to declare the MIME type of the
     * entity. This can be used to tell the mobile device what kind of an
     * object it can expect before it opens the connection. The Type record
     * is optional."
     */
    private final String mType;


    private SmartPoster(UriRecord uri, @Nullable TextRecord title,
            @Nullable ImageRecord image, RecommendedAction action,
            @Nullable String type) {
        mUriRecord = Preconditions.checkNotNull(uri);
        mTitleRecord = title;
        mImageRecord = image;
        mAction = Preconditions.checkNotNull(action);
        mType = type;
    }

    public UriRecord getUriRecord() {
        return mUriRecord;
    }

    /**
     * Returns the title of the smart poster.  This may be {@code null}.
     */
    public TextRecord getTitle() {
        return mTitleRecord;
    }

    public static SmartPoster parse(NdefRecord record) {
        Preconditions.checkArgument(record.getTnf() == NdefRecord.TNF_WELL_KNOWN);
        Preconditions.checkArgument(Arrays.equals(record.getType(), NdefRecord.RTD_SMART_POSTER));
        try {
            NdefMessage subRecords = new NdefMessage(record.getPayload());
            return parse(subRecords.getRecords());
        } catch (FormatException e) {
            throw new IllegalArgumentException(e);
        }
    }

    public static SmartPoster parse(NdefRecord[] recordsRaw) {
        try {
            Iterable<ParsedNdefRecord> records = NdefMessageParser.getRecords(recordsRaw);
            UriRecord uri = Iterables.getOnlyElement(Iterables.filter(records, UriRecord.class));
            TextRecord title = getFirstIfExists(records, TextRecord.class);
            ImageRecord image = getFirstIfExists(records, ImageRecord.class);
            RecommendedAction action = parseRecommendedAction(recordsRaw);
            String type = parseType(recordsRaw);

            return new SmartPoster(uri, title, image, action, type);
        } catch (NoSuchElementException e) {
            throw new IllegalArgumentException(e);
        }
    }

    public static boolean isPoster(NdefRecord record) {
        try {
            parse(record);
            return true;
        } catch (IllegalArgumentException e) {
            return false;
        }
    }

    @Override
    public View getView(Activity activity, LayoutInflater inflater, ViewGroup parent, int offset) {
        if (mTitleRecord != null) {
            // Build a container to hold the title and the URI
            LinearLayout container = new LinearLayout(activity);
            container.setOrientation(LinearLayout.VERTICAL);
            container.setLayoutParams(new LayoutParams(
                    LayoutParams.MATCH_PARENT, LayoutParams.WRAP_CONTENT));

            container.addView(mTitleRecord.getView(activity, inflater, container, offset));
            inflater.inflate(R.layout.tag_divider, container);
            container.addView(mUriRecord.getView(activity, inflater, container, offset));
            return container;
        } else {
            // Just a URI, return a view for it directly
            return mUriRecord.getView(activity, inflater, parent, offset);
        }
    }

    @Override
    public String getSnippet(Context context, Locale locale) {
        if (mTitleRecord != null) {
            return mTitleRecord.getText();
        }

        return mUriRecord.getPrettyUriString(context);
    }


    /**
     * Returns the first element of {@code elements} which is an instance
     * of {@code type}, or {@code null} if no such element exists.
     */
    private static <T> T getFirstIfExists(Iterable<?> elements, Class<T> type) {
        Iterable<T> filtered = Iterables.filter(elements, type);
        T instance = null;
        if (!Iterables.isEmpty(filtered)) {
            instance = Iterables.get(filtered, 0);
        }
        return instance;
    }

    private enum RecommendedAction {
        UNKNOWN((byte) -1), DO_ACTION((byte) 0),
        SAVE_FOR_LATER((byte) 1), OPEN_FOR_EDITING((byte) 2);

        private static final ImmutableMap<Byte, RecommendedAction> LOOKUP;
        static {
            ImmutableMap.Builder<Byte, RecommendedAction> builder = ImmutableMap.builder();
            for (RecommendedAction action : RecommendedAction.values()) {
                builder.put(action.getByte(), action);
            }
            LOOKUP = builder.build();
        }

        private final byte mAction;

        private RecommendedAction(byte val) {
            this.mAction = val;
        }
        private byte getByte() {
            return mAction;
        }
    }

    private static NdefRecord getByType(byte[] type, NdefRecord[] records) {
        for (NdefRecord record : records) {
            if (Arrays.equals(type, record.getType())) {
                return record;
            }
        }
        return null;
    }

    private static final byte[] ACTION_RECORD_TYPE = new byte[] { 'a', 'c', 't' };

    private static RecommendedAction parseRecommendedAction(NdefRecord[] records) {
        NdefRecord record = getByType(ACTION_RECORD_TYPE, records);
        if (record == null) {
            return RecommendedAction.UNKNOWN;
        }
        byte action = record.getPayload()[0];
        if (RecommendedAction.LOOKUP.containsKey(action)) {
            return RecommendedAction.LOOKUP.get(action);
        }
        return RecommendedAction.UNKNOWN;
    }

    private static final byte[] TYPE_TYPE = new byte[] { 't' };

    private static String parseType(NdefRecord[] records) {
        NdefRecord type = getByType(TYPE_TYPE, records);
        if (type == null) {
            return null;
        }
        return new String(type.getPayload(), Charsets.UTF_8);
    }
}
