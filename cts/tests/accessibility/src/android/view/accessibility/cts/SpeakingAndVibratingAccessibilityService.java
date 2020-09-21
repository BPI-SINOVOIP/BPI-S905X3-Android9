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

package android.view.accessibility.cts;

import android.accessibilityservice.AccessibilityService;
import android.accessibilityservice.AccessibilityServiceInfo;
import android.view.accessibility.AccessibilityEvent;

/**
 * Stub accessibility service that reports itself as providing multiple feedback types.
 */
public class SpeakingAndVibratingAccessibilityService extends AccessibilityService {
    public static Object sWaitObjectForConnecting = new Object();

    public static SpeakingAndVibratingAccessibilityService sConnectedInstance;

    @Override
    public void onDestroy() {
        sConnectedInstance = null;
    }


    @Override
    protected void onServiceConnected() {
        synchronized (sWaitObjectForConnecting) {
            sConnectedInstance = this;
            sWaitObjectForConnecting.notifyAll();
        }
    }

    @Override
    public void onAccessibilityEvent(AccessibilityEvent event) {
    }

    @Override
    public void onInterrupt() {
    }
}
