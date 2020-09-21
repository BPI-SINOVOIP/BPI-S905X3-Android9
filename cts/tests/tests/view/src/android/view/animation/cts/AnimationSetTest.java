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
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertSame;
import static org.junit.Assert.assertTrue;

import android.app.Activity;
import android.app.Instrumentation;
import android.content.res.XmlResourceParser;
import android.support.test.InstrumentationRegistry;
import android.support.test.filters.MediumTest;
import android.support.test.rule.ActivityTestRule;
import android.support.test.runner.AndroidJUnit4;
import android.util.AttributeSet;
import android.util.Xml;
import android.view.View;
import android.view.animation.AccelerateInterpolator;
import android.view.animation.AlphaAnimation;
import android.view.animation.Animation;
import android.view.animation.AnimationSet;
import android.view.animation.ScaleAnimation;
import android.view.animation.Transformation;
import android.view.animation.TranslateAnimation;
import android.view.cts.R;

import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;

import java.util.List;

@MediumTest
@RunWith(AndroidJUnit4.class)
public class AnimationSetTest {
    private static final float DELTA = 0.001f;
    private static final long SHORT_CHILD_DURATION = 400;
    private static final long MEDIUM_CHILD_DURATION = 800;
    private static final long LONG_CHILD_DURATION = 1200;
    /**
     * initial size for initialize(int width, int height, int parentWidth, int parentHeight)
     */
    private static final int INITIAL_SIZE = 100;
    private static final long ANIMATIONSET_DURATION = 1000;

    private Instrumentation mInstrumentation;
    private Activity mActivity;

    @Rule
    public ActivityTestRule<AnimationTestCtsActivity> mActivityRule =
            new ActivityTestRule<>(AnimationTestCtsActivity.class);

    @Before
    public void setup() {
        mInstrumentation = InstrumentationRegistry.getInstrumentation();
        mActivity = mActivityRule.getActivity();
    }

    @Test
    public void testConstructor() {
        new AnimationSet(true);

        final XmlResourceParser parser = mActivity.getResources().getAnimation(
                R.anim.anim_set);
        final AttributeSet attr = Xml.asAttributeSet(parser);
        assertNotNull(attr);
        // Test with real AttributeSet
        new AnimationSet(mActivity, attr);
    }

    @Test
    public void testInitialize() {
        final AnimationSet animationSet = createAnimationSet();
        animationSet.setDuration(ANIMATIONSET_DURATION);
        // Before initialize, the durations are original.
        List<Animation> children = animationSet.getAnimations();
        assertEquals(SHORT_CHILD_DURATION, children.get(0).getDuration());
        assertEquals(MEDIUM_CHILD_DURATION, children.get(1).getDuration());
        assertEquals(LONG_CHILD_DURATION, children.get(2).getDuration());

        // After initialize, AnimationSet override the child values.
        assertFalse(animationSet.isInitialized());
        animationSet.initialize(INITIAL_SIZE, INITIAL_SIZE, INITIAL_SIZE, INITIAL_SIZE);
        assertTrue(animationSet.isInitialized());
        children = animationSet.getAnimations();
        assertEquals(ANIMATIONSET_DURATION, children.get(0).getDuration());
        assertEquals(ANIMATIONSET_DURATION, children.get(1).getDuration());
        assertEquals(ANIMATIONSET_DURATION, children.get(2).getDuration());
    }

    private AnimationSet createAnimationSet() {
        AnimationSet animationSet = new AnimationSet(true);

        Animation animation1 = new AlphaAnimation(0.0f, 1.0f);
        animation1.setDuration(SHORT_CHILD_DURATION);
        animationSet.addAnimation(animation1);

        Animation animation2 = new ScaleAnimation(1.0f, 2.0f, 1.0f, 3.0f);
        animation2.setDuration(MEDIUM_CHILD_DURATION);
        animationSet.addAnimation(animation2);

        Animation animation3 = new TranslateAnimation(0.0f, 50.0f, 0.0f, 5.0f);
        animation3.setDuration(LONG_CHILD_DURATION);
        animationSet.addAnimation(animation3);

        return animationSet;
    }

    @Test
    public void testSetFillAfter() {
        final AnimationSet animationSet = createAnimationSet();
        assertFalse(animationSet.getFillAfter());

        List<Animation> children = animationSet.getAnimations();
        children.get(0).setFillAfter(true);
        children.get(1).setFillAfter(false);

        animationSet.setFillAfter(true);
        animationSet.initialize(INITIAL_SIZE, INITIAL_SIZE, INITIAL_SIZE, INITIAL_SIZE);
        assertTrue(animationSet.getFillAfter());
        children = animationSet.getAnimations();
        for (int i = 0; i < children.size(); i++) {
            assertTrue(children.get(i).getFillAfter());
        }
    }

    @Test
    public void testSetFillBefore() {
        final AnimationSet animationSet = createAnimationSet();
        assertTrue(animationSet.getFillBefore());

        List<Animation> children = animationSet.getAnimations();
        children.get(0).setFillBefore(true);
        children.get(1).setFillBefore(false);

        animationSet.setFillBefore(false);
        animationSet.initialize(INITIAL_SIZE, INITIAL_SIZE, INITIAL_SIZE, INITIAL_SIZE);
        assertFalse(animationSet.getFillBefore());
        children = animationSet.getAnimations();
        for (int i = 0; i < children.size(); i++) {
            assertFalse(children.get(i).getFillBefore());
        }
    }

    @Test
    public void testAccessDuration() {
        final AnimationSet animationSet = createAnimationSet();
        assertEquals(LONG_CHILD_DURATION, animationSet.getDuration());

        assertTrue(animationSet.getDuration() > ANIMATIONSET_DURATION);
        animationSet.setDuration(ANIMATIONSET_DURATION);
        animationSet.initialize(INITIAL_SIZE, INITIAL_SIZE, INITIAL_SIZE, INITIAL_SIZE);
        assertEquals(ANIMATIONSET_DURATION, animationSet.getDuration());
        final List<Animation> children = animationSet.getAnimations();
        for (int i = 0; i < children.size(); i++) {
            assertEquals(ANIMATIONSET_DURATION, children.get(i).getDuration());
        }
    }

    @Test
    public void testRestrictDuration() {
        final AnimationSet animationSet = new AnimationSet(false);
        Animation child = null;
        final long[] originChildDuration = { 1000, 1000, 500 };
        final long[] originChildStartOffset = { 2000, 1000, 0 };
        final int[] originChildRepeatCount = { 0, 0, 4 };
        final long[] originChildDurationHint = new long[3];
        for (int i = 0; i < 3; i++) {
            child = new AlphaAnimation(0.0f, 1.0f);
            child.setDuration(originChildDuration[i]);
            child.setStartOffset(originChildStartOffset[i]);
            child.setRepeatCount(originChildRepeatCount[i]);
            originChildDurationHint[i] = child.computeDurationHint();
            animationSet.addAnimation(child);
        }
        final long restrictDuration = 1500;
        animationSet.restrictDuration(restrictDuration);
        final List<Animation> children = animationSet.getAnimations();

        assertTrue(originChildStartOffset[0] > restrictDuration);
        assertEquals(0, children.get(0).getDuration());
        assertEquals(restrictDuration, children.get(0).getStartOffset());

        assertTrue(originChildStartOffset[1] < restrictDuration);
        assertTrue(originChildDurationHint[1] > restrictDuration);
        assertTrue(children.get(1).computeDurationHint() <= restrictDuration);

        assertTrue(originChildDurationHint[2] > restrictDuration);
        assertTrue(children.get(2).computeDurationHint() <= restrictDuration);
        assertTrue(originChildRepeatCount[2] > children.get(2).getRepeatCount());
    }

    @Test
    public void testComputeDurationHint() {
        final AnimationSet animationSet = createAnimationSet();
        final List<Animation> children = animationSet.getAnimations();
        long expectedDuration = 0;
        for (int i = 0; i < children.size(); i++) {
            expectedDuration = Math.max(expectedDuration, children.get(i).computeDurationHint());
        }
        assertEquals(expectedDuration, animationSet.computeDurationHint());
    }

    @Test
    public void testScaleCurrentDuration() {
        final AnimationSet animationSet = createAnimationSet();
        List<Animation> children = animationSet.getAnimations();
        final long[] originDurations = new long[children.size()];
        for (int i = 0; i < children.size(); i++) {
            originDurations[i] = children.get(i).getDuration();
        }

        final float scaleFactor = 2.0f;
        animationSet.scaleCurrentDuration(scaleFactor);
        children = animationSet.getAnimations();
        for (int i = 0; i < children.size(); i++) {
            assertEquals((long) (originDurations[i] * scaleFactor), children.get(i).getDuration());
        }
    }

    @Test
    public void testAccessRepeatMode() {
        final AnimationSet animationSet = createAnimationSet();
        animationSet.setRepeatMode(Animation.RESTART);
        animationSet.initialize(INITIAL_SIZE, INITIAL_SIZE, INITIAL_SIZE, INITIAL_SIZE);
        assertEquals(Animation.RESTART, animationSet.getRepeatMode());
        List<Animation> children = animationSet.getAnimations();
        for (int i = 0; i < children.size(); i++) {
            assertEquals(Animation.RESTART, children.get(i).getRepeatMode());
        }

        animationSet.setRepeatMode(Animation.REVERSE);
        animationSet.initialize(INITIAL_SIZE, INITIAL_SIZE, INITIAL_SIZE, INITIAL_SIZE);
        assertEquals(Animation.REVERSE, animationSet.getRepeatMode());
        children = animationSet.getAnimations();
        for (int i = 0; i < children.size(); i++) {
            assertEquals(Animation.REVERSE, children.get(i).getRepeatMode());
        }
    }

    @Test
    public void testAccessStartOffset() {
        final AnimationSet animationSet = createAnimationSet();
        assertEquals(0, animationSet.getStartOffset());
        List<Animation> children = animationSet.getAnimations();
        final long[] originStartOffset = new long[children.size()];
        for (int i = 0; i < children.size(); i++) {
            originStartOffset[i] = children.get(i).getStartOffset();
        }

        animationSet.setStartOffset(100);
        animationSet.initialize(INITIAL_SIZE, INITIAL_SIZE, INITIAL_SIZE, INITIAL_SIZE);
        assertEquals(100, animationSet.getStartOffset());
        children = animationSet.getAnimations();
        for (int i = 0; i < children.size(); i++) {
            assertEquals(originStartOffset[i] + animationSet.getStartOffset(),
                    children.get(i).getStartOffset());
        }

        assertTrue(animationSet.isInitialized());
        animationSet.reset();
        assertFalse(animationSet.isInitialized());
        children = animationSet.getAnimations();
        for (int i = 0; i < children.size(); i++) {
            assertEquals(originStartOffset[i], children.get(i).getStartOffset());
        }
    }

    @Test
    public void testAccessStartTime() {
        final AnimationSet animationSet = createAnimationSet();
        final long[] originChildStartTime = {1000, 2000, 3000};
        List<Animation> children = animationSet.getAnimations();
        for (int i = 0; i < children.size(); i++) {
            children.get(i).setStartTime(originChildStartTime[i]);
        }

        // Get earliest start time of children animations
        assertEquals(1000, animationSet.getStartTime());

        final long startTime = 200;
        animationSet.setStartTime(startTime);
        assertEquals(startTime, animationSet.getStartTime());

        children = animationSet.getAnimations();
        for (int i = 0; i < children.size(); i++) {
            assertEquals(startTime, children.get(i).getStartTime());
        }
    }

    @Test
    public void testGetTransformation() throws Throwable {
        final View animWindowParent = mActivity.findViewById(R.id.anim_window_parent);
        final View animWindow = mActivity.findViewById(R.id.anim_window);
        final AnimationSet animationSet = createAnimationSet();
        animationSet.setDuration(2000);
        animationSet.initialize(animWindow.getWidth(), animWindow.getHeight(),
                animWindowParent.getWidth(), animWindowParent.getHeight());

        AnimationTestUtils.assertRunAnimation(mInstrumentation, mActivityRule, animWindow,
                animationSet);
        final long startTime = animationSet.getStartTime();

        assertGetTransformation(animationSet, startTime, true);
        assertGetTransformation(animationSet, startTime + 100, true);
        assertGetTransformation(animationSet, startTime + animationSet.getDuration(), false);
    }

    private void assertGetTransformation(final AnimationSet animationSet,
            final long currentTime, final boolean result) {
        final Transformation transformation = new Transformation();
        final Transformation expectedTransformation = new Transformation();
        final Transformation tempTransformation = new Transformation();

        assertEquals(result, animationSet.getTransformation(currentTime, transformation));
        final List<Animation> children = animationSet.getAnimations();
        for (int i = children.size() - 1; i >= 0; i--) {
            tempTransformation.clear();
            children.get(i).getTransformation(currentTime, tempTransformation);
            expectedTransformation.compose(tempTransformation);
        }
        assertTransformationEquals(expectedTransformation, transformation);
    }

    private void assertTransformationEquals(final Transformation expected,
            final Transformation actual) {
        assertEquals(expected.getAlpha(), actual.getAlpha(), DELTA);
        final float[] expectedValues = new float[9];
        final float[] actualValues = new float[9];
        expected.getMatrix().getValues(expectedValues);
        actual.getMatrix().getValues(actualValues);
        for (int i = 0; i < expectedValues.length; i++) {
            assertEquals(expectedValues[i], actualValues[i], DELTA);
        }
    }

    @Test
    public void testAccessAnimations() {
        final AnimationSet animationSet = new AnimationSet(true);
        final Animation animation1 = new AlphaAnimation(0.0f, 1.0f);
        animationSet.addAnimation(animation1);
        final Animation animation2 = new AlphaAnimation(0.5f, 1.0f);
        animationSet.addAnimation(animation2);
        final Animation animation3 = new AlphaAnimation(1.0f, 0.5f);
        animationSet.addAnimation(animation3);

        final List<Animation> children = animationSet.getAnimations();
        assertEquals(3, children.size());
        assertSame(animation1, children.get(0));
        assertSame(animation2, children.get(1));
        assertSame(animation3, children.get(2));
    }

    @Test
    public void testWillChangeTransformationMatrix() {
        final AnimationSet animationSet = new AnimationSet(true);
        assertFalse(animationSet.willChangeTransformationMatrix());

        // Add first animation, this is an alpha animation and will not change
        // the transformation matrix.
        animationSet.addAnimation(new AlphaAnimation(0.0f, 1.0f));
        assertFalse(animationSet.willChangeTransformationMatrix());
        assertFalse(animationSet.willChangeBounds());

        // Add second animation, this is an scale animation and will change
        // the transformation matrix.
        animationSet.addAnimation(new ScaleAnimation(1.0f, 2.0f, 1.0f, 2.0f));
        assertTrue(animationSet.willChangeTransformationMatrix());
        assertTrue(animationSet.willChangeBounds());
    }

    @Test
    public void testClone() throws CloneNotSupportedException {
        final MyAnimationSet animationSet = new MyAnimationSet(false);
        final Animation alpha = new AlphaAnimation(0.0f, 1.0f);
        alpha.setInterpolator(new AccelerateInterpolator());
        alpha.initialize(INITIAL_SIZE, INITIAL_SIZE, INITIAL_SIZE, INITIAL_SIZE);
        final Animation scale = new ScaleAnimation(1.0f, 2.0f, 1.0f, 3.0f);
        scale.initialize(INITIAL_SIZE, INITIAL_SIZE, INITIAL_SIZE, INITIAL_SIZE);
        animationSet.addAnimation(alpha);
        animationSet.addAnimation(scale);
        final long startTime = 0;
        animationSet.setStartTime(startTime);
        animationSet.setDuration(ANIMATIONSET_DURATION);
        animationSet.initialize(INITIAL_SIZE, INITIAL_SIZE, INITIAL_SIZE, INITIAL_SIZE);

        final AnimationSet clone = animationSet.clone();
        clone.initialize(INITIAL_SIZE, INITIAL_SIZE, INITIAL_SIZE, INITIAL_SIZE);
        final List<Animation> children = animationSet.getAnimations();
        final List<Animation> cloneChildren = clone.getAnimations();
        assertEquals(children.size(), cloneChildren.size());
        final Transformation expectedTransformation = new Transformation();
        final Transformation transformation = new Transformation();
        for (int i = 0; i < children.size(); i++) {
            children.get(i).getTransformation(startTime, expectedTransformation);
            cloneChildren.get(i).getTransformation(startTime, transformation);
            assertTransformationEquals(expectedTransformation, transformation);

            children.get(i).getTransformation(startTime + ANIMATIONSET_DURATION / 2,
                    expectedTransformation);
            cloneChildren.get(i).getTransformation(startTime  + ANIMATIONSET_DURATION /2,
                    transformation);
            assertTransformationEquals(expectedTransformation, transformation);

            children.get(i).getTransformation(startTime + ANIMATIONSET_DURATION,
                    expectedTransformation);
            cloneChildren.get(i).getTransformation(startTime  + ANIMATIONSET_DURATION,
                    transformation);
            assertTransformationEquals(expectedTransformation, transformation);
        }
    }

    private static class MyAnimationSet extends AnimationSet {

        public MyAnimationSet(boolean shareInterpolator) {
            super(shareInterpolator);
        }

        @Override
        protected AnimationSet clone() throws CloneNotSupportedException {
            return super.clone();
        }
    }
}
