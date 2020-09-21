/*
 * Copyright (c) 2016, The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
package com.android.car.hvac.controllers;

import android.animation.Animator;
import android.animation.AnimatorSet;
import android.animation.ObjectAnimator;
import android.animation.ValueAnimator;
import android.content.Context;
import android.content.res.Resources;
import android.graphics.drawable.Drawable;
import android.os.Handler;
import android.view.MotionEvent;
import android.view.View;
import android.view.WindowManager;
import android.widget.ImageView;

import androidx.annotation.IntDef;

import com.android.car.hvac.HvacController;
import com.android.car.hvac.R;
import com.android.car.hvac.ui.FanDirectionButtons;
import com.android.car.hvac.ui.FanSpeedBar;
import com.android.car.hvac.ui.HvacPanelRow;
import com.android.car.hvac.ui.SeatWarmerButton;
import com.android.car.hvac.ui.TemperatureBarOverlay;
import com.android.car.hvac.ui.ToggleButton;

import java.util.ArrayList;
import java.util.List;

/**
 * A state machine to control transition from various HVAC UI layouts.
 */
public class HvacPanelController {
    private static final int PANEL_ANIMATION_TIME_MS = 200;
    private static final int PANEL_COLLAPSE_ANIMATION_TIME_MS = 500;

    private static final int PANEL_ANIMATION_DELAY_MS = 100;
    private static final int PANEL_ANIMATION_LONG_DELAY_MS = 3 * PANEL_ANIMATION_DELAY_MS;

    private static final float DISABLED_BUTTON_ALPHA = 0.20f;
    private static final float ENABLED_BUTTON_ALPHA = 1.0f;

    private static final int STATE_COLLAPSED = 0;
    private static final int STATE_COLLAPSED_DIMMED = 1;
    private static final int STATE_FULL_EXPANDED = 2;
    // Allows for delayed invocation of code. Thus we can control UI events to happen after
    // others. Example: set something visible but do it after we've complete current UI updates.
    private static Handler handler = new Handler();

    // We have both a collapsed and expanded version of the overlays due to a bug
    // that does not correctly rendering a window resize event. Thus we toggle the the visibility
    // of the windows instead. A better solution would be a having separate views collapsed state
    // since the it does not need the other elements but this works for now.
    private TemperatureBarOverlay mDriverTemperatureBarCollapsed;
    private TemperatureBarOverlay mPassengerTemperatureBarCollapsed;
    private TemperatureBarOverlay mDriverTemperatureBarExpanded;
    private TemperatureBarOverlay mPassengerTemperatureBarExpanded;
    private final boolean mShowCollapsed;

    @IntDef({STATE_COLLAPSED,
            STATE_COLLAPSED_DIMMED,
            STATE_FULL_EXPANDED})
    private @interface HvacPanelState {}

    private @HvacPanelState int mCurrentState;

    private int mPanelCollapsedHeight;
    private int mPanelFullExpandedHeight;

    private View mPanel;
    private View mContainer;

    private SeatWarmerButton mDriverSeatWarmer;
    private SeatWarmerButton mPassengerSeatWarmer;

    private ToggleButton mHvacPowerSwitch;

    private ToggleButton mAcButton;
    private ToggleButton mRecycleAirButton;

    private ToggleButton mFrontDefrosterButton;
    private ToggleButton mRearDefrosterButton;

    private Drawable mAutoOnDrawable;
    private Drawable mAutoOffDrawable;

    private ImageView mAutoButton;

    private HvacPanelRow mPanelTopRow;
    private HvacPanelRow mPanelBottomRow;

    private FanSpeedBar mFanSpeedBar;
    private FanDirectionButtons mFanDirectionButtons;

    private float mTopPanelMaxAlpha = 1.0f;

    private WindowManager mWindowManager;

    private HvacPanelStateTransition mTransition;

    private View mHvacFanControlBackground;

    private HvacController mHvacController;
    private FanSpeedBarController mFanSpeedBarController;
    private FanDirectionButtonsController mFanDirectionButtonsController;
    private TemperatureController mTemperatureController;
    private TemperatureController mTemperatureControllerCollapsed;
    private SeatWarmerController mSeatWarmerController;

    private boolean mInAnimation;

    // TODO: read from shared pref
    private boolean mAutoMode;

    public HvacPanelController(Context context, View container,
            WindowManager windowManager,
            TemperatureBarOverlay driverTemperatureExpanded,
            TemperatureBarOverlay passengerTemperatureExpanded,
            TemperatureBarOverlay driverTemperatureBarCollapsed,
            TemperatureBarOverlay passengerTemperatureBarCollapsed) {
        Resources res = context.getResources();
        mShowCollapsed = res.getBoolean(R.bool.config_showCollapsedBars);

        mDriverTemperatureBarCollapsed = driverTemperatureBarCollapsed;
        mPassengerTemperatureBarCollapsed = passengerTemperatureBarCollapsed;

        mCurrentState = STATE_COLLAPSED;
        mWindowManager = windowManager;

        mPanelCollapsedHeight = res.getDimensionPixelSize(R.dimen.car_hvac_panel_collapsed_height);
        mPanelFullExpandedHeight
                = res.getDimensionPixelSize(R.dimen.car_hvac_panel_full_expanded_height);

        mAutoOffDrawable = res.getDrawable(R.drawable.ic_auto_off);
        mAutoOnDrawable = res.getDrawable(R.drawable.ic_auto_on);

        mDriverTemperatureBarExpanded = driverTemperatureExpanded;
        mPassengerTemperatureBarExpanded = passengerTemperatureExpanded;

        mDriverTemperatureBarExpanded.setCloseButtonOnClickListener(mCollapseHvac);
        mPassengerTemperatureBarExpanded.setCloseButtonOnClickListener(mCollapseHvac);

        // Initially the hvac panel is collapsed, hide the expanded version.
        mDriverTemperatureBarExpanded.setVisibility(View.INVISIBLE);
        mPassengerTemperatureBarExpanded.setVisibility(View.INVISIBLE);

        mPassengerTemperatureBarCollapsed.setBarOnClickListener(mExpandHvac);
        mDriverTemperatureBarCollapsed.setBarOnClickListener(mExpandHvac);

        mContainer = container;
        mContainer.setVisibility(View.INVISIBLE);
        mContainer.setOnClickListener(mCollapseHvac);
        mPanel = mContainer.findViewById(R.id.hvac_center_panel);

        mHvacFanControlBackground = mPanel.findViewById(R.id.fan_control_bg);
        // set clickable so that clicks are not forward to the mContainer. This way a miss click
        // does not close the UI
        mPanel.setClickable(true);

        // Set up top row buttons
        mPanelTopRow = (HvacPanelRow) mContainer.findViewById(R.id.top_row);

        mAcButton = (ToggleButton) mPanelTopRow.findViewById(R.id.ac_button);
        mAcButton.setToggleIcons(res.getDrawable(R.drawable.ic_ac_on),
                res.getDrawable(R.drawable.ic_ac_off));

        mRecycleAirButton = (ToggleButton) mPanelTopRow.findViewById(R.id.recycle_air_button);

        mRecycleAirButton.setToggleIcons(res.getDrawable(R.drawable.ic_recycle_air_on),
                res.getDrawable(R.drawable.ic_recycle_air_off));

        // Setup bottom row buttons
        mPanelBottomRow = (HvacPanelRow) mContainer.findViewById(R.id.bottom_row);

        mAutoButton = (ImageView) mContainer.findViewById(R.id.auto_button);
        mAutoButton.setOnClickListener(mAutoButtonClickListener);

        mFrontDefrosterButton = (ToggleButton) mPanelBottomRow.findViewById(R.id.front_defroster);
        mRearDefrosterButton = (ToggleButton) mPanelBottomRow.findViewById(R.id.rear_defroster);

        mFrontDefrosterButton.setToggleIcons(res.getDrawable(R.drawable.ic_front_defroster_on),
                res.getDrawable(R.drawable.ic_front_defroster_off));

        mRearDefrosterButton.setToggleIcons(res.getDrawable(R.drawable.ic_rear_defroster_on),
                res.getDrawable(R.drawable.ic_rear_defroster_off));

        mFanSpeedBar = (FanSpeedBar) mContainer.findViewById(R.id.fan_speed_bar);
        mFanDirectionButtons = (FanDirectionButtons) mContainer.findViewById(R.id.fan_direction_buttons);

        mDriverSeatWarmer = (SeatWarmerButton) mContainer.findViewById(R.id.left_seat_heater);
        mPassengerSeatWarmer = (SeatWarmerButton) mContainer.findViewById(R.id.right_seat_heater);

        mHvacPowerSwitch = (ToggleButton)mPanelBottomRow.findViewById(R.id.hvac_master_switch);
        // TODO: this is not good UX design - just a placeholder
        mHvacPowerSwitch.setToggleIcons(res.getDrawable(R.drawable.ac_master_switch_on),
            res.getDrawable(R.drawable.ac_master_switch_off));

        if (!mShowCollapsed) {
            mDriverTemperatureBarCollapsed.setVisibility(View.INVISIBLE);
            mPassengerTemperatureBarCollapsed.setVisibility(View.INVISIBLE);
        }
    }

    public void updateHvacController(HvacController controller) {
        //TODO: handle disconnected HvacController.
        mHvacController = controller;

        mFanSpeedBarController = new FanSpeedBarController(mFanSpeedBar, mHvacController);
        mFanDirectionButtonsController
                = new FanDirectionButtonsController(mFanDirectionButtons, mHvacController);
        mTemperatureController = new TemperatureController(
                mPassengerTemperatureBarExpanded,
                mDriverTemperatureBarExpanded,
                mPassengerTemperatureBarCollapsed,
                mDriverTemperatureBarCollapsed,
                mHvacController);
        mSeatWarmerController = new SeatWarmerController(mPassengerSeatWarmer,
                mDriverSeatWarmer, mHvacController);

        // Toggle buttons do not need additional logic to map between hardware
        // and UI settings. Simply use a ToggleListener to handle clicks.
        mAcButton.setIsOn(mHvacController.getAcState());
        mAcButton.setToggleListener(new ToggleButton.ToggleListener() {
            @Override
            public void onToggled(boolean isOn) {
                mHvacController.setAcState(isOn);
            }
        });

        mFrontDefrosterButton.setIsOn(mHvacController.getFrontDefrosterState());
        mFrontDefrosterButton.setToggleListener(new ToggleButton.ToggleListener() {
            @Override
            public void onToggled(boolean isOn) {
                mHvacController.setFrontDefrosterState(isOn);
            }
        });

        mRearDefrosterButton.setIsOn(mHvacController.getRearDefrosterState());
        mRearDefrosterButton.setToggleListener(new ToggleButton.ToggleListener() {
            @Override
            public void onToggled(boolean isOn) {
                mHvacController.setRearDefrosterState(isOn);
            }
        });

        mRecycleAirButton.setIsOn(mHvacController.getAirCirculationState());
        mRecycleAirButton.setToggleListener(new ToggleButton.ToggleListener() {
            @Override
            public void onToggled(boolean isOn) {
                mHvacController.setAirCirculation(isOn);
            }
        });

        setAutoMode(mHvacController.getAutoModeState());

        mHvacPowerSwitch.setIsOn(mHvacController.getHvacPowerState());
        mHvacPowerSwitch.setToggleListener(isOn -> mHvacController.setHvacPowerState(isOn));

        mHvacController.registerCallback(mToggleButtonCallbacks);
        mToggleButtonCallbacks.onHvacPowerChange(mHvacController.getHvacPowerState());
    }

    private HvacController.Callback mToggleButtonCallbacks
            = new HvacController.Callback() {
        @Override
        public void onAirCirculationChange(boolean isOn) {
            mRecycleAirButton.setIsOn(isOn);
        }

        @Override
        public void onFrontDefrosterChange(boolean isOn) {
            mFrontDefrosterButton.setIsOn(isOn);
        }

        @Override
        public void onRearDefrosterChange(boolean isOn) {
            mRearDefrosterButton.setIsOn(isOn);
        }

        @Override
        public void onAcStateChange(boolean isOn) {
            mAcButton.setIsOn(isOn);
        }

        @Override
        public void onAutoModeChange(boolean isOn) {
            mAutoMode = isOn;
            setAutoMode(mAutoMode);
        }
    };

    /**
     * Take the listeners and animators from a {@link AnimatorSet} and merge them to the
     * input {@link Animator} and {@link android.animation.Animator.AnimatorListener} lists.
     */
    private void combineAnimationSet(List<Animator> animatorList,
            List<Animator.AnimatorListener> listenerList, AnimatorSet set) {

        ArrayList<Animator> list = set.getChildAnimations();
        if (list != null) {
            int size = list.size();
            for (int i = 0; i < size; i++) {
                animatorList.add(list.get(i));
            }
        }

        ArrayList<Animator.AnimatorListener> listeners = set.getListeners();
        if (listeners != null) {
            int size = listeners.size();
            for (int i = 0; i < size; i++) {
                listenerList.add(listeners.get(i));
            }
        }
    }

    /**
     * Play necessary animations between {@link HvacPanelState} transitions
     */
    private void transitionState(@HvacPanelState int startState, @HvacPanelState int endState) {
        if (startState == endState || mInAnimation) {
            return;
        }

        List<Animator> animationList = new ArrayList<>();
        List<Animator.AnimatorListener> listenerList = new ArrayList<>();
        ValueAnimator heightAnimator = getPanelHeightAnimator(startState, endState);
        mTransition = new HvacPanelStateTransition(startState, endState);
        ValueAnimator fanBgAlphaAnimator;
        switch (endState) {
            case STATE_COLLAPSED:
                // Transition to collapsed state:
                // 1. Collapse the temperature bars.
                // 2. Collapse the top and bottom panel, staggered with a different delay.
                // 3. Decrease height of the hvac center panel, but maintain container height.
                // 4. Fade the background of the fan controls seperately to create staggered effect.
                animationList.add(heightAnimator);
                heightAnimator.setDuration(PANEL_COLLAPSE_ANIMATION_TIME_MS);
                fanBgAlphaAnimator
                        = ObjectAnimator.ofFloat(mHvacFanControlBackground, View.ALPHA, 1, 0)
                        .setDuration(PANEL_COLLAPSE_ANIMATION_TIME_MS);
                fanBgAlphaAnimator.setStartDelay(PANEL_ANIMATION_DELAY_MS);
                animationList.add(fanBgAlphaAnimator);

                ValueAnimator panelAlphaAnimator
                        = ObjectAnimator.ofFloat(mContainer, View.ALPHA, 1, 0);
                panelAlphaAnimator.setDuration(200);
                panelAlphaAnimator.setStartDelay(300);

                animationList.add(panelAlphaAnimator);

                combineAnimationSet(animationList, listenerList,
                        mDriverTemperatureBarExpanded.getCollapseAnimations());
                combineAnimationSet(animationList, listenerList,
                        mPassengerTemperatureBarExpanded.getCollapseAnimations());

                combineAnimationSet(animationList, listenerList,
                        mPanelTopRow.getCollapseAnimation(PANEL_ANIMATION_DELAY_MS,
                                mTopPanelMaxAlpha));
                combineAnimationSet(animationList, listenerList,
                        mPanelBottomRow.getCollapseAnimation(PANEL_ANIMATION_DELAY_MS,
                                mTopPanelMaxAlpha));
                break;
            case STATE_COLLAPSED_DIMMED:
                // Hide the temperature numbers, open arrows and auto state button.
                // TODO: determine if this section is still needed.
                break;
            case STATE_FULL_EXPANDED:
                // Transition to expaneded state:
                // 1. Expand the temperature bars.
                // 2. Expand the top and bottom panel, staggered with a different delay.
                // 3. Increase height of the hvac center panel, but maintain container height.
                // 4. Fade in fan control background in a staggered manner.
                fanBgAlphaAnimator
                        = ObjectAnimator.ofFloat(mHvacFanControlBackground, View.ALPHA, 0, 1)
                        .setDuration(PANEL_ANIMATION_TIME_MS);
                fanBgAlphaAnimator.setStartDelay(PANEL_ANIMATION_DELAY_MS);
                animationList.add(fanBgAlphaAnimator);

                animationList.add(heightAnimator);
                combineAnimationSet(animationList, listenerList,
                        mDriverTemperatureBarExpanded.getExpandAnimatons());
                combineAnimationSet(animationList, listenerList,
                        mPassengerTemperatureBarExpanded.getExpandAnimatons());

                // During expansion, the bottom panel animation should be delayed
                combineAnimationSet(animationList, listenerList,
                        mPanelTopRow.getExpandAnimation(PANEL_ANIMATION_DELAY_MS,
                                mTopPanelMaxAlpha));
                combineAnimationSet(animationList, listenerList,
                        mPanelBottomRow.getExpandAnimation(PANEL_ANIMATION_LONG_DELAY_MS, 1f));
                break;
            default:
        }

        // If there are animations for the state change, play them all together and ensure
        // the animation listeners are attached.
        if (animationList.size() > 0) {
            AnimatorSet animatorSet = new AnimatorSet();
            animatorSet.playTogether(animationList);
            for (Animator.AnimatorListener listener : listenerList) {
                animatorSet.addListener(listener);
            }
            animatorSet.addListener(mAnimatorListener);
            animatorSet.start();
        }
    }

    private AnimatorSet.AnimatorListener mAnimatorListener = new AnimatorSet.AnimatorListener() {
        @Override
        public void onAnimationStart(Animator animation) {
            mTransition.onTransitionStart();
        }

        @Override
        public void onAnimationEnd(Animator animation) {
            mTransition.onTransitionComplete();
        }

        @Override
        public void onAnimationCancel(Animator animation) {}

        @Override
        public void onAnimationRepeat(Animator animation) {}
    };

    private ValueAnimator getPanelHeightAnimator(@HvacPanelState int startState,
            @HvacPanelState int endState) {
        int startHeight = getStateHeight(startState);
        int endHeight = getStateHeight(endState);
        if (startHeight == endHeight) {
            return null;
        }

        ValueAnimator heightAnimator = new ValueAnimator().ofInt(startHeight, endHeight)
                .setDuration(PANEL_ANIMATION_TIME_MS);
        heightAnimator.addUpdateListener(mHeightUpdateListener);
        return heightAnimator;
    }

    private int getStateHeight(@HvacPanelState int state) {
        switch (state) {
            case STATE_COLLAPSED:
            case STATE_COLLAPSED_DIMMED:
                return mPanelCollapsedHeight;
            case STATE_FULL_EXPANDED:
                return mPanelFullExpandedHeight;
            default:
                throw new IllegalArgumentException("No height mapped to HVAC State: " + state);
        }
    }

    private void setAutoMode(boolean isOn) {
        if (isOn) {
            mPanelTopRow.setOnTouchListener(new View.OnTouchListener() {
                @Override
                public boolean onTouch(View v, MotionEvent event) {
                    return true;
                }
            });
            mAutoMode = true;
            mPanelTopRow.disablePanel(true);
            mTopPanelMaxAlpha = DISABLED_BUTTON_ALPHA;
            mAutoButton.setImageDrawable(mAutoOnDrawable);
        } else {
            mPanelTopRow.disablePanel(false);
            mTopPanelMaxAlpha = ENABLED_BUTTON_ALPHA;
            mAutoButton.setImageDrawable(mAutoOffDrawable);
        }
        mHvacFanControlBackground.setAlpha(mTopPanelMaxAlpha);
        mPanelTopRow.setAlpha(mTopPanelMaxAlpha);
    }

    private View.OnClickListener mAutoButtonClickListener = new View.OnClickListener() {
        @Override
        public void onClick(View v) {
            if (mAutoMode) {
                mAutoMode = false;
            } else {
                mAutoMode = true;
            }
            mHvacController.setAutoMode(mAutoMode);
            setAutoMode(mAutoMode);
        }
    };

     private View.OnClickListener mCollapseHvac = new View.OnClickListener() {
         @Override
         public void onClick(View v) {
             if (mInAnimation) {
                 return;
             }
             if (mCurrentState != STATE_COLLAPSED) {
                 transitionState(mCurrentState, STATE_COLLAPSED);
             }
         }
     };

    public void toggleHvacUi() {
        if(mCurrentState != STATE_COLLAPSED) {
            mCollapseHvac.onClick(null);
        } else {
            mExpandHvac.onClick(null);
        }
    }

    public View.OnClickListener mExpandHvac = new View.OnClickListener() {
        @Override
        public void onClick(View v) {
             if (mInAnimation) {
                return;
            }
            if (mCurrentState != STATE_FULL_EXPANDED) {
                transitionState(mCurrentState, STATE_FULL_EXPANDED);
            }
        }
    };


    private ValueAnimator.AnimatorUpdateListener mHeightUpdateListener
            = new ValueAnimator.AnimatorUpdateListener() {
        @Override
        public void onAnimationUpdate(ValueAnimator animation) {
            int height = (Integer) animation.getAnimatedValue();
            int currentHeight = mPanel.getLayoutParams().height;
            mPanel.getLayoutParams().height = height;
            mPanel.setTop(mPanel.getTop() + height - currentHeight);
            mPanel.requestLayout();
        }
    };

    /**
     * Handles the necessary setup/clean up before and after a state transition.
     */
    private class HvacPanelStateTransition {
        private @HvacPanelState int mEndState;
        private @HvacPanelState int mStartState;

        public HvacPanelStateTransition(@HvacPanelState int startState,
                @HvacPanelState int endState) {
            mStartState = startState;
            mEndState = endState;
        }

        public void onTransitionStart() {
            mInAnimation = true;
            if (mEndState == STATE_FULL_EXPANDED) {
                mPassengerTemperatureBarExpanded.setVisibility(View.VISIBLE);
                mDriverTemperatureBarExpanded.setVisibility(View.VISIBLE);
                if (mShowCollapsed) {
                    mDriverTemperatureBarCollapsed.setVisibility(View.INVISIBLE);
                    mPassengerTemperatureBarCollapsed.setVisibility(View.INVISIBLE);
                }
                mContainer.setAlpha(1);
                mContainer.setVisibility(View.VISIBLE);
            }
        }

        public void onTransitionComplete() {
            if (mEndState == STATE_COLLAPSED) {
                if (mShowCollapsed) {
                    mDriverTemperatureBarCollapsed.setVisibility(View.VISIBLE);
                    mPassengerTemperatureBarCollapsed.setVisibility(View.VISIBLE);
                }
                handler.postAtFrontOfQueue(() -> {
                    mDriverTemperatureBarExpanded.setVisibility(View.INVISIBLE);
                    mPassengerTemperatureBarExpanded.setVisibility(View.INVISIBLE);
                });
            }
            if (mStartState == STATE_FULL_EXPANDED) {
                mContainer.setVisibility(View.INVISIBLE);
            }

            // set new states
            mCurrentState = mEndState;
            mInAnimation = false;
        }
    }
}
