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

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Point;
import android.support.test.filters.MediumTest;
import android.support.test.runner.AndroidJUnit4;
import android.uirendering.cts.R;
import android.uirendering.cts.bitmapverifiers.ColorVerifier;
import android.uirendering.cts.testinfrastructure.ActivityTestBase;
import android.uirendering.cts.testinfrastructure.ViewInitializer;
import android.view.View;
import android.view.ViewGroup;
import android.widget.FrameLayout;
import org.junit.Test;
import org.junit.runner.RunWith;

@MediumTest
@RunWith(AndroidJUnit4.class)
public class AlphaBlendTest extends ActivityTestBase {

    class ViewWithAlpha extends View {
        public ViewWithAlpha(Context context) {
            super(context);
        }

        @Override
        protected void onDraw(Canvas canvas) {
            canvas.drawColor(Color.RED);
            canvas.drawColor(Color.BLUE);
        }

        @Override
        public boolean hasOverlappingRendering() {
            return false;
        }
    }

    /*
     * The following test verifies that a RED and BLUE paints on a non-overlapping view with a 0.5f
     * alpha blends correctly with a BLACK parent (without using an offscreen surface).
     */
    @Test
    public void testBlendAlphaNonOverlappingView() {

        ViewInitializer initializer = new ViewInitializer() {

            @Override
            public void initializeView(View view) {
                FrameLayout root = (FrameLayout) view.findViewById(R.id.frame_layout);
                root.setBackgroundColor(Color.BLACK);

                final ViewWithAlpha child = new ViewWithAlpha(view.getContext());

                child.setLayoutParams(new FrameLayout.LayoutParams(
                        ViewGroup.LayoutParams.FILL_PARENT,  ViewGroup.LayoutParams.FILL_PARENT));
                child.setAlpha(0.5f);
                root.addView(child);
            }
        };

        createTest()
            .addLayout(R.layout.frame_layout, initializer, true)
            .runWithVerifier(new ColorVerifier(0xff40007f));
    }
}

