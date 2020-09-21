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

import android.media.BufferingParams;
import android.media.DataSourceDesc;
import android.media.MediaFormat;
import android.media.MediaPlayer2;
import android.media.MediaPlayer2.TrackInfo;
import android.media.TimedMetaData;
import android.media.cts.TestUtils.Monitor;
import android.net.Uri;
import android.os.Bundle;
import android.os.Looper;
import android.os.PowerManager;
import android.os.SystemClock;
import android.platform.test.annotations.AppModeFull;
import android.test.InstrumentationTestRunner;
import android.util.Log;
import android.webkit.cts.CtsTestServer;

import com.android.compatibility.common.util.DynamicConfigDeviceSide;
import com.android.compatibility.common.util.MediaUtils;

import java.net.HttpCookie;
import java.util.concurrent.atomic.AtomicInteger;
import java.util.HashMap;
import java.util.List;

/**
 * Tests of MediaPlayer2 streaming capabilities.
 */
@AppModeFull(reason = "TODO: evaluate and port to instant")
public class StreamingMediaPlayer2Test extends MediaPlayer2TestBase {
    // TODO: remove this flag to enable tests.
    private static final boolean IGNORE_TESTS = true;

    private static final String TAG = "StreamingMediaPlayer2Test";

    private static final String HTTP_H263_AMR_VIDEO_1_KEY =
            "streaming_media_player_test_http_h263_amr_video1";
    private static final String HTTP_H263_AMR_VIDEO_2_KEY =
            "streaming_media_player_test_http_h263_amr_video2";
    private static final String HTTP_H264_BASE_AAC_VIDEO_1_KEY =
            "streaming_media_player_test_http_h264_base_aac_video1";
    private static final String HTTP_H264_BASE_AAC_VIDEO_2_KEY =
            "streaming_media_player_test_http_h264_base_aac_video2";
    private static final String HTTP_MPEG4_SP_AAC_VIDEO_1_KEY =
            "streaming_media_player_test_http_mpeg4_sp_aac_video1";
    private static final String HTTP_MPEG4_SP_AAC_VIDEO_2_KEY =
            "streaming_media_player_test_http_mpeg4_sp_aac_video2";
    private static final String MODULE_NAME = "CtsMediaTestCases";
    private DynamicConfigDeviceSide dynamicConfig;

    private CtsTestServer mServer;

    private String mInputUrl;

    @Override
    protected void setUp() throws Exception {
        // if launched with InstrumentationTestRunner to pass a command line argument
        if (getInstrumentation() instanceof InstrumentationTestRunner) {
            InstrumentationTestRunner testRunner =
                    (InstrumentationTestRunner)getInstrumentation();

            Bundle arguments = testRunner.getArguments();
            mInputUrl = arguments.getString("url");
            Log.v(TAG, "setUp: arguments: " + arguments);
            if (mInputUrl != null) {
                Log.v(TAG, "setUp: arguments[url] " + mInputUrl);
            }
        }

        super.setUp();
        dynamicConfig = new DynamicConfigDeviceSide(MODULE_NAME);
    }

/* RTSP tests are more flaky and vulnerable to network condition.
   Disable until better solution is available
    // Streaming RTSP video from YouTube
    public void testRTSP_H263_AMR_Video1() throws Exception {
        playVideoTest("rtsp://v2.cache7.c.youtube.com/video.3gp?cid=0x271de9756065677e"
                + "&fmt=13&user=android-device-test", 176, 144);
    }
    public void testRTSP_H263_AMR_Video2() throws Exception {
        playVideoTest("rtsp://v2.cache7.c.youtube.com/video.3gp?cid=0xc80658495af60617"
                + "&fmt=13&user=android-device-test", 176, 144);
    }

    public void testRTSP_MPEG4SP_AAC_Video1() throws Exception {
        playVideoTest("rtsp://v2.cache7.c.youtube.com/video.3gp?cid=0x271de9756065677e"
                + "&fmt=17&user=android-device-test", 176, 144);
    }
    public void testRTSP_MPEG4SP_AAC_Video2() throws Exception {
        playVideoTest("rtsp://v2.cache7.c.youtube.com/video.3gp?cid=0xc80658495af60617"
                + "&fmt=17&user=android-device-test", 176, 144);
    }

    public void testRTSP_H264Base_AAC_Video1() throws Exception {
        playVideoTest("rtsp://v2.cache7.c.youtube.com/video.3gp?cid=0x271de9756065677e"
                + "&fmt=18&user=android-device-test", 480, 270);
    }
    public void testRTSP_H264Base_AAC_Video2() throws Exception {
        playVideoTest("rtsp://v2.cache7.c.youtube.com/video.3gp?cid=0xc80658495af60617"
                + "&fmt=18&user=android-device-test", 480, 270);
    }
*/
    // Streaming HTTP video from YouTube
    public void testHTTP_H263_AMR_Video1() throws Exception {
        if (IGNORE_TESTS) {
            return;
        }
        if (!MediaUtils.checkDecoder(MediaFormat.MIMETYPE_VIDEO_H263,
                  MediaFormat.MIMETYPE_AUDIO_AMR_NB)) {
            return; // skip
        }

        String urlString = dynamicConfig.getValue(HTTP_H263_AMR_VIDEO_1_KEY);
        playVideoTest(urlString, 176, 144);
    }

    public void testHTTP_H263_AMR_Video2() throws Exception {
        if (IGNORE_TESTS) {
            return;
        }
        if (!MediaUtils.checkDecoder(MediaFormat.MIMETYPE_VIDEO_H263,
                  MediaFormat.MIMETYPE_AUDIO_AMR_NB)) {
            return; // skip
        }

        String urlString = dynamicConfig.getValue(HTTP_H263_AMR_VIDEO_2_KEY);
        playVideoTest(urlString, 176, 144);
    }

    public void testHTTP_MPEG4SP_AAC_Video1() throws Exception {
        if (IGNORE_TESTS) {
            return;
        }
        if (!MediaUtils.checkDecoder(MediaFormat.MIMETYPE_VIDEO_MPEG4)) {
            return; // skip
        }

        String urlString = dynamicConfig.getValue(HTTP_MPEG4_SP_AAC_VIDEO_1_KEY);
        playVideoTest(urlString, 176, 144);
    }

    public void testHTTP_MPEG4SP_AAC_Video2() throws Exception {
        if (IGNORE_TESTS) {
            return;
        }
        if (!MediaUtils.checkDecoder(MediaFormat.MIMETYPE_VIDEO_MPEG4)) {
            return; // skip
        }

        String urlString = dynamicConfig.getValue(HTTP_MPEG4_SP_AAC_VIDEO_2_KEY);
        playVideoTest(urlString, 176, 144);
    }

    public void testHTTP_H264Base_AAC_Video1() throws Exception {
        if (IGNORE_TESTS) {
            return;
        }
        if (!MediaUtils.checkDecoder(MediaFormat.MIMETYPE_VIDEO_AVC)) {
            return; // skip
        }

        String urlString = dynamicConfig.getValue(HTTP_H264_BASE_AAC_VIDEO_1_KEY);
        playVideoTest(urlString, 640, 360);
    }

    public void testHTTP_H264Base_AAC_Video2() throws Exception {
        if (IGNORE_TESTS) {
            return;
        }
        if (!MediaUtils.checkDecoder(MediaFormat.MIMETYPE_VIDEO_AVC)) {
            return; // skip
        }

        String urlString = dynamicConfig.getValue(HTTP_H264_BASE_AAC_VIDEO_2_KEY);
        playVideoTest(urlString, 640, 360);
    }

    // Streaming HLS video from YouTube
    public void testHLS() throws Exception {
        if (IGNORE_TESTS) {
            return;
        }
        if (!MediaUtils.checkDecoder(MediaFormat.MIMETYPE_VIDEO_AVC)) {
            return; // skip
        }

        // Play stream for 60 seconds
        playLiveVideoTest("http://www.youtube.com/api/manifest/hls_variant/id/"
                + "0168724d02bd9945/itag/5/source/youtube/playlist_type/DVR/ip/"
                + "0.0.0.0/ipbits/0/expire/19000000000/sparams/ip,ipbits,expire"
                + ",id,itag,source,playlist_type/signature/773AB8ACC68A96E5AA48"
                + "1996AD6A1BBCB70DCB87.95733B544ACC5F01A1223A837D2CF04DF85A336"
                + "0/key/ik0/file/m3u8", 60 * 1000);
    }

    public void testHlsWithHeadersCookies() throws Exception {
        if (IGNORE_TESTS) {
            return;
        }
        if (!MediaUtils.checkDecoder(MediaFormat.MIMETYPE_VIDEO_AVC)) {
            return; // skip
        }

        final Uri uri = Uri.parse(
                "http://www.youtube.com/api/manifest/hls_variant/id/"
                + "0168724d02bd9945/itag/5/source/youtube/playlist_type/DVR/ip/"
                + "0.0.0.0/ipbits/0/expire/19000000000/sparams/ip,ipbits,expire"
                + ",id,itag,source,playlist_type/signature/773AB8ACC68A96E5AA48"
                + "1996AD6A1BBCB70DCB87.95733B544ACC5F01A1223A837D2CF04DF85A336"
                + "0/key/ik0/file/m3u8");

        // TODO: dummy values for headers/cookies till we find a server that actually needs them
        HashMap<String, String> headers = new HashMap<>();
        headers.put("header0", "value0");
        headers.put("header1", "value1");

        String cookieName = "auth_1234567";
        String cookieValue = "0123456789ABCDEF0123456789ABCDEF";
        HttpCookie cookie = new HttpCookie(cookieName, cookieValue);
        cookie.setHttpOnly(true);
        cookie.setDomain("www.youtube.com");
        cookie.setPath("/");        // all paths
        cookie.setSecure(false);
        cookie.setDiscard(false);
        cookie.setMaxAge(24 * 3600);  // 24hrs

        java.util.Vector<HttpCookie> cookies = new java.util.Vector<HttpCookie>();
        cookies.add(cookie);

        // Play stream for 60 seconds
        playLiveVideoTest(uri, headers, cookies, 60 * 1000);
    }

    public void testHlsSampleAes_bbb_audio_only_overridable() throws Exception {
        if (IGNORE_TESTS) {
            return;
        }
        if (!MediaUtils.checkDecoder(MediaFormat.MIMETYPE_VIDEO_AVC)) {
            return; // skip
        }

        String defaultUrl = "http://storage.googleapis.com/wvmedia/cenc/hls/sample_aes/" +
                            "bbb_1080p_30fps_11min/audio_only/prog_index.m3u8";

        // if url override provided
        String testUrl = (mInputUrl != null) ? mInputUrl : defaultUrl;

        // Play stream for 60 seconds
        playLiveAudioOnlyTest(
                testUrl,
                60 * 1000);
    }

    public void testHlsSampleAes_bbb_unmuxed_1500k() throws Exception {
        if (IGNORE_TESTS) {
            return;
        }
        if (!MediaUtils.checkDecoder(MediaFormat.MIMETYPE_VIDEO_AVC)) {
            return; // skip
        }

        // Play stream for 60 seconds
        playLiveVideoTest(
                "http://storage.googleapis.com/wvmedia/cenc/hls/sample_aes/" +
                "bbb_1080p_30fps_11min/unmuxed_1500k/prog_index.m3u8",
                60 * 1000);
    }


    // Streaming audio from local HTTP server
    public void testPlayMp3Stream1() throws Throwable {
        if (IGNORE_TESTS) {
            return;
        }
        localHttpAudioStreamTest("ringer.mp3", false, false);
    }
    public void testPlayMp3Stream2() throws Throwable {
        if (IGNORE_TESTS) {
            return;
        }
        localHttpAudioStreamTest("ringer.mp3", false, false);
    }
    public void testPlayMp3StreamRedirect() throws Throwable {
        if (IGNORE_TESTS) {
            return;
        }
        localHttpAudioStreamTest("ringer.mp3", true, false);
    }
    public void testPlayMp3StreamNoLength() throws Throwable {
        if (IGNORE_TESTS) {
            return;
        }
        localHttpAudioStreamTest("noiseandchirps.mp3", false, true);
    }
    public void testPlayOggStream() throws Throwable {
        if (IGNORE_TESTS) {
            return;
        }
        localHttpAudioStreamTest("noiseandchirps.ogg", false, false);
    }
    public void testPlayOggStreamRedirect() throws Throwable {
        if (IGNORE_TESTS) {
            return;
        }
        localHttpAudioStreamTest("noiseandchirps.ogg", true, false);
    }
    public void testPlayOggStreamNoLength() throws Throwable {
        if (IGNORE_TESTS) {
            return;
        }
        localHttpAudioStreamTest("noiseandchirps.ogg", false, true);
    }
    public void testPlayMp3Stream1Ssl() throws Throwable {
        if (IGNORE_TESTS) {
            return;
        }
        localHttpsAudioStreamTest("ringer.mp3", false, false);
    }

    private void localHttpAudioStreamTest(final String name, boolean redirect, boolean nolength)
            throws Throwable {
        mServer = new CtsTestServer(mContext);
        try {
            String stream_url = null;
            if (redirect) {
                // Stagefright doesn't have a limit, but we can't test support of infinite redirects
                // Up to 4 redirects seems reasonable though.
                stream_url = mServer.getRedirectingAssetUrl(name, 4);
            } else {
                stream_url = mServer.getAssetUrl(name);
            }
            if (nolength) {
                stream_url = stream_url + "?" + CtsTestServer.NOLENGTH_POSTFIX;
            }

            if (!MediaUtils.checkCodecsForPath(mContext, stream_url)) {
                return; // skip
            }

            final Uri uri = Uri.parse(stream_url);
            mPlayer.setDataSource(new DataSourceDesc.Builder()
                    .setDataSource(mContext, uri)
                    .build());

            mPlayer.setDisplay(getActivity().getSurfaceHolder());
            mPlayer.setScreenOnWhilePlaying(true);

            mOnBufferingUpdateCalled.reset();
            MediaPlayer2.MediaPlayer2EventCallback ecb =
                new MediaPlayer2.MediaPlayer2EventCallback() {
                    @Override
                    public void onError(MediaPlayer2 mp, DataSourceDesc dsd, int what, int extra) {
                        fail("Media player had error " + what + " playing " + name);
                    }

                    @Override
                    public void onInfo(MediaPlayer2 mp, DataSourceDesc dsd, int what, int extra) {
                        if (what == MediaPlayer2.MEDIA_INFO_PREPARED) {
                            mOnPrepareCalled.signal();
                        } else if (what == MediaPlayer2.MEDIA_INFO_BUFFERING_UPDATE) {
                            mOnBufferingUpdateCalled.signal();
                        }
                    }
                };
            mPlayer.setMediaPlayer2EventCallback(mExecutor, ecb);

            assertFalse(mOnBufferingUpdateCalled.isSignalled());

            mPlayer.prepare();
            mOnPrepareCalled.waitForSignal();

            if (nolength) {
                mPlayer.play();
                Thread.sleep(LONG_SLEEP_TIME);
                assertFalse(mPlayer.isPlaying());
            } else {
                mOnBufferingUpdateCalled.waitForSignal();
                mPlayer.play();
                Thread.sleep(SLEEP_TIME);
            }
            mPlayer.stop();
            mPlayer.reset();
        } finally {
            mServer.shutdown();
        }
    }
    private void localHttpsAudioStreamTest(final String name, boolean redirect, boolean nolength)
            throws Throwable {
        mServer = new CtsTestServer(mContext, true);
        try {
            String stream_url = null;
            if (redirect) {
                // Stagefright doesn't have a limit, but we can't test support of infinite redirects
                // Up to 4 redirects seems reasonable though.
                stream_url = mServer.getRedirectingAssetUrl(name, 4);
            } else {
                stream_url = mServer.getAssetUrl(name);
            }
            if (nolength) {
                stream_url = stream_url + "?" + CtsTestServer.NOLENGTH_POSTFIX;
            }

            final Uri uri = Uri.parse(stream_url);
            mPlayer.setDataSource(new DataSourceDesc.Builder()
                    .setDataSource(mContext, uri)
                    .build());

            mPlayer.setDisplay(getActivity().getSurfaceHolder());
            mPlayer.setScreenOnWhilePlaying(true);

            mOnBufferingUpdateCalled.reset();

            MediaPlayer2.MediaPlayer2EventCallback ecb =
                new MediaPlayer2.MediaPlayer2EventCallback() {
                    @Override
                    public void onError(MediaPlayer2 mp, DataSourceDesc dsd, int what, int extra) {
                        mOnErrorCalled.signal();
                    }

                    @Override
                    public void onInfo(MediaPlayer2 mp, DataSourceDesc dsd, int what, int extra) {
                        if (what == MediaPlayer2.MEDIA_INFO_PREPARED) {
                            mOnPrepareCalled.signal();
                        } else if (what == MediaPlayer2.MEDIA_INFO_BUFFERING_UPDATE) {
                            mOnBufferingUpdateCalled.signal();
                        }
                    }
                };
            mPlayer.setMediaPlayer2EventCallback(mExecutor, ecb);

            assertFalse(mOnBufferingUpdateCalled.isSignalled());
            try {
                mPlayer.prepare();
                mOnErrorCalled.waitForSignal();
            } catch (Exception ex) {
                return;
            }
        } finally {
            mServer.shutdown();
        }
    }

    // TODO: unhide this test when we sort out how to expose buffering control API.
    private void doTestBuffering() throws Throwable {
        final String name = "ringer.mp3";
        mServer = new CtsTestServer(mContext);
        try {
            String stream_url = mServer.getAssetUrl(name);

            if (!MediaUtils.checkCodecsForPath(mContext, stream_url)) {
                Log.w(TAG, "can not find stream " + stream_url + ", skipping test");
                return; // skip
            }

            Monitor onSetBufferingParamsCalled = new Monitor();
            MediaPlayer2.MediaPlayer2EventCallback ecb =
                new MediaPlayer2.MediaPlayer2EventCallback() {
                    @Override
                    public void onError(MediaPlayer2 mp, DataSourceDesc dsd, int what, int extra) {
                        fail("Media player had error " + what + " playing " + name);
                    }
                    @Override
                    public void onInfo(MediaPlayer2 mp, DataSourceDesc dsd, int what, int extra) {
                        if (what == MediaPlayer2.MEDIA_INFO_BUFFERING_UPDATE) {
                            mOnBufferingUpdateCalled.signal();
                        }
                    }
                    @Override
                    public void onCallCompleted(MediaPlayer2 mp, DataSourceDesc dsd,
                            int what, int status) {
                        if (what == MediaPlayer2.CALL_COMPLETED_SET_BUFFERING_PARAMS) {
                            mCallStatus = status;
                            onSetBufferingParamsCalled.signal();
                        }
                    }
                };
            mPlayer.setMediaPlayer2EventCallback(mExecutor, ecb);

            // getBufferingParams should be called after setDataSource.
            try {
                BufferingParams params = mPlayer.getBufferingParams();
                fail("MediaPlayer2 failed to check state for getBufferingParams");
            } catch (IllegalStateException e) {
                // expected
            }

            // setBufferingParams should be called after setDataSource.
            BufferingParams params = new BufferingParams.Builder()
                    .setInitialMarkMs(2)
                    .setResumePlaybackMarkMs(3)
                    .build();
            mCallStatus = MediaPlayer2.CALL_STATUS_NO_ERROR;
            onSetBufferingParamsCalled.reset();
            mPlayer.setBufferingParams(params);
            onSetBufferingParamsCalled.waitForSignal();
            assertTrue(mCallStatus != MediaPlayer2.CALL_STATUS_NO_ERROR);

            final Uri uri = Uri.parse(stream_url);
            mPlayer.setDataSource(new DataSourceDesc.Builder()
                    .setDataSource(mContext, uri)
                    .build());

            mPlayer.setDisplay(getActivity().getSurfaceHolder());
            mPlayer.setScreenOnWhilePlaying(true);

            mOnBufferingUpdateCalled.reset();

            assertFalse(mOnBufferingUpdateCalled.isSignalled());

            params = mPlayer.getBufferingParams();

            int newMark = params.getInitialMarkMs() + 2;
            BufferingParams newParams =
                    new BufferingParams.Builder(params).setInitialMarkMs(newMark).build();

            onSetBufferingParamsCalled.reset();
            mPlayer.setBufferingParams(newParams);
            onSetBufferingParamsCalled.waitForSignal();

            int checkMark = -1;
            BufferingParams checkParams = mPlayer.getBufferingParams();
            checkMark = checkParams.getInitialMarkMs();
            assertEquals("marks do not match", newMark, checkMark);

            // TODO: add more dynamic checking, e.g., buffering shall not exceed pre-set mark.

            mPlayer.reset();
        } finally {
            mServer.shutdown();
        }
    }

    public void testPlayHlsStream() throws Throwable {
        if (IGNORE_TESTS) {
            return;
        }
        if (!MediaUtils.checkDecoder(MediaFormat.MIMETYPE_VIDEO_AVC)) {
            return; // skip
        }
        localHlsTest("hls.m3u8", false, false);
    }

    public void testPlayHlsStreamWithQueryString() throws Throwable {
        if (IGNORE_TESTS) {
            return;
        }
        if (!MediaUtils.checkDecoder(MediaFormat.MIMETYPE_VIDEO_AVC)) {
            return; // skip
        }
        localHlsTest("hls.m3u8", true, false);
    }

    public void testPlayHlsStreamWithRedirect() throws Throwable {
        if (IGNORE_TESTS) {
            return;
        }
        if (!MediaUtils.checkDecoder(MediaFormat.MIMETYPE_VIDEO_AVC)) {
            return; // skip
        }
        localHlsTest("hls.m3u8", false, true);
    }

    public void testPlayHlsStreamWithTimedId3() throws Throwable {
        if (IGNORE_TESTS) {
            return;
        }
        if (!MediaUtils.checkDecoder(MediaFormat.MIMETYPE_VIDEO_AVC)) {
            Log.d(TAG, "Device doesn't have video codec, skipping test");
            return;
        }

        mServer = new CtsTestServer(mContext);
        try {
            // counter must be final if we want to access it inside onTimedMetaData;
            // use AtomicInteger so we can have a final counter object with mutable integer value.
            final AtomicInteger counter = new AtomicInteger();
            String stream_url = mServer.getAssetUrl("prog_index.m3u8");
            final Uri uri = Uri.parse(stream_url);
            mPlayer.setDataSource(new DataSourceDesc.Builder()
                    .setDataSource(mContext, uri)
                    .build());
            mPlayer.setDisplay(getActivity().getSurfaceHolder());
            mPlayer.setScreenOnWhilePlaying(true);
            mPlayer.setWakeMode(mContext, PowerManager.PARTIAL_WAKE_LOCK);

            final Object completion = new Object();
            MediaPlayer2.MediaPlayer2EventCallback ecb =
                new MediaPlayer2.MediaPlayer2EventCallback() {
                    int run;
                    @Override
                    public void onInfo(MediaPlayer2 mp, DataSourceDesc dsd, int what, int extra) {
                        if (what == MediaPlayer2.MEDIA_INFO_PREPARED) {
                            mOnPrepareCalled.signal();
                        } else if (what == MediaPlayer2.MEDIA_INFO_PLAYBACK_COMPLETE) {
                            if (run++ == 0) {
                                mPlayer.seekTo(0, MediaPlayer2.SEEK_PREVIOUS_SYNC);
                                mPlayer.play();
                            } else {
                                mPlayer.stop();
                                synchronized (completion) {
                                    completion.notify();
                                }
                            }
                        }
                    }

                @Override
                public void onTimedMetaDataAvailable(MediaPlayer2 mp, DataSourceDesc dsd,
                        TimedMetaData md) {
                    counter.incrementAndGet();
                    long pos = mp.getCurrentPosition();
                    long timeUs = md.getTimestamp();
                    byte[] rawData = md.getMetaData();
                    // Raw data contains an id3 tag holding the decimal string representation of
                    // the associated time stamp rounded to the closest half second.

                    int offset = 0;
                    offset += 3; // "ID3"
                    offset += 2; // version
                    offset += 1; // flags
                    offset += 4; // size
                    offset += 4; // "TXXX"
                    offset += 4; // frame size
                    offset += 2; // frame flags
                    offset += 1; // "\x03" : UTF-8 encoded Unicode
                    offset += 1; // "\x00" : null-terminated empty description

                    int length = rawData.length;
                    length -= offset;
                    length -= 1; // "\x00" : terminating null

                    String data = new String(rawData, offset, length);
                    int dataTimeUs = Integer.parseInt(data);
                    assertTrue("Timed ID3 timestamp does not match content",
                            Math.abs(dataTimeUs - timeUs) < 500000);
                    assertTrue("Timed ID3 arrives after timestamp", pos * 1000 < timeUs);
                }

                @Override
                public void onCallCompleted(MediaPlayer2 mp, DataSourceDesc dsd,
                        int what, int status) {
                    if (what == MediaPlayer2.CALL_COMPLETED_PLAY) {
                        mOnPlayCalled.signal();
                    }
                }
            };
            mPlayer.setMediaPlayer2EventCallback(mExecutor, ecb);

            mPlayer.prepare();
            mOnPrepareCalled.waitForSignal();

            mOnPlayCalled.reset();
            mPlayer.play();
            mOnPlayCalled.waitForSignal();
            assertTrue("MediaPlayer2 not playing", mPlayer.isPlaying());

            int i = -1;
            List<TrackInfo> trackInfos = mPlayer.getTrackInfo();
            for (i = 0; i < trackInfos.size(); i++) {
                TrackInfo trackInfo = trackInfos.get(i);
                if (trackInfo.getTrackType() == TrackInfo.MEDIA_TRACK_TYPE_METADATA) {
                    break;
                }
            }
            assertTrue("Stream has no timed ID3 track", i >= 0);
            mPlayer.selectTrack(i);

            synchronized (completion) {
                completion.wait();
            }

            // There are a total of 19 metadata access units in the test stream; every one of them
            // should be received twice: once before the seek and once after.
            assertTrue("Incorrect number of timed ID3s recieved", counter.get() == 38);
        } finally {
            mServer.shutdown();
        }
    }

    private static class WorkerWithPlayer implements Runnable {
        private final Object mLock = new Object();
        private Looper mLooper;
        private MediaPlayer2 mPlayer;

        /**
         * Creates a worker thread with the given name. The thread
         * then runs a {@link android.os.Looper}.
         * @param name A name for the new thread
         */
        WorkerWithPlayer(String name) {
            Thread t = new Thread(null, this, name);
            t.setPriority(Thread.MIN_PRIORITY);
            t.start();
            synchronized (mLock) {
                while (mLooper == null) {
                    try {
                        mLock.wait();
                    } catch (InterruptedException ex) {
                    }
                }
            }
        }

        public MediaPlayer2 getPlayer() {
            return mPlayer;
        }

        @Override
        public void run() {
            synchronized (mLock) {
                Looper.prepare();
                mLooper = Looper.myLooper();
                mPlayer = MediaPlayer2.create();
                mLock.notifyAll();
            }
            Looper.loop();
        }

        public void quit() {
            mLooper.quit();
            mPlayer.close();
        }
    }

    public void testBlockingReadRelease() throws Throwable {
        if (IGNORE_TESTS) {
            return;
        }

        mServer = new CtsTestServer(mContext);

        WorkerWithPlayer worker = new WorkerWithPlayer("player");
        final MediaPlayer2 mp = worker.getPlayer();

        try {
            String path = mServer.getDelayedAssetUrl("noiseandchirps.ogg", 15000);
            final Uri uri = Uri.parse(path);
            mp.setDataSource(new DataSourceDesc.Builder()
                    .setDataSource(mContext, uri)
                    .build());

            MediaPlayer2.MediaPlayer2EventCallback ecb =
                new MediaPlayer2.MediaPlayer2EventCallback() {
                    @Override
                    public void onInfo(MediaPlayer2 mp, DataSourceDesc dsd, int what, int extra) {
                        if (what == MediaPlayer2.MEDIA_INFO_PREPARED) {
                            fail("prepare should not succeed");
                        }
                    }
                };
            mp.setMediaPlayer2EventCallback(mExecutor, ecb);

            mp.prepare();
            Thread.sleep(1000);
            long start = SystemClock.elapsedRealtime();
            mp.close();
            long end = SystemClock.elapsedRealtime();
            long releaseDuration = (end - start);
            assertTrue("release took too long: " + releaseDuration, releaseDuration < 1000);
        } catch (IllegalArgumentException e) {
            fail(e.getMessage());
        } catch (SecurityException e) {
            fail(e.getMessage());
        } catch (IllegalStateException e) {
            fail(e.getMessage());
        } catch (InterruptedException e) {
            fail(e.getMessage());
        } finally {
            mServer.shutdown();
        }

        // give the worker a bit of time to start processing the message before shutting it down
        Thread.sleep(5000);
        worker.quit();
    }

    private void localHlsTest(final String name, boolean appendQueryString, boolean redirect)
            throws Throwable {
        mServer = new CtsTestServer(mContext);
        try {
            String stream_url = null;
            if (redirect) {
                stream_url = mServer.getQueryRedirectingAssetUrl(name);
            } else {
                stream_url = mServer.getAssetUrl(name);
            }
            if (appendQueryString) {
                stream_url += "?foo=bar/baz";
            }

            playLiveVideoTest(stream_url, 10);
        } finally {
            mServer.shutdown();
        }
    }
}
