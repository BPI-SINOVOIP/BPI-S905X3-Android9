/*
 * Copyright 2016 The Android Open Source Project
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
 *
 * Modifications copyright (C) 2018 DTVKit
 */

package org.dtvkit.companionlibrary.utils;

import android.content.ContentResolver;
import android.content.ContentValues;
import android.content.Context;
import android.content.SharedPreferences;
import android.database.Cursor;
import android.media.tv.TvContentRating;
import android.media.tv.TvContract;
import android.media.tv.TvContract.Channels;
import android.net.Uri;
import android.os.AsyncTask;
import android.support.annotation.NonNull;
import android.text.TextUtils;
import android.util.ArrayMap;
import android.util.Log;
import android.util.LongSparseArray;
import android.util.SparseArray;
import android.content.ContentProviderOperation;
import android.content.OperationApplicationException;

import org.dtvkit.companionlibrary.model.Channel;
import org.dtvkit.companionlibrary.model.Program;
import org.dtvkit.companionlibrary.model.InternalProviderData;

import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.MalformedURLException;
import java.net.URL;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

/**
 * Static helper methods for working with {@link android.media.tv.TvContract}.
 */
public class TvContractUtils {
    public static final String PREFERENCES_FILE_KEY = "org.dtvkit.companionlibrary";
    private static final String TAG = "TvContractUtils";
    private static final boolean DEBUG = true;

    /**
     * Updates the list of available channels.
     *
     * @param context The application's context.
     * @param inputId The ID of the TV input service that provides this TV channel.
     * @param channels The updated list of channels.
     * @hide
     */

    public static void updateChannels(Context context, String inputId, boolean isSearched, List<Channel> channels) {
        // Create a map from original network ID to channel row ID for existing channels.
        ArrayMap<Long, Long> channelMap = new ArrayMap<>();
        ArrayMap<Long, String> channelSetFavMap = new ArrayMap<>();
        ArrayMap<Long, String> channelSetSkipMap = new ArrayMap<>();
        ArrayMap<Long, String> channelSkipValueMap = new ArrayMap<>();
        ArrayMap<Long, String> channelFavValueMap = new ArrayMap<>();
        ArrayMap<Long, String> channelFavListValueMap = new ArrayMap<>();
        ArrayMap<Long, String> channelSetDisPlayNameMap = new ArrayMap<>();
        ArrayMap<Long, String> channelSetDisPlayNumberMap = new ArrayMap<>();
        ArrayMap<Long, String> channelDisPlayNameValueMap = new ArrayMap<>();
        ArrayMap<Long, String> channelDisPlayNumberValueMap = new ArrayMap<>();
        Uri channelsUri = TvContract.buildChannelsUriForInput(inputId);
        String[] projection = {Channels._ID, Channels.COLUMN_ORIGINAL_NETWORK_ID, Channels.COLUMN_TRANSPORT_STREAM_ID,
                Channels.COLUMN_SERVICE_ID, Channels.COLUMN_DISPLAY_NAME, Channels.COLUMN_DISPLAY_NUMBER,
                Channels.COLUMN_INTERNAL_PROVIDER_DATA};
        ContentResolver resolver = context.getContentResolver();
        Cursor cursor = null;
        try {
            cursor = resolver.query(channelsUri, projection, null, null, null);
            InternalProviderData internalProviderData = null;
            byte[] internalProviderByteData = null;
            String setFavFlag = "0";
            String setSkipFlag = "0";
            String favValue = "0";
            String favListValue = "";
            String skipValue = "false";
            String setDisplayNameFlag = "0";
            String setDisplayNumberFlag = "0";
            String displayName = null;
            String displayNumber = null;
            while (cursor != null && cursor.moveToNext()) {
                long rowId = cursor.getLong(0);
                int originalNetworkId = cursor.getInt(1);
                int transportStreamId = cursor.getInt(2);
                int serviceId = cursor.getInt(3);
                displayName = cursor.getString(4);
                displayNumber = cursor.getString(5);
                int type = cursor.getType(6);//InternalProviderData type
                setFavFlag = "0";
                setSkipFlag = "0";
                favValue = "0";
                favListValue = "";
                skipValue = "false";
                setDisplayNameFlag = "0";
                setDisplayNumberFlag = "0";
                if (type == Cursor.FIELD_TYPE_BLOB) {
                    internalProviderByteData = cursor.getBlob(6);
                    if (internalProviderByteData != null) {
                        internalProviderData = new InternalProviderData(internalProviderByteData);
                    }
                    if (internalProviderData != null) {
                        if (DEBUG) Log.i(TAG, "internalProviderData = " + internalProviderData.toString());
                        setSkipFlag = (String)internalProviderData.get(Channel.KEY_SET_HIDDEN);
                        setFavFlag = (String)internalProviderData.get(Channel.KEY_SET_FAVOURITE);
                        favValue = (String)internalProviderData.get(Channel.KEY_IS_FAVOURITE);
                        favListValue = (String)internalProviderData.get(Channel.KEY_FAVOURITE_INFO);
                        skipValue = (String)internalProviderData.get(Channel.KEY_HIDDEN);
                        setDisplayNameFlag = (String)internalProviderData.get(Channel.KEY_SET_DISPLAYNAME);
                        setDisplayNumberFlag = (String)internalProviderData.get(Channel.KEY_SET_DISPLAYNUMBER);
                        if (setSkipFlag == null) {
                            setSkipFlag = "0";
                        }
                        if (setFavFlag == null) {
                            setFavFlag = "0";
                        }
                        if (favValue == null) {
                            favValue = "0";
                        }
                        if (favListValue == null) {
                            favListValue = "";
                        }
                        if (skipValue == null) {
                            skipValue = "false";
                        }
                        if (setDisplayNameFlag == null) {
                            setDisplayNameFlag = "0";
                        }
                        if (setDisplayNumberFlag == null) {
                            setDisplayNumberFlag = "0";
                        }
                    }
                    if (DEBUG) Log.i(TAG, "skipValue = " + skipValue + ", setSkipFlag = " + setSkipFlag + ", favValue = " + favValue + ", setFavFlag = " + setFavFlag +
                            ", setDisplayNameFlag = " + setDisplayNameFlag + ", setDisplayNumberFlag = " + setDisplayNumberFlag + ", displayName = " + displayName + " displayNumber = " + displayNumber);
                } else {
                    if (DEBUG) Log.i(TAG, "COLUMN_INTERNAL_PROVIDER_DATA other type");
                }

                channelMap.put(((long)originalNetworkId << 32) | (transportStreamId << 16) | serviceId, rowId);
                if (!isSearched) {
                    channelSetFavMap.put(((long)originalNetworkId << 32) | (transportStreamId << 16) | serviceId, setFavFlag);
                    channelSetSkipMap.put(((long)originalNetworkId << 32) | (transportStreamId << 16) | serviceId, setSkipFlag);
                    channelSkipValueMap.put(((long)originalNetworkId << 32) | (transportStreamId << 16) | serviceId, skipValue);
                    channelFavValueMap.put(((long)originalNetworkId << 32) | (transportStreamId << 16) | serviceId, favValue);
                    channelFavListValueMap.put(((long)originalNetworkId << 32) | (transportStreamId << 16) | serviceId, favListValue);
                    channelSetDisPlayNameMap.put(((long)originalNetworkId << 32) | (transportStreamId << 16) | serviceId, setDisplayNameFlag);
                    channelSetDisPlayNumberMap.put(((long)originalNetworkId << 32) | (transportStreamId << 16) | serviceId, setDisplayNumberFlag);
                    channelDisPlayNameValueMap.put(((long)originalNetworkId << 32) | (transportStreamId << 16) | serviceId, displayName);
                    channelDisPlayNumberValueMap.put(((long)originalNetworkId << 32) | (transportStreamId << 16) | serviceId, displayNumber);
                }
            }
        } catch (Exception e) {
            Log.e(TAG, "updateChannels query Failed = " + e.getMessage());
        } finally {
            if (cursor != null) {
                cursor.close();
            }
        }

        // If a channel exists, update it. If not, insert a new one.
        Map<Uri, String> logos = new HashMap<>();
        ArrayList<ContentProviderOperation> ops = new ArrayList<>();
        for (Channel channel : channels) {
            ContentValues values = new ContentValues();
            values.put(Channels.COLUMN_INPUT_ID, inputId);
            values.putAll(channel.toContentValues());
            // If some required fields are not populated, the app may crash, so defaults are used
            if (channel.getPackageName() == null) {
                // If channel does not include package name, it will be added
                values.put(Channels.COLUMN_PACKAGE_NAME, context.getPackageName());
            }
            if (channel.getInputId() == null) {
                // If channel does not include input id, it will be added
                values.put(Channels.COLUMN_INPUT_ID, inputId);
            }
            if (channel.getType() == null) {
                // If channel does not include type it will be added
                values.put(Channels.COLUMN_TYPE, Channels.TYPE_OTHER);
            }

            int originalNetworkId = channel.getOriginalNetworkId();
            int transportStreamId = channel.getTransportStreamId();
            int serviceId = channel.getServiceId();
            long triplet = ((long)originalNetworkId << 32) | (transportStreamId << 16) | serviceId;
            Long rowId = channelMap.get(triplet);
            String setFavFlag = channelSetFavMap.get(triplet);
            String setSkipFlag = channelSetSkipMap.get(triplet);
            String favValue = channelFavValueMap.get(triplet);
            String favListValue = channelFavListValueMap.get(triplet);
            String skipValue = channelSkipValueMap.get(triplet);
            String setDisplayNameFlag = channelSetDisPlayNameMap.get(triplet);
            String setDisplayNumberFlag = channelSetDisPlayNumberMap.get(triplet);
            String displayName = channelDisPlayNameValueMap.get(triplet);
            String displayNumber = channelDisPlayNumberValueMap.get(triplet);
            InternalProviderData internalProviderData = channel.getInternalProviderData();
            byte[] dataByte = null;
            if (setSkipFlag == null) {
                setSkipFlag = "0";
            }
            if (setFavFlag == null) {
                setFavFlag = "0";
            }
            if (favValue == null) {
                favValue = "0";
            }
            if (favListValue == null) {
                favListValue = "";
            }
            if (skipValue == null) {
                skipValue = "false";
            }
            if (setDisplayNameFlag == null) {
                setSkipFlag = "0";
            }
            if (setDisplayNumberFlag == null) {
                setFavFlag = "0";
            }
            if (internalProviderData == null) {
                internalProviderData = new InternalProviderData();
            }
            try {
                if ("1".equals(setDisplayNameFlag) && displayName != null) {
                    internalProviderData.put(Channel.KEY_SET_DISPLAYNAME, "1");
                    values.put(Channels.COLUMN_DISPLAY_NAME, displayName);
                } else {
                    internalProviderData.put(Channel.KEY_SET_DISPLAYNAME, "0");
                }
                if ("1".equals(setDisplayNumberFlag) && displayNumber != null) {
                    internalProviderData.put(Channel.KEY_SET_DISPLAYNUMBER, "1");
                    values.put(Channels.COLUMN_DISPLAY_NUMBER, displayNumber);
                } else {
                    internalProviderData.put(Channel.KEY_SET_DISPLAYNUMBER, "0");
                }
                if ("1".equals(setFavFlag)) {
                    internalProviderData.put(Channel.KEY_SET_FAVOURITE, "1");
                    internalProviderData.put(Channel.KEY_IS_FAVOURITE, favValue);
                    internalProviderData.put(Channel.KEY_FAVOURITE_INFO, favListValue);
                } else {
                    internalProviderData.put(Channel.KEY_SET_FAVOURITE, "0");
                    String getFavValue = ((String)internalProviderData.get(Channel.KEY_IS_FAVOURITE));
                    String getFavListValue = ((String)internalProviderData.get(Channel.KEY_FAVOURITE_INFO));
                    internalProviderData.put(Channel.KEY_IS_FAVOURITE, getFavValue == null ? "0" : getFavValue);
                    internalProviderData.put(Channel.KEY_FAVOURITE_INFO, getFavListValue == null ? "" : getFavListValue);
                }
                if ("1".equals(setSkipFlag)) {
                    internalProviderData.put(Channel.KEY_SET_HIDDEN, "1");
                    internalProviderData.put(Channel.KEY_HIDDEN, skipValue);
                } else {
                    internalProviderData.put(Channel.KEY_SET_HIDDEN, "0");
                    String getSkipValue = ((String)internalProviderData.get(Channel.KEY_HIDDEN));
                    internalProviderData.put(Channel.KEY_SET_HIDDEN, getSkipValue == null ? "false" : getSkipValue);
                }
                dataByte = internalProviderData.toString().getBytes();
            } catch (Exception e) {
                Log.e(TAG, "updateChannels put Failed = " + e.getMessage());
            }
            if (dataByte != null && dataByte.length > 0) {
                values.put(TvContract.Channels.COLUMN_INTERNAL_PROVIDER_DATA, dataByte);
            } else {
                values.putNull(TvContract.Channels.COLUMN_INTERNAL_PROVIDER_DATA);
            }
            if (DEBUG) {
                Log.d(TAG, String.format("Mapping %d to %d", triplet, rowId));
            }
            Uri uri = null;
            if (rowId == null) {
                //uri = resolver.insert(TvContract.Channels.CONTENT_URI, values);
                ops.add(ContentProviderOperation.newInsert(
                                    TvContract.Channels.CONTENT_URI)
                                    .withValues(values)
                                    .build());
                if (DEBUG) {
                    Log.d(TAG, "Adding channel " + channel.getDisplayName() + " at " + uri);
                }
            } else {
                values.put(Channels._ID, rowId);
                uri = TvContract.buildChannelUri(rowId);
                if (DEBUG) {
                    Log.d(TAG, "Updating channel " + channel.getDisplayName() + " at " + uri);
                }
                //resolver.update(uri, values, null, null);
                ops.add(ContentProviderOperation.newUpdate(
                                    TvContract.buildChannelUri(rowId))
                                    .withValues(values)
                                    .build());
                channelMap.remove(triplet);
            }
            /*if (channel.getChannelLogo() != null && !TextUtils.isEmpty(channel.getChannelLogo())) {
                logos.put(TvContract.buildChannelLogoUri(uri), channel.getChannelLogo());
            }*/
        }
        try {
            resolver.applyBatch(TvContract.AUTHORITY, ops);
        } catch (Exception e) {
            Log.e(TAG, "updateChannels Failed = " + e.getMessage());
        }
        ops.clear();
        if (!logos.isEmpty()) {
            new InsertLogosTask(context).executeOnExecutor(AsyncTask.THREAD_POOL_EXECUTOR, logos);
        }

        // Deletes channels which don't exist in the new feed.
        int size = channelMap.size();
        for (int i = 0; i < size; ++i) {
            Long rowId = channelMap.valueAt(i);
            if (DEBUG) {
                Log.d(TAG, "Deleting channel " + rowId);
            }
            resolver.delete(TvContract.buildChannelUri(rowId), null, null);
            SharedPreferences.Editor editor = context.getSharedPreferences(
                    PREFERENCES_FILE_KEY, Context.MODE_PRIVATE).edit();
            editor.apply();
        }
    }

    public static boolean updateSingleChannelColumn(ContentResolver contentResolver, long id, String columnKey, Object value) {
        boolean ret = false;
        if (id == -1 || TextUtils.isEmpty(columnKey) || contentResolver == null) {
            return ret;
        }
        String[] projection = {columnKey};
        Uri channelsUri = TvContract.buildChannelUri(id);
        Cursor cursor = null;
        ContentValues values = null;
        try {
            cursor = contentResolver.query(channelsUri, projection, Channels._ID + "=?", new String[]{String.valueOf(id)}, null);
            while (cursor != null && cursor.moveToNext()) {
                values = new ContentValues();
                if (value instanceof byte[]) {
                    values.put(columnKey, (byte[])value);
                } else if (value instanceof String) {
                    values.put(columnKey, (String)value);
                } else if (value instanceof Integer) {
                    values.put(columnKey, (Integer)value);
                } else {
                    Log.i(TAG, "updateChannelInternalProviderData unkown data type");
                    return ret;
                }
                ret = true;
                contentResolver.update(channelsUri, values, null, null);
            }
        } catch (Exception e) {
            e.printStackTrace();
            Log.i(TAG, "updateSingleColumn mContentResolver operation Exception = " + e.getMessage());
        }
        try {
            if (cursor != null) {
                cursor.close();
            }
        } catch (Exception e) {
            e.printStackTrace();
            Log.i(TAG, "updateSingleColumn cursor.close() Exception = " + e.getMessage());
        }
        if (DEBUG)
            Log.d(TAG, "updateSingleColumn " + (ret ? "found" : "notfound")
                    + " _id:" + id + " key:" + columnKey + " value:" + value);
        return ret;
    }

    /**
     * Builds a map of available channels.
     *
     * @param resolver Application's ContentResolver.
     * @param inputId The ID of the TV input service that provides this TV channel.
     * @return LongSparseArray mapping each channel's {@link TvContract.Channels#_ID} to the
     * Channel object.
     * @hide
     */
    public static LongSparseArray<Channel> buildChannelMap(@NonNull ContentResolver resolver,
            @NonNull String inputId) {
        Uri uri = TvContract.buildChannelsUriForInput(inputId);
        LongSparseArray<Channel> channelMap = new LongSparseArray<>();
        Cursor cursor = null;
        try {
            cursor = resolver.query(uri, Channel.PROJECTION, null, null, null);
            if (cursor == null || cursor.getCount() == 0) {
                if (DEBUG) {
                    Log.d(TAG, "Cursor is null or found no results");
                }
                return null;
            }

            while (cursor.moveToNext()) {
                Channel nextChannel = Channel.fromCursor(cursor);
                channelMap.put(nextChannel.getId(), nextChannel);
            }
        } catch (Exception e) {
            Log.d(TAG, "Content provider query: " + Arrays.toString(e.getStackTrace()));
            return null;
        } finally {
            if (cursor != null) {
                cursor.close();
            }
        }
        return channelMap;
    }

    /**
     * Returns the current list of channels your app provides.
     *
     * @param resolver Application's ContentResolver.
     * @return List of channels.
     */
    public static List<Channel> getChannels(ContentResolver resolver) {
        List<Channel> channels = new ArrayList<>();
        // TvProvider returns programs in chronological order by default.
        Cursor cursor = null;
        try {
            cursor = resolver.query(Channels.CONTENT_URI, Channel.PROJECTION, null, null, null);
            if (cursor == null || cursor.getCount() == 0) {
                return channels;
            }
            while (cursor.moveToNext()) {
                channels.add(Channel.fromCursor(cursor));
            }
        } catch (Exception e) {
            Log.w(TAG, "Unable to get channels", e);
        } finally {
            if (cursor != null) {
                cursor.close();
            }
        }
        return channels;
    }

    /**
     * Returns the {@link Channel} with specified channel URI.
     * @param resolver {@link ContentResolver} used to query database.
     * @param channelUri URI of channel.
     * @return An channel object with specified channel URI.
     * @hide
     */
    public static Channel getChannel(ContentResolver resolver, Uri channelUri) {
        Cursor cursor = null;
        try {
            cursor = resolver.query(channelUri, Channel.PROJECTION, null, null, null);
            if (cursor == null || cursor.getCount() == 0) {
                Log.w(TAG, "No channel matches " + channelUri);
                return null;
            }
            cursor.moveToNext();
            return Channel.fromCursor(cursor);
        } catch (Exception e) {
            Log.w(TAG, "Unable to get the channel with URI " + channelUri, e);
            return null;
        } finally {
            if (cursor != null) {
                cursor.close();

            }
        }
    }

    /**
     * Returns the {@link Channel} with specified displayName.
     * @param resolver {@link ContentResolver} used to query database.
     * @param displayName of channel.
     * @return An channel object with specified displayName.
     */
    public static Channel getChannelByDisplayName(ContentResolver resolver, String displayName) {
        Cursor cursor = null;
        String selection = TvContract.Channels.COLUMN_DISPLAY_NAME + "=?";
        String[] selectionArgs = new String[]{displayName};
        try {
            cursor = resolver.query(TvContract.Channels.CONTENT_URI, Channel.PROJECTION, selection, selectionArgs, null);
            if (cursor == null || cursor.getCount() == 0) {
                Log.w(TAG, "getChannelByDisplayName No channel matches " + displayName);
                return null;
            }
            cursor.moveToNext();
            return Channel.fromCursor(cursor);
        } catch (Exception e) {
            Log.w(TAG, "displayName Unable to get the channel " + displayName, e);
            return null;
        } finally {
            if (cursor != null) {
                cursor.close();

            }
        }
    }

    /**
     * Returns the current list of programs on a given channel.
     *
     * @param resolver Application's ContentResolver.
     * @param channelUri Channel's Uri.
     * @return List of programs.
     * @hide
     */
    public static List<Program> getPrograms(ContentResolver resolver, Uri channelUri) {
        if (channelUri == null) {
            return null;
        }
        Uri uri = TvContract.buildProgramsUriForChannel(channelUri);
        List<Program> programs = new ArrayList<>();
        // TvProvider returns programs in chronological order by default.
        Cursor cursor = null;
        try {
            cursor = resolver.query(uri, Program.PROJECTION, null, null, null);
            if (cursor == null || cursor.getCount() == 0) {
                return programs;
            }
            while (cursor.moveToNext()) {
                programs.add(Program.fromCursor(cursor));
            }
        } catch (Exception e) {
            Log.w(TAG, "Unable to get programs for " + channelUri, e);
        } finally {
            if (cursor != null) {
                cursor.close();

            }
        }
        return programs;
    }

    /**
     * Returns the program that is scheduled to be playing now on a given channel.
     *
     * @param resolver Application's ContentResolver.
     * @param channelUri Channel's Uri.
     * @return The program that is scheduled for now in the EPG.
     */
    public static Program getCurrentProgram(ContentResolver resolver, Uri channelUri) {
        List<Program> programs = getPrograms(resolver, channelUri);
        if (programs == null) {
            return null;
        }
        long nowMs = System.currentTimeMillis();
        for (Program program : programs) {
            if (program.getStartTimeUtcMillis() <= nowMs && program.getEndTimeUtcMillis() > nowMs) {
                return program;
            }
        }
        return null;
    }

    public static Program getCurrentProgramExt(ContentResolver resolver, Uri channelUri, long nowMs) {
        List<Program> programs = getPrograms(resolver, channelUri);
        if (programs == null) {
            return null;
        }
        //long nowMs = System.currentTimeMillis();
        for (Program program : programs) {
            if (program.getStartTimeUtcMillis() <= nowMs && program.getEndTimeUtcMillis() > nowMs) {
                return program;
            }
        }
        return null;
    }


    /**
     * Returns the program that is scheduled to be playing after a given program on a given channel.
     *
     * @param resolver Application's ContentResolver.
     * @param channelUri Channel's Uri.
     * @param currentProgram Program which plays before the desired program.If null, returns current
     *                       program
     * @return The program that is scheduled after given program in the EPG.
     */
    public static Program getNextProgram(ContentResolver resolver, Uri channelUri,
                                         Program currentProgram) {
        if (currentProgram == null) {
            return getCurrentProgram(resolver, channelUri);
        }
        List<Program> programs = getPrograms(resolver, channelUri);
        if (programs == null) {
            return null;
        }
        int currentProgramIndex = programs.indexOf(currentProgram);
        if (currentProgramIndex + 1 < programs.size()) {
            return programs.get(currentProgramIndex + 1);
        }
        return null;
    }

    private static void insertUrl(Context context, Uri contentUri, URL sourceUrl) {
        if (DEBUG) {
            Log.d(TAG, "Inserting " + sourceUrl + " to " + contentUri);
        }
        InputStream is = null;
        OutputStream os = null;
        try {
            is = sourceUrl.openStream();
            os = context.getContentResolver().openOutputStream(contentUri);
            copy(is, os);
        } catch (IOException ioe) {
            Log.e(TAG, "Failed to write " + sourceUrl + "  to " + contentUri, ioe);
        } finally {
            if (is != null) {
                try {
                    is.close();
                } catch (IOException e) {
                    // Ignore exception.
                }
            }
            if (os != null) {
                try {
                    os.close();
                } catch (IOException e) {
                    // Ignore exception.
                }
            }
        }
    }

    private static void copy(InputStream is, OutputStream os) throws IOException {
        byte[] buffer = new byte[1024];
        int len;
        while ((len = is.read(buffer)) != -1) {
            os.write(buffer, 0, len);
        }
    }

    /**
     * Parses a string of comma-separated ratings into an array of {@link TvContentRating}.
     *
     * @param commaSeparatedRatings String containing various ratings, separated by commas.
     * @return An array of TvContentRatings.
     * @hide
     */
    public static TvContentRating[] stringToContentRatings(String commaSeparatedRatings) {
        if (TextUtils.isEmpty(commaSeparatedRatings)) {
            return null;
        }
        String[] ratings = commaSeparatedRatings.split("\\s*,\\s*");
        TvContentRating[] contentRatings = new TvContentRating[ratings.length];
        for (int i = 0; i < contentRatings.length; ++i) {
            contentRatings[i] = TvContentRating.unflattenFromString(ratings[i]);
        }
        return contentRatings;
    }

    /**
     * Flattens an array of {@link TvContentRating} into a String to be inserted into a database.
     *
     * @param contentRatings An array of TvContentRatings.
     * @return A comma-separated String of ratings.
     * @hide
     */
    public static String contentRatingsToString(TvContentRating[] contentRatings) {
        if (contentRatings == null || contentRatings.length == 0) {
            return null;
        }
        final String DELIMITER = ",";
        StringBuilder ratings = new StringBuilder(contentRatings[0].flattenToString());
        for (int i = 1; i < contentRatings.length; ++i) {
            ratings.append(DELIMITER);
            ratings.append(contentRatings[i].flattenToString());
        }
        return ratings.toString();
    }

    private TvContractUtils() {
    }

    private static class InsertLogosTask extends AsyncTask<Map<Uri, String>, Void, Void> {
        private final Context mContext;

        InsertLogosTask(Context context) {
            mContext = context;
        }

        @Override
        public Void doInBackground(Map<Uri, String>... logosList) {
            for (Map<Uri, String> logos : logosList) {
                for (Uri uri : logos.keySet()) {
                    try {
                        insertUrl(mContext, uri, new URL(logos.get(uri)));
                    } catch (MalformedURLException e) {
                        Log.e(TAG, "Can't load " + logos.get(uri), e);
                    }
                }
            }
            return null;
        }
    }
}
