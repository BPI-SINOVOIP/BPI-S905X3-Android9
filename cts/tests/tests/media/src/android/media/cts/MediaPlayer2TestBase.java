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
package android.media.cts;

import android.content.Context;
import android.content.pm.PackageManager;
import android.content.res.AssetFileDescriptor;
import android.content.res.Resources;
import android.media.AudioAttributes;
import android.media.AudioManager;
import android.media.DataSourceDesc;
import android.media.MediaPlayer2;
import android.media.MediaTimestamp;
import android.media.TimedMetaData;
import android.media.TimedText;
import android.media.cts.TestUtils.Monitor;
import android.net.Uri;
import android.os.PersistableBundle;
import android.test.ActivityInstrumentationTestCase2;
import android.view.SurfaceHolder;

import com.android.compatibility.common.util.MediaUtils;

import java.io.IOException;
import java.net.HttpCookie;
import java.util.ArrayList;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.logging.Logger;

/**
 * Base class for tests which use MediaPlayer2 to play audio or video.
 */
public class MediaPlayer2TestBase extends ActivityInstrumentationTestCase2<MediaStubActivity> {
    private static final Logger LOG = Logger.getLogger(MediaPlayer2TestBase.class.getName());

    protected static final int SLEEP_TIME = 1000;
    protected static final int LONG_SLEEP_TIME = 6000;
    protected static final int STREAM_RETRIES = 20;
    protected static boolean sUseScaleToFitMode = false;

    protected Monitor mOnVideoSizeChangedCalled = new Monitor();
    protected Monitor mOnVideoRenderingStartCalled = new Monitor();
    protected Monitor mOnBufferingUpdateCalled = new Monitor();
    protected Monitor mOnPrepareCalled = new Monitor();
    protected Monitor mOnPlayCalled = new Monitor();
    protected Monitor mOnDeselectTrackCalled = new Monitor();
    protected Monitor mOnSeekCompleteCalled = new Monitor();
    protected Monitor mOnCompletionCalled = new Monitor();
    protected Monitor mOnInfoCalled = new Monitor();
    protected Monitor mOnErrorCalled = new Monitor();
    protected int mCallStatus;

    protected Context mContext;
    protected Resources mResources;

    protected ExecutorService mExecutor;

    protected MediaPlayer2 mPlayer = null;
    protected MediaPlayer2 mPlayer2 = null;
    protected MediaStubActivity mActivity;

    protected final Object mEventCbLock = new Object();
    protected List<MediaPlayer2.MediaPlayer2EventCallback> mEventCallbacks =
            new ArrayList<MediaPlayer2.MediaPlayer2EventCallback>();
    protected final Object mEventCbLock2 = new Object();
    protected List<MediaPlayer2.MediaPlayer2EventCallback> mEventCallbacks2 =
            new ArrayList<MediaPlayer2.MediaPlayer2EventCallback>();

    // convenience functions to create MediaPlayer2
    protected static MediaPlayer2 createMediaPlayer2(Context context, Uri uri) {
        return createMediaPlayer2(context, uri, null);
    }

    protected static MediaPlayer2 createMediaPlayer2(Context context, Uri uri,
            SurfaceHolder holder) {
        AudioManager am = (AudioManager)context.getSystemService(Context.AUDIO_SERVICE);
        int s = am.generateAudioSessionId();
        return createMediaPlayer2(context, uri, holder, null, s > 0 ? s : 0);
    }

    protected static MediaPlayer2 createMediaPlayer2(Context context, Uri uri, SurfaceHolder holder,
            AudioAttributes audioAttributes, int audioSessionId) {
        try {
            MediaPlayer2 mp = MediaPlayer2.create();
            final AudioAttributes aa = audioAttributes != null ? audioAttributes :
                    new AudioAttributes.Builder().build();
            mp.setAudioAttributes(aa);
            mp.setAudioSessionId(audioSessionId);
            mp.setDataSource(new DataSourceDesc.Builder()
                    .setDataSource(context, uri)
                    .build());
            if (holder != null) {
                mp.setDisplay(holder);
            }
            Monitor onPrepareCalled = new Monitor();
            ExecutorService executor = Executors.newFixedThreadPool(1);
            MediaPlayer2.MediaPlayer2EventCallback ecb =
                new MediaPlayer2.MediaPlayer2EventCallback() {
                    @Override
                    public void onInfo(MediaPlayer2 mp, DataSourceDesc dsd, int what, int extra) {
                        if (what == MediaPlayer2.MEDIA_INFO_PREPARED) {
                            onPrepareCalled.signal();
                        }
                    }
                };
            mp.setMediaPlayer2EventCallback(executor, ecb);
            mp.prepare();
            onPrepareCalled.waitForSignal();
            mp.clearMediaPlayer2EventCallback();
            executor.shutdown();
            return mp;
        } catch (IllegalArgumentException ex) {
            LOG.warning("create failed:" + ex);
            // fall through
        } catch (SecurityException ex) {
            LOG.warning("create failed:" + ex);
            // fall through
        } catch (InterruptedException ex) {
            LOG.warning("create failed:" + ex);
            // fall through
        }
        return null;
    }

    protected static MediaPlayer2 createMediaPlayer2(Context context, int resid) {
        AudioManager am = (AudioManager)context.getSystemService(Context.AUDIO_SERVICE);
        int s = am.generateAudioSessionId();
        return createMediaPlayer2(context, resid, null, s > 0 ? s : 0);
    }

    protected static MediaPlayer2 createMediaPlayer2(Context context, int resid,
            AudioAttributes audioAttributes, int audioSessionId) {
        try {
            AssetFileDescriptor afd = context.getResources().openRawResourceFd(resid);
            if (afd == null) {
                return null;
            }

            MediaPlayer2 mp = MediaPlayer2.create();

            final AudioAttributes aa = audioAttributes != null ? audioAttributes :
                    new AudioAttributes.Builder().build();
            mp.setAudioAttributes(aa);
            mp.setAudioSessionId(audioSessionId);

            mp.setDataSource(new DataSourceDesc.Builder()
                    .setDataSource(afd.getFileDescriptor(), afd.getStartOffset(), afd.getLength())
                    .build());

            Monitor onPrepareCalled = new Monitor();
            ExecutorService executor = Executors.newFixedThreadPool(1);
            MediaPlayer2.MediaPlayer2EventCallback ecb =
                new MediaPlayer2.MediaPlayer2EventCallback() {
                    @Override
                    public void onInfo(MediaPlayer2 mp, DataSourceDesc dsd, int what, int extra) {
                        if (what == MediaPlayer2.MEDIA_INFO_PREPARED) {
                            onPrepareCalled.signal();
                        }
                    }
                };
            mp.setMediaPlayer2EventCallback(executor, ecb);
            mp.prepare();
            onPrepareCalled.waitForSignal();
            mp.clearMediaPlayer2EventCallback();
            afd.close();
            executor.shutdown();
            return mp;
        } catch (IOException ex) {
            LOG.warning("create failed:" + ex);
            // fall through
        } catch (IllegalArgumentException ex) {
            LOG.warning("create failed:" + ex);
            // fall through
        } catch (SecurityException ex) {
            LOG.warning("create failed:" + ex);
            // fall through
        } catch (InterruptedException ex) {
            LOG.warning("create failed:" + ex);
            // fall through
        }
        return null;
    }

    public MediaPlayer2TestBase() {
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
                    mPlayer = MediaPlayer2.create();
                    mPlayer2 = MediaPlayer2.create();
                }
            });
        } catch (Throwable e) {
            e.printStackTrace();
            fail();
        }
        mContext = getInstrumentation().getTargetContext();
        mResources = mContext.getResources();
        mExecutor = Executors.newFixedThreadPool(1);

        setUpMP2ECb(mPlayer, mEventCbLock, mEventCallbacks);
        setUpMP2ECb(mPlayer2, mEventCbLock2, mEventCallbacks2);
    }

    @Override
    protected void tearDown() throws Exception {
        if (mPlayer != null) {
            mPlayer.close();
            mPlayer = null;
        }
        if (mPlayer2 != null) {
            mPlayer2.close();
            mPlayer2 = null;
        }
        mExecutor.shutdown();
        mActivity = null;
        super.tearDown();
    }

    protected void setUpMP2ECb(MediaPlayer2 mp, Object cbLock,
            List<MediaPlayer2.MediaPlayer2EventCallback> ecbs) {
        mp.setMediaPlayer2EventCallback(mExecutor, new MediaPlayer2.MediaPlayer2EventCallback() {
            @Override
            public void onVideoSizeChanged(MediaPlayer2 mp, DataSourceDesc dsd, int w, int h) {
                synchronized (cbLock) {
                    for (MediaPlayer2.MediaPlayer2EventCallback ecb : ecbs) {
                        ecb.onVideoSizeChanged(mp, dsd, w, h);
                    }
                }
            }

            @Override
            public void onTimedText(MediaPlayer2 mp, DataSourceDesc dsd, TimedText text) {
                synchronized (cbLock) {
                    for (MediaPlayer2.MediaPlayer2EventCallback ecb : ecbs) {
                        ecb.onTimedText(mp, dsd, text);
                    }
                }
            }

            @Override
            public void onTimedMetaDataAvailable(MediaPlayer2 mp, DataSourceDesc dsd,
                    TimedMetaData data) {
                synchronized (cbLock) {
                    for (MediaPlayer2.MediaPlayer2EventCallback ecb : ecbs) {
                        ecb.onTimedMetaDataAvailable(mp, dsd, data);
                    }
                }
            }

            @Override
            public void onError(MediaPlayer2 mp, DataSourceDesc dsd, int what, int extra) {
                synchronized (cbLock) {
                    for (MediaPlayer2.MediaPlayer2EventCallback ecb : ecbs) {
                        ecb.onError(mp, dsd, what, extra);
                    }
                }
            }

            @Override
            public void onInfo(MediaPlayer2 mp, DataSourceDesc dsd, int what, int extra) {
                synchronized (cbLock) {
                    for (MediaPlayer2.MediaPlayer2EventCallback ecb : ecbs) {
                        ecb.onInfo(mp, dsd, what, extra);
                    }
                }
            }

            @Override
            public void onCallCompleted(MediaPlayer2 mp, DataSourceDesc dsd, int what, int status) {
                synchronized (cbLock) {
                    for (MediaPlayer2.MediaPlayer2EventCallback ecb : ecbs) {
                        ecb.onCallCompleted(mp, dsd, what, status);
                    }
                }
            }

            @Override
            public void onMediaTimeChanged(MediaPlayer2 mp, DataSourceDesc dsd,
                    MediaTimestamp timestamp) {
                synchronized (cbLock) {
                    for (MediaPlayer2.MediaPlayer2EventCallback ecb : ecbs) {
                        ecb.onMediaTimeChanged(mp, dsd, timestamp);
                    }
                }
            }

            @Override
            public void onCommandLabelReached(MediaPlayer2 mp, Object label) {
                synchronized (cbLock) {
                    for (MediaPlayer2.MediaPlayer2EventCallback ecb : ecbs) {
                        ecb.onCommandLabelReached(mp, label);
                    }
                }
            }
        });
    }

    // returns true on success
    protected boolean loadResource(int resid) throws Exception {
        if (!MediaUtils.hasCodecsForResource(mContext, resid)) {
            return false;
        }

        AssetFileDescriptor afd = mResources.openRawResourceFd(resid);
        try {
            mPlayer.setDataSource(new DataSourceDesc.Builder()
                    .setDataSource(afd.getFileDescriptor(), afd.getStartOffset(), afd.getLength())
                    .build());

            // Although it is only meant for video playback, it should not
            // cause issues for audio-only playback.
            int videoScalingMode = sUseScaleToFitMode?
                                    MediaPlayer2.VIDEO_SCALING_MODE_SCALE_TO_FIT
                                  : MediaPlayer2.VIDEO_SCALING_MODE_SCALE_TO_FIT_WITH_CROPPING;

            mPlayer.setVideoScalingMode(videoScalingMode);
        } finally {
            // TODO: close afd only after setDataSource is confirmed.
            // afd.close();
        }
        sUseScaleToFitMode = !sUseScaleToFitMode;  // Alternate the scaling mode
        return true;
    }

    protected DataSourceDesc createDataSourceDesc(int resid) throws Exception {
        if (!MediaUtils.hasCodecsForResource(mContext, resid)) {
            return null;
        }

        AssetFileDescriptor afd = mResources.openRawResourceFd(resid);
        return new DataSourceDesc.Builder()
                .setDataSource(afd.getFileDescriptor(), afd.getStartOffset(), afd.getLength())
                .build();
    }

    protected boolean checkLoadResource(int resid) throws Exception {
        return MediaUtils.check(loadResource(resid), "no decoder found");
    }

    protected void loadSubtitleSource(int resid) throws Exception {
        AssetFileDescriptor afd = mResources.openRawResourceFd(resid);
        try {
            mPlayer.addTimedTextSource(afd.getFileDescriptor(), afd.getStartOffset(),
                      afd.getLength(), MediaPlayer2.MEDIA_MIMETYPE_TEXT_SUBRIP);
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
        final Uri uri = Uri.parse(path);
        for (int i = 0; i < STREAM_RETRIES; i++) {
          try {
            mPlayer.setDataSource(new DataSourceDesc.Builder()
                    .setDataSource(mContext, uri)
                    .build());
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
                mPlayer.setDataSource(new DataSourceDesc.Builder()
                        .setDataSource(getInstrumentation().getTargetContext(),
                            uri, headers, cookies)
                        .build());
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
        final float volume = 0.5f;

        boolean audioOnly = (width != null && width.intValue() == -1) ||
                (height != null && height.intValue() == -1);

        mPlayer.setDisplay(mActivity.getSurfaceHolder());
        mPlayer.setScreenOnWhilePlaying(true);

        synchronized (mEventCbLock) {
            mEventCallbacks.add(new MediaPlayer2.MediaPlayer2EventCallback() {
                @Override
                public void onVideoSizeChanged(MediaPlayer2 mp, DataSourceDesc dsd, int w, int h) {
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

                @Override
                public void onError(MediaPlayer2 mp, DataSourceDesc dsd, int what, int extra) {
                    fail("Media player had error " + what + " playing video");
                }

                @Override
                public void onInfo(MediaPlayer2 mp, DataSourceDesc dsd, int what, int extra) {
                    if (what == MediaPlayer2.MEDIA_INFO_VIDEO_RENDERING_START) {
                        mOnVideoRenderingStartCalled.signal();
                    } else if (what == MediaPlayer2.MEDIA_INFO_PREPARED) {
                        mOnPrepareCalled.signal();
                    }
                }

                @Override
                public void onCallCompleted(MediaPlayer2 mp, DataSourceDesc dsd,
                        int what, int status) {
                    if (what == MediaPlayer2.CALL_COMPLETED_PLAY) {
                        mOnPlayCalled.signal();
                    }
                }
            });
        }
        try {
          mOnPrepareCalled.reset();
          mPlayer.prepare();
          mOnPrepareCalled.waitForSignal();
        } catch (Exception e) {
          mPlayer.reset();
          throw new PrepareFailedException();
        }

        mOnPlayCalled.reset();
        mPlayer.play();
        mOnPlayCalled.waitForSignal();
        if (!audioOnly) {
            mOnVideoSizeChangedCalled.waitForSignal();
            mOnVideoRenderingStartCalled.waitForSignal();
        }
        mPlayer.setPlayerVolume(volume);

        // waiting to complete
        if (playTime == -1) {
            return;
        } else if (playTime == 0) {
            while (mPlayer.isPlaying()) {
                Thread.sleep(SLEEP_TIME);
            }
        } else {
            Thread.sleep(playTime);
        }

        // validate a few MediaMetrics.
        PersistableBundle metrics = mPlayer.getMetrics();
        if (metrics == null) {
            fail("MediaPlayer2.getMetrics() returned null metrics");
        } else if (metrics.isEmpty()) {
            fail("MediaPlayer2.getMetrics() returned empty metrics");
        } else {

            int size = metrics.size();
            Set<String> keys = metrics.keySet();

            if (keys == null) {
                fail("MediaMetricsSet returned no keys");
            } else if (keys.size() != size) {
                fail("MediaMetricsSet.keys().size() mismatch MediaMetricsSet.size()");
            }

            // we played something; so one of these should be non-null
            String vmime = metrics.getString(MediaPlayer2.MetricsConstants.MIME_TYPE_VIDEO, null);
            String amime = metrics.getString(MediaPlayer2.MetricsConstants.MIME_TYPE_AUDIO, null);
            if (vmime == null && amime == null) {
                fail("getMetrics() returned neither video nor audio mime value");
            }

            long duration = metrics.getLong(MediaPlayer2.MetricsConstants.DURATION, -2);
            if (duration == -2) {
                fail("getMetrics() didn't return a duration");
            }
            long playing = metrics.getLong(MediaPlayer2.MetricsConstants.PLAYING, -2);
            if (playing == -2) {
                fail("getMetrics() didn't return a playing time");
            }
            if (!keys.contains(MediaPlayer2.MetricsConstants.PLAYING)) {
                fail("MediaMetricsSet.keys() missing: " + MediaPlayer2.MetricsConstants.PLAYING);
            }
        }

        mPlayer.stop();
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
        synchronized (mEventCbLock) {
            mEventCallbacks.add(new MediaPlayer2.MediaPlayer2EventCallback() {
                @Override
                public void onError(MediaPlayer2 mp, DataSourceDesc dsd, int what, int extra) {
                    mOnErrorCalled.signal();
                }
            });
        }
    }
}
