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

import android.Manifest;
import android.net.Uri;
import android.os.Build;
import android.os.Environment;
import android.platform.test.annotations.AppModeFull;
import android.support.test.filters.LargeTest;
import android.support.test.filters.SdkSuppress;
import android.support.test.rule.GrantPermissionRule;
import android.support.test.runner.AndroidJUnit4;

import org.junit.After;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;

import java.io.File;

/**
 * Tests for the MediaPlayer2 API and local video/audio playback.
 *
 * The files in res/raw used by testLocalVideo* are (c) copyright 2008,
 * Blender Foundation / www.bigbuckbunny.org, and are licensed under the Creative Commons
 * Attribution 3.0 License at http://creativecommons.org/licenses/by/3.0/us/.
 */
@LargeTest
@RunWith(AndroidJUnit4.class)
@SdkSuppress(minSdkVersion = Build.VERSION_CODES.P)
@AppModeFull(reason = "Instant apps cannot hold READ/WRITE_EXTERNAL_STORAGE")
public class MediaPlayer2DrmTest extends MediaPlayer2DrmTestBase {

    private static final String LOG_TAG = "MediaPlayer2DrmTest";

    @Rule
    public GrantPermissionRule mRuntimePermissionRule =
            GrantPermissionRule.grant(Manifest.permission.WRITE_EXTERNAL_STORAGE);

    @Before
    @Override
    public void setUp() throws Throwable {
        super.setUp();
    }

    @After
    @Override
    public void tearDown() throws Throwable {
        super.tearDown();
    }


    //////////////////////////////////////////////////////////////////////////////////////////////
    // Asset helpers

    private static Uri getUriFromFile(String path) {
        return Uri.fromFile(new File(getDownloadedPath(path)));
    }

    private static String getDownloadedPath(String fileName) {
        return getDownloadedFolder() + File.separator + fileName;
    }

    private static String getDownloadedFolder() {
        return Environment.getExternalStoragePublicDirectory(
                Environment.DIRECTORY_DOWNLOADS).getPath();
    }

    private static final class Resolution {
        public final boolean isHD;
        public final int width;
        public final int height;

        Resolution(boolean isHD, int width, int height) {
            this.isHD = isHD;
            this.width = width;
            this.height = height;
        }
    }

    private static final Resolution RES_720P  = new Resolution(true, 1280,  720);
    private static final Resolution RES_AUDIO = new Resolution(false,   0,    0);


    // Assets

    private static final Uri CENC_AUDIO_URL = Uri.parse(
            "https://storage.googleapis.com/wvmedia/cenc/clearkey/car_cenc-20120827-8c-pssh.mp4");
    private static final Uri CENC_AUDIO_URL_DOWNLOADED = getUriFromFile("car_cenc-20120827-8c.mp4");

    private static final Uri CENC_VIDEO_URL = Uri.parse(
            "https://storage.googleapis.com/wvmedia/cenc/clearkey/car_cenc-20120827-88-pssh.mp4");
    private static final Uri CENC_VIDEO_URL_DOWNLOADED = getUriFromFile("car_cenc-20120827-88.mp4");


    // Tests

    @Test
    @LargeTest
    public void testCAR_CLEARKEY_AUDIO_DOWNLOADED_V0_SYNC() throws Exception {
        download(CENC_AUDIO_URL,
                CENC_AUDIO_URL_DOWNLOADED,
                RES_AUDIO,
                ModularDrmTestType.V0_SYNC_TEST);
    }

    @Test
    @LargeTest
    public void testCAR_CLEARKEY_AUDIO_DOWNLOADED_V1_ASYNC() throws Exception {
        download(CENC_AUDIO_URL,
                CENC_AUDIO_URL_DOWNLOADED,
                RES_AUDIO,
                ModularDrmTestType.V1_ASYNC_TEST);
    }

    @Test
    @LargeTest
    public void testCAR_CLEARKEY_AUDIO_DOWNLOADED_V2_SYNC_CONFIG() throws Exception {
        download(CENC_AUDIO_URL,
                CENC_AUDIO_URL_DOWNLOADED,
                RES_AUDIO,
                ModularDrmTestType.V2_SYNC_CONFIG_TEST);
    }

    @Test
    @LargeTest
    public void testCAR_CLEARKEY_AUDIO_DOWNLOADED_V3_ASYNC_DRMPREPARED() throws Exception {
        download(CENC_AUDIO_URL,
                CENC_AUDIO_URL_DOWNLOADED,
                RES_AUDIO,
                ModularDrmTestType.V3_ASYNC_DRMPREPARED_TEST);
    }

    // helpers

    private void stream(Uri uri, Resolution res, ModularDrmTestType testType) throws Exception {
        playModularDrmVideo(uri, res.width, res.height, testType);
    }

    private void download(Uri remote, Uri local, Resolution res, ModularDrmTestType testType)
            throws Exception {
        playModularDrmVideoDownload(remote, local, res.width, res.height, testType);
    }

}
