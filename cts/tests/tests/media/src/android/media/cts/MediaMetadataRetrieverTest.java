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

package android.media.cts;

import android.media.cts.R;

import android.content.pm.PackageManager;
import android.content.res.AssetFileDescriptor;
import android.content.res.Resources;
import android.media.MediaDataSource;
import android.media.MediaExtractor;
import android.media.MediaFormat;
import android.media.MediaMetadataRetriever;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Color;
import android.graphics.Rect;
import android.platform.test.annotations.AppModeFull;
import android.support.test.filters.SmallTest;
import android.platform.test.annotations.RequiresDevice;
import android.test.AndroidTestCase;
import android.util.Log;

import com.android.compatibility.common.util.MediaUtils;

import static android.content.pm.PackageManager.FEATURE_WATCH;
import static android.media.MediaMetadataRetriever.OPTION_CLOSEST;
import static android.media.MediaMetadataRetriever.OPTION_CLOSEST_SYNC;
import static android.media.MediaMetadataRetriever.OPTION_NEXT_SYNC;
import static android.media.MediaMetadataRetriever.OPTION_PREVIOUS_SYNC;

import java.io.InputStream;
import java.io.IOException;
import java.util.ArrayList;
import java.util.List;
import java.util.function.Function;

@SmallTest
@RequiresDevice
@AppModeFull(reason = "No interaction with system server")
public class MediaMetadataRetrieverTest extends AndroidTestCase {
    private static final String TAG = "MediaMetadataRetrieverTest";
    private static final boolean SAVE_BITMAP_OUTPUT = false;

    protected Resources mResources;
    protected MediaMetadataRetriever mRetriever;
    private PackageManager mPackageManager;

    private static int BORDER_WIDTH = 16;
    private static Color COLOR_BLOCK =
            Color.valueOf(1.0f, 1.0f, 1.0f);
    private static Color[] COLOR_BARS = {
            Color.valueOf(0.0f, 0.0f, 0.0f),
            Color.valueOf(0.0f, 0.0f, 0.64f),
            Color.valueOf(0.0f, 0.64f, 0.0f),
            Color.valueOf(0.0f, 0.64f, 0.64f),
            Color.valueOf(0.64f, 0.0f, 0.0f),
            Color.valueOf(0.64f, 0.0f, 0.64f),
            Color.valueOf(0.64f, 0.64f, 0.0f),
    };

    @Override
    protected void setUp() throws Exception {
        super.setUp();
        mResources = getContext().getResources();
        mRetriever = new MediaMetadataRetriever();
        mPackageManager = getContext().getPackageManager();
    }

    @Override
    protected void tearDown() throws Exception {
        super.tearDown();
        mRetriever.release();
    }

    protected void setDataSourceFd(int resid) {
        try {
            AssetFileDescriptor afd = mResources.openRawResourceFd(resid);
            mRetriever.setDataSource(afd.getFileDescriptor(), afd.getStartOffset(), afd.getLength());
            afd.close();
        } catch (Exception e) {
            fail("Unable to open file");
        }
    }

    protected TestMediaDataSource setDataSourceCallback(int resid) {
        TestMediaDataSource ds = null;
        try {
            AssetFileDescriptor afd = mResources.openRawResourceFd(resid);
            ds = TestMediaDataSource.fromAssetFd(afd);
            mRetriever.setDataSource(ds);
        } catch (Exception e) {
            fail("Unable to open file");
        }
        return ds;
    }

    protected TestMediaDataSource getFaultyDataSource(int resid, boolean throwing) {
        TestMediaDataSource ds = null;
        try {
            AssetFileDescriptor afd = mResources.openRawResourceFd(resid);
            ds = TestMediaDataSource.fromAssetFd(afd);
            if (throwing) {
                ds.throwFromReadAt();
            } else {
                ds.returnFromReadAt(-2);
            }
        } catch (Exception e) {
            fail("Unable to open file");
        }
        return ds;
    }

    public void test3gppMetadata() {
        setDataSourceCallback(R.raw.testvideo);

        assertEquals("Title was other than expected",
                "Title", mRetriever.extractMetadata(MediaMetadataRetriever.METADATA_KEY_TITLE));

        assertEquals("Artist was other than expected",
                "UTF16LE エンディアン ",
                mRetriever.extractMetadata(MediaMetadataRetriever.METADATA_KEY_ARTIST));

        assertEquals("Album was other than expected",
                "Test album",
                mRetriever.extractMetadata(MediaMetadataRetriever.METADATA_KEY_ALBUM));

        assertEquals("Track number was other than expected",
                "10",
                mRetriever.extractMetadata(MediaMetadataRetriever.METADATA_KEY_CD_TRACK_NUMBER));

        assertEquals("Year was other than expected",
                "2013", mRetriever.extractMetadata(MediaMetadataRetriever.METADATA_KEY_YEAR));

        assertEquals("Date was other than expected",
                "19040101T000000.000Z",
                mRetriever.extractMetadata(MediaMetadataRetriever.METADATA_KEY_DATE));

        assertNull("Writer was unexpected present",
                mRetriever.extractMetadata(MediaMetadataRetriever.METADATA_KEY_WRITER));
    }

    public void testID3v2Metadata() {
        setDataSourceFd(R.raw.video_480x360_mp4_h264_500kbps_25fps_aac_stereo_128kbps_44100hz_id3v2);

        assertEquals("Title was other than expected",
                "Title", mRetriever.extractMetadata(MediaMetadataRetriever.METADATA_KEY_TITLE));

        assertEquals("Artist was other than expected",
                "UTF16LE エンディアン ",
                mRetriever.extractMetadata(MediaMetadataRetriever.METADATA_KEY_ARTIST));

        assertEquals("Album was other than expected",
                "Test album",
                mRetriever.extractMetadata(MediaMetadataRetriever.METADATA_KEY_ALBUM));

        assertEquals("Track number was other than expected",
                "10",
                mRetriever.extractMetadata(MediaMetadataRetriever.METADATA_KEY_CD_TRACK_NUMBER));

        assertEquals("Year was other than expected",
                "2013", mRetriever.extractMetadata(MediaMetadataRetriever.METADATA_KEY_YEAR));

        assertEquals("Date was other than expected",
                "19700101T000000.000Z",
                mRetriever.extractMetadata(MediaMetadataRetriever.METADATA_KEY_DATE));

        assertNull("Writer was unexpectedly present",
                mRetriever.extractMetadata(MediaMetadataRetriever.METADATA_KEY_WRITER));
    }

    public void testLargeAlbumArt() {
        setDataSourceFd(R.raw.largealbumart);

        assertNotNull("couldn't retrieve album art", mRetriever.getEmbeddedPicture());
    }

    public void testSetDataSourceNullPath() {
        try {
            mRetriever.setDataSource((String)null);
            fail("Expected IllegalArgumentException.");
        } catch (IllegalArgumentException ex) {
            // Expected, test passed.
        }
    }

    public void testNullMediaDataSourceIsRejected() {
        try {
            mRetriever.setDataSource((MediaDataSource)null);
            fail("Expected IllegalArgumentException.");
        } catch (IllegalArgumentException ex) {
            // Expected, test passed.
        }
    }

    public void testMediaDataSourceIsClosedOnRelease() throws Exception {
        TestMediaDataSource dataSource = setDataSourceCallback(R.raw.testvideo);
        mRetriever.extractMetadata(MediaMetadataRetriever.METADATA_KEY_TITLE);
        mRetriever.release();
        assertTrue(dataSource.isClosed());
    }

    public void testRetrieveFailsIfMediaDataSourceThrows() throws Exception {
        TestMediaDataSource ds = getFaultyDataSource(R.raw.testvideo, true /* throwing */);
        try {
            mRetriever.setDataSource(ds);
            fail("Failed to throw exceptions");
        } catch (RuntimeException e) {
            assertTrue(mRetriever.extractMetadata(MediaMetadataRetriever.METADATA_KEY_TITLE) == null);
        }
    }

    public void testRetrieveFailsIfMediaDataSourceReturnsAnError() throws Exception {
        TestMediaDataSource ds = getFaultyDataSource(R.raw.testvideo, false /* throwing */);
        try {
            mRetriever.setDataSource(ds);
            fail("Failed to throw exceptions");
        } catch (RuntimeException e) {
            assertTrue(mRetriever.extractMetadata(MediaMetadataRetriever.METADATA_KEY_TITLE) == null);
        }
    }

    private void testThumbnail(int resId) {
        if (!MediaUtils.hasCodecForResourceAndDomain(getContext(), resId, "video/")) {
            MediaUtils.skipTest("no video codecs for resource");
            return;
        }

        MediaMetadataRetriever retriever = new MediaMetadataRetriever();
        Resources resources = getContext().getResources();
        AssetFileDescriptor afd = resources.openRawResourceFd(resId);

        retriever.setDataSource(afd.getFileDescriptor(), afd.getStartOffset(), afd.getLength());

        try {
            afd.close();
        } catch (IOException e) {
            fail("Unable to open file");
        }

        assertNotNull(retriever.getFrameAtTime(-1 /* timeUs (any) */));
    }

    public void testThumbnailH264() {
        testThumbnail(R.raw.bbb_s4_1280x720_mp4_h264_mp31_8mbps_30fps_aac_he_mono_40kbps_44100hz);
    }

    public void testThumbnailH263() {
        testThumbnail(R.raw.video_176x144_3gp_h263_56kbps_12fps_aac_mono_24kbps_11025hz);
    }

    public void testThumbnailMPEG4() {
        testThumbnail(R.raw.video_1280x720_mp4_mpeg4_1000kbps_25fps_aac_stereo_128kbps_44100hz);
    }

    public void testThumbnailVP8() {
        testThumbnail(R.raw.bbb_s1_640x360_webm_vp8_2mbps_30fps_vorbis_5ch_320kbps_48000hz);
    }

    public void testThumbnailVP9() {
        testThumbnail(R.raw.bbb_s1_640x360_webm_vp9_0p21_1600kbps_30fps_vorbis_stereo_128kbps_48000hz);
    }

    public void testThumbnailHEVC() {
        testThumbnail(R.raw.bbb_s1_720x480_mp4_hevc_mp3_1600kbps_30fps_aac_he_6ch_240kbps_48000hz);
    }


    /**
     * The following tests verifies MediaMetadataRetriever.getFrameAtTime behavior.
     *
     * We use a simple stream with binary counter at the top to check which frame
     * is actually captured. The stream is 30fps with 600 frames in total. It has
     * I/P/B frames, with I interval of 30. Due to the encoding structure, pts starts
     * at 66666 (instead of 0), so we have I frames at 66666, 1066666, ..., etc..
     *
     * For each seek option, we check the following five cases:
     *     1) frame time falls right on a sync frame
     *     2) frame time is near the middle of two sync frames but closer to the previous one
     *     3) frame time is near the middle of two sync frames but closer to the next one
     *     4) frame time is shortly before a sync frame
     *     5) frame time is shortly after a sync frame
     */
    public void testGetFrameAtTimePreviousSync() {
        int[][] testCases = {
                { 2066666, 60 }, { 2500000, 60 }, { 2600000, 60 }, { 3000000, 60 }, { 3200000, 90}};
        testGetFrameAtTime(OPTION_PREVIOUS_SYNC, testCases);
    }

    public void testGetFrameAtTimeNextSync() {
        int[][] testCases = {
                { 2066666, 60 }, { 2500000, 90 }, { 2600000, 90 }, { 3000000, 90 }, { 3200000, 120}};
        testGetFrameAtTime(OPTION_NEXT_SYNC, testCases);
    }

    public void testGetFrameAtTimeClosestSync() {
        int[][] testCases = {
                { 2066666, 60 }, { 2500000, 60 }, { 2600000, 90 }, { 3000000, 90 }, { 3200000, 90}};
        testGetFrameAtTime(OPTION_CLOSEST_SYNC, testCases);
    }

    public void testGetFrameAtTimeClosest() {
        int[][] testCases = {
                { 2066666, 60 }, { 2500001, 73 }, { 2599999, 76 }, { 3016000, 88 }, { 3184000, 94}};
        testGetFrameAtTime(OPTION_CLOSEST, testCases);
    }

    private void testGetFrameAtTime(int option, int[][] testCases) {
        testGetFrameAt(testCases, (r) -> {
            List<Bitmap> bitmaps = new ArrayList<>();
            for (int i = 0; i < testCases.length; i++) {
                bitmaps.add(r.getFrameAtTime(testCases[i][0], option));
            }
            return bitmaps;
        });
    }

    public void testGetFrameAtIndex() {
        int[][] testCases = { { 60, 60 }, { 73, 73 }, { 76, 76 }, { 88, 88 }, { 94, 94} };

        testGetFrameAt(testCases, (r) -> {
            List<Bitmap> bitmaps = new ArrayList<>();
            for (int i = 0; i < testCases.length; i++) {
                bitmaps.add(r.getFrameAtIndex(testCases[i][0]));
            }
            return bitmaps;
        });

        MediaMetadataRetriever.BitmapParams params = new MediaMetadataRetriever.BitmapParams();
        params.setPreferredConfig(Bitmap.Config.RGB_565);
        assertEquals("Failed to set preferred config",
                Bitmap.Config.RGB_565, params.getPreferredConfig());

        testGetFrameAt(testCases, (r) -> {
            List<Bitmap> bitmaps = new ArrayList<>();
            for (int i = 0; i < testCases.length; i++) {
                Bitmap bitmap = r.getFrameAtIndex(testCases[i][0], params);
                assertEquals(Bitmap.Config.RGB_565, params.getActualConfig());
                bitmaps.add(bitmap);
            }
            return bitmaps;
        });
    }

    public void testGetFramesAtIndex() {
        int[][] testCases = { { 27, 27 }, { 28, 28 }, { 29, 29 }, { 30, 30 }, { 31, 31} };

        testGetFrameAt(testCases, (r) -> {
            return r.getFramesAtIndex(testCases[0][0], testCases.length);
        });

        MediaMetadataRetriever.BitmapParams params = new MediaMetadataRetriever.BitmapParams();
        params.setPreferredConfig(Bitmap.Config.RGB_565);
        assertEquals("Failed to set preferred config",
                Bitmap.Config.RGB_565, params.getPreferredConfig());

        testGetFrameAt(testCases, (r) -> {
            List<Bitmap> bitmaps = r.getFramesAtIndex(testCases[0][0], testCases.length, params);
            assertEquals(Bitmap.Config.RGB_565, params.getActualConfig());
            return bitmaps;
        });
    }

    private void testGetFrameAt(int[][] testCases,
            Function<MediaMetadataRetriever, List<Bitmap> > bitmapRetriever) {
        int resId = R.raw.binary_counter_320x240_30fps_600frames;
        if (!MediaUtils.hasCodecForResourceAndDomain(getContext(), resId, "video/")
            && mPackageManager.hasSystemFeature(PackageManager.FEATURE_WATCH)) {
            MediaUtils.skipTest("no video codecs for resource on watch");
            return;
        }

        MediaMetadataRetriever retriever = new MediaMetadataRetriever();
        Resources resources = getContext().getResources();
        AssetFileDescriptor afd = resources.openRawResourceFd(resId);

        retriever.setDataSource(afd.getFileDescriptor(), afd.getStartOffset(), afd.getLength());
        try {
            afd.close();
        } catch (IOException e) {
            fail("Unable to close file");
        }

        List<Bitmap> bitmaps = bitmapRetriever.apply(retriever);

        for (int i = 0; i < testCases.length; i++) {
            verifyVideoFrame(bitmaps.get(i), testCases[i]);
        }
        retriever.release();
    }

    private void verifyVideoFrame(Bitmap bitmap, int[] testCase) {
        try {
            assertTrue("Failed to get bitmap for " + testCase[0], bitmap != null);
            assertEquals("Counter value incorrect for " + testCase[0],
                    testCase[1], CodecUtils.readBinaryCounterFromBitmap(bitmap));

            if (SAVE_BITMAP_OUTPUT) {
                CodecUtils.saveBitmapToFile(bitmap, "test_" + testCase[0] + ".jpg");
            }
        } catch (Exception e) {
            fail("Exception getting bitmap: " + e);
        }
    }

    /**
     * The following tests verifies MediaMetadataRetriever.getScaledFrameAtTime behavior.
     */
    public void testGetScaledFrameAtTime() {
        int resId = R.raw.binary_counter_320x240_30fps_600frames;
        if (!MediaUtils.hasCodecForResourceAndDomain(getContext(), resId, "video/")
            && mPackageManager.hasSystemFeature(PackageManager.FEATURE_WATCH)) {
            MediaUtils.skipTest("no video codecs for resource on watch");
            return;
        }

        MediaMetadataRetriever retriever = new MediaMetadataRetriever();
        Resources resources = getContext().getResources();
        AssetFileDescriptor afd = resources.openRawResourceFd(resId);

        retriever.setDataSource(afd.getFileDescriptor(), afd.getStartOffset(), afd.getLength());
        try {
            afd.close();
        } catch (IOException e) {
            fail("Unable to close file");
        }

        try {
            Bitmap bitmap = retriever.getScaledFrameAtTime(
                    2066666 /*timeUs*/ , OPTION_CLOSEST, 0 /*width*/, 120 /*height*/);
            fail("Failed to receive exception");
        } catch (IllegalArgumentException e) {
            // Expect exception
        }

        try {
            Bitmap bitmap = retriever.getScaledFrameAtTime(
                    2066666 /*timeUs*/ , OPTION_CLOSEST, -1 /*width*/, 0 /*height*/);
            fail("Failed to receive exception");
        } catch (IllegalArgumentException e) {
            // Expect exception
        }

        try {
            Bitmap bitmap = retriever.getScaledFrameAtTime(
                    2066666 /*timeUs*/ , OPTION_CLOSEST, -1 /*width*/, 120 /*height*/);
            fail("Failed to receive exception");
        } catch (IllegalArgumentException e) {
            // Expect exception
        }

        try {
            Bitmap bitmap = retriever.getScaledFrameAtTime(
                2066666 /*timeUs */, OPTION_CLOSEST, 140 /*width*/, -1 /*height*/);
            fail("Failed to receive exception");
        } catch (IllegalArgumentException e) {
            // Expect exception
        }

        try {
            Bitmap bitmap = retriever.getScaledFrameAtTime(
                2066666 /*timeUs */, OPTION_CLOSEST, -1 /*width*/, -1 /*height*/);
            fail("Failed to receive exception");
        } catch (IllegalArgumentException e) {
            // Expect exception
        }

        // Test desided size of 160 x 120. Return should be 160 x 120
        try {
            Bitmap bitmap = retriever.getScaledFrameAtTime(
                2066666 /*timeUs */, OPTION_CLOSEST, 160 /*width*/, 120 /*height*/);
            if (bitmap == null) {
                fail("Failed to get scaled bitmap");
            }
            if (SAVE_BITMAP_OUTPUT) {
                CodecUtils.saveBitmapToFile(bitmap, "test_160x120" + ".jpg");
            }

            if (bitmap.getWidth() != 160 /* width */) {
                fail("Bitmap width is " + bitmap.getWidth() + "Expect: 160");
            }
            if (bitmap.getHeight() != 120 /* height */) {
                fail("Bitmap height is " + bitmap.getHeight() + "Expect: 120");
            }

        } catch (Exception e) {
            fail("Exception getting bitmap: " + e);
        }

        // Test scaled up bitmap to 640 x 480. Return should be 640 x 480
        try {
            Bitmap bitmap = retriever.getScaledFrameAtTime(
                2066666 /*timeUs */, OPTION_CLOSEST, 640 /*width*/, 480 /*height*/);
            if (bitmap == null) {
                fail("Failed to get scaled bitmap");
            }
            if (SAVE_BITMAP_OUTPUT) {
                CodecUtils.saveBitmapToFile(bitmap, "test_640x480" + ".jpg");
            }

            if (bitmap.getWidth() != 640 /* width */) {
                fail("Bitmap width is " + bitmap.getWidth() + "Expect: 640");
            }
            if (bitmap.getHeight() != 480 /* height */) {
                fail("Bitmap height is " + bitmap.getHeight() + "Expect: 480");
            }

        } catch (Exception e) {
            fail("Exception getting bitmap: " + e);
        }

        // Test scaled up bitmap to 320 x 120. Return should be 160 x 120
        try {
            Bitmap bitmap = retriever.getScaledFrameAtTime(
                2066666 /*timeUs */, OPTION_CLOSEST, 320 /*width*/, 120 /*height*/);
            if (bitmap == null) {
                fail("Failed to get scaled bitmap");
            }
            if (SAVE_BITMAP_OUTPUT) {
                CodecUtils.saveBitmapToFile(bitmap, "test_320x120" + ".jpg");
            }

            if (bitmap.getWidth() != 160 /* width */) {
                fail("Bitmap width is " + bitmap.getWidth() + "Expect: 160");
            }
            if (bitmap.getHeight() != 120 /* height */) {
                fail("Bitmap height is " + bitmap.getHeight() + "Expect: 120");
            }

        } catch (Exception e) {
            fail("Exception getting bitmap: " + e);
        }

        // Test scaled up bitmap to 160 x 240. Return should be 160 x 120
        try {
            Bitmap bitmap = retriever.getScaledFrameAtTime(
                2066666 /*timeUs */, OPTION_CLOSEST, 160 /*width*/, 240 /*height*/);
            if (bitmap == null) {
                fail("Failed to get scaled bitmap");
            }
            if (SAVE_BITMAP_OUTPUT) {
                CodecUtils.saveBitmapToFile(bitmap, "test_160x240" + ".jpg");
            }

            if (bitmap.getWidth() != 160 /* width */) {
                fail("Bitmap width is " + bitmap.getWidth() + "Expect: 160");
            }
            if (bitmap.getHeight() != 120 /* height */) {
                fail("Bitmap height is " + bitmap.getHeight() + "Expect: 120");
            }

        } catch (Exception e) {
            fail("Exception getting bitmap: " + e);
        }

        // Test scaled the video with aspect ratio
        resId = R.raw.binary_counter_320x240_720x240_30fps_600frames;
        afd = resources.openRawResourceFd(resId);

        retriever.setDataSource(afd.getFileDescriptor(), afd.getStartOffset(), afd.getLength());
        try {
            afd.close();
        } catch (IOException e) {
            fail("Unable to close file");
        }
        try {
            Bitmap bitmap = retriever.getScaledFrameAtTime(
                2066666 /*timeUs */, OPTION_CLOSEST, 330 /*width*/, 240 /*height*/);
            if (bitmap == null) {
                fail("Failed to get scaled bitmap");
            }
            if (SAVE_BITMAP_OUTPUT) {
                CodecUtils.saveBitmapToFile(bitmap, "test_330x240" + ".jpg");
            }

            if (bitmap.getWidth() != 330 /* width */) {
                fail("Bitmap width is " + bitmap.getWidth() + "Expect: 330");
            }
            if (bitmap.getHeight() != 110 /* height */) {
                fail("Bitmap height is " + bitmap.getHeight() + "Expect: 110");
            }

        } catch (Exception e) {
            fail("Exception getting bitmap: " + e);
        }
    }

    public void testGetImageAtIndex() throws Exception {
        if (!MediaUtils.hasDecoder(MediaFormat.MIMETYPE_VIDEO_HEVC)) {
            MediaUtils.skipTest("no video decoders for resource");
            return;
        }

        testGetImage(R.raw.heifwriter_input, 1920, 1080, 0 /*rotation*/,
                4 /*imageCount*/, 3 /*primary*/, true /*useGrid*/, true /*checkColor*/);
    }

    /**
     * Determines if two color values are approximately equal.
     */
    private static boolean approxEquals(Color expected, Color actual) {
        final float MAX_DELTA = 0.025f;
        return (Math.abs(expected.red() - actual.red()) <= MAX_DELTA)
            && (Math.abs(expected.green() - actual.green()) <= MAX_DELTA)
            && (Math.abs(expected.blue() - actual.blue()) <= MAX_DELTA);
    }

    private static Rect getColorBarRect(int index, int width, int height) {
        int barWidth = (width - BORDER_WIDTH * 2) / COLOR_BARS.length;
        return new Rect(BORDER_WIDTH + barWidth * index, BORDER_WIDTH,
                BORDER_WIDTH + barWidth * (index + 1), height - BORDER_WIDTH);
    }

    private static Rect getColorBlockRect(int index, int width, int height) {
        int blockCenterX = (width / 5) * (index % 4 + 1);
        return new Rect(blockCenterX - width / 10, height / 6,
                        blockCenterX + width / 10, height / 3);
    }

    private void testGetImage(
            int resId, int width, int height, int rotation,
            int imageCount, int primary, boolean useGrid, boolean checkColor)
                    throws Exception {
        MediaMetadataRetriever retriever = null;
        MediaExtractor extractor = null;
        AssetFileDescriptor afd = null;
        InputStream inputStream = null;

        try {
            retriever = new MediaMetadataRetriever();

            Resources resources = getContext().getResources();
            afd = resources.openRawResourceFd(resId);

            retriever.setDataSource(afd.getFileDescriptor(), afd.getStartOffset(), afd.getLength());

            // Verify image related meta keys.
            String hasImage = retriever.extractMetadata(
                    MediaMetadataRetriever.METADATA_KEY_HAS_IMAGE);
            assertTrue("No images found in resId " + resId, "yes".equals(hasImage));
            assertEquals("Wrong width", width,
                    Integer.parseInt(retriever.extractMetadata(
                            MediaMetadataRetriever.METADATA_KEY_IMAGE_WIDTH)));
            assertEquals("Wrong height", height,
                    Integer.parseInt(retriever.extractMetadata(
                            MediaMetadataRetriever.METADATA_KEY_IMAGE_HEIGHT)));
            assertEquals("Wrong rotation", rotation,
                    Integer.parseInt(retriever.extractMetadata(
                            MediaMetadataRetriever.METADATA_KEY_IMAGE_ROTATION)));
            assertEquals("Wrong image count", imageCount,
                    Integer.parseInt(retriever.extractMetadata(
                            MediaMetadataRetriever.METADATA_KEY_IMAGE_COUNT)));
            assertEquals("Wrong primary index", primary,
                    Integer.parseInt(retriever.extractMetadata(
                            MediaMetadataRetriever.METADATA_KEY_IMAGE_PRIMARY)));

            if (checkColor) {
                Bitmap bitmap = null;
                // For each image in the image collection, check the 7 color bars' color.
                // Also check the position of the color block, which should move left-to-right
                // with the index.
                for (int imageIndex = 0; imageIndex < imageCount; imageIndex++) {
                    bitmap = retriever.getImageAtIndex(imageIndex);

                    for (int barIndex = 0; barIndex < COLOR_BARS.length; barIndex++) {
                        Rect r = getColorBarRect(barIndex, width, height);
                        assertTrue("Color bar " + barIndex +
                                " for image " + imageIndex + " doesn't match",
                                approxEquals(COLOR_BARS[barIndex], Color.valueOf(
                                        bitmap.getPixel(r.centerX(), r.centerY()))));
                    }

                    Rect r = getColorBlockRect(imageIndex, width, height);
                    assertTrue("Color block for image " + imageIndex + " doesn't match",
                            approxEquals(COLOR_BLOCK, Color.valueOf(
                                    bitmap.getPixel(r.centerX(), height - r.centerY()))));
                    bitmap.recycle();
                }

                // Check the color block position on the primary image.
                Rect r = getColorBlockRect(primary, width, height);
                bitmap = retriever.getPrimaryImage();
                assertTrue("Color block for primary image doesn't match",
                        approxEquals(COLOR_BLOCK, Color.valueOf(
                                bitmap.getPixel(r.centerX(), height - r.centerY()))));
                bitmap.recycle();

                // Check the color block position on the bitmap decoded by BitmapFactory.
                // This should match the primary image as well.
                inputStream = getContext().getResources().openRawResource(resId);
                bitmap = BitmapFactory.decodeStream(inputStream);
                assertTrue("Color block for bitmap decoding doesn't match",
                        approxEquals(COLOR_BLOCK, Color.valueOf(
                                bitmap.getPixel(r.centerX(), height - r.centerY()))));
                bitmap.recycle();
            }

            // Check the grid configuration related keys.
            if (useGrid) {
                extractor = new MediaExtractor();
                extractor.setDataSource(
                        afd.getFileDescriptor(), afd.getStartOffset(), afd.getLength());
                MediaFormat format = extractor.getTrackFormat(0);
                int tileWidth = format.getInteger(MediaFormat.KEY_TILE_WIDTH);
                int tileHeight = format.getInteger(MediaFormat.KEY_TILE_HEIGHT);
                int gridRows = format.getInteger(MediaFormat.KEY_GRID_ROWS);
                int gridCols = format.getInteger(MediaFormat.KEY_GRID_COLUMNS);
                assertTrue("Wrong tile width or grid cols",
                        ((width + tileWidth - 1) / tileWidth) == gridCols);
                assertTrue("Wrong tile height or grid rows",
                        ((height + tileHeight - 1) / tileHeight) == gridRows);
            }
        } catch (IOException e) {
            fail("Unable to open file");
        } finally {
            if (retriever != null) {
                retriever.release();
            }
            if (extractor != null) {
                extractor.release();
            }
            if (afd != null) {
                afd.close();
            }
            if (inputStream != null) {
                inputStream.close();
            }
        }
    }
}
