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

package com.android.settings.testutils.shadow;

import static org.robolectric.util.ReflectionHelpers.ClassParameter.from;

import android.hardware.input.IInputManager;
import android.hardware.input.InputManager;
import android.os.Handler;

import org.robolectric.annotation.Implementation;
import org.robolectric.annotation.Implements;
import org.robolectric.util.ReflectionHelpers;

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

/*
 * Shadow for {@ InputManager} that has assessors for registered {@link InputDeviceListener}s.
 */
@Implements(value = InputManager.class, callThroughByDefault = false)
public class ShadowInputManager {

    private ArrayList<InputManager.InputDeviceListener> mInputDeviceListeners;

    @Implementation
    public void __constructor__(IInputManager service) {
        mInputDeviceListeners = new ArrayList<>();
    }

    @Implementation
    public static InputManager getInstance() {
        return ReflectionHelpers.callConstructor(
                InputManager.class,
                from(IInputManager.class, null));
    }

    @Implementation
    public void registerInputDeviceListener(InputManager.InputDeviceListener listener,
            Handler handler) {
        // TODO: Use handler.
        if (!mInputDeviceListeners.contains(listener)) {
            mInputDeviceListeners.add(listener);
        }
    }

    @Implementation
    public void unregisterInputDeviceListener(InputManager.InputDeviceListener listener) {
        if (mInputDeviceListeners.contains(listener)) {
            mInputDeviceListeners.remove(listener);
        }
    }

    // Non-Android accessor.
    public List<InputManager.InputDeviceListener> getRegisteredInputDeviceListeners() {
        return Collections.unmodifiableList(mInputDeviceListeners);
    }
}
