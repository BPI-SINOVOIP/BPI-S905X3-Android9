/*
 * Copyright (C) 2017 The Android Open Source Project
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
 * limitations under the License
 */

package com.android.tv.testing;

import android.annotation.SuppressLint;
import android.content.ContentProvider;
import android.content.ContentProviderOperation;
import android.content.ContentProviderResult;
import android.content.ContentUris;
import android.content.ContentValues;
import android.content.Context;
import android.content.OperationApplicationException;
import android.content.SharedPreferences;
import android.content.UriMatcher;
import android.content.pm.PackageManager;
import android.database.Cursor;
import android.database.DatabaseUtils;
import android.database.SQLException;
import android.database.sqlite.SQLiteDatabase;
import android.database.sqlite.SQLiteOpenHelper;
import android.database.sqlite.SQLiteQueryBuilder;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.media.tv.TvContract;
import android.net.Uri;
import android.os.AsyncTask;
import android.os.Bundle;
import android.os.ParcelFileDescriptor;
import android.os.ParcelFileDescriptor.AutoCloseInputStream;
import android.preference.PreferenceManager;
import android.provider.BaseColumns;
import android.support.annotation.VisibleForTesting;
import android.support.media.tv.TvContractCompat;
import android.support.media.tv.TvContractCompat.BaseTvColumns;
import android.support.media.tv.TvContractCompat.Channels;
import android.support.media.tv.TvContractCompat.PreviewPrograms;
import android.support.media.tv.TvContractCompat.Programs;
import android.support.media.tv.TvContractCompat.Programs.Genres;
import android.support.media.tv.TvContractCompat.RecordedPrograms;
import android.support.media.tv.TvContractCompat.WatchNextPrograms;
import android.text.TextUtils;
import android.util.Log;
import com.android.tv.util.SqlParams;
import java.io.ByteArrayOutputStream;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Iterator;
import java.util.Map;
import java.util.Set;
import java.util.concurrent.ConcurrentHashMap;

/**
 * Fake TV content provider suitable for unit tests. The contract between this provider and
 * applications is defined in {@link TvContractCompat}.
 */
// TODO(b/62143348): remove when error prone check fixed
@SuppressWarnings({"AndroidApiChecker", "TryWithResources"})
public class FakeTvProvider extends ContentProvider {
    // TODO either make this a shadow or move it to the support library

    private static final boolean DEBUG = false;
    private static final String TAG = "TvProvider";

    static final int DATABASE_VERSION = 34;
    static final String SHARED_PREF_BLOCKED_PACKAGES_KEY = "blocked_packages";
    static final String CHANNELS_TABLE = "channels";
    static final String PROGRAMS_TABLE = "programs";
    static final String RECORDED_PROGRAMS_TABLE = "recorded_programs";
    static final String PREVIEW_PROGRAMS_TABLE = "preview_programs";
    static final String WATCH_NEXT_PROGRAMS_TABLE = "watch_next_programs";
    static final String WATCHED_PROGRAMS_TABLE = "watched_programs";
    static final String PROGRAMS_TABLE_PACKAGE_NAME_INDEX = "programs_package_name_index";
    static final String PROGRAMS_TABLE_CHANNEL_ID_INDEX = "programs_channel_id_index";
    static final String PROGRAMS_TABLE_START_TIME_INDEX = "programs_start_time_index";
    static final String PROGRAMS_TABLE_END_TIME_INDEX = "programs_end_time_index";
    static final String WATCHED_PROGRAMS_TABLE_CHANNEL_ID_INDEX =
            "watched_programs_channel_id_index";
    // The internal column in the watched programs table to indicate whether the current log entry
    // is consolidated or not. Unconsolidated entries may have columns with missing data.
    static final String WATCHED_PROGRAMS_COLUMN_CONSOLIDATED = "consolidated";
    static final String CHANNELS_COLUMN_LOGO = "logo";
    private static final String DATABASE_NAME = "tv.db";
    private static final String DELETED_CHANNELS_TABLE = "deleted_channels"; // Deprecated
    private static final String DEFAULT_PROGRAMS_SORT_ORDER =
            Programs.COLUMN_START_TIME_UTC_MILLIS + " ASC";
    private static final String DEFAULT_WATCHED_PROGRAMS_SORT_ORDER =
            WatchedPrograms.COLUMN_WATCH_START_TIME_UTC_MILLIS + " DESC";
    private static final String CHANNELS_TABLE_INNER_JOIN_PROGRAMS_TABLE =
            CHANNELS_TABLE
                    + " INNER JOIN "
                    + PROGRAMS_TABLE
                    + " ON ("
                    + CHANNELS_TABLE
                    + "."
                    + Channels._ID
                    + "="
                    + PROGRAMS_TABLE
                    + "."
                    + Programs.COLUMN_CHANNEL_ID
                    + ")";

    // Operation names for createSqlParams().
    private static final String OP_QUERY = "query";
    private static final String OP_UPDATE = "update";
    private static final String OP_DELETE = "delete";

    private static final UriMatcher sUriMatcher;
    private static final int MATCH_CHANNEL = 1;
    private static final int MATCH_CHANNEL_ID = 2;
    private static final int MATCH_CHANNEL_ID_LOGO = 3;
    private static final int MATCH_PASSTHROUGH_ID = 4;
    private static final int MATCH_PROGRAM = 5;
    private static final int MATCH_PROGRAM_ID = 6;
    private static final int MATCH_WATCHED_PROGRAM = 7;
    private static final int MATCH_WATCHED_PROGRAM_ID = 8;
    private static final int MATCH_RECORDED_PROGRAM = 9;
    private static final int MATCH_RECORDED_PROGRAM_ID = 10;
    private static final int MATCH_PREVIEW_PROGRAM = 11;
    private static final int MATCH_PREVIEW_PROGRAM_ID = 12;
    private static final int MATCH_WATCH_NEXT_PROGRAM = 13;
    private static final int MATCH_WATCH_NEXT_PROGRAM_ID = 14;

    private static final int MAX_LOGO_IMAGE_SIZE = 256;

    private static final String EMPTY_STRING = "";

    private static final Map<String, String> sChannelProjectionMap;
    private static final Map<String, String> sProgramProjectionMap;
    private static final Map<String, String> sWatchedProgramProjectionMap;
    private static final Map<String, String> sRecordedProgramProjectionMap;
    private static final Map<String, String> sPreviewProgramProjectionMap;
    private static final Map<String, String> sWatchNextProgramProjectionMap;

    // TvContract hidden
    private static final String PARAM_PACKAGE = "package";
    private static final String PARAM_PREVIEW = "preview";

    private static boolean sInitialized;

    static {
        sUriMatcher = new UriMatcher(UriMatcher.NO_MATCH);
        sUriMatcher.addURI(TvContractCompat.AUTHORITY, "channel", MATCH_CHANNEL);
        sUriMatcher.addURI(TvContractCompat.AUTHORITY, "channel/#", MATCH_CHANNEL_ID);
        sUriMatcher.addURI(TvContractCompat.AUTHORITY, "channel/#/logo", MATCH_CHANNEL_ID_LOGO);
        sUriMatcher.addURI(TvContractCompat.AUTHORITY, "passthrough/*", MATCH_PASSTHROUGH_ID);
        sUriMatcher.addURI(TvContractCompat.AUTHORITY, "program", MATCH_PROGRAM);
        sUriMatcher.addURI(TvContractCompat.AUTHORITY, "program/#", MATCH_PROGRAM_ID);
        sUriMatcher.addURI(TvContractCompat.AUTHORITY, "watched_program", MATCH_WATCHED_PROGRAM);
        sUriMatcher.addURI(
                TvContractCompat.AUTHORITY, "watched_program/#", MATCH_WATCHED_PROGRAM_ID);
        sUriMatcher.addURI(TvContractCompat.AUTHORITY, "recorded_program", MATCH_RECORDED_PROGRAM);
        sUriMatcher.addURI(
                TvContractCompat.AUTHORITY, "recorded_program/#", MATCH_RECORDED_PROGRAM_ID);
        sUriMatcher.addURI(TvContractCompat.AUTHORITY, "preview_program", MATCH_PREVIEW_PROGRAM);
        sUriMatcher.addURI(
                TvContractCompat.AUTHORITY, "preview_program/#", MATCH_PREVIEW_PROGRAM_ID);
        sUriMatcher.addURI(
                TvContractCompat.AUTHORITY, "watch_next_program", MATCH_WATCH_NEXT_PROGRAM);
        sUriMatcher.addURI(
                TvContractCompat.AUTHORITY, "watch_next_program/#", MATCH_WATCH_NEXT_PROGRAM_ID);

        sChannelProjectionMap = new HashMap<>();
        sChannelProjectionMap.put(Channels._ID, CHANNELS_TABLE + "." + Channels._ID);
        sChannelProjectionMap.put(
                Channels.COLUMN_PACKAGE_NAME, CHANNELS_TABLE + "." + Channels.COLUMN_PACKAGE_NAME);
        sChannelProjectionMap.put(
                Channels.COLUMN_INPUT_ID, CHANNELS_TABLE + "." + Channels.COLUMN_INPUT_ID);
        sChannelProjectionMap.put(
                Channels.COLUMN_TYPE, CHANNELS_TABLE + "." + Channels.COLUMN_TYPE);
        sChannelProjectionMap.put(
                Channels.COLUMN_SERVICE_TYPE, CHANNELS_TABLE + "." + Channels.COLUMN_SERVICE_TYPE);
        sChannelProjectionMap.put(
                Channels.COLUMN_ORIGINAL_NETWORK_ID,
                CHANNELS_TABLE + "." + Channels.COLUMN_ORIGINAL_NETWORK_ID);
        sChannelProjectionMap.put(
                Channels.COLUMN_TRANSPORT_STREAM_ID,
                CHANNELS_TABLE + "." + Channels.COLUMN_TRANSPORT_STREAM_ID);
        sChannelProjectionMap.put(
                Channels.COLUMN_SERVICE_ID, CHANNELS_TABLE + "." + Channels.COLUMN_SERVICE_ID);
        sChannelProjectionMap.put(
                Channels.COLUMN_DISPLAY_NUMBER,
                CHANNELS_TABLE + "." + Channels.COLUMN_DISPLAY_NUMBER);
        sChannelProjectionMap.put(
                Channels.COLUMN_DISPLAY_NAME, CHANNELS_TABLE + "." + Channels.COLUMN_DISPLAY_NAME);
        sChannelProjectionMap.put(
                Channels.COLUMN_NETWORK_AFFILIATION,
                CHANNELS_TABLE + "." + Channels.COLUMN_NETWORK_AFFILIATION);
        sChannelProjectionMap.put(
                Channels.COLUMN_DESCRIPTION, CHANNELS_TABLE + "." + Channels.COLUMN_DESCRIPTION);
        sChannelProjectionMap.put(
                Channels.COLUMN_VIDEO_FORMAT, CHANNELS_TABLE + "." + Channels.COLUMN_VIDEO_FORMAT);
        sChannelProjectionMap.put(
                Channels.COLUMN_BROWSABLE, CHANNELS_TABLE + "." + Channels.COLUMN_BROWSABLE);
        sChannelProjectionMap.put(
                Channels.COLUMN_SEARCHABLE, CHANNELS_TABLE + "." + Channels.COLUMN_SEARCHABLE);
        sChannelProjectionMap.put(
                Channels.COLUMN_LOCKED, CHANNELS_TABLE + "." + Channels.COLUMN_LOCKED);
        sChannelProjectionMap.put(
                Channels.COLUMN_APP_LINK_ICON_URI,
                CHANNELS_TABLE + "." + Channels.COLUMN_APP_LINK_ICON_URI);
        sChannelProjectionMap.put(
                Channels.COLUMN_APP_LINK_POSTER_ART_URI,
                CHANNELS_TABLE + "." + Channels.COLUMN_APP_LINK_POSTER_ART_URI);
        sChannelProjectionMap.put(
                Channels.COLUMN_APP_LINK_TEXT,
                CHANNELS_TABLE + "." + Channels.COLUMN_APP_LINK_TEXT);
        sChannelProjectionMap.put(
                Channels.COLUMN_APP_LINK_COLOR,
                CHANNELS_TABLE + "." + Channels.COLUMN_APP_LINK_COLOR);
        sChannelProjectionMap.put(
                Channels.COLUMN_APP_LINK_INTENT_URI,
                CHANNELS_TABLE + "." + Channels.COLUMN_APP_LINK_INTENT_URI);
        sChannelProjectionMap.put(
                Channels.COLUMN_INTERNAL_PROVIDER_DATA,
                CHANNELS_TABLE + "." + Channels.COLUMN_INTERNAL_PROVIDER_DATA);
        sChannelProjectionMap.put(
                Channels.COLUMN_INTERNAL_PROVIDER_FLAG1,
                CHANNELS_TABLE + "." + Channels.COLUMN_INTERNAL_PROVIDER_FLAG1);
        sChannelProjectionMap.put(
                Channels.COLUMN_INTERNAL_PROVIDER_FLAG2,
                CHANNELS_TABLE + "." + Channels.COLUMN_INTERNAL_PROVIDER_FLAG2);
        sChannelProjectionMap.put(
                Channels.COLUMN_INTERNAL_PROVIDER_FLAG3,
                CHANNELS_TABLE + "." + Channels.COLUMN_INTERNAL_PROVIDER_FLAG3);
        sChannelProjectionMap.put(
                Channels.COLUMN_INTERNAL_PROVIDER_FLAG4,
                CHANNELS_TABLE + "." + Channels.COLUMN_INTERNAL_PROVIDER_FLAG4);
        sChannelProjectionMap.put(
                Channels.COLUMN_VERSION_NUMBER,
                CHANNELS_TABLE + "." + Channels.COLUMN_VERSION_NUMBER);
        sChannelProjectionMap.put(
                Channels.COLUMN_TRANSIENT, CHANNELS_TABLE + "." + Channels.COLUMN_TRANSIENT);
        sChannelProjectionMap.put(
                Channels.COLUMN_INTERNAL_PROVIDER_ID,
                CHANNELS_TABLE + "." + Channels.COLUMN_INTERNAL_PROVIDER_ID);

        sProgramProjectionMap = new HashMap<>();
        sProgramProjectionMap.put(Programs._ID, Programs._ID);
        sProgramProjectionMap.put(Programs.COLUMN_PACKAGE_NAME, Programs.COLUMN_PACKAGE_NAME);
        sProgramProjectionMap.put(Programs.COLUMN_CHANNEL_ID, Programs.COLUMN_CHANNEL_ID);
        sProgramProjectionMap.put(Programs.COLUMN_TITLE, Programs.COLUMN_TITLE);
        // COLUMN_SEASON_NUMBER is deprecated. Return COLUMN_SEASON_DISPLAY_NUMBER instead.
        sProgramProjectionMap.put(
                Programs.COLUMN_SEASON_NUMBER,
                Programs.COLUMN_SEASON_DISPLAY_NUMBER + " AS " + Programs.COLUMN_SEASON_NUMBER);
        sProgramProjectionMap.put(
                Programs.COLUMN_SEASON_DISPLAY_NUMBER, Programs.COLUMN_SEASON_DISPLAY_NUMBER);
        sProgramProjectionMap.put(Programs.COLUMN_SEASON_TITLE, Programs.COLUMN_SEASON_TITLE);
        // COLUMN_EPISODE_NUMBER is deprecated. Return COLUMN_EPISODE_DISPLAY_NUMBER instead.
        sProgramProjectionMap.put(
                Programs.COLUMN_EPISODE_NUMBER,
                Programs.COLUMN_EPISODE_DISPLAY_NUMBER + " AS " + Programs.COLUMN_EPISODE_NUMBER);
        sProgramProjectionMap.put(
                Programs.COLUMN_EPISODE_DISPLAY_NUMBER, Programs.COLUMN_EPISODE_DISPLAY_NUMBER);
        sProgramProjectionMap.put(Programs.COLUMN_EPISODE_TITLE, Programs.COLUMN_EPISODE_TITLE);
        sProgramProjectionMap.put(
                Programs.COLUMN_START_TIME_UTC_MILLIS, Programs.COLUMN_START_TIME_UTC_MILLIS);
        sProgramProjectionMap.put(
                Programs.COLUMN_END_TIME_UTC_MILLIS, Programs.COLUMN_END_TIME_UTC_MILLIS);
        sProgramProjectionMap.put(Programs.COLUMN_BROADCAST_GENRE, Programs.COLUMN_BROADCAST_GENRE);
        sProgramProjectionMap.put(Programs.COLUMN_CANONICAL_GENRE, Programs.COLUMN_CANONICAL_GENRE);
        sProgramProjectionMap.put(
                Programs.COLUMN_SHORT_DESCRIPTION, Programs.COLUMN_SHORT_DESCRIPTION);
        sProgramProjectionMap.put(
                Programs.COLUMN_LONG_DESCRIPTION, Programs.COLUMN_LONG_DESCRIPTION);
        sProgramProjectionMap.put(Programs.COLUMN_VIDEO_WIDTH, Programs.COLUMN_VIDEO_WIDTH);
        sProgramProjectionMap.put(Programs.COLUMN_VIDEO_HEIGHT, Programs.COLUMN_VIDEO_HEIGHT);
        sProgramProjectionMap.put(Programs.COLUMN_AUDIO_LANGUAGE, Programs.COLUMN_AUDIO_LANGUAGE);
        sProgramProjectionMap.put(Programs.COLUMN_CONTENT_RATING, Programs.COLUMN_CONTENT_RATING);
        sProgramProjectionMap.put(Programs.COLUMN_POSTER_ART_URI, Programs.COLUMN_POSTER_ART_URI);
        sProgramProjectionMap.put(Programs.COLUMN_THUMBNAIL_URI, Programs.COLUMN_THUMBNAIL_URI);
        sProgramProjectionMap.put(Programs.COLUMN_SEARCHABLE, Programs.COLUMN_SEARCHABLE);
        sProgramProjectionMap.put(
                Programs.COLUMN_RECORDING_PROHIBITED, Programs.COLUMN_RECORDING_PROHIBITED);
        sProgramProjectionMap.put(
                Programs.COLUMN_INTERNAL_PROVIDER_DATA, Programs.COLUMN_INTERNAL_PROVIDER_DATA);
        sProgramProjectionMap.put(
                Programs.COLUMN_INTERNAL_PROVIDER_FLAG1, Programs.COLUMN_INTERNAL_PROVIDER_FLAG1);
        sProgramProjectionMap.put(
                Programs.COLUMN_INTERNAL_PROVIDER_FLAG2, Programs.COLUMN_INTERNAL_PROVIDER_FLAG2);
        sProgramProjectionMap.put(
                Programs.COLUMN_INTERNAL_PROVIDER_FLAG3, Programs.COLUMN_INTERNAL_PROVIDER_FLAG3);
        sProgramProjectionMap.put(
                Programs.COLUMN_INTERNAL_PROVIDER_FLAG4, Programs.COLUMN_INTERNAL_PROVIDER_FLAG4);
        sProgramProjectionMap.put(Programs.COLUMN_VERSION_NUMBER, Programs.COLUMN_VERSION_NUMBER);
        sProgramProjectionMap.put(
                Programs.COLUMN_REVIEW_RATING_STYLE, Programs.COLUMN_REVIEW_RATING_STYLE);
        sProgramProjectionMap.put(Programs.COLUMN_REVIEW_RATING, Programs.COLUMN_REVIEW_RATING);

        sWatchedProgramProjectionMap = new HashMap<>();
        sWatchedProgramProjectionMap.put(WatchedPrograms._ID, WatchedPrograms._ID);
        sWatchedProgramProjectionMap.put(
                WatchedPrograms.COLUMN_WATCH_START_TIME_UTC_MILLIS,
                WatchedPrograms.COLUMN_WATCH_START_TIME_UTC_MILLIS);
        sWatchedProgramProjectionMap.put(
                WatchedPrograms.COLUMN_WATCH_END_TIME_UTC_MILLIS,
                WatchedPrograms.COLUMN_WATCH_END_TIME_UTC_MILLIS);
        sWatchedProgramProjectionMap.put(
                WatchedPrograms.COLUMN_CHANNEL_ID, WatchedPrograms.COLUMN_CHANNEL_ID);
        sWatchedProgramProjectionMap.put(
                WatchedPrograms.COLUMN_TITLE, WatchedPrograms.COLUMN_TITLE);
        sWatchedProgramProjectionMap.put(
                WatchedPrograms.COLUMN_START_TIME_UTC_MILLIS,
                WatchedPrograms.COLUMN_START_TIME_UTC_MILLIS);
        sWatchedProgramProjectionMap.put(
                WatchedPrograms.COLUMN_END_TIME_UTC_MILLIS,
                WatchedPrograms.COLUMN_END_TIME_UTC_MILLIS);
        sWatchedProgramProjectionMap.put(
                WatchedPrograms.COLUMN_DESCRIPTION, WatchedPrograms.COLUMN_DESCRIPTION);
        sWatchedProgramProjectionMap.put(
                WatchedPrograms.COLUMN_INTERNAL_TUNE_PARAMS,
                WatchedPrograms.COLUMN_INTERNAL_TUNE_PARAMS);
        sWatchedProgramProjectionMap.put(
                WatchedPrograms.COLUMN_INTERNAL_SESSION_TOKEN,
                WatchedPrograms.COLUMN_INTERNAL_SESSION_TOKEN);
        sWatchedProgramProjectionMap.put(
                WATCHED_PROGRAMS_COLUMN_CONSOLIDATED, WATCHED_PROGRAMS_COLUMN_CONSOLIDATED);

        sRecordedProgramProjectionMap = new HashMap<>();
        sRecordedProgramProjectionMap.put(RecordedPrograms._ID, RecordedPrograms._ID);
        sRecordedProgramProjectionMap.put(
                RecordedPrograms.COLUMN_PACKAGE_NAME, RecordedPrograms.COLUMN_PACKAGE_NAME);
        sRecordedProgramProjectionMap.put(
                RecordedPrograms.COLUMN_INPUT_ID, RecordedPrograms.COLUMN_INPUT_ID);
        sRecordedProgramProjectionMap.put(
                RecordedPrograms.COLUMN_CHANNEL_ID, RecordedPrograms.COLUMN_CHANNEL_ID);
        sRecordedProgramProjectionMap.put(
                RecordedPrograms.COLUMN_TITLE, RecordedPrograms.COLUMN_TITLE);
        sRecordedProgramProjectionMap.put(
                RecordedPrograms.COLUMN_SEASON_DISPLAY_NUMBER,
                RecordedPrograms.COLUMN_SEASON_DISPLAY_NUMBER);
        sRecordedProgramProjectionMap.put(
                RecordedPrograms.COLUMN_SEASON_TITLE, RecordedPrograms.COLUMN_SEASON_TITLE);
        sRecordedProgramProjectionMap.put(
                RecordedPrograms.COLUMN_EPISODE_DISPLAY_NUMBER,
                RecordedPrograms.COLUMN_EPISODE_DISPLAY_NUMBER);
        sRecordedProgramProjectionMap.put(
                RecordedPrograms.COLUMN_EPISODE_TITLE, RecordedPrograms.COLUMN_EPISODE_TITLE);
        sRecordedProgramProjectionMap.put(
                RecordedPrograms.COLUMN_START_TIME_UTC_MILLIS,
                RecordedPrograms.COLUMN_START_TIME_UTC_MILLIS);
        sRecordedProgramProjectionMap.put(
                RecordedPrograms.COLUMN_END_TIME_UTC_MILLIS,
                RecordedPrograms.COLUMN_END_TIME_UTC_MILLIS);
        sRecordedProgramProjectionMap.put(
                RecordedPrograms.COLUMN_BROADCAST_GENRE, RecordedPrograms.COLUMN_BROADCAST_GENRE);
        sRecordedProgramProjectionMap.put(
                RecordedPrograms.COLUMN_CANONICAL_GENRE, RecordedPrograms.COLUMN_CANONICAL_GENRE);
        sRecordedProgramProjectionMap.put(
                RecordedPrograms.COLUMN_SHORT_DESCRIPTION,
                RecordedPrograms.COLUMN_SHORT_DESCRIPTION);
        sRecordedProgramProjectionMap.put(
                RecordedPrograms.COLUMN_LONG_DESCRIPTION, RecordedPrograms.COLUMN_LONG_DESCRIPTION);
        sRecordedProgramProjectionMap.put(
                RecordedPrograms.COLUMN_VIDEO_WIDTH, RecordedPrograms.COLUMN_VIDEO_WIDTH);
        sRecordedProgramProjectionMap.put(
                RecordedPrograms.COLUMN_VIDEO_HEIGHT, RecordedPrograms.COLUMN_VIDEO_HEIGHT);
        sRecordedProgramProjectionMap.put(
                RecordedPrograms.COLUMN_AUDIO_LANGUAGE, RecordedPrograms.COLUMN_AUDIO_LANGUAGE);
        sRecordedProgramProjectionMap.put(
                RecordedPrograms.COLUMN_CONTENT_RATING, RecordedPrograms.COLUMN_CONTENT_RATING);
        sRecordedProgramProjectionMap.put(
                RecordedPrograms.COLUMN_POSTER_ART_URI, RecordedPrograms.COLUMN_POSTER_ART_URI);
        sRecordedProgramProjectionMap.put(
                RecordedPrograms.COLUMN_THUMBNAIL_URI, RecordedPrograms.COLUMN_THUMBNAIL_URI);
        sRecordedProgramProjectionMap.put(
                RecordedPrograms.COLUMN_SEARCHABLE, RecordedPrograms.COLUMN_SEARCHABLE);
        sRecordedProgramProjectionMap.put(
                RecordedPrograms.COLUMN_RECORDING_DATA_URI,
                RecordedPrograms.COLUMN_RECORDING_DATA_URI);
        sRecordedProgramProjectionMap.put(
                RecordedPrograms.COLUMN_RECORDING_DATA_BYTES,
                RecordedPrograms.COLUMN_RECORDING_DATA_BYTES);
        sRecordedProgramProjectionMap.put(
                RecordedPrograms.COLUMN_RECORDING_DURATION_MILLIS,
                RecordedPrograms.COLUMN_RECORDING_DURATION_MILLIS);
        sRecordedProgramProjectionMap.put(
                RecordedPrograms.COLUMN_RECORDING_EXPIRE_TIME_UTC_MILLIS,
                RecordedPrograms.COLUMN_RECORDING_EXPIRE_TIME_UTC_MILLIS);
        sRecordedProgramProjectionMap.put(
                RecordedPrograms.COLUMN_INTERNAL_PROVIDER_DATA,
                RecordedPrograms.COLUMN_INTERNAL_PROVIDER_DATA);
        sRecordedProgramProjectionMap.put(
                RecordedPrograms.COLUMN_INTERNAL_PROVIDER_FLAG1,
                RecordedPrograms.COLUMN_INTERNAL_PROVIDER_FLAG1);
        sRecordedProgramProjectionMap.put(
                RecordedPrograms.COLUMN_INTERNAL_PROVIDER_FLAG2,
                RecordedPrograms.COLUMN_INTERNAL_PROVIDER_FLAG2);
        sRecordedProgramProjectionMap.put(
                RecordedPrograms.COLUMN_INTERNAL_PROVIDER_FLAG3,
                RecordedPrograms.COLUMN_INTERNAL_PROVIDER_FLAG3);
        sRecordedProgramProjectionMap.put(
                RecordedPrograms.COLUMN_INTERNAL_PROVIDER_FLAG4,
                RecordedPrograms.COLUMN_INTERNAL_PROVIDER_FLAG4);
        sRecordedProgramProjectionMap.put(
                RecordedPrograms.COLUMN_VERSION_NUMBER, RecordedPrograms.COLUMN_VERSION_NUMBER);
        sRecordedProgramProjectionMap.put(
                RecordedPrograms.COLUMN_REVIEW_RATING_STYLE,
                RecordedPrograms.COLUMN_REVIEW_RATING_STYLE);
        sRecordedProgramProjectionMap.put(
                RecordedPrograms.COLUMN_REVIEW_RATING, RecordedPrograms.COLUMN_REVIEW_RATING);

        sPreviewProgramProjectionMap = new HashMap<>();
        sPreviewProgramProjectionMap.put(PreviewPrograms._ID, PreviewPrograms._ID);
        sPreviewProgramProjectionMap.put(
                PreviewPrograms.COLUMN_PACKAGE_NAME, PreviewPrograms.COLUMN_PACKAGE_NAME);
        sPreviewProgramProjectionMap.put(
                PreviewPrograms.COLUMN_CHANNEL_ID, PreviewPrograms.COLUMN_CHANNEL_ID);
        sPreviewProgramProjectionMap.put(
                PreviewPrograms.COLUMN_TITLE, PreviewPrograms.COLUMN_TITLE);
        sPreviewProgramProjectionMap.put(
                PreviewPrograms.COLUMN_SEASON_DISPLAY_NUMBER,
                PreviewPrograms.COLUMN_SEASON_DISPLAY_NUMBER);
        sPreviewProgramProjectionMap.put(
                PreviewPrograms.COLUMN_SEASON_TITLE, PreviewPrograms.COLUMN_SEASON_TITLE);
        sPreviewProgramProjectionMap.put(
                PreviewPrograms.COLUMN_EPISODE_DISPLAY_NUMBER,
                PreviewPrograms.COLUMN_EPISODE_DISPLAY_NUMBER);
        sPreviewProgramProjectionMap.put(
                PreviewPrograms.COLUMN_EPISODE_TITLE, PreviewPrograms.COLUMN_EPISODE_TITLE);
        sPreviewProgramProjectionMap.put(
                PreviewPrograms.COLUMN_CANONICAL_GENRE, PreviewPrograms.COLUMN_CANONICAL_GENRE);
        sPreviewProgramProjectionMap.put(
                PreviewPrograms.COLUMN_SHORT_DESCRIPTION, PreviewPrograms.COLUMN_SHORT_DESCRIPTION);
        sPreviewProgramProjectionMap.put(
                PreviewPrograms.COLUMN_LONG_DESCRIPTION, PreviewPrograms.COLUMN_LONG_DESCRIPTION);
        sPreviewProgramProjectionMap.put(
                PreviewPrograms.COLUMN_VIDEO_WIDTH, PreviewPrograms.COLUMN_VIDEO_WIDTH);
        sPreviewProgramProjectionMap.put(
                PreviewPrograms.COLUMN_VIDEO_HEIGHT, PreviewPrograms.COLUMN_VIDEO_HEIGHT);
        sPreviewProgramProjectionMap.put(
                PreviewPrograms.COLUMN_AUDIO_LANGUAGE, PreviewPrograms.COLUMN_AUDIO_LANGUAGE);
        sPreviewProgramProjectionMap.put(
                PreviewPrograms.COLUMN_CONTENT_RATING, PreviewPrograms.COLUMN_CONTENT_RATING);
        sPreviewProgramProjectionMap.put(
                PreviewPrograms.COLUMN_POSTER_ART_URI, PreviewPrograms.COLUMN_POSTER_ART_URI);
        sPreviewProgramProjectionMap.put(
                PreviewPrograms.COLUMN_THUMBNAIL_URI, PreviewPrograms.COLUMN_THUMBNAIL_URI);
        sPreviewProgramProjectionMap.put(
                PreviewPrograms.COLUMN_SEARCHABLE, PreviewPrograms.COLUMN_SEARCHABLE);
        sPreviewProgramProjectionMap.put(
                PreviewPrograms.COLUMN_INTERNAL_PROVIDER_DATA,
                PreviewPrograms.COLUMN_INTERNAL_PROVIDER_DATA);
        sPreviewProgramProjectionMap.put(
                PreviewPrograms.COLUMN_INTERNAL_PROVIDER_FLAG1,
                PreviewPrograms.COLUMN_INTERNAL_PROVIDER_FLAG1);
        sPreviewProgramProjectionMap.put(
                PreviewPrograms.COLUMN_INTERNAL_PROVIDER_FLAG2,
                PreviewPrograms.COLUMN_INTERNAL_PROVIDER_FLAG2);
        sPreviewProgramProjectionMap.put(
                PreviewPrograms.COLUMN_INTERNAL_PROVIDER_FLAG3,
                PreviewPrograms.COLUMN_INTERNAL_PROVIDER_FLAG3);
        sPreviewProgramProjectionMap.put(
                PreviewPrograms.COLUMN_INTERNAL_PROVIDER_FLAG4,
                PreviewPrograms.COLUMN_INTERNAL_PROVIDER_FLAG4);
        sPreviewProgramProjectionMap.put(
                PreviewPrograms.COLUMN_VERSION_NUMBER, PreviewPrograms.COLUMN_VERSION_NUMBER);
        sPreviewProgramProjectionMap.put(
                PreviewPrograms.COLUMN_INTERNAL_PROVIDER_ID,
                PreviewPrograms.COLUMN_INTERNAL_PROVIDER_ID);
        sPreviewProgramProjectionMap.put(
                PreviewPrograms.COLUMN_PREVIEW_VIDEO_URI, PreviewPrograms.COLUMN_PREVIEW_VIDEO_URI);
        sPreviewProgramProjectionMap.put(
                PreviewPrograms.COLUMN_LAST_PLAYBACK_POSITION_MILLIS,
                PreviewPrograms.COLUMN_LAST_PLAYBACK_POSITION_MILLIS);
        sPreviewProgramProjectionMap.put(
                PreviewPrograms.COLUMN_DURATION_MILLIS, PreviewPrograms.COLUMN_DURATION_MILLIS);
        sPreviewProgramProjectionMap.put(
                PreviewPrograms.COLUMN_INTENT_URI, PreviewPrograms.COLUMN_INTENT_URI);
        sPreviewProgramProjectionMap.put(
                PreviewPrograms.COLUMN_WEIGHT, PreviewPrograms.COLUMN_WEIGHT);
        sPreviewProgramProjectionMap.put(
                PreviewPrograms.COLUMN_TRANSIENT, PreviewPrograms.COLUMN_TRANSIENT);
        sPreviewProgramProjectionMap.put(PreviewPrograms.COLUMN_TYPE, PreviewPrograms.COLUMN_TYPE);
        sPreviewProgramProjectionMap.put(
                PreviewPrograms.COLUMN_POSTER_ART_ASPECT_RATIO,
                PreviewPrograms.COLUMN_POSTER_ART_ASPECT_RATIO);
        sPreviewProgramProjectionMap.put(
                PreviewPrograms.COLUMN_THUMBNAIL_ASPECT_RATIO,
                PreviewPrograms.COLUMN_THUMBNAIL_ASPECT_RATIO);
        sPreviewProgramProjectionMap.put(
                PreviewPrograms.COLUMN_LOGO_URI, PreviewPrograms.COLUMN_LOGO_URI);
        sPreviewProgramProjectionMap.put(
                PreviewPrograms.COLUMN_AVAILABILITY, PreviewPrograms.COLUMN_AVAILABILITY);
        sPreviewProgramProjectionMap.put(
                PreviewPrograms.COLUMN_STARTING_PRICE, PreviewPrograms.COLUMN_STARTING_PRICE);
        sPreviewProgramProjectionMap.put(
                PreviewPrograms.COLUMN_OFFER_PRICE, PreviewPrograms.COLUMN_OFFER_PRICE);
        sPreviewProgramProjectionMap.put(
                PreviewPrograms.COLUMN_RELEASE_DATE, PreviewPrograms.COLUMN_RELEASE_DATE);
        sPreviewProgramProjectionMap.put(
                PreviewPrograms.COLUMN_ITEM_COUNT, PreviewPrograms.COLUMN_ITEM_COUNT);
        sPreviewProgramProjectionMap.put(PreviewPrograms.COLUMN_LIVE, PreviewPrograms.COLUMN_LIVE);
        sPreviewProgramProjectionMap.put(
                PreviewPrograms.COLUMN_INTERACTION_TYPE, PreviewPrograms.COLUMN_INTERACTION_TYPE);
        sPreviewProgramProjectionMap.put(
                PreviewPrograms.COLUMN_INTERACTION_COUNT, PreviewPrograms.COLUMN_INTERACTION_COUNT);
        sPreviewProgramProjectionMap.put(
                PreviewPrograms.COLUMN_AUTHOR, PreviewPrograms.COLUMN_AUTHOR);
        sPreviewProgramProjectionMap.put(
                PreviewPrograms.COLUMN_REVIEW_RATING_STYLE,
                PreviewPrograms.COLUMN_REVIEW_RATING_STYLE);
        sPreviewProgramProjectionMap.put(
                PreviewPrograms.COLUMN_REVIEW_RATING, PreviewPrograms.COLUMN_REVIEW_RATING);
        sPreviewProgramProjectionMap.put(
                PreviewPrograms.COLUMN_BROWSABLE, PreviewPrograms.COLUMN_BROWSABLE);
        sPreviewProgramProjectionMap.put(
                PreviewPrograms.COLUMN_CONTENT_ID, PreviewPrograms.COLUMN_CONTENT_ID);

        sWatchNextProgramProjectionMap = new HashMap<>();
        sWatchNextProgramProjectionMap.put(WatchNextPrograms._ID, WatchNextPrograms._ID);
        sWatchNextProgramProjectionMap.put(
                WatchNextPrograms.COLUMN_PACKAGE_NAME, WatchNextPrograms.COLUMN_PACKAGE_NAME);
        sWatchNextProgramProjectionMap.put(
                WatchNextPrograms.COLUMN_TITLE, WatchNextPrograms.COLUMN_TITLE);
        sWatchNextProgramProjectionMap.put(
                WatchNextPrograms.COLUMN_SEASON_DISPLAY_NUMBER,
                WatchNextPrograms.COLUMN_SEASON_DISPLAY_NUMBER);
        sWatchNextProgramProjectionMap.put(
                WatchNextPrograms.COLUMN_SEASON_TITLE, WatchNextPrograms.COLUMN_SEASON_TITLE);
        sWatchNextProgramProjectionMap.put(
                WatchNextPrograms.COLUMN_EPISODE_DISPLAY_NUMBER,
                WatchNextPrograms.COLUMN_EPISODE_DISPLAY_NUMBER);
        sWatchNextProgramProjectionMap.put(
                WatchNextPrograms.COLUMN_EPISODE_TITLE, WatchNextPrograms.COLUMN_EPISODE_TITLE);
        sWatchNextProgramProjectionMap.put(
                WatchNextPrograms.COLUMN_CANONICAL_GENRE, WatchNextPrograms.COLUMN_CANONICAL_GENRE);
        sWatchNextProgramProjectionMap.put(
                WatchNextPrograms.COLUMN_SHORT_DESCRIPTION,
                WatchNextPrograms.COLUMN_SHORT_DESCRIPTION);
        sWatchNextProgramProjectionMap.put(
                WatchNextPrograms.COLUMN_LONG_DESCRIPTION,
                WatchNextPrograms.COLUMN_LONG_DESCRIPTION);
        sWatchNextProgramProjectionMap.put(
                WatchNextPrograms.COLUMN_VIDEO_WIDTH, WatchNextPrograms.COLUMN_VIDEO_WIDTH);
        sWatchNextProgramProjectionMap.put(
                WatchNextPrograms.COLUMN_VIDEO_HEIGHT, WatchNextPrograms.COLUMN_VIDEO_HEIGHT);
        sWatchNextProgramProjectionMap.put(
                WatchNextPrograms.COLUMN_AUDIO_LANGUAGE, WatchNextPrograms.COLUMN_AUDIO_LANGUAGE);
        sWatchNextProgramProjectionMap.put(
                WatchNextPrograms.COLUMN_CONTENT_RATING, WatchNextPrograms.COLUMN_CONTENT_RATING);
        sWatchNextProgramProjectionMap.put(
                WatchNextPrograms.COLUMN_POSTER_ART_URI, WatchNextPrograms.COLUMN_POSTER_ART_URI);
        sWatchNextProgramProjectionMap.put(
                WatchNextPrograms.COLUMN_THUMBNAIL_URI, WatchNextPrograms.COLUMN_THUMBNAIL_URI);
        sWatchNextProgramProjectionMap.put(
                WatchNextPrograms.COLUMN_SEARCHABLE, WatchNextPrograms.COLUMN_SEARCHABLE);
        sWatchNextProgramProjectionMap.put(
                WatchNextPrograms.COLUMN_INTERNAL_PROVIDER_DATA,
                WatchNextPrograms.COLUMN_INTERNAL_PROVIDER_DATA);
        sWatchNextProgramProjectionMap.put(
                WatchNextPrograms.COLUMN_INTERNAL_PROVIDER_FLAG1,
                WatchNextPrograms.COLUMN_INTERNAL_PROVIDER_FLAG1);
        sWatchNextProgramProjectionMap.put(
                WatchNextPrograms.COLUMN_INTERNAL_PROVIDER_FLAG2,
                WatchNextPrograms.COLUMN_INTERNAL_PROVIDER_FLAG2);
        sWatchNextProgramProjectionMap.put(
                WatchNextPrograms.COLUMN_INTERNAL_PROVIDER_FLAG3,
                WatchNextPrograms.COLUMN_INTERNAL_PROVIDER_FLAG3);
        sWatchNextProgramProjectionMap.put(
                WatchNextPrograms.COLUMN_INTERNAL_PROVIDER_FLAG4,
                WatchNextPrograms.COLUMN_INTERNAL_PROVIDER_FLAG4);
        sWatchNextProgramProjectionMap.put(
                WatchNextPrograms.COLUMN_VERSION_NUMBER, WatchNextPrograms.COLUMN_VERSION_NUMBER);
        sWatchNextProgramProjectionMap.put(
                WatchNextPrograms.COLUMN_INTERNAL_PROVIDER_ID,
                WatchNextPrograms.COLUMN_INTERNAL_PROVIDER_ID);
        sWatchNextProgramProjectionMap.put(
                WatchNextPrograms.COLUMN_PREVIEW_VIDEO_URI,
                WatchNextPrograms.COLUMN_PREVIEW_VIDEO_URI);
        sWatchNextProgramProjectionMap.put(
                WatchNextPrograms.COLUMN_LAST_PLAYBACK_POSITION_MILLIS,
                WatchNextPrograms.COLUMN_LAST_PLAYBACK_POSITION_MILLIS);
        sWatchNextProgramProjectionMap.put(
                WatchNextPrograms.COLUMN_DURATION_MILLIS, WatchNextPrograms.COLUMN_DURATION_MILLIS);
        sWatchNextProgramProjectionMap.put(
                WatchNextPrograms.COLUMN_INTENT_URI, WatchNextPrograms.COLUMN_INTENT_URI);
        sWatchNextProgramProjectionMap.put(
                WatchNextPrograms.COLUMN_TRANSIENT, WatchNextPrograms.COLUMN_TRANSIENT);
        sWatchNextProgramProjectionMap.put(
                WatchNextPrograms.COLUMN_TYPE, WatchNextPrograms.COLUMN_TYPE);
        sWatchNextProgramProjectionMap.put(
                WatchNextPrograms.COLUMN_WATCH_NEXT_TYPE, WatchNextPrograms.COLUMN_WATCH_NEXT_TYPE);
        sWatchNextProgramProjectionMap.put(
                WatchNextPrograms.COLUMN_POSTER_ART_ASPECT_RATIO,
                WatchNextPrograms.COLUMN_POSTER_ART_ASPECT_RATIO);
        sWatchNextProgramProjectionMap.put(
                WatchNextPrograms.COLUMN_THUMBNAIL_ASPECT_RATIO,
                WatchNextPrograms.COLUMN_THUMBNAIL_ASPECT_RATIO);
        sWatchNextProgramProjectionMap.put(
                WatchNextPrograms.COLUMN_LOGO_URI, WatchNextPrograms.COLUMN_LOGO_URI);
        sWatchNextProgramProjectionMap.put(
                WatchNextPrograms.COLUMN_AVAILABILITY, WatchNextPrograms.COLUMN_AVAILABILITY);
        sWatchNextProgramProjectionMap.put(
                WatchNextPrograms.COLUMN_STARTING_PRICE, WatchNextPrograms.COLUMN_STARTING_PRICE);
        sWatchNextProgramProjectionMap.put(
                WatchNextPrograms.COLUMN_OFFER_PRICE, WatchNextPrograms.COLUMN_OFFER_PRICE);
        sWatchNextProgramProjectionMap.put(
                WatchNextPrograms.COLUMN_RELEASE_DATE, WatchNextPrograms.COLUMN_RELEASE_DATE);
        sWatchNextProgramProjectionMap.put(
                WatchNextPrograms.COLUMN_ITEM_COUNT, WatchNextPrograms.COLUMN_ITEM_COUNT);
        sWatchNextProgramProjectionMap.put(
                WatchNextPrograms.COLUMN_LIVE, WatchNextPrograms.COLUMN_LIVE);
        sWatchNextProgramProjectionMap.put(
                WatchNextPrograms.COLUMN_INTERACTION_TYPE,
                WatchNextPrograms.COLUMN_INTERACTION_TYPE);
        sWatchNextProgramProjectionMap.put(
                WatchNextPrograms.COLUMN_INTERACTION_COUNT,
                WatchNextPrograms.COLUMN_INTERACTION_COUNT);
        sWatchNextProgramProjectionMap.put(
                WatchNextPrograms.COLUMN_AUTHOR, WatchNextPrograms.COLUMN_AUTHOR);
        sWatchNextProgramProjectionMap.put(
                WatchNextPrograms.COLUMN_REVIEW_RATING_STYLE,
                WatchNextPrograms.COLUMN_REVIEW_RATING_STYLE);
        sWatchNextProgramProjectionMap.put(
                WatchNextPrograms.COLUMN_REVIEW_RATING, WatchNextPrograms.COLUMN_REVIEW_RATING);
        sWatchNextProgramProjectionMap.put(
                WatchNextPrograms.COLUMN_BROWSABLE, WatchNextPrograms.COLUMN_BROWSABLE);
        sWatchNextProgramProjectionMap.put(
                WatchNextPrograms.COLUMN_CONTENT_ID, WatchNextPrograms.COLUMN_CONTENT_ID);
        sWatchNextProgramProjectionMap.put(
                WatchNextPrograms.COLUMN_LAST_ENGAGEMENT_TIME_UTC_MILLIS,
                WatchNextPrograms.COLUMN_LAST_ENGAGEMENT_TIME_UTC_MILLIS);
    }

    // Mapping from broadcast genre to canonical genre.
    private static Map<String, String> sGenreMap;

    private static final String PERMISSION_READ_TV_LISTINGS = "android.permission.READ_TV_LISTINGS";

    private static final String PERMISSION_ACCESS_ALL_EPG_DATA =
            "com.android.providers.tv.permission.ACCESS_ALL_EPG_DATA";

    private static final String PERMISSION_ACCESS_WATCHED_PROGRAMS =
            "com.android.providers.tv.permission.ACCESS_WATCHED_PROGRAMS";

    private static final String CREATE_RECORDED_PROGRAMS_TABLE_SQL =
            "CREATE TABLE "
                    + RECORDED_PROGRAMS_TABLE
                    + " ("
                    + RecordedPrograms._ID
                    + " INTEGER PRIMARY KEY AUTOINCREMENT,"
                    + RecordedPrograms.COLUMN_PACKAGE_NAME
                    + " TEXT NOT NULL,"
                    + RecordedPrograms.COLUMN_INPUT_ID
                    + " TEXT NOT NULL,"
                    + RecordedPrograms.COLUMN_CHANNEL_ID
                    + " INTEGER,"
                    + RecordedPrograms.COLUMN_TITLE
                    + " TEXT,"
                    + RecordedPrograms.COLUMN_SEASON_DISPLAY_NUMBER
                    + " TEXT,"
                    + RecordedPrograms.COLUMN_SEASON_TITLE
                    + " TEXT,"
                    + RecordedPrograms.COLUMN_EPISODE_DISPLAY_NUMBER
                    + " TEXT,"
                    + RecordedPrograms.COLUMN_EPISODE_TITLE
                    + " TEXT,"
                    + RecordedPrograms.COLUMN_START_TIME_UTC_MILLIS
                    + " INTEGER,"
                    + RecordedPrograms.COLUMN_END_TIME_UTC_MILLIS
                    + " INTEGER,"
                    + RecordedPrograms.COLUMN_BROADCAST_GENRE
                    + " TEXT,"
                    + RecordedPrograms.COLUMN_CANONICAL_GENRE
                    + " TEXT,"
                    + RecordedPrograms.COLUMN_SHORT_DESCRIPTION
                    + " TEXT,"
                    + RecordedPrograms.COLUMN_LONG_DESCRIPTION
                    + " TEXT,"
                    + RecordedPrograms.COLUMN_VIDEO_WIDTH
                    + " INTEGER,"
                    + RecordedPrograms.COLUMN_VIDEO_HEIGHT
                    + " INTEGER,"
                    + RecordedPrograms.COLUMN_AUDIO_LANGUAGE
                    + " TEXT,"
                    + RecordedPrograms.COLUMN_CONTENT_RATING
                    + " TEXT,"
                    + RecordedPrograms.COLUMN_POSTER_ART_URI
                    + " TEXT,"
                    + RecordedPrograms.COLUMN_THUMBNAIL_URI
                    + " TEXT,"
                    + RecordedPrograms.COLUMN_SEARCHABLE
                    + " INTEGER NOT NULL DEFAULT 1,"
                    + RecordedPrograms.COLUMN_RECORDING_DATA_URI
                    + " TEXT,"
                    + RecordedPrograms.COLUMN_RECORDING_DATA_BYTES
                    + " INTEGER,"
                    + RecordedPrograms.COLUMN_RECORDING_DURATION_MILLIS
                    + " INTEGER,"
                    + RecordedPrograms.COLUMN_RECORDING_EXPIRE_TIME_UTC_MILLIS
                    + " INTEGER,"
                    + RecordedPrograms.COLUMN_INTERNAL_PROVIDER_DATA
                    + " BLOB,"
                    + RecordedPrograms.COLUMN_INTERNAL_PROVIDER_FLAG1
                    + " INTEGER,"
                    + RecordedPrograms.COLUMN_INTERNAL_PROVIDER_FLAG2
                    + " INTEGER,"
                    + RecordedPrograms.COLUMN_INTERNAL_PROVIDER_FLAG3
                    + " INTEGER,"
                    + RecordedPrograms.COLUMN_INTERNAL_PROVIDER_FLAG4
                    + " INTEGER,"
                    + RecordedPrograms.COLUMN_VERSION_NUMBER
                    + " INTEGER,"
                    + RecordedPrograms.COLUMN_REVIEW_RATING_STYLE
                    + " INTEGER,"
                    + RecordedPrograms.COLUMN_REVIEW_RATING
                    + " TEXT,"
                    + "FOREIGN KEY("
                    + RecordedPrograms.COLUMN_CHANNEL_ID
                    + ") "
                    + "REFERENCES "
                    + CHANNELS_TABLE
                    + "("
                    + Channels._ID
                    + ") "
                    + "ON UPDATE CASCADE ON DELETE SET NULL);";

    private static final String CREATE_PREVIEW_PROGRAMS_TABLE_SQL =
            "CREATE TABLE "
                    + PREVIEW_PROGRAMS_TABLE
                    + " ("
                    + PreviewPrograms._ID
                    + " INTEGER PRIMARY KEY AUTOINCREMENT,"
                    + PreviewPrograms.COLUMN_PACKAGE_NAME
                    + " TEXT NOT NULL,"
                    + PreviewPrograms.COLUMN_CHANNEL_ID
                    + " INTEGER,"
                    + PreviewPrograms.COLUMN_TITLE
                    + " TEXT,"
                    + PreviewPrograms.COLUMN_SEASON_DISPLAY_NUMBER
                    + " TEXT,"
                    + PreviewPrograms.COLUMN_SEASON_TITLE
                    + " TEXT,"
                    + PreviewPrograms.COLUMN_EPISODE_DISPLAY_NUMBER
                    + " TEXT,"
                    + PreviewPrograms.COLUMN_EPISODE_TITLE
                    + " TEXT,"
                    + PreviewPrograms.COLUMN_CANONICAL_GENRE
                    + " TEXT,"
                    + PreviewPrograms.COLUMN_SHORT_DESCRIPTION
                    + " TEXT,"
                    + PreviewPrograms.COLUMN_LONG_DESCRIPTION
                    + " TEXT,"
                    + PreviewPrograms.COLUMN_VIDEO_WIDTH
                    + " INTEGER,"
                    + PreviewPrograms.COLUMN_VIDEO_HEIGHT
                    + " INTEGER,"
                    + PreviewPrograms.COLUMN_AUDIO_LANGUAGE
                    + " TEXT,"
                    + PreviewPrograms.COLUMN_CONTENT_RATING
                    + " TEXT,"
                    + PreviewPrograms.COLUMN_POSTER_ART_URI
                    + " TEXT,"
                    + PreviewPrograms.COLUMN_THUMBNAIL_URI
                    + " TEXT,"
                    + PreviewPrograms.COLUMN_SEARCHABLE
                    + " INTEGER NOT NULL DEFAULT 1,"
                    + PreviewPrograms.COLUMN_INTERNAL_PROVIDER_DATA
                    + " BLOB,"
                    + PreviewPrograms.COLUMN_INTERNAL_PROVIDER_FLAG1
                    + " INTEGER,"
                    + PreviewPrograms.COLUMN_INTERNAL_PROVIDER_FLAG2
                    + " INTEGER,"
                    + PreviewPrograms.COLUMN_INTERNAL_PROVIDER_FLAG3
                    + " INTEGER,"
                    + PreviewPrograms.COLUMN_INTERNAL_PROVIDER_FLAG4
                    + " INTEGER,"
                    + PreviewPrograms.COLUMN_VERSION_NUMBER
                    + " INTEGER,"
                    + PreviewPrograms.COLUMN_INTERNAL_PROVIDER_ID
                    + " TEXT,"
                    + PreviewPrograms.COLUMN_PREVIEW_VIDEO_URI
                    + " TEXT,"
                    + PreviewPrograms.COLUMN_LAST_PLAYBACK_POSITION_MILLIS
                    + " INTEGER,"
                    + PreviewPrograms.COLUMN_DURATION_MILLIS
                    + " INTEGER,"
                    + PreviewPrograms.COLUMN_INTENT_URI
                    + " TEXT,"
                    + PreviewPrograms.COLUMN_WEIGHT
                    + " INTEGER,"
                    + PreviewPrograms.COLUMN_TRANSIENT
                    + " INTEGER NOT NULL DEFAULT 0,"
                    + PreviewPrograms.COLUMN_TYPE
                    + " INTEGER NOT NULL,"
                    + PreviewPrograms.COLUMN_POSTER_ART_ASPECT_RATIO
                    + " INTEGER,"
                    + PreviewPrograms.COLUMN_THUMBNAIL_ASPECT_RATIO
                    + " INTEGER,"
                    + PreviewPrograms.COLUMN_LOGO_URI
                    + " TEXT,"
                    + PreviewPrograms.COLUMN_AVAILABILITY
                    + " INTERGER,"
                    + PreviewPrograms.COLUMN_STARTING_PRICE
                    + " TEXT,"
                    + PreviewPrograms.COLUMN_OFFER_PRICE
                    + " TEXT,"
                    + PreviewPrograms.COLUMN_RELEASE_DATE
                    + " TEXT,"
                    + PreviewPrograms.COLUMN_ITEM_COUNT
                    + " INTEGER,"
                    + PreviewPrograms.COLUMN_LIVE
                    + " INTEGER NOT NULL DEFAULT 0,"
                    + PreviewPrograms.COLUMN_INTERACTION_TYPE
                    + " INTEGER,"
                    + PreviewPrograms.COLUMN_INTERACTION_COUNT
                    + " INTEGER,"
                    + PreviewPrograms.COLUMN_AUTHOR
                    + " TEXT,"
                    + PreviewPrograms.COLUMN_REVIEW_RATING_STYLE
                    + " INTEGER,"
                    + PreviewPrograms.COLUMN_REVIEW_RATING
                    + " TEXT,"
                    + PreviewPrograms.COLUMN_BROWSABLE
                    + " INTEGER NOT NULL DEFAULT 1,"
                    + PreviewPrograms.COLUMN_CONTENT_ID
                    + " TEXT,"
                    + "FOREIGN KEY("
                    + PreviewPrograms.COLUMN_CHANNEL_ID
                    + ","
                    + PreviewPrograms.COLUMN_PACKAGE_NAME
                    + ") REFERENCES "
                    + CHANNELS_TABLE
                    + "("
                    + Channels._ID
                    + ","
                    + Channels.COLUMN_PACKAGE_NAME
                    + ") ON UPDATE CASCADE ON DELETE CASCADE"
                    + ");";
    private static final String CREATE_PREVIEW_PROGRAMS_PACKAGE_NAME_INDEX_SQL =
            "CREATE INDEX preview_programs_package_name_index ON "
                    + PREVIEW_PROGRAMS_TABLE
                    + "("
                    + PreviewPrograms.COLUMN_PACKAGE_NAME
                    + ");";
    private static final String CREATE_PREVIEW_PROGRAMS_CHANNEL_ID_INDEX_SQL =
            "CREATE INDEX preview_programs_id_index ON "
                    + PREVIEW_PROGRAMS_TABLE
                    + "("
                    + PreviewPrograms.COLUMN_CHANNEL_ID
                    + ");";
    private static final String CREATE_WATCH_NEXT_PROGRAMS_TABLE_SQL =
            "CREATE TABLE "
                    + WATCH_NEXT_PROGRAMS_TABLE
                    + " ("
                    + WatchNextPrograms._ID
                    + " INTEGER PRIMARY KEY AUTOINCREMENT,"
                    + WatchNextPrograms.COLUMN_PACKAGE_NAME
                    + " TEXT NOT NULL,"
                    + WatchNextPrograms.COLUMN_TITLE
                    + " TEXT,"
                    + WatchNextPrograms.COLUMN_SEASON_DISPLAY_NUMBER
                    + " TEXT,"
                    + WatchNextPrograms.COLUMN_SEASON_TITLE
                    + " TEXT,"
                    + WatchNextPrograms.COLUMN_EPISODE_DISPLAY_NUMBER
                    + " TEXT,"
                    + WatchNextPrograms.COLUMN_EPISODE_TITLE
                    + " TEXT,"
                    + WatchNextPrograms.COLUMN_CANONICAL_GENRE
                    + " TEXT,"
                    + WatchNextPrograms.COLUMN_SHORT_DESCRIPTION
                    + " TEXT,"
                    + WatchNextPrograms.COLUMN_LONG_DESCRIPTION
                    + " TEXT,"
                    + WatchNextPrograms.COLUMN_VIDEO_WIDTH
                    + " INTEGER,"
                    + WatchNextPrograms.COLUMN_VIDEO_HEIGHT
                    + " INTEGER,"
                    + WatchNextPrograms.COLUMN_AUDIO_LANGUAGE
                    + " TEXT,"
                    + WatchNextPrograms.COLUMN_CONTENT_RATING
                    + " TEXT,"
                    + WatchNextPrograms.COLUMN_POSTER_ART_URI
                    + " TEXT,"
                    + WatchNextPrograms.COLUMN_THUMBNAIL_URI
                    + " TEXT,"
                    + WatchNextPrograms.COLUMN_SEARCHABLE
                    + " INTEGER NOT NULL DEFAULT 1,"
                    + WatchNextPrograms.COLUMN_INTERNAL_PROVIDER_DATA
                    + " BLOB,"
                    + WatchNextPrograms.COLUMN_INTERNAL_PROVIDER_FLAG1
                    + " INTEGER,"
                    + WatchNextPrograms.COLUMN_INTERNAL_PROVIDER_FLAG2
                    + " INTEGER,"
                    + WatchNextPrograms.COLUMN_INTERNAL_PROVIDER_FLAG3
                    + " INTEGER,"
                    + WatchNextPrograms.COLUMN_INTERNAL_PROVIDER_FLAG4
                    + " INTEGER,"
                    + WatchNextPrograms.COLUMN_VERSION_NUMBER
                    + " INTEGER,"
                    + WatchNextPrograms.COLUMN_INTERNAL_PROVIDER_ID
                    + " TEXT,"
                    + WatchNextPrograms.COLUMN_PREVIEW_VIDEO_URI
                    + " TEXT,"
                    + WatchNextPrograms.COLUMN_LAST_PLAYBACK_POSITION_MILLIS
                    + " INTEGER,"
                    + WatchNextPrograms.COLUMN_DURATION_MILLIS
                    + " INTEGER,"
                    + WatchNextPrograms.COLUMN_INTENT_URI
                    + " TEXT,"
                    + WatchNextPrograms.COLUMN_TRANSIENT
                    + " INTEGER NOT NULL DEFAULT 0,"
                    + WatchNextPrograms.COLUMN_TYPE
                    + " INTEGER NOT NULL,"
                    + WatchNextPrograms.COLUMN_WATCH_NEXT_TYPE
                    + " INTEGER,"
                    + WatchNextPrograms.COLUMN_POSTER_ART_ASPECT_RATIO
                    + " INTEGER,"
                    + WatchNextPrograms.COLUMN_THUMBNAIL_ASPECT_RATIO
                    + " INTEGER,"
                    + WatchNextPrograms.COLUMN_LOGO_URI
                    + " TEXT,"
                    + WatchNextPrograms.COLUMN_AVAILABILITY
                    + " INTEGER,"
                    + WatchNextPrograms.COLUMN_STARTING_PRICE
                    + " TEXT,"
                    + WatchNextPrograms.COLUMN_OFFER_PRICE
                    + " TEXT,"
                    + WatchNextPrograms.COLUMN_RELEASE_DATE
                    + " TEXT,"
                    + WatchNextPrograms.COLUMN_ITEM_COUNT
                    + " INTEGER,"
                    + WatchNextPrograms.COLUMN_LIVE
                    + " INTEGER NOT NULL DEFAULT 0,"
                    + WatchNextPrograms.COLUMN_INTERACTION_TYPE
                    + " INTEGER,"
                    + WatchNextPrograms.COLUMN_INTERACTION_COUNT
                    + " INTEGER,"
                    + WatchNextPrograms.COLUMN_AUTHOR
                    + " TEXT,"
                    + WatchNextPrograms.COLUMN_REVIEW_RATING_STYLE
                    + " INTEGER,"
                    + WatchNextPrograms.COLUMN_REVIEW_RATING
                    + " TEXT,"
                    + WatchNextPrograms.COLUMN_BROWSABLE
                    + " INTEGER NOT NULL DEFAULT 1,"
                    + WatchNextPrograms.COLUMN_CONTENT_ID
                    + " TEXT,"
                    + WatchNextPrograms.COLUMN_LAST_ENGAGEMENT_TIME_UTC_MILLIS
                    + " INTEGER"
                    + ");";
    private static final String CREATE_WATCH_NEXT_PROGRAMS_PACKAGE_NAME_INDEX_SQL =
            "CREATE INDEX watch_next_programs_package_name_index ON "
                    + WATCH_NEXT_PROGRAMS_TABLE
                    + "("
                    + WatchNextPrograms.COLUMN_PACKAGE_NAME
                    + ");";

    private String mCallingPackage = "com.android.tv";

    static class DatabaseHelper extends SQLiteOpenHelper {
        private Context mContext;

        public static synchronized DatabaseHelper createInstance(Context context) {
            return new DatabaseHelper(context);
        }

        private DatabaseHelper(Context context) {
            this(context, DATABASE_NAME, DATABASE_VERSION);
        }

        @VisibleForTesting
        DatabaseHelper(Context context, String databaseName, int databaseVersion) {
            super(context, databaseName, null, databaseVersion);
            mContext = context;
        }

        @Override
        public void onConfigure(SQLiteDatabase db) {
            db.setForeignKeyConstraintsEnabled(true);
        }

        @Override
        public void onCreate(SQLiteDatabase db) {
            if (DEBUG) {
                Log.d(TAG, "Creating database");
            }
            // Set up the database schema.
            db.execSQL(
                    "CREATE TABLE "
                            + CHANNELS_TABLE
                            + " ("
                            + Channels._ID
                            + " INTEGER PRIMARY KEY AUTOINCREMENT,"
                            + Channels.COLUMN_PACKAGE_NAME
                            + " TEXT NOT NULL,"
                            + Channels.COLUMN_INPUT_ID
                            + " TEXT NOT NULL,"
                            + Channels.COLUMN_TYPE
                            + " TEXT NOT NULL DEFAULT '"
                            + Channels.TYPE_OTHER
                            + "',"
                            + Channels.COLUMN_SERVICE_TYPE
                            + " TEXT NOT NULL DEFAULT '"
                            + Channels.SERVICE_TYPE_AUDIO_VIDEO
                            + "',"
                            + Channels.COLUMN_ORIGINAL_NETWORK_ID
                            + " INTEGER NOT NULL DEFAULT 0,"
                            + Channels.COLUMN_TRANSPORT_STREAM_ID
                            + " INTEGER NOT NULL DEFAULT 0,"
                            + Channels.COLUMN_SERVICE_ID
                            + " INTEGER NOT NULL DEFAULT 0,"
                            + Channels.COLUMN_DISPLAY_NUMBER
                            + " TEXT,"
                            + Channels.COLUMN_DISPLAY_NAME
                            + " TEXT,"
                            + Channels.COLUMN_NETWORK_AFFILIATION
                            + " TEXT,"
                            + Channels.COLUMN_DESCRIPTION
                            + " TEXT,"
                            + Channels.COLUMN_VIDEO_FORMAT
                            + " TEXT,"
                            + Channels.COLUMN_BROWSABLE
                            + " INTEGER NOT NULL DEFAULT 0,"
                            + Channels.COLUMN_SEARCHABLE
                            + " INTEGER NOT NULL DEFAULT 1,"
                            + Channels.COLUMN_LOCKED
                            + " INTEGER NOT NULL DEFAULT 0,"
                            + Channels.COLUMN_APP_LINK_ICON_URI
                            + " TEXT,"
                            + Channels.COLUMN_APP_LINK_POSTER_ART_URI
                            + " TEXT,"
                            + Channels.COLUMN_APP_LINK_TEXT
                            + " TEXT,"
                            + Channels.COLUMN_APP_LINK_COLOR
                            + " INTEGER,"
                            + Channels.COLUMN_APP_LINK_INTENT_URI
                            + " TEXT,"
                            + Channels.COLUMN_INTERNAL_PROVIDER_DATA
                            + " BLOB,"
                            + Channels.COLUMN_INTERNAL_PROVIDER_FLAG1
                            + " INTEGER,"
                            + Channels.COLUMN_INTERNAL_PROVIDER_FLAG2
                            + " INTEGER,"
                            + Channels.COLUMN_INTERNAL_PROVIDER_FLAG3
                            + " INTEGER,"
                            + Channels.COLUMN_INTERNAL_PROVIDER_FLAG4
                            + " INTEGER,"
                            + CHANNELS_COLUMN_LOGO
                            + " BLOB,"
                            + Channels.COLUMN_VERSION_NUMBER
                            + " INTEGER,"
                            + Channels.COLUMN_TRANSIENT
                            + " INTEGER NOT NULL DEFAULT 0,"
                            + Channels.COLUMN_INTERNAL_PROVIDER_ID
                            + " TEXT,"
                            // Needed for foreign keys in other tables.
                            + "UNIQUE("
                            + Channels._ID
                            + ","
                            + Channels.COLUMN_PACKAGE_NAME
                            + ")"
                            + ");");
            db.execSQL(
                    "CREATE TABLE "
                            + PROGRAMS_TABLE
                            + " ("
                            + Programs._ID
                            + " INTEGER PRIMARY KEY AUTOINCREMENT,"
                            + Programs.COLUMN_PACKAGE_NAME
                            + " TEXT NOT NULL,"
                            + Programs.COLUMN_CHANNEL_ID
                            + " INTEGER,"
                            + Programs.COLUMN_TITLE
                            + " TEXT,"
                            + Programs.COLUMN_SEASON_DISPLAY_NUMBER
                            + " TEXT,"
                            + Programs.COLUMN_SEASON_TITLE
                            + " TEXT,"
                            + Programs.COLUMN_EPISODE_DISPLAY_NUMBER
                            + " TEXT,"
                            + Programs.COLUMN_EPISODE_TITLE
                            + " TEXT,"
                            + Programs.COLUMN_START_TIME_UTC_MILLIS
                            + " INTEGER,"
                            + Programs.COLUMN_END_TIME_UTC_MILLIS
                            + " INTEGER,"
                            + Programs.COLUMN_BROADCAST_GENRE
                            + " TEXT,"
                            + Programs.COLUMN_CANONICAL_GENRE
                            + " TEXT,"
                            + Programs.COLUMN_SHORT_DESCRIPTION
                            + " TEXT,"
                            + Programs.COLUMN_LONG_DESCRIPTION
                            + " TEXT,"
                            + Programs.COLUMN_VIDEO_WIDTH
                            + " INTEGER,"
                            + Programs.COLUMN_VIDEO_HEIGHT
                            + " INTEGER,"
                            + Programs.COLUMN_AUDIO_LANGUAGE
                            + " TEXT,"
                            + Programs.COLUMN_CONTENT_RATING
                            + " TEXT,"
                            + Programs.COLUMN_POSTER_ART_URI
                            + " TEXT,"
                            + Programs.COLUMN_THUMBNAIL_URI
                            + " TEXT,"
                            + Programs.COLUMN_SEARCHABLE
                            + " INTEGER NOT NULL DEFAULT 1,"
                            + Programs.COLUMN_RECORDING_PROHIBITED
                            + " INTEGER NOT NULL DEFAULT 0,"
                            + Programs.COLUMN_INTERNAL_PROVIDER_DATA
                            + " BLOB,"
                            + Programs.COLUMN_INTERNAL_PROVIDER_FLAG1
                            + " INTEGER,"
                            + Programs.COLUMN_INTERNAL_PROVIDER_FLAG2
                            + " INTEGER,"
                            + Programs.COLUMN_INTERNAL_PROVIDER_FLAG3
                            + " INTEGER,"
                            + Programs.COLUMN_INTERNAL_PROVIDER_FLAG4
                            + " INTEGER,"
                            + Programs.COLUMN_REVIEW_RATING_STYLE
                            + " INTEGER,"
                            + Programs.COLUMN_REVIEW_RATING
                            + " TEXT,"
                            + Programs.COLUMN_VERSION_NUMBER
                            + " INTEGER,"
                            + "FOREIGN KEY("
                            + Programs.COLUMN_CHANNEL_ID
                            + ","
                            + Programs.COLUMN_PACKAGE_NAME
                            + ") REFERENCES "
                            + CHANNELS_TABLE
                            + "("
                            + Channels._ID
                            + ","
                            + Channels.COLUMN_PACKAGE_NAME
                            + ") ON UPDATE CASCADE ON DELETE CASCADE"
                            + ");");
            db.execSQL(
                    "CREATE INDEX "
                            + PROGRAMS_TABLE_PACKAGE_NAME_INDEX
                            + " ON "
                            + PROGRAMS_TABLE
                            + "("
                            + Programs.COLUMN_PACKAGE_NAME
                            + ");");
            db.execSQL(
                    "CREATE INDEX "
                            + PROGRAMS_TABLE_CHANNEL_ID_INDEX
                            + " ON "
                            + PROGRAMS_TABLE
                            + "("
                            + Programs.COLUMN_CHANNEL_ID
                            + ");");
            db.execSQL(
                    "CREATE INDEX "
                            + PROGRAMS_TABLE_START_TIME_INDEX
                            + " ON "
                            + PROGRAMS_TABLE
                            + "("
                            + Programs.COLUMN_START_TIME_UTC_MILLIS
                            + ");");
            db.execSQL(
                    "CREATE INDEX "
                            + PROGRAMS_TABLE_END_TIME_INDEX
                            + " ON "
                            + PROGRAMS_TABLE
                            + "("
                            + Programs.COLUMN_END_TIME_UTC_MILLIS
                            + ");");
            db.execSQL(
                    "CREATE TABLE "
                            + WATCHED_PROGRAMS_TABLE
                            + " ("
                            + WatchedPrograms._ID
                            + " INTEGER PRIMARY KEY AUTOINCREMENT,"
                            + WatchedPrograms.COLUMN_PACKAGE_NAME
                            + " TEXT NOT NULL,"
                            + WatchedPrograms.COLUMN_WATCH_START_TIME_UTC_MILLIS
                            + " INTEGER NOT NULL DEFAULT 0,"
                            + WatchedPrograms.COLUMN_WATCH_END_TIME_UTC_MILLIS
                            + " INTEGER NOT NULL DEFAULT 0,"
                            + WatchedPrograms.COLUMN_CHANNEL_ID
                            + " INTEGER,"
                            + WatchedPrograms.COLUMN_TITLE
                            + " TEXT,"
                            + WatchedPrograms.COLUMN_START_TIME_UTC_MILLIS
                            + " INTEGER,"
                            + WatchedPrograms.COLUMN_END_TIME_UTC_MILLIS
                            + " INTEGER,"
                            + WatchedPrograms.COLUMN_DESCRIPTION
                            + " TEXT,"
                            + WatchedPrograms.COLUMN_INTERNAL_TUNE_PARAMS
                            + " TEXT,"
                            + WatchedPrograms.COLUMN_INTERNAL_SESSION_TOKEN
                            + " TEXT NOT NULL,"
                            + WATCHED_PROGRAMS_COLUMN_CONSOLIDATED
                            + " INTEGER NOT NULL DEFAULT 0,"
                            + "FOREIGN KEY("
                            + WatchedPrograms.COLUMN_CHANNEL_ID
                            + ","
                            + WatchedPrograms.COLUMN_PACKAGE_NAME
                            + ") REFERENCES "
                            + CHANNELS_TABLE
                            + "("
                            + Channels._ID
                            + ","
                            + Channels.COLUMN_PACKAGE_NAME
                            + ") ON UPDATE CASCADE ON DELETE CASCADE"
                            + ");");
            db.execSQL(
                    "CREATE INDEX "
                            + WATCHED_PROGRAMS_TABLE_CHANNEL_ID_INDEX
                            + " ON "
                            + WATCHED_PROGRAMS_TABLE
                            + "("
                            + WatchedPrograms.COLUMN_CHANNEL_ID
                            + ");");
            db.execSQL(CREATE_RECORDED_PROGRAMS_TABLE_SQL);
            db.execSQL(CREATE_PREVIEW_PROGRAMS_TABLE_SQL);
            db.execSQL(CREATE_PREVIEW_PROGRAMS_PACKAGE_NAME_INDEX_SQL);
            db.execSQL(CREATE_PREVIEW_PROGRAMS_CHANNEL_ID_INDEX_SQL);
            db.execSQL(CREATE_WATCH_NEXT_PROGRAMS_TABLE_SQL);
            db.execSQL(CREATE_WATCH_NEXT_PROGRAMS_PACKAGE_NAME_INDEX_SQL);
        }

        @Override
        public void onUpgrade(SQLiteDatabase db, int oldVersion, int newVersion) {
            if (oldVersion < 23) {
                Log.i(
                        TAG,
                        "Upgrading from version "
                                + oldVersion
                                + " to "
                                + newVersion
                                + ", data will be lost!");
                db.execSQL("DROP TABLE IF EXISTS " + DELETED_CHANNELS_TABLE);
                db.execSQL("DROP TABLE IF EXISTS " + WATCHED_PROGRAMS_TABLE);
                db.execSQL("DROP TABLE IF EXISTS " + PROGRAMS_TABLE);
                db.execSQL("DROP TABLE IF EXISTS " + CHANNELS_TABLE);

                onCreate(db);
                return;
            }

            Log.i(TAG, "Upgrading from version " + oldVersion + " to " + newVersion + ".");
            if (oldVersion <= 23) {
                db.execSQL(
                        "ALTER TABLE "
                                + CHANNELS_TABLE
                                + " ADD "
                                + Channels.COLUMN_INTERNAL_PROVIDER_FLAG1
                                + " INTEGER;");
                db.execSQL(
                        "ALTER TABLE "
                                + CHANNELS_TABLE
                                + " ADD "
                                + Channels.COLUMN_INTERNAL_PROVIDER_FLAG2
                                + " INTEGER;");
                db.execSQL(
                        "ALTER TABLE "
                                + CHANNELS_TABLE
                                + " ADD "
                                + Channels.COLUMN_INTERNAL_PROVIDER_FLAG3
                                + " INTEGER;");
                db.execSQL(
                        "ALTER TABLE "
                                + CHANNELS_TABLE
                                + " ADD "
                                + Channels.COLUMN_INTERNAL_PROVIDER_FLAG4
                                + " INTEGER;");
            }
            if (oldVersion <= 24) {
                db.execSQL(
                        "ALTER TABLE "
                                + PROGRAMS_TABLE
                                + " ADD "
                                + Programs.COLUMN_INTERNAL_PROVIDER_FLAG1
                                + " INTEGER;");
                db.execSQL(
                        "ALTER TABLE "
                                + PROGRAMS_TABLE
                                + " ADD "
                                + Programs.COLUMN_INTERNAL_PROVIDER_FLAG2
                                + " INTEGER;");
                db.execSQL(
                        "ALTER TABLE "
                                + PROGRAMS_TABLE
                                + " ADD "
                                + Programs.COLUMN_INTERNAL_PROVIDER_FLAG3
                                + " INTEGER;");
                db.execSQL(
                        "ALTER TABLE "
                                + PROGRAMS_TABLE
                                + " ADD "
                                + Programs.COLUMN_INTERNAL_PROVIDER_FLAG4
                                + " INTEGER;");
            }
            if (oldVersion <= 25) {
                db.execSQL(
                        "ALTER TABLE "
                                + CHANNELS_TABLE
                                + " ADD "
                                + Channels.COLUMN_APP_LINK_ICON_URI
                                + " TEXT;");
                db.execSQL(
                        "ALTER TABLE "
                                + CHANNELS_TABLE
                                + " ADD "
                                + Channels.COLUMN_APP_LINK_POSTER_ART_URI
                                + " TEXT;");
                db.execSQL(
                        "ALTER TABLE "
                                + CHANNELS_TABLE
                                + " ADD "
                                + Channels.COLUMN_APP_LINK_TEXT
                                + " TEXT;");
                db.execSQL(
                        "ALTER TABLE "
                                + CHANNELS_TABLE
                                + " ADD "
                                + Channels.COLUMN_APP_LINK_COLOR
                                + " INTEGER;");
                db.execSQL(
                        "ALTER TABLE "
                                + CHANNELS_TABLE
                                + " ADD "
                                + Channels.COLUMN_APP_LINK_INTENT_URI
                                + " TEXT;");
                db.execSQL(
                        "ALTER TABLE "
                                + PROGRAMS_TABLE
                                + " ADD "
                                + Programs.COLUMN_SEARCHABLE
                                + " INTEGER NOT NULL DEFAULT 1;");
            }
            if (oldVersion <= 28) {
                db.execSQL(
                        "ALTER TABLE "
                                + PROGRAMS_TABLE
                                + " ADD "
                                + Programs.COLUMN_SEASON_TITLE
                                + " TEXT;");
                migrateIntegerColumnToTextColumn(
                        db,
                        PROGRAMS_TABLE,
                        Programs.COLUMN_SEASON_NUMBER,
                        Programs.COLUMN_SEASON_DISPLAY_NUMBER);
                migrateIntegerColumnToTextColumn(
                        db,
                        PROGRAMS_TABLE,
                        Programs.COLUMN_EPISODE_NUMBER,
                        Programs.COLUMN_EPISODE_DISPLAY_NUMBER);
            }
            if (oldVersion <= 29) {
                db.execSQL("DROP TABLE IF EXISTS " + RECORDED_PROGRAMS_TABLE);
                db.execSQL(CREATE_RECORDED_PROGRAMS_TABLE_SQL);
            }
            if (oldVersion <= 30) {
                db.execSQL(
                        "ALTER TABLE "
                                + PROGRAMS_TABLE
                                + " ADD "
                                + Programs.COLUMN_RECORDING_PROHIBITED
                                + " INTEGER NOT NULL DEFAULT 0;");
            }
            if (oldVersion <= 32) {
                db.execSQL(
                        "ALTER TABLE "
                                + CHANNELS_TABLE
                                + " ADD "
                                + Channels.COLUMN_TRANSIENT
                                + " INTEGER NOT NULL DEFAULT 0;");
                db.execSQL(
                        "ALTER TABLE "
                                + CHANNELS_TABLE
                                + " ADD "
                                + Channels.COLUMN_INTERNAL_PROVIDER_ID
                                + " TEXT;");
                db.execSQL(
                        "ALTER TABLE "
                                + PROGRAMS_TABLE
                                + " ADD "
                                + Programs.COLUMN_REVIEW_RATING_STYLE
                                + " INTEGER;");
                db.execSQL(
                        "ALTER TABLE "
                                + PROGRAMS_TABLE
                                + " ADD "
                                + Programs.COLUMN_REVIEW_RATING
                                + " TEXT;");
                if (oldVersion > 29) {
                    db.execSQL(
                            "ALTER TABLE "
                                    + RECORDED_PROGRAMS_TABLE
                                    + " ADD "
                                    + RecordedPrograms.COLUMN_REVIEW_RATING_STYLE
                                    + " INTEGER;");
                    db.execSQL(
                            "ALTER TABLE "
                                    + RECORDED_PROGRAMS_TABLE
                                    + " ADD "
                                    + RecordedPrograms.COLUMN_REVIEW_RATING
                                    + " TEXT;");
                }
            }
            if (oldVersion <= 33) {
                db.execSQL("DROP TABLE IF EXISTS " + PREVIEW_PROGRAMS_TABLE);
                db.execSQL("DROP TABLE IF EXISTS " + WATCH_NEXT_PROGRAMS_TABLE);
                db.execSQL(CREATE_PREVIEW_PROGRAMS_TABLE_SQL);
                db.execSQL(CREATE_PREVIEW_PROGRAMS_PACKAGE_NAME_INDEX_SQL);
                db.execSQL(CREATE_PREVIEW_PROGRAMS_CHANNEL_ID_INDEX_SQL);
                db.execSQL(CREATE_WATCH_NEXT_PROGRAMS_TABLE_SQL);
                db.execSQL(CREATE_WATCH_NEXT_PROGRAMS_PACKAGE_NAME_INDEX_SQL);
            }
            Log.i(TAG, "Upgrading from version " + oldVersion + " to " + newVersion + " is done.");
        }

        @Override
        public void onOpen(SQLiteDatabase db) {
            // Call a static method on the TvProvider because changes to sInitialized must
            // be guarded by a lock on the class.
            initOnOpenIfNeeded(mContext, db);
        }

        private static void migrateIntegerColumnToTextColumn(
                SQLiteDatabase db, String table, String integerColumn, String textColumn) {
            db.execSQL("ALTER TABLE " + table + " ADD " + textColumn + " TEXT;");
            db.execSQL(
                    "UPDATE "
                            + table
                            + " SET "
                            + textColumn
                            + " = CAST("
                            + integerColumn
                            + " AS TEXT);");
        }
    }

    private DatabaseHelper mOpenHelper;
    private static SharedPreferences sBlockedPackagesSharedPreference;
    private static Map<String, Boolean> sBlockedPackages;

    @Override
    public boolean onCreate() {
        if (DEBUG) {
            Log.d(TAG, "Creating TvProvider");
        }
        mOpenHelper = DatabaseHelper.createInstance(getContext());
        return true;
    }

    @VisibleForTesting
    String getCallingPackage_() {
        return mCallingPackage;
    }

    public void setCallingPackage(String packageName) {
        mCallingPackage = packageName;
    }

    void setOpenHelper(DatabaseHelper helper) {
        mOpenHelper = helper;
    }

    @Override
    public String getType(Uri uri) {
        switch (sUriMatcher.match(uri)) {
            case MATCH_CHANNEL:
                return Channels.CONTENT_TYPE;
            case MATCH_CHANNEL_ID:
                return Channels.CONTENT_ITEM_TYPE;
            case MATCH_CHANNEL_ID_LOGO:
                return "image/png";
            case MATCH_PASSTHROUGH_ID:
                return Channels.CONTENT_ITEM_TYPE;
            case MATCH_PROGRAM:
                return Programs.CONTENT_TYPE;
            case MATCH_PROGRAM_ID:
                return Programs.CONTENT_ITEM_TYPE;
            case MATCH_WATCHED_PROGRAM:
                return WatchedPrograms.CONTENT_TYPE;
            case MATCH_WATCHED_PROGRAM_ID:
                return WatchedPrograms.CONTENT_ITEM_TYPE;
            case MATCH_RECORDED_PROGRAM:
                return RecordedPrograms.CONTENT_TYPE;
            case MATCH_RECORDED_PROGRAM_ID:
                return RecordedPrograms.CONTENT_ITEM_TYPE;
            case MATCH_PREVIEW_PROGRAM:
                return PreviewPrograms.CONTENT_TYPE;
            case MATCH_PREVIEW_PROGRAM_ID:
                return PreviewPrograms.CONTENT_ITEM_TYPE;
            case MATCH_WATCH_NEXT_PROGRAM:
                return WatchNextPrograms.CONTENT_TYPE;
            case MATCH_WATCH_NEXT_PROGRAM_ID:
                return WatchNextPrograms.CONTENT_ITEM_TYPE;
            default:
                throw new IllegalArgumentException("Unknown URI " + uri);
        }
    }

    @Override
    public Bundle call(String method, String arg, Bundle extras) {
        throw new UnsupportedOperationException();
    }

    @Override
    public Cursor query(
            Uri uri,
            String[] projection,
            String selection,
            String[] selectionArgs,
            String sortOrder) {
        ensureInitialized();
        boolean needsToValidateSortOrder = !callerHasAccessAllEpgDataPermission();
        SqlParams params = createSqlParams(OP_QUERY, uri, selection, selectionArgs);

        SQLiteQueryBuilder queryBuilder = new SQLiteQueryBuilder();
        queryBuilder.setStrict(needsToValidateSortOrder);
        queryBuilder.setTables(params.getTables());
        String orderBy = null;
        Map<String, String> projectionMap;
        switch (params.getTables()) {
            case PROGRAMS_TABLE:
                projectionMap = sProgramProjectionMap;
                orderBy = DEFAULT_PROGRAMS_SORT_ORDER;
                break;
            case WATCHED_PROGRAMS_TABLE:
                projectionMap = sWatchedProgramProjectionMap;
                orderBy = DEFAULT_WATCHED_PROGRAMS_SORT_ORDER;
                break;
            case RECORDED_PROGRAMS_TABLE:
                projectionMap = sRecordedProgramProjectionMap;
                break;
            case PREVIEW_PROGRAMS_TABLE:
                projectionMap = sPreviewProgramProjectionMap;
                break;
            case WATCH_NEXT_PROGRAMS_TABLE:
                projectionMap = sWatchNextProgramProjectionMap;
                break;
            default:
                projectionMap = sChannelProjectionMap;
                break;
        }
        queryBuilder.setProjectionMap(createProjectionMapForQuery(projection, projectionMap));
        if (needsToValidateSortOrder) {
            validateSortOrder(sortOrder, projectionMap.keySet());
        }

        // Use the default sort order only if no sort order is specified.
        if (!TextUtils.isEmpty(sortOrder)) {
            orderBy = sortOrder;
        }

        // Get the database and run the query.
        SQLiteDatabase db = mOpenHelper.getReadableDatabase();
        Cursor c =
                queryBuilder.query(
                        db,
                        projection,
                        params.getSelection(),
                        params.getSelectionArgs(),
                        null,
                        null,
                        orderBy);

        // Tell the cursor what URI to watch, so it knows when its source data changes.
        c.setNotificationUri(getContext().getContentResolver(), uri);
        return c;
    }

    @Override
    public Uri insert(Uri uri, ContentValues values) {
        ensureInitialized();
        switch (sUriMatcher.match(uri)) {
            case MATCH_CHANNEL:
                // Preview channels are not necessarily associated with TV input service.
                // Therefore, we fill a fake ID to meet not null restriction for preview channels.
                if (values.get(Channels.COLUMN_INPUT_ID) == null
                        && TvContractCompat.PARAM_CHANNEL.equals(
                                values.get(Channels.COLUMN_TYPE))) {
                    values.put(Channels.COLUMN_INPUT_ID, EMPTY_STRING);
                }
                filterContentValues(values, sChannelProjectionMap);
                return insertChannel(uri, values);
            case MATCH_PROGRAM:
                filterContentValues(values, sProgramProjectionMap);
                return insertProgram(uri, values);
            case MATCH_WATCHED_PROGRAM:
                return insertWatchedProgram(uri, values);
            case MATCH_RECORDED_PROGRAM:
                filterContentValues(values, sRecordedProgramProjectionMap);
                return insertRecordedProgram(uri, values);
            case MATCH_PREVIEW_PROGRAM:
                filterContentValues(values, sPreviewProgramProjectionMap);
                return insertPreviewProgram(uri, values);
            case MATCH_WATCH_NEXT_PROGRAM:
                filterContentValues(values, sWatchNextProgramProjectionMap);
                return insertWatchNextProgram(uri, values);
            case MATCH_CHANNEL_ID:
            case MATCH_CHANNEL_ID_LOGO:
            case MATCH_PASSTHROUGH_ID:
            case MATCH_PROGRAM_ID:
            case MATCH_WATCHED_PROGRAM_ID:
            case MATCH_RECORDED_PROGRAM_ID:
            case MATCH_PREVIEW_PROGRAM_ID:
                throw new UnsupportedOperationException("Cannot insert into that URI: " + uri);
            default:
                throw new IllegalArgumentException("Unknown URI " + uri);
        }
    }

    private Uri insertChannel(Uri uri, ContentValues values) {
        if (TextUtils.equals(
                values.getAsString(Channels.COLUMN_TYPE), TvContractCompat.Channels.TYPE_PREVIEW)) {
            blockIllegalAccessFromBlockedPackage();
        }
        // Mark the owner package of this channel.
        values.put(Channels.COLUMN_PACKAGE_NAME, getCallingPackage_());
        blockIllegalAccessToChannelsSystemColumns(values);

        SQLiteDatabase db = mOpenHelper.getWritableDatabase();
        long rowId = db.insert(CHANNELS_TABLE, null, values);
        if (rowId > 0) {
            Uri channelUri = TvContractCompat.buildChannelUri(rowId);
            notifyChange(channelUri);
            return channelUri;
        }

        throw new SQLException("Failed to insert row into " + uri);
    }

    private Uri insertProgram(Uri uri, ContentValues values) {
        if (!callerHasAccessAllEpgDataPermission()
                || !values.containsKey(Programs.COLUMN_PACKAGE_NAME)) {
            // Mark the owner package of this program. System app with a proper permission may
            // change the owner of the program.
            values.put(Programs.COLUMN_PACKAGE_NAME, getCallingPackage_());
        }

        checkAndConvertGenre(values);
        checkAndConvertDeprecatedColumns(values);

        SQLiteDatabase db = mOpenHelper.getWritableDatabase();
        long rowId = db.insert(PROGRAMS_TABLE, null, values);
        if (rowId > 0) {
            Uri programUri = TvContractCompat.buildProgramUri(rowId);
            notifyChange(programUri);
            return programUri;
        }

        throw new SQLException("Failed to insert row into " + uri);
    }

    private Uri insertWatchedProgram(Uri uri, ContentValues values) {
        if (DEBUG) {
            Log.d(TAG, "insertWatchedProgram(uri=" + uri + ", values={" + values + "})");
        }
        Long watchStartTime = values.getAsLong(WatchedPrograms.COLUMN_WATCH_START_TIME_UTC_MILLIS);
        Long watchEndTime = values.getAsLong(WatchedPrograms.COLUMN_WATCH_END_TIME_UTC_MILLIS);
        // The system sends only two kinds of watch events:
        // 1. The user tunes to a new channel. (COLUMN_WATCH_START_TIME_UTC_MILLIS)
        // 2. The user stops watching. (COLUMN_WATCH_END_TIME_UTC_MILLIS)
        if (watchStartTime != null && watchEndTime == null) {
            SQLiteDatabase db = mOpenHelper.getWritableDatabase();
            long rowId = db.insert(WATCHED_PROGRAMS_TABLE, null, values);
            if (rowId > 0) {
                return ContentUris.withAppendedId(WatchedPrograms.CONTENT_URI, rowId);
            }
            Log.w(TAG, "Failed to insert row for " + values + ". Channel does not exist.");
            return null;
        } else if (watchStartTime == null && watchEndTime != null) {
            return null;
        }
        // All the other cases are invalid.
        throw new IllegalArgumentException(
                "Only one of COLUMN_WATCH_START_TIME_UTC_MILLIS and"
                        + " COLUMN_WATCH_END_TIME_UTC_MILLIS should be specified");
    }

    private Uri insertRecordedProgram(Uri uri, ContentValues values) {
        // Mark the owner package of this program.
        values.put(Programs.COLUMN_PACKAGE_NAME, getCallingPackage_());

        checkAndConvertGenre(values);

        SQLiteDatabase db = mOpenHelper.getWritableDatabase();
        long rowId = db.insert(RECORDED_PROGRAMS_TABLE, null, values);
        if (rowId > 0) {
            Uri recordedProgramUri = TvContractCompat.buildRecordedProgramUri(rowId);
            notifyChange(recordedProgramUri);
            return recordedProgramUri;
        }

        throw new SQLException("Failed to insert row into " + uri);
    }

    private Uri insertPreviewProgram(Uri uri, ContentValues values) {
        if (!values.containsKey(PreviewPrograms.COLUMN_TYPE)) {
            throw new IllegalArgumentException(
                    "Missing the required column: " + PreviewPrograms.COLUMN_TYPE);
        }
        blockIllegalAccessFromBlockedPackage();
        // Mark the owner package of this program.
        values.put(Programs.COLUMN_PACKAGE_NAME, getCallingPackage_());
        blockIllegalAccessToPreviewProgramsSystemColumns(values);

        SQLiteDatabase db = mOpenHelper.getWritableDatabase();
        long rowId = db.insert(PREVIEW_PROGRAMS_TABLE, null, values);
        if (rowId > 0) {
            Uri previewProgramUri = TvContractCompat.buildPreviewProgramUri(rowId);
            notifyChange(previewProgramUri);
            return previewProgramUri;
        }

        throw new SQLException("Failed to insert row into " + uri);
    }

    private Uri insertWatchNextProgram(Uri uri, ContentValues values) {
        if (!values.containsKey(WatchNextPrograms.COLUMN_TYPE)) {
            throw new IllegalArgumentException(
                    "Missing the required column: " + WatchNextPrograms.COLUMN_TYPE);
        }
        blockIllegalAccessFromBlockedPackage();
        if (!callerHasAccessAllEpgDataPermission()
                || !values.containsKey(Programs.COLUMN_PACKAGE_NAME)) {
            // Mark the owner package of this program. System app with a proper permission may
            // change the owner of the program.
            values.put(Programs.COLUMN_PACKAGE_NAME, getCallingPackage_());
        }
        blockIllegalAccessToPreviewProgramsSystemColumns(values);

        SQLiteDatabase db = mOpenHelper.getWritableDatabase();
        long rowId = db.insert(WATCH_NEXT_PROGRAMS_TABLE, null, values);
        if (rowId > 0) {
            Uri watchNextProgramUri = TvContractCompat.buildWatchNextProgramUri(rowId);
            notifyChange(watchNextProgramUri);
            return watchNextProgramUri;
        }

        throw new SQLException("Failed to insert row into " + uri);
    }

    @Override
    public int delete(Uri uri, String selection, String[] selectionArgs) {
        SqlParams params = createSqlParams(OP_DELETE, uri, selection, selectionArgs);
        SQLiteDatabase db = mOpenHelper.getWritableDatabase();
        int count;
        switch (sUriMatcher.match(uri)) {
            case MATCH_CHANNEL_ID_LOGO:
                ContentValues values = new ContentValues();
                values.putNull(CHANNELS_COLUMN_LOGO);
                count =
                        db.update(
                                params.getTables(),
                                values,
                                params.getSelection(),
                                params.getSelectionArgs());
                break;
            case MATCH_CHANNEL:
            case MATCH_PROGRAM:
            case MATCH_WATCHED_PROGRAM:
            case MATCH_RECORDED_PROGRAM:
            case MATCH_PREVIEW_PROGRAM:
            case MATCH_WATCH_NEXT_PROGRAM:
            case MATCH_CHANNEL_ID:
            case MATCH_PASSTHROUGH_ID:
            case MATCH_PROGRAM_ID:
            case MATCH_WATCHED_PROGRAM_ID:
            case MATCH_RECORDED_PROGRAM_ID:
            case MATCH_PREVIEW_PROGRAM_ID:
            case MATCH_WATCH_NEXT_PROGRAM_ID:
                count =
                        db.delete(
                                params.getTables(),
                                params.getSelection(),
                                params.getSelectionArgs());
                break;
            default:
                throw new IllegalArgumentException("Unknown URI " + uri);
        }
        if (count > 0) {
            notifyChange(uri);
        }
        return count;
    }

    @Override
    public int update(Uri uri, ContentValues values, String selection, String[] selectionArgs) {
        SqlParams params = createSqlParams(OP_UPDATE, uri, selection, selectionArgs);
        blockIllegalAccessToIdAndPackageName(uri, values);
        boolean containImmutableColumn = false;
        if (params.getTables().equals(CHANNELS_TABLE)) {
            filterContentValues(values, sChannelProjectionMap);
            containImmutableColumn = disallowModifyChannelType(values, params);
            if (containImmutableColumn && sUriMatcher.match(uri) != MATCH_CHANNEL_ID) {
                Log.i(TAG, "Updating failed. Attempt to change immutable column for channels.");
                return 0;
            }
            blockIllegalAccessToChannelsSystemColumns(values);
        } else if (params.getTables().equals(PROGRAMS_TABLE)) {
            filterContentValues(values, sProgramProjectionMap);
            checkAndConvertGenre(values);
            checkAndConvertDeprecatedColumns(values);
        } else if (params.getTables().equals(RECORDED_PROGRAMS_TABLE)) {
            filterContentValues(values, sRecordedProgramProjectionMap);
            checkAndConvertGenre(values);
        } else if (params.getTables().equals(PREVIEW_PROGRAMS_TABLE)) {
            filterContentValues(values, sPreviewProgramProjectionMap);
            containImmutableColumn = disallowModifyChannelId(values, params);
            if (containImmutableColumn && PreviewPrograms.CONTENT_URI.equals(uri)) {
                Log.i(
                        TAG,
                        "Updating failed. Attempt to change unmodifiable column for "
                                + "preview programs.");
                return 0;
            }
            blockIllegalAccessToPreviewProgramsSystemColumns(values);
        } else if (params.getTables().equals(WATCH_NEXT_PROGRAMS_TABLE)) {
            filterContentValues(values, sWatchNextProgramProjectionMap);
            blockIllegalAccessToPreviewProgramsSystemColumns(values);
        }
        if (values.size() == 0) {
            // All values may be filtered out, no need to update
            return 0;
        }
        SQLiteDatabase db = mOpenHelper.getWritableDatabase();
        int count =
                db.update(
                        params.getTables(),
                        values,
                        params.getSelection(),
                        params.getSelectionArgs());
        if (count > 0) {
            notifyChange(uri);
        } else if (containImmutableColumn) {
            Log.i(
                    TAG,
                    "Updating failed. The item may not exist or attempt to change "
                            + "immutable column.");
        }
        return count;
    }

    private synchronized void ensureInitialized() {
        if (!sInitialized) {
            // Database is not accessed before and the projection maps and the blocked package list
            // are not updated yet. Gets database here to make it initialized.
            mOpenHelper.getReadableDatabase();
        }
    }

    private static synchronized void initOnOpenIfNeeded(Context context, SQLiteDatabase db) {
        if (!sInitialized) {
            updateProjectionMap(db, CHANNELS_TABLE, sChannelProjectionMap);
            updateProjectionMap(db, PROGRAMS_TABLE, sProgramProjectionMap);
            updateProjectionMap(db, WATCHED_PROGRAMS_TABLE, sWatchedProgramProjectionMap);
            updateProjectionMap(db, RECORDED_PROGRAMS_TABLE, sRecordedProgramProjectionMap);
            updateProjectionMap(db, PREVIEW_PROGRAMS_TABLE, sPreviewProgramProjectionMap);
            updateProjectionMap(db, WATCH_NEXT_PROGRAMS_TABLE, sWatchNextProgramProjectionMap);
            sBlockedPackagesSharedPreference =
                    PreferenceManager.getDefaultSharedPreferences(context);
            sBlockedPackages = new ConcurrentHashMap<>();
            for (String packageName :
                    sBlockedPackagesSharedPreference.getStringSet(
                            SHARED_PREF_BLOCKED_PACKAGES_KEY, new HashSet<>())) {
                sBlockedPackages.put(packageName, true);
            }
            sInitialized = true;
        }
    }

    private static void updateProjectionMap(
            SQLiteDatabase db, String tableName, Map<String, String> projectionMap) {
        try (Cursor cursor = db.rawQuery("SELECT * FROM " + tableName + " LIMIT 0", null)) {
            for (String columnName : cursor.getColumnNames()) {
                if (!projectionMap.containsKey(columnName)) {
                    projectionMap.put(columnName, tableName + '.' + columnName);
                }
            }
        }
    }

    private Map<String, String> createProjectionMapForQuery(
            String[] projection, Map<String, String> projectionMap) {
        if (projection == null) {
            return projectionMap;
        }
        Map<String, String> columnProjectionMap = new HashMap<>();
        for (String columnName : projection) {
            // Value NULL will be provided if the requested column does not exist in the database.
            columnProjectionMap.put(
                    columnName, projectionMap.getOrDefault(columnName, "NULL as " + columnName));
        }
        return columnProjectionMap;
    }

    private void filterContentValues(ContentValues values, Map<String, String> projectionMap) {
        Iterator<String> iter = values.keySet().iterator();
        while (iter.hasNext()) {
            String columnName = iter.next();
            if (!projectionMap.containsKey(columnName)) {
                iter.remove();
            }
        }
    }

    private SqlParams createSqlParams(
            String operation, Uri uri, String selection, String[] selectionArgs) {
        int match = sUriMatcher.match(uri);
        SqlParams params = new SqlParams(null, selection, selectionArgs);

        // Control access to EPG data (excluding watched programs) when the caller doesn't have all
        // access.
        String prefix = match == MATCH_CHANNEL ? CHANNELS_TABLE + "." : "";
        if (!callerHasAccessAllEpgDataPermission()
                && match != MATCH_WATCHED_PROGRAM
                && match != MATCH_WATCHED_PROGRAM_ID) {
            if (!TextUtils.isEmpty(selection)) {
                throw new SecurityException("Selection not allowed for " + uri);
            }
            // Limit the operation only to the data that the calling package owns except for when
            // the caller tries to read TV listings and has the appropriate permission.
            if (operation.equals(OP_QUERY) && callerHasReadTvListingsPermission()) {
                params.setWhere(
                        prefix
                                + BaseTvColumns.COLUMN_PACKAGE_NAME
                                + "=? OR "
                                + Channels.COLUMN_SEARCHABLE
                                + "=?",
                        getCallingPackage_(),
                        "1");
            } else {
                params.setWhere(
                        prefix + BaseTvColumns.COLUMN_PACKAGE_NAME + "=?", getCallingPackage_());
            }
        }
        String packageName = uri.getQueryParameter(PARAM_PACKAGE);
        if (packageName != null) {
            params.appendWhere(prefix + BaseTvColumns.COLUMN_PACKAGE_NAME + "=?", packageName);
        }

        switch (match) {
            case MATCH_CHANNEL:
                String genre = uri.getQueryParameter(TvContractCompat.PARAM_CANONICAL_GENRE);
                if (genre == null) {
                    params.setTables(CHANNELS_TABLE);
                } else {
                    if (!operation.equals(OP_QUERY)) {
                        throw new SecurityException(
                                capitalize(operation) + " not allowed for " + uri);
                    }
                    if (!Genres.isCanonical(genre)) {
                        throw new IllegalArgumentException("Not a canonical genre : " + genre);
                    }
                    params.setTables(CHANNELS_TABLE_INNER_JOIN_PROGRAMS_TABLE);
                    String curTime = String.valueOf(System.currentTimeMillis());
                    params.appendWhere(
                            "LIKE(?, "
                                    + Programs.COLUMN_CANONICAL_GENRE
                                    + ") AND "
                                    + Programs.COLUMN_START_TIME_UTC_MILLIS
                                    + "<=? AND "
                                    + Programs.COLUMN_END_TIME_UTC_MILLIS
                                    + ">=?",
                            "%" + genre + "%",
                            curTime,
                            curTime);
                }
                String inputId = uri.getQueryParameter(TvContractCompat.PARAM_INPUT);
                if (inputId != null) {
                    params.appendWhere(Channels.COLUMN_INPUT_ID + "=?", inputId);
                }
                boolean browsableOnly =
                        uri.getBooleanQueryParameter(TvContractCompat.PARAM_BROWSABLE_ONLY, false);
                if (browsableOnly) {
                    params.appendWhere(Channels.COLUMN_BROWSABLE + "=1");
                }
                String preview = uri.getQueryParameter(PARAM_PREVIEW);
                if (preview != null) {
                    String previewSelection =
                            Channels.COLUMN_TYPE
                                    + (preview.equals(String.valueOf(true)) ? "=?" : "!=?");
                    params.appendWhere(previewSelection, Channels.TYPE_PREVIEW);
                }
                break;
            case MATCH_CHANNEL_ID:
                params.setTables(CHANNELS_TABLE);
                params.appendWhere(Channels._ID + "=?", uri.getLastPathSegment());
                break;
            case MATCH_PROGRAM:
                params.setTables(PROGRAMS_TABLE);
                String paramChannelId = uri.getQueryParameter(TvContractCompat.PARAM_CHANNEL);
                if (paramChannelId != null) {
                    String channelId = String.valueOf(Long.parseLong(paramChannelId));
                    params.appendWhere(Programs.COLUMN_CHANNEL_ID + "=?", channelId);
                }
                String paramStartTime = uri.getQueryParameter(TvContractCompat.PARAM_START_TIME);
                String paramEndTime = uri.getQueryParameter(TvContractCompat.PARAM_END_TIME);
                if (paramStartTime != null && paramEndTime != null) {
                    String startTime = String.valueOf(Long.parseLong(paramStartTime));
                    String endTime = String.valueOf(Long.parseLong(paramEndTime));
                    params.appendWhere(
                            Programs.COLUMN_START_TIME_UTC_MILLIS
                                    + "<=? AND "
                                    + Programs.COLUMN_END_TIME_UTC_MILLIS
                                    + ">=? AND ?<=?",
                            endTime,
                            startTime,
                            startTime,
                            endTime);
                }
                break;
            case MATCH_PROGRAM_ID:
                params.setTables(PROGRAMS_TABLE);
                params.appendWhere(Programs._ID + "=?", uri.getLastPathSegment());
                break;
            case MATCH_WATCHED_PROGRAM:
                if (!callerHasAccessWatchedProgramsPermission()) {
                    throw new SecurityException("Access not allowed for " + uri);
                }
                params.setTables(WATCHED_PROGRAMS_TABLE);
                params.appendWhere(WATCHED_PROGRAMS_COLUMN_CONSOLIDATED + "=?", "1");
                break;
            case MATCH_WATCHED_PROGRAM_ID:
                if (!callerHasAccessWatchedProgramsPermission()) {
                    throw new SecurityException("Access not allowed for " + uri);
                }
                params.setTables(WATCHED_PROGRAMS_TABLE);
                params.appendWhere(WatchedPrograms._ID + "=?", uri.getLastPathSegment());
                params.appendWhere(WATCHED_PROGRAMS_COLUMN_CONSOLIDATED + "=?", "1");
                break;
            case MATCH_RECORDED_PROGRAM_ID:
                params.appendWhere(RecordedPrograms._ID + "=?", uri.getLastPathSegment());
                // fall-through
            case MATCH_RECORDED_PROGRAM:
                params.setTables(RECORDED_PROGRAMS_TABLE);
                paramChannelId = uri.getQueryParameter(TvContractCompat.PARAM_CHANNEL);
                if (paramChannelId != null) {
                    String channelId = String.valueOf(Long.parseLong(paramChannelId));
                    params.appendWhere(Programs.COLUMN_CHANNEL_ID + "=?", channelId);
                }
                break;
            case MATCH_PREVIEW_PROGRAM_ID:
                params.appendWhere(PreviewPrograms._ID + "=?", uri.getLastPathSegment());
                // fall-through
            case MATCH_PREVIEW_PROGRAM:
                params.setTables(PREVIEW_PROGRAMS_TABLE);
                paramChannelId = uri.getQueryParameter(TvContractCompat.PARAM_CHANNEL);
                if (paramChannelId != null) {
                    String channelId = String.valueOf(Long.parseLong(paramChannelId));
                    params.appendWhere(PreviewPrograms.COLUMN_CHANNEL_ID + "=?", channelId);
                }
                break;
            case MATCH_WATCH_NEXT_PROGRAM_ID:
                params.appendWhere(WatchNextPrograms._ID + "=?", uri.getLastPathSegment());
                // fall-through
            case MATCH_WATCH_NEXT_PROGRAM:
                params.setTables(WATCH_NEXT_PROGRAMS_TABLE);
                break;
            case MATCH_CHANNEL_ID_LOGO:
                if (operation.equals(OP_DELETE)) {
                    params.setTables(CHANNELS_TABLE);
                    params.appendWhere(Channels._ID + "=?", uri.getPathSegments().get(1));
                    break;
                }
                // fall-through
            case MATCH_PASSTHROUGH_ID:
                throw new UnsupportedOperationException(operation + " not permmitted on " + uri);
            default:
                throw new IllegalArgumentException("Unknown URI " + uri);
        }
        return params;
    }

    private static String capitalize(String str) {
        return Character.toUpperCase(str.charAt(0)) + str.substring(1);
    }

    @SuppressLint("DefaultLocale")
    private void checkAndConvertGenre(ContentValues values) {
        String canonicalGenres = values.getAsString(Programs.COLUMN_CANONICAL_GENRE);

        if (!TextUtils.isEmpty(canonicalGenres)) {
            // Check if the canonical genres are valid. If not, clear them.
            String[] genres = Genres.decode(canonicalGenres);
            for (String genre : genres) {
                if (!Genres.isCanonical(genre)) {
                    values.putNull(Programs.COLUMN_CANONICAL_GENRE);
                    canonicalGenres = null;
                    break;
                }
            }
        }

        if (TextUtils.isEmpty(canonicalGenres)) {
            // If the canonical genre is not set, try to map the broadcast genre to the canonical
            // genre.
            String broadcastGenres = values.getAsString(Programs.COLUMN_BROADCAST_GENRE);
            if (!TextUtils.isEmpty(broadcastGenres)) {
                Set<String> genreSet = new HashSet<>();
                String[] genres = Genres.decode(broadcastGenres);
                for (String genre : genres) {
                    String canonicalGenre = sGenreMap.get(genre.toUpperCase());
                    if (Genres.isCanonical(canonicalGenre)) {
                        genreSet.add(canonicalGenre);
                    }
                }
                if (genreSet.size() > 0) {
                    values.put(
                            Programs.COLUMN_CANONICAL_GENRE,
                            Genres.encode(genreSet.toArray(new String[genreSet.size()])));
                }
            }
        }
    }

    private void checkAndConvertDeprecatedColumns(ContentValues values) {
        if (values.containsKey(Programs.COLUMN_SEASON_NUMBER)) {
            if (!values.containsKey(Programs.COLUMN_SEASON_DISPLAY_NUMBER)) {
                values.put(
                        Programs.COLUMN_SEASON_DISPLAY_NUMBER,
                        values.getAsInteger(Programs.COLUMN_SEASON_NUMBER));
            }
            values.remove(Programs.COLUMN_SEASON_NUMBER);
        }
        if (values.containsKey(Programs.COLUMN_EPISODE_NUMBER)) {
            if (!values.containsKey(Programs.COLUMN_EPISODE_DISPLAY_NUMBER)) {
                values.put(
                        Programs.COLUMN_EPISODE_DISPLAY_NUMBER,
                        values.getAsInteger(Programs.COLUMN_EPISODE_NUMBER));
            }
            values.remove(Programs.COLUMN_EPISODE_NUMBER);
        }
    }

    // We might have more than one thread trying to make its way through applyBatch() so the
    // notification coalescing needs to be thread-local to work correctly.
    private final ThreadLocal<Set<Uri>> mTLBatchNotifications = new ThreadLocal<>();

    private Set<Uri> getBatchNotificationsSet() {
        return mTLBatchNotifications.get();
    }

    private void setBatchNotificationsSet(Set<Uri> batchNotifications) {
        mTLBatchNotifications.set(batchNotifications);
    }

    @Override
    public ContentProviderResult[] applyBatch(ArrayList<ContentProviderOperation> operations)
            throws OperationApplicationException {
        setBatchNotificationsSet(new HashSet<Uri>());
        Context context = getContext();
        SQLiteDatabase db = mOpenHelper.getWritableDatabase();
        db.beginTransaction();
        try {
            ContentProviderResult[] results = super.applyBatch(operations);
            db.setTransactionSuccessful();
            return results;
        } finally {
            db.endTransaction();
            final Set<Uri> notifications = getBatchNotificationsSet();
            setBatchNotificationsSet(null);
            for (final Uri uri : notifications) {
                context.getContentResolver().notifyChange(uri, null);
            }
        }
    }

    @Override
    public int bulkInsert(Uri uri, ContentValues[] values) {
        setBatchNotificationsSet(new HashSet<Uri>());
        Context context = getContext();
        SQLiteDatabase db = mOpenHelper.getWritableDatabase();
        db.beginTransaction();
        try {
            int result = super.bulkInsert(uri, values);
            db.setTransactionSuccessful();
            return result;
        } finally {
            db.endTransaction();
            final Set<Uri> notifications = getBatchNotificationsSet();
            setBatchNotificationsSet(null);
            for (final Uri notificationUri : notifications) {
                context.getContentResolver().notifyChange(notificationUri, null);
            }
        }
    }

    private void notifyChange(Uri uri) {
        final Set<Uri> batchNotifications = getBatchNotificationsSet();
        if (batchNotifications != null) {
            batchNotifications.add(uri);
        } else {
            getContext().getContentResolver().notifyChange(uri, null);
        }
    }

    private boolean callerHasReadTvListingsPermission() {
        return getContext().checkCallingOrSelfPermission(PERMISSION_READ_TV_LISTINGS)
                == PackageManager.PERMISSION_GRANTED;
    }

    private boolean callerHasAccessAllEpgDataPermission() {
        return getContext().checkCallingOrSelfPermission(PERMISSION_ACCESS_ALL_EPG_DATA)
                == PackageManager.PERMISSION_GRANTED;
    }

    private boolean callerHasAccessWatchedProgramsPermission() {
        return getContext().checkCallingOrSelfPermission(PERMISSION_ACCESS_WATCHED_PROGRAMS)
                == PackageManager.PERMISSION_GRANTED;
    }

    private boolean callerHasModifyParentalControlsPermission() {
        return getContext()
                        .checkCallingOrSelfPermission(
                                android.Manifest.permission.MODIFY_PARENTAL_CONTROLS)
                == PackageManager.PERMISSION_GRANTED;
    }

    private void blockIllegalAccessToIdAndPackageName(Uri uri, ContentValues values) {
        if (values.containsKey(BaseColumns._ID)) {
            int match = sUriMatcher.match(uri);
            switch (match) {
                case MATCH_CHANNEL_ID:
                case MATCH_PROGRAM_ID:
                case MATCH_PREVIEW_PROGRAM_ID:
                case MATCH_RECORDED_PROGRAM_ID:
                case MATCH_WATCH_NEXT_PROGRAM_ID:
                case MATCH_WATCHED_PROGRAM_ID:
                    if (TextUtils.equals(
                            values.getAsString(BaseColumns._ID), uri.getLastPathSegment())) {
                        break;
                    }
                    // fall through
                default:
                    throw new IllegalArgumentException("Not allowed to change ID.");
            }
        }
        if (values.containsKey(BaseTvColumns.COLUMN_PACKAGE_NAME)
                && !callerHasAccessAllEpgDataPermission()
                && !TextUtils.equals(
                        values.getAsString(BaseTvColumns.COLUMN_PACKAGE_NAME),
                        getCallingPackage_())) {
            throw new SecurityException("Not allowed to change package name.");
        }
    }

    private void blockIllegalAccessToChannelsSystemColumns(ContentValues values) {
        if (values.containsKey(Channels.COLUMN_LOCKED)
                && !callerHasModifyParentalControlsPermission()) {
            throw new SecurityException("Not allowed to access Channels.COLUMN_LOCKED");
        }
        Boolean hasAccessAllEpgDataPermission = null;
        if (values.containsKey(Channels.COLUMN_BROWSABLE)) {
            hasAccessAllEpgDataPermission = callerHasAccessAllEpgDataPermission();
            if (!hasAccessAllEpgDataPermission) {
                throw new SecurityException("Not allowed to access Channels.COLUMN_BROWSABLE");
            }
        }
    }

    private void blockIllegalAccessToPreviewProgramsSystemColumns(ContentValues values) {
        if (values.containsKey(PreviewPrograms.COLUMN_BROWSABLE)
                && !callerHasAccessAllEpgDataPermission()) {
            throw new SecurityException("Not allowed to access Programs.COLUMN_BROWSABLE");
        }
    }

    private void blockIllegalAccessFromBlockedPackage() {
        String callingPackageName = getCallingPackage_();
        if (sBlockedPackages.containsKey(callingPackageName)) {
            throw new SecurityException(
                    "Not allowed to access "
                            + TvContractCompat.AUTHORITY
                            + ", "
                            + callingPackageName
                            + " is blocked");
        }
    }

    private boolean disallowModifyChannelType(ContentValues values, SqlParams params) {
        if (values.containsKey(Channels.COLUMN_TYPE)) {
            params.appendWhere(
                    Channels.COLUMN_TYPE + "=?", values.getAsString(Channels.COLUMN_TYPE));
            return true;
        }
        return false;
    }

    private boolean disallowModifyChannelId(ContentValues values, SqlParams params) {
        if (values.containsKey(PreviewPrograms.COLUMN_CHANNEL_ID)) {
            params.appendWhere(
                    PreviewPrograms.COLUMN_CHANNEL_ID + "=?",
                    values.getAsString(PreviewPrograms.COLUMN_CHANNEL_ID));
            return true;
        }
        return false;
    }

    @Override
    public ParcelFileDescriptor openFile(Uri uri, String mode) throws FileNotFoundException {
        switch (sUriMatcher.match(uri)) {
            case MATCH_CHANNEL_ID_LOGO:
                return openLogoFile(uri, mode);
            default:
                throw new FileNotFoundException(uri.toString());
        }
    }

    private ParcelFileDescriptor openLogoFile(Uri uri, String mode) throws FileNotFoundException {
        long channelId = Long.parseLong(uri.getPathSegments().get(1));

        SqlParams params =
                new SqlParams(CHANNELS_TABLE, Channels._ID + "=?", String.valueOf(channelId));
        if (!callerHasAccessAllEpgDataPermission()) {
            if (callerHasReadTvListingsPermission()) {
                params.appendWhere(
                        Channels.COLUMN_PACKAGE_NAME + "=? OR " + Channels.COLUMN_SEARCHABLE + "=?",
                        getCallingPackage_(),
                        "1");
            } else {
                params.appendWhere(Channels.COLUMN_PACKAGE_NAME + "=?", getCallingPackage_());
            }
        }

        SQLiteQueryBuilder queryBuilder = new SQLiteQueryBuilder();
        queryBuilder.setTables(params.getTables());

        // We don't write the database here.
        SQLiteDatabase db = mOpenHelper.getReadableDatabase();
        if (mode.equals("r")) {
            String sql =
                    queryBuilder.buildQuery(
                            new String[] {CHANNELS_COLUMN_LOGO},
                            params.getSelection(),
                            null,
                            null,
                            null,
                            null);
            ParcelFileDescriptor fd =
                    DatabaseUtils.blobFileDescriptorForQuery(db, sql, params.getSelectionArgs());
            if (fd == null) {
                throw new FileNotFoundException(uri.toString());
            }
            return fd;
        } else {
            try (Cursor cursor =
                    queryBuilder.query(
                            db,
                            new String[] {Channels._ID},
                            params.getSelection(),
                            params.getSelectionArgs(),
                            null,
                            null,
                            null)) {
                if (cursor.getCount() < 1) {
                    // Fails early if corresponding channel does not exist.
                    // PipeMonitor may still fail to update DB later.
                    throw new FileNotFoundException(uri.toString());
                }
            }

            try {
                ParcelFileDescriptor[] pipeFds = ParcelFileDescriptor.createPipe();
                PipeMonitor pipeMonitor = new PipeMonitor(pipeFds[0], channelId, params);
                pipeMonitor.executeOnExecutor(AsyncTask.THREAD_POOL_EXECUTOR);
                return pipeFds[1];
            } catch (IOException ioe) {
                FileNotFoundException fne = new FileNotFoundException(uri.toString());
                fne.initCause(ioe);
                throw fne;
            }
        }
    }

    /**
     * Validates the sort order based on the given field set.
     *
     * @throws IllegalArgumentException if there is any unknown field.
     */
    @SuppressLint("DefaultLocale")
    private static void validateSortOrder(String sortOrder, Set<String> possibleFields) {
        if (TextUtils.isEmpty(sortOrder) || possibleFields.isEmpty()) {
            return;
        }
        String[] orders = sortOrder.split(",");
        for (String order : orders) {
            String field =
                    order.replaceAll("\\s+", " ")
                            .trim()
                            .toLowerCase()
                            .replace(" asc", "")
                            .replace(" desc", "");
            if (!possibleFields.contains(field)) {
                throw new IllegalArgumentException("Illegal field in sort order " + order);
            }
        }
    }

    private class PipeMonitor extends AsyncTask<Void, Void, Void> {
        private final ParcelFileDescriptor mPfd;
        private final long mChannelId;
        private final SqlParams mParams;

        private PipeMonitor(ParcelFileDescriptor pfd, long channelId, SqlParams params) {
            mPfd = pfd;
            mChannelId = channelId;
            mParams = params;
        }

        @Override
        protected Void doInBackground(Void... params) {
            int count = 0;
            try (AutoCloseInputStream is = new AutoCloseInputStream(mPfd);
                    ByteArrayOutputStream baos = new ByteArrayOutputStream()) {
                Bitmap bitmap = BitmapFactory.decodeStream(is);
                if (bitmap == null) {
                    Log.e(TAG, "Failed to decode logo image for channel ID " + mChannelId);
                    return null;
                }

                float scaleFactor =
                        Math.min(
                                1f,
                                ((float) MAX_LOGO_IMAGE_SIZE)
                                        / Math.max(bitmap.getWidth(), bitmap.getHeight()));
                if (scaleFactor < 1f) {
                    bitmap =
                            Bitmap.createScaledBitmap(
                                    bitmap,
                                    (int) (bitmap.getWidth() * scaleFactor),
                                    (int) (bitmap.getHeight() * scaleFactor),
                                    false);
                }
                bitmap.compress(Bitmap.CompressFormat.PNG, 100, baos);
                byte[] bytes = baos.toByteArray();

                ContentValues values = new ContentValues();
                values.put(CHANNELS_COLUMN_LOGO, bytes);
                SQLiteDatabase db = mOpenHelper.getWritableDatabase();
                count =
                        db.update(
                                mParams.getTables(),
                                values,
                                mParams.getSelection(),
                                mParams.getSelectionArgs());
                if (count > 0) {
                    Uri uri = TvContractCompat.buildChannelLogoUri(mChannelId);
                    notifyChange(uri);
                }
            } catch (IOException e) {
                Log.e(TAG, "Failed to write logo for channel ID " + mChannelId, e);

            } finally {
                if (count == 0) {
                    try {
                        mPfd.closeWithError("Failed to write logo for channel ID " + mChannelId);
                    } catch (IOException ioe) {
                        Log.e(TAG, "Failed to close pipe", ioe);
                    }
                }
            }
            return null;
        }
    }

    /**
     * Column definitions for the TV programs that the user watched. Applications do not have access
     * to this table.
     *
     * <p>
     *
     * <p>By default, the query results will be sorted by {@link
     * WatchedPrograms#COLUMN_WATCH_START_TIME_UTC_MILLIS} in descending order.
     *
     * @hide
     */
    public static final class WatchedPrograms implements BaseTvColumns {

        /** The content:// style URI for this table. */
        public static final Uri CONTENT_URI =
                Uri.parse("content://" + TvContract.AUTHORITY + "/watched_program");

        /** The MIME type of a directory of watched programs. */
        public static final String CONTENT_TYPE = "vnd.android.cursor.dir/watched_program";

        /** The MIME type of a single item in this table. */
        public static final String CONTENT_ITEM_TYPE = "vnd.android.cursor.item/watched_program";

        /**
         * The UTC time that the user started watching this TV program, in milliseconds since the
         * epoch.
         *
         * <p>
         *
         * <p>Type: INTEGER (long)
         */
        public static final String COLUMN_WATCH_START_TIME_UTC_MILLIS =
                "watch_start_time_utc_millis";

        /**
         * The UTC time that the user stopped watching this TV program, in milliseconds since the
         * epoch.
         *
         * <p>
         *
         * <p>Type: INTEGER (long)
         */
        public static final String COLUMN_WATCH_END_TIME_UTC_MILLIS = "watch_end_time_utc_millis";

        /**
         * The ID of the TV channel that provides this TV program.
         *
         * <p>
         *
         * <p>This is a required field.
         *
         * <p>
         *
         * <p>Type: INTEGER (long)
         */
        public static final String COLUMN_CHANNEL_ID = "channel_id";

        /**
         * The title of this TV program.
         *
         * <p>
         *
         * <p>Type: TEXT
         */
        public static final String COLUMN_TITLE = "title";

        /**
         * The start time of this TV program, in milliseconds since the epoch.
         *
         * <p>
         *
         * <p>Type: INTEGER (long)
         */
        public static final String COLUMN_START_TIME_UTC_MILLIS = "start_time_utc_millis";

        /**
         * The end time of this TV program, in milliseconds since the epoch.
         *
         * <p>
         *
         * <p>Type: INTEGER (long)
         */
        public static final String COLUMN_END_TIME_UTC_MILLIS = "end_time_utc_millis";

        /**
         * The description of this TV program.
         *
         * <p>
         *
         * <p>Type: TEXT
         */
        public static final String COLUMN_DESCRIPTION = "description";

        /**
         * Extra parameters given to {@link TvInputService.Session#tune(Uri, android.os.Bundle)
         * TvInputService.Session.tune(Uri, android.os.Bundle)} when tuning to the channel that
         * provides this TV program. (Used internally.)
         *
         * <p>
         *
         * <p>This column contains an encoded string that represents comma-separated key-value pairs
         * of the tune parameters. (Ex. "[key1]=[value1], [key2]=[value2]"). '%' is used as an
         * escape character for '%', '=', and ','.
         *
         * <p>
         *
         * <p>Type: TEXT
         */
        public static final String COLUMN_INTERNAL_TUNE_PARAMS = "tune_params";

        /**
         * The session token of this TV program. (Used internally.)
         *
         * <p>
         *
         * <p>This contains a String representation of {@link IBinder} for {@link
         * TvInputService.Session} that provides the current TV program. It is used internally to
         * distinguish watched programs entries from different TV input sessions.
         *
         * <p>
         *
         * <p>Type: TEXT
         */
        public static final String COLUMN_INTERNAL_SESSION_TOKEN = "session_token";

        private WatchedPrograms() {}
    }
}
