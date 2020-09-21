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

package androidx.appcompat.view;

import static androidx.annotation.RestrictTo.Scope.LIBRARY_GROUP;

import android.view.View;
import android.view.animation.Interpolator;

import androidx.annotation.RestrictTo;
import androidx.core.view.ViewPropertyAnimatorCompat;
import androidx.core.view.ViewPropertyAnimatorListener;
import androidx.core.view.ViewPropertyAnimatorListenerAdapter;

import java.util.ArrayList;

/**
 * A very naive implementation of a set of
 * {@link androidx.core.view.ViewPropertyAnimatorCompat}.
 *
 * @hide
 */
@RestrictTo(LIBRARY_GROUP)
public class ViewPropertyAnimatorCompatSet {

    final ArrayList<ViewPropertyAnimatorCompat> mAnimators;

    private long mDuration = -1;
    private Interpolator mInterpolator;
    ViewPropertyAnimatorListener mListener;

    private boolean mIsStarted;

    public ViewPropertyAnimatorCompatSet() {
        mAnimators = new ArrayList<ViewPropertyAnimatorCompat>();
    }

    public ViewPropertyAnimatorCompatSet play(ViewPropertyAnimatorCompat animator) {
        if (!mIsStarted) {
            mAnimators.add(animator);
        }
        return this;
    }

    public ViewPropertyAnimatorCompatSet playSequentially(ViewPropertyAnimatorCompat anim1,
            ViewPropertyAnimatorCompat anim2) {
        mAnimators.add(anim1);
        anim2.setStartDelay(anim1.getDuration());
        mAnimators.add(anim2);
        return this;
    }

    public void start() {
        if (mIsStarted) return;
        for (ViewPropertyAnimatorCompat animator : mAnimators) {
            if (mDuration >= 0) {
                animator.setDuration(mDuration);
            }
            if (mInterpolator != null) {
                animator.setInterpolator(mInterpolator);
            }
            if (mListener != null) {
                animator.setListener(mProxyListener);
            }
            animator.start();
        }

        mIsStarted = true;
    }

    void onAnimationsEnded() {
        mIsStarted = false;
    }

    public void cancel() {
        if (!mIsStarted) {
            return;
        }
        for (ViewPropertyAnimatorCompat animator : mAnimators) {
            animator.cancel();
        }
        mIsStarted = false;
    }

    public ViewPropertyAnimatorCompatSet setDuration(long duration) {
        if (!mIsStarted) {
            mDuration = duration;
        }
        return this;
    }

    public ViewPropertyAnimatorCompatSet setInterpolator(Interpolator interpolator) {
        if (!mIsStarted) {
            mInterpolator = interpolator;
        }
        return this;
    }

    public ViewPropertyAnimatorCompatSet setListener(ViewPropertyAnimatorListener listener) {
        if (!mIsStarted) {
            mListener = listener;
        }
        return this;
    }

    private final ViewPropertyAnimatorListenerAdapter mProxyListener
            = new ViewPropertyAnimatorListenerAdapter() {
        private boolean mProxyStarted = false;
        private int mProxyEndCount = 0;

        @Override
        public void onAnimationStart(View view) {
            if (mProxyStarted) {
                return;
            }
            mProxyStarted = true;
            if (mListener != null) {
                mListener.onAnimationStart(null);
            }
        }

        void onEnd() {
            mProxyEndCount = 0;
            mProxyStarted = false;
            onAnimationsEnded();
        }

        @Override
        public void onAnimationEnd(View view) {
            if (++mProxyEndCount == mAnimators.size()) {
                if (mListener != null) {
                    mListener.onAnimationEnd(null);
                }
                onEnd();
            }
        }
    };
}
