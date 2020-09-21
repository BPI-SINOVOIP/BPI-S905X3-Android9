/*
 * Copyright (C) 2016 The Android Open Source Project
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

package android.view.cts;

import static android.opengl.GLES20.GL_COLOR_BUFFER_BIT;
import static android.opengl.GLES20.GL_SCISSOR_TEST;
import static android.opengl.GLES20.glClear;
import static android.opengl.GLES20.glClearColor;
import static android.opengl.GLES20.glEnable;
import static android.opengl.GLES20.glScissor;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;

import android.graphics.Bitmap;
import android.graphics.Color;
import android.graphics.ColorSpace;
import android.graphics.Matrix;
import android.graphics.Point;
import android.support.test.filters.MediumTest;
import android.support.test.rule.ActivityTestRule;
import android.support.test.runner.AndroidJUnit4;
import android.util.Half;
import android.view.PixelCopy;
import android.view.TextureView;
import android.view.View;
import android.view.Window;

import com.android.compatibility.common.util.SynchronousPixelCopy;
import com.android.compatibility.common.util.WidgetTestUtils;

import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.util.concurrent.TimeoutException;

@MediumTest
@RunWith(AndroidJUnit4.class)
public class TextureViewTest {

    static final int EGL_GL_COLORSPACE_SRGB_KHR = 0x3089;
    static final int EGL_GL_COLORSPACE_DISPLAY_P3_EXT = 0x3363;
    static final int EGL_GL_COLORSPACE_SCRGB_LINEAR_EXT = 0x3350;

    @Rule
    public ActivityTestRule<TextureViewCtsActivity> mActivityRule =
            new ActivityTestRule<>(TextureViewCtsActivity.class, false, false);

    @Test
    public void testFirstFrames() throws Throwable {
        final TextureViewCtsActivity activity = mActivityRule.launchActivity(null);
        activity.waitForEnterAnimationComplete();

        final Point center = new Point();
        final Window[] windowRet = new Window[1];
        mActivityRule.runOnUiThread(() -> {
            View content = activity.findViewById(android.R.id.content);
            int[] outLocation = new int[2];
            content.getLocationOnScreen(outLocation);
            center.x = outLocation[0] + (content.getWidth() / 2);
            center.y = outLocation[1] + (content.getHeight() / 2);
            windowRet[0] = activity.getWindow();
        });
        final Window window = windowRet[0];
        assertTrue(center.x > 0);
        assertTrue(center.y > 0);
        waitForColor(window, center, Color.WHITE);
        activity.waitForSurface();
        activity.initGl();
        int updatedCount;
        updatedCount = activity.waitForSurfaceUpdateCount(0);
        assertEquals(0, updatedCount);
        activity.drawColor(Color.GREEN);
        updatedCount = activity.waitForSurfaceUpdateCount(1);
        assertEquals(1, updatedCount);
        assertEquals(Color.WHITE, getPixel(window, center));
        WidgetTestUtils.runOnMainAndDrawSync(mActivityRule,
                activity.findViewById(android.R.id.content), () -> activity.removeCover());

        int color = waitForChange(window, center, Color.WHITE);
        assertEquals(Color.GREEN, color);
        activity.drawColor(Color.BLUE);
        updatedCount = activity.waitForSurfaceUpdateCount(2);
        assertEquals(2, updatedCount);
        color = waitForChange(window, center, color);
        assertEquals(Color.BLUE, color);
    }

    @Test
    public void testScaling() throws Throwable {
        final TextureViewCtsActivity activity = mActivityRule.launchActivity(null);
        activity.drawFrame(TextureViewTest::drawGlQuad);
        final Bitmap bitmap = Bitmap.createBitmap(10, 10, Bitmap.Config.ARGB_8888);
        mActivityRule.runOnUiThread(() -> {
            activity.getTextureView().getBitmap(bitmap);
        });
        PixelCopyTest.assertBitmapQuadColor(bitmap,
                Color.RED, Color.GREEN, Color.BLUE, Color.BLACK);
    }

    @Test
    public void testRotateScale() throws Throwable {
        final TextureViewCtsActivity activity = mActivityRule.launchActivity(null);
        final TextureView textureView = activity.getTextureView();
        WidgetTestUtils.runOnMainAndDrawSync(mActivityRule, activity.getTextureView(), null);
        Matrix rotate = new Matrix();
        rotate.setRotate(180, textureView.getWidth() / 2, textureView.getHeight() / 2);
        activity.drawFrame(rotate, TextureViewTest::drawGlQuad);
        final Bitmap bitmap = Bitmap.createBitmap(10, 10, Bitmap.Config.ARGB_8888);
        mActivityRule.runOnUiThread(() -> {
            activity.getTextureView().getBitmap(bitmap);
        });
        PixelCopyTest.assertBitmapQuadColor(bitmap,
                Color.BLACK, Color.BLUE, Color.GREEN, Color.RED);
    }

    @Test
    public void testGetBitmap_8888_P3() throws Throwable {
        testGetBitmap(EGL_GL_COLORSPACE_DISPLAY_P3_EXT, ColorSpace.Named.DISPLAY_P3, false,
                new FP16Compare(ColorSpace.Named.EXTENDED_SRGB));
    }

    @Test
    public void testGetBitmap_FP16_P3() throws Throwable {
        testGetBitmap(EGL_GL_COLORSPACE_DISPLAY_P3_EXT, ColorSpace.Named.DISPLAY_P3, true,
                new FP16Compare(ColorSpace.Named.EXTENDED_SRGB));
    }

    @Test
    public void testGetBitmap_FP16_LinearExtendedSRGB() throws Throwable {
        testGetBitmap(EGL_GL_COLORSPACE_SCRGB_LINEAR_EXT, ColorSpace.Named.LINEAR_EXTENDED_SRGB,
                true, new FP16Compare(ColorSpace.Named.EXTENDED_SRGB));
    }

    @Test
    public void testGet565Bitmap_SRGB() throws Throwable {
        testGetBitmap(EGL_GL_COLORSPACE_SRGB_KHR, ColorSpace.Named.SRGB, true,
                new SRGBCompare(Bitmap.Config.RGB_565));
    }

    @Test
    public void testGetBitmap_SRGB() throws Throwable {
        testGetBitmap(EGL_GL_COLORSPACE_SRGB_KHR, ColorSpace.Named.SRGB, true,
                new SRGBCompare(Bitmap.Config.ARGB_8888));
    }

    interface CompareFunction {
        Bitmap.Config getConfig();
        ColorSpace getColorSpace();
        void verify(float[] srcColor, ColorSpace.Named srcColorSpace, Bitmap dstBitmap);
    }

    private class FP16Compare implements CompareFunction {
        private ColorSpace mDstColorSpace;

        FP16Compare(ColorSpace.Named namedCS) {
            mDstColorSpace = ColorSpace.get(namedCS);
        }

        public Bitmap.Config getConfig() {
            return Bitmap.Config.RGBA_F16;
        }

        public ColorSpace getColorSpace() {
            return mDstColorSpace;
        }

        public void verify(float[] srcColor, ColorSpace.Named srcColorSpace, Bitmap dstBitmap) {
            // read pixels into buffer and compare using colorspace connector
            ByteBuffer buffer = ByteBuffer.allocate(dstBitmap.getAllocationByteCount());
            buffer.order(ByteOrder.LITTLE_ENDIAN);
            dstBitmap.copyPixelsToBuffer(buffer);
            Half alpha = Half.valueOf(buffer.getShort(6));
            assertEquals(1.0f, alpha.floatValue(), 0.0f);

            final ColorSpace srcSpace = ColorSpace.get(srcColorSpace);
            final ColorSpace dstSpace = getColorSpace();
            float[] expectedColor = ColorSpace.connect(srcSpace, dstSpace).transform(srcColor);
            float[] outputColor = {
                    Half.valueOf(buffer.getShort(0)).floatValue(),
                    Half.valueOf(buffer.getShort(2)).floatValue(),
                    Half.valueOf(buffer.getShort(4)).floatValue() };

            assertEquals(expectedColor[0], outputColor[0], 0.01f);
            assertEquals(expectedColor[1], outputColor[1], 0.01f);
            assertEquals(expectedColor[2], outputColor[2], 0.01f);
        }
    }

    private class SRGBCompare implements CompareFunction {
        private Bitmap.Config mConfig;

        SRGBCompare(Bitmap.Config config) {
            mConfig = config;
        }

        public Bitmap.Config getConfig() {
            return mConfig;
        }

        public ColorSpace getColorSpace() {
            return ColorSpace.get(ColorSpace.Named.SRGB);
        }

        public void verify(float[] srcColor, ColorSpace.Named srcColorSpace, Bitmap dstBitmap) {
            int color = dstBitmap.getPixel(0, 0);
            assertEquals(1.0f, Color.alpha(color) / 255.0f, 0.0f);
            assertEquals(srcColor[0], Color.red(color) / 255.0f, 0.01f);
            assertEquals(srcColor[1], Color.green(color) / 255.0f, 0.01f);
            assertEquals(srcColor[2], Color.blue(color) / 255.0f, 0.01f);
        }
    }

    private void testGetBitmap(int eglColorSpace, ColorSpace.Named colorSpace,
            boolean useHalfFloat, CompareFunction compareFunction) throws Throwable {
        final TextureViewCtsActivity activity = mActivityRule.launchActivity(null);
        activity.waitForSurface();

        try {
            activity.initGl(eglColorSpace, useHalfFloat);
        } catch (RuntimeException e) {
            // failure to init GL with the right colorspace is not a TextureView failure as some
            // devices may not support 16-bits or the colorspace extension
            if (!activity.initGLExtensionUnsupported()) {
                fail("Unable to initGL : " + e);
            }
            return;
        }

        final float[] inputColor = { 1.0f, 128 / 255.0f, 0.0f};

        int updatedCount;
        updatedCount = activity.waitForSurfaceUpdateCount(0);
        assertEquals(0, updatedCount);
        activity.drawColor(inputColor[0], inputColor[1], inputColor[2], 1.0f);
        updatedCount = activity.waitForSurfaceUpdateCount(1);
        assertEquals(1, updatedCount);

        final Bitmap bitmap = activity.getContents(compareFunction.getConfig(),
                compareFunction.getColorSpace());
        compareFunction.verify(inputColor, colorSpace, bitmap);
    }

    private static void drawGlQuad(int width, int height) {
        int cx = width / 2;
        int cy = height / 2;

        glEnable(GL_SCISSOR_TEST);

        glScissor(0, cy, cx, height - cy);
        clearColor(Color.RED);

        glScissor(cx, cy, width - cx, height - cy);
        clearColor(Color.GREEN);

        glScissor(0, 0, cx, cy);
        clearColor(Color.BLUE);

        glScissor(cx, 0, width - cx, cy);
        clearColor(Color.BLACK);
    }

    private static void clearColor(int color) {
        glClearColor(Color.red(color) / 255.0f,
                Color.green(color) / 255.0f,
                Color.blue(color) / 255.0f,
                Color.alpha(color) / 255.0f);
        glClear(GL_COLOR_BUFFER_BIT);
    }

    private int getPixel(Window window, Point point) {
        Bitmap screenshot = Bitmap.createBitmap(window.getDecorView().getWidth(),
                window.getDecorView().getHeight(), Bitmap.Config.ARGB_8888);
        int result = new SynchronousPixelCopy().request(window, screenshot);
        assertEquals("Copy request failed", PixelCopy.SUCCESS, result);
        int pixel = screenshot.getPixel(point.x, point.y);
        screenshot.recycle();
        return pixel;
    }

    private void waitForColor(Window window, Point point, int color)
            throws InterruptedException, TimeoutException {
        for (int i = 0; i < 20; i++) {
            int pixel = getPixel(window, point);
            if (pixel == color) {
                return;
            }
            Thread.sleep(16);
        }
        throw new TimeoutException();
    }

    private int waitForChange(Window window, Point point, int color)
            throws InterruptedException, TimeoutException {
        for (int i = 0; i < 30; i++) {
            int pixel = getPixel(window, point);
            if (pixel != color) {
                return pixel;
            }
            Thread.sleep(16);
        }
        throw new TimeoutException();
    }
}
