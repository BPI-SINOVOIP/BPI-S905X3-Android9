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

package android.uirendering.cts.testclasses;

import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.ColorMatrixColorFilter;
import android.graphics.Paint;
import android.support.test.filters.MediumTest;
import android.support.test.runner.AndroidJUnit4;
import android.uirendering.cts.bitmapverifiers.ColorVerifier;
import android.uirendering.cts.testinfrastructure.ActivityTestBase;

import org.junit.Test;
import org.junit.runner.RunWith;

@MediumTest
@RunWith(AndroidJUnit4.class)
public class ColorFilterTests extends ActivityTestBase {

    @Test
    public void testColorMatrix() {
        createTest()
                .addCanvasClient((canvas, width, height) -> {
                    Bitmap whiteBitmap = Bitmap.createBitmap(width, height, Bitmap.Config.ARGB_8888);
                    whiteBitmap.eraseColor(Color.WHITE);

                    Paint paint = new Paint();
                    canvas.drawBitmap(whiteBitmap, 0, 0, paint);

                    paint.setAntiAlias(true);
                    paint.setColorFilter(new ColorMatrixColorFilter(new float[] {
                            -1, 0, 0, 0, 255,
                            0, -1, 0, 0, 255,
                            0, 0, -1, 0, 255,
                            0, 0, 0, 1, 0
                            }));
                    canvas.drawBitmap(whiteBitmap, 0, 0, paint);

                }, true)
                .runWithVerifier(new ColorVerifier(Color.BLACK));
    }

}
