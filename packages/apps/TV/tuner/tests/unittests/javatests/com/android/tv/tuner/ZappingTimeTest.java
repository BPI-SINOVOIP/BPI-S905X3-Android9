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
 * limitations under the License.
 */

package com.android.tv.tuner;

import android.content.Context;
import android.graphics.SurfaceTexture;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.Message;
import android.support.test.filters.LargeTest;
import android.test.InstrumentationTestCase;
import android.util.Log;
import android.view.Surface;
import com.android.tv.tuner.data.Cea708Data;
import com.android.tv.tuner.data.PsiData;
import com.android.tv.tuner.data.PsipData;
import com.android.tv.tuner.data.TunerChannel;
import com.android.tv.tuner.data.nano.Channel;
import com.android.tv.tuner.exoplayer.MpegTsPlayer;
import com.android.tv.tuner.exoplayer.MpegTsRendererBuilder;
import com.android.tv.tuner.exoplayer.buffer.BufferManager;
import com.android.tv.tuner.exoplayer.buffer.TrickplayStorageManager;
import com.android.tv.tuner.source.TsDataSourceManager;
import com.android.tv.tuner.tvinput.EventDetector;
import com.android.tv.tuner.tvinput.PlaybackBufferListener;
import com.google.android.exoplayer.ExoPlayer;
import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicLong;
import org.junit.Ignore;

/** This class use {@link FileTunerHal} to simulate tunerhal's actions to test zapping time. */
@LargeTest
public class ZappingTimeTest extends InstrumentationTestCase {
    private static final String TAG = "ZappingTimeTest";
    private static final boolean DEBUG = false;
    private static final int TS_COPY_BUFFER_SIZE = 1024 * 512;
    private static final int PROGRAM_NUMBER = 1;
    private static final int VIDEO_PID = 49;
    private static final int PCR_PID = 49;
    private static final List<Integer> AUDIO_PIDS = Arrays.asList(51, 52, 53);
    private static final long BUFFER_SIZE_DEF = 2 * 1024;
    private static final int FREQUENCY = -1;
    private static final String MODULATION = "";
    private static final long ZAPPING_TIME_OUT_MS = 10000;
    private static final long MAX_AVERAGE_ZAPPING_TIME_MS = 4000;
    private static final int TEST_ITERATION_COUNT = 10;
    private static final int STRESS_ZAPPING_TEST_COUNT = 50;
    private static final long SKIP_DURATION_MS_TO_ADD = 200;
    private static final String TEST_TS_FILE_PATH = "capture_kqed.ts";

    private static final int MSG_START_PLAYBACK = 1;

    private TunerChannel mChannel;
    private FileTunerHal mTunerHal;
    private MpegTsPlayer mPlayer;
    private TsDataSourceManager mSourceManager;
    private Handler mHandler;
    private Context mTargetContext;
    private File mTrickplayBufferDir;
    private Surface mSurface;
    private CountDownLatch mErrorLatch;
    private CountDownLatch mDrawnToSurfaceLatch;
    private CountDownLatch mWaitTuneExecuteLatch;
    private AtomicLong mOnDrawnToSurfaceTimeMs = new AtomicLong(0);
    private MockMpegTsPlayerListener mMpegTsPlayerListener = new MockMpegTsPlayerListener();
    private MockPlaybackBufferListener mPlaybackBufferListener = new MockPlaybackBufferListener();
    private MockEventListener mEventListener = new MockEventListener();

    @Override
    protected void setUp() throws Exception {
        super.setUp();
        mTargetContext = getInstrumentation().getTargetContext();
        mTrickplayBufferDir = mTargetContext.getCacheDir();
        HandlerThread handlerThread = new HandlerThread(TAG);
        handlerThread.start();
        List<PsiData.PmtItem> pmtItems = new ArrayList<>();
        pmtItems.add(new PsiData.PmtItem(Channel.VideoStreamType.MPEG2, VIDEO_PID, null, null));
        for (int audioPid : AUDIO_PIDS) {
            pmtItems.add(
                    new PsiData.PmtItem(Channel.AudioStreamType.A52AC3AUDIO, audioPid, null, null));
        }

        Context context = getInstrumentation().getContext();
        // Since assets and resource files are compressed, random access to the specified offset
        // in assets or resource files will add some delay which is proportional to the offset.
        // So the TS stream asset file are copied to a cache file, and the starting stream position
        // in the file will be accessed by underlying {@link RandomAccessFile}.
        File tsCacheFile = createCacheFile(context, mTargetContext, TEST_TS_FILE_PATH);
        pmtItems.add(new PsiData.PmtItem(0x100, PCR_PID, null, null));
        mChannel = new TunerChannel(PROGRAM_NUMBER, pmtItems);
        mChannel.setFrequency(FREQUENCY);
        mChannel.setModulation(MODULATION);
        mTunerHal = new FileTunerHal(context, tsCacheFile);
        mTunerHal.openFirstAvailable();
        mSourceManager = TsDataSourceManager.createSourceManager(false);
        mSourceManager.addTunerHalForTest(mTunerHal);
        mHandler =
                new Handler(
                        handlerThread.getLooper(),
                        new Handler.Callback() {
                            @Override
                            public boolean handleMessage(Message msg) {
                                switch (msg.what) {
                                    case MSG_START_PLAYBACK:
                                        {
                                            mHandler.removeCallbacksAndMessages(null);
                                            stopPlayback();
                                            mOnDrawnToSurfaceTimeMs.set(0);
                                            mDrawnToSurfaceLatch = new CountDownLatch(1);
                                            if (mWaitTuneExecuteLatch != null) {
                                                mWaitTuneExecuteLatch.countDown();
                                            }
                                            int frequency = msg.arg1;
                                            boolean useSimpleSampleBuffer = (msg.arg2 == 1);
                                            BufferManager bufferManager = null;
                                            if (!useSimpleSampleBuffer) {
                                                bufferManager =
                                                        new BufferManager(
                                                                new TrickplayStorageManager(
                                                                        mTargetContext,
                                                                        mTrickplayBufferDir,
                                                                        1024L
                                                                                * 1024L
                                                                                * BUFFER_SIZE_DEF));
                                            }
                                            mChannel.setFrequency(frequency);
                                            mSourceManager.setKeepTuneStatus(true);
                                            mPlayer =
                                                    new MpegTsPlayer(
                                                            new MpegTsRendererBuilder(
                                                                    mTargetContext,
                                                                    bufferManager,
                                                                    mPlaybackBufferListener),
                                                            mHandler,
                                                            mSourceManager,
                                                            null,
                                                            mMpegTsPlayerListener);
                                            mPlayer.setCaptionServiceNumber(
                                                    Cea708Data.EMPTY_SERVICE_NUMBER);
                                            mPlayer.prepare(
                                                    mTargetContext,
                                                    mChannel,
                                                    false,
                                                    mEventListener);
                                            return true;
                                        }
                                    default:
                                        {
                                            Log.i(TAG, "Unhandled message code: " + msg.what);
                                            return true;
                                        }
                                }
                            }
                        });
    }

    @Override
    protected void tearDown() throws Exception {
        if (mPlayer != null) {
            mPlayer.release();
        }
        if (mSurface != null) {
            mSurface.release();
        }
        mHandler.getLooper().quitSafely();
        super.tearDown();
    }

    public void testZappingTime() {
        zappingTimeTest(false, TEST_ITERATION_COUNT, true);
    }

    public void testZappingTimeWithSimpleSampleBuffer() {
        zappingTimeTest(true, TEST_ITERATION_COUNT, true);
    }

    @Ignore("b/69978026")
    @SuppressWarnings("JUnit4ClassUsedInJUnit3")
    public void testStressZapping() {
        zappingTimeTest(false, STRESS_ZAPPING_TEST_COUNT, false);
    }

    @Ignore("b/69978093")
    @SuppressWarnings("JUnit4ClassUsedInJUnit3")
    public void testZappingWithPacketMissing() {
        mTunerHal.setEnablePacketMissing(true);
        mTunerHal.setEnableArtificialDelay(true);
        SurfaceTexture surfaceTexture = new SurfaceTexture(0);
        mSurface = new Surface(surfaceTexture);
        long zappingStartTimeMs = System.currentTimeMillis();
        mErrorLatch = new CountDownLatch(1);
        mHandler.obtainMessage(MSG_START_PLAYBACK, FREQUENCY, 0).sendToTarget();
        boolean errorAppeared = false;
        while (System.currentTimeMillis() - zappingStartTimeMs < ZAPPING_TIME_OUT_MS) {
            try {
                errorAppeared = mErrorLatch.await(100, TimeUnit.MILLISECONDS);
                if (errorAppeared) {
                    break;
                }
            } catch (InterruptedException e) {
            }
        }
        assertFalse("Error happened when packet lost", errorAppeared);
    }

    private static File createCacheFile(Context context, Context targetContext, String filename)
            throws IOException {
        File cacheFile = new File(targetContext.getCacheDir(), filename);

        if (cacheFile.createNewFile() == false) {
            cacheFile.delete();
            cacheFile.createNewFile();
        }

        InputStream inputStream = context.getResources().getAssets().open(filename);
        FileOutputStream fileOutputStream = new FileOutputStream(cacheFile);

        byte[] buffer = new byte[TS_COPY_BUFFER_SIZE];
        while (inputStream.read(buffer, 0, TS_COPY_BUFFER_SIZE) != -1) {
            fileOutputStream.write(buffer);
        }

        fileOutputStream.close();
        inputStream.close();

        return cacheFile;
    }

    private void zappingTimeTest(
            boolean useSimpleSampleBuffer, int testIterationCount, boolean enableArtificialDelay) {
        String bufferManagerLogString =
                !enableArtificialDelay
                        ? "for stress test"
                        : useSimpleSampleBuffer ? "with simple sample buffer" : "";
        SurfaceTexture surfaceTexture = new SurfaceTexture(0);
        mSurface = new Surface(surfaceTexture);
        mTunerHal.setEnablePacketMissing(false);
        mTunerHal.setEnableArtificialDelay(enableArtificialDelay);
        double totalZappingTime = 0.0;
        for (int i = 0; i < testIterationCount; i++) {
            mWaitTuneExecuteLatch = new CountDownLatch(1);
            long zappingStartTimeMs = System.currentTimeMillis();
            mTunerHal.setInitialSkipMs(SKIP_DURATION_MS_TO_ADD * (i % TEST_ITERATION_COUNT));
            mHandler.obtainMessage(MSG_START_PLAYBACK, FREQUENCY + i, useSimpleSampleBuffer ? 1 : 0)
                    .sendToTarget();
            try {
                mWaitTuneExecuteLatch.await();
            } catch (InterruptedException e) {
            }
            boolean drawnToSurface = false;
            while (System.currentTimeMillis() - zappingStartTimeMs < ZAPPING_TIME_OUT_MS) {
                try {
                    drawnToSurface = mDrawnToSurfaceLatch.await(100, TimeUnit.MILLISECONDS);
                    if (drawnToSurface) {
                        break;
                    }
                } catch (InterruptedException e) {
                }
            }
            if (i == 0) {
                continue;
                // Get rid of the first result, which shows outlier often.
            }
            // In 10s, all zapping request will finish. Set the maximum zapping time as 10s could be
            // reasonable.
            totalZappingTime +=
                    (mOnDrawnToSurfaceTimeMs.get() > 0
                            ? mOnDrawnToSurfaceTimeMs.get() - zappingStartTimeMs
                            : ZAPPING_TIME_OUT_MS);
        }
        double averageZappingTime = totalZappingTime / (testIterationCount - 1);
        Log.i(TAG, "Average zapping time " + bufferManagerLogString + ":" + averageZappingTime);
        assertTrue(
                "Average Zapping time "
                        + bufferManagerLogString
                        + " is too large:"
                        + averageZappingTime,
                averageZappingTime < MAX_AVERAGE_ZAPPING_TIME_MS);
    }

    private void stopPlayback() {
        if (mPlayer != null) {
            mPlayer.setPlayWhenReady(false);
            mPlayer.release();
            mPlayer = null;
        }
    }

    private class MockMpegTsPlayerListener implements MpegTsPlayer.Listener {

        @Override
        public void onStateChanged(boolean playWhenReady, int playbackState) {
            if (DEBUG) {
                Log.d(TAG, "ExoPlayer state change: " + playbackState + " " + playWhenReady);
            }
            if (playbackState == ExoPlayer.STATE_READY) {
                mPlayer.setSurface(mSurface);
                mPlayer.setPlayWhenReady(true);
                mPlayer.setVolume(1.0f);
            }
        }

        @Override
        public void onError(Exception e) {
            if (DEBUG) {
                Log.d(TAG, "onError");
            }
            if (mErrorLatch != null) {
                mErrorLatch.countDown();
            }
        }

        @Override
        public void onVideoSizeChanged(int width, int height, float pixelWidthHeightRatio) {
            if (DEBUG) {
                Log.d(TAG, "onVideoSizeChanged");
            }
        }

        @Override
        public void onDrawnToSurface(MpegTsPlayer player, Surface surface) {
            if (DEBUG) {
                Log.d(TAG, "onDrawnToSurface");
            }
            mOnDrawnToSurfaceTimeMs.set(System.currentTimeMillis());
            if (mDrawnToSurfaceLatch != null) {
                mDrawnToSurfaceLatch.countDown();
            }
        }

        @Override
        public void onAudioUnplayable() {
            if (DEBUG) {
                Log.d(TAG, "onAudioUnplayable");
            }
        }

        @Override
        public void onSmoothTrickplayForceStopped() {
            if (DEBUG) {
                Log.d(TAG, "onSmoothTrickplayForceStopped");
            }
        }
    }

    private static class MockPlaybackBufferListener implements PlaybackBufferListener {
        @Override
        public void onBufferStartTimeChanged(long startTimeMs) {
            if (DEBUG) {
                Log.d(TAG, "onBufferStartTimeChanged");
            }
        }

        @Override
        public void onBufferStateChanged(boolean available) {
            if (DEBUG) {
                Log.d(TAG, "onBufferStateChanged");
            }
        }

        @Override
        public void onDiskTooSlow() {
            if (DEBUG) {
                Log.d(TAG, "onDiskTooSlow");
            }
        }
    }

    private static class MockEventListener implements EventDetector.EventListener {
        @Override
        public void onChannelDetected(TunerChannel channel, boolean channelArrivedAtFirstTime) {
            if (DEBUG) {
                Log.d(TAG, "onChannelDetected");
            }
        }

        @Override
        public void onEventDetected(TunerChannel channel, List<PsipData.EitItem> items) {
            if (DEBUG) {
                Log.d(TAG, "onEventDetected");
            }
        }

        @Override
        public void onChannelScanDone() {
            if (DEBUG) {
                Log.d(TAG, "onChannelScanDone");
            }
        }
    }
}
