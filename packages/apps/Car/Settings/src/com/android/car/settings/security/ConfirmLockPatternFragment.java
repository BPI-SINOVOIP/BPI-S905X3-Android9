/*
 * Copyright (C) 2018 The Android Open Source Project
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

package com.android.car.settings.security;

import android.content.Context;
import android.os.Bundle;
import android.os.UserHandle;
import android.view.View;
import android.widget.TextView;

import com.android.car.settings.R;
import com.android.car.settings.common.BaseFragment;
import com.android.internal.widget.LockPatternUtils;
import com.android.internal.widget.LockPatternView;

import java.util.List;

/**
 * Fragment for confirming existing lock pattern
 */
public class ConfirmLockPatternFragment extends BaseFragment {

    private static final String FRAGMENT_TAG_CHECK_LOCK_WORKER = "check_lock_worker";
    // Time we wait before clearing a wrong pattern and the error message.
    private static final long CLEAR_WRONG_ATTEMPT_TIMEOUT_MS = 2500L;

    private LockPatternView mLockPatternView;
    private TextView mMsgView;

    private LockPatternUtils mLockPatternUtils;
    private CheckLockWorker mCheckLockWorker;
    private CheckLockListener mCheckLockListener;

    private int mUserId;
    private List<LockPatternView.Cell> mPattern;

    /**
     * Factory method for creating ConfirmLockPatternFragment
     */
    public static ConfirmLockPatternFragment newInstance() {
        ConfirmLockPatternFragment patternFragment = new ConfirmLockPatternFragment();
        Bundle bundle = BaseFragment.getBundle();
        bundle.putInt(EXTRA_TITLE_ID, R.string.security_settings_title);
        bundle.putInt(EXTRA_ACTION_BAR_LAYOUT, R.layout.action_bar_with_button);
        bundle.putInt(EXTRA_LAYOUT, R.layout.confirm_lock_pattern_fragment);
        patternFragment.setArguments(bundle);
        return patternFragment;
    }

    @Override
    public void onAttach(Context context) {
        super.onAttach(context);
        if ((getActivity() instanceof CheckLockListener)) {
            mCheckLockListener = (CheckLockListener) getActivity();
        } else {
            throw new RuntimeException("The activity must implement CheckLockListener");
        }
    }

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        mLockPatternUtils = new LockPatternUtils(getContext());
        mUserId = UserHandle.myUserId();
    }

    @Override
    public void onViewCreated(View view, Bundle savedInstanceState) {
        super.onViewCreated(view, savedInstanceState);

        mMsgView = (TextView) view.findViewById(R.id.message);
        mLockPatternView = (LockPatternView) view.findViewById(R.id.lockPattern);
        mLockPatternView.setFadePattern(false);
        mLockPatternView.setInStealthMode(!mLockPatternUtils.isVisiblePatternEnabled(mUserId));
        mLockPatternView.setOnPatternListener(mLockPatternListener);

        if (savedInstanceState != null) {
            mCheckLockWorker = (CheckLockWorker) getFragmentManager().findFragmentByTag(
                    FRAGMENT_TAG_CHECK_LOCK_WORKER);
        }
    }

    @Override
    public void onStart() {
        super.onStart();
        if (mCheckLockWorker != null) {
            mCheckLockWorker.setListener(this::onCheckCompleted);
        }
    }

    @Override
    public void onStop() {
        super.onStop();
        if (mCheckLockWorker != null) {
            mCheckLockWorker.setListener(null);
        }
    }

    private Runnable mClearErrorRunnable = () ->  {
        mLockPatternView.clearPattern();
        mMsgView.setText("");
    };

    private LockPatternView.OnPatternListener mLockPatternListener =
            new LockPatternView.OnPatternListener() {

        public void onPatternStart() {
            mLockPatternView.removeCallbacks(mClearErrorRunnable);
            mMsgView.setText("");
        }

        public void onPatternCleared() {
            mLockPatternView.removeCallbacks(mClearErrorRunnable);
        }

        public void onPatternCellAdded(List<LockPatternView.Cell> pattern) {}

        public void onPatternDetected(List<LockPatternView.Cell> pattern) {
            mLockPatternView.setEnabled(false);

            if (mCheckLockWorker == null) {
                mCheckLockWorker = new CheckLockWorker();
                mCheckLockWorker.setListener(ConfirmLockPatternFragment.this::onCheckCompleted);

                getFragmentManager()
                        .beginTransaction()
                        .add(mCheckLockWorker, FRAGMENT_TAG_CHECK_LOCK_WORKER)
                        .commitNow();
            }

            mPattern = pattern;
            mCheckLockWorker.checkPattern(mUserId, pattern);
        }
    };

    private void onCheckCompleted(boolean lockMatched) {
        if (lockMatched) {
            mCheckLockListener.onLockVerified(LockPatternUtils.patternToString(mPattern));
        } else {
            mLockPatternView.setEnabled(true);
            mMsgView.setText(R.string.lockpattern_pattern_wrong);

            // Set timer to clear wrong pattern
            mLockPatternView.removeCallbacks(mClearErrorRunnable);
            mLockPatternView.postDelayed(mClearErrorRunnable, CLEAR_WRONG_ATTEMPT_TIMEOUT_MS);
        }
    }
}
