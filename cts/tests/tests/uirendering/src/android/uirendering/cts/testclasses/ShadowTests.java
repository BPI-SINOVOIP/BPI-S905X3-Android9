/*
 * Copyright (C) 2015 The Android Open Source Project
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

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;

import android.content.Context;
import android.content.res.Resources;
import android.content.res.TypedArray;
import android.graphics.Color;
import android.graphics.Point;
import android.support.test.filters.MediumTest;
import android.support.test.runner.AndroidJUnit4;
import android.uirendering.cts.R;
import android.uirendering.cts.bitmapverifiers.SamplePointVerifier;
import android.uirendering.cts.testinfrastructure.ActivityTestBase;
import android.uirendering.cts.util.CompareUtils;
import android.util.TypedValue;
import android.view.ContextThemeWrapper;
import android.view.View;

import org.junit.Test;
import org.junit.runner.RunWith;

@MediumTest
@RunWith(AndroidJUnit4.class)
public class ShadowTests extends ActivityTestBase {

    private static SamplePointVerifier createVerifier(int expectedColor,
            SamplePointVerifier.VerifyPixelColor verifier) {
        return SamplePointVerifier.create(
                new Point[] {
                        // view area
                        new Point(25, 64),
                        new Point(64, 64),
                        // shadow area
                        new Point(25, 65),
                        new Point(64, 65)
                },
                new int[] {
                        Color.WHITE,
                        Color.WHITE,
                        expectedColor,
                        expectedColor,
                }, SamplePointVerifier.DEFAULT_TOLERANCE, verifier);
    }

    @Test
    public void testShadowResources() {
        final Context context = new ContextThemeWrapper(getInstrumentation().getTargetContext(),
                android.R.style.Theme_Material_Light);
        final Resources resources = context.getResources();
        final TypedValue value = new TypedValue();

        resources.getValue(R.dimen.expected_spot_shadow_alpha, value, false);
        assertEquals(TypedValue.TYPE_FLOAT, value.type);
        float expectedSpot = value.getFloat();

        resources.getValue(R.dimen.expected_ambient_shadow_alpha, value, false);
        assertEquals(TypedValue.TYPE_FLOAT, value.type);
        float expectedAmbient = value.getFloat();

        assertTrue(expectedSpot > 0);
        assertTrue(expectedAmbient > 0);

        TypedArray typedArray = context.obtainStyledAttributes(new int[] {
                android.R.attr.spotShadowAlpha,
                android.R.attr.ambientShadowAlpha,
        });

        assertEquals(expectedSpot, typedArray.getFloat(0, 0.0f), 0);
        assertEquals(expectedAmbient, typedArray.getFloat(1, 0.0f), 0);
    }

    @Test
    public void testShadowLayout() {
        // Use a higher threshold than default value (20), since we also double check gray scale;
        SamplePointVerifier verifier = createVerifier(0xffe6e6e6,
                color -> CompareUtils.verifyPixelGrayScale(color, 1));

        createTest()
                .addLayout(R.layout.simple_shadow_layout, null, true/* HW only */)
                .runWithVerifier(verifier);
    }

    @Test
    public void testRedSpotShadow() {
        SamplePointVerifier verifier = createVerifier(0xfff8e6e6,
                color -> {
                    if (color == Color.WHITE) return true;
                    return Color.red(color) > Color.green(color)
                            && Color.red(color) > Color.blue(color);
                });

        createTest()
                .addLayout(R.layout.simple_shadow_layout, view -> {
                    view.findViewById(R.id.shadow_view).setOutlineSpotShadowColor(Color.RED);
                }, true/* HW only */)
                .runWithVerifier(verifier);
    }

    @Test
    public void testRedAmbientShadow() {
        SamplePointVerifier verifier = createVerifier(0xffede6e6,
                color -> {
                    if (color == Color.WHITE) return true;
                    return Color.red(color) > Color.green(color)
                            && Color.red(color) > Color.blue(color);
                });

        createTest()
                .addLayout(R.layout.simple_shadow_layout, view -> {
                    view.findViewById(R.id.shadow_view).setOutlineAmbientShadowColor(Color.RED);
                }, true/* HW only */)
                .runWithVerifier(verifier);
    }

    @Test
    public void testRedAmbientBlueSpotShadow() {
        SamplePointVerifier verifier = createVerifier(0xffede6f8,
                color -> {
                    if (color == Color.WHITE) return true;
                    return Color.red(color) > Color.green(color)
                            && Color.blue(color) > Color.red(color);
                });

        createTest()
                .addLayout(R.layout.simple_shadow_layout, view -> {
                    View shadow = view.findViewById(R.id.shadow_view);
                    shadow.setOutlineAmbientShadowColor(Color.RED);
                    shadow.setOutlineSpotShadowColor(Color.BLUE);
                }, true/* HW only */)
                .runWithVerifier(verifier);
    }
}
