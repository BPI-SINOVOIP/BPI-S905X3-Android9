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
 *
 *
 * This code was provided to AOSP by Zimperium Inc and was
 * written by:
 *
 * Simone "evilsocket" Margaritelli
 * Joshua "jduck" Drake
 */
package android.security.cts;

import android.test.AndroidTestCase;
import android.util.Log;
import android.content.Context;
import android.content.res.AssetFileDescriptor;
import android.content.res.Resources;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.SurfaceTexture;
import android.media.MediaCodec;
import android.media.MediaCodecInfo;
import android.media.MediaCodecList;
import android.media.MediaDrm;
import android.media.MediaDrm.MediaDrmStateException;
import android.media.MediaExtractor;
import android.media.MediaFormat;
import android.media.MediaMetadataRetriever;
import android.media.MediaPlayer;
import android.opengl.GLES20;
import android.opengl.GLES11Ext;
import android.os.Looper;
import android.os.SystemClock;
import android.platform.test.annotations.SecurityTest;
import android.test.InstrumentationTestCase;
import android.util.Log;
import android.view.Surface;
import android.webkit.cts.CtsTestServer;

import java.io.BufferedInputStream;
import java.io.FileInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.net.URL;
import java.nio.ByteBuffer;
import java.io.FileOutputStream;
import java.io.File;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.UUID;
import java.util.concurrent.locks.Condition;
import java.util.concurrent.locks.ReentrantLock;

import android.security.cts.R;


/**
 * Verify that the device is not vulnerable to any known Stagefright
 * vulnerabilities.
 */
@SecurityTest
public class StagefrightTest extends InstrumentationTestCase {
    static final String TAG = "StagefrightTest";

    private final long TIMEOUT_NS = 10000000000L;  // 10 seconds.

    public StagefrightTest() {
    }

    /***********************************************************
     to prevent merge conflicts, add K tests below this comment,
     before any existing test methods
     ***********************************************************/

    @SecurityTest
    public void testStagefright_bug_36725407() throws Exception {
        doStagefrightTest(R.raw.bug_36725407);
    }

    @SecurityTest
    public void testStagefright_cve_2016_3829() throws Exception {
        doStagefrightTest(R.raw.cve_2016_3829);
    }

    @SecurityTest
    public void testStagefright_cve_2016_3828() throws Exception {
        doStagefrightTest(R.raw.cve_2016_3828);
    }

    @SecurityTest
    public void testStagefright_bug_64710074() throws Exception {
        doStagefrightTest(R.raw.bug_64710074);
    }

    @SecurityTest
    public void testStagefright_cve_2017_0643() throws Exception {
        doStagefrightTest(R.raw.cve_2017_0643);
    }

    @SecurityTest
    public void testStagefright_cve_2017_0728() throws Exception {
        doStagefrightTest(R.raw.cve_2017_0728);
    }

    @SecurityTest
    public void testStagefright_bug_62187433() throws Exception {
        doStagefrightTest(R.raw.bug_62187433);
    }

    @SecurityTest
    public void testStagefrightANR_bug_62673844() throws Exception {
        doStagefrightTestANR(R.raw.bug_62673844);
    }

    @SecurityTest
    public void testStagefright_bug_37079296() throws Exception {
        doStagefrightTest(R.raw.bug_37079296);
    }

    @SecurityTest
    public void testStagefright_bug_38342499() throws Exception {
        doStagefrightTest(R.raw.bug_38342499);
    }

    @SecurityTest
    public void testStagefright_bug_22771132() throws Exception {
        doStagefrightTest(R.raw.bug_22771132);
    }

    public void testStagefright_bug_21443020() throws Exception {
        doStagefrightTest(R.raw.bug_21443020_webm);
    }

    public void testStagefright_bug_34360591() throws Exception {
        doStagefrightTest(R.raw.bug_34360591);
    }

    public void testStagefright_bug_35763994() throws Exception {
        doStagefrightTest(R.raw.bug_35763994);
    }

    @SecurityTest
    public void testStagefright_bug_33137046() throws Exception {
        doStagefrightTest(R.raw.bug_33137046);
    }

    @SecurityTest
    public void testStagefright_cve_2016_2507() throws Exception {
        doStagefrightTest(R.raw.cve_2016_2507);
    }

    @SecurityTest
    public void testStagefright_bug_31647370() throws Exception {
        doStagefrightTest(R.raw.bug_31647370);
    }

    @SecurityTest
    public void testStagefright_bug_32577290() throws Exception {
        doStagefrightTest(R.raw.bug_32577290);
    }

    @SecurityTest
    public void testStagefright_cve_2015_1538_1() throws Exception {
        doStagefrightTest(R.raw.cve_2015_1538_1);
    }

    @SecurityTest
    public void testStagefright_cve_2015_1538_2() throws Exception {
        doStagefrightTest(R.raw.cve_2015_1538_2);
    }

    @SecurityTest
    public void testStagefright_cve_2015_1538_3() throws Exception {
        doStagefrightTest(R.raw.cve_2015_1538_3);
    }

    @SecurityTest
    public void testStagefright_cve_2015_1538_4() throws Exception {
        doStagefrightTest(R.raw.cve_2015_1538_4);
    }

    @SecurityTest
    public void testStagefright_cve_2015_1539() throws Exception {
        doStagefrightTest(R.raw.cve_2015_1539);
    }

    @SecurityTest
    public void testStagefright_cve_2015_3824() throws Exception {
        doStagefrightTest(R.raw.cve_2015_3824);
    }

    @SecurityTest
    public void testStagefright_cve_2015_3826() throws Exception {
        doStagefrightTest(R.raw.cve_2015_3826);
    }

    @SecurityTest
    public void testStagefright_cve_2015_3827() throws Exception {
        doStagefrightTest(R.raw.cve_2015_3827);
    }

    @SecurityTest
    public void testStagefright_cve_2015_3828() throws Exception {
        doStagefrightTest(R.raw.cve_2015_3828);
    }

    @SecurityTest
    public void testStagefright_cve_2015_3829() throws Exception {
        doStagefrightTest(R.raw.cve_2015_3829);
    }

    @SecurityTest
    public void testStagefright_cve_2015_3836() throws Exception {
        doStagefrightTest(R.raw.cve_2015_3836);
    }

    @SecurityTest
    public void testStagefright_cve_2015_3864() throws Exception {
        doStagefrightTest(R.raw.cve_2015_3864);
    }

    @SecurityTest
    public void testStagefright_cve_2015_3864_b23034759() throws Exception {
        doStagefrightTest(R.raw.cve_2015_3864_b23034759);
    }

    @SecurityTest
    public void testStagefright_cve_2015_6598() throws Exception {
        doStagefrightTest(R.raw.cve_2015_6598);
    }

    @SecurityTest
    public void testStagefright_cve_2016_6766() throws Exception {
        doStagefrightTest(R.raw.cve_2016_6766);
    }

    @SecurityTest
    public void testStagefright_bug_26366256() throws Exception {
        doStagefrightTest(R.raw.bug_26366256);
    }

    @SecurityTest
    public void testStagefright_cve_2016_2429_b_27211885() throws Exception {
        doStagefrightTest(R.raw.cve_2016_2429_b_27211885);
    }

    @SecurityTest
    public void testStagefright_bug_34031018() throws Exception {
        doStagefrightTest(R.raw.bug_34031018_32bit);
        doStagefrightTest(R.raw.bug_34031018_64bit);
    }

    /***********************************************************
     to prevent merge conflicts, add L tests below this comment,
     before any existing test methods
     ***********************************************************/

    @SecurityTest
    public void testStagefright_bug_65123471() throws Exception {
        doStagefrightTest(R.raw.bug_65123471);
    }

    @SecurityTest
    public void testStagefright_bug_72165027() throws Exception {
        doStagefrightTest(R.raw.bug_72165027);
    }

    @SecurityTest
    public void testStagefright_bug_65483665() throws Exception {
        doStagefrightTest(R.raw.bug_65483665);
    }

    @SecurityTest
    public void testStagefright_cve_2017_0852_b_62815506() throws Exception {
        doStagefrightTest(R.raw.cve_2017_0852_b_62815506);
    }

    @SecurityTest
    public void testStagefright_cve_2017_13229() throws Exception {
        doStagefrightTest(R.raw.cve_2017_13229);
    }

    @SecurityTest
    public void testStagefright_cve_2017_0763() throws Exception {
        doStagefrightTest(R.raw.cve_2017_0763);
    }

    /***********************************************************
     to prevent merge conflicts, add M tests below this comment,
     before any existing test methods
     ***********************************************************/

    @SecurityTest
    public void testStagefright_bug_68953854() throws Exception {
        doStagefrightTest(R.raw.bug_68953854, 1 * 60 * 1000);
    }

    @SecurityTest
    public void testStagefright_bug_38448381() throws Exception {
        doStagefrightTest(R.raw.bug_38448381);
    }

    @SecurityTest
    public void testStagefright_cve_2016_3821() throws Exception {
        doStagefrightTest(R.raw.cve_2016_3821);
    }

    @SecurityTest
    public void testStagefright_bug_70897454() throws Exception {
        doStagefrightTestRawBlob(R.raw.b70897454_avc, "video/avc", 320, 420);
    }

    @SecurityTest
    public void testStagefright_bug_38115076() throws Exception {
        doStagefrightTest(R.raw.bug_38115076);
    }

    @SecurityTest
    public void testStagefright_bug_34618607() throws Exception {
        doStagefrightTest(R.raw.bug_34618607);
    }

    @SecurityTest
    public void testStagefright_bug_69478425() throws Exception {
        doStagefrightTest(R.raw.bug_69478425);
    }

    @SecurityTest
    public void testStagefright_bug_65735716() throws Exception {
        doStagefrightTestRawBlob(R.raw.bug_65735716_avc, "video/avc", 320, 240);
    }

    @SecurityTest
    public void testStagefright_bug_65717533() throws Exception {
        doStagefrightTest(R.raw.bug_65717533_header_corrupt);
    }

    @SecurityTest
    public void testStagefright_bug_38239864() throws Exception {
        doStagefrightTest(R.raw.bug_38239864, (4 * 60 * 1000));
    }

    @SecurityTest
    public void testStagefright_cve_2017_0600() throws Exception {
        doStagefrightTest(R.raw.cve_2017_0600);
    }

    @SecurityTest
    public void testBug_38014992() throws Exception {
        int[] frameSizes = getFrameSizes(R.raw.bug_38014992_framelen);
        doStagefrightTestRawBlob(R.raw.bug_38014992_avc, "video/avc", 640, 480, frameSizes);
    }

    @SecurityTest
    public void testBug_35584425() throws Exception {
        int[] frameSizes = getFrameSizes(R.raw.bug_35584425_framelen);
        doStagefrightTestRawBlob(R.raw.bug_35584425_avc, "video/avc", 352, 288, frameSizes);
    }

    @SecurityTest
    public void testBug_31092462() throws Exception {
        int[] frameSizes = getFrameSizes(R.raw.bug_31092462_framelen);
        doStagefrightTestRawBlob(R.raw.bug_31092462_avc, "video/avc", 1280, 1024, frameSizes);
    }

    @SecurityTest
    public void testBug_34097866() throws Exception {
        int[] frameSizes = getFrameSizes(R.raw.bug_34097866_frame_len);
        doStagefrightTestRawBlob(R.raw.bug_34097866_avc, "video/avc", 352, 288, frameSizes);
    }

    @SecurityTest
    public void testBug_33862021() throws Exception {
        int[] frameSizes = getFrameSizes(R.raw.bug_33862021_frame_len);
        doStagefrightTestRawBlob(R.raw.bug_33862021_hevc, "video/hevc", 160, 96, frameSizes);
    }

    @SecurityTest
    public void testBug_33387820() throws Exception {
        int[] frameSizes = {45, 3202, 430, 2526};
        doStagefrightTestRawBlob(R.raw.bug_33387820_avc, "video/avc", 320, 240, frameSizes);
    }

    @SecurityTest
    public void testBug_37008096() throws Exception {
        int[] frameSizes = {245, 12, 33, 140, 164};
        doStagefrightTestRawBlob(R.raw.bug_37008096_avc, "video/avc", 320, 240, frameSizes);
    }

    @SecurityTest
    public void testStagefright_bug_34231163() throws Exception {
        int[] frameSizes = {22, 357, 217, 293, 175};
        doStagefrightTestRawBlob(R.raw.bug_34231163_mpeg2, "video/mpeg2", 320, 240, frameSizes);
    }

    @SecurityTest
    public void testStagefright_bug_33933140() throws Exception {
        int[] frameSizes = getFrameSizes(R.raw.bug_33933140_framelen);
        doStagefrightTestRawBlob(R.raw.bug_33933140_avc, "video/avc", 320, 240, frameSizes);
    }

    @SecurityTest
    public void testStagefright_bug_34097915() throws Exception {
        int[] frameSizes = {4140, 593, 0, 15495};
        doStagefrightTestRawBlob(R.raw.bug_34097915_avc, "video/avc", 320, 240, frameSizes);
    }

    @SecurityTest
    public void testStagefright_bug_34097213() throws Exception {
        int[] frameSizes = {2571, 210, 33858};
        doStagefrightTestRawBlob(R.raw.bug_34097213_avc, "video/avc", 320, 240, frameSizes);
    }

    @SecurityTest
    public void testBug_28816956() throws Exception {
        int[] frameSizes = getFrameSizes(R.raw.bug_28816956_framelen);
        doStagefrightTestRawBlob(R.raw.bug_28816956_hevc, "video/hevc", 352, 288, frameSizes);
    }

    @SecurityTest
    public void testBug_33818500() throws Exception {
        int[] frameSizes = getFrameSizes(R.raw.bug_33818500_framelen);
        doStagefrightTestRawBlob(R.raw.bug_33818500_avc, "video/avc", 64, 32, frameSizes);
    }

    @SecurityTest
    public void testBug_64784973() throws Exception {
        int[] frameSizes = getFrameSizes(R.raw.bug_64784973_framelen);
        doStagefrightTestRawBlob(R.raw.bug_64784973_hevc, "video/hevc", 1280, 720, frameSizes);
    }

    @SecurityTest
    public void testBug_34231231() throws Exception {
        int[] frameSizes = getFrameSizes(R.raw.bug_34231231_framelen);
        doStagefrightTestRawBlob(R.raw.bug_34231231_mpeg2, "video/mpeg2", 352, 288, frameSizes);
    }

    @SecurityTest
    public void testBug_63045918() throws Exception {
        int[] frameSizes = getFrameSizes(R.raw.bug_63045918_framelen);
        doStagefrightTestRawBlob(R.raw.bug_63045918_hevc, "video/hevc", 352, 288, frameSizes);
    }

    @SecurityTest
    public void testBug_33298089() throws Exception {
        int[] frameSizes = {3247, 430, 221, 2305};
        doStagefrightTestRawBlob(R.raw.bug_33298089_avc, "video/avc", 32, 64, frameSizes);
    }

    @SecurityTest
    public void testStagefright_cve_2017_0599() throws Exception {
        doStagefrightTest(R.raw.cve_2017_0599);
    }

    @SecurityTest
    public void testStagefright_bug_36492741() throws Exception {
        doStagefrightTest(R.raw.bug_36492741);
    }

    @SecurityTest
    public void testStagefright_bug_38487564() throws Exception {
        doStagefrightTest(R.raw.bug_38487564, (4 * 60 * 1000));
    }

    @SecurityTest
    public void testStagefright_bug_37237396() throws Exception {
        doStagefrightTest(R.raw.bug_37237396);
    }

    @SecurityTest
    public void testStagefright_cve_2016_0842() throws Exception {
        doStagefrightTest(R.raw.cve_2016_0842);
    }

    @SecurityTest
    public void testStagefright_bug_63121644() throws Exception {
        doStagefrightTest(R.raw.bug_63121644);
    }

    @SecurityTest
    public void testStagefright_cve_2016_6712() throws Exception {
        doStagefrightTest(R.raw.cve_2016_6712);
    }

    @SecurityTest
    public void testStagefright_bug_34097231() throws Exception {
        doStagefrightTestRawBlob(R.raw.bug_34097231_avc, "video/avc", 320, 240);
    }

    @SecurityTest
    public void testStagefright_bug_34097672() throws Exception {
        doStagefrightTest(R.raw.bug_34097672);
    }


    @SecurityTest
    public void testStagefright_bug_33751193() throws Exception {
        doStagefrightTestRawBlob(R.raw.bug_33751193_avc, "video/avc", 320, 240);
    }

    @SecurityTest
    public void testBug_36993291() throws Exception {
        doStagefrightTestRawBlob(R.raw.bug_36993291_avc, "video/avc", 320, 240);
    }

    @SecurityTest
    public void testStagefright_bug_33818508() throws Exception {
        doStagefrightTest(R.raw.bug_33818508);
    }

    @SecurityTest
    public void testStagefright_bug_32873375() throws Exception {
        doStagefrightTest(R.raw.bug_32873375);
    }

    @SecurityTest
    public void testStagefright_bug_63522067() throws Exception {
        doStagefrightTestRawBlob(R.raw.bug_63522067_1_hevc, "video/hevc", 320, 420);
        doStagefrightTestRawBlob(R.raw.bug_63522067_2_hevc, "video/hevc", 320, 420);
        doStagefrightTestRawBlob(R.raw.bug_63522067_3_hevc, "video/hevc", 320, 420);
        doStagefrightTestRawBlob(R.raw.bug_63522067_4_hevc, "video/hevc", 320, 420);
    }

    @SecurityTest
    public void testStagefright_bug_25765591() throws Exception {
        doStagefrightTest(R.raw.bug_25765591);
    }

    @SecurityTest
    public void testStagefright_bug_62673179() throws Exception {
        doStagefrightTest(R.raw.bug_62673179_ts, (4 * 60 * 1000));
    }

    @SecurityTest
    public void testStagefright_bug_69269702() throws Exception {
        doStagefrightTest(R.raw.bug_69269702);
    }

    @SecurityTest
    public void testStagefright_cve_2015_3867() throws Exception {
        doStagefrightTest(R.raw.cve_2015_3867);
    }

    @SecurityTest
    public void testStagefright_bug_65398821() throws Exception {
        doStagefrightTest(R.raw.bug_65398821, ( 4 * 60 * 1000 ) );
    }

    @SecurityTest
    public void testStagefright_cve_2015_3869() throws Exception {
        doStagefrightTest(R.raw.cve_2015_3869);
    }

    @SecurityTest
    public void testStagefright_bug_23452792() throws Exception {
        doStagefrightTest(R.raw.bug_23452792);
    }

    @SecurityTest
    public void testStagefright_cve_2016_3820() throws Exception {
        doStagefrightTest(R.raw.cve_2016_3820);
    }

    @SecurityTest
    public void testStagefright_cve_2016_3741() throws Exception {
        doStagefrightTest(R.raw.cve_2016_3741);
    }

    @SecurityTest
    public void testStagefright_cve_2016_2506() throws Exception {
        doStagefrightTest(R.raw.cve_2016_2506);
    }

    @SecurityTest
    public void testStagefright_cve_2016_2428() throws Exception {
        doStagefrightTest(R.raw.cve_2016_2428);
    }

    @SecurityTest
    public void testStagefright_bug_36592202() throws Exception {
        Resources resources = getInstrumentation().getContext().getResources();
        AssetFileDescriptor fd = resources.openRawResourceFd(R.raw.bug_36592202);
        int page_size = 25627;
        byte [] blob = new byte[page_size];

        // 127 bytes read and  25500 zeros constitute one Ogg page
        FileInputStream fis = fd.createInputStream();
        int numRead = fis.read(blob);
        fis.close();

        // Creating temp file
        final File tempFile = File.createTempFile("poc_tmp", ".ogg", null);

        try {
            final FileOutputStream tempFos = new FileOutputStream(tempFile.getAbsolutePath());
            int bytesWritten = 0;
            // Repeat data till size is ~1 GB
            for (int i = 0; i < 50000; i++) {
                tempFos.write(blob);
                bytesWritten += page_size;
            }
            tempFos.close();

            final int fileSize = bytesWritten;
            int timeout = (10 * 60 * 1000);

            runWithTimeout(new Runnable() {
                @Override
                public void run() {
                    try {
                        doStagefrightTestMediaCodec(tempFile.getAbsolutePath());
                    } catch (Exception | AssertionError  e) {
                        if (!tempFile.delete()) {
                            Log.e(TAG, "Failed to delete temporary PoC file");
                        }
                        fail("Operation was not successful");
                    }
                }
            }, timeout);
        } catch (Exception e) {
            fail("Failed to test b/36592202");
        } finally {
            if (!tempFile.delete()) {
                Log.e(TAG, "Failed to delete temporary PoC file");
            }
        }
    }

    @SecurityTest
    public void testStagefright_bug_30822755() throws Exception {
        doStagefrightTest(R.raw.bug_30822755);
    }

    @SecurityTest
    public void testStagefright_bug_32322258() throws Exception {
        doStagefrightTest(R.raw.bug_32322258);
    }

    @SecurityTest
    public void testStagefright_bug_37710346() throws Exception {
        UUID CLEARKEY_SCHEME_UUID = new UUID(0x1077efecc0b24d02L, 0xace33c1e52e2fb4bL);

        String drmInitString = "0000003470737368" +
                               "01000000" +
                               "1077efecc0b24d02" +
                               "ace33c1e52e2fb4b" +
                               "10000001" +
                               "60061e017e477e87" +
                               "7e57d00d1ed00d1e" +
                               "00000000";
        int len = drmInitString.length();
        byte[] drmInitData = new byte[len / 2];
        for (int i = 0; i < len; i += 2) {
            drmInitData[i / 2] = (byte) ((Character.digit(drmInitString.charAt(i), 16) << 4) +
                Character.digit(drmInitString.charAt(i + 1), 16));
        }

        try {
            MediaDrm drm = new MediaDrm(CLEARKEY_SCHEME_UUID);
            byte[] sessionId;
            String initDataType = "video/mp4";

            sessionId = drm.openSession();
            MediaDrm.KeyRequest drmRequest = drm.getKeyRequest(sessionId, drmInitData,
                initDataType, MediaDrm.KEY_TYPE_STREAMING, null);
        } catch (Exception e) {
            if (!(e instanceof MediaDrmStateException))
                fail("media drm server died");
        }
    }

    @SecurityTest
    public void testStagefright_cve_2015_3873_b_23248776() throws Exception {
        doStagefrightTest(R.raw.cve_2015_3873_b_23248776);
    }

    @SecurityTest
    public void testStagefright_bug_35472997() throws Exception {
        doStagefrightTest(R.raw.bug_35472997);
    }

    @SecurityTest
    public void testStagefright_cve_2015_3873_b_20718524() throws Exception {
        doStagefrightTest(R.raw.cve_2015_3873_b_20718524);
    }

    @SecurityTest
    public void testStagefright_bug_34896431() throws Exception {
        doStagefrightTest(R.raw.bug_34896431);
    }

    @SecurityTest
    public void testStagefright_cve_2015_3862_b_22954006() throws Exception {
        doStagefrightTest(R.raw.cve_2015_3862_b_22954006);
    }

    @SecurityTest
    public void testStagefright_cve_2015_3867_b_23213430() throws Exception {
        doStagefrightTest(R.raw.cve_2015_3867_b_23213430);
    }

    @SecurityTest
    public void testStagefright_cve_2015_3873_b_21814993() throws Exception {
        doStagefrightTest(R.raw.cve_2015_3873_b_21814993);
    }

    @SecurityTest
    public void testStagefright_bug_25812590() throws Exception {
        doStagefrightTest(R.raw.bug_25812590);
    }

    public void testStagefright_cve_2015_6600() throws Exception {
        doStagefrightTest(R.raw.cve_2015_6600);
    }

    public void testStagefright_cve_2015_6603() throws Exception {
        doStagefrightTest(R.raw.cve_2015_6603);
    }

    public void testStagefright_cve_2015_6604() throws Exception {
        doStagefrightTest(R.raw.cve_2015_6604);
    }

    @SecurityTest
    public void testStagefright_bug_24157524() throws Exception {
        doStagefrightTest(R.raw.bug_24157524);
    }

    @SecurityTest
    public void testStagefright_cve_2015_3871() throws Exception {
        doStagefrightTest(R.raw.cve_2015_3871);
    }

    public void testStagefright_bug_26070014() throws Exception {
        doStagefrightTest(R.raw.bug_26070014);
    }

    public void testStagefright_bug_32915871() throws Exception {
        doStagefrightTest(R.raw.bug_32915871);
    }

    @SecurityTest
    public void testStagefright_bug_28333006() throws Exception {
        doStagefrightTest(R.raw.bug_28333006);
    }

    @SecurityTest
    public void testStagefright_bug_14388161() throws Exception {
        doStagefrightTestMediaPlayer(R.raw.bug_14388161);
    }

    @SecurityTest
    public void testStagefright_cve_2016_3755() throws Exception {
        doStagefrightTest(R.raw.cve_2016_3755);
    }

    @SecurityTest
    public void testStagefright_cve_2016_3878_b_29493002() throws Exception {
        doStagefrightTest(R.raw.cve_2016_3878_b_29493002);
    }

    @SecurityTest
    public void testStagefright_cve_2015_6608_b_23680780() throws Exception {
        doStagefrightTest(R.raw.cve_2015_6608_b_23680780);
    }

    @SecurityTest
    public void testStagefright_bug_27855419_CVE_2016_2463() throws Exception {
        doStagefrightTest(R.raw.bug_27855419);
    }

    public void testStagefright_bug_19779574() throws Exception {
        doStagefrightTest(R.raw.bug_19779574);
    }

    /***********************************************************
     to prevent merge conflicts, add N tests below this comment,
     before any existing test methods
     ***********************************************************/

    @SecurityTest
    public void testStagefright_cve_2017_0474() throws Exception {
        doStagefrightTest(R.raw.cve_2017_0474, 120000);
    }

    @SecurityTest
    public void testStagefright_cve_2017_0765() throws Exception {
        doStagefrightTest(R.raw.cve_2017_0765);
    }

    @SecurityTest
    public void testStagefright_cve_2017_13276() throws Exception {
        doStagefrightTest(R.raw.cve_2017_13276);
    }

    @SecurityTest
    public void testStagefright_cve_2016_6764() throws Exception {
        doStagefrightTest(R.raw.cve_2016_6764);
    }

    @SecurityTest
    public void testStagefright_cve_2017_13214() throws Exception {
        doStagefrightTest(R.raw.cve_2017_13214);
    }

    @SecurityTest
    public void testStagefright_bug_35467107() throws Exception {
        doStagefrightTest(R.raw.bug_35467107);
    }

    /***********************************************************
     to prevent merge conflicts, add O tests below this comment,
     before any existing test methods
     ***********************************************************/

    @SecurityTest
    public void testBug_67737022() throws Exception {
        doStagefrightTest(R.raw.bug_67737022);
    }

    @SecurityTest
    public void testStagefright_bug_37093318() throws Exception {
        doStagefrightTest(R.raw.bug_37093318, (4 * 60 * 1000));
    }

    @SecurityTest
    public void testStagefright_bug_73172046() throws Exception {
        doStagefrightTest(R.raw.bug_73172046);

        Bitmap bitmap = BitmapFactory.decodeResource(
                getInstrumentation().getContext().getResources(), R.raw.bug_73172046);
        // OK if the decoding failed, but shouldn't cause crashes
        if (bitmap != null) {
            bitmap.recycle();
        }
    }

    @SecurityTest
    public void testStagefright_cve_2016_0824() throws Exception {
        doStagefrightTest(R.raw.cve_2016_0824);
    }

    @SecurityTest
    public void testStagefright_cve_2016_0815() throws Exception {
        doStagefrightTest(R.raw.cve_2016_0815);
    }

    @SecurityTest
    public void testStagefright_cve_2016_2454() throws Exception {
        doStagefrightTest(R.raw.cve_2016_2454);
    }

    @SecurityTest
    public void testStagefright_cve_2016_6765() throws Exception {
        doStagefrightTest(R.raw.cve_2016_6765);
    }

    @SecurityTest
    public void testStagefright_cve_2016_2508() throws Exception {
        doStagefrightTest(R.raw.cve_2016_2508);
    }

    @SecurityTest
    public void testStagefright_cve_2016_6699() throws Exception {
        doStagefrightTest(R.raw.cve_2016_6699);
    }

    private void doStagefrightTest(final int rid) throws Exception {
        doStagefrightTestMediaPlayer(rid);
        doStagefrightTestMediaCodec(rid);
        doStagefrightTestMediaMetadataRetriever(rid);

        Context context = getInstrumentation().getContext();
        Resources resources =  context.getResources();
        CtsTestServer server = new CtsTestServer(context);
        String rname = resources.getResourceEntryName(rid);
        String url = server.getAssetUrl("raw/" + rname);
        verifyServer(rid, url);
        doStagefrightTestMediaPlayer(url);
        doStagefrightTestMediaCodec(url);
        doStagefrightTestMediaMetadataRetriever(url);
        server.shutdown();
    }

    // verify that CtsTestServer is functional by retrieving the asset
    // and comparing it to the resource
    private void verifyServer(final int rid, final String uri) throws Exception {
        Log.i(TAG, "checking server");
        URL url = new URL(uri);
        InputStream in1 = new BufferedInputStream(url.openStream());

        AssetFileDescriptor fd = getInstrumentation().getContext().getResources()
                        .openRawResourceFd(rid);
        InputStream in2 = new BufferedInputStream(fd.createInputStream());

        while (true) {
            int b1 = in1.read();
            int b2 = in2.read();
            assertEquals("CtsTestServer fail", b1, b2);
            if (b1 < 0) {
                break;
            }
        }

        in1.close();
        in2.close();
        Log.i(TAG, "checked server");
    }

    private void doStagefrightTest(final int rid, int timeout) throws Exception {
        runWithTimeout(new Runnable() {
            @Override
            public void run() {
                try {
                  doStagefrightTest(rid);
                } catch (Exception e) {
                  fail(e.toString());
                }
            }
        }, timeout);
    }

    private void doStagefrightTestANR(final int rid) throws Exception {
        doStagefrightTestMediaPlayerANR(rid, null);
    }

    private Surface getDummySurface() {
        int[] textures = new int[1];
        GLES20.glGenTextures(1, textures, 0);
        GLES20.glBindTexture(GLES11Ext.GL_TEXTURE_EXTERNAL_OES, textures[0]);
        GLES20.glTexParameterf(GLES11Ext.GL_TEXTURE_EXTERNAL_OES,
                GLES20.GL_TEXTURE_MIN_FILTER,
                GLES20.GL_NEAREST);
        GLES20.glTexParameterf(GLES11Ext.GL_TEXTURE_EXTERNAL_OES,
                GLES20.GL_TEXTURE_MAG_FILTER,
                GLES20.GL_LINEAR);
        GLES20.glTexParameteri(GLES11Ext.GL_TEXTURE_EXTERNAL_OES,
                GLES20.GL_TEXTURE_WRAP_S,
                GLES20.GL_CLAMP_TO_EDGE);
        GLES20.glTexParameteri(GLES11Ext.GL_TEXTURE_EXTERNAL_OES,
                GLES20.GL_TEXTURE_WRAP_T,
                GLES20.GL_CLAMP_TO_EDGE);
        SurfaceTexture surfaceTex = new SurfaceTexture(textures[0]);
        surfaceTex.setOnFrameAvailableListener(new SurfaceTexture.OnFrameAvailableListener() {
            @Override
            public void onFrameAvailable(SurfaceTexture surfaceTexture) {
                Log.i(TAG, "new frame available");
            }
        });
        return new Surface(surfaceTex);
    }

    class MediaPlayerCrashListener
    implements MediaPlayer.OnErrorListener,
        MediaPlayer.OnPreparedListener,
        MediaPlayer.OnCompletionListener {
        @Override
        public boolean onError(MediaPlayer mp, int newWhat, int extra) {
            Log.i(TAG, "error: " + newWhat + "/" + extra);
            // don't overwrite a more severe error with a less severe one
            if (what != MediaPlayer.MEDIA_ERROR_SERVER_DIED) {
                what = newWhat;
            }
            lock.lock();
            condition.signal();
            lock.unlock();

            return true; // don't call oncompletion
        }

        @Override
        public void onPrepared(MediaPlayer mp) {
            mp.start();
        }

        @Override
        public void onCompletion(MediaPlayer mp) {
            // preserve error condition, if any
            lock.lock();
            completed = true;
            condition.signal();
            lock.unlock();
        }

        public int waitForError() throws InterruptedException {
            lock.lock();
            if (condition.awaitNanos(TIMEOUT_NS) <= 0) {
                Log.d(TAG, "timed out on waiting for error");
            }
            lock.unlock();
            if (what != 0) {
                // Sometimes mediaserver signals a decoding error first, and *then* crashes
                // due to additional in-flight buffers being processed, so wait a little
                // and see if more errors show up.
                SystemClock.sleep(1000);
            }
            return what;
        }

        public boolean waitForErrorOrCompletion() throws InterruptedException {
            lock.lock();
            if (condition.awaitNanos(TIMEOUT_NS) <= 0) {
                Log.d(TAG, "timed out on waiting for error or completion");
            }
            lock.unlock();
            return (what != 0 && what != MediaPlayer.MEDIA_ERROR_SERVER_DIED) || completed;
        }

        ReentrantLock lock = new ReentrantLock();
        Condition condition = lock.newCondition();
        int what;
        boolean completed = false;
    }

    class LooperThread extends Thread {
        private Looper mLooper;

        LooperThread(Runnable runner) {
            super(runner);
        }

        @Override
        public void run() {
            Looper.prepare();
            mLooper = Looper.myLooper();
            super.run();
        }

        public void stopLooper() {
            mLooper.quitSafely();
        }
    }

    private void doStagefrightTestMediaPlayer(final int rid) throws Exception {
        doStagefrightTestMediaPlayer(rid, null);
    }

    private void doStagefrightTestMediaPlayer(final String url) throws Exception {
        doStagefrightTestMediaPlayer(-1, url);
    }

    private void closeQuietly(AutoCloseable closeable) {
        if (closeable != null) {
            try {
                closeable.close();
            } catch (RuntimeException rethrown) {
                throw rethrown;
            } catch (Exception ignored) {
            }
        }
    }

    private void doStagefrightTestMediaPlayer(final int rid, final String uri) throws Exception {

        String name = uri != null ? uri :
            getInstrumentation().getContext().getResources().getResourceEntryName(rid);
        Log.i(TAG, "start mediaplayer test for: " + name);

        final MediaPlayerCrashListener mpcl = new MediaPlayerCrashListener();

        LooperThread t = new LooperThread(new Runnable() {
            @Override
            public void run() {

                MediaPlayer mp = new MediaPlayer();
                mp.setOnErrorListener(mpcl);
                mp.setOnPreparedListener(mpcl);
                mp.setOnCompletionListener(mpcl);
                Surface surface = getDummySurface();
                mp.setSurface(surface);
                AssetFileDescriptor fd = null;
                try {
                    if (uri == null) {
                        fd = getInstrumentation().getContext().getResources()
                                .openRawResourceFd(rid);

                        mp.setDataSource(fd.getFileDescriptor(),
                                         fd.getStartOffset(),
                                         fd.getLength());

                    } else {
                        mp.setDataSource(uri);
                    }
                    mp.prepareAsync();
                } catch (Exception e) {
                } finally {
                    closeQuietly(fd);
                }

                Looper.loop();
                mp.release();
            }
        });

        t.start();
        String cve = name.replace("_", "-").toUpperCase();
        assertFalse("Device *IS* vulnerable to " + cve,
                    mpcl.waitForError() == MediaPlayer.MEDIA_ERROR_SERVER_DIED);
        t.stopLooper();
        t.join(); // wait for thread to exit so we're sure the player was released
    }

    private void doStagefrightTestMediaCodec(final int rid) throws Exception {
        doStagefrightTestMediaCodec(rid, null);
    }

    private void doStagefrightTestMediaCodec(final String url) throws Exception {
        doStagefrightTestMediaCodec(-1, url);
    }

    private void doStagefrightTestMediaCodec(final int rid, final String url) throws Exception {

        final MediaPlayerCrashListener mpcl = new MediaPlayerCrashListener();

        LooperThread thr = new LooperThread(new Runnable() {
            @Override
            public void run() {

                MediaPlayer mp = new MediaPlayer();
                mp.setOnErrorListener(mpcl);
                try {
                    AssetFileDescriptor fd = getInstrumentation().getContext().getResources()
                        .openRawResourceFd(R.raw.good);

                    // the onErrorListener won't receive MEDIA_ERROR_SERVER_DIED until
                    // setDataSource has been called
                    mp.setDataSource(fd.getFileDescriptor(),
                                     fd.getStartOffset(),
                                     fd.getLength());
                    fd.close();
                } catch (Exception e) {
                    // this is a known-good file, so no failure should occur
                    fail("setDataSource of known-good file failed");
                }

                synchronized(mpcl) {
                    mpcl.notify();
                }
                Looper.loop();
                mp.release();
            }
        });
        thr.start();
        // wait until the thread has initialized the MediaPlayer
        synchronized(mpcl) {
            mpcl.wait();
        }

        Resources resources =  getInstrumentation().getContext().getResources();
        MediaExtractor ex = new MediaExtractor();
        if (url == null) {
            AssetFileDescriptor fd = resources.openRawResourceFd(rid);
            try {
                ex.setDataSource(fd.getFileDescriptor(), fd.getStartOffset(), fd.getLength());
            } catch (IOException e) {
                // ignore
            } finally {
                closeQuietly(fd);
            }
        } else {
            try {
                ex.setDataSource(url);
            } catch (Exception e) {
                // indicative of problems with our tame CTS test web server
            }
        }
        int numtracks = ex.getTrackCount();
        String rname = url != null ? url: resources.getResourceEntryName(rid);
        Log.i(TAG, "start mediacodec test for: " + rname + ", which has " + numtracks + " tracks");
        for (int t = 0; t < numtracks; t++) {
            // find all the available decoders for this format
            ArrayList<String> matchingCodecs = new ArrayList<String>();
            MediaFormat format = null;
            try {
                format = ex.getTrackFormat(t);
            } catch (IllegalArgumentException e) {
                Log.e(TAG, "could not get track format for track " + t);
                continue;
            }
            String mime = format.getString(MediaFormat.KEY_MIME);
            int numCodecs = MediaCodecList.getCodecCount();
            for (int i = 0; i < numCodecs; i++) {
                MediaCodecInfo info = MediaCodecList.getCodecInfoAt(i);
                if (info.isEncoder()) {
                    continue;
                }
                try {
                    MediaCodecInfo.CodecCapabilities caps = info.getCapabilitiesForType(mime);
                    if (caps != null) {
                        matchingCodecs.add(info.getName());
                        Log.i(TAG, "Found matching codec " + info.getName() + " for track " + t);
                    }
                } catch (IllegalArgumentException e) {
                    // type is not supported
                }
            }

            if (matchingCodecs.size() == 0) {
                Log.w(TAG, "no codecs for track " + t + ", type " + mime);
            }
            // decode this track once with each matching codec
            try {
                ex.selectTrack(t);
            } catch (IllegalArgumentException e) {
                Log.w(TAG, "couldn't select track " + t);
                // continue on with codec initialization anyway, since that might still crash
            }
            for (String codecName: matchingCodecs) {
                Log.i(TAG, "Decoding track " + t + " using codec " + codecName);
                ex.seekTo(0, MediaExtractor.SEEK_TO_CLOSEST_SYNC);
                MediaCodec codec = MediaCodec.createByCodecName(codecName);
                Surface surface = null;
                if (mime.startsWith("video/")) {
                    surface = getDummySurface();
                }
                try {
                    codec.configure(format, surface, null, 0);
                    codec.start();
                } catch (Exception e) {
                    Log.i(TAG, "Failed to start/configure:", e);
                }
                MediaCodec.BufferInfo info = new MediaCodec.BufferInfo();
                try {
                    ByteBuffer [] inputBuffers = codec.getInputBuffers();
                    while (true) {
                        int flags = ex.getSampleFlags();
                        long time = ex.getSampleTime();
                        ex.getCachedDuration();
                        int bufidx = codec.dequeueInputBuffer(5000);
                        if (bufidx >= 0) {
                            int n = ex.readSampleData(inputBuffers[bufidx], 0);
                            if (n < 0) {
                                flags = MediaCodec.BUFFER_FLAG_END_OF_STREAM;
                                time = 0;
                                n = 0;
                            }
                            codec.queueInputBuffer(bufidx, 0, n, time, flags);
                            ex.advance();
                        }
                        int status = codec.dequeueOutputBuffer(info, 5000);
                        if (status >= 0) {
                            if ((info.flags & MediaCodec.BUFFER_FLAG_END_OF_STREAM) != 0) {
                                break;
                            }
                            if (info.presentationTimeUs > TIMEOUT_NS / 1000) {
                                Log.d(TAG, "stopping after 10 seconds worth of data");
                                break;
                            }
                            codec.releaseOutputBuffer(status, true);
                        }
                    }
                } catch (Exception e) {
                    // local exceptions ignored, not security issues
                } finally {
                    codec.release();
                }
            }
            ex.unselectTrack(t);
        }
        ex.release();
        String cve = rname.replace("_", "-").toUpperCase();
        assertFalse("Device *IS* vulnerable to " + cve,
                    mpcl.waitForError() == MediaPlayer.MEDIA_ERROR_SERVER_DIED);
        thr.stopLooper();
        thr.join();
    }

    private void doStagefrightTestMediaMetadataRetriever(final int rid) throws Exception {
        doStagefrightTestMediaMetadataRetriever(rid, null);
    }

    private void doStagefrightTestMediaMetadataRetriever(final String url) throws Exception {
        doStagefrightTestMediaMetadataRetriever(-1, url);
    }

    private void doStagefrightTestMediaMetadataRetriever(
            final int rid, final String url) throws Exception {

        final MediaPlayerCrashListener mpcl = new MediaPlayerCrashListener();

        LooperThread thr = new LooperThread(new Runnable() {
            @Override
            public void run() {

                MediaPlayer mp = new MediaPlayer();
                mp.setOnErrorListener(mpcl);
                AssetFileDescriptor fd = null;
                try {
                    fd = getInstrumentation().getContext().getResources()
                        .openRawResourceFd(R.raw.good);

                    // the onErrorListener won't receive MEDIA_ERROR_SERVER_DIED until
                    // setDataSource has been called
                    mp.setDataSource(fd.getFileDescriptor(),
                                     fd.getStartOffset(),
                                     fd.getLength());
                    fd.close();
                } catch (Exception e) {
                    // this is a known-good file, so no failure should occur
                    fail("setDataSource of known-good file failed");
                }

                synchronized(mpcl) {
                    mpcl.notify();
                }
                Looper.loop();
                mp.release();
            }
        });
        thr.start();
        // wait until the thread has initialized the MediaPlayer
        synchronized(mpcl) {
            mpcl.wait();
        }

        Resources resources =  getInstrumentation().getContext().getResources();
        MediaMetadataRetriever retriever = new MediaMetadataRetriever();
        if (url == null) {
            AssetFileDescriptor fd = resources.openRawResourceFd(rid);
            try {
                retriever.setDataSource(fd.getFileDescriptor(), fd.getStartOffset(), fd.getLength());
            } catch (Exception e) {
                // ignore
            } finally {
                closeQuietly(fd);
            }
        } else {
            try {
                retriever.setDataSource(url, new HashMap<String, String>());
            } catch (Exception e) {
                // indicative of problems with our tame CTS test web server
            }
        }
        retriever.extractMetadata(MediaMetadataRetriever.METADATA_KEY_DURATION);
        retriever.getEmbeddedPicture();
        retriever.getFrameAtTime();

        retriever.release();
        String rname = url != null ? url : resources.getResourceEntryName(rid);
        String cve = rname.replace("_", "-").toUpperCase();
        assertFalse("Device *IS* vulnerable to " + cve,
                    mpcl.waitForError() == MediaPlayer.MEDIA_ERROR_SERVER_DIED);
        thr.stopLooper();
        thr.join();
    }

    public void testBug36215950() throws Exception {
        doStagefrightTestRawBlob(R.raw.bug_36215950, "video/hevc", 320, 240);
    }

    public void testBug36816007() throws Exception {
        doStagefrightTestRawBlob(R.raw.bug_36816007, "video/avc", 320, 240);
    }

    public void testBug36895511() throws Exception {
        doStagefrightTestRawBlob(R.raw.bug_36895511, "video/hevc", 320, 240);
    }

    public void testBug64836894() throws Exception {
        doStagefrightTestRawBlob(R.raw.bug_64836894, "video/avc", 320, 240);
    }

    @SecurityTest
    public void testCve_2017_0687() throws Exception {
        doStagefrightTestRawBlob(R.raw.cve_2017_0687, "video/avc", 320, 240);
    }

    @SecurityTest
    public void testBug_37930177() throws Exception {
        doStagefrightTestRawBlob(R.raw.bug_37930177_hevc, "video/hevc", 320, 240);
    }

    @SecurityTest
    public void testBug_37712181() throws Exception {
        doStagefrightTestRawBlob(R.raw.bug_37712181_hevc, "video/hevc", 320, 240);
    }

    @SecurityTest
    public void testBug_70897394() throws Exception {
        doStagefrightTestRawBlob(R.raw.bug_70897394_avc, "video/avc", 320, 240);
    }

    private int[] getFrameSizes(int rid) throws IOException {
        final Context context = getInstrumentation().getContext();
        final Resources resources =  context.getResources();
        AssetFileDescriptor fd = resources.openRawResourceFd(rid);
        FileInputStream fis = fd.createInputStream();
        byte[] frameInfo = new byte[(int) fd.getLength()];
        fis.read(frameInfo);
        fis.close();
        String[] valueStr = new String(frameInfo).trim().split("\\s+");
        int[] frameSizes = new int[valueStr.length];
        for (int i = 0; i < valueStr.length; i++)
            frameSizes[i] = Integer.parseInt(valueStr[i]);
        return frameSizes;
    }

    private void runWithTimeout(Runnable runner, int timeout) {
        Thread t = new Thread(runner);
        t.start();
        try {
            t.join(timeout);
        } catch (InterruptedException e) {
            fail("operation was interrupted");
        }
        if (t.isAlive()) {
            fail("operation not completed within timeout of " + timeout + "ms");
        }
    }

    private void releaseCodec(final MediaCodec codec) {
        runWithTimeout(new Runnable() {
            @Override
            public void run() {
                codec.release();
            }
        }, 5000);
    }

    private void doStagefrightTestRawBlob(int rid, String mime, int initWidth, int initHeight) throws Exception {

        final MediaPlayerCrashListener mpcl = new MediaPlayerCrashListener();
        final Context context = getInstrumentation().getContext();
        final Resources resources =  context.getResources();

        LooperThread thr = new LooperThread(new Runnable() {
            @Override
            public void run() {

                MediaPlayer mp = new MediaPlayer();
                mp.setOnErrorListener(mpcl);
                AssetFileDescriptor fd = null;
                try {
                    fd = resources.openRawResourceFd(R.raw.good);

                    // the onErrorListener won't receive MEDIA_ERROR_SERVER_DIED until
                    // setDataSource has been called
                    mp.setDataSource(fd.getFileDescriptor(),
                                     fd.getStartOffset(),
                                     fd.getLength());
                    fd.close();
                } catch (Exception e) {
                    // this is a known-good file, so no failure should occur
                    fail("setDataSource of known-good file failed");
                }

                synchronized(mpcl) {
                    mpcl.notify();
                }
                Looper.loop();
                mp.release();
            }
        });
        thr.start();
        // wait until the thread has initialized the MediaPlayer
        synchronized(mpcl) {
            mpcl.wait();
        }

        AssetFileDescriptor fd = resources.openRawResourceFd(rid);
        byte [] blob = new byte[(int)fd.getLength()];
        FileInputStream fis = fd.createInputStream();
        int numRead = fis.read(blob);
        fis.close();
        //Log.i("@@@@", "read " + numRead + " bytes");

        // find all the available decoders for this format
        ArrayList<String> matchingCodecs = new ArrayList<String>();
        int numCodecs = MediaCodecList.getCodecCount();
        for (int i = 0; i < numCodecs; i++) {
            MediaCodecInfo info = MediaCodecList.getCodecInfoAt(i);
            if (info.isEncoder()) {
                continue;
            }
            try {
                MediaCodecInfo.CodecCapabilities caps = info.getCapabilitiesForType(mime);
                if (caps != null) {
                    matchingCodecs.add(info.getName());
                }
            } catch (IllegalArgumentException e) {
                // type is not supported
            }
        }

        if (matchingCodecs.size() == 0) {
            Log.w(TAG, "no codecs for mime type " + mime);
        }
        String rname = resources.getResourceEntryName(rid);
        // decode this blob once with each matching codec
        for (String codecName: matchingCodecs) {
            Log.i(TAG, "Decoding blob " + rname + " using codec " + codecName);
            MediaCodec codec = MediaCodec.createByCodecName(codecName);
            MediaFormat format = MediaFormat.createVideoFormat(mime, initWidth, initHeight);
            codec.configure(format, null, null, 0);
            codec.start();

            try {
                MediaCodec.BufferInfo info = new MediaCodec.BufferInfo();
                ByteBuffer [] inputBuffers = codec.getInputBuffers();
                // enqueue the bad data a number of times, in case
                // the codec needs multiple buffers to fail.
                for(int i = 0; i < 64; i++) {
                    int bufidx = codec.dequeueInputBuffer(5000);
                    if (bufidx >= 0) {
                        Log.i(TAG, "got input buffer of size " + inputBuffers[bufidx].capacity());
                        inputBuffers[bufidx].rewind();
                        inputBuffers[bufidx].put(blob, 0, numRead);
                        codec.queueInputBuffer(bufidx, 0, numRead, 0, 0);
                    } else {
                        Log.i(TAG, "no input buffer");
                    }
                    bufidx = codec.dequeueOutputBuffer(info, 5000);
                    if (bufidx >= 0) {
                        Log.i(TAG, "got output buffer");
                        codec.releaseOutputBuffer(bufidx, false);
                    } else {
                        Log.i(TAG, "no output buffer");
                    }
                }
            } catch (Exception e) {
                // ignore, not a security issue
            } finally {
                releaseCodec(codec);
            }
        }

        String cve = rname.replace("_", "-").toUpperCase();
        assertFalse("Device *IS* vulnerable to " + cve,
                    mpcl.waitForError() == MediaPlayer.MEDIA_ERROR_SERVER_DIED);
        thr.stopLooper();
        thr.join();
    }

    private void doStagefrightTestRawBlob(int rid, String mime, int initWidth, int initHeight,
        int frameSizes[]) throws Exception {

        final MediaPlayerCrashListener mpcl = new MediaPlayerCrashListener();
        final Context context = getInstrumentation().getContext();
        final Resources resources =  context.getResources();

        LooperThread thr = new LooperThread(new Runnable() {
            @Override
            public void run() {

                MediaPlayer mp = new MediaPlayer();
                mp.setOnErrorListener(mpcl);
                AssetFileDescriptor fd = null;
                try {
                    fd = resources.openRawResourceFd(R.raw.good);

                    // the onErrorListener won't receive MEDIA_ERROR_SERVER_DIED until
                    // setDataSource has been called
                    mp.setDataSource(fd.getFileDescriptor(),
                                     fd.getStartOffset(),
                                     fd.getLength());
                    fd.close();
                } catch (Exception e) {
                    // this is a known-good file, so no failure should occur
                    fail("setDataSource of known-good file failed");
                }

                synchronized(mpcl) {
                    mpcl.notify();
                }
                Looper.loop();
                mp.release();
            }
        });
        thr.start();
        // wait until the thread has initialized the MediaPlayer
        synchronized(mpcl) {
            mpcl.wait();
        }

        AssetFileDescriptor fd = resources.openRawResourceFd(rid);
        byte [] blob = new byte[(int)fd.getLength()];
        FileInputStream fis = fd.createInputStream();
        int numRead = fis.read(blob);
        fis.close();

        // find all the available decoders for this format
        ArrayList<String> matchingCodecs = new ArrayList<String>();
        int numCodecs = MediaCodecList.getCodecCount();
        for (int i = 0; i < numCodecs; i++) {
            MediaCodecInfo info = MediaCodecList.getCodecInfoAt(i);
            if (info.isEncoder()) {
                continue;
            }
            try {
                MediaCodecInfo.CodecCapabilities caps = info.getCapabilitiesForType(mime);
                if (caps != null) {
                    matchingCodecs.add(info.getName());
                }
            } catch (IllegalArgumentException e) {
                // type is not supported
            }
        }

        if (matchingCodecs.size() == 0) {
            Log.w(TAG, "no codecs for mime type " + mime);
        }
        String rname = resources.getResourceEntryName(rid);
        // decode this blob once with each matching codec
        for (String codecName: matchingCodecs) {
            Log.i(TAG, "Decoding blob " + rname + " using codec " + codecName);
            MediaCodec codec = MediaCodec.createByCodecName(codecName);
            MediaFormat format = MediaFormat.createVideoFormat(mime, initWidth, initHeight);
            try {
                codec.configure(format, null, null, 0);
                codec.start();
            } catch (Exception e) {
                Log.i(TAG, "Exception from codec " + codecName);
                releaseCodec(codec);
                continue;
            }

            try {
                MediaCodec.BufferInfo info = new MediaCodec.BufferInfo();
                ByteBuffer [] inputBuffers = codec.getInputBuffers();
                int numFrames = 0;
                if (frameSizes != null) {
                    numFrames = frameSizes.length;
                }

                if (0 == numFrames) {
                    fail("Improper picture length file");
                }

                int offset = 0;
                int bytesToFeed = 0;
                int flags = 0;
                byte [] tempBlob = new byte[(int)inputBuffers[0].capacity()];
                for (int j = 0; j < numFrames; j++) {
                    int bufidx = codec.dequeueInputBuffer(5000);
                    if (bufidx >= 0) {
                        inputBuffers[bufidx].rewind();
                        bytesToFeed = Math.min((int)(fd.getLength() - offset),
                                               inputBuffers[bufidx].capacity());
                        if(j == (numFrames - 1)) {
                            flags = MediaCodec.BUFFER_FLAG_END_OF_STREAM;
                        }
                        System.arraycopy(blob, offset, tempBlob, 0, bytesToFeed);
                        inputBuffers[bufidx].put(tempBlob, 0, inputBuffers[bufidx].capacity());
                        codec.queueInputBuffer(bufidx, 0, bytesToFeed, 0, flags);
                        offset = offset + frameSizes[j];
                    } else {
                        Log.i(TAG, "no input buffer");
                    }
                    bufidx = codec.dequeueOutputBuffer(info, 5000);
                    if (bufidx >= 0) {
                        codec.releaseOutputBuffer(bufidx, false);
                    } else {
                      Log.i(TAG, "no output buffer");
                    }
                }
            } catch (Exception e) {
                // ignore, not a security issue
            } finally {
                releaseCodec(codec);
            }
        }

        String cve = rname.replace("_", "-").toUpperCase();
        assertFalse("Device *IS* vulnerable to " + cve,
                    mpcl.waitForError() == MediaPlayer.MEDIA_ERROR_SERVER_DIED);
        thr.stopLooper();
        thr.join();
    }

    private void doStagefrightTestMediaPlayerANR(final int rid, final String uri) throws Exception {
        String name = uri != null ? uri :
            getInstrumentation().getContext().getResources().getResourceEntryName(rid);
        Log.i(TAG, "start mediaplayerANR test for: " + name);

        final MediaPlayerCrashListener mpl = new MediaPlayerCrashListener();

        LooperThread t = new LooperThread(new Runnable() {
            @Override
            public void run() {
                MediaPlayer mp = new MediaPlayer();
                mp.setOnErrorListener(mpl);
                mp.setOnPreparedListener(mpl);
                mp.setOnCompletionListener(mpl);
                Surface surface = getDummySurface();
                mp.setSurface(surface);
                AssetFileDescriptor fd = null;
                try {
                    if (uri == null) {
                        fd = getInstrumentation().getContext().getResources()
                                .openRawResourceFd(rid);

                        mp.setDataSource(fd.getFileDescriptor(),
                                fd.getStartOffset(),
                                fd.getLength());
                    } else {
                        mp.setDataSource(uri);
                    }
                    mp.prepareAsync();
                } catch (Exception e) {
                } finally {
                    closeQuietly(fd);
                }

                Looper.loop();
                mp.release();
            }
        });

        t.start();
        String cve = name.replace("_", "-").toUpperCase();
        assertTrue("Device *IS* vulnerable to " + cve, mpl.waitForErrorOrCompletion());
        t.stopLooper();
        t.join(); // wait for thread to exit so we're sure the player was released
    }
}
