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
package android.graphics.cts;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;

import android.content.res.Resources;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.ImageFormat;
import android.graphics.Rect;
import android.graphics.YuvImage;
import android.support.test.InstrumentationRegistry;
import android.support.test.filters.SmallTest;
import android.support.test.runner.AndroidJUnit4;
import android.util.Log;

import org.junit.Test;
import org.junit.runner.RunWith;

import java.io.ByteArrayOutputStream;
import java.util.Arrays;

@SmallTest
@RunWith(AndroidJUnit4.class)
public class YuvImageTest {
    // Coefficients are taken from jcolor.c in libjpeg.
    private static final int CSHIFT = 16;
    private static final int CYR = 19595;
    private static final int CYG = 38470;
    private static final int CYB = 7471;
    private static final int CUR = -11059;
    private static final int CUG = -21709;
    private static final int CUB = 32768;
    private static final int CVR = 32768;
    private static final int CVG = -27439;
    private static final int CVB = -5329;

    private static final String TAG = "YuvImageTest";

    private static final int[] FORMATS = { ImageFormat.NV21, ImageFormat.YUY2 };

    private static final int WIDTH = 256;
    private static final int HEIGHT = 128;

    private static final int[] RECT_WIDTHS = { 128, 124, 123 };
    private static final int[] RECT_HEIGHTS = { 64, 60, 59 };

    // Various rectangles:
    // mRects[0] : a normal one.
    // mRects[1] : same size to that of mRects[1], but its left-top point is shifted
    // mRects[2] : sides are not multiples of 16
    // mRects[3] : the left-top point is at an odd position
    private static final Rect[] RECTS = { new Rect(0, 0, 0 + RECT_WIDTHS[0],  0 + RECT_HEIGHTS[0]),
            new Rect(10, 10, 10 + RECT_WIDTHS[0], 10 + RECT_HEIGHTS[0]),
            new Rect(0, 0, 0 + RECT_WIDTHS[1], 0 + RECT_HEIGHTS[1]),
            new Rect(11, 11, 11 + RECT_WIDTHS[1], 11 + RECT_HEIGHTS[1]) };

    // Two rectangles of same size but at different positions
    private static final Rect[] RECTS_SHIFTED = { RECTS[0], RECTS[1] };

    // A rect whose side lengths are odd.
    private static final Rect RECT_ODD_SIDES = new Rect(10, 10, 10 + RECT_WIDTHS[2],
            10 + RECT_HEIGHTS[2]);

    private static final int[] PADDINGS = { 0, 32 };

    // There are three color components and
    // each should be within a square difference of 15 * 15.
    private static final int MSE_MARGIN = 3 * (15 * 15);

    private Bitmap[] mTestBitmaps = new Bitmap[1];

    @Test
    public void testYuvImage() {
        int width = 100;
        int height = 100;
        byte[] yuv = new byte[width * height * 2];
        YuvImage image;

        // normal case: test that the required formats are all supported
        for (int i = 0; i < FORMATS.length; ++i) {
            try {
                new YuvImage(yuv, FORMATS[i], width, height, null);
            } catch (Exception e) {
                Log.e(TAG, "unexpected exception", e);
                fail("unexpected exception");
            }
        }

        // normal case: test that default strides are returned correctly
        for (int i = 0; i < FORMATS.length; ++i) {
            int[] expected = null;
            int[] actual = null;
            if (FORMATS[i] == ImageFormat.NV21) {
                expected = new int[]{width, width};
            } else if (FORMATS[i] == ImageFormat.YUY2) {
                expected = new int[]{width * 2};
            }

            try {
                image = new YuvImage(yuv, FORMATS[i], width, height, null);
                actual = image.getStrides();
                assertTrue("default strides not calculated correctly",
                        Arrays.equals(expected, actual));
            } catch (Exception e) {
                Log.e(TAG, "unexpected exception", e);
                fail("unexpected exception");
            }
        }
    }

    @Test(expected=IllegalArgumentException.class)
    public void testYuvImageNegativeWidth(){
        new YuvImage(new byte[100 * 100 * 2], FORMATS[0], -1, 100, null);
    }

    @Test(expected=IllegalArgumentException.class)
    public void testYuvImageNegativeHeight(){
        new YuvImage(new byte[100 * 100 * 2], FORMATS[0], 100, -1, null);
    }

    @Test(expected=IllegalArgumentException.class)
    public void testYuvImageNullArray(){
        new YuvImage(null, FORMATS[0], 100, 100, null);
   }

    @Test
    public void testCompressYuvToJpeg() {
        generateTestBitmaps(WIDTH, HEIGHT);

        // test if handling compression parameters correctly
        verifyParameters();

        // test various cases by varing
        // <ImageFormat, Bitmap, HasPaddings, Rect>
        for (int i = 0; i < FORMATS.length; ++i) {
            for (int j = 0; j < mTestBitmaps.length; ++j) {
                for (int k = 0; k < PADDINGS.length; ++k) {
                    YuvImage image = generateYuvImage(FORMATS[i],
                        mTestBitmaps[j], PADDINGS[k]);
                    for (int l = 0; l < RECTS.length; ++l) {

                        // test compressing the same rect in
                        // mTestBitmaps[j] and image.
                        compressRects(mTestBitmaps[j], image,
                                RECTS[l], RECTS[l]);
                    }

                    // test compressing different rects in
                    // mTestBitmap[j] and image.
                    compressRects(mTestBitmaps[j], image, RECTS_SHIFTED[0],
                            RECTS_SHIFTED[1]);

                    // test compressing a rect whose side lengths are odd.
                    compressOddRect(mTestBitmaps[j], image, RECT_ODD_SIDES);
                }
            }
        }

    }

    @Test
    public void testGetHeight() {
        generateTestBitmaps(WIDTH, HEIGHT);
        YuvImage image = generateYuvImage(ImageFormat.YUY2, mTestBitmaps[0], 0);
        assertEquals(mTestBitmaps[0].getHeight(), image.getHeight());
        assertEquals(mTestBitmaps[0].getWidth(), image.getWidth());
    }

    @Test
    public void testGetYuvData() {
        generateTestBitmaps(WIDTH, HEIGHT);
        int width = mTestBitmaps[0].getWidth();
        int height = mTestBitmaps[0].getHeight();
        int stride = width;
        int[] argb = new int[stride * height];
        mTestBitmaps[0].getPixels(argb, 0, stride, 0, 0, width, height);
        byte[] yuv = convertArgbsToYuvs(argb, stride, height, ImageFormat.NV21);
        int[] strides = new int[] {
                stride, stride
        };
        YuvImage image = new YuvImage(yuv, ImageFormat.NV21, width, height, strides);
        assertEquals(yuv, image.getYuvData());
    }

    @Test
    public void testGetYuvFormat() {
        generateTestBitmaps(WIDTH, HEIGHT);
        YuvImage image = generateYuvImage(ImageFormat.YUY2, mTestBitmaps[0], 0);
        assertEquals(ImageFormat.YUY2, image.getYuvFormat());
    }

    private void generateTestBitmaps(int width, int height) {
        Bitmap dst = Bitmap.createBitmap(width, height, Bitmap.Config.ARGB_8888);
        Canvas c = new Canvas(dst);

        // mTestBitmap[0] = scaled testimage.jpg
        Resources res = InstrumentationRegistry.getTargetContext().getResources();
        Bitmap src = BitmapFactory.decodeResource(res, R.drawable.testimage);
        c.drawBitmap(src, null, new Rect(0, 0, WIDTH, HEIGHT), null);
        mTestBitmaps[0] = dst;
    }

    // Generate YuvImage based on the content in bitmap. If paddings > 0, the
    // strides of YuvImage are calculated by adding paddings to bitmap.getWidth().
    private YuvImage generateYuvImage(int format, Bitmap bitmap, int paddings) {
        int width = bitmap.getWidth();
        int height = bitmap.getHeight();

        int stride = width + paddings;

        YuvImage image = null;
        int[] argb = new int [stride * height];
        bitmap.getPixels(argb, 0, stride, 0, 0, width, height);
        byte[] yuv = convertArgbsToYuvs(argb, stride, height, format);

        int[] strides = null;
        if (format == ImageFormat.NV21) {
            strides = new int[] {stride, stride};
        } else if (format == ImageFormat.YUY2) {
           strides = new int[] {stride * 2};
        }
        image = new YuvImage(yuv, format, width, height, strides);

        return image;
    }

    // Compress rect1 in testBitmap and rect2 in image.
    // Then, compare the two resutls to check their MSE.
    private void compressRects(Bitmap testBitmap, YuvImage image,
            Rect rect1, Rect rect2) {
        Bitmap expected = null;
        Bitmap actual = null;
        boolean sameRect = rect1.equals(rect2) ? true : false;

		Rect actualRect = new Rect(rect2);
        actual = compressDecompress(image, actualRect);

        Rect expectedRect = sameRect ? actualRect : rect1;
        expected = Bitmap.createBitmap(testBitmap, expectedRect.left, expectedRect.top,
                expectedRect.width(), expectedRect.height());
        compareBitmaps(expected, actual, MSE_MARGIN, sameRect);
    }

    // Compress rect in image.
    // If side lengths of rect are odd, the rect might be modified by image,
    // We use the modified one to get pixels from testBitmap.
    private void compressOddRect(Bitmap testBitmap, YuvImage image,
            Rect rect) {
        Bitmap expected = null;
        Bitmap actual = null;
        actual = compressDecompress(image, rect);

        Rect newRect = rect;
        expected = Bitmap.createBitmap(testBitmap, newRect.left, newRect.top,
              newRect.width(), newRect.height());

        compareBitmaps(expected, actual, MSE_MARGIN, true);
    }

    // Compress rect in image to a jpeg and then decode the jpeg to a bitmap.
    private Bitmap compressDecompress(YuvImage image, Rect rect) {
        Bitmap result = null;
        ByteArrayOutputStream stream = new ByteArrayOutputStream();
        try {
            boolean rt = image.compressToJpeg(rect, 90, stream);
            assertTrue("fail in compression", rt);
            byte[] jpegData = stream.toByteArray();
            result = BitmapFactory.decodeByteArray(jpegData, 0,
                    jpegData.length);
        } catch(Exception e){
            Log.e(TAG, "unexpected exception", e);
            fail("unexpected exception");
        }
        return result;
    }

    private byte[] convertArgbsToYuvs(int[] argb, int width, int height,
            int format) {
        byte[] yuv = new byte[width * height *
                ImageFormat.getBitsPerPixel(format)];
        if (format == ImageFormat.NV21) {
            int vuStart = width * height;
            byte[] yuvColor = new byte[3];
            for (int row = 0; row < height; ++row) {
                for (int col = 0; col < width; ++col) {
                    int idx = row * width + col;
                    argb2yuv(argb[idx], yuvColor);
                    yuv[idx] = yuvColor[0];
                    if ((row & 1) == 0 && (col & 1) == 0) {
                        int offset = row / 2 * width + col / 2 * 2;
                        yuv[vuStart + offset] = yuvColor[2];
                        yuv[vuStart + offset + 1] = yuvColor[1];
                    }
                }
            }
        } else if (format == ImageFormat.YUY2) {
            byte[] yuvColor0 = new byte[3];
            byte[] yuvColor1 = new byte[3];
            for (int row = 0; row < height; ++row) {
                for (int col = 0; col < width; col += 2) {
                    int idx = row * width + col;
                    argb2yuv(argb[idx], yuvColor0);
                    argb2yuv(argb[idx + 1], yuvColor1);
                    int offset = idx / 2 * 4;
                    yuv[offset] = yuvColor0[0];
                    yuv[offset + 1] = yuvColor0[1];
                    yuv[offset + 2] = yuvColor1[0];
                    yuv[offset + 3] = yuvColor0[2];
                }
            }
        }

        return yuv;
    }

    // Compare expected to actual to see if their diff is less then mseMargin.
    // lessThanMargin is to indicate whether we expect the diff to be
    // "less than" or "no less than".
    private void compareBitmaps(Bitmap expected, Bitmap actual,
            int mseMargin, boolean lessThanMargin) {
        assertEquals("mismatching widths", expected.getWidth(),
                actual.getWidth());
        assertEquals("mismatching heights", expected.getHeight(),
                actual.getHeight());

        double mse = 0;
        int width = expected.getWidth();
        int height = expected.getHeight();
        int[] expColors = new int [width * height];
        expected.getPixels(expColors, 0, width, 0, 0, width, height);

        int[] actualColors = new int [width * height];
        actual.getPixels(actualColors, 0, width, 0, 0, width, height);

        for (int row = 0; row < height; ++row) {
            for (int col = 0; col < width; ++col) {
                int idx = row * width + col;
                mse += distance(expColors[idx], actualColors[idx]);
            }
        }
        mse /= width * height;

        Log.i(TAG, "MSE: " + mse);
        if (lessThanMargin) {
            assertTrue("MSE too large for normal case: " + mse,
                    mse <= mseMargin);
        } else {
            assertFalse("MSE too small for abnormal case: " + mse,
                    mse <= mseMargin);
        }
    }

    private double distance(int exp, int actual) {
        int r = Color.red(actual) - Color.red(exp);
        int g = Color.green(actual) - Color.green(exp);
        int b = Color.blue(actual) - Color.blue(exp);
        return r * r + g * g + b * b;
    }

    private void argb2yuv(int argb, byte[] yuv) {
        int r = Color.red(argb);
        int g = Color.green(argb);
        int b = Color.blue(argb);
        yuv[0] = (byte) ((CYR * r + CYG * g + CYB * b) >> CSHIFT);
        yuv[1] = (byte) (((CUR * r + CUG * g + CUB * b) >> CSHIFT) + 128);
        yuv[2] = (byte) (((CVR * r + CVG * g + CVB * b) >> CSHIFT) + 128);
    }

    private void verifyParameters() {
        int format = ImageFormat.NV21;
        int[] argb = new int[WIDTH * HEIGHT];
        mTestBitmaps[0].getPixels(argb, 0, WIDTH, 0, 0, WIDTH, HEIGHT);
        byte[] yuv = convertArgbsToYuvs(argb, WIDTH, HEIGHT, format);

        YuvImage image = new YuvImage(yuv, format, WIDTH, HEIGHT, null);
        ByteArrayOutputStream stream = new ByteArrayOutputStream();

        // abnormal case: quality > 100
        try{
            Rect rect = new Rect(0, 0, WIDTH, HEIGHT);
            image.compressToJpeg(rect, 101, stream);
            fail("not catching illegal compression quality");
        } catch(IllegalArgumentException e){
            // expected
        }

        // abnormal case: quality < 0
        try{
            Rect rect = new Rect(0, 0, WIDTH, HEIGHT);
            image.compressToJpeg(rect, -1, stream);
            fail("not catching illegal compression quality");
        } catch(IllegalArgumentException e){
            // expected
        }

        // abnormal case: stream is null
        try {
            Rect rect = new Rect(0, 0, WIDTH, HEIGHT);
            image.compressToJpeg(rect, 80, null);
            fail("not catching null stream");
        } catch(IllegalArgumentException e){
            // expected
        }

        // abnormal case: rectangle not within the whole image
        try {
            Rect rect = new Rect(10, 0, WIDTH, HEIGHT + 5);
            image.compressToJpeg(rect, 80, stream);
            fail("not catching illegal rectangular region");
        } catch(IllegalArgumentException e){
            // expected
        }
    }
}
