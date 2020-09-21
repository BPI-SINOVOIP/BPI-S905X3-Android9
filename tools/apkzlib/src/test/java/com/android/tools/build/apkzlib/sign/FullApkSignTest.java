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

package com.android.tools.build.apkzlib.sign;

import static org.junit.Assert.assertArrayEquals;
import static org.junit.Assert.assertNotNull;

import com.android.tools.build.apkzlib.utils.ApkZFileTestUtils;
import com.android.tools.build.apkzlib.utils.ApkZLibPair;
import com.android.tools.build.apkzlib.zip.AlignmentRule;
import com.android.tools.build.apkzlib.zip.AlignmentRules;
import com.android.tools.build.apkzlib.zip.StoredEntry;
import com.android.tools.build.apkzlib.zip.ZFile;
import com.android.tools.build.apkzlib.zip.ZFileOptions;
import com.android.tools.build.apkzlib.zip.ZFileTestConstants;
import java.io.ByteArrayInputStream;
import java.io.File;
import java.security.PrivateKey;
import java.security.cert.X509Certificate;
import org.junit.Rule;
import org.junit.Test;
import org.junit.rules.TemporaryFolder;

/**
 * Tests that verify APK Signature Scheme v2 signing using {@link SigningExtension}.
 */
public class FullApkSignTest {

    /**
     * Folder used for tests.
     */
    @Rule
    public TemporaryFolder mTemporaryFolder = new TemporaryFolder();

    @Test
    public void testSignature() throws Exception {
        File out = new File(mTemporaryFolder.getRoot(), "apk");

        ApkZLibPair<PrivateKey, X509Certificate> signData =
                SignatureTestUtils.generateSignaturePre18();

        // The byte arrays below are larger when compressed, so we end up storing them uncompressed,
        // which would normally cause them to be 4-aligned. Disable that, to make calculations
        // easier.
        ZFileOptions options = new ZFileOptions();
        options.setAlignmentRule(AlignmentRules.constant(AlignmentRule.NO_ALIGNMENT));

        /*
         * Generate a signed zip.
         */
        ZFile zf = new ZFile(out, options);
        new SigningExtension(13, signData.v2, signData.v1, false, true)
                .register(zf);
        String f1Name = "abc";
        byte[] f1Data = new byte[] { 1, 1, 1, 1 };
        zf.add(f1Name, new ByteArrayInputStream(f1Data));
        String f2Name = "defg";
        byte[] f2Data = new byte[] { 2, 2, 2, 2, 3, 3, 3, 3};
        zf.add(f2Name, new ByteArrayInputStream(f2Data));
        zf.close();

        /*
         * We should see the data in place.
         */
        int f1DataStart = ZFileTestConstants.LOCAL_HEADER_SIZE + f1Name.length();
        int f1DataEnd = f1DataStart + f1Data.length;
        int f2DataStart = f1DataEnd + ZFileTestConstants.LOCAL_HEADER_SIZE + f2Name.length();
        int f2DataEnd = f2DataStart + f2Data.length;

        byte[] read1 = ApkZFileTestUtils.readSegment(out, f1DataStart, f1Data.length);
        assertArrayEquals(f1Data, read1);
        byte[] read2 = ApkZFileTestUtils.readSegment(out, f2DataStart, f2Data.length);
        assertArrayEquals(f2Data, read2);

        /*
         * Read the signed zip.
         */
        ZFile zf2 = new ZFile(out);

        StoredEntry se1 = zf2.get(f1Name);
        assertNotNull(se1);
        assertArrayEquals(f1Data, se1.read());

        StoredEntry se2 = zf2.get(f2Name);
        assertNotNull(se2);
        assertArrayEquals(f2Data, se2.read());

        zf2.close();
    }
}
