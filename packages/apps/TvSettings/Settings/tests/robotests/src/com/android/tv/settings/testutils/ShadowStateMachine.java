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

package com.android.tv.settings.testutils;

import com.android.tv.settings.connectivity.util.State;
import com.android.tv.settings.connectivity.util.StateMachine;

import org.robolectric.annotation.Implementation;
import org.robolectric.annotation.Implements;

@Implements(value = StateMachine.class, inheritImplementationMethods = true)
public class ShadowStateMachine {
    private static State.StateCompleteListener sListener;

    @Implementation
    public State.StateCompleteListener getListener() {
        return sListener;
    }

    @Implementation
    public void start(boolean mvoingForward) {
    }

    public void setListener(State.StateCompleteListener listener) {
        sListener = listener;
    }
}
