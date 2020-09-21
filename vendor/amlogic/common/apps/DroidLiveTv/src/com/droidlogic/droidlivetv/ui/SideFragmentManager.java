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

package com.droidlogic.droidlivetv.ui;

import android.animation.Animator;
import android.animation.AnimatorInflater;
import android.animation.AnimatorListenerAdapter;
import android.app.Activity;
import android.app.FragmentManager;
import android.app.FragmentTransaction;
import android.util.Log;
import android.view.View;
import com.droidlogic.droidlivetv.R;

public class SideFragmentManager {
	private static final String TAG = "SideFragmentManager";
    private static final String FIRST_BACKSTACK_RECORD_NAME = "0";

    private final Activity mActivity;
    private final FragmentManager mFragmentManager;
    private final Runnable mPreShowRunnable = null;
    private final Runnable mPostHideRunnable = null;

    // To get the count reliably while using popBackStack(),
    // instead of using getBackStackEntryCount() with popBackStackImmediate().
    private int mFragmentCount;

    private final View mPanel;
    private final Animator mShowAnimator;
    private final Animator mHideAnimator;

    private final Runnable mHideAllRunnable = new Runnable() {
        @Override
        public void run() {
            hideAll(true);
        }
    };
    private long mShowDurationMillis;

    public SideFragmentManager(Activity activity) {
        mActivity = activity;
        mFragmentManager = mActivity.getFragmentManager();

        mPanel = mActivity.findViewById(R.id.side_panel);
        mShowAnimator = AnimatorInflater.loadAnimator(mActivity, R.animator.side_panel_enter);
        mShowAnimator.setTarget(mPanel);
        mHideAnimator = AnimatorInflater.loadAnimator(mActivity, R.animator.side_panel_exit);
        mHideAnimator.setTarget(mPanel);
        mHideAnimator.addListener(new AnimatorListenerAdapter() {
            @Override
            public void onAnimationEnd(Animator animation) {
                // Animation is still in running state at this point.
                Log.d(TAG, "onAnimationEnd");
                hideAllInternal();
            }
        });

        mShowDurationMillis = mActivity.getResources().getInteger(
                R.integer.side_panel_show_duration);
    }

    public int getCount() {
        return mFragmentCount;
    }

    public boolean isActive() {
        return mFragmentCount != 0 && !isHiding();
    }

    public boolean isHiding() {
        return mHideAnimator.isStarted();
    }

    /**
     * Shows the given {@link SideFragment}.
     */
    public void show(SideFragment sideFragment) {
        show(sideFragment, true);
    }

    /**
     * Shows the given {@link SideFragment}.
     */
    public void show(SideFragment sideFragment, boolean showEnterAnimation) {
        SideFragment.preloadRecycledViews(mActivity);
        if (isHiding()) {
            mHideAnimator.end();
        }
        boolean isFirst = (mFragmentCount == 0);
        if (isFirst) {
            if (mPreShowRunnable != null) {
                mPreShowRunnable.run();
            }
        }

        FragmentTransaction ft = mFragmentManager.beginTransaction();
        if (!isFirst) {
            ft.setCustomAnimations(
                    showEnterAnimation ? R.animator.side_panel_fragment_enter : 0,
                    R.animator.side_panel_fragment_exit,
                    R.animator.side_panel_fragment_pop_enter,
                    R.animator.side_panel_fragment_pop_exit);
        }
        ft.replace(R.id.side_fragment_container, sideFragment)
                .addToBackStack(Integer.toString(mFragmentCount)).commit();
        mFragmentCount++;

        if (isFirst) {
            mPanel.setVisibility(View.VISIBLE);
            mShowAnimator.start();
        }
        scheduleHideAll();
    }

    public void popSideFragment() {
        if (!isActive()) {
            return;
        } else if (mFragmentCount == 1) {
            // Show closing animation with the last fragment.
            hideAll(true);
            return;
        }
        mFragmentManager.popBackStack();
        mFragmentCount--;
    }

    public void hideAll(boolean withAnimation) {
        if (withAnimation) {
            if (!isHiding()) {
                mHideAnimator.start();
            }
            return;
        }
        if (isHiding()) {
            mHideAnimator.end();
            return;
        }
        hideAllInternal();
    }

    private void hideAllInternal() {
        if (mFragmentCount == 0) {
            return;
        }

        mPanel.setVisibility(View.GONE);
        mFragmentManager.popBackStack(FIRST_BACKSTACK_RECORD_NAME,
                FragmentManager.POP_BACK_STACK_INCLUSIVE);
        mFragmentCount = 0;

        if (mPostHideRunnable != null) {
            mPostHideRunnable.run();
        }
    }

    /**
     * Show the side panel with animation. If there are many entries in the fragment stack,
     * the animation look like that there's only one fragment.
     *
     * @param withAnimation specifies if animation should be shown.
     */
    public void showSidePanel(boolean withAnimation) {
        SideFragment.preloadRecycledViews(mActivity);
        if (mFragmentCount == 0) {
            return;
        }

        mPanel.setVisibility(View.VISIBLE);
        if (withAnimation) {
            mShowAnimator.start();
        }
        scheduleHideAll();
    }

    /**
     * Hide the side panel. This method just hide the panel and preserves the back
     * stack. If you want to empty the back stack, call {@link #hideAll}.
     */
    public void hideSidePanel(boolean withAnimation) {
        if (withAnimation) {
            mPanel.removeCallbacks(mHideAllRunnable);
            Animator hideAnimator =
                    AnimatorInflater.loadAnimator(mActivity, R.animator.side_panel_exit);
            hideAnimator.setTarget(mPanel);
            hideAnimator.start();
            hideAnimator.addListener(new AnimatorListenerAdapter() {
                @Override
                public void onAnimationEnd(Animator animation) {
                    mPanel.setVisibility(View.GONE);
                }
            });
        } else {
            mPanel.setVisibility(View.GONE);
        }
    }

    public boolean isSidePanelVisible() {
        return mPanel.getVisibility() == View.VISIBLE;
    }

    public void scheduleHideAll() {
        //mPanel.removeCallbacks(mHideAllRunnable);
        //mPanel.postDelayed(mHideAllRunnable, mShowDurationMillis);
    }
}
