/*
 * Copyright (C) 2014 The Android Open Source Project
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

import android.graphics.Color;
import android.graphics.Rect;
import android.support.test.filters.MediumTest;
import android.support.test.runner.AndroidJUnit4;
import android.uirendering.cts.R;
import android.uirendering.cts.bitmapverifiers.ColorVerifier;
import android.uirendering.cts.bitmapverifiers.RectVerifier;
import android.uirendering.cts.testinfrastructure.ActivityTestBase;

import org.junit.Test;
import org.junit.runner.RunWith;

@MediumTest
@RunWith(AndroidJUnit4.class)
public class LayoutTests extends ActivityTestBase {
    @Test
    public void testSimpleRedLayout() {
        createTest().addLayout(R.layout.simple_red_layout, null, false).runWithVerifier(
                new ColorVerifier(Color.RED));
    }

    @Test
    public void testSimpleRectLayout() {
        createTest().addLayout(R.layout.simple_rect_layout, null, false).runWithVerifier(
                new RectVerifier(Color.WHITE, Color.BLUE, new Rect(5, 5, 85, 85)));
    }
}

