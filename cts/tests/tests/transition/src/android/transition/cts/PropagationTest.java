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
package android.transition.cts;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;

import android.graphics.Rect;
import android.support.test.filters.MediumTest;
import android.support.test.runner.AndroidJUnit4;
import android.transition.CircularPropagation;
import android.transition.SidePropagation;
import android.transition.Transition;
import android.transition.TransitionValues;
import android.view.Gravity;
import android.view.View;

import org.junit.Test;
import org.junit.runner.RunWith;

@MediumTest
@RunWith(AndroidJUnit4.class)
public class PropagationTest extends BaseTransitionTest {
    @Test
    public void testCircularPropagation() throws Throwable {
        enterScene(R.layout.scene10);
        CircularPropagation propagation = new CircularPropagation();
        mTransition.setPropagation(propagation);
        final TransitionValues redValues = new TransitionValues();
        redValues.view = mActivity.findViewById(R.id.redSquare);
        propagation.captureValues(redValues);

        // Only the reported propagation properties are set
        for (String prop : propagation.getPropagationProperties()) {
            assertTrue(redValues.values.keySet().contains(prop));
        }
        assertEquals(propagation.getPropagationProperties().length, redValues.values.size());

        // check the visibility
        assertEquals(View.VISIBLE, propagation.getViewVisibility(redValues));
        assertEquals(View.GONE, propagation.getViewVisibility(null));

        // Check the positions
        int[] pos = new int[2];
        redValues.view.getLocationOnScreen(pos);
        pos[0] += redValues.view.getWidth() / 2;
        pos[1] += redValues.view.getHeight() / 2;
        assertEquals(pos[0], propagation.getViewX(redValues));
        assertEquals(pos[1], propagation.getViewY(redValues));

        mTransition.setEpicenterCallback(new Transition.EpicenterCallback() {
            @Override
            public Rect onGetEpicenter(Transition transition) {
                return new Rect(0, 0, redValues.view.getWidth(), redValues.view.getHeight());
            }
        });

        long redDelay = getDelay(R.id.redSquare);
        // red square's delay should be roughly 0 since it is at the epicenter
        assertEquals(0f, redDelay, 30f);

        // The green square is on the upper-right
        long greenDelay = getDelay(R.id.greenSquare);
        assertTrue(greenDelay < redDelay);

        // The blue square is on the lower-right
        long blueDelay = getDelay(R.id.blueSquare);
        assertTrue(blueDelay < greenDelay);

        // Test propagation speed
        propagation.setPropagationSpeed(1000000000f);
        assertEquals(0, getDelay(R.id.blueSquare));
    }

    @Test
    public void testSidePropagationBottom() throws Throwable {
        SidePropagation propagation = new SidePropagation();
        propagation.setSide(Gravity.BOTTOM);
        mTransition.setEpicenterCallback(new Transition.EpicenterCallback() {
            @Override
            public Rect onGetEpicenter(Transition transition) {
                return new Rect(0, 0, 1, 1);
            }
        });
        mTransition.setPropagation(propagation);

        enterScene(R.layout.scene10);

        // The red square is on the upper-left
        long redDelay = getDelay(R.id.redSquare);

        // The green square is on the upper-right
        long greenDelay = getDelay(R.id.greenSquare);

        // The blue square is on the lower-right
        long blueDelay = getDelay(R.id.blueSquare);

        // The yellow square is on the lower-left
        long yellowDelay = getDelay(R.id.yellowSquare);

        assertTrue(redDelay > greenDelay);
        assertTrue(redDelay > yellowDelay);
        assertTrue(greenDelay > blueDelay);
        assertTrue(yellowDelay > blueDelay);

        // Test propagation speed
        propagation.setPropagationSpeed(1000000000f);
        assertEquals(0, getDelay(R.id.blueSquare));
    }

    @Test
    public void testSidePropagationTop() throws Throwable {
        SidePropagation propagation = new SidePropagation();
        propagation.setSide(Gravity.TOP);
        mTransition.setEpicenterCallback(new Transition.EpicenterCallback() {
            @Override
            public Rect onGetEpicenter(Transition transition) {
                return new Rect(0, 0, 1, 1);
            }
        });
        mTransition.setPropagation(propagation);

        enterScene(R.layout.scene10);

        // The red square is on the upper-left
        long redDelay = getDelay(R.id.redSquare);

        // The green square is on the upper-right
        long greenDelay = getDelay(R.id.greenSquare);

        // The blue square is on the lower-right
        long blueDelay = getDelay(R.id.blueSquare);

        // The yellow square is on the lower-left
        long yellowDelay = getDelay(R.id.yellowSquare);

        assertTrue(yellowDelay > redDelay);
        assertTrue(yellowDelay > blueDelay);
        assertTrue(redDelay > greenDelay);
        assertTrue(blueDelay > greenDelay);
    }

    @Test
    public void testSidePropagationRight() throws Throwable {
        SidePropagation propagation = new SidePropagation();
        propagation.setSide(Gravity.RIGHT);
        mTransition.setEpicenterCallback(new Transition.EpicenterCallback() {
            @Override
            public Rect onGetEpicenter(Transition transition) {
                return new Rect(0, 0, 1, 1);
            }
        });
        mTransition.setPropagation(propagation);

        enterScene(R.layout.scene10);

        // The red square is on the upper-left
        long redDelay = getDelay(R.id.redSquare);

        // The green square is on the upper-right
        long greenDelay = getDelay(R.id.greenSquare);

        // The blue square is on the lower-right
        long blueDelay = getDelay(R.id.blueSquare);

        // The yellow square is on the lower-left
        long yellowDelay = getDelay(R.id.yellowSquare);

        assertTrue(redDelay > greenDelay);
        assertTrue(redDelay > yellowDelay);
        assertTrue(yellowDelay > blueDelay);
        assertTrue(greenDelay > blueDelay);
    }

    @Test
    public void testSidePropagationLeft() throws Throwable {
        SidePropagation propagation = new SidePropagation();
        propagation.setSide(Gravity.LEFT);
        mTransition.setEpicenterCallback(new Transition.EpicenterCallback() {
            @Override
            public Rect onGetEpicenter(Transition transition) {
                return new Rect(0, 0, 1, 1);
            }
        });
        mTransition.setPropagation(propagation);

        enterScene(R.layout.scene10);

        // The red square is on the upper-left
        long redDelay = getDelay(R.id.redSquare);

        // The green square is on the upper-right
        long greenDelay = getDelay(R.id.greenSquare);

        // The blue square is on the lower-right
        long blueDelay = getDelay(R.id.blueSquare);

        // The yellow square is on the lower-left
        long yellowDelay = getDelay(R.id.yellowSquare);

        assertTrue(greenDelay > redDelay);
        assertTrue(greenDelay > blueDelay);
        assertTrue(redDelay > yellowDelay);
        assertTrue(blueDelay > yellowDelay);
    }

    private TransitionValues capturePropagationValues(int viewId) {
        TransitionValues transitionValues = new TransitionValues();
        transitionValues.view = mSceneRoot.findViewById(viewId);
        mTransition.getPropagation().captureValues(transitionValues);
        return transitionValues;
    }

    private long getDelay(int viewId) {
        TransitionValues transitionValues = capturePropagationValues(viewId);
        return mTransition.getPropagation().
                getStartDelay(mSceneRoot, mTransition, transitionValues, null);
    }
}
