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
package com.android.server.usb;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNotEquals;
import static org.junit.Assert.assertTrue;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import android.app.ActivityManager;
import android.content.ContentResolver;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.hardware.usb.UsbManager;
import android.os.Handler;
import android.os.Looper;
import android.os.Message;
import android.os.UserHandle;
import android.provider.Settings;
import android.support.test.InstrumentationRegistry;
import android.support.test.filters.SmallTest;
import android.support.test.runner.AndroidJUnit4;

import com.android.server.FgThread;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;

import java.util.HashMap;
import java.util.Locale;
import java.util.Map;

/**
 * Tests for UsbHandler state changes.
 */
@RunWith(AndroidJUnit4.class)
public class UsbHandlerTest {
    private static final String TAG = UsbHandlerTest.class.getSimpleName();

    @Mock
    private UsbDeviceManager mUsbDeviceManager;
    @Mock
    private UsbDebuggingManager mUsbDebuggingManager;
    @Mock
    private UsbAlsaManager mUsbAlsaManager;
    @Mock
    private UsbSettingsManager mUsbSettingsManager;
    @Mock
    private SharedPreferences mSharedPreferences;
    @Mock
    private SharedPreferences.Editor mEditor;

    private MockUsbHandler mUsbHandler;

    private static final int MSG_UPDATE_STATE = 0;
    private static final int MSG_ENABLE_ADB = 1;
    private static final int MSG_SET_CURRENT_FUNCTIONS = 2;
    private static final int MSG_SYSTEM_READY = 3;
    private static final int MSG_BOOT_COMPLETED = 4;
    private static final int MSG_USER_SWITCHED = 5;
    private static final int MSG_UPDATE_USER_RESTRICTIONS = 6;
    private static final int MSG_SET_SCREEN_UNLOCKED_FUNCTIONS = 12;
    private static final int MSG_UPDATE_SCREEN_LOCK = 13;

    private Map<String, String> mMockProperties;
    private Map<String, Integer> mMockGlobalSettings;

    private class MockUsbHandler extends UsbDeviceManager.UsbHandler {
        boolean mIsUsbTransferAllowed;
        Intent mBroadcastedIntent;

        MockUsbHandler(Looper looper, Context context, UsbDeviceManager deviceManager,
                UsbDebuggingManager debuggingManager, UsbAlsaManager alsaManager,
                UsbSettingsManager settingsManager) {
            super(looper, context, deviceManager, debuggingManager, alsaManager, settingsManager);
            mUseUsbNotification = false;
            mIsUsbTransferAllowed = true;
            mCurrentUsbFunctionsReceived = true;
        }

        @Override
        protected void setEnabledFunctions(long functions, boolean force) {
            mCurrentFunctions = functions;
        }

        @Override
        protected void setSystemProperty(String property, String value) {
            mMockProperties.put(property, value);
        }

        @Override
        protected void putGlobalSettings(ContentResolver resolver, String setting, int val) {
            mMockGlobalSettings.put(setting, val);
        }

        @Override
        protected String getSystemProperty(String property, String def) {
            if (mMockProperties.containsKey(property)) {
                return mMockProperties.get(property);
            }
            return def;
        }

        @Override
        protected boolean isUsbTransferAllowed() {
            return mIsUsbTransferAllowed;
        }

        @Override
        protected SharedPreferences getPinnedSharedPrefs(Context context) {
            return mSharedPreferences;
        }

        @Override
        protected void sendStickyBroadcast(Intent intent) {
            mBroadcastedIntent = intent;
        }
    }

    @Before
    public void before() {
        MockitoAnnotations.initMocks(this);
        mMockProperties = new HashMap<>();
        mMockGlobalSettings = new HashMap<>();
        when(mSharedPreferences.edit()).thenReturn(mEditor);

        mUsbHandler = new MockUsbHandler(FgThread.get().getLooper(),
                InstrumentationRegistry.getContext(), mUsbDeviceManager, mUsbDebuggingManager,
                mUsbAlsaManager, mUsbSettingsManager);
    }

    @SmallTest
    @Test
    public void setFunctionsMtp() {
        mUsbHandler.handleMessage(mUsbHandler.obtainMessage(MSG_SET_CURRENT_FUNCTIONS,
                UsbManager.FUNCTION_MTP));
        assertNotEquals(mUsbHandler.getEnabledFunctions() & UsbManager.FUNCTION_MTP, 0);
    }

    @SmallTest
    @Test
    public void setFunctionsPtp() {
        mUsbHandler.handleMessage(mUsbHandler.obtainMessage(MSG_SET_CURRENT_FUNCTIONS,
                UsbManager.FUNCTION_PTP));
        assertNotEquals(mUsbHandler.getEnabledFunctions() & UsbManager.FUNCTION_PTP, 0);
    }

    @SmallTest
    @Test
    public void setFunctionsMidi() {
        mUsbHandler.handleMessage(mUsbHandler.obtainMessage(MSG_SET_CURRENT_FUNCTIONS,
                UsbManager.FUNCTION_MIDI));
        assertNotEquals(mUsbHandler.getEnabledFunctions() & UsbManager.FUNCTION_MIDI, 0);
    }

    @SmallTest
    @Test
    public void setFunctionsRndis() {
        mUsbHandler.handleMessage(mUsbHandler.obtainMessage(MSG_SET_CURRENT_FUNCTIONS,
                UsbManager.FUNCTION_RNDIS));
        assertNotEquals(mUsbHandler.getEnabledFunctions() & UsbManager.FUNCTION_RNDIS, 0);
    }

    @SmallTest
    @Test
    public void enableAdb() {
        sendBootCompleteMessages(mUsbHandler);
        Message msg = mUsbHandler.obtainMessage(MSG_ENABLE_ADB);
        msg.arg1 = 1;
        mUsbHandler.handleMessage(msg);
        assertEquals(mUsbHandler.getEnabledFunctions(), UsbManager.FUNCTION_NONE);
        assertEquals(mMockProperties.get(UsbDeviceManager.UsbHandler
                .USB_PERSISTENT_CONFIG_PROPERTY), UsbManager.USB_FUNCTION_ADB);
        verify(mUsbDebuggingManager).setAdbEnabled(true);
        assertTrue(mUsbHandler.mAdbEnabled);

        mUsbHandler.handleMessage(mUsbHandler.obtainMessage(MSG_UPDATE_STATE, 1, 1));

        assertTrue(mUsbHandler.mBroadcastedIntent.getBooleanExtra(UsbManager.USB_CONNECTED, false));
        assertTrue(mUsbHandler.mBroadcastedIntent
                .getBooleanExtra(UsbManager.USB_CONFIGURED, false));
        assertTrue(mUsbHandler.mBroadcastedIntent
                .getBooleanExtra(UsbManager.USB_FUNCTION_ADB, false));
    }

    @SmallTest
    @Test
    public void disableAdb() {
        mMockProperties.put(UsbDeviceManager.UsbHandler.USB_PERSISTENT_CONFIG_PROPERTY,
                UsbManager.USB_FUNCTION_ADB);
        mUsbHandler = new MockUsbHandler(FgThread.get().getLooper(),
                InstrumentationRegistry.getContext(), mUsbDeviceManager, mUsbDebuggingManager,
                mUsbAlsaManager, mUsbSettingsManager);

        sendBootCompleteMessages(mUsbHandler);
        mUsbHandler.handleMessage(mUsbHandler.obtainMessage(MSG_ENABLE_ADB, 0));
        assertEquals(mUsbHandler.getEnabledFunctions(), UsbManager.FUNCTION_NONE);
        assertFalse(mUsbHandler.mAdbEnabled);
        assertEquals(mMockProperties.get(UsbDeviceManager.UsbHandler
                .USB_PERSISTENT_CONFIG_PROPERTY), "");
        verify(mUsbDebuggingManager).setAdbEnabled(false);
    }

    @SmallTest
    @Test
    public void bootCompletedCharging() {
        sendBootCompleteMessages(mUsbHandler);
        assertEquals(mUsbHandler.getEnabledFunctions(), UsbManager.FUNCTION_NONE);
    }

    @SmallTest
    @Test
    public void bootCompletedAdbEnabled() {
        mMockProperties.put(UsbDeviceManager.UsbHandler.USB_PERSISTENT_CONFIG_PROPERTY, "adb");
        mUsbHandler = new MockUsbHandler(FgThread.get().getLooper(),
                InstrumentationRegistry.getContext(), mUsbDeviceManager, mUsbDebuggingManager,
                mUsbAlsaManager, mUsbSettingsManager);

        sendBootCompleteMessages(mUsbHandler);
        assertEquals(mUsbHandler.getEnabledFunctions(), UsbManager.FUNCTION_NONE);
        assertEquals(mMockGlobalSettings.get(Settings.Global.ADB_ENABLED).intValue(), 1);
        assertTrue(mUsbHandler.mAdbEnabled);
        verify(mUsbDebuggingManager).setAdbEnabled(true);
    }

    @SmallTest
    @Test
    public void userSwitchedDisablesMtp() {
        mUsbHandler.handleMessage(mUsbHandler.obtainMessage(MSG_SET_CURRENT_FUNCTIONS,
                UsbManager.FUNCTION_MTP));
        assertNotEquals(mUsbHandler.getEnabledFunctions() & UsbManager.FUNCTION_MTP, 0);

        Message msg = mUsbHandler.obtainMessage(MSG_USER_SWITCHED);
        msg.arg1 = ActivityManager.getCurrentUser() + 1;
        mUsbHandler.handleMessage(msg);
        assertEquals(mUsbHandler.getEnabledFunctions(), UsbManager.FUNCTION_NONE);
    }

    @SmallTest
    @Test
    public void changedRestrictionsDisablesMtp() {
        mUsbHandler.handleMessage(mUsbHandler.obtainMessage(MSG_SET_CURRENT_FUNCTIONS,
                UsbManager.FUNCTION_MTP));
        assertNotEquals(mUsbHandler.getEnabledFunctions() & UsbManager.FUNCTION_MTP, 0);

        mUsbHandler.mIsUsbTransferAllowed = false;
        mUsbHandler.handleMessage(mUsbHandler.obtainMessage(MSG_UPDATE_USER_RESTRICTIONS));
        assertEquals(mUsbHandler.getEnabledFunctions(), UsbManager.FUNCTION_NONE);
    }

    @SmallTest
    @Test
    public void disconnectResetsCharging() {
        sendBootCompleteMessages(mUsbHandler);

        mUsbHandler.handleMessage(mUsbHandler.obtainMessage(MSG_SET_CURRENT_FUNCTIONS,
                UsbManager.FUNCTION_MTP));
        assertNotEquals(mUsbHandler.getEnabledFunctions() & UsbManager.FUNCTION_MTP, 0);

        mUsbHandler.handleMessage(mUsbHandler.obtainMessage(MSG_UPDATE_STATE, 0, 0));

        assertEquals(mUsbHandler.getEnabledFunctions(), UsbManager.FUNCTION_NONE);
    }

    @SmallTest
    @Test
    public void configuredSendsBroadcast() {
        sendBootCompleteMessages(mUsbHandler);
        mUsbHandler.handleMessage(mUsbHandler.obtainMessage(MSG_SET_CURRENT_FUNCTIONS,
                UsbManager.FUNCTION_MTP));
        assertNotEquals(mUsbHandler.getEnabledFunctions() & UsbManager.FUNCTION_MTP, 0);

        mUsbHandler.handleMessage(mUsbHandler.obtainMessage(MSG_UPDATE_STATE, 1, 1));

        assertNotEquals(mUsbHandler.getEnabledFunctions() & UsbManager.FUNCTION_MTP, 0);
        assertTrue(mUsbHandler.mBroadcastedIntent.getBooleanExtra(UsbManager.USB_CONNECTED, false));
        assertTrue(mUsbHandler.mBroadcastedIntent
                .getBooleanExtra(UsbManager.USB_CONFIGURED, false));
        assertTrue(mUsbHandler.mBroadcastedIntent
                .getBooleanExtra(UsbManager.USB_FUNCTION_MTP, false));
    }

    @SmallTest
    @Test
    public void setScreenUnlockedFunctions() {
        sendBootCompleteMessages(mUsbHandler);
        mUsbHandler.handleMessage(mUsbHandler.obtainMessage(MSG_UPDATE_SCREEN_LOCK, 0));

        mUsbHandler.handleMessage(mUsbHandler.obtainMessage(MSG_SET_SCREEN_UNLOCKED_FUNCTIONS,
                UsbManager.FUNCTION_MTP));
        assertNotEquals(mUsbHandler.getScreenUnlockedFunctions() & UsbManager.FUNCTION_MTP, 0);
        assertNotEquals(mUsbHandler.getEnabledFunctions() & UsbManager.FUNCTION_MTP, 0);
        verify(mEditor).putString(String.format(Locale.ENGLISH,
                UsbDeviceManager.UNLOCKED_CONFIG_PREF, mUsbHandler.mCurrentUser),
                UsbManager.USB_FUNCTION_MTP);
    }

    @SmallTest
    @Test
    public void unlockScreen() {
        when(mSharedPreferences.getString(String.format(Locale.ENGLISH,
                UsbDeviceManager.UNLOCKED_CONFIG_PREF, mUsbHandler.mCurrentUser), ""))
                .thenReturn(UsbManager.USB_FUNCTION_MTP);
        mUsbHandler = new MockUsbHandler(FgThread.get().getLooper(),
                InstrumentationRegistry.getContext(), mUsbDeviceManager, mUsbDebuggingManager,
                mUsbAlsaManager, mUsbSettingsManager);
        sendBootCompleteMessages(mUsbHandler);
        mUsbHandler.handleMessage(mUsbHandler.obtainMessage(MSG_UPDATE_SCREEN_LOCK, 1));
        mUsbHandler.handleMessage(mUsbHandler.obtainMessage(MSG_UPDATE_SCREEN_LOCK, 0));

        assertNotEquals(mUsbHandler.getScreenUnlockedFunctions() & UsbManager.FUNCTION_MTP, 0);
        assertNotEquals(mUsbHandler.getEnabledFunctions() & UsbManager.FUNCTION_MTP, 0);
    }

    private static void sendBootCompleteMessages(Handler handler) {
        handler.handleMessage(handler.obtainMessage(MSG_BOOT_COMPLETED));
        handler.handleMessage(handler.obtainMessage(MSG_SYSTEM_READY));
    }
}