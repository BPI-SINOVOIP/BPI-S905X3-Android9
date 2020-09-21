/*
 * Copyright (C) 2012 The Android Open Source Project
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

package android.provider.cts;

import android.provider.cts.R;

import android.content.ContentResolver;
import android.content.ContentUris;
import android.content.ContentValues;
import android.database.Cursor;
import android.media.MediaCodecInfo;
import android.media.MediaCodecList;
import android.net.Uri;
import android.os.Environment;
import android.provider.MediaStore.Files;
import android.provider.MediaStore.Video.Media;
import android.provider.MediaStore.Video.Thumbnails;
import android.provider.MediaStore.Video.VideoColumns;
import android.test.AndroidTestCase;
import android.util.Log;

import com.android.compatibility.common.util.FileCopyHelper;
import com.android.compatibility.common.util.MediaUtils;

import java.io.File;
import java.io.IOException;

public class MediaStore_Video_ThumbnailsTest extends AndroidTestCase {
    private static final String TAG = "MediaStore_Video_ThumbnailsTest";

    private ContentResolver mResolver;

    private FileCopyHelper mFileHelper;

    private boolean hasCodec() {
        return MediaUtils.hasCodecForResourceAndDomain(
                mContext, R.raw.testthumbvideo, "video/");
    }

    @Override
    protected void setUp() throws Exception {
        super.setUp();
        mResolver = mContext.getContentResolver();
        mFileHelper = new FileCopyHelper(mContext);
    }

    @Override
    protected void tearDown() throws Exception {
        mFileHelper.clear();
        super.tearDown();
    }

    public void testGetContentUri() {
        Uri internalUri = Thumbnails.getContentUri(MediaStoreAudioTestHelper.INTERNAL_VOLUME_NAME);
        Uri externalUri = Thumbnails.getContentUri(MediaStoreAudioTestHelper.EXTERNAL_VOLUME_NAME);
        assertEquals(Thumbnails.INTERNAL_CONTENT_URI, internalUri);
        assertEquals(Thumbnails.EXTERNAL_CONTENT_URI, externalUri);
    }

    public void testGetThumbnail() throws Exception {
        // Insert a video into the provider.
        Uri videoUri = insertVideo();
        long videoId = ContentUris.parseId(videoUri);
        assertTrue(videoId != -1);
        assertEquals(ContentUris.withAppendedId(Media.EXTERNAL_CONTENT_URI, videoId),
                videoUri);

        // Get the current thumbnail count for future comparison.
        int count = getThumbnailCount(Thumbnails.EXTERNAL_CONTENT_URI);

        // Don't run the test if the codec isn't supported.
        if (!hasCodec()) {
            // Calling getThumbnail should not generate a new thumbnail.
            assertNull(Thumbnails.getThumbnail(mResolver, videoId, Thumbnails.MINI_KIND, null));
            Log.i(TAG, "SKIPPING testGetThumbnail(): codec not supported");
            return;
        }

        // Calling getThumbnail should generate a new thumbnail.
        assertNotNull(Thumbnails.getThumbnail(mResolver, videoId, Thumbnails.MINI_KIND, null));
        assertNotNull(Thumbnails.getThumbnail(mResolver, videoId, Thumbnails.MICRO_KIND, null));

        try {
            Thumbnails.getThumbnail(mResolver, videoId, Thumbnails.FULL_SCREEN_KIND, null);
            fail();
        } catch (IllegalArgumentException e) {
            // Full screen thumbnails not supported by getThumbnail...
        }

        // Check that an additional thumbnails have been registered.
        int count2 = getThumbnailCount(Thumbnails.EXTERNAL_CONTENT_URI);
        assertTrue(count2 > count);

        Cursor c = mResolver.query(Thumbnails.EXTERNAL_CONTENT_URI,
                new String[] { Thumbnails._ID, Thumbnails.DATA, Thumbnails.VIDEO_ID },
                null, null, null);

        if (c.moveToLast()) {
            long vid = c.getLong(2);
            assertEquals(videoId, vid);
            String path = c.getString(1);
            assertTrue("thumbnail file does not exist", new File(path).exists());
            long id = c.getLong(0);
            mResolver.delete(ContentUris.withAppendedId(Thumbnails.EXTERNAL_CONTENT_URI, id),
                    null, null);
            assertFalse("thumbnail file should no longer exist", new File(path).exists());
        }
        c.close();

        assertEquals(1, mResolver.delete(videoUri, null, null));
    }

    public void testThumbnailGenerationAndCleanup() throws Exception {

        if (!hasCodec()) {
            // we don't support video, so no need to run the test
            Log.i(TAG, "SKIPPING testThumbnailGenerationAndCleanup(): codec not supported");
            return;
        }

        // insert a video
        Uri uri = insertVideo();

        // request thumbnail creation
        Thumbnails.getThumbnail(mResolver, Long.valueOf(uri.getLastPathSegment()),
                Thumbnails.MINI_KIND, null /* options */);

        // query the thumbnail
        Cursor c = mResolver.query(
                Thumbnails.EXTERNAL_CONTENT_URI,
                new String [] {Thumbnails.DATA},
                "video_id=?",
                new String[] {uri.getLastPathSegment()},
                null /* sort */
                );
        assertTrue("couldn't find thumbnail", c.moveToNext());
        String path = c.getString(0);
        c.close();
        assertTrue("thumbnail does not exist", new File(path).exists());

        // delete the source video and check that the thumbnail is gone too
        mResolver.delete(uri, null /* where clause */, null /* where args */);
        assertFalse("thumbnail still exists after source file delete", new File(path).exists());

        // insert again
        uri = insertVideo();

        // request thumbnail creation
        Thumbnails.getThumbnail(mResolver, Long.valueOf(uri.getLastPathSegment()),
                Thumbnails.MINI_KIND, null);

        // query its thumbnail again
        c = mResolver.query(
                Thumbnails.EXTERNAL_CONTENT_URI,
                new String [] {Thumbnails.DATA},
                "video_id=?",
                new String[] {uri.getLastPathSegment()},
                null /* sort */
                );
        assertTrue("couldn't find thumbnail", c.moveToNext());
        path = c.getString(0);
        c.close();
        assertTrue("thumbnail does not exist", new File(path).exists());

        // update the media type
        ContentValues values = new ContentValues();
        values.put("media_type", 0);
        assertEquals("unexpected number of updated rows",
                1, mResolver.update(uri, values, null /* where */, null /* where args */));

        // video was marked as regular file in the database, which should have deleted its thumbnail

        // query its thumbnail again
        c = mResolver.query(
                Thumbnails.EXTERNAL_CONTENT_URI,
                new String [] {Thumbnails.DATA},
                "video_id=?",
                new String[] {uri.getLastPathSegment()},
                null /* sort */
                );
        if (c != null) {
            assertFalse("thumbnail entry exists for non-thumbnail file", c.moveToNext());
            c.close();
        }
        assertFalse("thumbnail remains after source file type change", new File(path).exists());

        // check source no longer exists as video
        c = mResolver.query(uri,
                null /* projection */, null /* where */, null /* where args */, null /* sort */);
        assertFalse("source entry should be gone", c.moveToNext());
        c.close();

        // check source still exists as file
        Uri fileUri = ContentUris.withAppendedId(
                Files.getContentUri("external"),
                Long.valueOf(uri.getLastPathSegment()));
        c = mResolver.query(fileUri,
                null /* projection */, null /* where */, null /* where args */, null /* sort */);
        assertTrue("source entry should be gone", c.moveToNext());
        String sourcePath = c.getString(c.getColumnIndex("_data"));
        c.close();

        // clean up
        mResolver.delete(uri, null /* where */, null /* where args */);
        new File(sourcePath).delete();
    }

    private Uri insertVideo() throws IOException {
        File file = new File(Environment.getExternalStorageDirectory(), "testVideo.3gp");
        // clean up any potential left over entries from a previous aborted run
        mResolver.delete(Media.EXTERNAL_CONTENT_URI,
                "_data=?", new String[] { file.getAbsolutePath() });
        file.delete();
        mFileHelper.copyToExternalStorage(R.raw.testthumbvideo, file);

        ContentValues values = new ContentValues();
        values.put(VideoColumns.DATA, file.getAbsolutePath());
        return mResolver.insert(Media.EXTERNAL_CONTENT_URI, values);
    }

    private int getThumbnailCount(Uri uri) {
        Cursor cursor = mResolver.query(uri, null, null, null, null);
        try {
            return cursor.getCount();
        } finally {
            cursor.close();
        }
    }
}
