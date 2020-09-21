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
 * limitations under the License
 */

package com.android.server.wm;

import static com.android.server.wm.WindowStateAnimator.STACK_CLIP_AFTER_ANIM;
import static com.android.server.wm.WindowStateAnimator.STACK_CLIP_BEFORE_ANIM;
import static com.android.server.wm.WindowStateAnimator.STACK_CLIP_NONE;

import static org.mockito.ArgumentMatchers.argThat;
import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.verify;

import android.graphics.Point;
import android.graphics.Rect;
import android.platform.test.annotations.Presubmit;
import android.support.test.filters.SmallTest;
import android.support.test.runner.AndroidJUnit4;
import android.view.SurfaceControl;
import android.view.animation.Animation;
import android.view.animation.ClipRectAnimation;

import org.junit.Test;
import org.junit.runner.RunWith;

/**
 * Tests for the {@link WindowAnimationSpec} class.
 *
 *  atest FrameworksServicesTests:com.android.server.wm.WindowAnimationSpecTest
 */
@SmallTest
@Presubmit
@RunWith(AndroidJUnit4.class)
public class WindowAnimationSpecTest {
    private final SurfaceControl mSurfaceControl = mock(SurfaceControl.class);
    private final SurfaceControl.Transaction mTransaction = mock(SurfaceControl.Transaction.class);
    private final Animation mAnimation = mock(Animation.class);
    private final Rect mStackBounds = new Rect(0, 0, 10, 10);

    @Test
    public void testApply_clipNone() {
        Rect windowCrop = new Rect(0, 0, 20, 20);
        Animation a = createClipRectAnimation(windowCrop, windowCrop);
        WindowAnimationSpec windowAnimationSpec = new WindowAnimationSpec(a, null,
                mStackBounds, false /* canSkipFirstFrame */, STACK_CLIP_NONE,
                true /* isAppAnimation */);
        windowAnimationSpec.apply(mTransaction, mSurfaceControl, 0);
        verify(mTransaction).setWindowCrop(eq(mSurfaceControl),
                argThat(rect -> rect.equals(windowCrop)));
    }

    @Test
    public void testApply_clipAfter() {
        WindowAnimationSpec windowAnimationSpec = new WindowAnimationSpec(mAnimation, null,
                mStackBounds, false /* canSkipFirstFrame */, STACK_CLIP_AFTER_ANIM,
                true /* isAppAnimation */);
        windowAnimationSpec.apply(mTransaction, mSurfaceControl, 0);
        verify(mTransaction).setWindowCrop(eq(mSurfaceControl), argThat(Rect::isEmpty));
        verify(mTransaction).setFinalCrop(eq(mSurfaceControl),
                argThat(rect -> rect.equals(mStackBounds)));
    }

    @Test
    public void testApply_clipAfterOffsetPosition() {
        // Stack bounds is (0, 0, 10, 10) position is (20, 40)
        WindowAnimationSpec windowAnimationSpec = new WindowAnimationSpec(mAnimation,
                new Point(20, 40), mStackBounds, false /* canSkipFirstFrame */,
                STACK_CLIP_AFTER_ANIM,
                true /* isAppAnimation */);
        windowAnimationSpec.apply(mTransaction, mSurfaceControl, 0);
        verify(mTransaction).setWindowCrop(eq(mSurfaceControl), argThat(Rect::isEmpty));
        verify(mTransaction).setFinalCrop(eq(mSurfaceControl),
                argThat(rect -> rect.left == 20 && rect.top == 40 && rect.right == 30
                        && rect.bottom == 50));
    }

    @Test
    public void testApply_clipBeforeNoAnimationBounds() {
        // Stack bounds is (0, 0, 10, 10) animation clip is (0, 0, 0, 0)
        WindowAnimationSpec windowAnimationSpec = new WindowAnimationSpec(mAnimation, null,
                mStackBounds, false /* canSkipFirstFrame */, STACK_CLIP_BEFORE_ANIM,
                true /* isAppAnimation */);
        windowAnimationSpec.apply(mTransaction, mSurfaceControl, 0);
        verify(mTransaction).setWindowCrop(eq(mSurfaceControl),
                argThat(rect -> rect.equals(mStackBounds)));
    }

    @Test
    public void testApply_clipBeforeNoStackBounds() {
        // Stack bounds is (0, 0, 0, 0) animation clip is (0, 0, 20, 20)
        Rect windowCrop = new Rect(0, 0, 20, 20);
        Animation a = createClipRectAnimation(windowCrop, windowCrop);
        a.initialize(0, 0, 0, 0);
        WindowAnimationSpec windowAnimationSpec = new WindowAnimationSpec(a, null,
                null, false /* canSkipFirstFrame */, STACK_CLIP_BEFORE_ANIM,
                true /* isAppAnimation */);
        windowAnimationSpec.apply(mTransaction, mSurfaceControl, 0);
        verify(mTransaction).setWindowCrop(eq(mSurfaceControl), argThat(Rect::isEmpty));
    }

    @Test
    public void testApply_clipBeforeSmallerAnimationClip() {
        // Stack bounds is (0, 0, 10, 10) animation clip is (0, 0, 5, 5)
        Rect windowCrop = new Rect(0, 0, 5, 5);
        Animation a = createClipRectAnimation(windowCrop, windowCrop);
        WindowAnimationSpec windowAnimationSpec = new WindowAnimationSpec(a, null,
                mStackBounds, false /* canSkipFirstFrame */, STACK_CLIP_BEFORE_ANIM,
                true /* isAppAnimation */);
        windowAnimationSpec.apply(mTransaction, mSurfaceControl, 0);
        verify(mTransaction).setWindowCrop(eq(mSurfaceControl),
                argThat(rect -> rect.equals(windowCrop)));
    }

    @Test
    public void testApply_clipBeforeSmallerStackClip() {
        // Stack bounds is (0, 0, 10, 10) animation clip is (0, 0, 20, 20)
        Rect windowCrop = new Rect(0, 0, 20, 20);
        Animation a = createClipRectAnimation(windowCrop, windowCrop);
        WindowAnimationSpec windowAnimationSpec = new WindowAnimationSpec(a, null,
                mStackBounds, false /* canSkipFirstFrame */, STACK_CLIP_BEFORE_ANIM,
                true /* isAppAnimation */);
        windowAnimationSpec.apply(mTransaction, mSurfaceControl, 0);
        verify(mTransaction).setWindowCrop(eq(mSurfaceControl),
                argThat(rect -> rect.equals(mStackBounds)));
    }

    private Animation createClipRectAnimation(Rect fromClip, Rect toClip) {
        Animation a = new ClipRectAnimation(fromClip, toClip);
        a.initialize(0, 0, 0, 0);
        return a;
    }
}
