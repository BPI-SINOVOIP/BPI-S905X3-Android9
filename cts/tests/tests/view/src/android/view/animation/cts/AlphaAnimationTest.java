/*
 * Copyright (C) 2008 The Android Open Source Project
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

package android.view.animation.cts;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;

import android.content.Context;
import android.content.res.XmlResourceParser;
import android.support.test.InstrumentationRegistry;
import android.support.test.filters.SmallTest;
import android.support.test.runner.AndroidJUnit4;
import android.util.AttributeSet;
import android.util.Xml;
import android.view.animation.AlphaAnimation;
import android.view.animation.Transformation;
import android.view.cts.R;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;

/**
 * Test {@link AlphaAnimation}.
 */
@SmallTest
@RunWith(AndroidJUnit4.class)
public class AlphaAnimationTest {
    private Context mContext;

    @Before
    public void setup() {
        mContext = InstrumentationRegistry.getTargetContext();
    }

    @Test
    public void testConstructor() {
        XmlResourceParser parser = mContext.getResources().getAnimation(R.anim.alpha);
        AttributeSet attrs = Xml.asAttributeSet(parser);
        new AlphaAnimation(mContext, attrs);

        new AlphaAnimation(0.0f, 1.0f);
    }

    @Test
    public void testWillChangeBounds() {
        AlphaAnimation animation = new AlphaAnimation(mContext, null);
        assertFalse(animation.willChangeBounds());
    }

    @Test
    public void testWillChangeTransformationMatrix() {
        AlphaAnimation animation = new AlphaAnimation(0.0f, 0.5f);
        assertFalse(animation.willChangeTransformationMatrix());
    }

    @Test
    public void testApplyTransformation() {
        MyAlphaAnimation animation = new MyAlphaAnimation(0.0f, 1.0f);
        Transformation transformation = new Transformation();
        assertEquals(1.0f, transformation.getAlpha(), 0.0001f);

        animation.applyTransformation(0.0f, transformation);
        assertEquals(0.0f, transformation.getAlpha(), 0.0001f);

        animation.applyTransformation(0.5f, transformation);
        assertEquals(0.5f, transformation.getAlpha(), 0.0001f);

        animation.applyTransformation(1.0f, transformation);
        assertEquals(1.0f, transformation.getAlpha(), 0.0001f);

        animation = new MyAlphaAnimation(0.2f, 0.9f);
        transformation = new Transformation();
        assertEquals(1.0f, transformation.getAlpha(), 0.0001f);

        animation.applyTransformation(0.0f, transformation);
        assertEquals(0.2f, transformation.getAlpha(), 0.0001f);

        animation.applyTransformation(0.5f, transformation);
        assertEquals(0.55f, transformation.getAlpha(), 0.0001f);

        animation.applyTransformation(1.0f, transformation);
        assertEquals(0.9f, transformation.getAlpha(), 0.0001f);
    }

    private final class MyAlphaAnimation extends AlphaAnimation {
        public MyAlphaAnimation(float fromAlpha, float toAlpha) {
            super(fromAlpha, toAlpha);
        }

        @Override
        protected void applyTransformation(float interpolatedTime, Transformation t) {
            super.applyTransformation(interpolatedTime, t);
        }
    }
}
