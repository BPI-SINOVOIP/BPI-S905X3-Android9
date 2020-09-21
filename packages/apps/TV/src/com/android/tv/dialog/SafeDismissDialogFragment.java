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

package com.android.tv.dialog;

import android.app.Activity;
import android.app.DialogFragment;
import android.util.Log;

import com.android.tv.MainActivity;
import com.android.tv.TvSingletons;
import com.android.tv.analytics.HasTrackerLabel;
import com.android.tv.analytics.Tracker;
import com.android.tv.common.util.SystemProperties;

/** Provides the safe dismiss feature regardless of the DialogFragment's life cycle. */
public abstract class SafeDismissDialogFragment extends DialogFragment implements HasTrackerLabel {
    private static final String TAG = "SafeDismissDialogFragment";
    private static final boolean DEBUG = false || SystemProperties.USE_DEBUG_DISPLAY.getValue();
    private MainActivity mActivity;
    private boolean mAttached = false;
    private boolean mDismissPending = false;
    private Tracker mTracker;

    @Override
    public void onAttach(Activity activity) {
        super.onAttach(activity);
        mAttached = true;
        if (activity instanceof MainActivity) {
            mActivity = (MainActivity) activity;
        }
        mTracker = TvSingletons.getSingletons(activity).getTracker();
        if (DEBUG) {
            Log.d(TAG, "onAttach mDismissPending " + mDismissPending);
        }
        if (mDismissPending) {
            mDismissPending = false;
            dismiss();
        }
    }

    @Override
    public void onResume() {
        super.onResume();
        mTracker.sendScreenView(getTrackerLabel());
        if (DEBUG) {
            Log.d(TAG, "onResume");
        }
    }

    @Override
    public void onDestroy() {
        if (DEBUG) {
            Log.d(TAG, "onDestroy " + mActivity);
        }
        if (mActivity != null) {
            mActivity.getOverlayManager().onDialogDestroyed();
        }
        super.onDestroy();
    }

    @Override
    public void onDetach() {
        super.onDetach();
        mAttached = false;
        mTracker = null;
        if (DEBUG) {
            Log.d(TAG, "onDetach");
        }
    }

    /** Dismiss safely regardless of the DialogFragment's life cycle. */
    @Override
    public void dismiss() {
        if (DEBUG) {
            Log.d(TAG, "dismiss mAttached = " + mAttached + ", mDismissListener = " + mDismissListener);
        }
        if (!mAttached) {
            // dismiss() before getFragmentManager() is set cause NPE in dismissInternal().
            // FragmentManager is set when a fragment is used in a transaction,
            // so wait here until we can dismiss safely.
            mDismissPending = true;
        } else {
            super.dismiss();
            if (mDismissListener != null) {
                mDismissListener.onDismiss();
            }
        }
    }

    public void setDismissListener(DismissListener listener) {
        if (DEBUG) {
            Log.d(TAG, "setDismissListener = " + listener);
        }
        mDismissListener = listener;
    }

    private DismissListener mDismissListener = null;

    public interface DismissListener {
        void onDismiss();
    }
}
