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

package com.android.bluetooth.a2dpsink;

import static org.mockito.Mockito.*;

import android.content.Context;
import android.content.pm.PackageManager;
import android.content.res.Resources;
import android.media.AudioManager;
import android.os.HandlerThread;
import android.os.Looper;
import android.support.test.InstrumentationRegistry;
import android.support.test.filters.MediumTest;
import android.support.test.runner.AndroidJUnit4;

import com.android.bluetooth.R;

import org.junit.Assume;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;

@MediumTest
@RunWith(AndroidJUnit4.class)
public class A2dpSinkStreamHandlerTest {
    private static final int DUCK_PERCENT = 75;
    private HandlerThread mHandlerThread;
    private A2dpSinkStreamHandler mStreamHandler;
    private Context mTargetContext;

    @Mock private Context mMockContext;

    @Mock private A2dpSinkStateMachine mMockA2dpSink;

    @Mock private AudioManager mMockAudioManager;

    @Mock private Resources mMockResources;

    @Mock private PackageManager mMockPackageManager;

    @Before
    public void setUp() {
        mTargetContext = InstrumentationRegistry.getTargetContext();
        Assume.assumeTrue("Ignore test when A2dpSinkService is not enabled",
                mTargetContext.getResources().getBoolean(R.bool.profile_supported_a2dp_sink));
        MockitoAnnotations.initMocks(this);
        // Mock the looper
        if (Looper.myLooper() == null) {
            Looper.prepare();
        }

        mHandlerThread = new HandlerThread("A2dpSinkStreamHandlerTest");
        mHandlerThread.start();

        when(mMockContext.getSystemService(Context.AUDIO_SERVICE)).thenReturn(mMockAudioManager);
        when(mMockContext.getResources()).thenReturn(mMockResources);
        when(mMockResources.getInteger(anyInt())).thenReturn(DUCK_PERCENT);
        when(mMockAudioManager.requestAudioFocus(any())).thenReturn(
                AudioManager.AUDIOFOCUS_REQUEST_GRANTED);
        when(mMockAudioManager.abandonAudioFocus(any())).thenReturn(AudioManager.AUDIOFOCUS_GAIN);
        doNothing().when(mMockA2dpSink).informAudioTrackGainNative(anyFloat());
        when(mMockContext.getMainLooper()).thenReturn(mHandlerThread.getLooper());
        when(mMockContext.getPackageManager()).thenReturn(mMockPackageManager);
        when(mMockPackageManager.hasSystemFeature(any())).thenReturn(false);

        mStreamHandler = spy(new A2dpSinkStreamHandler(mMockA2dpSink, mMockContext));
    }

    @Test
    public void testSrcStart() {
        // Stream started without local play, expect no change in streaming.
        mStreamHandler.handleMessage(
                mStreamHandler.obtainMessage(A2dpSinkStreamHandler.SRC_STR_START));
        verify(mMockAudioManager, times(0)).requestAudioFocus(any());
        verify(mMockA2dpSink, times(0)).informAudioFocusStateNative(1);
        verify(mMockA2dpSink, times(0)).informAudioTrackGainNative(1.0f);
    }

    @Test
    public void testSrcStop() {
        // Stream stopped without local play, expect no change in streaming.
        mStreamHandler.handleMessage(
                mStreamHandler.obtainMessage(A2dpSinkStreamHandler.SRC_STR_STOP));
        verify(mMockAudioManager, times(0)).requestAudioFocus(any());
        verify(mMockA2dpSink, times(0)).informAudioFocusStateNative(1);
        verify(mMockA2dpSink, times(0)).informAudioTrackGainNative(1.0f);
    }

    @Test
    public void testSnkPlay() {
        // Play was pressed locally, expect streaming to start.
        mStreamHandler.handleMessage(mStreamHandler.obtainMessage(A2dpSinkStreamHandler.SNK_PLAY));
        verify(mMockAudioManager, times(1)).requestAudioFocus(any());
        verify(mMockA2dpSink, times(1)).informAudioFocusStateNative(1);
        verify(mMockA2dpSink, times(1)).informAudioTrackGainNative(1.0f);
    }

    @Test
    public void testSnkPause() {
        // Pause was pressed locally, expect streaming to stop.
        mStreamHandler.handleMessage(mStreamHandler.obtainMessage(A2dpSinkStreamHandler.SNK_PAUSE));
        verify(mMockAudioManager, times(0)).requestAudioFocus(any());
        verify(mMockA2dpSink, times(0)).informAudioFocusStateNative(1);
        verify(mMockA2dpSink, times(0)).informAudioTrackGainNative(1.0f);
    }

    @Test
    public void testDisconnect() {
        // Remote device was disconnected, expect streaming to stop.
        testSnkPlay();
        mStreamHandler.handleMessage(
                mStreamHandler.obtainMessage(A2dpSinkStreamHandler.DISCONNECT));
        verify(mMockAudioManager, times(0)).abandonAudioFocus(any());
        verify(mMockA2dpSink, times(0)).informAudioFocusStateNative(0);
    }

    @Test
    public void testSrcPlay() {
        // Play was pressed remotely, expect no streaming due to lack of audio focus.
        mStreamHandler.handleMessage(mStreamHandler.obtainMessage(A2dpSinkStreamHandler.SRC_PLAY));
        verify(mMockAudioManager, times(0)).requestAudioFocus(any());
        verify(mMockA2dpSink, times(0)).informAudioFocusStateNative(1);
        verify(mMockA2dpSink, times(0)).informAudioTrackGainNative(1.0f);
    }

    @Test
    public void testSrcPlayIot() {
        // Play was pressed remotely for an iot device, expect streaming to start.
        when(mMockPackageManager.hasSystemFeature(any())).thenReturn(true);
        mStreamHandler.handleMessage(mStreamHandler.obtainMessage(A2dpSinkStreamHandler.SRC_PLAY));
        verify(mMockAudioManager, times(1)).requestAudioFocus(any());
        verify(mMockA2dpSink, times(1)).informAudioFocusStateNative(1);
        verify(mMockA2dpSink, times(1)).informAudioTrackGainNative(1.0f);
    }

    @Test
    public void testSrcPause() {
        // Play was pressed locally, expect streaming to start.
        mStreamHandler.handleMessage(mStreamHandler.obtainMessage(A2dpSinkStreamHandler.SRC_PLAY));
        verify(mMockAudioManager, times(0)).requestAudioFocus(any());
        verify(mMockA2dpSink, times(0)).informAudioFocusStateNative(1);
        verify(mMockA2dpSink, times(0)).informAudioTrackGainNative(1.0f);
    }

    @Test
    public void testFocusGain() {
        // Focus was gained, expect streaming to resume.
        testSnkPlay();
        mStreamHandler.handleMessage(
                mStreamHandler.obtainMessage(A2dpSinkStreamHandler.AUDIO_FOCUS_CHANGE,
                        AudioManager.AUDIOFOCUS_GAIN));
        verify(mMockAudioManager, times(1)).requestAudioFocus(any());
        verify(mMockA2dpSink, times(2)).informAudioFocusStateNative(1);
        verify(mMockA2dpSink, times(2)).informAudioTrackGainNative(1.0f);
    }

    @Test
    public void testFocusTransientMayDuck() {
        // TransientMayDuck focus was gained, expect audio stream to duck.
        testSnkPlay();
        mStreamHandler.handleMessage(
                mStreamHandler.obtainMessage(A2dpSinkStreamHandler.AUDIO_FOCUS_CHANGE,
                        AudioManager.AUDIOFOCUS_LOSS_TRANSIENT_CAN_DUCK));
        verify(mMockA2dpSink, times(1)).informAudioTrackGainNative(DUCK_PERCENT / 100.0f);
    }

    @Test
    public void testFocusLostTransient() {
        // Focus was lost transiently, expect streaming to stop.
        testSnkPlay();
        mStreamHandler.handleMessage(
                mStreamHandler.obtainMessage(A2dpSinkStreamHandler.AUDIO_FOCUS_CHANGE,
                        AudioManager.AUDIOFOCUS_LOSS_TRANSIENT));
        verify(mMockAudioManager, times(0)).abandonAudioFocus(any());
        verify(mMockA2dpSink, times(1)).informAudioFocusStateNative(0);
    }

    @Test
    public void testFocusLost() {
        // Focus was lost permanently, expect streaming to stop.
        testSnkPlay();
        mStreamHandler.handleMessage(
                mStreamHandler.obtainMessage(A2dpSinkStreamHandler.AUDIO_FOCUS_CHANGE,
                        AudioManager.AUDIOFOCUS_LOSS));
        verify(mMockAudioManager, times(1)).abandonAudioFocus(any());
        verify(mMockA2dpSink, times(1)).informAudioFocusStateNative(0);
    }
}
