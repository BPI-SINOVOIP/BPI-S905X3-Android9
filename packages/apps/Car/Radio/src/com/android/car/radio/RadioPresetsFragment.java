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

package com.android.car.radio;

import android.animation.Animator;
import android.animation.AnimatorSet;
import android.animation.ObjectAnimator;
import android.annotation.NonNull;
import android.content.Context;
import android.hardware.radio.RadioManager.ProgramInfo;
import android.os.Bundle;
import android.support.v4.app.Fragment;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import androidx.car.widget.DayNightStyle;
import androidx.car.widget.PagedListView;

import com.android.car.broadcastradio.support.Program;
import com.android.car.radio.storage.RadioStorage;

import java.util.List;

/**
 * A fragment that is responsible for displaying a list of the user's saved radio stations.
 */
public class RadioPresetsFragment extends Fragment implements
        FragmentWithFade,
        RadioAnimationManager.OnExitCompleteListener,
        RadioController.ProgramInfoChangeListener,
        RadioStorage.PresetsChangeListener {
    private static final String TAG = "PresetsFragment";
    private static final int ANIM_DURATION_MS = 200;

    private View mCurrentRadioCard;

    private RadioStorage mRadioStorage;
    private RadioController mRadioController;
    private RadioAnimationManager mAnimManager;

    private PresetListExitListener mPresetListExitListener;

    private PagedListView mPresetsList;
    private final PresetsAdapter mPresetsAdapter = new PresetsAdapter();

    /**
     * An interface that will be notified when the user has indicated that they would like to
     * exit the preset list.
     */
    public interface PresetListExitListener {
        /**
         * Method to be called when the preset list should be closed and the main radio display
         * should be shown.
         */
        void OnPresetListExit();
    }

    /**
     * Sets the {@link RadioController} that is responsible for updating the UI of this fragment
     * with the information of the current radio station.
     */
    private void setRadioController(RadioController radioController) {
        mRadioController = radioController;
    }

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container,
            Bundle savedInstanceState) {
        return inflater.inflate(R.layout.radio_presets_list, container, false);
    }

    @Override
    public void onViewCreated(View view, Bundle savedInstanceState) {
        Context context = getContext();

        mPresetsAdapter.setOnPresetItemClickListener(mRadioController::tune);
        mPresetsAdapter.setOnPresetItemFavoriteListener(this::handlePresetItemFavoriteChanged);

        mCurrentRadioCard = view.findViewById(R.id.current_radio_station_card);

        mAnimManager = new RadioAnimationManager(getContext(), view);
        mAnimManager.playEnterAnimation();

        // Clicking on the current radio station card will close the activity and return to the
        // main radio screen.
        mCurrentRadioCard.setOnClickListener(
                v -> mAnimManager.playExitAnimation(RadioPresetsFragment.this /* listener */));

        mPresetsList = view.findViewById(R.id.presets_list);
        mPresetsList.setDayNightStyle(DayNightStyle.ALWAYS_LIGHT);
        mPresetsList.setAdapter(mPresetsAdapter);
        mPresetsList.getRecyclerView().addOnScrollListener(new PresetListScrollListener(
                context, view, mCurrentRadioCard, mPresetsList));

        mRadioStorage = RadioStorage.getInstance(context);
        mRadioStorage.addPresetsChangeListener(this);
        setPresetsOnList(mRadioStorage.getPresets());
    }

    @Override
    public void fadeOutContent() {
        ObjectAnimator containerAlphaAnimator =
                ObjectAnimator.ofFloat(mCurrentRadioCard, View.ALPHA, 0f);
        containerAlphaAnimator.setDuration(ANIM_DURATION_MS);
        containerAlphaAnimator.addListener(new Animator.AnimatorListener() {
            @Override
            public void onAnimationStart(Animator animation) {}

            @Override
            public void onAnimationEnd(Animator animation) {
                mCurrentRadioCard.setVisibility(View.GONE);
            }

            @Override
            public void onAnimationCancel(Animator animation) {}

            @Override
            public void onAnimationRepeat(Animator animation) {}
        });

        ObjectAnimator presetListAlphaAnimator =
                ObjectAnimator.ofFloat(mPresetsList, View.ALPHA, 0f);
        presetListAlphaAnimator.setDuration(ANIM_DURATION_MS);
        presetListAlphaAnimator.addListener(new Animator.AnimatorListener() {
            @Override
            public void onAnimationStart(Animator animation) {}

            @Override
            public void onAnimationEnd(Animator animation) {
                mPresetsList.setVisibility(View.GONE);
            }

            @Override
            public void onAnimationCancel(Animator animation) {}

            @Override
            public void onAnimationRepeat(Animator animation) {}
        });

        AnimatorSet animatorSet = new AnimatorSet();
        animatorSet.playTogether(containerAlphaAnimator, presetListAlphaAnimator);
        animatorSet.start();
    }

    @Override
    public void fadeInContent() {
        // Only the current radio card needs to be faded in as that is the only part
        // of the fragment that will peek over the manual tuner.
        ObjectAnimator containerAlphaAnimator =
                ObjectAnimator.ofFloat(mCurrentRadioCard, View.ALPHA, 1f);
        containerAlphaAnimator.setDuration(ANIM_DURATION_MS);
        containerAlphaAnimator.addListener(new Animator.AnimatorListener() {
            @Override
            public void onAnimationStart(Animator animation) {
                mCurrentRadioCard.setVisibility(View.VISIBLE);
            }

            @Override
            public void onAnimationEnd(Animator animation) {}

            @Override
            public void onAnimationCancel(Animator animation) {}

            @Override
            public void onAnimationRepeat(Animator animation) {}
        });

        ObjectAnimator presetListAlphaAnimator =
                ObjectAnimator.ofFloat(mPresetsList, View.ALPHA, 1f);
        presetListAlphaAnimator.setDuration(ANIM_DURATION_MS);
        presetListAlphaAnimator.addListener(new Animator.AnimatorListener() {
            @Override
            public void onAnimationStart(Animator animation) {
                mPresetsList.setVisibility(View.VISIBLE);
            }

            @Override
            public void onAnimationEnd(Animator animation) {}

            @Override
            public void onAnimationCancel(Animator animation) {}

            @Override
            public void onAnimationRepeat(Animator animation) {}
        });

        AnimatorSet animatorSet = new AnimatorSet();
        animatorSet.playTogether(containerAlphaAnimator, presetListAlphaAnimator);
        animatorSet.start();
    }

    @Override
    public void onStart() {
        super.onStart();
        mRadioController.initialize(getView());
        mRadioController.setShouldColorStatusBar(true);
        mRadioController.addProgramInfoChangeListener(this);

        // TODO(b/73950974): use callback only
        ProgramInfo info = mRadioController.getCurrentProgramInfo();
        if (info != null) {
            mPresetsAdapter.setActiveProgram(Program.fromProgramInfo(info));
        }
    }

    @Override
    public void onDestroy() {
        mRadioStorage.removePresetsChangeListener(this);
        mRadioController.removeProgramInfoChangeListener(this);
        mPresetListExitListener = null;
        super.onDestroy();
    }

    @Override
    public void onExitAnimationComplete() {
        if (mPresetListExitListener != null) {
            mPresetListExitListener.OnPresetListExit();
        }
    }


    @Override
    public void onProgramInfoChanged(@NonNull ProgramInfo info) {
        mPresetsAdapter.setPresets(mRadioStorage.getPresets());
        mPresetsAdapter.setActiveProgram(Program.fromProgramInfo(info));
    }

    @Override
    public void onPresetsRefreshed() {
        if (Log.isLoggable(TAG, Log.DEBUG)) {
            Log.d(TAG, "onPresetsRefreshed()");
        }
    }

    private void handlePresetItemFavoriteChanged(Program program, boolean saveAsFavorite) {
        if (saveAsFavorite) {
            mRadioStorage.storePreset(program);
        } else {
            mRadioStorage.removePreset(program.getSelector());
        }
    }

    /**
     * Sets the given list of presets into the PagedListView {@link #mPresetsList}.
     */
    private void setPresetsOnList(List<Program> presets) {
        if (Log.isLoggable(TAG, Log.DEBUG)) {
            Log.d(TAG, "setPresetsOnList(). # of presets: " + presets.size());
        }

        mPresetsAdapter.setPresets(presets);
    }

    /**
     * Returns a new instance of the {@link RadioPresetsFragment}.
     *
     * @param radioController The {@link RadioController} that is responsible for updating the UI
     *                        of the returned fragment.
     */
    public static RadioPresetsFragment newInstance(RadioController radioController,
            PresetListExitListener existListener) {
        RadioPresetsFragment fragment = new RadioPresetsFragment();
        fragment.setRadioController(radioController);
        fragment.mPresetListExitListener = existListener;

        return fragment;
    }
}
