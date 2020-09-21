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
package android.graphics.cts;

import static org.junit.Assert.assertEquals;

import android.graphics.Bitmap;
import android.graphics.Bitmap.Config;
import android.graphics.BlurMaskFilter;
import android.graphics.BlurMaskFilter.Blur;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.support.test.filters.SmallTest;
import android.support.test.runner.AndroidJUnit4;

import org.junit.Test;
import org.junit.runner.RunWith;

@SmallTest
@RunWith(AndroidJUnit4.class)
public class BlurMaskFilterTest {
    private static final int OFFSET = 10;
    private static final int RADIUS = 5;
    private static final int CHECK_RADIUS = 8;
    private static final int BITMAP_WIDTH = 100;
    private static final int BITMAP_HEIGHT = 100;
    private static final int CENTER = BITMAP_HEIGHT / 2;

    @Test
    public void testBlurMaskFilter() {
        BlurMaskFilter filter = new BlurMaskFilter(RADIUS, Blur.NORMAL);
        Paint paint = new Paint();
        paint.setMaskFilter(filter);
        paint.setColor(Color.RED);
        Bitmap b = Bitmap.createBitmap(BITMAP_WIDTH, BITMAP_HEIGHT, Config.ARGB_8888);
        b.eraseColor(Color.TRANSPARENT);
        Canvas canvas = new Canvas(b);
        canvas.drawRect(CENTER - OFFSET, CENTER - OFFSET, CENTER + OFFSET, CENTER + OFFSET, paint);
        for (int x = 0; x < CENTER; x++) {
            for (int y = 0; y < CENTER; y++) {
                if (x < CENTER - OFFSET - CHECK_RADIUS || y < CENTER - OFFSET - CHECK_RADIUS) {
                    // check that color didn't bleed (much) beyond radius
                    verifyQuadrants(Color.TRANSPARENT, b, x, y, 5);
                } else if (x > CENTER - OFFSET + RADIUS && y > CENTER - OFFSET + RADIUS) {
                    // check that color didn't wash out (much) in the center
                    verifyQuadrants(Color.RED, b, x, y, 8);
                } else if (x > CENTER - OFFSET - RADIUS && y > CENTER - OFFSET - RADIUS) {
                    // check blur zone, color should remain, alpha varies
                    verifyQuadrants(Color.RED, b, x, y, 255);
                }
            }
        }
    }

    private void verifyQuadrants(int color, Bitmap bitmap, int x, int y, int alphaTolerance) {
        int right = bitmap.getWidth() - 1;
        int bottom = bitmap.getHeight() - 1;

        verifyColor(color, bitmap.getPixel(x, y), alphaTolerance);
        verifyColor(color, bitmap.getPixel(right - x, y), alphaTolerance);
        verifyColor(color, bitmap.getPixel(x, bottom - y), alphaTolerance);
        verifyColor(color, bitmap.getPixel(right - x, bottom - y), alphaTolerance);
    }

    private void verifyColor(int expected, int actual, int alphaTolerance) {
        assertEquals(Color.red(expected), Color.red(actual));
        assertEquals(Color.green(expected), Color.green(actual));
        assertEquals(Color.blue(expected), Color.blue(actual));
        assertEquals(Color.alpha(expected), Color.alpha(actual), alphaTolerance);
    }
}
