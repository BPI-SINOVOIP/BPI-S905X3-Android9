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
package android.transition.cts;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;

import android.support.test.filters.MediumTest;
import android.support.test.runner.AndroidJUnit4;
import android.transition.ChangeScroll;
import android.transition.TransitionManager;
import android.view.View;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;

import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;

@MediumTest
@RunWith(AndroidJUnit4.class)
public class ChangeScrollTest extends BaseTransitionTest {
    ChangeScroll mChangeScroll;

    @Override
    @Before
    public void setup() {
        super.setup();
        mChangeScroll = new ChangeScroll();
        mTransition = mChangeScroll;
        resetListener();
    }

    @Test
    public void testChangeScroll() throws Throwable {
        enterScene(R.layout.scene5);
        final CountDownLatch scrollChanged = new CountDownLatch(1);
        final View.OnScrollChangeListener listener = (v, newX, newY, oldX, oldY) -> {
            if (newX != 0 && newY != 0) {
                scrollChanged.countDown();
            }
        };
        mActivityRule.runOnUiThread(() -> {
            final View view = mActivity.findViewById(R.id.text);
            assertEquals(0, view.getScrollX());
            assertEquals(0, view.getScrollY());
            TransitionManager.beginDelayedTransition(mSceneRoot, mChangeScroll);
            view.scrollTo(150, 300);
            view.setOnScrollChangeListener(listener);
        });
        waitForStart();
        assertTrue(scrollChanged.await(1, TimeUnit.SECONDS));
        mActivityRule.runOnUiThread(() -> {
            final View view = mActivity.findViewById(R.id.text);
            final int scrollX = view.getScrollX();
            final int scrollY = view.getScrollY();
            assertTrue(scrollX > 0);
            assertTrue(scrollX < 150);
            assertTrue(scrollY > 0);
            assertTrue(scrollY < 300);
        });
        waitForEnd(800);
        mActivityRule.runOnUiThread(() -> {
            final View view = mActivity.findViewById(R.id.text);
            assertEquals(150, view.getScrollX());
            assertEquals(300, view.getScrollY());
        });
    }
}

