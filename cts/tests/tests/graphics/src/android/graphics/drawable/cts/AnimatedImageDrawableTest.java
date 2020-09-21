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

package android.graphics.drawable.cts;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNotEquals;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;

import android.app.Activity;
import android.content.ContentResolver;
import android.content.res.Resources;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.ColorFilter;
import android.graphics.ImageDecoder;
import android.graphics.LightingColorFilter;
import android.graphics.Paint;
import android.graphics.PixelFormat;
import android.graphics.Rect;
import android.graphics.cts.R;
import android.graphics.drawable.AnimatedImageDrawable;
import android.graphics.drawable.Drawable;
import android.net.Uri;
import android.support.test.InstrumentationRegistry;
import android.support.test.rule.ActivityTestRule;
import android.support.test.runner.AndroidJUnit4;
import android.view.View;
import android.widget.ImageView;

import com.android.compatibility.common.util.BitmapUtils;

import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.xmlpull.v1.XmlPullParser;
import org.xmlpull.v1.XmlPullParserException;

import java.io.IOException;
import java.util.function.BiFunction;

@RunWith(AndroidJUnit4.class)
public class AnimatedImageDrawableTest {
    private Resources mRes;
    private ContentResolver mContentResolver;

    private static final int RES_ID = R.drawable.animated;
    private static final int WIDTH = 278;
    private static final int HEIGHT = 183;
    private static final int NUM_FRAMES = 4;
    private static final int FRAME_DURATION = 250; // in milliseconds
    private static final int DURATION = NUM_FRAMES * FRAME_DURATION;
    private static final int LAYOUT = R.layout.animated_image_layout;
    private static final int IMAGE_ID = R.id.animated_image;
    @Rule
    public ActivityTestRule<DrawableStubActivity> mActivityRule =
            new ActivityTestRule<DrawableStubActivity>(DrawableStubActivity.class);
    private Activity mActivity;

    private Uri getAsResourceUri(int resId) {
        return new Uri.Builder()
            .scheme(ContentResolver.SCHEME_ANDROID_RESOURCE)
            .authority(mRes.getResourcePackageName(resId))
            .appendPath(mRes.getResourceTypeName(resId))
            .appendPath(mRes.getResourceEntryName(resId))
            .build();
    }

    @Before
    public void setup() {
        mRes = InstrumentationRegistry.getTargetContext().getResources();
        mContentResolver = InstrumentationRegistry.getTargetContext().getContentResolver();
        mActivity = mActivityRule.getActivity();
    }

    @Test
    public void testEmptyConstructor() {
        new AnimatedImageDrawable();
    }

    @Test
    public void testMutate() {
        AnimatedImageDrawable aid1 = (AnimatedImageDrawable) mRes.getDrawable(R.drawable.animated);
        AnimatedImageDrawable aid2 = (AnimatedImageDrawable) mRes.getDrawable(R.drawable.animated);

        final int originalAlpha = aid1.getAlpha();
        assertEquals(255, originalAlpha);
        assertEquals(255, aid2.getAlpha());

        try {
            aid1.mutate();
            aid1.setAlpha(100);
            assertEquals(originalAlpha, aid2.getAlpha());
        } finally {
            mRes.getDrawable(R.drawable.animated).setAlpha(originalAlpha);
        }
    }

    private AnimatedImageDrawable createFromImageDecoder(int resId) {
        Uri uri = null;
        try {
            uri = getAsResourceUri(resId);
            ImageDecoder.Source source = ImageDecoder.createSource(mContentResolver, uri);
            Drawable drawable = ImageDecoder.decodeDrawable(source);
            assertTrue(drawable instanceof AnimatedImageDrawable);
            return (AnimatedImageDrawable) drawable;
        } catch (IOException e) {
            fail("failed to create image from " + uri);
            return null;
        }
    }

    @Test
    public void testDecodeAnimatedImageDrawable() {
        Drawable drawable = createFromImageDecoder(RES_ID);
        assertEquals(WIDTH,  drawable.getIntrinsicWidth());
        assertEquals(HEIGHT, drawable.getIntrinsicHeight());
    }

    private static class Callback extends Animatable2Callback {
        private final Drawable mDrawable;

        public Callback(Drawable d) {
            mDrawable = d;
        }

        @Override
        public void onAnimationStart(Drawable drawable) {
            assertNotNull(drawable);
            assertEquals(mDrawable, drawable);
            super.onAnimationStart(drawable);
        }

        @Override
        public void onAnimationEnd(Drawable drawable) {
            assertNotNull(drawable);
            assertEquals(mDrawable, drawable);
            super.onAnimationEnd(drawable);
        }
    };

    @Test(expected=IllegalStateException.class)
    public void testRegisterWithoutLooper() {
        AnimatedImageDrawable drawable = createFromImageDecoder(R.drawable.animated);

        // registerAnimationCallback must be run on a thread with a Looper,
        // which the test thread does not have.
        Callback cb = new Callback(drawable);
        drawable.registerAnimationCallback(cb);
    }

    @Test
    public void testRegisterCallback() throws Throwable {
        AnimatedImageDrawable drawable = createFromImageDecoder(R.drawable.animated);

        mActivityRule.runOnUiThread(() -> {
            // Register a callback.
            Callback cb = new Callback(drawable);
            drawable.registerAnimationCallback(cb);
            assertTrue(drawable.unregisterAnimationCallback(cb));

            // Now that it has been removed, it cannot be removed again.
            assertFalse(drawable.unregisterAnimationCallback(cb));
        });
    }

    @Test
    public void testClearCallbacks() throws Throwable {
        AnimatedImageDrawable drawable = createFromImageDecoder(R.drawable.animated);

        Callback[] callbacks = new Callback[] {
            new Callback(drawable),
            new Callback(drawable),
            new Callback(drawable),
            new Callback(drawable),
            new Callback(drawable),
            new Callback(drawable),
            new Callback(drawable),
            new Callback(drawable),
        };

        mActivityRule.runOnUiThread(() -> {
            for (Callback cb : callbacks) {
                drawable.registerAnimationCallback(cb);
            }
        });

        drawable.clearAnimationCallbacks();

        for (Callback cb : callbacks) {
            // It has already been removed.
            assertFalse(drawable.unregisterAnimationCallback(cb));
        }
    }

    /**
     *  Helper for attaching drawable to the view system.
     *
     *  Necessary for the drawable to animate.
     *
     *  Must be called from UI thread.
     */
    private void setContentView(AnimatedImageDrawable drawable) {
        mActivity.setContentView(LAYOUT);
        ImageView imageView = (ImageView) mActivity.findViewById(IMAGE_ID);
        imageView.setImageDrawable(drawable);
    }

    @Test
    public void testUnregisterCallback() throws Throwable {
        AnimatedImageDrawable drawable = createFromImageDecoder(R.drawable.animated);

        Callback cb = new Callback(drawable);
        mActivityRule.runOnUiThread(() -> {
            setContentView(drawable);

            drawable.registerAnimationCallback(cb);
            assertTrue(drawable.unregisterAnimationCallback(cb));
            drawable.setRepeatCount(0);
            drawable.start();
        });

        cb.waitForStart();
        cb.assertStarted(false);

        cb.waitForEnd(DURATION * 2);
        cb.assertEnded(false);
    }

    @Test
    public void testLifeCycle() throws Throwable {
        AnimatedImageDrawable drawable = createFromImageDecoder(RES_ID);

        // Only run the animation one time.
        drawable.setRepeatCount(0);

        Callback cb = new Callback(drawable);
        mActivityRule.runOnUiThread(() -> {
            setContentView(drawable);

            drawable.registerAnimationCallback(cb);
        });

        assertFalse(drawable.isRunning());
        cb.assertStarted(false);
        cb.assertEnded(false);

        mActivityRule.runOnUiThread(() -> {
            drawable.start();
            assertTrue(drawable.isRunning());
        });
        cb.waitForStart();
        cb.assertStarted(true);

        // Extra time, to wait for the message to post.
        cb.waitForEnd(DURATION * 3);
        cb.assertEnded(true);
        assertFalse(drawable.isRunning());
    }

    @Test
    public void testLifeCycleSoftware() throws Throwable {
        AnimatedImageDrawable drawable = createFromImageDecoder(RES_ID);

        Bitmap bm = Bitmap.createBitmap(drawable.getIntrinsicWidth(), drawable.getIntrinsicHeight(),
                Bitmap.Config.ARGB_8888);
        Canvas canvas = new Canvas(bm);

        Callback cb = new Callback(drawable);
        mActivityRule.runOnUiThread(() -> {
            drawable.registerAnimationCallback(cb);
            drawable.draw(canvas);
        });

        assertFalse(drawable.isRunning());
        cb.assertStarted(false);
        cb.assertEnded(false);

        mActivityRule.runOnUiThread(() -> {
            drawable.start();
            assertTrue(drawable.isRunning());
            drawable.draw(canvas);
        });
        cb.waitForStart();
        cb.assertStarted(true);

        // Only run the animation one time.
        drawable.setRepeatCount(0);

        // The drawable will prevent skipping frames, so we actually have to
        // draw each frame. (Start with 1, since we already drew frame 0.)
        for (int i = 1; i < NUM_FRAMES; i++) {
            cb.waitForEnd(FRAME_DURATION);
            cb.assertEnded(false);
            mActivityRule.runOnUiThread(() -> {
                assertTrue(drawable.isRunning());
                drawable.draw(canvas);
            });
        }

        cb.waitForEnd(FRAME_DURATION);
        assertFalse(drawable.isRunning());
        cb.assertEnded(true);
    }

    @Test
    public void testAddCallbackAfterStart() throws Throwable {
        AnimatedImageDrawable drawable = createFromImageDecoder(RES_ID);
        Callback cb = new Callback(drawable);
        mActivityRule.runOnUiThread(() -> {
            setContentView(drawable);

            drawable.setRepeatCount(0);
            drawable.start();
            drawable.registerAnimationCallback(cb);
        });

        // Add extra duration to wait for the message posted by the end of the
        // animation. This should help fix flakiness.
        cb.waitForEnd(DURATION * 3);
        cb.assertEnded(true);
    }

    @Test
    public void testStop() throws Throwable {
        AnimatedImageDrawable drawable = createFromImageDecoder(RES_ID);
        Callback cb = new Callback(drawable);
        mActivityRule.runOnUiThread(() -> {
            setContentView(drawable);

            drawable.registerAnimationCallback(cb);

            drawable.start();
            assertTrue(drawable.isRunning());

            drawable.stop();
            assertFalse(drawable.isRunning());
        });

        // This duration may be overkill, but we need to wait for the message
        // to post. Increasing it should help with flakiness on bots.
        cb.waitForEnd(DURATION * 3);
        cb.assertStarted(true);
        cb.assertEnded(true);
    }

    @Test
    public void testRepeatCounts() throws Throwable {
        for (int repeatCount : new int[] { 3, 5, 7, 16 }) {
            AnimatedImageDrawable drawable = createFromImageDecoder(RES_ID);
            assertEquals(AnimatedImageDrawable.REPEAT_INFINITE, drawable.getRepeatCount());

            Callback cb = new Callback(drawable);
            mActivityRule.runOnUiThread(() -> {
                setContentView(drawable);

                drawable.registerAnimationCallback(cb);
                drawable.setRepeatCount(repeatCount);
                assertEquals(repeatCount, drawable.getRepeatCount());
                drawable.start();
            });

            // The animation runs repeatCount + 1 total times.
            cb.waitForEnd(DURATION * repeatCount);
            cb.assertEnded(false);

            cb.waitForEnd(DURATION * 2);
            cb.assertEnded(true);

            drawable.setRepeatCount(AnimatedImageDrawable.REPEAT_INFINITE);
            assertEquals(AnimatedImageDrawable.REPEAT_INFINITE, drawable.getRepeatCount());
        }
    }

    @Test
    public void testRepeatCountInfinite() throws Throwable {
        AnimatedImageDrawable drawable = createFromImageDecoder(RES_ID);
        Callback cb = new Callback(drawable);
        mActivityRule.runOnUiThread(() -> {
            setContentView(drawable);

            drawable.registerAnimationCallback(cb);
            drawable.setRepeatCount(AnimatedImageDrawable.REPEAT_INFINITE);
            drawable.start();
        });

        // There is no way to truly test infinite, but let it run for a long
        // time and verify that it's still running.
        cb.waitForEnd(DURATION * 30);
        cb.assertEnded(false);
        assertTrue(drawable.isRunning());
    }

    @Test
    public void testGetOpacity() {
        AnimatedImageDrawable drawable = createFromImageDecoder(RES_ID);
        assertEquals(PixelFormat.TRANSLUCENT, drawable.getOpacity());
    }

    @Test
    public void testColorFilter() {
        AnimatedImageDrawable drawable = createFromImageDecoder(RES_ID);

        ColorFilter filter = new LightingColorFilter(0, Color.RED);
        drawable.setColorFilter(filter);
        assertEquals(filter, drawable.getColorFilter());

        Bitmap actual = Bitmap.createBitmap(drawable.getIntrinsicWidth(),
                drawable.getIntrinsicHeight(), Bitmap.Config.ARGB_8888);
        {
            Canvas canvas = new Canvas(actual);
            drawable.draw(canvas);
        }

        for (int i = 0; i < actual.getWidth(); ++i) {
            for (int j = 0; j < actual.getHeight(); ++j) {
                int color = actual.getPixel(i, j);
                // The LightingColorFilter does not affect the transparent pixels,
                // so all pixels should either remain transparent or turn red.
                if (color != Color.RED && color != Color.TRANSPARENT) {
                    fail("pixel at " + i + ", " + j + " does not match expected. "
                            + "expected: " + Color.RED + " OR " + Color.TRANSPARENT
                            + " actual: " + color);
                }
            }
        }
    }

    @Test
    public void testPostProcess() {
        // Compare post processing a Rect in the middle of the (not-animating)
        // image with drawing manually. They should be exactly the same.
        BiFunction<Integer, Integer, Rect> rectCreator = (width, height) -> {
            int quarterWidth  = width  / 4;
            int quarterHeight = height / 4;
            return new Rect(quarterWidth, quarterHeight,
                    3 * quarterWidth, 3 * quarterHeight);
        };

        AnimatedImageDrawable drawable = createFromImageDecoder(RES_ID);
        Bitmap expected = Bitmap.createBitmap(drawable.getIntrinsicWidth(),
                drawable.getIntrinsicHeight(), Bitmap.Config.ARGB_8888);

        Paint paint = new Paint();
        paint.setColor(Color.RED);

        {
            Rect r = rectCreator.apply(drawable.getIntrinsicWidth(),
                                       drawable.getIntrinsicHeight());
            Canvas canvas = new Canvas(expected);
            drawable.draw(canvas);

            for (int i = r.left; i < r.right; ++i) {
                for (int j = r.top; j < r.bottom; ++j) {
                    assertNotEquals(Color.RED, expected.getPixel(i, j));
                }
            }

            canvas.drawRect(r, paint);

            for (int i = r.left; i < r.right; ++i) {
                for (int j = r.top; j < r.bottom; ++j) {
                    assertEquals(Color.RED, expected.getPixel(i, j));
                }
            }
        }


        AnimatedImageDrawable testDrawable = null;
        Uri uri = null;
        try {
            uri = getAsResourceUri(RES_ID);
            ImageDecoder.Source source = ImageDecoder.createSource(mContentResolver, uri);
            Drawable dr = ImageDecoder.decodeDrawable(source, (decoder, info, src) -> {
                decoder.setPostProcessor((canvas) -> {
                    canvas.drawRect(rectCreator.apply(canvas.getWidth(),
                                                      canvas.getHeight()), paint);
                    return PixelFormat.TRANSLUCENT;
                });
            });
            assertTrue(dr instanceof AnimatedImageDrawable);
            testDrawable = (AnimatedImageDrawable) dr;
        } catch (IOException e) {
            fail("failed to create image from " + uri);
        }

        Bitmap actual = Bitmap.createBitmap(drawable.getIntrinsicWidth(),
                drawable.getIntrinsicHeight(), Bitmap.Config.ARGB_8888);

        {
            Canvas canvas = new Canvas(actual);
            testDrawable.draw(canvas);
        }

        BitmapUtils.compareBitmaps(expected, actual);
    }

    @Test
    public void testCreateFromXml() throws XmlPullParserException, IOException {
        XmlPullParser parser = mRes.getXml(R.drawable.animatedimagedrawable_tag);
        Drawable drawable = Drawable.createFromXml(mRes, parser);
        assertNotNull(drawable);
        assertTrue(drawable instanceof AnimatedImageDrawable);
    }

    @Test
    public void testCreateFromXmlClass() throws XmlPullParserException, IOException {
        XmlPullParser parser = mRes.getXml(R.drawable.animatedimagedrawable);
        Drawable drawable = Drawable.createFromXml(mRes, parser);
        assertNotNull(drawable);
        assertTrue(drawable instanceof AnimatedImageDrawable);
    }

    @Test
    public void testCreateFromXmlClassAttribute() throws XmlPullParserException, IOException {
        XmlPullParser parser = mRes.getXml(R.drawable.animatedimagedrawable_class);
        Drawable drawable = Drawable.createFromXml(mRes, parser);
        assertNotNull(drawable);
        assertTrue(drawable instanceof AnimatedImageDrawable);
    }

    @Test(expected=XmlPullParserException.class)
    public void testMissingSrcInflate() throws XmlPullParserException, IOException  {
        XmlPullParser parser = mRes.getXml(R.drawable.animatedimagedrawable_nosrc);
        Drawable drawable = Drawable.createFromXml(mRes, parser);
    }

    @Test
    public void testAutoMirrored() {
        AnimatedImageDrawable drawable = createFromImageDecoder(RES_ID);
        assertFalse(drawable.isAutoMirrored());

        drawable.setAutoMirrored(true);
        assertTrue(drawable.isAutoMirrored());

        drawable.setAutoMirrored(false);
        assertFalse(drawable.isAutoMirrored());
    }

    @Test
    public void testAutoMirroredFromXml() throws XmlPullParserException, IOException {
        AnimatedImageDrawable drawable = parseXml(R.drawable.animatedimagedrawable_tag);
        assertFalse(drawable.isAutoMirrored());

        drawable = parseXml(R.drawable.animatedimagedrawable_automirrored);
        assertTrue(drawable.isAutoMirrored());
    }

    private AnimatedImageDrawable parseXml(int resId) throws XmlPullParserException, IOException {
        XmlPullParser parser = mRes.getXml(resId);
        Drawable drawable = Drawable.createFromXml(mRes, parser);
        assertNotNull(drawable);
        assertTrue(drawable instanceof AnimatedImageDrawable);
        return (AnimatedImageDrawable) drawable;
    }

    @Test
    public void testAutoStartFromXml() throws XmlPullParserException, IOException {
        AnimatedImageDrawable drawable = parseXml(R.drawable.animatedimagedrawable_tag);
        assertFalse(drawable.isRunning());

        drawable = parseXml(R.drawable.animatedimagedrawable_autostart_false);
        assertFalse(drawable.isRunning());

        drawable = parseXml(R.drawable.animatedimagedrawable_autostart);
        assertTrue(drawable.isRunning());
    }

    private void drawAndCompare(Bitmap expected, Drawable drawable) {
        Bitmap test = Bitmap.createBitmap(drawable.getIntrinsicWidth(),
                drawable.getIntrinsicHeight(), Bitmap.Config.ARGB_8888);
        Canvas canvas = new Canvas(test);
        drawable.draw(canvas);
        BitmapUtils.compareBitmaps(expected, test);
    }

    @Test
    public void testAutoMirroredDrawing() {
        AnimatedImageDrawable drawable = createFromImageDecoder(RES_ID);
        assertFalse(drawable.isAutoMirrored());

        final int width = drawable.getIntrinsicWidth();
        final int height = drawable.getIntrinsicHeight();
        Bitmap normal = Bitmap.createBitmap(width, height, Bitmap.Config.ARGB_8888);
        {
            Canvas canvas = new Canvas(normal);
            drawable.draw(canvas);
        }

        Bitmap flipped = Bitmap.createBitmap(width, height, Bitmap.Config.ARGB_8888);
        {
            Canvas canvas = new Canvas(flipped);
            canvas.translate(width, 0);
            canvas.scale(-1, 1);
            drawable.draw(canvas);
        }

        for (int i = 0; i < width; ++i) {
            for (int j = 0; j < height; ++j) {
                assertEquals(normal.getPixel(i, j), flipped.getPixel(width - 1 - i, j));
            }
        }

        drawable.setAutoMirrored(true);
        drawAndCompare(normal, drawable);

        drawable.setLayoutDirection(View.LAYOUT_DIRECTION_RTL);
        drawAndCompare(flipped, drawable);

        drawable.setAutoMirrored(false);
        drawAndCompare(normal, drawable);
    }

    @Test
    public void testRepeatCountFromXml() throws XmlPullParserException, IOException {
        XmlPullParser parser = mRes.getXml(R.drawable.animatedimagedrawable_loop_count);
        Drawable drawable = Drawable.createFromXml(mRes, parser);
        assertNotNull(drawable);
        assertTrue(drawable instanceof AnimatedImageDrawable);

        AnimatedImageDrawable aid = (AnimatedImageDrawable) drawable;
        assertEquals(17, aid.getRepeatCount());
    }

    @Test
    public void testInfiniteRepeatCountFromXml() throws XmlPullParserException, IOException {
        // This image has an encoded repeat count of 1. Verify that.
        Drawable drawable = mRes.getDrawable(R.drawable.animated_one_loop);
        assertNotNull(drawable);
        assertTrue(drawable instanceof AnimatedImageDrawable);
        AnimatedImageDrawable aid = (AnimatedImageDrawable) drawable;
        assertEquals(1, aid.getRepeatCount());

        // This layout uses the same image and overrides the repeat count to infinity.
        XmlPullParser parser = mRes.getXml(R.drawable.animatedimagedrawable_loop_count_infinite);
        drawable = Drawable.createFromXml(mRes, parser);
        assertNotNull(drawable);
        assertTrue(drawable instanceof AnimatedImageDrawable);

        aid = (AnimatedImageDrawable) drawable;
        assertEquals(AnimatedImageDrawable.REPEAT_INFINITE, aid.getRepeatCount());
    }
}
