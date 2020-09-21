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
 * limitations under the License
 */

package com.android.server.telecom.tests;

import static com.android.server.telecom.tests.TelecomSystemTest.TEST_TIMEOUT;

import static junit.framework.Assert.assertFalse;
import static junit.framework.Assert.assertTrue;

import static org.mockito.ArgumentMatchers.any;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import android.content.ComponentName;
import android.media.AudioFormat;
import android.media.AudioManager;
import android.media.AudioRecordingConfiguration;
import android.media.MediaRecorder;
import android.os.Handler;
import android.os.Looper;
import android.telecom.PhoneAccountHandle;
import android.test.suitebuilder.annotation.MediumTest;

import com.android.server.telecom.Call;
import com.android.server.telecom.CallRecordingTonePlayer;
import com.android.server.telecom.TelecomSystem;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;
import org.mockito.ArgumentCaptor;
import org.mockito.Mock;
import org.mockito.Mockito;
import org.mockito.MockitoAnnotations;

import java.util.ArrayList;
import java.util.List;

/**
 * Unit tests for the {@link com.android.server.telecom.CallRecordingTonePlayer} class.
 */
@RunWith(JUnit4.class)
public class CallRecordingTonePlayerTest extends TelecomTestCase {

    private static final String PHONE_ACCOUNT_PACKAGE = "com.android.telecom.test";
    private static final String PHONE_ACCOUNT_CLASS = "MyFancyConnectionService";
    private static final String PHONE_ACCOUNT_ID = "1";
    private static final String RECORDING_APP_PACKAGE = "com.recording.app";

    private static final PhoneAccountHandle TEST_PHONE_ACCOUNT = new PhoneAccountHandle(
            new ComponentName(PHONE_ACCOUNT_PACKAGE, PHONE_ACCOUNT_CLASS), PHONE_ACCOUNT_ID);

    private CallRecordingTonePlayer mCallRecordingTonePlayer;
    private TelecomSystem.SyncRoot mSyncRoot = new TelecomSystem.SyncRoot() { };
    @Mock private AudioManager mAudioManager;

    @Override
    @Before
    public void setUp() throws Exception {
        super.setUp();
        MockitoAnnotations.initMocks(this);
        mCallRecordingTonePlayer = new CallRecordingTonePlayer(
                mComponentContextFixture.getTestDouble().getApplicationContext(),
                mAudioManager, mSyncRoot);
        when(mAudioManager.getActiveRecordingConfigurations()).thenReturn(null);
    }

    /**
     * Ensures that child calls are not tracked.
     */
    @MediumTest
    @Test
    public void testChildCall() {
        Call childCall = Mockito.mock(Call.class);
        Call parentcall = Mockito.mock(Call.class);
        when(childCall.getParentCall()).thenReturn(parentcall);
        mCallRecordingTonePlayer.onCallAdded(childCall);

        assertFalse(mCallRecordingTonePlayer.hasCalls());
    }

    /**
     * Ensures that external calls are not tracked.
     */
    @MediumTest
    @Test
    public void testAddExternalCall() {
        Call call = Mockito.mock(Call.class);
        when(call.getParentCall()).thenReturn(null);
        when(call.isExternalCall()).thenReturn(true);
        mCallRecordingTonePlayer.onCallAdded(call);

        assertFalse(mCallRecordingTonePlayer.hasCalls());
    }

    /**
     * Ensures that emergency calls are not tracked.
     */
    @MediumTest
    @Test
    public void testAddEmergencyCall() {
        Call call = Mockito.mock(Call.class);
        when(call.getParentCall()).thenReturn(null);
        when(call.isExternalCall()).thenReturn(false);
        when(call.isEmergencyCall()).thenReturn(true);
        mCallRecordingTonePlayer.onCallAdded(call);

        assertFalse(mCallRecordingTonePlayer.hasCalls());
    }

    /**
     * Ensures that calls which don't use the recording tone are not tracked.
     */
    @MediumTest
    @Test
    public void testAddIneligibleCall() {
        Call call = Mockito.mock(Call.class);
        when(call.getParentCall()).thenReturn(null);
        when(call.isExternalCall()).thenReturn(false);
        when(call.isEmergencyCall()).thenReturn(false);
        when(call.isUsingCallRecordingTone()).thenReturn(false);
        mCallRecordingTonePlayer.onCallAdded(call);

        assertFalse(mCallRecordingTonePlayer.hasCalls());
    }

    /**
     * Ensures that an eligible call is tracked.
     */
    @MediumTest
    @Test
    public void testAddEligibleCall() {
        Call call = addValidCall();

        mCallRecordingTonePlayer.onCallRemoved(call);
        assertFalse(mCallRecordingTonePlayer.hasCalls());
    }

    /**
     * Verifies registration and unregistration of the recording callback.
     */
    @MediumTest
    @Test
    public void testRecordingCallbackRegistered() {
        Call call = addValidCall();

        // Ensure we got a request for the first set of recordings.
        verify(mAudioManager).getActiveRecordingConfigurations();

        // Ensure that we registered an audio recording callback.
        verify(mAudioManager).registerAudioRecordingCallback(
                any(AudioManager.AudioRecordingCallback.class), any());

        mCallRecordingTonePlayer.onCallRemoved(call);

        // Ensure we unregistered the audio recording callback after the last call was removed.
        verify(mAudioManager).unregisterAudioRecordingCallback(
                any(AudioManager.AudioRecordingCallback.class));
    }

    /**
     * Verify that we are in a recording state when we add a call and there is a recording taking
     * place prior to the call starting.
     */
    @MediumTest
    @Test
    public void testIsRecordingInitial() {
        // Return an active recording configuration when we add the first call.
        when(mAudioManager.getActiveRecordingConfigurations()).thenReturn(
                getAudioRecordingConfig(RECORDING_APP_PACKAGE));

        addValidCall();

        // Ensure we got a request for the first set of recordings.
        verify(mAudioManager).getActiveRecordingConfigurations();

        assertTrue(mCallRecordingTonePlayer.isRecording());
    }

    /**
     * Verify that we are in a recording state when we add a call and a recording start after the
     * call starts.
     */
    @MediumTest
    @Test
    public void testIsRecordingLater() {
        // Return no active recording configuration when we add the first call.
        when(mAudioManager.getActiveRecordingConfigurations()).thenReturn( null);

        addValidCall();

        // Capture the registered callback so we can pass back test data via it.
        ArgumentCaptor<AudioManager.AudioRecordingCallback> callbackCaptor =
                ArgumentCaptor.forClass(AudioManager.AudioRecordingCallback.class);
        verify(mAudioManager).registerAudioRecordingCallback(callbackCaptor.capture(), any());

        // Pass back some test configuration data.
        callbackCaptor.getValue().onRecordingConfigChanged(getAudioRecordingConfig(
                RECORDING_APP_PACKAGE));
        waitForHandlerAction(new Handler(Looper.getMainLooper()), TEST_TIMEOUT);

        assertTrue(mCallRecordingTonePlayer.isRecording());
    }

    /**
     * Verifies that we are not in a recording state if the PhoneAccount associated with the call is
     * the recording app.
     */
    @MediumTest
    @Test
    public void testNotRecordingApp() {
        // Return no active recording configuration when we add the first call.
        when(mAudioManager.getActiveRecordingConfigurations()).thenReturn( null);

        addValidCall();

        // Capture the registered callback so we can pass back test data via it.
        ArgumentCaptor<AudioManager.AudioRecordingCallback> callbackCaptor =
                ArgumentCaptor.forClass(AudioManager.AudioRecordingCallback.class);
        verify(mAudioManager).registerAudioRecordingCallback(callbackCaptor.capture(), any());

        // Report that the recording app is the call's phone account.
        callbackCaptor.getValue().onRecordingConfigChanged(getAudioRecordingConfig(
                PHONE_ACCOUNT_PACKAGE));
        waitForHandlerAction(new Handler(Looper.getMainLooper()), TEST_TIMEOUT);

        // Since the app which is recording is the phone account of the call, we should not be in
        // a recording state.
        assertFalse(mCallRecordingTonePlayer.isRecording());
    }

    /**
     * @return Test audio recording configuration.
     */
    private List<AudioRecordingConfiguration> getAudioRecordingConfig(String packageName) {
        List<AudioRecordingConfiguration> configs = new ArrayList<>();
        configs.add(new AudioRecordingConfiguration(0, 0, MediaRecorder.AudioSource.MIC,
                new AudioFormat.Builder().build(), new AudioFormat.Builder().build(),
                0, packageName));
        return configs;
    }

    private Call addValidCall() {
        Call call = Mockito.mock(Call.class);
        when(call.getParentCall()).thenReturn(null);
        when(call.isExternalCall()).thenReturn(false);
        when(call.isEmergencyCall()).thenReturn(false);
        when(call.isUsingCallRecordingTone()).thenReturn(true);
        when(call.getConnectionManagerPhoneAccount()).thenReturn(null);
        when(call.getTargetPhoneAccount()).thenReturn(TEST_PHONE_ACCOUNT);
        mCallRecordingTonePlayer.onCallAdded(call);
        assertTrue(mCallRecordingTonePlayer.hasCalls());
        return call;
    }


}
