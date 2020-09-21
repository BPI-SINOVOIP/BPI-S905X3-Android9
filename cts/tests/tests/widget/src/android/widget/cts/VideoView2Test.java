/*
 * Copyright 2018 The Android Open Source Project
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

package android.widget.cts;

import static android.content.Context.KEYGUARD_SERVICE;

import static junit.framework.Assert.assertEquals;

import static org.mockito.Matchers.same;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.timeout;
import static org.mockito.Mockito.verify;

import android.app.Activity;
import android.app.Instrumentation;
import android.app.KeyguardManager;
import android.content.Context;
import android.media.AudioAttributes;
import android.media.session.MediaController;
import android.media.session.PlaybackState;
import android.support.test.InstrumentationRegistry;
import android.support.test.annotation.UiThreadTest;
import android.support.test.filters.LargeTest;
import android.support.test.rule.ActivityTestRule;
import android.support.test.runner.AndroidJUnit4;
import android.util.Log;
import android.view.View;
import android.view.WindowManager;
import android.widget.VideoView2;

import com.android.compatibility.common.util.MediaUtils;

import org.junit.After;
import org.junit.Before;
import org.junit.Ignore;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.ArgumentCaptor;

import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.util.List;

/**
 * Test {@link VideoView2}.
 */
@Ignore
@LargeTest
@RunWith(AndroidJUnit4.class)
public class VideoView2Test {
    /** Debug TAG. **/
    private static final String TAG = "VideoView2Test";
    /** The maximum time to wait for an operation. */
    private static final long   TIME_OUT = 15000L;
    /** The interval time to wait for completing an operation. */
    private static final long   OPERATION_INTERVAL  = 1500L;
    /** The duration of R.raw.testvideo. */
    private static final int    TEST_VIDEO_DURATION = 11047;
    /** The full name of R.raw.testvideo. */
    private static final String VIDEO_NAME   = "testvideo.3gp";
    /** delta for duration in case user uses different decoders on different
        hardware that report a duration that's different by a few milliseconds */
    private static final int DURATION_DELTA = 100;
    /** AudioAttributes to be used by this player */
    private static final AudioAttributes AUDIO_ATTR = new AudioAttributes.Builder()
            .setUsage(AudioAttributes.USAGE_GAME)
            .setContentType(AudioAttributes.CONTENT_TYPE_SONIFICATION)
            .build();
    private Instrumentation mInstrumentation;
    private Activity mActivity;
    private KeyguardManager mKeyguardManager;
    private VideoView2 mVideoView;
    private MediaController mController;
    private String mVideoPath;

    @Rule
    public ActivityTestRule<VideoView2CtsActivity> mActivityRule =
            new ActivityTestRule<>(VideoView2CtsActivity.class);

    @Before
    public void setup() throws Throwable {
        mInstrumentation = InstrumentationRegistry.getInstrumentation();
        mKeyguardManager = (KeyguardManager)
                mInstrumentation.getTargetContext().getSystemService(KEYGUARD_SERVICE);
        mActivity = mActivityRule.getActivity();
        mVideoView = (VideoView2) mActivity.findViewById(R.id.videoview);
        mVideoPath = prepareSampleVideo();

        mActivityRule.runOnUiThread(() -> {
            // Keep screen on while testing.
            mActivity.getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
            mActivity.setTurnScreenOn(true);
            mActivity.setShowWhenLocked(true);
            mKeyguardManager.requestDismissKeyguard(mActivity, null);
        });
        mInstrumentation.waitForIdleSync();

        final View.OnAttachStateChangeListener mockAttachListener =
                mock(View.OnAttachStateChangeListener.class);
        if (!mVideoView.isAttachedToWindow()) {
            mVideoView.addOnAttachStateChangeListener(mockAttachListener);
            verify(mockAttachListener, timeout(TIME_OUT)).onViewAttachedToWindow(same(mVideoView));
        }
        mController = mVideoView.getMediaController();
    }

    @After
    public void tearDown() throws Throwable {
        /** call media controller's stop */
    }

    private boolean hasCodec() {
        return MediaUtils.hasCodecsForResource(mActivity, R.raw.testvideo);
    }

    private String prepareSampleVideo() throws IOException {
        try (InputStream source = mActivity.getResources().openRawResource(R.raw.testvideo);
             OutputStream target = mActivity.openFileOutput(VIDEO_NAME, Context.MODE_PRIVATE)) {
            final byte[] buffer = new byte[1024];
            for (int len = source.read(buffer); len > 0; len = source.read(buffer)) {
                target.write(buffer, 0, len);
            }
        }

        return mActivity.getFileStreamPath(VIDEO_NAME).getAbsolutePath();
    }

    @UiThreadTest
    @Test
    public void testConstructor() {
        new VideoView2(mActivity);
        new VideoView2(mActivity, null);
        new VideoView2(mActivity, null, 0);
    }

    @Test
    public void testPlayVideo() throws Throwable {
        // Don't run the test if the codec isn't supported.
        if (!hasCodec()) {
            Log.i(TAG, "SKIPPING testPlayVideo(): codec is not supported");
            return;
        }
        final MediaController.Callback mockControllerCallback =
                mock(MediaController.Callback.class);
        mActivityRule.runOnUiThread(() -> {
            mController.registerCallback(mockControllerCallback);
            mVideoView.setVideoPath(mVideoPath);
            mController.getTransportControls().play();
        });
        ArgumentCaptor<PlaybackState> someState = ArgumentCaptor.forClass(PlaybackState.class);
        verify(mockControllerCallback, timeout(TIME_OUT).atLeast(3)).onPlaybackStateChanged(
                someState.capture());
        List<PlaybackState> states = someState.getAllValues();
        assertEquals(PlaybackState.STATE_PAUSED, states.get(0).getState());
        assertEquals(PlaybackState.STATE_PLAYING, states.get(1).getState());
        assertEquals(PlaybackState.STATE_STOPPED, states.get(2).getState());
    }

    @Test
    public void testPlayVideoOnTextureView() throws Throwable {
        // Don't run the test if the codec isn't supported.
        if (!hasCodec()) {
            Log.i(TAG, "SKIPPING testPlayVideoOnTextureView(): codec is not supported");
            return;
        }
        final VideoView2.OnViewTypeChangedListener mockViewTypeListener =
                mock(VideoView2.OnViewTypeChangedListener.class);
        final MediaController.Callback mockControllerCallback =
                mock(MediaController.Callback.class);
        mActivityRule.runOnUiThread(() -> {
            mVideoView.setOnViewTypeChangedListener(mockViewTypeListener);
            mVideoView.setViewType(mVideoView.VIEW_TYPE_TEXTUREVIEW);
            mController.registerCallback(mockControllerCallback);
            mVideoView.setVideoPath(mVideoPath);
        });
        verify(mockViewTypeListener, timeout(TIME_OUT))
                .onViewTypeChanged(mVideoView, VideoView2.VIEW_TYPE_TEXTUREVIEW);

        mActivityRule.runOnUiThread(() -> {
            mController.getTransportControls().play();
        });
        ArgumentCaptor<PlaybackState> someState = ArgumentCaptor.forClass(PlaybackState.class);
        verify(mockControllerCallback, timeout(TIME_OUT).atLeast(3)).onPlaybackStateChanged(
                someState.capture());
        List<PlaybackState> states = someState.getAllValues();
        assertEquals(PlaybackState.STATE_PAUSED, states.get(0).getState());
        assertEquals(PlaybackState.STATE_PLAYING, states.get(1).getState());
        assertEquals(PlaybackState.STATE_STOPPED, states.get(2).getState());
    }
}
