/*
 * Copyright (C) 2017 The Android Open Source Project
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

package com.android.tv.settings.connectivity.util;


import android.support.v4.app.Fragment;

/**
 * State used by {@link StateMachine}.
 */
public interface State {

    /**
     * Process when moving forward.
     */
    void processForward();

    /**
     * Process when moving backward.
     */
    void processBackward();

    /**
     * Listener for sending notification about state completion.
     */
    interface StateCompleteListener {
        void onComplete(@StateMachine.Event int event);
    }

    /**
     * Listener for sending notification about fragment change.
     */
    interface FragmentChangeListener {
        void onFragmentChange(Fragment newFragment, boolean movingForward);
    }

    /**
     * @return the corresponding fragment for current state. If there is no corresponding fragment,
     * return null.
     */
    Fragment getFragment();
}
