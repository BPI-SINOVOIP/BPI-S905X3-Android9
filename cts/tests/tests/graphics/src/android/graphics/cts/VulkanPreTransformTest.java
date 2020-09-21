/*
 * Copyright 2018 The Android Open Source Project
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
import static org.junit.Assert.assertNotNull;

import android.graphics.Bitmap;
import android.graphics.Color;
import android.graphics.Rect;
import android.os.SystemClock;
import android.support.test.rule.ActivityTestRule;
import android.support.test.runner.AndroidJUnit4;
import android.test.suitebuilder.annotation.LargeTest;
import android.util.Log;
import android.view.PixelCopy;
import android.view.SurfaceView;

import com.android.compatibility.common.util.SynchronousPixelCopy;

import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;

/*
 * This test case runs in landscape mode
 *
 * testVulkanPreTransformSetToMatchCurrentTransform()
 *
 *      Buffer          ExpectedScreen
 *      ---------       ---------------
 *      | R | G |       | GGGG | YYYY |
 *      ---------       ---------------
 *      | B | Y |       | RRRR | BBBB |
 *      ---------       ---------------
 *
 * testVulkanPreTransformNotSetToMatchCurrentTransform()
 *
 *      Buffer          ExpectedScreen
 *      ---------       ---------------
 *      | R | G |       | RRRR | GGGG |
 *      ---------       ---------------
 *      | B | Y |       | BBBB | YYYY |
 *      ---------       ---------------
 */

@LargeTest
@RunWith(AndroidJUnit4.class)
public class VulkanPreTransformTest {
    private static final String TAG = "vulkan";
    private static final boolean DEBUG = false;
    private static VulkanPreTransformCtsActivity sActivity = null;

    @Rule
    public ActivityTestRule<VulkanPreTransformCtsActivity> mActivityRule =
            new ActivityTestRule<>(VulkanPreTransformCtsActivity.class, false, false);

    @Before
    public void setUp() {
        Log.d(TAG, "setUp!");
        // Work around for b/77148807
        // Activity was falsely created before ActivityManager set config change to landscape
        SystemClock.sleep(2000);
    }

    @Test
    public void testVulkanPreTransformSetToMatchCurrentTransform() throws Throwable {
        Log.d(TAG, "testVulkanPreTransformSetToMatchCurrentTransform start");
        sActivity = mActivityRule.launchActivity(null);
        sActivity.testVulkanPreTransform(true);
        sActivity.finish();
        sActivity = null;
    }

    @Test
    public void testVulkanPreTransformNotSetToMatchCurrentTransform() throws Throwable {
        Log.d(TAG, "testVulkanPreTransformNotSetToMatchCurrentTransform start");
        sActivity = mActivityRule.launchActivity(null);
        sActivity.testVulkanPreTransform(false);
        sActivity.finish();
        sActivity = null;
    }

    private static Bitmap takeScreenshot() {
        assertNotNull("sActivity should not be null", sActivity);
        Rect srcRect = new Rect();
        sActivity.findViewById(R.id.surfaceview).getGlobalVisibleRect(srcRect);
        SynchronousPixelCopy copy = new SynchronousPixelCopy();
        Bitmap dest =
                Bitmap.createBitmap(srcRect.width(), srcRect.height(), Bitmap.Config.ARGB_8888);
        int copyResult =
                copy.request((SurfaceView) sActivity.findViewById(R.id.surfaceview), srcRect, dest);
        assertEquals("PixelCopy failed", PixelCopy.SUCCESS, copyResult);
        return dest;
    }

    private static int pixelDiff(int pixel, int expectedR, int expectedG, int expectedB) {
        int actualR = Color.red(pixel);
        int actualG = Color.green(pixel);
        int actualB = Color.blue(pixel);
        Log.d(TAG,
                "ExpectedPixel(" + expectedR + "," + expectedG + "," + expectedB + "), ActualPixel("
                        + actualR + "," + actualG + "," + actualB + ")");
        return Math.abs(actualR - expectedR) + Math.abs(actualG - expectedG)
                + Math.abs(actualB - expectedB);
    }

    private static boolean validatePixelValuesAfterRotation(boolean setPreTransform) {
        Bitmap bitmap = takeScreenshot();

        int width = bitmap.getWidth();
        int height = bitmap.getHeight();
        int diff = 0;
        if (setPreTransform) {
            diff += pixelDiff(bitmap.getPixel(0, 0), 0, 255, 0);
            diff += pixelDiff(bitmap.getPixel(width - 1, 0), 255, 255, 0);
            diff += pixelDiff(bitmap.getPixel(0, height - 1), 255, 0, 0);
            diff += pixelDiff(bitmap.getPixel(width - 1, height - 1), 0, 0, 255);
        } else {
            diff += pixelDiff(bitmap.getPixel(0, 0), 255, 0, 0);
            diff += pixelDiff(bitmap.getPixel(width - 1, 0), 0, 255, 0);
            diff += pixelDiff(bitmap.getPixel(0, height - 1), 0, 0, 255);
            diff += pixelDiff(bitmap.getPixel(width - 1, height - 1), 255, 255, 0);
        }

        return diff < 10;
    }
}
