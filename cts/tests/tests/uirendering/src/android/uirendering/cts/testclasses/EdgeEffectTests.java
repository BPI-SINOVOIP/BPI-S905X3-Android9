/*
 * Copyright (C) 2016 The Android Open Source Project
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

import static android.uirendering.cts.util.MockVsyncHelper.nextFrame;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;
import static org.mockito.Matchers.anyFloat;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.verify;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.support.test.InstrumentationRegistry;
import android.support.test.filters.SmallTest;
import android.support.test.runner.AndroidJUnit4;
import android.uirendering.cts.bitmapverifiers.PerPixelBitmapVerifier;
import android.uirendering.cts.testinfrastructure.Tracer;
import android.uirendering.cts.util.BitmapAsserter;
import android.uirendering.cts.util.MockVsyncHelper;
import android.view.ContextThemeWrapper;
import android.widget.EdgeEffect;

import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.ArgumentCaptor;

@SmallTest
@RunWith(AndroidJUnit4.class)
public class EdgeEffectTests {

    private static final int WIDTH = 90;
    private static final int HEIGHT = 90;

    @Rule
    public Tracer name = new Tracer();

    private BitmapAsserter mBitmapAsserter = new BitmapAsserter(this.getClass().getSimpleName(),
            name.getMethodName());

    private Context mThemeContext;

    interface EdgeEffectInitializer {
        void initialize(EdgeEffect edgeEffect);
    }

    private Context getContext() {
        return mThemeContext;
    }

    @Before
    public void setUp() {
        final Context targetContext = InstrumentationRegistry.getTargetContext();
        mThemeContext = new ContextThemeWrapper(targetContext,
                android.R.style.Theme_Material_Light);
    }

    private static class EdgeEffectValidator extends PerPixelBitmapVerifier {
        public int matchedColorCount;

        private int mInverseColorMask;
        private int mColorMask;

        public EdgeEffectValidator(int drawColor) {
            mColorMask = drawColor & 0x00FFFFFF;
            mInverseColorMask = ~(drawColor & 0x00FFFFFF);
        }

        @Override
        protected boolean verifyPixel(int x, int y, int observedColor) {
            if ((observedColor & mColorMask) != 0) {
                matchedColorCount++;
            }
            return (observedColor & mInverseColorMask) == 0xFF000000;
        }
    }

    private void assertEdgeEffect(EdgeEffectInitializer initializer) {
        Bitmap bitmap = Bitmap.createBitmap(WIDTH, HEIGHT, Bitmap.Config.ARGB_8888);
        Canvas canvas = new Canvas(bitmap);
        canvas.drawColor(Color.BLACK);
        EdgeEffect edgeEffect = new EdgeEffect(getContext());
        edgeEffect.setSize(WIDTH, HEIGHT);
        edgeEffect.setColor(Color.RED);
        assertEquals(Color.RED, edgeEffect.getColor());
        initializer.initialize(edgeEffect);
        edgeEffect.draw(canvas);

        EdgeEffectValidator verifier = new EdgeEffectValidator(edgeEffect.getColor());
        mBitmapAsserter.assertBitmapIsVerified(bitmap, verifier,
                name.getMethodName(), "EdgeEffect doesn't match expected");
        assertTrue(verifier.matchedColorCount > 0);
    }

    @Test
    public void testOnPull() {
        assertEdgeEffect(edgeEffect -> {
            edgeEffect.onPull(1);
        });
    }

    @Test
    public void testSetSize() {
        assertEdgeEffect(edgeEffect -> {
            edgeEffect.setSize(70, 70);
            edgeEffect.onPull(1);
        });
    }

    @Test
    public void testSetColor() {
        assertEdgeEffect(edgeEffect -> {
            edgeEffect.setColor(Color.GREEN);
            assertEquals(Color.GREEN, edgeEffect.getColor());
            edgeEffect.onPull(1);
        });
    }

    @Test
    public void testOnPullWithDisplacement() {
        assertEdgeEffect(edgeEffect -> {
            edgeEffect.onPull(1, 0);
        });

        assertEdgeEffect(edgeEffect -> {
            edgeEffect.onPull(1, 1);
        });
    }

    @Test
    public void testIsFinished() {
        EdgeEffect effect = new EdgeEffect(getContext());
        assertTrue(effect.isFinished());
        effect.onPull(0.5f);
        assertFalse(effect.isFinished());
    }

    @Test
    public void testFinish() {
        EdgeEffect effect = new EdgeEffect(getContext());
        effect.onPull(1);
        effect.finish();
        assertTrue(effect.isFinished());

        effect.onAbsorb(1000);
        effect.finish();
        assertFalse(effect.draw(new Canvas()));
    }

    @Test
    public void testGetColor() {
        EdgeEffect effect = new EdgeEffect(getContext());
        effect.setColor(Color.GREEN);
        assertEquals(Color.GREEN, effect.getColor());
    }

    @Test
    public void testGetMaxHeight() {
        EdgeEffect edgeEffect = new EdgeEffect(getContext());
        edgeEffect.setSize(200, 200);
        assertTrue(edgeEffect.getMaxHeight() <= 200 * 2 + 1);
        edgeEffect.setSize(200, 0);
        assertEquals(0, edgeEffect.getMaxHeight());
    }

    private interface AlphaVerifier {
        void verify(int oldAlpha, int newAlpha);
    }

    // validates changes to the alpha of draw commands produced by EdgeEffect
    // over the course of an animation
    private void verifyAlpha(EdgeEffectInitializer initializer, AlphaVerifier alphaVerifier) {
        MockVsyncHelper.runOnVsyncThread(() -> {
            Canvas canvas = mock(Canvas.class);
            ArgumentCaptor<Paint> captor = ArgumentCaptor.forClass(Paint.class);
            EdgeEffect edgeEffect = new EdgeEffect(getContext());
            edgeEffect.setSize(200, 200);
            initializer.initialize(edgeEffect);
            edgeEffect.draw(canvas);
            verify(canvas).drawCircle(anyFloat(), anyFloat(), anyFloat(), captor.capture());
            int oldAlpha = captor.getValue().getAlpha();
            for (int i = 0; i < 3; i++) {
                nextFrame();
                canvas = mock(Canvas.class);
                edgeEffect.draw(canvas);
                verify(canvas).drawCircle(anyFloat(), anyFloat(), anyFloat(), captor.capture());
                int newAlpha = captor.getValue().getAlpha();
                alphaVerifier.verify(oldAlpha, newAlpha);
                oldAlpha = newAlpha;
            }
        });
    }

    @Test
    public void testOnAbsorb() {
        verifyAlpha(edgeEffect -> {
            edgeEffect.onAbsorb(10000);
        }, ((oldAlpha, newAlpha) -> {
            assertTrue("Alpha should grow", oldAlpha < newAlpha);
        }));
    }

    @Test
    public void testOnRelease() {
        verifyAlpha(edgeEffect -> {
            edgeEffect.onPull(1);
            edgeEffect.onRelease();
        }, ((oldAlpha, newAlpha) -> {
            assertTrue("Alpha should decrease", oldAlpha > newAlpha);
        }));
    }

}
