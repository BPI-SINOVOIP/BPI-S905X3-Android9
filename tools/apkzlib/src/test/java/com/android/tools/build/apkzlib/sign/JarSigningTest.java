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

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotEquals;
import static org.junit.Assert.assertNotNull;

import com.android.tools.build.apkzlib.utils.ApkZFileTestUtils;
import com.android.tools.build.apkzlib.utils.ApkZLibPair;
import com.android.tools.build.apkzlib.zip.StoredEntry;
import com.android.tools.build.apkzlib.zip.ZFile;
import com.google.common.base.Charsets;
import com.google.common.hash.Hashing;
import java.io.ByteArrayInputStream;
import java.io.File;
import java.io.InputStream;
import java.security.PrivateKey;
import java.security.cert.X509Certificate;
import java.util.Base64;
import java.util.jar.Attributes;
import java.util.jar.Manifest;
import org.junit.Rule;
import org.junit.Test;
import org.junit.rules.TemporaryFolder;

public class JarSigningTest {

    @Rule
    public TemporaryFolder mTemporaryFolder = new TemporaryFolder();

    @Test
    public void signEmptyJar() throws Exception {
        File zipFile = new File(mTemporaryFolder.getRoot(), "a.zip");

        try (ZFile zf = new ZFile(zipFile)) {
            ApkZFileTestUtils.addAndroidManifest(zf);
            ManifestGenerationExtension manifestExtension =
                    new ManifestGenerationExtension("Me", "Me");
            manifestExtension.register(zf);

            ApkZLibPair<PrivateKey, X509Certificate> p =
                    SignatureTestUtils.generateSignaturePre18();

            new SigningExtension(12, p.v2, p.v1, true, false).register(zf);
        }

        try (ZFile verifyZFile = new ZFile(zipFile)) {
            StoredEntry manifestEntry = verifyZFile.get("META-INF/MANIFEST.MF");
            assertNotNull(manifestEntry);

            Manifest manifest = new Manifest(new ByteArrayInputStream(manifestEntry.read()));
            assertEquals(3, manifest.getMainAttributes().size());
            assertEquals("1.0", manifest.getMainAttributes().getValue("Manifest-Version"));
            assertEquals("Me", manifest.getMainAttributes().getValue("Created-By"));
            assertEquals("Me", manifest.getMainAttributes().getValue("Built-By"));
        }
    }

    @Test
    public void signJarWithPrexistingSimpleTextFilePre18() throws Exception {
        File zipFile = new File(mTemporaryFolder.getRoot(), "a.zip");
        ApkZLibPair<PrivateKey, X509Certificate> p = SignatureTestUtils.generateSignaturePre18();

        try (ZFile zf1 = new ZFile(zipFile)) {
            ApkZFileTestUtils.addAndroidManifest(zf1);
            zf1.add("directory/file",
                    new ByteArrayInputStream("useless text".getBytes(Charsets.US_ASCII)));
        }

        try (ZFile zf2 = new ZFile(zipFile)) {
            ManifestGenerationExtension me = new ManifestGenerationExtension("Merry", "Christmas");
            me.register(zf2);
            new SigningExtension(10, p.v2, p.v1, true, false).register(zf2);
        }

        try (ZFile zf3 = new ZFile(zipFile)) {
            StoredEntry manifestEntry = zf3.get("META-INF/MANIFEST.MF");
            assertNotNull(manifestEntry);

            Manifest manifest = new Manifest(new ByteArrayInputStream(manifestEntry.read()));
            assertEquals(3, manifest.getMainAttributes().size());
            assertEquals("1.0", manifest.getMainAttributes().getValue("Manifest-Version"));
            assertEquals("Merry", manifest.getMainAttributes().getValue("Built-By"));
            assertEquals("Christmas", manifest.getMainAttributes().getValue("Created-By"));

            Attributes attrs = manifest.getAttributes("directory/file");
            assertNotNull(attrs);
            assertEquals(1, attrs.size());
            assertEquals("OOQgIEXBissIvva3ydRoaXk29Rk=", attrs.getValue("SHA1-Digest"));

            StoredEntry signatureEntry = zf3.get("META-INF/CERT.SF");
            assertNotNull(signatureEntry);

            Manifest signature = new Manifest(new ByteArrayInputStream(signatureEntry.read()));
            assertEquals(3, signature.getMainAttributes().size());
            assertEquals("1.0", signature.getMainAttributes().getValue("Signature-Version"));
            assertEquals("1.0 (Android)", signature.getMainAttributes().getValue("Created-By"));

            byte[] manifestTextBytes = manifestEntry.read();
            byte[] manifestSha1Bytes = Hashing.sha1().hashBytes(manifestTextBytes).asBytes();
            String manifestSha1 = Base64.getEncoder().encodeToString(manifestSha1Bytes);

            assertEquals(manifestSha1,
                    signature.getMainAttributes().getValue("SHA1-Digest-Manifest"));

            Attributes signAttrs = signature.getAttributes("directory/file");
            assertNotNull(signAttrs);
            assertEquals(1, signAttrs.size());
            assertEquals("LGSOwy4uGcUWoc+ZhS8ukzmf0fY=", signAttrs.getValue("SHA1-Digest"));

            StoredEntry rsaEntry = zf3.get("META-INF/CERT.RSA");
            assertNotNull(rsaEntry);
        }
    }

    @Test
    public void signJarWithPrexistingSimpleTextFilePos18() throws Exception {
        File zipFile = new File(mTemporaryFolder.getRoot(), "a.zip");
        try (ZFile zf1 = new ZFile(zipFile)) {
            ApkZFileTestUtils.addAndroidManifest(zf1);
            zf1.add("directory/file", new ByteArrayInputStream("useless text".getBytes(
                    Charsets.US_ASCII)));
        }

        ApkZLibPair<PrivateKey, X509Certificate> p = SignatureTestUtils.generateSignaturePos18();

        try (ZFile zf2 = new ZFile(zipFile)) {
            ManifestGenerationExtension me = new ManifestGenerationExtension("Merry", "Christmas");
            me.register(zf2);
            new SigningExtension(21, p.v2, p.v1, true, false).register(zf2);
        }

        try (ZFile zf3 = new ZFile(zipFile)) {
            StoredEntry manifestEntry = zf3.get("META-INF/MANIFEST.MF");
            assertNotNull(manifestEntry);

            Manifest manifest = new Manifest(new ByteArrayInputStream(manifestEntry.read()));
            assertEquals(3, manifest.getMainAttributes().size());
            assertEquals("1.0", manifest.getMainAttributes().getValue("Manifest-Version"));
            assertEquals("Merry", manifest.getMainAttributes().getValue("Built-By"));
            assertEquals("Christmas", manifest.getMainAttributes().getValue("Created-By"));

            Attributes attrs = manifest.getAttributes("directory/file");
            assertNotNull(attrs);
            assertEquals(1, attrs.size());
            assertEquals("QjupZsopQM/01O6+sWHqH64ilMmoBEtljg9VEqN6aI4=",
                    attrs.getValue("SHA-256-Digest"));

            StoredEntry signatureEntry = zf3.get("META-INF/CERT.SF");
            assertNotNull(signatureEntry);

            Manifest signature = new Manifest(new ByteArrayInputStream(signatureEntry.read()));
            assertEquals(3, signature.getMainAttributes().size());
            assertEquals("1.0", signature.getMainAttributes().getValue("Signature-Version"));
            assertEquals("1.0 (Android)", signature.getMainAttributes().getValue("Created-By"));

            byte[] manifestTextBytes = manifestEntry.read();
            byte[] manifestSha256Bytes = Hashing.sha256().hashBytes(manifestTextBytes).asBytes();
            String manifestSha256 = Base64.getEncoder().encodeToString(manifestSha256Bytes);

            assertEquals(manifestSha256, signature.getMainAttributes().getValue(
                    "SHA-256-Digest-Manifest"));

            Attributes signAttrs = signature.getAttributes("directory/file");
            assertNotNull(signAttrs);
            assertEquals(1, signAttrs.size());
            assertEquals("dBnaLpqNjmUnLlZF4tNqOcDWL8wy8Tsw1ZYFqTZhjIs=",
                    signAttrs.getValue("SHA-256-Digest"));

            StoredEntry ecdsaEntry = zf3.get("META-INF/CERT.EC");
            assertNotNull(ecdsaEntry);
        }
    }

    @Test
    public void v2SignAddsApkSigningBlock() throws Exception {
        File zipFile = new File(mTemporaryFolder.getRoot(), "a.zip");
        try (ZFile zf = new ZFile(zipFile)) {
            ApkZFileTestUtils.addAndroidManifest(zf);
            ManifestGenerationExtension manifestExtension =
                    new ManifestGenerationExtension("Me", "Me");
            manifestExtension.register(zf);

            ApkZLibPair<PrivateKey, X509Certificate> p = SignatureTestUtils.generateSignaturePre18();

            new SigningExtension(12, p.v2, p.v1, false, true).register(zf);
        }

        try (ZFile verifyZFile = new ZFile(zipFile)) {
            long centralDirOffset = verifyZFile.getCentralDirectoryOffset();
            byte[] apkSigningBlockMagic = new byte[16];
            verifyZFile.directFullyRead(
                    centralDirOffset - apkSigningBlockMagic.length, apkSigningBlockMagic);
            assertEquals("APK Sig Block 42", new String(apkSigningBlockMagic, "US-ASCII"));
        }
    }

    @Test
    public void v1ReSignOnFileChange() throws Exception {
        File zipFile = new File(mTemporaryFolder.getRoot(), "a.zip");
        ApkZLibPair<PrivateKey, X509Certificate> p = SignatureTestUtils.generateSignaturePos18();

        byte[] file1Contents = "I am a test file".getBytes(Charsets.US_ASCII);
        String file1Name = "path/to/file1";
        byte[] file1Sha = Hashing.sha256().hashBytes(file1Contents).asBytes();
        String file1ShaTxt = Base64.getEncoder().encodeToString(file1Sha);

        String builtBy = "Santa Claus";
        String createdBy = "Uses Android";

        try (ZFile zf1 = new ZFile(zipFile)) {
            ApkZFileTestUtils.addAndroidManifest(zf1);
            zf1.add(file1Name, new ByteArrayInputStream(file1Contents));
            ManifestGenerationExtension me = new ManifestGenerationExtension(builtBy, createdBy);
            me.register(zf1);
            new SigningExtension(21, p.v2, p.v1, true, false).register(zf1);

            zf1.update();

            StoredEntry manifestEntry = zf1.get("META-INF/MANIFEST.MF");
            assertNotNull(manifestEntry);

            try (InputStream manifestIs = manifestEntry.open()) {
                Manifest manifest = new Manifest(manifestIs);

                assertEquals(2, manifest.getEntries().size());

                Attributes file1Attrs = manifest.getEntries().get(file1Name);
                assertNotNull(file1Attrs);
                assertEquals(file1ShaTxt, file1Attrs.getValue("SHA-256-Digest"));
            }

            /*
             * Change the file without closing the zip.
             */
            file1Contents = "I am a modified test file".getBytes(Charsets.US_ASCII);
            file1Sha = Hashing.sha256().hashBytes(file1Contents).asBytes();
            file1ShaTxt = Base64.getEncoder().encodeToString(file1Sha);

            zf1.add(file1Name, new ByteArrayInputStream(file1Contents));

            zf1.update();

            manifestEntry = zf1.get("META-INF/MANIFEST.MF");
            assertNotNull(manifestEntry);

            try (InputStream manifestIs = manifestEntry.open()) {
                Manifest manifest = new Manifest(manifestIs);

                assertEquals(2, manifest.getEntries().size());

                Attributes file1Attrs = manifest.getEntries().get(file1Name);
                assertNotNull(file1Attrs);
                assertEquals(file1ShaTxt, file1Attrs.getValue("SHA-256-Digest"));
            }
        }

        /*
         * Change the file closing the zip.
         */
        file1Contents = "I have changed again!".getBytes(Charsets.US_ASCII);
        file1Sha = Hashing.sha256().hashBytes(file1Contents).asBytes();
        file1ShaTxt = Base64.getEncoder().encodeToString(file1Sha);

        try (ZFile zf2 = new ZFile(zipFile)) {
            ApkZFileTestUtils.addAndroidManifest(zf2);
            ManifestGenerationExtension me = new ManifestGenerationExtension(builtBy, createdBy);
            me.register(zf2);
            new SigningExtension(21, p.v2, p.v1, true, false).register(zf2);

            zf2.add(file1Name, new ByteArrayInputStream(file1Contents));

            zf2.update();

            StoredEntry manifestEntry = zf2.get("META-INF/MANIFEST.MF");
            assertNotNull(manifestEntry);

            try (InputStream manifestIs = manifestEntry.open()) {
                Manifest manifest = new Manifest(manifestIs);

                assertEquals(2, manifest.getEntries().size());

                Attributes file1Attrs = manifest.getEntries().get(file1Name);
                assertNotNull(file1Attrs);
                assertEquals(file1ShaTxt, file1Attrs.getValue("SHA-256-Digest"));
            }
        }
    }

    @Test
    public void openSignedJarDoesNotForcesWriteIfSignatureIsNotCorrect() throws Exception {
        File zipFile = new File(mTemporaryFolder.getRoot(), "a.zip");

        ApkZLibPair<PrivateKey, X509Certificate> p = SignatureTestUtils.generateSignaturePos18();

        String fileName = "file";
        byte[] fileContents = "Very interesting contents".getBytes(Charsets.US_ASCII);

        try (ZFile zf = new ZFile(zipFile)) {
            ApkZFileTestUtils.addAndroidManifest(zf);
            ManifestGenerationExtension me = new ManifestGenerationExtension("I", "Android");
            me.register(zf);
            new SigningExtension(21, p.v2, p.v1, true, false).register(zf);

            zf.add(fileName, new ByteArrayInputStream(fileContents));
        }

        long fileTimestamp = zipFile.lastModified();

        ApkZFileTestUtils.waitForFileSystemTick(fileTimestamp);

        /*
         * Open the zip file, but don't touch it.
         */
        try (ZFile zf = new ZFile(zipFile)) {
            ManifestGenerationExtension me = new ManifestGenerationExtension("I", "Android");
            me.register(zf);
            new SigningExtension(21, p.v2, p.v1, true, false).register(zf);
        }

        /*
         * Check the file wasn't touched.
         */
        assertEquals(fileTimestamp, zipFile.lastModified());

        /*
         * Change the file contents ignoring any signing.
         */
        fileContents = "Not so interesting contents".getBytes(Charsets.US_ASCII);
        try (ZFile zf = new ZFile(zipFile)) {
            zf.add(fileName, new ByteArrayInputStream(fileContents));
        }

        fileTimestamp = zipFile.lastModified();

        /*
         * Wait to make sure the timestamp can increase.
         */
        while (true) {
            File notUsed = mTemporaryFolder.newFile();
            long notTimestamp = notUsed.lastModified();
            notUsed.delete();
            if (notTimestamp > fileTimestamp) {
                break;
            }
        }

        /*
         * Open the zip file, but do any changes. The need to updating the signature should force
         * a file update.
         */
        try (ZFile zf = new ZFile(zipFile)) {
            ManifestGenerationExtension me = new ManifestGenerationExtension("I", "Android");
            me.register(zf);
            new SigningExtension(21, p.v2, p.v1, true, false).register(zf);
        }

        /*
         * Check the file was touched.
         */
        assertNotEquals(fileTimestamp, zipFile.lastModified());
    }
}
