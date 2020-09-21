/*
 * Copyright (C) 2011 The Android Open Source Project
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
package android.media.cts;

import android.content.Context;
import android.content.pm.PackageManager;
import android.content.res.AssetFileDescriptor;
import android.content.res.Resources;
import android.media.MediaPlayer;
import android.media.cts.TestUtils.Monitor;
import android.net.Uri;
import android.os.PersistableBundle;
import android.test.ActivityInstrumentationTestCase2;

import com.android.compatibility.common.util.MediaUtils;

import java.io.IOException;
import java.net.HttpCookie;
import java.util.List;
import java.util.logging.Logger;
import java.util.Map;
import java.util.Set;

/**
 * Base class for tests which use MediaPlayer to play audio or video.
 */
public class MediaPlayerTestBase extends ActivityInstrumentationTestCase2<MediaStubActivity> {
    private static final Logger LOG = Logger.getLogger(MediaPlayerTestBase.class.getName());

    protected static final int SLEEP_TIME = 1000;
    protected static final int LONG_SLEEP_TIME = 6000;
    protected static final int STREAM_RETRIES = 20;
    protected static boolean sUseScaleToFitMode = false;

    protected Monitor mOnVideoSizeChangedCalled = new Monitor();
    protected Monitor mOnVideoRenderingStartCalled = new Monitor();
    protected Monitor mOnBufferingUpdateCalled = new Monitor();
    protected Monitor mOnPrepareCalled = new Monitor();
    protected Monitor mOnSeekCompleteCalled = new Monitor();
    protected Monitor mOnCompletionCalled = new Monitor();
    protected Monitor mOnInfoCalled = new Monitor();
    protected Monitor mOnErrorCalled = new Monitor();

    protected Context mContext;
    protected Resources mResources;


    protected MediaPlayer mMediaPlayer = null;
    protected MediaPlayer mMediaPlayer2 = null;
    protected MediaStubActivity mActivity;

    public MediaPlayerTestBase() {
        super(MediaStubActivity.class);
    }

    @Override
    protected void setUp() throws Exception {
        super.setUp();
        mActivity = getActivity();
        getInstrumentation().waitForIdleSync();
        try {
            runTestOnUiThread(new Runnable() {
                public void run() {
                    mMediaPlayer = new MediaPlayer();
                    mMediaPlayer2 = new MediaPlayer();
                }
            });
        } catch (Throwable e) {
            e.printStackTrace();
            fail();
        }
        mContext = getInstrumentation().getTargetContext();
        mResources = mContext.getResources();
    }

    @Override
    protected void tearDown() throws Exception {
        if (mMediaPlayer != null) {
            mMediaPlayer.release();
            mMediaPlayer = null;
        }
        if (mMediaPlayer2 != null) {
            mMediaPlayer2.release();
            mMediaPlayer2 = null;
        }
        mActivity = null;
        super.tearDown();
    }

    // returns true on success
    protected boolean loadResource(int resid) throws Exception {
        if (!MediaUtils.hasCodecsForResource(mContext, resid)) {
            return false;
        }

        AssetFileDescriptor afd = mResources.openRawResourceFd(resid);
        try {
            mMediaPlayer.setDataSource(afd.getFileDescriptor(), afd.getStartOffset(),
                    afd.getLength());

            // Although it is only meant for video playback, it should not
            // cause issues for audio-only playback.
            int videoScalingMode = sUseScaleToFitMode?
                                    MediaPlayer.VIDEO_SCALING_MODE_SCALE_TO_FIT
                                  : MediaPlayer.VIDEO_SCALING_MODE_SCALE_TO_FIT_WITH_CROPPING;

            mMediaPlayer.setVideoScalingMode(videoScalingMode);
        } finally {
            afd.close();
        }
        sUseScaleToFitMode = !sUseScaleToFitMode;  // Alternate the scaling mode
        return true;
    }

    protected boolean checkLoadResource(int resid) throws Exception {
        return MediaUtils.check(loadResource(resid), "no decoder found");
    }

    protected void loadSubtitleSource(int resid) throws Exception {
        AssetFileDescriptor afd = mResources.openRawResourceFd(resid);
        try {
            mMediaPlayer.addTimedTextSource(afd.getFileDescriptor(), afd.getStartOffset(),
                      afd.getLength(), MediaPlayer.MEDIA_MIMETYPE_TEXT_SUBRIP);
        } finally {
            afd.close();
        }
    }

    protected void playLiveVideoTest(String path, int playTime) throws Exception {
        playVideoWithRetries(path, null, null, playTime);
    }

    protected void playLiveAudioOnlyTest(String path, int playTime) throws Exception {
        playVideoWithRetries(path, -1, -1, playTime);
    }

    protected void playVideoTest(String path, int width, int height) throws Exception {
        playVideoWithRetries(path, width, height, 0);
    }

    protected void playVideoWithRetries(String path, Integer width, Integer height, int playTime)
            throws Exception {
        boolean playedSuccessfully = false;
        for (int i = 0; i < STREAM_RETRIES; i++) {
          try {
            mMediaPlayer.setDataSource(path);
            playLoadedVideo(width, height, playTime);
            playedSuccessfully = true;
            break;
          } catch (PrepareFailedException e) {
            // prepare() can fail because of network issues, so try again
            LOG.warning("prepare() failed on try " + i + ", trying playback again");
          }
        }
        assertTrue("Stream did not play successfully after all attempts", playedSuccessfully);
    }

    protected void playVideoTest(int resid, int width, int height) throws Exception {
        if (!checkLoadResource(resid)) {
            return; // skip
        }

        playLoadedVideo(width, height, 0);
    }

    protected void playLiveVideoTest(
            Uri uri, Map<String, String> headers, List<HttpCookie> cookies,
            int playTime) throws Exception {
        playVideoWithRetries(uri, headers, cookies, null /* width */, null /* height */, playTime);
    }

    protected void playVideoWithRetries(
            Uri uri, Map<String, String> headers, List<HttpCookie> cookies,
            Integer width, Integer height, int playTime) throws Exception {
        boolean playedSuccessfully = false;
        for (int i = 0; i < STREAM_RETRIES; i++) {
            try {
                mMediaPlayer.setDataSource(getInstrumentation().getTargetContext(),
                        uri, headers, cookies);
                playLoadedVideo(width, height, playTime);
                playedSuccessfully = true;
                break;
            } catch (PrepareFailedException e) {
                // prepare() can fail because of network issues, so try again
                // playLoadedVideo already has reset the player so we can try again safely.
                LOG.warning("prepare() failed on try " + i + ", trying playback again");
            }
        }
        assertTrue("Stream did not play successfully after all attempts", playedSuccessfully);
    }

    /**
     * Play a video which has already been loaded with setDataSource().
     *
     * @param width width of the video to verify, or null to skip verification
     * @param height height of the video to verify, or null to skip verification
     * @param playTime length of time to play video, or 0 to play entire video.
     * with a non-negative value, this method stops the playback after the length of
     * time or the duration the video is elapsed. With a value of -1,
     * this method simply starts the video and returns immediately without
     * stoping the video playback.
     */
    protected void playLoadedVideo(final Integer width, final Integer height, int playTime)
            throws Exception {
        final float leftVolume = 0.5f;
        final float rightVolume = 0.5f;

        boolean audioOnly = (width != null && width.intValue() == -1) ||
                (height != null && height.intValue() == -1);

        mMediaPlayer.setDisplay(mActivity.getSurfaceHolder());
        mMediaPlayer.setScreenOnWhilePlaying(true);
        mMediaPlayer.setOnVideoSizeChangedListener(new MediaPlayer.OnVideoSizeChangedListener() {
            @Override
            public void onVideoSizeChanged(MediaPlayer mp, int w, int h) {
                if (w == 0 && h == 0) {
                    // A size of 0x0 can be sent initially one time when using NuPlayer.
                    assertFalse(mOnVideoSizeChangedCalled.isSignalled());
                    return;
                }
                mOnVideoSizeChangedCalled.signal();
                if (width != null) {
                    assertEquals(width.intValue(), w);
                }
                if (height != null) {
                    assertEquals(height.intValue(), h);
                }
            }
        });
        mMediaPlayer.setOnErrorListener(new MediaPlayer.OnErrorListener() {
            @Override
            public boolean onError(MediaPlayer mp, int what, int extra) {
                fail("Media player had error " + what + " playing video");
                return true;
            }
        });
        mMediaPlayer.setOnInfoListener(new MediaPlayer.OnInfoListener() {
            @Override
            public boolean onInfo(MediaPlayer mp, int what, int extra) {
                if (what == MediaPlayer.MEDIA_INFO_VIDEO_RENDERING_START) {
                    mOnVideoRenderingStartCalled.signal();
                }
                return true;
            }
        });
        try {
          mMediaPlayer.prepare();
        } catch (IOException e) {
          mMediaPlayer.reset();
          throw new PrepareFailedException();
        }

        mMediaPlayer.start();
        if (!audioOnly) {
            mOnVideoSizeChangedCalled.waitForSignal();
            mOnVideoRenderingStartCalled.waitForSignal();
        }
        mMediaPlayer.setVolume(leftVolume, rightVolume);

        // waiting to complete
        if (playTime == -1) {
            return;
        } else if (playTime == 0) {
            while (mMediaPlayer.isPlaying()) {
                Thread.sleep(SLEEP_TIME);
            }
        } else {
            Thread.sleep(playTime);
        }

        // validate a few MediaMetrics.
        PersistableBundle metrics = mMediaPlayer.getMetrics();
        if (metrics == null) {
            fail("MediaPlayer.getMetrics() returned null metrics");
        } else if (metrics.isEmpty()) {
            fail("MediaPlayer.getMetrics() returned empty metrics");
        } else {

            int size = metrics.size();
            Set<String> keys = metrics.keySet();

            if (keys == null) {
                fail("MediaMetricsSet returned no keys");
            } else if (keys.size() != size) {
                fail("MediaMetricsSet.keys().size() mismatch MediaMetricsSet.size()");
            }

            // we played something; so one of these should be non-null
            String vmime = metrics.getString(MediaPlayer.MetricsConstants.MIME_TYPE_VIDEO, null);
            String amime = metrics.getString(MediaPlayer.MetricsConstants.MIME_TYPE_AUDIO, null);
            if (vmime == null && amime == null) {
                fail("getMetrics() returned neither video nor audio mime value");
            }

            long duration = metrics.getLong(MediaPlayer.MetricsConstants.DURATION, -2);
            if (duration == -2) {
                fail("getMetrics() didn't return a duration");
            }
            long playing = metrics.getLong(MediaPlayer.MetricsConstants.PLAYING, -2);
            if (playing == -2) {
                fail("getMetrics() didn't return a playing time");
            }
            if (!keys.contains(MediaPlayer.MetricsConstants.PLAYING)) {
                fail("MediaMetricsSet.keys() missing: " + MediaPlayer.MetricsConstants.PLAYING);
            }
        }

        mMediaPlayer.stop();
    }

    private static class PrepareFailedException extends Exception {}

    public boolean isTv() {
        PackageManager pm = getInstrumentation().getTargetContext().getPackageManager();
        return pm.hasSystemFeature(pm.FEATURE_TELEVISION)
                && pm.hasSystemFeature(pm.FEATURE_LEANBACK);
    }

    public boolean checkTv() {
        return MediaUtils.check(isTv(), "not a TV");
    }

    protected void setOnErrorListener() {
        mMediaPlayer.setOnErrorListener(new MediaPlayer.OnErrorListener() {
            @Override
            public boolean onError(MediaPlayer mp, int what, int extra) {
                mOnErrorCalled.signal();
                return false;
            }
        });
    }
}
