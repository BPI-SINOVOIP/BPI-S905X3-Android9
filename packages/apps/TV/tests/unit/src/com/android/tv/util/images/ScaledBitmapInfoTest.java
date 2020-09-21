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
package com.android.tv.util.images;

import static com.google.common.truth.Truth.assertWithMessage;

import android.graphics.Bitmap;
import android.support.test.filters.SmallTest;
import android.support.test.runner.AndroidJUnit4;
import com.android.tv.util.images.BitmapUtils.ScaledBitmapInfo;
import org.junit.Test;
import org.junit.runner.RunWith;

/** Tests for {@link ScaledBitmapInfo}. */
@SmallTest
@RunWith(AndroidJUnit4.class)
public class ScaledBitmapInfoTest {
    private static final Bitmap B80x100 = Bitmap.createBitmap(80, 100, Bitmap.Config.RGB_565);
    private static final Bitmap B960x1440 = Bitmap.createBitmap(960, 1440, Bitmap.Config.RGB_565);

    @Test
    public void testSize_B100x100to50x50() {
        ScaledBitmapInfo actual = BitmapUtils.createScaledBitmapInfo("B80x100", B80x100, 50, 50);
        assertScaledBitmapSize(2, 40, 50, actual);
    }

    @Test
    public void testNeedsToReload_B100x100to50x50() {
        ScaledBitmapInfo actual = BitmapUtils.createScaledBitmapInfo("B80x100", B80x100, 50, 50);
        assertNeedsToReload(false, actual, 25, 25);
        assertNeedsToReload(false, actual, 50, 50);
        assertNeedsToReload(false, actual, 99, 99);
        assertNeedsToReload(true, actual, 100, 100);
        assertNeedsToReload(true, actual, 101, 101);
    }

    /** Reproduces <a href="http://b/20488453">b/20488453</a>. */
    @Test
    public void testBug20488453() {
        ScaledBitmapInfo actual =
                BitmapUtils.createScaledBitmapInfo("B960x1440", B960x1440, 284, 160);
        assertScaledBitmapSize(8, 107, 160, actual);
        assertNeedsToReload(false, actual, 284, 160);
    }

    private static void assertNeedsToReload(
            boolean expected, ScaledBitmapInfo scaledBitmap, int reqWidth, int reqHeight) {
    assertWithMessage(scaledBitmap.id + " needToReload(" + reqWidth + "," + reqHeight + ")")
        .that(scaledBitmap.needToReload(reqWidth, reqHeight))
        .isEqualTo(expected);
    }

    private static void assertScaledBitmapSize(
            int expectedInSampleSize,
            int expectedWidth,
            int expectedHeight,
            ScaledBitmapInfo actual) {
    assertWithMessage(actual.id + " inSampleSize")
        .that(actual.inSampleSize)
        .isEqualTo(expectedInSampleSize);
    assertWithMessage(actual.id + " width").that(actual.bitmap.getWidth()).isEqualTo(expectedWidth);
    assertWithMessage(actual.id + " height")
        .that(actual.bitmap.getHeight())
        .isEqualTo(expectedHeight);
    }
}
