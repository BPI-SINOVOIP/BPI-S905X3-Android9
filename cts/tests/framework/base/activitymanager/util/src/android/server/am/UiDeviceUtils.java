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

package android.server.am;

import static android.server.am.StateLogger.logE;
import static android.support.test.InstrumentationRegistry.getContext;
import static android.view.KeyEvent.KEYCODE_APP_SWITCH;
import static android.view.KeyEvent.KEYCODE_MENU;
import static android.view.KeyEvent.KEYCODE_SLEEP;
import static android.view.KeyEvent.KEYCODE_WAKEUP;
import static android.view.KeyEvent.KEYCODE_WINDOW;

import android.app.KeyguardManager;
import android.graphics.Point;
import android.os.PowerManager;
import android.os.RemoteException;
import android.os.SystemClock;
import android.support.test.InstrumentationRegistry;
import android.support.test.uiautomator.UiDevice;
import android.util.Log;
import android.view.KeyEvent;

import java.util.function.BooleanSupplier;

/**
 * Helper class to interact with {@link UiDevice}.
 *
 * All references to {@link UiDevice} and {@link KeyEvent} should be here for easy debugging.
 */
public class UiDeviceUtils {

    private static final String TAG = "UiDeviceUtils";
    private static final boolean DEBUG = false;

    static void waitForDeviceIdle(long timeout) {
        if (DEBUG) Log.d(TAG, "waitForDeviceIdle: timeout=" + timeout);
        getDevice().waitForIdle(timeout);
    }

    public static void wakeUpDevice() throws RemoteException {
        if (DEBUG) Log.d(TAG, "wakeUpDevice");
        getDevice().wakeUp();
    }

    public static void dragPointer(Point from, Point to, int steps) {
        if (DEBUG) Log.d(TAG, "dragPointer: from=" + from + " to=" + to + " steps=" + steps);
        getDevice().drag(from.x, from.y, to.x, to.y, steps);
    }

    static void pressEnterButton() {
        if (DEBUG) Log.d(TAG, "pressEnterButton");
        getDevice().pressEnter();
    }

    static void pressHomeButton() {
        if (DEBUG) Log.d(TAG, "pressHomeButton");
        getDevice().pressHome();
    }

    public static void pressBackButton() {
        if (DEBUG) Log.d(TAG, "pressBackButton");
        getDevice().pressBack();
    }

    public static void pressMenuButton() {
        if (DEBUG) Log.d(TAG, "pressMenuButton");
        getDevice().pressMenu();
    }

    static void pressSleepButton() {
        if (DEBUG) Log.d(TAG, "pressSleepButton");
        final PowerManager pm = getContext().getSystemService(PowerManager.class);
        retryPressKeyCode(KEYCODE_SLEEP, () -> pm != null && !pm.isInteractive(),
                "***Waiting for device sleep...");
    }

    static void pressWakeupButton() {
        if (DEBUG) Log.d(TAG, "pressWakeupButton");
        final PowerManager pm = getContext().getSystemService(PowerManager.class);
        retryPressKeyCode(KEYCODE_WAKEUP, () -> pm != null && pm.isInteractive(),
                "***Waiting for device wakeup...");
    }

    static void pressUnlockButton() {
        if (DEBUG) Log.d(TAG, "pressUnlockButton");
        final KeyguardManager kgm = getContext().getSystemService(KeyguardManager.class);
        retryPressKeyCode(KEYCODE_MENU, () -> kgm != null && !kgm.isKeyguardLocked(),
                "***Waiting for device unlock...");
    }

    static void pressWindowButton() {
        if (DEBUG) Log.d(TAG, "pressWindowButton");
        pressKeyCode(KEYCODE_WINDOW);
    }

    static void pressAppSwitchButton() {
        if (DEBUG) Log.d(TAG, "pressAppSwitchButton");
        pressKeyCode(KEYCODE_APP_SWITCH);
    }

    private static void retryPressKeyCode(int keyCode, BooleanSupplier waitFor, String msg) {
        int retry = 1;
        do {
            pressKeyCode(keyCode);
            if (waitFor.getAsBoolean()) {
                return;
            }
            Log.d(TAG, msg + " retry=" + retry);
            SystemClock.sleep(50);
        } while (retry++ < 5);
        if (!waitFor.getAsBoolean()) {
            logE(msg + " FAILED");
        }
    }

    private static void pressKeyCode(int keyCode) {
        getDevice().pressKeyCode(keyCode);
    }

    private static UiDevice getDevice() {
        return UiDevice.getInstance(InstrumentationRegistry.getInstrumentation());
    }
}
