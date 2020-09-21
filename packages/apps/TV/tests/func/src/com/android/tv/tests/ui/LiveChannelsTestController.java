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
package com.android.tv.tests.ui;

import static junit.framework.Assert.assertTrue;

import android.app.Instrumentation;
import android.app.UiAutomation;
import android.content.Context;
import android.content.res.Resources;
import android.os.Build;
import android.os.SystemClock;
import android.support.test.InstrumentationRegistry;
import android.support.test.uiautomator.BySelector;
import android.support.test.uiautomator.Configurator;
import android.support.test.uiautomator.SearchCondition;
import android.support.test.uiautomator.UiDevice;
import android.support.test.uiautomator.Until;
import android.view.InputDevice;
import android.view.KeyEvent;
import com.android.tv.testing.data.ChannelInfo;
import com.android.tv.testing.testinput.ChannelStateData;
import com.android.tv.testing.testinput.TestInputControlConnection;
import com.android.tv.testing.testinput.TestInputControlUtils;
import com.android.tv.testing.uihelper.Constants;
import com.android.tv.testing.uihelper.LiveChannelsUiDeviceHelper;
import com.android.tv.testing.uihelper.MenuHelper;
import com.android.tv.testing.uihelper.SidePanelHelper;
import com.android.tv.testing.uihelper.UiDeviceAsserts;
import com.android.tv.testing.uihelper.UiDeviceUtils;
import org.junit.rules.ExternalResource;

/** UIHelpers and TestInputController for LiveChannels. */
public class LiveChannelsTestController extends ExternalResource {

    private final TestInputControlConnection mConnection = new TestInputControlConnection();

    public MenuHelper menuHelper;
    public SidePanelHelper sidePanelHelper;
    public LiveChannelsUiDeviceHelper liveChannelsHelper;

    private UiDevice mDevice;
    private Resources mTargetResources;
    private Instrumentation mInstrumentation;

    private void assertPressKeyUp(int keyCode, boolean sync) {
        assertPressKey(KeyEvent.ACTION_UP, keyCode, sync);
    }

    private void assertPressKey(int action, int keyCode, boolean sync) {
        long eventTime = SystemClock.uptimeMillis();
        KeyEvent event =
                new KeyEvent(
                        eventTime,
                        eventTime,
                        action,
                        keyCode,
                        0,
                        0,
                        -1,
                        0,
                        0,
                        InputDevice.SOURCE_KEYBOARD);
        assertTrue("Failed to inject key up event:" + event, injectEvent(event, sync));
    }

    private UiAutomation getUiAutomation() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.N) {
            int flags = Configurator.getInstance().getUiAutomationFlags();
            return mInstrumentation.getUiAutomation(flags);
        } else {
            return mInstrumentation.getUiAutomation();
        }
    }

    @Override
    protected void before() throws Exception {
        mInstrumentation = InstrumentationRegistry.getInstrumentation();
        Context context = mInstrumentation.getContext();
        context.bindService(
                TestInputControlUtils.createIntent(), mConnection, Context.BIND_AUTO_CREATE);
        mDevice = UiDevice.getInstance(mInstrumentation);
        mTargetResources = mInstrumentation.getTargetContext().getResources();
        menuHelper = new MenuHelper(mDevice, mTargetResources);
        sidePanelHelper = new SidePanelHelper(mDevice, mTargetResources);
        liveChannelsHelper = new LiveChannelsUiDeviceHelper(mDevice, mTargetResources, context);
    }

    @Override
    protected void after() {
        if (mConnection.isBound()) {
            mInstrumentation.getContext().unbindService(mConnection);
        }

        // TODO: robustly handle left over pops from failed tests.
        // Clear any side panel, menu, ...
        // Scene container should not be checked here because pressing the BACK key in some scenes
        // might launch the home screen.
        if (mDevice.hasObject(Constants.SIDE_PANEL)
                || mDevice.hasObject(Constants.MENU)
                || mDevice.hasObject(Constants.PROGRAM_GUIDE)) {
            mDevice.pressBack();
        }
        // To destroy the activity to make sure next test case's activity launch check works well.
        mDevice.pressBack();
    }

    /**
     * Update the channel state to {@code data} then tune to that channel.
     *
     * @param data the state to update the channel with.
     * @param channel the channel to tune to
     */
    protected void updateThenTune(ChannelStateData data, ChannelInfo channel) {
        mConnection.updateChannelState(channel, data);
        pressKeysForChannel(channel);
    }

    /**
     * Send the keys for the channel number of {@code channel} and press the DPAD center.
     *
     * <p>Usually this will tune to the given channel.
     */
    public void pressKeysForChannel(ChannelInfo channel) {
        UiDeviceUtils.pressKeys(mDevice, channel.number);
        UiDeviceAsserts.assertWaitForCondition(
                mDevice, Until.hasObject(Constants.KEYPAD_CHANNEL_SWITCH));
        mDevice.pressDPadCenter();
    }

    public void assertHas(BySelector bySelector, boolean expected) {
        UiDeviceAsserts.assertHas(mDevice, bySelector, expected);
    }

    public void assertWaitForCondition(SearchCondition<Boolean> booleanSearchCondition) {
        UiDeviceAsserts.assertWaitForCondition(mDevice, booleanSearchCondition);
    }

    public void assertWaitForCondition(SearchCondition<Boolean> gone, long timeout) {
        UiDeviceAsserts.assertWaitForCondition(mDevice, gone, timeout);
    }

    public void assertWaitUntilFocused(BySelector bySelector) {
        UiDeviceAsserts.assertWaitUntilFocused(mDevice, bySelector);
    }

    public Resources getTargetResources() {
        return mTargetResources;
    }

    public UiDevice getUiDevice() {
        return mDevice;
    }

    public boolean injectEvent(KeyEvent event, boolean sync) {
        return getUiAutomation().injectInputEvent(event, sync);
    }

    public void pressBack() {
        mDevice.pressBack();
    }

    public void pressDPadCenter() {
        pressDPadCenter(1);
    }

    public void pressDPadCenter(int repeat) {
        UiDevice.getInstance(mInstrumentation).waitForIdle();
        for (int i = 0; i < repeat; ++i) {
            assertPressKey(KeyEvent.ACTION_DOWN, KeyEvent.KEYCODE_DPAD_CENTER, false);
            if (i < repeat - 1) {
                assertPressKeyUp(KeyEvent.KEYCODE_DPAD_CENTER, false);
            }
        }
        // Send last key event synchronously.
        assertPressKeyUp(KeyEvent.KEYCODE_DPAD_CENTER, true);
    }

    public void pressDPadDown() {
        mDevice.pressDPadDown();
    }

    public void pressDPadLeft() {
        mDevice.pressDPadLeft();
    }

    public void pressDPadRight() {
        mDevice.pressDPadRight();
    }

    public void pressDPadUp() {
        mDevice.pressDPadUp();
    }

    public void pressEnter() {
        mDevice.pressEnter();
    }

    public void pressHome() {
        mDevice.pressHome();
    }

    public void pressKeyCode(int keyCode) {
        mDevice.pressKeyCode(keyCode);
    }

    public void pressMenu() {
        mDevice.pressMenu();
    }

    public void waitForIdle() {
        mDevice.waitForIdle();
    }

    public void waitForIdleSync() {
        mInstrumentation.waitForIdleSync();
    }
}
