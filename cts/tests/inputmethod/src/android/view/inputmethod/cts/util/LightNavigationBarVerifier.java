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

import android.graphics.Bitmap;
import android.graphics.Color;
import androidx.annotation.ColorInt;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import android.util.SparseIntArray;

import java.util.Arrays;
import java.util.OptionalDouble;
import java.util.function.Supplier;

/**
 * A utility class to evaluate if {@link android.view.View#SYSTEM_UI_FLAG_LIGHT_NAVIGATION_BAR} is
 * supported on the device or not.
 */
public class LightNavigationBarVerifier {

    /** This value actually does not have strong rationale. */
    private static final float LIGHT_NAVBAR_SUPPORTED_THRESHOLD = 20.0f;

    /** This value actually does not have strong rationale. */
    private static final float LIGHT_NAVBAR_NOT_SUPPORTED_THRESHOLD = 5.0f;

    @FunctionalInterface
    public interface ScreenshotSupplier {
        @NonNull
        Bitmap takeScreenshot(@ColorInt int navigationBarColor, boolean lightMode) throws Exception;
    }

    public enum ResultType {
        /**
         * {@link android.view.View#SYSTEM_UI_FLAG_LIGHT_NAVIGATION_BAR} seems to be not supported.
         */
        NOT_SUPPORTED,
        /**
         * {@link android.view.View#SYSTEM_UI_FLAG_LIGHT_NAVIGATION_BAR} seems to be supported.
         */
        SUPPORTED,
        /**
         * Not sure if {@link android.view.View#SYSTEM_UI_FLAG_LIGHT_NAVIGATION_BAR} is supported.
         */
        UNKNOWN,
    }

    static final class Result {
        @NonNull
        public final ResultType mResult;
        @NonNull
        public final Supplier<String> mAssertionMessageProvider;

        Result(@NonNull ResultType result,
                @Nullable Supplier<String> assertionMessageProvider) {
            mResult = result;
            mAssertionMessageProvider = assertionMessageProvider;
        }

        @NonNull
        public ResultType getResult() {
            return mResult;
        }

        @NonNull
        public String getAssertionMessage() {
            return mAssertionMessageProvider.get();
        }
    }

    /**
     * Asserts that {@link android.view.View#SYSTEM_UI_FLAG_LIGHT_NAVIGATION_BAR} is supported on
     * this device.
     *
     * @param screenshotSupplier callback to provide {@link Bitmap} of the navigation bar region
     */
    public static void expectLightNavigationBarSupported(
            @NonNull ScreenshotSupplier screenshotSupplier) throws Exception {
        final Result result = verify(screenshotSupplier);
        assertEquals(result.getAssertionMessage(), ResultType.SUPPORTED, result.getResult());
    }

    /**
     * Asserts that {@link android.view.View#SYSTEM_UI_FLAG_LIGHT_NAVIGATION_BAR} is not supported
     * on this device.
     *
     * @param screenshotSupplier callback to provide {@link Bitmap} of the navigation bar region
     */
    public static void expectLightNavigationBarNotSupported(
            @NonNull ScreenshotSupplier screenshotSupplier) throws Exception {
        final Result result = verify(screenshotSupplier);
        assertEquals(result.getAssertionMessage(), ResultType.NOT_SUPPORTED, result.getResult());
    }

    @FunctionalInterface
    private interface ColorOperator {
        int operate(@ColorInt int color1, @ColorInt int color2);
    }

    private static int[] operateColorArrays(@NonNull int[] pixels1, @NonNull int[] pixels2,
            @NonNull ColorOperator operator) {
        assertEquals(pixels1.length, pixels2.length);
        final int numPixels = pixels1.length;
        final int[] result = new int[numPixels];
        for (int i = 0; i < numPixels; ++i) {
            result[i] = operator.operate(pixels1[i], pixels2[i]);
        }
        return result;
    }

    @NonNull
    private static int[] getPixels(@NonNull Bitmap bitmap) {
        final int width = bitmap.getWidth();
        final int height = bitmap.getHeight();
        final int[] pixels = new int[width * height];
        bitmap.getPixels(pixels, 0 /* offset */, width /* stride */, 0 /* x */, 0 /* y */,
                width, height);
        return pixels;
    }

    @NonNull
    static Result verify(
            @NonNull ScreenshotSupplier screenshotSupplier) throws Exception {
        final int[] darkNavBarPixels = getPixels(
                screenshotSupplier.takeScreenshot(Color.BLACK, false));
        final int[] lightNavBarPixels = getPixels(
                screenshotSupplier.takeScreenshot(Color.BLACK, true));

        if (darkNavBarPixels.length != lightNavBarPixels.length) {
            throw new IllegalStateException("Pixel count mismatch."
                    + " dark=" + darkNavBarPixels.length + " light=" + lightNavBarPixels.length);
        }

        final int[][] channelDiffs = new int[][] {
                operateColorArrays(darkNavBarPixels, lightNavBarPixels,
                        (dark, light) -> Color.red(dark) - Color.red(light)),
                operateColorArrays(darkNavBarPixels, lightNavBarPixels,
                        (dark, light) -> Color.green(dark) - Color.green(light)),
                operateColorArrays(darkNavBarPixels, lightNavBarPixels,
                        (dark, light) -> Color.blue(dark) - Color.blue(light)),
        };

        if (Arrays.stream(channelDiffs).allMatch(
                diffs -> Arrays.stream(diffs).allMatch(diff -> diff == 0))) {
            // Exactly the same image.  Safe to conclude that light navigation bar is not supported.
            return new Result(ResultType.NOT_SUPPORTED, () -> dumpDiffStreams(channelDiffs));
        }

        if (Arrays.stream(channelDiffs).anyMatch(diffs -> {
            final OptionalDouble average = Arrays.stream(diffs).filter(diff -> diff != 0).average();
            return average.isPresent() && average.getAsDouble() > LIGHT_NAVBAR_SUPPORTED_THRESHOLD;
        })) {
            // If darkNavBarPixels have brighter pixels in at least one color channel
            // (red, green, blue), we consider that it is because light navigation bar takes effect.
            // Keep in mind that with SYSTEM_UI_FLAG_LIGHT_NAVIGATION_BAR navigation bar buttons
            // are expected to become darker.  So if everything works fine, darkNavBarPixels should
            // have brighter pixels.
            return new Result(ResultType.SUPPORTED, () -> dumpDiffStreams(channelDiffs));
        }

        if (Arrays.stream(channelDiffs).allMatch(diffs -> {
            final OptionalDouble average = Arrays.stream(diffs).filter(diff -> diff != 0).average();
            return average.isPresent()
                    && Math.abs(average.getAsDouble()) < LIGHT_NAVBAR_NOT_SUPPORTED_THRESHOLD;
        })) {
            // If all color channels (red, green, blue) have diffs less than a certain threshold,
            // consider light navigation bar is not supported.  For instance, some devices may
            // intentionally add some fluctuations to the navigation bar button colors/positions.
            return new Result(ResultType.NOT_SUPPORTED, () -> dumpDiffStreams(channelDiffs));
        }

        return new Result(ResultType.UNKNOWN, () -> dumpDiffStreams(channelDiffs));
    }

    @NonNull
    private static String dumpDiffStreams(@NonNull int[][] diffStreams) {
        final String[] channelNames = {"red", "green", "blue"};
        final StringBuilder sb = new StringBuilder();
        sb.append("diff histogram: ");
        for (int i = 0; i < diffStreams.length; ++i) {
            if (i != 0) {
                sb.append(", ");
            }
            final int[] channel = diffStreams[i];
            final SparseIntArray histogram = new SparseIntArray();
            Arrays.stream(channel).sorted().forEachOrdered(
                    diff -> histogram.put(diff, histogram.get(diff, 0) + 1));
            sb.append(channelNames[i]).append(": ").append(histogram);
        }
        return sb.toString();
    }
}
