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

package android.provider.cts;

import android.provider.cts.R;

import android.content.ContentResolver;
import android.content.ContentUris;
import android.content.ContentValues;
import android.content.Context;
import android.database.Cursor;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.net.Uri;
import android.os.Environment;
import android.provider.MediaStore;
import android.provider.MediaStore.Images.Media;
import android.provider.MediaStore.Images.Thumbnails;
import android.provider.MediaStore.MediaColumns;
import android.test.InstrumentationTestCase;

import com.android.compatibility.common.util.FileCopyHelper;

import java.io.File;
import java.util.ArrayList;
import android.util.DisplayMetrics;

public class MediaStore_Images_ThumbnailsTest extends InstrumentationTestCase {
    private ArrayList<Uri> mRowsAdded;

    private Context mContext;

    private ContentResolver mContentResolver;

    private FileCopyHelper mHelper;

    @Override
    protected void tearDown() throws Exception {
        for (Uri row : mRowsAdded) {
            try {
                mContentResolver.delete(row, null, null);
            } catch (UnsupportedOperationException e) {
                // There is no way to delete rows from table "thumbnails" of internals database.
                // ignores the exception and make the loop goes on
            }
        }

        mHelper.clear();
        super.tearDown();
    }

    @Override
    protected void setUp() throws Exception {
        super.setUp();

        mContext = getInstrumentation().getTargetContext();
        mContentResolver = mContext.getContentResolver();

        mHelper = new FileCopyHelper(mContext);
        mRowsAdded = new ArrayList<Uri>();
    }

    public void testQueryInternalThumbnails() throws Exception {
        Cursor c = Thumbnails.queryMiniThumbnails(mContentResolver,
                Thumbnails.INTERNAL_CONTENT_URI, Thumbnails.MICRO_KIND, null);
        int previousMicroKindCount = c.getCount();
        c.close();

        // add a thumbnail
        String path = mHelper.copy(R.raw.scenery, "testThumbnails.jpg");
        ContentValues values = new ContentValues();
        values.put(Thumbnails.KIND, Thumbnails.MINI_KIND);
        values.put(Thumbnails.DATA, path);
        Uri uri = mContentResolver.insert(Thumbnails.INTERNAL_CONTENT_URI, values);
        if (uri != null) {
            mRowsAdded.add(uri);
        }

        // query with the uri of the thumbnail and the kind
        c = Thumbnails.queryMiniThumbnails(mContentResolver, uri, Thumbnails.MINI_KIND, null);
        c.moveToFirst();
        assertEquals(1, c.getCount());
        assertEquals(Thumbnails.MINI_KIND, c.getInt(c.getColumnIndex(Thumbnails.KIND)));
        assertEquals(path, c.getString(c.getColumnIndex(Thumbnails.DATA)));

        // query all thumbnails with other kind
        c = Thumbnails.queryMiniThumbnails(mContentResolver, Thumbnails.INTERNAL_CONTENT_URI,
                Thumbnails.MICRO_KIND, null);
        assertEquals(previousMicroKindCount, c.getCount());
        c.close();

        // query without kind
        c = Thumbnails.query(mContentResolver, uri, null);
        assertEquals(1, c.getCount());
        c.moveToFirst();
        assertEquals(Thumbnails.MINI_KIND, c.getInt(c.getColumnIndex(Thumbnails.KIND)));
        assertEquals(path, c.getString(c.getColumnIndex(Thumbnails.DATA)));
        c.close();
    }

    public void testQueryExternalMiniThumbnails() {
        // insert the image by bitmap
        BitmapFactory.Options opts = new BitmapFactory.Options();
        opts.inTargetDensity = DisplayMetrics.DENSITY_XHIGH;
        Bitmap src = BitmapFactory.decodeResource(mContext.getResources(), R.raw.scenery,opts);
        String stringUrl = null;
        try{
            stringUrl = Media.insertImage(mContentResolver, src, null, null);
        } catch (UnsupportedOperationException e) {
            // the tests will be aborted because the image will be put in sdcard
            fail("There is no sdcard attached! " + e.getMessage());
        }
        assertNotNull(stringUrl);
        Uri stringUri = Uri.parse(stringUrl);
        mRowsAdded.add(stringUri);

        // get the original image id and path
        Cursor c = mContentResolver.query(stringUri,
                new String[]{ Media._ID, Media.DATA }, null, null, null);
        c.moveToFirst();
        long imageId = c.getLong(c.getColumnIndex(Media._ID));
        String imagePath = c.getString(c.getColumnIndex(Media.DATA));
        c.close();

        assertTrue("image file does not exist", new File(imagePath).exists());

        String[] sizeProjection = new String[] { Thumbnails.WIDTH, Thumbnails.HEIGHT };
        c = Thumbnails.queryMiniThumbnail(mContentResolver, imageId, Thumbnails.MINI_KIND,
                sizeProjection);
        assertEquals(1, c.getCount());
        assertTrue(c.moveToFirst());
        assertTrue(c.getLong(c.getColumnIndex(Thumbnails.WIDTH)) >= Math.min(src.getWidth(), 240));
        assertTrue(c.getLong(c.getColumnIndex(Thumbnails.HEIGHT)) >= Math.min(src.getHeight(), 240));
        c.close();
        c = Thumbnails.queryMiniThumbnail(mContentResolver, imageId, Thumbnails.MICRO_KIND,
                sizeProjection);
        assertEquals(1, c.getCount());
        assertTrue(c.moveToFirst());
        assertEquals(50, c.getLong(c.getColumnIndex(Thumbnails.WIDTH)));
        assertEquals(50, c.getLong(c.getColumnIndex(Thumbnails.HEIGHT)));
        c.close();

        c = Thumbnails.queryMiniThumbnail(mContentResolver, imageId, Thumbnails.MINI_KIND,
                new String[] { Thumbnails._ID, Thumbnails.DATA, Thumbnails.IMAGE_ID});

        c.moveToNext();
        long img = c.getLong(c.getColumnIndex(Thumbnails.IMAGE_ID));
        assertEquals(imageId, img);
        String thumbPath = c.getString(c.getColumnIndex(Thumbnails.DATA));
        assertTrue("thumbnail file does not exist", new File(thumbPath).exists());

        // deleting the image from the database also deletes the image file, and the
        // corresponding entry in the thumbnail table, which in turn triggers deletion
        // of the thumbnail file on disk
        mContentResolver.delete(stringUri, null, null);
        mRowsAdded.remove(stringUri);

        assertFalse("image file should no longer exist", new File(imagePath).exists());

        Cursor c2 = Thumbnails.queryMiniThumbnail(mContentResolver, imageId, Thumbnails.MINI_KIND,
                new String[] { Thumbnails._ID, Thumbnails.DATA, Thumbnails.IMAGE_ID});
        assertEquals(0, c2.getCount());
        c2.close();

        assertFalse("thumbnail file should no longer exist", new File(thumbPath).exists());
        c.close();

        // insert image, then delete it via the files table
        stringUrl = Media.insertImage(mContentResolver, src, null, null);
        c = mContentResolver.query(Uri.parse(stringUrl),
                new String[]{ Media._ID, Media.DATA}, null, null, null);
        c.moveToFirst();
        imageId = c.getLong(c.getColumnIndex(Media._ID));
        imagePath = c.getString(c.getColumnIndex(Media.DATA));
        c.close();
        assertTrue("image file does not exist", new File(imagePath).exists());
        Uri fileuri = MediaStore.Files.getContentUri("external", imageId);
        mContentResolver.delete(fileuri, null, null);
        assertFalse("image file should no longer exist", new File(imagePath).exists());


        // insert image, then delete its thumbnail
        stringUrl = Media.insertImage(mContentResolver, src, null, null);
        c = mContentResolver.query(Uri.parse(stringUrl),
                new String[]{ Media._ID, Media.DATA}, null, null, null);
        c.moveToFirst();
        imageId = c.getLong(c.getColumnIndex(Media._ID));
        imagePath = c.getString(c.getColumnIndex(Media.DATA));
        c.close();
        c2 = Thumbnails.queryMiniThumbnail(mContentResolver, imageId, Thumbnails.MINI_KIND,
                new String[] { Thumbnails._ID, Thumbnails.DATA, Thumbnails.IMAGE_ID});
        c2.moveToFirst();
        thumbPath = c2.getString(c2.getColumnIndex(Thumbnails.DATA));
        assertTrue("thumbnail file does not exist", new File(thumbPath).exists());

        Uri imguri = Uri.parse(stringUrl);
        long imgid = ContentUris.parseId(imguri);
        assertEquals(imgid, imageId);
        mRowsAdded.add(imguri);
        mContentResolver.delete(MediaStore.Images.Thumbnails.EXTERNAL_CONTENT_URI,
                MediaStore.Images.Thumbnails.IMAGE_ID + "=?", new String[]{ "" + imgid});
        assertFalse("thumbnail file should no longer exist", new File(thumbPath).exists());
        assertTrue("image file should still exist", new File(imagePath).exists());
    }

    public void testGetContentUri() {
        Cursor c = null;
        assertNotNull(c = mContentResolver.query(Thumbnails.getContentUri("internal"), null, null,
                null, null));
        c.close();
        assertNotNull(c = mContentResolver.query(Thumbnails.getContentUri("external"), null, null,
                null, null));
        c.close();

        // can not accept any other volume names
        String volume = "fakeVolume";
        assertNull(mContentResolver.query(Thumbnails.getContentUri(volume), null, null, null,
                null));
    }

    public void testStoreImagesMediaExternal() {
        final String externalImgPath = Environment.getExternalStorageDirectory() +
                "/testimage.jpg";
        final String externalImgPath2 = Environment.getExternalStorageDirectory() +
                "/testimage1.jpg";
        ContentValues values = new ContentValues();
        values.put(Thumbnails.KIND, Thumbnails.FULL_SCREEN_KIND);
        values.put(Thumbnails.IMAGE_ID, 0);
        values.put(Thumbnails.HEIGHT, 480);
        values.put(Thumbnails.WIDTH, 320);
        values.put(Thumbnails.DATA, externalImgPath);

        // insert
        Uri uri = mContentResolver.insert(Thumbnails.EXTERNAL_CONTENT_URI, values);
        assertNotNull(uri);

        // query
        Cursor c = mContentResolver.query(uri, null, null, null, null);
        assertEquals(1, c.getCount());
        c.moveToFirst();
        long id = c.getLong(c.getColumnIndex(Thumbnails._ID));
        assertTrue(id > 0);
        assertEquals(Thumbnails.FULL_SCREEN_KIND, c.getInt(c.getColumnIndex(Thumbnails.KIND)));
        assertEquals(0, c.getLong(c.getColumnIndex(Thumbnails.IMAGE_ID)));
        assertEquals(480, c.getInt(c.getColumnIndex(Thumbnails.HEIGHT)));
        assertEquals(320, c.getInt(c.getColumnIndex(Thumbnails.WIDTH)));
        assertEquals(externalImgPath, c.getString(c.getColumnIndex(Thumbnails.DATA)));
        c.close();

        // update
        values.clear();
        values.put(Thumbnails.KIND, Thumbnails.MICRO_KIND);
        values.put(Thumbnails.IMAGE_ID, 1);
        values.put(Thumbnails.HEIGHT, 50);
        values.put(Thumbnails.WIDTH, 50);
        values.put(Thumbnails.DATA, externalImgPath2);
        assertEquals(1, mContentResolver.update(uri, values, null, null));

        // delete
        assertEquals(1, mContentResolver.delete(uri, null, null));
    }

    public void testThumbnailGenerationAndCleanup() throws Exception {
        // insert an image
        Bitmap src = BitmapFactory.decodeResource(mContext.getResources(), R.raw.scenery);
        Uri uri = Uri.parse(Media.insertImage(mContentResolver, src, "test", "test description"));

        // query its thumbnail
        Cursor c = mContentResolver.query(
                Thumbnails.EXTERNAL_CONTENT_URI,
                new String [] {Thumbnails.DATA},
                "image_id=?",
                new String[] {uri.getLastPathSegment()},
                null /* sort */
                );
        assertTrue("couldn't find thumbnail", c.moveToNext());
        String path = c.getString(0);
        c.close();
        assertTrue("thumbnail does not exist", new File(path).exists());

        // delete the source image and check that the thumbnail is gone too
        mContentResolver.delete(uri, null /* where clause */, null /* where args */);
        assertFalse("thumbnail still exists after source file delete", new File(path).exists());

        // insert again
        uri = Uri.parse(Media.insertImage(mContentResolver, src, "test", "test description"));

        // query its thumbnail again
        c = mContentResolver.query(
                Thumbnails.EXTERNAL_CONTENT_URI,
                new String [] {Thumbnails.DATA},
                "image_id=?",
                new String[] {uri.getLastPathSegment()},
                null /* sortOrder */
                );
        assertTrue("couldn't find thumbnail", c.moveToNext());
        path = c.getString(0);
        c.close();
        assertTrue("thumbnail does not exist", new File(path).exists());

        // update the media type
        ContentValues values = new ContentValues();
        values.put("media_type", 0);
        assertEquals("unexpected number of updated rows",
                1, mContentResolver.update(uri, values, null /* where */, null /* where args */));

        // image was marked as regular file in the database, which should have deleted its thumbnail

        // query its thumbnail again
        c = mContentResolver.query(
                Thumbnails.EXTERNAL_CONTENT_URI,
                new String [] {Thumbnails.DATA},
                "image_id=?",
                new String[] {uri.getLastPathSegment()},
                null /* sort */
                );
        if (c != null) {
            assertFalse("thumbnail entry exists for non-thumbnail file", c.moveToNext());
            c.close();
        }
        assertFalse("thumbnail remains after source file type change", new File(path).exists());

        // check source no longer exists as image
        c = mContentResolver.query(uri,
                null /* projection */, null /* where */, null /* where args */, null /* sort */);
        assertFalse("source entry should be gone", c.moveToNext());
        c.close();

        // check source still exists as file
        Uri fileUri = ContentUris.withAppendedId(
                MediaStore.Files.getContentUri("external"),
                Long.valueOf(uri.getLastPathSegment()));
        c = mContentResolver.query(fileUri,
                null /* projection */, null /* where */, null /* where args */, null /* sort */);
        assertTrue("source entry is gone", c.moveToNext());
        String sourcePath = c.getString(c.getColumnIndex("_data"));
        c.close();

        // clean up
        mContentResolver.delete(uri, null /* where */, null /* where args */);
        new File(sourcePath).delete();
    }

    public void testStoreImagesMediaInternal() {
        // can not insert any data, so other operations can not be tested
        try {
            mContentResolver.insert(Thumbnails.INTERNAL_CONTENT_URI, new ContentValues());
            fail("Should throw UnsupportedOperationException when inserting into internal "
                    + "database");
        } catch (UnsupportedOperationException e) {
            // expected
        }
    }

    public void testThumbnailOrderedQuery() throws Exception {
        Bitmap src = BitmapFactory.decodeResource(mContext.getResources(), R.raw.scenery);
        Uri url[] = new Uri[3];
        try{
            for (int i = 0; i < url.length; i++) {
                url[i] = Uri.parse(Media.insertImage(mContentResolver, src, null, null));
                long origId = Long.parseLong(url[i].getLastPathSegment());
                Bitmap foo = MediaStore.Images.Thumbnails.getThumbnail(mContentResolver,
                        origId, Thumbnails.MICRO_KIND, null);
                assertNotNull(foo);
            }

            // remove one of the images, its thumbnail, and the thumbnail cache
            Cursor c = mContentResolver.query(url[1], null, null, null, null);
            assertEquals(1, c.getCount());
            c.moveToFirst();
            String path = c.getString(c.getColumnIndex(MediaColumns.DATA));
            long id = c.getLong(c.getColumnIndex(MediaColumns._ID));
            c.close();
            assertTrue(new File(path).delete());

            long removedId = Long.parseLong(url[1].getLastPathSegment());
            long remainingId1 = Long.parseLong(url[0].getLastPathSegment());
            long remainingId2 = Long.parseLong(url[2].getLastPathSegment());
            c = mContentResolver.query(
                    MediaStore.Images.Thumbnails.EXTERNAL_CONTENT_URI, null,
                    Thumbnails.IMAGE_ID + "=?", new String[] { Long.toString(removedId) }, null);
            assertTrue(c.getCount() > 0);  // one or more thumbnail kinds
            while (c.moveToNext()) {
                path = c.getString(c.getColumnIndex(MediaColumns.DATA));
                assertTrue(new File(path).delete());
            }
            c.close();

            File thumbdir = new File(path).getParentFile();
            File[] list = thumbdir.listFiles();
            for (int i = 0; i < list.length; i++) {
                if (list[i].getName().startsWith(".thumbdata")) {
                    assertTrue(list[i].delete());
                }
            }

            // check if a thumbnail is still being returned for the image that was removed
            Bitmap foo = MediaStore.Images.Thumbnails.getThumbnail(mContentResolver,
                    removedId, Thumbnails.MICRO_KIND, null);
            assertNull(foo);

            for (String order: new String[] { " ASC", " DESC" }) {
                c = mContentResolver.query(
                        MediaStore.Images.Media.EXTERNAL_CONTENT_URI, null, null, null,
                        MediaColumns._ID + order);
                while (c.moveToNext()) {
                    id = c.getLong(c.getColumnIndex(MediaColumns._ID));
                    foo = MediaStore.Images.Thumbnails.getThumbnail(
                            mContentResolver, id,
                            MediaStore.Images.Thumbnails.MICRO_KIND, null);
                    if (id == removedId) {
                        assertNull("unexpected bitmap with" + order + " ordering", foo);
                    } else if (id == remainingId1 || id == remainingId2) {
                        assertNotNull("missing bitmap with" + order + " ordering", foo);
                    }
                }
                c.close();
            }
        } catch (UnsupportedOperationException e) {
            // the tests will be aborted because the image will be put in sdcard
            fail("There is no sdcard attached! " + e.getMessage());
        }
    }
}
