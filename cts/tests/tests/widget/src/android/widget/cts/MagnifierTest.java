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
 * limitations under the License.
 */

package android.widget.cts;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;
import static org.junit.Assume.assumeTrue;

import android.app.Activity;
import android.graphics.Bitmap;
import android.graphics.PointF;
import android.graphics.Rect;
import android.support.test.annotation.UiThreadTest;
import android.support.test.filters.SmallTest;
import android.support.test.rule.ActivityTestRule;
import android.support.test.runner.AndroidJUnit4;
import android.util.DisplayMetrics;
import android.view.View;
import android.widget.LinearLayout;
import android.widget.LinearLayout.LayoutParams;
import android.widget.Magnifier;

import com.android.compatibility.common.util.PollingCheck;
import com.android.compatibility.common.util.WidgetTestUtils;

import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;

import java.util.ArrayList;
import java.util.Collections;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;

/**
 * Tests for {@link Magnifier}.
 */
@SmallTest
@RunWith(AndroidJUnit4.class)
public class MagnifierTest {
    private static final String TIME_LIMIT_EXCEEDED =
            "Completing the magnifier operation took too long";

    private Activity mActivity;
    private LinearLayout mLayout;
    private Magnifier mMagnifier;

    @Rule
    public ActivityTestRule<MagnifierCtsActivity> mActivityRule =
            new ActivityTestRule<>(MagnifierCtsActivity.class);

    @Before
    public void setup() {
        mActivity = mActivityRule.getActivity();
        PollingCheck.waitFor(mActivity::hasWindowFocus);
        mLayout = mActivity.findViewById(R.id.magnifier_activity_basic_layout);

        // Do not run the tests, unless the device screen is big enough to fit the magnifier.
        assumeTrue(isScreenBigEnough());
    }

    private boolean isScreenBigEnough() {
        // Get the size of the screen in dp.
        final DisplayMetrics displayMetrics = mActivity.getResources().getDisplayMetrics();
        final float dpScreenWidth = displayMetrics.widthPixels / displayMetrics.density;
        final float dpScreenHeight = displayMetrics.heightPixels / displayMetrics.density;
        // Get the size of the magnifier window in dp.
        final PointF dpMagnifier = Magnifier.getMagnifierDefaultSize();

        return dpScreenWidth >= dpMagnifier.x * 1.1 && dpScreenHeight >= dpMagnifier.y * 1.1;
    }

    @Test
    public void testConstructor() {
        new Magnifier(new View(mActivity));
    }

    @Test(expected = NullPointerException.class)
    public void testConstructor_NPE() {
        new Magnifier(null);
    }

    @Test
    @UiThreadTest
    public void testShow() {
        final View view = new View(mActivity);
        mLayout.addView(view, new LayoutParams(200, 200));
        mMagnifier = new Magnifier(view);
        // Valid coordinates.
        mMagnifier.show(0, 0);
        // Invalid coordinates, should both be clamped to 0.
        mMagnifier.show(-1, -1);
        // Valid coordinates.
        mMagnifier.show(10, 10);
        // Same valid coordinate as above, should skip making another copy request.
        mMagnifier.show(10, 10);
    }

    @Test
    @UiThreadTest
    public void testDismiss() {
        final View view = new View(mActivity);
        mLayout.addView(view, new LayoutParams(200, 200));
        mMagnifier = new Magnifier(view);
        // Valid coordinates.
        mMagnifier.show(10, 10);
        mMagnifier.dismiss();
        // Should be no-op.
        mMagnifier.dismiss();
    }

    @Test
    @UiThreadTest
    public void testUpdate() {
        final View view = new View(mActivity);
        mLayout.addView(view, new LayoutParams(200, 200));
        mMagnifier = new Magnifier(view);
        // Should be no-op.
        mMagnifier.update();
        // Valid coordinates.
        mMagnifier.show(10, 10);
        // Should not crash.
        mMagnifier.update();
        mMagnifier.dismiss();
        // Should be no-op.
        mMagnifier.update();
    }

    @Test
    @UiThreadTest
    public void testSizeAndZoom_areValid() {
        final View view = new View(mActivity);
        mLayout.addView(view, new LayoutParams(200, 200));
        mMagnifier = new Magnifier(view);
        mMagnifier.show(10, 10);
        // Size should be non-zero.
        assertTrue(mMagnifier.getWidth() > 0);
        assertTrue(mMagnifier.getHeight() > 0);
        // The magnified view region should be zoomed in, not out.
        assertTrue(mMagnifier.getZoom() > 1.0f);
    }

    @Test
    public void testWindowContent() throws Throwable {
        prepareFourQuadrantsScenario();
        final CountDownLatch latch = new CountDownLatch(1);
        mMagnifier.setOnOperationCompleteCallback(latch::countDown);

        // Show the magnifier at the center of the activity.
        mActivityRule.runOnUiThread(() -> {
            mMagnifier.show(mLayout.getWidth() / 2, mLayout.getHeight() / 2);
        });
        assertTrue(TIME_LIMIT_EXCEEDED, latch.await(1, TimeUnit.SECONDS));

        assertEquals(mMagnifier.getWidth(), mMagnifier.getContent().getWidth());
        assertEquals(mMagnifier.getHeight(), mMagnifier.getContent().getHeight());
        assertFourQuadrants(mMagnifier.getContent());
    }

    @Test
    public void testWindowPosition() throws Throwable {
        prepareFourQuadrantsScenario();
        final CountDownLatch latch = new CountDownLatch(1);
        mMagnifier.setOnOperationCompleteCallback(latch::countDown);

        // Show the magnifier at the center of the activity.
        mActivityRule.runOnUiThread(() -> {
            mMagnifier.show(mLayout.getWidth() / 2, mLayout.getHeight() / 2);
        });
        assertTrue(TIME_LIMIT_EXCEEDED, latch.await(1, TimeUnit.SECONDS));

        // Assert that the magnifier position represents a valid rectangle on screen.
        final Rect position = mMagnifier.getWindowPositionOnScreen();
        assertFalse(position.isEmpty());
        assertTrue(0 <= position.left && position.right <= mLayout.getWidth());
        assertTrue(0 <= position.top && position.bottom <= mLayout.getHeight());
    }

    @Test
    public void testWindowContent_modifiesAfterUpdate() throws Throwable {
        prepareFourQuadrantsScenario();

        // Show the magnifier at the center of the activity.
        final CountDownLatch latchForShow = new CountDownLatch(1);
        mMagnifier.setOnOperationCompleteCallback(latchForShow::countDown);
        mActivityRule.runOnUiThread(() -> {
            mMagnifier.show(mLayout.getWidth() / 2, mLayout.getHeight() / 2);
        });
        assertTrue(TIME_LIMIT_EXCEEDED, latchForShow.await(1, TimeUnit.SECONDS));

        final Bitmap initialBitmap = mMagnifier.getContent()
                .copy(mMagnifier.getContent().getConfig(), true);
        assertFourQuadrants(initialBitmap);

        // Make the one quadrant white.
        final View quadrant1 =
                mActivity.findViewById(R.id.magnifier_activity_four_quadrants_layout_quadrant_1);
        WidgetTestUtils.runOnMainAndDrawSync(mActivityRule, quadrant1, () -> {
            quadrant1.setBackground(null);
        });

        // Update the magnifier.
        final CountDownLatch latchForUpdate = new CountDownLatch(1);
        mMagnifier.setOnOperationCompleteCallback(latchForUpdate::countDown);
        mActivityRule.runOnUiThread(mMagnifier::update);
        assertTrue(TIME_LIMIT_EXCEEDED, latchForUpdate.await(1, TimeUnit.SECONDS));

        final Bitmap newBitmap = mMagnifier.getContent();
        assertFourQuadrants(newBitmap);
        assertFalse(newBitmap.sameAs(initialBitmap));
    }

    /**
     * Sets the activity to contain four equal quadrants coloured differently and
     * instantiates a magnifier. This method should not be called on the UI thread.
     */
    private void prepareFourQuadrantsScenario() throws Throwable {
        WidgetTestUtils.runOnMainAndLayoutSync(mActivityRule, () -> {
            mActivity.setContentView(R.layout.magnifier_activity_four_quadrants_layout);
            mLayout = mActivity.findViewById(R.id.magnifier_activity_four_quadrants_layout);
            mMagnifier = new Magnifier(mLayout);
        }, false);
        WidgetTestUtils.runOnMainAndDrawSync(mActivityRule, mLayout, null);
    }

    /**
     * Asserts that the current bitmap contains four different dominant colors, which
     * are (almost) equally distributed. The test takes into account an amount of
     * noise, possible consequence of upscaling and filtering the magnified content.
     *
     * @param bitmap the bitmap to be checked
     */
    private void assertFourQuadrants(final Bitmap bitmap) {
        final int expectedQuadrants = 4;
        final int totalPixels = bitmap.getWidth() * bitmap.getHeight();

        final Map<Integer, Integer> colorCount = new HashMap<>();
        for (int x = 0; x < bitmap.getWidth(); ++x) {
            for (int y = 0; y < bitmap.getHeight(); ++y) {
                final int currentColor = bitmap.getPixel(x, y);
                colorCount.put(currentColor, colorCount.getOrDefault(currentColor, 0) + 1);
            }
        }
        assertTrue(colorCount.size() >= expectedQuadrants);

        final List<Integer> counts = new ArrayList<>(colorCount.values());
        Collections.sort(counts);

        int quadrantsTotal = 0;
        for (int i = counts.size() - expectedQuadrants; i < counts.size(); ++i) {
            quadrantsTotal += counts.get(i);
        }
        assertTrue(1.0f * (totalPixels - quadrantsTotal) / totalPixels <= 0.1f);

        for (int i = counts.size() - expectedQuadrants; i < counts.size(); ++i) {
            final float proportion = 1.0f
                    * Math.abs(expectedQuadrants * counts.get(i) - quadrantsTotal) / quadrantsTotal;
            assertTrue(proportion <= 0.1f);
        }
    }
}
