/*
 * Copyright (C) 2013 The Android Open Source Project
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

package com.android.providers.media;

import android.annotation.Nullable;
import android.content.ContentResolver;
import android.content.ContentUris;
import android.content.Context;
import android.content.res.AssetFileDescriptor;
import android.database.Cursor;
import android.database.MatrixCursor;
import android.database.MatrixCursor.RowBuilder;
import android.graphics.BitmapFactory;
import android.graphics.Point;
import android.media.ExifInterface;
import android.media.MediaMetadata;
import android.net.Uri;
import android.os.Binder;
import android.os.Bundle;
import android.os.CancellationSignal;
import android.os.IBinder;
import android.os.ParcelFileDescriptor;
import android.os.UserHandle;
import android.os.UserManager;
import android.provider.BaseColumns;
import android.provider.DocumentsContract;
import android.provider.DocumentsContract.Document;
import android.provider.DocumentsContract.Root;
import android.provider.DocumentsProvider;
import android.provider.MediaStore;
import android.provider.MediaStore.Audio;
import android.provider.MediaStore.Audio.AlbumColumns;
import android.provider.MediaStore.Audio.Albums;
import android.provider.MediaStore.Audio.ArtistColumns;
import android.provider.MediaStore.Audio.Artists;
import android.provider.MediaStore.Audio.AudioColumns;
import android.provider.MediaStore.Files.FileColumns;
import android.provider.MediaStore.Images;
import android.provider.MediaStore.Images.ImageColumns;
import android.provider.MediaStore.Video;
import android.provider.MediaStore.Video.VideoColumns;
import android.provider.MetadataReader;
import android.text.TextUtils;
import android.text.format.DateFormat;
import android.text.format.DateUtils;
import android.util.Log;

import libcore.io.IoUtils;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.io.InputStream;
import java.util.Collection;
import java.util.HashMap;
import java.util.Locale;
import java.util.Map;

/**
 * Presents a {@link DocumentsContract} view of {@link MediaProvider} external
 * contents.
 */
public class MediaDocumentsProvider extends DocumentsProvider {
    private static final String TAG = "MediaDocumentsProvider";

    private static final String AUTHORITY = "com.android.providers.media.documents";

    private static final String[] DEFAULT_ROOT_PROJECTION = new String[] {
            Root.COLUMN_ROOT_ID, Root.COLUMN_FLAGS, Root.COLUMN_ICON,
            Root.COLUMN_TITLE, Root.COLUMN_DOCUMENT_ID, Root.COLUMN_MIME_TYPES
    };

    private static final String[] DEFAULT_DOCUMENT_PROJECTION = new String[] {
            Document.COLUMN_DOCUMENT_ID, Document.COLUMN_MIME_TYPE, Document.COLUMN_DISPLAY_NAME,
            Document.COLUMN_LAST_MODIFIED, Document.COLUMN_FLAGS, Document.COLUMN_SIZE,
    };

    private static final String IMAGE_MIME_TYPES = joinNewline("image/*");

    private static final String VIDEO_MIME_TYPES = joinNewline("video/*");

    private static final String AUDIO_MIME_TYPES = joinNewline(
            "audio/*", "application/ogg", "application/x-flac");

    private static final String TYPE_IMAGES_ROOT = "images_root";
    private static final String TYPE_IMAGES_BUCKET = "images_bucket";
    private static final String TYPE_IMAGE = "image";

    private static final String TYPE_VIDEOS_ROOT = "videos_root";
    private static final String TYPE_VIDEOS_BUCKET = "videos_bucket";
    private static final String TYPE_VIDEO = "video";

    private static final String TYPE_AUDIO_ROOT = "audio_root";
    private static final String TYPE_AUDIO = "audio";
    private static final String TYPE_ARTIST = "artist";
    private static final String TYPE_ALBUM = "album";

    private static boolean sReturnedImagesEmpty = false;
    private static boolean sReturnedVideosEmpty = false;
    private static boolean sReturnedAudioEmpty = false;

    private static String joinNewline(String... args) {
        return TextUtils.join("\n", args);
    }

    public static final String METADATA_KEY_AUDIO = "android.media.metadata.audio";
    public static final String METADATA_KEY_VIDEO = "android.media.metadata.video";
    // Video lat/long are just that. Lat/long. Unlike EXIF where the values are
    // in fact some funky string encoding. So we add our own contstant to convey coords.
    public static final String METADATA_VIDEO_LATITUDE = "android.media.metadata.video:latitude";
    public static final String METADATA_VIDEO_LONGITUTE = "android.media.metadata.video:longitude";

    /*
     * A mapping between media colums and metadata tag names. These keys of the
     * map form the projection for queries against the media store database.
     */
    private static final Map<String, String> IMAGE_COLUMN_MAP = new HashMap<>();
    private static final Map<String, String> VIDEO_COLUMN_MAP = new HashMap<>();
    private static final Map<String, String> AUDIO_COLUMN_MAP = new HashMap<>();

    static {
        /**
         * Note that for images (jpegs at least) we'll first try an alternate
         * means of extracting metadata, one that provides more data. But if
         * that fails, or if the image type is not JPEG, we fall back to these columns.
         */
        IMAGE_COLUMN_MAP.put(ImageColumns.WIDTH, ExifInterface.TAG_IMAGE_WIDTH);
        IMAGE_COLUMN_MAP.put(ImageColumns.HEIGHT, ExifInterface.TAG_IMAGE_LENGTH);
        IMAGE_COLUMN_MAP.put(ImageColumns.DATE_TAKEN, ExifInterface.TAG_DATETIME);
        IMAGE_COLUMN_MAP.put(ImageColumns.LATITUDE, ExifInterface.TAG_GPS_LATITUDE);
        IMAGE_COLUMN_MAP.put(ImageColumns.LONGITUDE, ExifInterface.TAG_GPS_LONGITUDE);

        VIDEO_COLUMN_MAP.put(VideoColumns.DURATION, MediaMetadata.METADATA_KEY_DURATION);
        VIDEO_COLUMN_MAP.put(VideoColumns.HEIGHT, ExifInterface.TAG_IMAGE_LENGTH);
        VIDEO_COLUMN_MAP.put(VideoColumns.WIDTH, ExifInterface.TAG_IMAGE_WIDTH);
        VIDEO_COLUMN_MAP.put(VideoColumns.LATITUDE, METADATA_VIDEO_LATITUDE);
        VIDEO_COLUMN_MAP.put(VideoColumns.LONGITUDE, METADATA_VIDEO_LONGITUTE);
        VIDEO_COLUMN_MAP.put(VideoColumns.DATE_TAKEN, MediaMetadata.METADATA_KEY_DATE);

        AUDIO_COLUMN_MAP.put(AudioColumns.ARTIST, MediaMetadata.METADATA_KEY_ARTIST);
        AUDIO_COLUMN_MAP.put(AudioColumns.COMPOSER, MediaMetadata.METADATA_KEY_COMPOSER);
        AUDIO_COLUMN_MAP.put(AudioColumns.ALBUM, MediaMetadata.METADATA_KEY_ALBUM);
        AUDIO_COLUMN_MAP.put(AudioColumns.YEAR, MediaMetadata.METADATA_KEY_YEAR);
        AUDIO_COLUMN_MAP.put(AudioColumns.DURATION, MediaMetadata.METADATA_KEY_DURATION);
    }

    private void copyNotificationUri(MatrixCursor result, Cursor cursor) {
        result.setNotificationUri(getContext().getContentResolver(), cursor.getNotificationUri());
    }

    @Override
    public boolean onCreate() {
        return true;
    }

    private void enforceShellRestrictions() {
        if (UserHandle.getCallingAppId() == android.os.Process.SHELL_UID
                && getContext().getSystemService(UserManager.class)
                        .hasUserRestriction(UserManager.DISALLOW_USB_FILE_TRANSFER)) {
            throw new SecurityException(
                    "Shell user cannot access files for user " + UserHandle.myUserId());
        }
    }

    @Override
    protected int enforceReadPermissionInner(Uri uri, String callingPkg, IBinder callerToken)
            throws SecurityException {
        enforceShellRestrictions();
        return super.enforceReadPermissionInner(uri, callingPkg, callerToken);
    }

    @Override
    protected int enforceWritePermissionInner(Uri uri, String callingPkg, IBinder callerToken)
            throws SecurityException {
        enforceShellRestrictions();
        return super.enforceWritePermissionInner(uri, callingPkg, callerToken);
    }

    private static void notifyRootsChanged(Context context) {
        context.getContentResolver()
                .notifyChange(DocumentsContract.buildRootsUri(AUTHORITY), null, false);
    }

    /**
     * When inserting the first item of each type, we need to trigger a roots
     * refresh to clear a previously reported {@link Root#FLAG_EMPTY}.
     */
    static void onMediaStoreInsert(Context context, String volumeName, int type, long id) {
        if (!"external".equals(volumeName)) return;

        if (type == FileColumns.MEDIA_TYPE_IMAGE && sReturnedImagesEmpty) {
            sReturnedImagesEmpty = false;
            notifyRootsChanged(context);
        } else if (type == FileColumns.MEDIA_TYPE_VIDEO && sReturnedVideosEmpty) {
            sReturnedVideosEmpty = false;
            notifyRootsChanged(context);
        } else if (type == FileColumns.MEDIA_TYPE_AUDIO && sReturnedAudioEmpty) {
            sReturnedAudioEmpty = false;
            notifyRootsChanged(context);
        }
    }

    /**
     * When deleting an item, we need to revoke any outstanding Uri grants.
     */
    static void onMediaStoreDelete(Context context, String volumeName, int type, long id) {
        if (!"external".equals(volumeName)) return;

        if (type == FileColumns.MEDIA_TYPE_IMAGE) {
            final Uri uri = DocumentsContract.buildDocumentUri(
                    AUTHORITY, getDocIdForIdent(TYPE_IMAGE, id));
            context.revokeUriPermission(uri, ~0);
        } else if (type == FileColumns.MEDIA_TYPE_VIDEO) {
            final Uri uri = DocumentsContract.buildDocumentUri(
                    AUTHORITY, getDocIdForIdent(TYPE_VIDEO, id));
            context.revokeUriPermission(uri, ~0);
        } else if (type == FileColumns.MEDIA_TYPE_AUDIO) {
            final Uri uri = DocumentsContract.buildDocumentUri(
                    AUTHORITY, getDocIdForIdent(TYPE_AUDIO, id));
            context.revokeUriPermission(uri, ~0);
        }
    }

    private static class Ident {
        public String type;
        public long id;
    }

    private static Ident getIdentForDocId(String docId) {
        final Ident ident = new Ident();
        final int split = docId.indexOf(':');
        if (split == -1) {
            ident.type = docId;
            ident.id = -1;
        } else {
            ident.type = docId.substring(0, split);
            ident.id = Long.parseLong(docId.substring(split + 1));
        }
        return ident;
    }

    private static String getDocIdForIdent(String type, long id) {
        return type + ":" + id;
    }

    private static String[] resolveRootProjection(String[] projection) {
        return projection != null ? projection : DEFAULT_ROOT_PROJECTION;
    }

    private static String[] resolveDocumentProjection(String[] projection) {
        return projection != null ? projection : DEFAULT_DOCUMENT_PROJECTION;
    }

    private Uri getUriForDocumentId(String docId) {
        final Ident ident = getIdentForDocId(docId);
        if (TYPE_IMAGE.equals(ident.type) && ident.id != -1) {
            return ContentUris.withAppendedId(
                    Images.Media.EXTERNAL_CONTENT_URI, ident.id);
        } else if (TYPE_VIDEO.equals(ident.type) && ident.id != -1) {
            return ContentUris.withAppendedId(
                    Video.Media.EXTERNAL_CONTENT_URI, ident.id);
        } else if (TYPE_AUDIO.equals(ident.type) && ident.id != -1) {
            return ContentUris.withAppendedId(
                    Audio.Media.EXTERNAL_CONTENT_URI, ident.id);
        } else {
            throw new UnsupportedOperationException("Unsupported document " + docId);
        }
    }

    @Override
    public void deleteDocument(String docId) throws FileNotFoundException {
        final Uri target = getUriForDocumentId(docId);

        // Delegate to real provider
        final long token = Binder.clearCallingIdentity();
        try {
            getContext().getContentResolver().delete(target, null, null);
        } finally {
            Binder.restoreCallingIdentity(token);
        }
    }

    @Override
    public @Nullable Bundle getDocumentMetadata(String docId) throws FileNotFoundException {

        String mimeType = getDocumentType(docId);

        if (MetadataReader.isSupportedMimeType(mimeType)) {
            return getDocumentMetadataFromStream(docId, mimeType);
        } else {
            return getDocumentMetadataFromIndex(docId);
        }
    }

    private @Nullable Bundle getDocumentMetadataFromStream(String docId, String mimeType) {
        assert MetadataReader.isSupportedMimeType(mimeType);
        InputStream stream = null;
        try {
            stream = new ParcelFileDescriptor.AutoCloseInputStream(
                    openDocument(docId, "r", null));
            Bundle metadata = new Bundle();
            MetadataReader.getMetadata(metadata, stream, mimeType, null);
            return metadata;
        } catch (IOException io) {
            return null;
        } finally {
            IoUtils.closeQuietly(stream);
        }
    }

    public @Nullable Bundle getDocumentMetadataFromIndex(String docId)
            throws FileNotFoundException {

        final Ident ident = getIdentForDocId(docId);

        Map<String, String> columnMap = null;
        String tagType;
        Uri query;

        switch (ident.type) {
            case TYPE_IMAGE:
                columnMap = IMAGE_COLUMN_MAP;
                tagType = DocumentsContract.METADATA_EXIF;
                query = Images.Media.EXTERNAL_CONTENT_URI;
                break;
            case TYPE_VIDEO:
                columnMap = VIDEO_COLUMN_MAP;
                tagType = METADATA_KEY_VIDEO;
                query = Video.Media.EXTERNAL_CONTENT_URI;
                break;
            case TYPE_AUDIO:
                columnMap = AUDIO_COLUMN_MAP;
                tagType = METADATA_KEY_AUDIO;
                query = Audio.Media.EXTERNAL_CONTENT_URI;
                break;
            default:
                // Unsupported file type.
                throw new FileNotFoundException(
                    "Metadata request for unsupported file type: " + ident.type);
        }

        final long token = Binder.clearCallingIdentity();
        Cursor cursor = null;
        Bundle result = null;

        final ContentResolver resolver = getContext().getContentResolver();
        Collection<String> columns = columnMap.keySet();
        String[] projection = columns.toArray(new String[columns.size()]);
        try {
            cursor = resolver.query(
                    query,
                    projection,
                    BaseColumns._ID + "=?",
                    new String[]{Long.toString(ident.id)},
                    null);

            if (!cursor.moveToFirst()) {
                throw new FileNotFoundException("Can't find document id: " + docId);
            }

            final Bundle metadata = extractMetadataFromCursor(cursor, columnMap);
            result = new Bundle();
            result.putBundle(tagType, metadata);
            result.putStringArray(
                    DocumentsContract.METADATA_TYPES,
                    new String[]{tagType});
        } finally {
            IoUtils.closeQuietly(cursor);
            Binder.restoreCallingIdentity(token);
        }
        return result;
    }

    private static Bundle extractMetadataFromCursor(Cursor cursor, Map<String, String> columns) {

        assert (cursor.getCount() == 1);

        final Bundle metadata = new Bundle();
        for (String col : columns.keySet()) {

            int index = cursor.getColumnIndex(col);
            String bundleTag = columns.get(col);

            // Special case to be able to pull longs out of a cursor, as long is not a supported
            // field of getType.
            if (ExifInterface.TAG_DATETIME.equals(bundleTag)) {
                // formate string to be consistent with how EXIF interface formats the date.
                long date = cursor.getLong(index);
                String format = DateFormat.getBestDateTimePattern(Locale.getDefault(),
                    "MMM dd, yyyy, hh:mm");
                metadata.putString(bundleTag, DateFormat.format(format, date).toString());
                continue;
            }

            switch (cursor.getType(index)) {
                case Cursor.FIELD_TYPE_INTEGER:
                    metadata.putInt(bundleTag, cursor.getInt(index));
                    break;
                case Cursor.FIELD_TYPE_FLOAT:
                    //Errors on the side of greater precision since interface doesnt support doubles
                    metadata.putFloat(bundleTag, cursor.getFloat(index));
                    break;
                case Cursor.FIELD_TYPE_STRING:
                    metadata.putString(bundleTag, cursor.getString(index));
                    break;
                case Cursor.FIELD_TYPE_BLOB:
                    Log.d(TAG, "Unsupported type, blob, for col: " + bundleTag);
                    break;
                case Cursor.FIELD_TYPE_NULL:
                    Log.d(TAG, "Unsupported type, null, for col: " + bundleTag);
                    break;
                default:
                    throw new RuntimeException("Data type not supported");
            }
        }

        return metadata;
    }

    @Override
    public Cursor queryRoots(String[] projection) throws FileNotFoundException {
        final MatrixCursor result = new MatrixCursor(resolveRootProjection(projection));
        includeImagesRoot(result);
        includeVideosRoot(result);
        includeAudioRoot(result);
        return result;
    }

    @Override
    public Cursor queryDocument(String docId, String[] projection) throws FileNotFoundException {
        final ContentResolver resolver = getContext().getContentResolver();
        final MatrixCursor result = new MatrixCursor(resolveDocumentProjection(projection));
        final Ident ident = getIdentForDocId(docId);
        final String[] queryArgs = new String[] { Long.toString(ident.id) } ;

        final long token = Binder.clearCallingIdentity();
        Cursor cursor = null;
        try {
            if (TYPE_IMAGES_ROOT.equals(ident.type)) {
                // single root
                includeImagesRootDocument(result);
            } else if (TYPE_IMAGES_BUCKET.equals(ident.type)) {
                // single bucket
                cursor = resolver.query(Images.Media.EXTERNAL_CONTENT_URI,
                        ImagesBucketQuery.PROJECTION, ImageColumns.BUCKET_ID + "=?",
                        queryArgs, ImagesBucketQuery.SORT_ORDER);
                copyNotificationUri(result, cursor);
                if (cursor.moveToFirst()) {
                    includeImagesBucket(result, cursor);
                }
            } else if (TYPE_IMAGE.equals(ident.type)) {
                // single image
                cursor = resolver.query(Images.Media.EXTERNAL_CONTENT_URI,
                        ImageQuery.PROJECTION, BaseColumns._ID + "=?", queryArgs,
                        null);
                copyNotificationUri(result, cursor);
                if (cursor.moveToFirst()) {
                    includeImage(result, cursor);
                }
            } else if (TYPE_VIDEOS_ROOT.equals(ident.type)) {
                // single root
                includeVideosRootDocument(result);
            } else if (TYPE_VIDEOS_BUCKET.equals(ident.type)) {
                // single bucket
                cursor = resolver.query(Video.Media.EXTERNAL_CONTENT_URI,
                        VideosBucketQuery.PROJECTION, VideoColumns.BUCKET_ID + "=?",
                        queryArgs, VideosBucketQuery.SORT_ORDER);
                copyNotificationUri(result, cursor);
                if (cursor.moveToFirst()) {
                    includeVideosBucket(result, cursor);
                }
            } else if (TYPE_VIDEO.equals(ident.type)) {
                // single video
                cursor = resolver.query(Video.Media.EXTERNAL_CONTENT_URI,
                        VideoQuery.PROJECTION, BaseColumns._ID + "=?", queryArgs,
                        null);
                copyNotificationUri(result, cursor);
                if (cursor.moveToFirst()) {
                    includeVideo(result, cursor);
                }
            } else if (TYPE_AUDIO_ROOT.equals(ident.type)) {
                // single root
                includeAudioRootDocument(result);
            } else if (TYPE_ARTIST.equals(ident.type)) {
                // single artist
                cursor = resolver.query(Artists.EXTERNAL_CONTENT_URI,
                        ArtistQuery.PROJECTION, BaseColumns._ID + "=?", queryArgs,
                        null);
                copyNotificationUri(result, cursor);
                if (cursor.moveToFirst()) {
                    includeArtist(result, cursor);
                }
            } else if (TYPE_ALBUM.equals(ident.type)) {
                // single album
                cursor = resolver.query(Albums.EXTERNAL_CONTENT_URI,
                        AlbumQuery.PROJECTION, BaseColumns._ID + "=?", queryArgs,
                        null);
                copyNotificationUri(result, cursor);
                if (cursor.moveToFirst()) {
                    includeAlbum(result, cursor);
                }
            } else if (TYPE_AUDIO.equals(ident.type)) {
                // single song
                cursor = resolver.query(Audio.Media.EXTERNAL_CONTENT_URI,
                        SongQuery.PROJECTION, BaseColumns._ID + "=?", queryArgs,
                        null);
                copyNotificationUri(result, cursor);
                if (cursor.moveToFirst()) {
                    includeAudio(result, cursor);
                }
            } else {
                throw new UnsupportedOperationException("Unsupported document " + docId);
            }
        } finally {
            IoUtils.closeQuietly(cursor);
            Binder.restoreCallingIdentity(token);
        }
        return result;
    }

    @Override
    public Cursor queryChildDocuments(String docId, String[] projection, String sortOrder)
            throws FileNotFoundException {
        final ContentResolver resolver = getContext().getContentResolver();
        final MatrixCursor result = new MatrixCursor(resolveDocumentProjection(projection));
        final Ident ident = getIdentForDocId(docId);
        final String[] queryArgs = new String[] { Long.toString(ident.id) } ;

        final long token = Binder.clearCallingIdentity();
        Cursor cursor = null;
        try {
            if (TYPE_IMAGES_ROOT.equals(ident.type)) {
                // include all unique buckets
                cursor = resolver.query(Images.Media.EXTERNAL_CONTENT_URI,
                        ImagesBucketQuery.PROJECTION, null, null, ImagesBucketQuery.SORT_ORDER);
                // multiple orders
                copyNotificationUri(result, cursor);
                long lastId = Long.MIN_VALUE;
                while (cursor.moveToNext()) {
                    final long id = cursor.getLong(ImagesBucketQuery.BUCKET_ID);
                    if (lastId != id) {
                        includeImagesBucket(result, cursor);
                        lastId = id;
                    }
                }
            } else if (TYPE_IMAGES_BUCKET.equals(ident.type)) {
                // include images under bucket
                cursor = resolver.query(Images.Media.EXTERNAL_CONTENT_URI,
                        ImageQuery.PROJECTION, ImageColumns.BUCKET_ID + "=?",
                        queryArgs, null);
                copyNotificationUri(result, cursor);
                while (cursor.moveToNext()) {
                    includeImage(result, cursor);
                }
            } else if (TYPE_VIDEOS_ROOT.equals(ident.type)) {
                // include all unique buckets
                cursor = resolver.query(Video.Media.EXTERNAL_CONTENT_URI,
                        VideosBucketQuery.PROJECTION, null, null, VideosBucketQuery.SORT_ORDER);
                copyNotificationUri(result, cursor);
                long lastId = Long.MIN_VALUE;
                while (cursor.moveToNext()) {
                    final long id = cursor.getLong(VideosBucketQuery.BUCKET_ID);
                    if (lastId != id) {
                        includeVideosBucket(result, cursor);
                        lastId = id;
                    }
                }
            } else if (TYPE_VIDEOS_BUCKET.equals(ident.type)) {
                // include videos under bucket
                cursor = resolver.query(Video.Media.EXTERNAL_CONTENT_URI,
                        VideoQuery.PROJECTION, VideoColumns.BUCKET_ID + "=?",
                        queryArgs, null);
                copyNotificationUri(result, cursor);
                while (cursor.moveToNext()) {
                    includeVideo(result, cursor);
                }
            } else if (TYPE_AUDIO_ROOT.equals(ident.type)) {
                // include all artists
                cursor = resolver.query(Audio.Artists.EXTERNAL_CONTENT_URI,
                        ArtistQuery.PROJECTION, null, null, null);
                copyNotificationUri(result, cursor);
                while (cursor.moveToNext()) {
                    includeArtist(result, cursor);
                }
            } else if (TYPE_ARTIST.equals(ident.type)) {
                // include all albums under artist
                cursor = resolver.query(Artists.Albums.getContentUri("external", ident.id),
                        AlbumQuery.PROJECTION, null, null, null);
                copyNotificationUri(result, cursor);
                while (cursor.moveToNext()) {
                    includeAlbum(result, cursor);
                }
            } else if (TYPE_ALBUM.equals(ident.type)) {
                // include all songs under album
                cursor = resolver.query(Audio.Media.EXTERNAL_CONTENT_URI,
                        SongQuery.PROJECTION, AudioColumns.ALBUM_ID + "=?",
                        queryArgs, null);
                copyNotificationUri(result, cursor);
                while (cursor.moveToNext()) {
                    includeAudio(result, cursor);
                }
            } else {
                throw new UnsupportedOperationException("Unsupported document " + docId);
            }
        } finally {
            IoUtils.closeQuietly(cursor);
            Binder.restoreCallingIdentity(token);
        }
        return result;
    }

    @Override
    public Cursor queryRecentDocuments(String rootId, String[] projection)
            throws FileNotFoundException {
        final ContentResolver resolver = getContext().getContentResolver();
        final MatrixCursor result = new MatrixCursor(resolveDocumentProjection(projection));

        final long token = Binder.clearCallingIdentity();
        Cursor cursor = null;
        try {
            if (TYPE_IMAGES_ROOT.equals(rootId)) {
                // include all unique buckets
                cursor = resolver.query(Images.Media.EXTERNAL_CONTENT_URI,
                        ImageQuery.PROJECTION, null, null, ImageColumns.DATE_MODIFIED + " DESC");
                copyNotificationUri(result, cursor);
                while (cursor.moveToNext() && result.getCount() < 64) {
                    includeImage(result, cursor);
                }
            } else if (TYPE_VIDEOS_ROOT.equals(rootId)) {
                // include all unique buckets
                cursor = resolver.query(Video.Media.EXTERNAL_CONTENT_URI,
                        VideoQuery.PROJECTION, null, null, VideoColumns.DATE_MODIFIED + " DESC");
                copyNotificationUri(result, cursor);
                while (cursor.moveToNext() && result.getCount() < 64) {
                    includeVideo(result, cursor);
                }
            } else {
                throw new UnsupportedOperationException("Unsupported root " + rootId);
            }
        } finally {
            IoUtils.closeQuietly(cursor);
            Binder.restoreCallingIdentity(token);
        }
        return result;
    }

    @Override
    public Cursor querySearchDocuments(String rootId, String query, String[] projection)
            throws FileNotFoundException {
        final ContentResolver resolver = getContext().getContentResolver();
        final MatrixCursor result = new MatrixCursor(resolveDocumentProjection(projection));

        final long token = Binder.clearCallingIdentity();
        final String[] queryArgs = new String[] { "%" + query + "%" };
        Cursor cursor = null;
        try {
            if (TYPE_IMAGES_ROOT.equals(rootId)) {
                cursor = resolver.query(Images.Media.EXTERNAL_CONTENT_URI, ImageQuery.PROJECTION,
                        ImageColumns.DISPLAY_NAME + " LIKE ?", queryArgs,
                        ImageColumns.DATE_MODIFIED + " DESC");
                copyNotificationUri(result, cursor);
                while (cursor.moveToNext()) {
                    includeImage(result, cursor);
                }
            } else if (TYPE_VIDEOS_ROOT.equals(rootId)) {
                cursor = resolver.query(Video.Media.EXTERNAL_CONTENT_URI, VideoQuery.PROJECTION,
                        VideoColumns.DISPLAY_NAME + " LIKE ?", queryArgs,
                        VideoColumns.DATE_MODIFIED + " DESC");
                copyNotificationUri(result, cursor);
                while (cursor.moveToNext()) {
                    includeVideo(result, cursor);
                }
            } else if (TYPE_AUDIO_ROOT.equals(rootId)) {
                cursor = resolver.query(Audio.Media.EXTERNAL_CONTENT_URI, SongQuery.PROJECTION,
                        AudioColumns.TITLE + " LIKE ?", queryArgs,
                        AudioColumns.DATE_MODIFIED + " DESC");
                copyNotificationUri(result, cursor);
                while (cursor.moveToNext()) {
                    includeAudio(result, cursor);
                }
            } else {
                throw new UnsupportedOperationException("Unsupported root " + rootId);
            }
        } finally {
            IoUtils.closeQuietly(cursor);
            Binder.restoreCallingIdentity(token);
        }
        return result;
    }

    @Override
    public ParcelFileDescriptor openDocument(String docId, String mode, CancellationSignal signal)
            throws FileNotFoundException {
        final Uri target = getUriForDocumentId(docId);

        if (!"r".equals(mode)) {
            throw new IllegalArgumentException("Media is read-only");
        }

        // Delegate to real provider
        final long token = Binder.clearCallingIdentity();
        try {
            return getContext().getContentResolver().openFileDescriptor(target, mode);
        } finally {
            Binder.restoreCallingIdentity(token);
        }
    }

    @Override
    public AssetFileDescriptor openDocumentThumbnail(
            String docId, Point sizeHint, CancellationSignal signal) throws FileNotFoundException {
        final Ident ident = getIdentForDocId(docId);

        final long token = Binder.clearCallingIdentity();
        try {
            if (TYPE_IMAGES_BUCKET.equals(ident.type)) {
                final long id = getImageForBucketCleared(ident.id);
                return openOrCreateImageThumbnailCleared(id, signal);
            } else if (TYPE_IMAGE.equals(ident.type)) {
                return openOrCreateImageThumbnailCleared(ident.id, signal);
            } else if (TYPE_VIDEOS_BUCKET.equals(ident.type)) {
                final long id = getVideoForBucketCleared(ident.id);
                return openOrCreateVideoThumbnailCleared(id, signal);
            } else if (TYPE_VIDEO.equals(ident.type)) {
                return openOrCreateVideoThumbnailCleared(ident.id, signal);
            } else {
                throw new UnsupportedOperationException("Unsupported document " + docId);
            }
        } finally {
            Binder.restoreCallingIdentity(token);
        }
    }

    private boolean isEmpty(Uri uri) {
        final ContentResolver resolver = getContext().getContentResolver();
        final long token = Binder.clearCallingIdentity();
        Cursor cursor = null;
        try {
            cursor = resolver.query(uri, new String[] {
                    BaseColumns._ID }, null, null, null);
            return (cursor == null) || (cursor.getCount() == 0);
        } finally {
            IoUtils.closeQuietly(cursor);
            Binder.restoreCallingIdentity(token);
        }
    }

    private void includeImagesRoot(MatrixCursor result) {
        int flags = Root.FLAG_LOCAL_ONLY | Root.FLAG_SUPPORTS_RECENTS | Root.FLAG_SUPPORTS_SEARCH;
        if (isEmpty(Images.Media.EXTERNAL_CONTENT_URI)) {
            flags |= Root.FLAG_EMPTY;
            sReturnedImagesEmpty = true;
        }

        final RowBuilder row = result.newRow();
        row.add(Root.COLUMN_ROOT_ID, TYPE_IMAGES_ROOT);
        row.add(Root.COLUMN_FLAGS, flags);
        row.add(Root.COLUMN_TITLE, getContext().getString(R.string.root_images));
        row.add(Root.COLUMN_DOCUMENT_ID, TYPE_IMAGES_ROOT);
        row.add(Root.COLUMN_MIME_TYPES, IMAGE_MIME_TYPES);
    }

    private void includeVideosRoot(MatrixCursor result) {
        int flags = Root.FLAG_LOCAL_ONLY | Root.FLAG_SUPPORTS_RECENTS | Root.FLAG_SUPPORTS_SEARCH;
        if (isEmpty(Video.Media.EXTERNAL_CONTENT_URI)) {
            flags |= Root.FLAG_EMPTY;
            sReturnedVideosEmpty = true;
        }

        final RowBuilder row = result.newRow();
        row.add(Root.COLUMN_ROOT_ID, TYPE_VIDEOS_ROOT);
        row.add(Root.COLUMN_FLAGS, flags);
        row.add(Root.COLUMN_TITLE, getContext().getString(R.string.root_videos));
        row.add(Root.COLUMN_DOCUMENT_ID, TYPE_VIDEOS_ROOT);
        row.add(Root.COLUMN_MIME_TYPES, VIDEO_MIME_TYPES);
    }

    private void includeAudioRoot(MatrixCursor result) {
        int flags = Root.FLAG_LOCAL_ONLY | Root.FLAG_SUPPORTS_SEARCH;
        if (isEmpty(Audio.Media.EXTERNAL_CONTENT_URI)) {
            flags |= Root.FLAG_EMPTY;
            sReturnedAudioEmpty = true;
        }

        final RowBuilder row = result.newRow();
        row.add(Root.COLUMN_ROOT_ID, TYPE_AUDIO_ROOT);
        row.add(Root.COLUMN_FLAGS, flags);
        row.add(Root.COLUMN_TITLE, getContext().getString(R.string.root_audio));
        row.add(Root.COLUMN_DOCUMENT_ID, TYPE_AUDIO_ROOT);
        row.add(Root.COLUMN_MIME_TYPES, AUDIO_MIME_TYPES);
    }

    private void includeImagesRootDocument(MatrixCursor result) {
        final RowBuilder row = result.newRow();
        row.add(Document.COLUMN_DOCUMENT_ID, TYPE_IMAGES_ROOT);
        row.add(Document.COLUMN_DISPLAY_NAME, getContext().getString(R.string.root_images));
        row.add(Document.COLUMN_FLAGS,
                Document.FLAG_DIR_PREFERS_GRID | Document.FLAG_DIR_PREFERS_LAST_MODIFIED);
        row.add(Document.COLUMN_MIME_TYPE, Document.MIME_TYPE_DIR);
    }

    private void includeVideosRootDocument(MatrixCursor result) {
        final RowBuilder row = result.newRow();
        row.add(Document.COLUMN_DOCUMENT_ID, TYPE_VIDEOS_ROOT);
        row.add(Document.COLUMN_DISPLAY_NAME, getContext().getString(R.string.root_videos));
        row.add(Document.COLUMN_FLAGS,
                Document.FLAG_DIR_PREFERS_GRID | Document.FLAG_DIR_PREFERS_LAST_MODIFIED);
        row.add(Document.COLUMN_MIME_TYPE, Document.MIME_TYPE_DIR);
    }

    private void includeAudioRootDocument(MatrixCursor result) {
        final RowBuilder row = result.newRow();
        row.add(Document.COLUMN_DOCUMENT_ID, TYPE_AUDIO_ROOT);
        row.add(Document.COLUMN_DISPLAY_NAME, getContext().getString(R.string.root_audio));
        row.add(Document.COLUMN_MIME_TYPE, Document.MIME_TYPE_DIR);
    }

    private interface ImagesBucketQuery {
        final String[] PROJECTION = new String[] {
                ImageColumns.BUCKET_ID,
                ImageColumns.BUCKET_DISPLAY_NAME,
                ImageColumns.DATE_MODIFIED };
        final String SORT_ORDER = ImageColumns.BUCKET_ID + ", " + ImageColumns.DATE_MODIFIED
                + " DESC";

        final int BUCKET_ID = 0;
        final int BUCKET_DISPLAY_NAME = 1;
        final int DATE_MODIFIED = 2;
    }

    private void includeImagesBucket(MatrixCursor result, Cursor cursor) {
        final long id = cursor.getLong(ImagesBucketQuery.BUCKET_ID);
        final String docId = getDocIdForIdent(TYPE_IMAGES_BUCKET, id);

        final RowBuilder row = result.newRow();
        row.add(Document.COLUMN_DOCUMENT_ID, docId);
        row.add(Document.COLUMN_DISPLAY_NAME,
                cursor.getString(ImagesBucketQuery.BUCKET_DISPLAY_NAME));
        row.add(Document.COLUMN_MIME_TYPE, Document.MIME_TYPE_DIR);
        row.add(Document.COLUMN_LAST_MODIFIED,
                cursor.getLong(ImagesBucketQuery.DATE_MODIFIED) * DateUtils.SECOND_IN_MILLIS);
        row.add(Document.COLUMN_FLAGS, Document.FLAG_DIR_PREFERS_GRID
                | Document.FLAG_SUPPORTS_THUMBNAIL | Document.FLAG_DIR_PREFERS_LAST_MODIFIED);
    }

    private interface ImageQuery {
        final String[] PROJECTION = new String[] {
                ImageColumns._ID,
                ImageColumns.DISPLAY_NAME,
                ImageColumns.MIME_TYPE,
                ImageColumns.SIZE,
                ImageColumns.DATE_MODIFIED };

        final int _ID = 0;
        final int DISPLAY_NAME = 1;
        final int MIME_TYPE = 2;
        final int SIZE = 3;
        final int DATE_MODIFIED = 4;
    }

    private void includeImage(MatrixCursor result, Cursor cursor) {
        final long id = cursor.getLong(ImageQuery._ID);
        final String docId = getDocIdForIdent(TYPE_IMAGE, id);

        final RowBuilder row = result.newRow();
        row.add(Document.COLUMN_DOCUMENT_ID, docId);
        row.add(Document.COLUMN_DISPLAY_NAME, cursor.getString(ImageQuery.DISPLAY_NAME));
        row.add(Document.COLUMN_SIZE, cursor.getLong(ImageQuery.SIZE));
        row.add(Document.COLUMN_MIME_TYPE, cursor.getString(ImageQuery.MIME_TYPE));
        row.add(Document.COLUMN_LAST_MODIFIED,
                cursor.getLong(ImageQuery.DATE_MODIFIED) * DateUtils.SECOND_IN_MILLIS);
        row.add(Document.COLUMN_FLAGS,
                Document.FLAG_SUPPORTS_THUMBNAIL
                    | Document.FLAG_SUPPORTS_DELETE
                    | Document.FLAG_SUPPORTS_METADATA);
    }

    private interface VideosBucketQuery {
        final String[] PROJECTION = new String[] {
                VideoColumns.BUCKET_ID,
                VideoColumns.BUCKET_DISPLAY_NAME,
                VideoColumns.DATE_MODIFIED };
        final String SORT_ORDER = VideoColumns.BUCKET_ID + ", " + VideoColumns.DATE_MODIFIED
                + " DESC";

        final int BUCKET_ID = 0;
        final int BUCKET_DISPLAY_NAME = 1;
        final int DATE_MODIFIED = 2;
    }

    private void includeVideosBucket(MatrixCursor result, Cursor cursor) {
        final long id = cursor.getLong(VideosBucketQuery.BUCKET_ID);
        final String docId = getDocIdForIdent(TYPE_VIDEOS_BUCKET, id);

        final RowBuilder row = result.newRow();
        row.add(Document.COLUMN_DOCUMENT_ID, docId);
        row.add(Document.COLUMN_DISPLAY_NAME,
                cursor.getString(VideosBucketQuery.BUCKET_DISPLAY_NAME));
        row.add(Document.COLUMN_MIME_TYPE, Document.MIME_TYPE_DIR);
        row.add(Document.COLUMN_LAST_MODIFIED,
                cursor.getLong(VideosBucketQuery.DATE_MODIFIED) * DateUtils.SECOND_IN_MILLIS);
        row.add(Document.COLUMN_FLAGS, Document.FLAG_DIR_PREFERS_GRID
                | Document.FLAG_SUPPORTS_THUMBNAIL | Document.FLAG_DIR_PREFERS_LAST_MODIFIED);
    }

    private interface VideoQuery {
        final String[] PROJECTION = new String[] {
                VideoColumns._ID,
                VideoColumns.DISPLAY_NAME,
                VideoColumns.MIME_TYPE,
                VideoColumns.SIZE,
                VideoColumns.DATE_MODIFIED };

        final int _ID = 0;
        final int DISPLAY_NAME = 1;
        final int MIME_TYPE = 2;
        final int SIZE = 3;
        final int DATE_MODIFIED = 4;
    }

    private void includeVideo(MatrixCursor result, Cursor cursor) {
        final long id = cursor.getLong(VideoQuery._ID);
        final String docId = getDocIdForIdent(TYPE_VIDEO, id);

        final RowBuilder row = result.newRow();
        row.add(Document.COLUMN_DOCUMENT_ID, docId);
        row.add(Document.COLUMN_DISPLAY_NAME, cursor.getString(VideoQuery.DISPLAY_NAME));
        row.add(Document.COLUMN_SIZE, cursor.getLong(VideoQuery.SIZE));
        row.add(Document.COLUMN_MIME_TYPE, cursor.getString(VideoQuery.MIME_TYPE));
        row.add(Document.COLUMN_LAST_MODIFIED,
                cursor.getLong(VideoQuery.DATE_MODIFIED) * DateUtils.SECOND_IN_MILLIS);
        row.add(Document.COLUMN_FLAGS,
                Document.FLAG_SUPPORTS_THUMBNAIL
                    | Document.FLAG_SUPPORTS_DELETE
                    | Document.FLAG_SUPPORTS_METADATA);
    }

    private interface ArtistQuery {
        final String[] PROJECTION = new String[] {
                BaseColumns._ID,
                ArtistColumns.ARTIST };

        final int _ID = 0;
        final int ARTIST = 1;
    }

    private void includeArtist(MatrixCursor result, Cursor cursor) {
        final long id = cursor.getLong(ArtistQuery._ID);
        final String docId = getDocIdForIdent(TYPE_ARTIST, id);

        final RowBuilder row = result.newRow();
        row.add(Document.COLUMN_DOCUMENT_ID, docId);
        row.add(Document.COLUMN_DISPLAY_NAME,
                cleanUpMediaDisplayName(cursor.getString(ArtistQuery.ARTIST)));
        row.add(Document.COLUMN_MIME_TYPE, Document.MIME_TYPE_DIR);
    }

    private interface AlbumQuery {
        final String[] PROJECTION = new String[] {
                BaseColumns._ID,
                AlbumColumns.ALBUM };

        final int _ID = 0;
        final int ALBUM = 1;
    }

    private void includeAlbum(MatrixCursor result, Cursor cursor) {
        final long id = cursor.getLong(AlbumQuery._ID);
        final String docId = getDocIdForIdent(TYPE_ALBUM, id);

        final RowBuilder row = result.newRow();
        row.add(Document.COLUMN_DOCUMENT_ID, docId);
        row.add(Document.COLUMN_DISPLAY_NAME,
                cleanUpMediaDisplayName(cursor.getString(AlbumQuery.ALBUM)));
        row.add(Document.COLUMN_MIME_TYPE, Document.MIME_TYPE_DIR);
    }

    private interface SongQuery {
        final String[] PROJECTION = new String[] {
                AudioColumns._ID,
                AudioColumns.TITLE,
                AudioColumns.MIME_TYPE,
                AudioColumns.SIZE,
                AudioColumns.DATE_MODIFIED };

        final int _ID = 0;
        final int TITLE = 1;
        final int MIME_TYPE = 2;
        final int SIZE = 3;
        final int DATE_MODIFIED = 4;
    }

    private void includeAudio(MatrixCursor result, Cursor cursor) {
        final long id = cursor.getLong(SongQuery._ID);
        final String docId = getDocIdForIdent(TYPE_AUDIO, id);

        final RowBuilder row = result.newRow();
        row.add(Document.COLUMN_DOCUMENT_ID, docId);
        row.add(Document.COLUMN_DISPLAY_NAME, cursor.getString(SongQuery.TITLE));
        row.add(Document.COLUMN_SIZE, cursor.getLong(SongQuery.SIZE));
        row.add(Document.COLUMN_MIME_TYPE, cursor.getString(SongQuery.MIME_TYPE));
        row.add(Document.COLUMN_LAST_MODIFIED,
                cursor.getLong(SongQuery.DATE_MODIFIED) * DateUtils.SECOND_IN_MILLIS);
        row.add(Document.COLUMN_FLAGS, Document.FLAG_SUPPORTS_DELETE
                | Document.FLAG_SUPPORTS_METADATA);
    }

    private interface ImagesBucketThumbnailQuery {
        final String[] PROJECTION = new String[] {
                ImageColumns._ID,
                ImageColumns.BUCKET_ID,
                ImageColumns.DATE_MODIFIED };

        final int _ID = 0;
        final int BUCKET_ID = 1;
        final int DATE_MODIFIED = 2;
    }

    private long getImageForBucketCleared(long bucketId) throws FileNotFoundException {
        final ContentResolver resolver = getContext().getContentResolver();
        Cursor cursor = null;
        try {
            cursor = resolver.query(Images.Media.EXTERNAL_CONTENT_URI,
                    ImagesBucketThumbnailQuery.PROJECTION, ImageColumns.BUCKET_ID + "=" + bucketId,
                    null, ImageColumns.DATE_MODIFIED + " DESC");
            if (cursor.moveToFirst()) {
                return cursor.getLong(ImagesBucketThumbnailQuery._ID);
            }
        } finally {
            IoUtils.closeQuietly(cursor);
        }
        throw new FileNotFoundException("No video found for bucket");
    }

    private interface ImageThumbnailQuery {
        final String[] PROJECTION = new String[] {
                Images.Thumbnails.DATA };

        final int _DATA = 0;
    }

    private ParcelFileDescriptor openImageThumbnailCleared(long id, CancellationSignal signal)
            throws FileNotFoundException {
        final ContentResolver resolver = getContext().getContentResolver();

        Cursor cursor = null;
        try {
            cursor = resolver.query(Images.Thumbnails.EXTERNAL_CONTENT_URI,
                    ImageThumbnailQuery.PROJECTION, Images.Thumbnails.IMAGE_ID + "=" + id, null,
                    null, signal);
            if (cursor.moveToFirst()) {
                final String data = cursor.getString(ImageThumbnailQuery._DATA);
                return ParcelFileDescriptor.open(
                        new File(data), ParcelFileDescriptor.MODE_READ_ONLY);
            }
        } finally {
            IoUtils.closeQuietly(cursor);
        }
        return null;
    }

    private AssetFileDescriptor openOrCreateImageThumbnailCleared(
            long id, CancellationSignal signal) throws FileNotFoundException {
        final ContentResolver resolver = getContext().getContentResolver();

        ParcelFileDescriptor pfd = openImageThumbnailCleared(id, signal);
        if (pfd == null) {
            // No thumbnail yet, so generate. This is messy, since we drop the
            // Bitmap on the floor, but its the least-complicated way.
            final BitmapFactory.Options opts = new BitmapFactory.Options();
            opts.inJustDecodeBounds = true;
            Images.Thumbnails.getThumbnail(resolver, id, Images.Thumbnails.MINI_KIND, opts);

            pfd = openImageThumbnailCleared(id, signal);
        }

        if (pfd == null) {
            // Phoey, fallback to full image
            final Uri fullUri = ContentUris.withAppendedId(Images.Media.EXTERNAL_CONTENT_URI, id);
            pfd = resolver.openFileDescriptor(fullUri, "r", signal);
        }

        final int orientation = queryOrientationForImage(id, signal);
        final Bundle extras;
        if (orientation != 0) {
            extras = new Bundle(1);
            extras.putInt(DocumentsContract.EXTRA_ORIENTATION, orientation);
        } else {
            extras = null;
        }

        return new AssetFileDescriptor(pfd, 0, AssetFileDescriptor.UNKNOWN_LENGTH, extras);
    }

    private interface VideosBucketThumbnailQuery {
        final String[] PROJECTION = new String[] {
                VideoColumns._ID,
                VideoColumns.BUCKET_ID,
                VideoColumns.DATE_MODIFIED };

        final int _ID = 0;
        final int BUCKET_ID = 1;
        final int DATE_MODIFIED = 2;
    }

    private long getVideoForBucketCleared(long bucketId)
            throws FileNotFoundException {
        final ContentResolver resolver = getContext().getContentResolver();
        Cursor cursor = null;
        try {
            cursor = resolver.query(Video.Media.EXTERNAL_CONTENT_URI,
                    VideosBucketThumbnailQuery.PROJECTION, VideoColumns.BUCKET_ID + "=" + bucketId,
                    null, VideoColumns.DATE_MODIFIED + " DESC");
            if (cursor.moveToFirst()) {
                return cursor.getLong(VideosBucketThumbnailQuery._ID);
            }
        } finally {
            IoUtils.closeQuietly(cursor);
        }
        throw new FileNotFoundException("No video found for bucket");
    }

    private interface VideoThumbnailQuery {
        final String[] PROJECTION = new String[] {
                Video.Thumbnails.DATA };

        final int _DATA = 0;
    }

    private AssetFileDescriptor openVideoThumbnailCleared(long id, CancellationSignal signal)
            throws FileNotFoundException {
        final ContentResolver resolver = getContext().getContentResolver();
        Cursor cursor = null;
        try {
            cursor = resolver.query(Video.Thumbnails.EXTERNAL_CONTENT_URI,
                    VideoThumbnailQuery.PROJECTION, Video.Thumbnails.VIDEO_ID + "=" + id, null,
                    null, signal);
            if (cursor.moveToFirst()) {
                final String data = cursor.getString(VideoThumbnailQuery._DATA);
                return new AssetFileDescriptor(ParcelFileDescriptor.open(
                        new File(data), ParcelFileDescriptor.MODE_READ_ONLY), 0,
                        AssetFileDescriptor.UNKNOWN_LENGTH);
            }
        } finally {
            IoUtils.closeQuietly(cursor);
        }
        return null;
    }

    private AssetFileDescriptor openOrCreateVideoThumbnailCleared(
            long id, CancellationSignal signal) throws FileNotFoundException {
        final ContentResolver resolver = getContext().getContentResolver();

        AssetFileDescriptor afd = openVideoThumbnailCleared(id, signal);
        if (afd == null) {
            // No thumbnail yet, so generate. This is messy, since we drop the
            // Bitmap on the floor, but its the least-complicated way.
            final BitmapFactory.Options opts = new BitmapFactory.Options();
            opts.inJustDecodeBounds = true;
            Video.Thumbnails.getThumbnail(resolver, id, Video.Thumbnails.MINI_KIND, opts);

            afd = openVideoThumbnailCleared(id, signal);
        }

        return afd;
    }

    private interface ImageOrientationQuery {
        final String[] PROJECTION = new String[] {
                ImageColumns.ORIENTATION };

        final int ORIENTATION = 0;
    }

    private int queryOrientationForImage(long id, CancellationSignal signal) {
        final ContentResolver resolver = getContext().getContentResolver();

        Cursor cursor = null;
        try {
            cursor = resolver.query(Images.Media.EXTERNAL_CONTENT_URI,
                    ImageOrientationQuery.PROJECTION, ImageColumns._ID + "=" + id, null, null,
                    signal);
            if (cursor.moveToFirst()) {
                return cursor.getInt(ImageOrientationQuery.ORIENTATION);
            } else {
                Log.w(TAG, "Missing orientation data for " + id);
                return 0;
            }
        } finally {
            IoUtils.closeQuietly(cursor);
        }
    }

    private String cleanUpMediaDisplayName(String displayName) {
        if (!MediaStore.UNKNOWN_STRING.equals(displayName)) {
            return displayName;
        }
        return getContext().getResources().getString(com.android.internal.R.string.unknownName);
    }
}
