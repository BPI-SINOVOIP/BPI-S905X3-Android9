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

package android.view.inputmethod.cts.util;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;

import android.graphics.Bitmap;
import android.graphics.Color;
import androidx.annotation.ColorInt;
import androidx.annotation.NonNull;

import java.util.ArrayList;

/**
 * A utility class to evaluate if {@link android.view.Window#setNavigationBarColor(int)} is
 * supported on the device or not.
 */
public class NavigationBarColorVerifier {

    private static final class ScreenShot {
        @ColorInt
        public final int mBackgroundColor;
        @NonNull
        public final int[] mPixels;

        ScreenShot(@ColorInt int backgroundColor, @NonNull Bitmap bitmap) {
            mBackgroundColor = backgroundColor;
            final int width = bitmap.getWidth();
            final int height = bitmap.getHeight();
            mPixels = new int[width * height];
            bitmap.getPixels(mPixels, 0 /* offset */, width /* stride */, 0 /* x */, 0 /* y */,
                    width, height);
        }
    }

    public enum ResultType {
        /**
         * {@link android.view.Window#setNavigationBarColor(int)} seems to be not supported.
         */
        NOT_SUPPORTED,
        /**
         * {@link android.view.Window#setNavigationBarColor(int)} seems to be supported.
         */
        SUPPORTED,
    }

    static final class Result {
        @NonNull
        public final ResultType mResult;
        @NonNull
        public final String mAssertionMessage;

        Result(@NonNull ResultType result, @NonNull String assertionMessage) {
            mResult = result;
            mAssertionMessage = assertionMessage;
        }

        @NonNull
        public ResultType getResult() {
            return mResult;
        }

        @NonNull
        public String getAssertionMessage() {
            return mAssertionMessage;
        }
    }

    @FunctionalInterface
    public interface ScreenshotSupplier {
        @NonNull
        Bitmap takeScreenshot(@ColorInt int navigationBarColor) throws Exception;
    }

    @NonNull
    static Result verify(@NonNull ScreenshotSupplier screenshotSupplier) throws Exception {
        final ArrayList<ScreenShot> screenShots = new ArrayList<>();
        final int[] colors = new int[]{Color.RED, Color.GREEN, Color.BLUE};
        for (int color : colors) {
            screenShots.add(new ScreenShot(color, screenshotSupplier.takeScreenshot(color)));
        }
        return verifyInternal(screenShots);
    }

    /**
     * Asserts that {@link android.view.Window#setNavigationBarColor(int)} is supported on this
     * device.
     *
     * @param screenshotSupplier callback to provide {@link Bitmap} of the navigation bar region
     */
    public static void expectNavigationBarColorSupported(
            @NonNull ScreenshotSupplier screenshotSupplier) throws Exception {
        final Result result = verify(screenshotSupplier);
        assertEquals(result.getAssertionMessage(), ResultType.SUPPORTED, result.getResult());
    }

    /**
     * Asserts that {@link android.view.Window#setNavigationBarColor(int)} is not supported on this
     * device.
     *
     * @param screenshotSupplier callback to provide {@link Bitmap} of the navigation bar region
     */
    public static void expectNavigationBarColorNotSupported(
            @NonNull ScreenshotSupplier screenshotSupplier) throws Exception {
        final Result result = verify(screenshotSupplier);
        assertEquals(result.getAssertionMessage(), ResultType.NOT_SUPPORTED, result.getResult());
    }

    private static Result verifyInternal(@NonNull ArrayList<ScreenShot> screenShots) {
        final int numScreenShots = screenShots.size();
        assertTrue(
                "This algorithm requires at least 3 screen shots. size=" + numScreenShots,
                numScreenShots >= 3);
        assertEquals("All screenshots must have different background colors",
                numScreenShots,
                screenShots.stream()
                        .mapToInt(screenShot -> screenShot.mBackgroundColor)
                        .distinct()
                        .count());
        assertEquals("All screenshots must have the same pixel count",
                1,
                screenShots.stream()
                        .mapToInt(screenShot -> screenShot.mPixels.length)
                        .distinct()
                        .count());

        long numCompletelyFixedColorPixels = 0;
        long numColorChangedAsRequestedPixels = 0;
        final int numPixels = screenShots.get(0).mPixels.length;
        for (int pixelIndex = 0; pixelIndex < numPixels; ++pixelIndex) {
            final int i = pixelIndex;
            final long numFoundColors = screenShots.stream()
                    .mapToInt(screenShot -> screenShot.mPixels[i])
                    .distinct()
                    .count();
            if (numFoundColors == 1) {
                numCompletelyFixedColorPixels++;
            }
            final long matchingScore =  screenShots.stream()
                    .filter(screenShot -> screenShot.mPixels[i] == screenShot.mBackgroundColor)
                    .count();
            if (matchingScore == numScreenShots) {
                numColorChangedAsRequestedPixels++;
            }
        }
        final String assertionMessage = "numPixels=" + numPixels
                + " numColorChangedAsRequestedPixels=" + numColorChangedAsRequestedPixels
                + " numCompletelyFixedColorPixels=" + numCompletelyFixedColorPixels;

        // OK, even 1 pixel is enough.
        if (numColorChangedAsRequestedPixels > 0) {
            return new Result(ResultType.SUPPORTED, assertionMessage);
        }

        return new Result(ResultType.NOT_SUPPORTED, assertionMessage);
    }
}
