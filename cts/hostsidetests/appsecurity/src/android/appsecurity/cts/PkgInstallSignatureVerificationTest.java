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

package android.appsecurity.cts;

import android.platform.test.annotations.AppModeFull;
import android.platform.test.annotations.SecurityTest;

import com.android.compatibility.common.tradefed.build.CompatibilityBuildHelper;
import com.android.tradefed.build.IBuildInfo;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.testtype.DeviceTestCase;
import com.android.tradefed.testtype.IBuildReceiver;

import java.io.BufferedOutputStream;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.util.Locale;

/**
 * Tests for APK signature verification during installation.
 */
@AppModeFull // TODO: Needs porting to instant
public class PkgInstallSignatureVerificationTest extends DeviceTestCase implements IBuildReceiver {

    private static final String TEST_PKG = "android.appsecurity.cts.tinyapp";
    private static final String COMPANION_TEST_PKG = "android.appsecurity.cts.tinyapp_companion";
    private static final String DEVICE_TESTS_APK = "CtsV3SigningSchemeRotationTest.apk";
    private static final String DEVICE_TESTS_PKG = "android.appsecurity.cts.v3rotationtests";
    private static final String DEVICE_TESTS_CLASS = DEVICE_TESTS_PKG + ".V3RotationTest";
    private static final String TEST_APK_RESOURCE_PREFIX = "/pkgsigverify/";

    private static final String[] DSA_KEY_NAMES = {"1024", "2048", "3072"};
    private static final String[] EC_KEY_NAMES = {"p256", "p384", "p521"};
    private static final String[] RSA_KEY_NAMES = {"1024", "2048", "3072", "4096", "8192", "16384"};
    private static final String[] RSA_KEY_NAMES_2048_AND_LARGER =
            {"2048", "3072", "4096", "8192", "16384"};

    private IBuildInfo mCtsBuild;

    @Override
    public void setBuild(IBuildInfo buildInfo) {
        mCtsBuild = buildInfo;
    }

    @Override
    protected void setUp() throws Exception {
        super.setUp();

        Utils.prepareSingleUser(getDevice());
        assertNotNull(mCtsBuild);
        uninstallPackage();
        uninstallCompanionPackage();
        installDeviceTestPkg();
    }

    @Override
    protected void tearDown() throws Exception {
        try {
            uninstallPackages();
        } catch (DeviceNotAvailableException ignored) {
        } finally {
            super.tearDown();
        }
    }

    public void testInstallOriginalSucceeds() throws Exception {
        // APK signed with v1 and v2 schemes. Obtained by building
        // cts/hostsidetests/appsecurity/test-apps/tinyapp.
        assertInstallSucceeds("original.apk");
    }

    public void testInstallV1OneSignerMD5withRSA() throws Exception {
        // APK signed with v1 scheme only, one signer.
        assertInstallSucceedsForEach(
                "v1-only-with-rsa-pkcs1-md5-1.2.840.113549.1.1.1-%s.apk", RSA_KEY_NAMES);
        assertInstallSucceedsForEach(
                "v1-only-with-rsa-pkcs1-md5-1.2.840.113549.1.1.4-%s.apk", RSA_KEY_NAMES);
    }

    public void testInstallV1OneSignerSHA1withRSA() throws Exception {
        // APK signed with v1 scheme only, one signer.
        assertInstallSucceedsForEach(
                "v1-only-with-rsa-pkcs1-sha1-1.2.840.113549.1.1.1-%s.apk", RSA_KEY_NAMES);
        assertInstallSucceedsForEach(
                "v1-only-with-rsa-pkcs1-sha1-1.2.840.113549.1.1.5-%s.apk", RSA_KEY_NAMES);
    }

    public void testInstallV1OneSignerSHA224withRSA() throws Exception {
        // APK signed with v1 scheme only, one signer.
        assertInstallSucceedsForEach(
                "v1-only-with-rsa-pkcs1-sha224-1.2.840.113549.1.1.1-%s.apk", RSA_KEY_NAMES);
        assertInstallSucceedsForEach(
                "v1-only-with-rsa-pkcs1-sha224-1.2.840.113549.1.1.14-%s.apk", RSA_KEY_NAMES);
    }

    public void testInstallV1OneSignerSHA256withRSA() throws Exception {
        // APK signed with v1 scheme only, one signer.
        assertInstallSucceedsForEach(
                "v1-only-with-rsa-pkcs1-sha256-1.2.840.113549.1.1.1-%s.apk", RSA_KEY_NAMES);
        assertInstallSucceedsForEach(
                "v1-only-with-rsa-pkcs1-sha256-1.2.840.113549.1.1.11-%s.apk", RSA_KEY_NAMES);
    }

    public void testInstallV1OneSignerSHA384withRSA() throws Exception {
        // APK signed with v1 scheme only, one signer.
        assertInstallSucceedsForEach(
                "v1-only-with-rsa-pkcs1-sha384-1.2.840.113549.1.1.1-%s.apk", RSA_KEY_NAMES);
        assertInstallSucceedsForEach(
                "v1-only-with-rsa-pkcs1-sha384-1.2.840.113549.1.1.12-%s.apk", RSA_KEY_NAMES);
    }

    public void testInstallV1OneSignerSHA512withRSA() throws Exception {
        // APK signed with v1 scheme only, one signer.
        assertInstallSucceedsForEach(
                "v1-only-with-rsa-pkcs1-sha512-1.2.840.113549.1.1.1-%s.apk", RSA_KEY_NAMES);
        assertInstallSucceedsForEach(
                "v1-only-with-rsa-pkcs1-sha512-1.2.840.113549.1.1.13-%s.apk", RSA_KEY_NAMES);
    }

    public void testInstallV1OneSignerSHA1withECDSA() throws Exception {
        // APK signed with v1 scheme only, one signer.
        assertInstallSucceedsForEach(
                "v1-only-with-ecdsa-sha1-1.2.840.10045.2.1-%s.apk", EC_KEY_NAMES);
        assertInstallSucceedsForEach(
                "v1-only-with-ecdsa-sha1-1.2.840.10045.4.1-%s.apk", EC_KEY_NAMES);
    }

    public void testInstallV1OneSignerSHA224withECDSA() throws Exception {
        // APK signed with v1 scheme only, one signer.
        assertInstallSucceedsForEach(
                "v1-only-with-ecdsa-sha224-1.2.840.10045.2.1-%s.apk", EC_KEY_NAMES);
        assertInstallSucceedsForEach(
                "v1-only-with-ecdsa-sha224-1.2.840.10045.4.3.1-%s.apk", EC_KEY_NAMES);
    }

    public void testInstallV1OneSignerSHA256withECDSA() throws Exception {
        // APK signed with v1 scheme only, one signer.
        assertInstallSucceedsForEach(
                "v1-only-with-ecdsa-sha256-1.2.840.10045.2.1-%s.apk", EC_KEY_NAMES);
        assertInstallSucceedsForEach(
                "v1-only-with-ecdsa-sha256-1.2.840.10045.4.3.2-%s.apk", EC_KEY_NAMES);
    }

    public void testInstallV1OneSignerSHA384withECDSA() throws Exception {
        // APK signed with v1 scheme only, one signer.
        assertInstallSucceedsForEach(
                "v1-only-with-ecdsa-sha384-1.2.840.10045.2.1-%s.apk", EC_KEY_NAMES);
        assertInstallSucceedsForEach(
                "v1-only-with-ecdsa-sha384-1.2.840.10045.4.3.3-%s.apk", EC_KEY_NAMES);
    }

    public void testInstallV1OneSignerSHA512withECDSA() throws Exception {
        // APK signed with v1 scheme only, one signer.
        assertInstallSucceedsForEach(
                "v1-only-with-ecdsa-sha512-1.2.840.10045.2.1-%s.apk", EC_KEY_NAMES);
        assertInstallSucceedsForEach(
                "v1-only-with-ecdsa-sha512-1.2.840.10045.4.3.4-%s.apk", EC_KEY_NAMES);
    }

    public void testInstallV1OneSignerSHA1withDSA() throws Exception {
        // APK signed with v1 scheme only, one signer.
        assertInstallSucceedsForEach(
                "v1-only-with-dsa-sha1-1.2.840.10040.4.1-%s.apk", DSA_KEY_NAMES);
        assertInstallSucceedsForEach(
                "v1-only-with-dsa-sha1-1.2.840.10040.4.3-%s.apk", DSA_KEY_NAMES);
    }

    public void testInstallV1OneSignerSHA224withDSA() throws Exception {
        // APK signed with v1 scheme only, one signer.
        assertInstallSucceedsForEach(
                "v1-only-with-dsa-sha224-1.2.840.10040.4.1-%s.apk", DSA_KEY_NAMES);
        assertInstallSucceedsForEach(
                "v1-only-with-dsa-sha224-2.16.840.1.101.3.4.3.1-%s.apk", DSA_KEY_NAMES);
    }

    public void testInstallV1OneSignerSHA256withDSA() throws Exception {
        // APK signed with v1 scheme only, one signer.
        assertInstallSucceedsForEach(
                "v1-only-with-dsa-sha256-1.2.840.10040.4.1-%s.apk", DSA_KEY_NAMES);
        assertInstallSucceedsForEach(
                "v1-only-with-dsa-sha256-2.16.840.1.101.3.4.3.2-%s.apk", DSA_KEY_NAMES);
    }

//  Android platform doesn't support DSA with SHA-384 and SHA-512.
//    public void testInstallV1OneSignerSHA384withDSA() throws Exception {
//        // APK signed with v1 scheme only, one signer.
//        assertInstallSucceedsForEach(
//                "v1-only-with-dsa-sha384-2.16.840.1.101.3.4.3.3-%s.apk", DSA_KEY_NAMES);
//    }
//
//    public void testInstallV1OneSignerSHA512withDSA() throws Exception {
//        // APK signed with v1 scheme only, one signer.
//        assertInstallSucceedsForEach(
//                "v1-only-with-dsa-sha512-2.16.840.1.101.3.4.3.3-%s.apk", DSA_KEY_NAMES);
//    }

    public void testInstallV2StrippedFails() throws Exception {
        // APK signed with v1 and v2 schemes, but v2 signature was stripped from the file (by using
        // zipalign).
        // This should fail because the v1 signature indicates that the APK was supposed to be
        // signed with v2 scheme as well, making the platform's anti-stripping protections reject
        // the APK.
        assertInstallFailsWithError("v2-stripped.apk", "Signature stripped");

        // Similar to above, but the X-Android-APK-Signed anti-stripping header in v1 signature
        // lists unknown signature schemes in addition to APK Signature Scheme v2. Unknown schemes
        // should be ignored.
        assertInstallFailsWithError(
                "v2-stripped-with-ignorable-signing-schemes.apk", "Signature stripped");
    }

    public void testInstallV2OneSignerOneSignature() throws Exception {
        // APK signed with v2 scheme only, one signer, one signature.
        assertInstallSucceedsForEach("v2-only-with-dsa-sha256-%s.apk", DSA_KEY_NAMES);
        assertInstallSucceedsForEach("v2-only-with-ecdsa-sha256-%s.apk", EC_KEY_NAMES);
        assertInstallSucceedsForEach("v2-only-with-rsa-pkcs1-sha256-%s.apk", RSA_KEY_NAMES);
        assertInstallSucceedsForEach("v2-only-with-rsa-pss-sha256-%s.apk", RSA_KEY_NAMES);

        // DSA with SHA-512 is not supported by Android platform and thus APK Signature Scheme v2
        // does not support that either
        // assertInstallSucceedsForEach("v2-only-with-dsa-sha512-%s.apk", DSA_KEY_NAMES);
        assertInstallSucceedsForEach("v2-only-with-ecdsa-sha512-%s.apk", EC_KEY_NAMES);
        assertInstallSucceedsForEach("v2-only-with-rsa-pkcs1-sha512-%s.apk", RSA_KEY_NAMES);
        assertInstallSucceedsForEach(
                "v2-only-with-rsa-pss-sha512-%s.apk",
                RSA_KEY_NAMES_2048_AND_LARGER // 1024-bit key is too short for PSS with SHA-512
                );
    }

    public void testInstallV1SignatureOnlyDoesNotVerify() throws Exception {
        // APK signed with v1 scheme only, but not all digests match those recorded in
        // META-INF/MANIFEST.MF.
        String error = "META-INF/MANIFEST.MF has invalid digest";

        // Bitflip in classes.dex of otherwise good file.
        assertInstallFailsWithError(
                "v1-only-with-tampered-classes-dex.apk", error);
    }

    public void testInstallV2SignatureDoesNotVerify() throws Exception {
        // APK signed with v2 scheme only, but the signature over signed-data does not verify.
        String error = "signature did not verify";

        // Bitflip in certificate field inside signed-data. Based on
        // v2-only-with-dsa-sha256-1024.apk.
        assertInstallFailsWithError("v2-only-with-dsa-sha256-1024-sig-does-not-verify.apk", error);

        // Signature claims to be RSA PKCS#1 v1.5 with SHA-256, but is actually using SHA-512.
        // Based on v2-only-with-rsa-pkcs1-sha256-2048.apk.
        assertInstallFailsWithError(
                "v2-only-with-rsa-pkcs1-sha256-2048-sig-does-not-verify.apk", error);

        // Signature claims to be RSA PSS with SHA-256 and 32 bytes of salt, but is actually using 0
        // bytes of salt. Based on v2-only-with-rsa-pkcs1-sha256-2048.apk. Obtained by modifying APK
        // signer to use the wrong amount of salt.
        assertInstallFailsWithError(
                "v2-only-with-rsa-pss-sha256-2048-sig-does-not-verify.apk", error);

        // Bitflip in the ECDSA signature. Based on v2-only-with-ecdsa-sha256-p256.apk.
        assertInstallFailsWithError(
                "v2-only-with-ecdsa-sha256-p256-sig-does-not-verify.apk", error);
    }

    public void testInstallV2ContentDigestMismatch() throws Exception {
        // APK signed with v2 scheme only, but the digest of contents does not match the digest
        // stored in signed-data.
        String error = "digest of contents did not verify";

        // Based on v2-only-with-rsa-pkcs1-sha512-4096.apk. Obtained by modifying APK signer to
        // flip the leftmost bit in content digest before signing signed-data.
        assertInstallFailsWithError(
                "v2-only-with-rsa-pkcs1-sha512-4096-digest-mismatch.apk", error);

        // Based on v2-only-with-ecdsa-sha256-p256.apk. Obtained by modifying APK signer to flip the
        // leftmost bit in content digest before signing signed-data.
        assertInstallFailsWithError(
                "v2-only-with-ecdsa-sha256-p256-digest-mismatch.apk", error);
    }

    public void testInstallNoApkSignatureSchemeBlock() throws Exception {
        // APK signed with v2 scheme only, but the rules for verifying APK Signature Scheme v2
        // signatures say that this APK must not be verified using APK Signature Scheme v2.

        // Obtained from v2-only-with-rsa-pkcs1-sha512-4096.apk by flipping a bit in the magic
        // field in the footer of APK Signing Block. This makes the APK Signing Block disappear.
        assertInstallFails("v2-only-wrong-apk-sig-block-magic.apk");

        // Obtained by modifying APK signer to insert "GARBAGE" between ZIP Central Directory and
        // End of Central Directory. The APK is otherwise fine and is signed with APK Signature
        // Scheme v2. Based on v2-only-with-rsa-pkcs1-sha256.apk.
        assertInstallFails("v2-only-garbage-between-cd-and-eocd.apk");

        // Obtained by modifying APK signer to truncate the ZIP Central Directory by one byte. The
        // APK is otherwise fine and is signed with APK Signature Scheme v2. Based on
        // v2-only-with-rsa-pkcs1-sha256.apk
        assertInstallFails("v2-only-truncated-cd.apk");

        // Obtained by modifying the size in APK Signature Block header. Based on
        // v2-only-with-ecdsa-sha512-p521.apk.
        assertInstallFails("v2-only-apk-sig-block-size-mismatch.apk");

        // Obtained by modifying the ID under which APK Signature Scheme v2 Block is stored in
        // APK Signing Block and by modifying the APK signer to not insert anti-stripping
        // protections into JAR Signature. The APK should appear as having no APK Signature Scheme
        // v2 Block and should thus successfully verify using JAR Signature Scheme.
        assertInstallSucceeds("v1-with-apk-sig-block-but-without-apk-sig-scheme-v2-block.apk");
    }

    public void testInstallV2UnknownPairIgnoredInApkSigningBlock() throws Exception {
        // Obtained by modifying APK signer to emit an unknown ID-value pair into APK Signing Block
        // before the ID-value pair containing the APK Signature Scheme v2 Block. The unknown
        // ID-value should be ignored.
        assertInstallSucceeds("v2-only-unknown-pair-in-apk-sig-block.apk");
    }

    public void testInstallV2IgnoresUnknownSignatureAlgorithms() throws Exception {
        // APK is signed with a known signature algorithm and with a couple of unknown ones.
        // Obtained by modifying APK signer to use "unknown" signature algorithms in addition to
        // known ones.
        assertInstallSucceeds("v2-only-with-ignorable-unsupported-sig-algs.apk");
    }

    public void testInstallV2RejectsMismatchBetweenSignaturesAndDigestsBlocks() throws Exception {
        // APK is signed with a single signature algorithm, but the digests block claims that it is
        // signed with two different signature algorithms. Obtained by modifying APK Signer to
        // emit an additional digest record with signature algorithm 0x12345678.
        assertInstallFailsWithError(
                "v2-only-signatures-and-digests-block-mismatch.apk",
                "Signature algorithms don't match between digests and signatures records");
    }

    public void testInstallV2RejectsMismatchBetweenPublicKeyAndCertificate() throws Exception {
        // APK is signed with v2 only. The public key field does not match the public key in the
        // leaf certificate. Obtained by modifying APK signer to write out a modified leaf
        // certificate where the RSA modulus has a bitflip.
        assertInstallFailsWithError(
                "v2-only-cert-and-public-key-mismatch.apk",
                "Public key mismatch between certificate and signature record");
    }

    public void testInstallV2RejectsSignerBlockWithNoCertificates() throws Exception {
        // APK is signed with v2 only. There are no certificates listed in the signer block.
        // Obtained by modifying APK signer to output no certificates.
        assertInstallFailsWithError("v2-only-no-certs-in-sig.apk", "No certificates listed");
    }

    public void testInstallTwoSigners() throws Exception {
        // APK signed by two different signers.
        assertInstallSucceeds("two-signers.apk");
        // Because the install attempt below is an update, it also tests that the signing
        // certificates exposed by v2 signatures above are the same as the one exposed by v1
        // signatures in this APK.
        assertInstallSucceeds("v1-only-two-signers.apk");
        assertInstallSucceeds("v2-only-two-signers.apk");
    }

    public void testInstallV2TwoSignersRejectsWhenOneBroken() throws Exception {
        // Bitflip in the ECDSA signature of second signer. Based on two-signers.apk.
        // This asserts that breakage in any signer leads to rejection of the APK.
        assertInstallFailsWithError(
                "two-signers-second-signer-v2-broken.apk", "signature did not verify");
    }

    public void testInstallV2TwoSignersRejectsWhenOneWithoutSignatures() throws Exception {
        // APK v2-signed by two different signers. However, there are no signatures for the second
        // signer.
        assertInstallFailsWithError(
                "v2-only-two-signers-second-signer-no-sig.apk", "No signatures");
    }

    public void testInstallV2TwoSignersRejectsWhenOneWithoutSupportedSignatures() throws Exception {
        // APK v2-signed by two different signers. However, there are no supported signatures for
        // the second signer.
        assertInstallFailsWithError(
                "v2-only-two-signers-second-signer-no-supported-sig.apk",
                "No supported signatures");
    }

    public void testInstallV2RejectsWhenMissingCode() throws Exception {
        // Obtained by removing classes.dex from original.apk and then signing with v2 only.
        // Although this has nothing to do with v2 signature verification, package manager wants
        // signature verification / certificate collection to reject APKs with missing code
        // (classes.dex) unless requested otherwise.
        assertInstallFailsWithError("v2-only-missing-classes.dex.apk", "code is missing");
    }

    public void testCorrectCertUsedFromPkcs7SignedDataCertsSet() throws Exception {
        // Obtained by prepending the rsa-1024 certificate to the PKCS#7 SignedData certificates set
        // of v1-only-with-rsa-pkcs1-sha1-1.2.840.113549.1.1.1-2048.apk META-INF/CERT.RSA. The certs
        // (in the order of appearance in the file) are thus: rsa-1024, rsa-2048. The package's
        // signing cert is rsa-2048.
        assertInstallSucceeds("v1-only-pkcs7-cert-bag-first-cert-not-used.apk");

        // Check that rsa-1024 was not used as the previously installed package's signing cert.
        assertInstallFailsWithError(
                "v1-only-with-rsa-pkcs1-sha1-1.2.840.113549.1.1.1-1024.apk",
                "signatures do not match");

        // Check that rsa-2048 was used as the previously installed package's signing cert.
        assertInstallSucceeds("v1-only-with-rsa-pkcs1-sha1-1.2.840.113549.1.1.1-2048.apk");
    }

    public void testV1SchemeSignatureCertNotReencoded() throws Exception {
        // Regression test for b/30148997 and b/18228011. When PackageManager does not preserve the
        // original encoded form of signing certificates, bad things happen, such as rejection of
        // completely valid updates to apps. The issue in b/30148997 and b/18228011 was that
        // PackageManager started re-encoding signing certs into DER. This normally produces exactly
        // the original form because X.509 certificates are supposed to be DER-encoded. However, a
        // small fraction of Android apps uses X.509 certificates which are not DER-encoded. For
        // such apps, re-encoding into DER changes the serialized form of the certificate, creating
        // a mismatch with the serialized form stored in the PackageManager database, leading to the
        // rejection of updates for the app.
        //
        // The signing certs of the two APKs differ only in how the cert's signature is encoded.
        // From Android's perspective, these two APKs are signed by different entities and thus
        // cannot be used to update one another. If signature verification code re-encodes certs
        // into DER, both certs will be exactly the same and Android will accept these APKs as
        // updates of each other. This test is thus asserting that the two APKs are not accepted as
        // updates of each other.
        //
        // * v1-only-with-rsa-1024.apk cert's signature is DER-encoded
        // * v1-only-with-rsa-1024-cert-not-der.apk cert's signature is not DER-encoded. It is
        //   BER-encoded, with length encoded as two bytes instead of just one.
        //   v1-only-with-rsa-1024-cert-not-der.apk META-INF/CERT.RSA was obtained from
        //   v1-only-with-rsa-1024.apk META-INF/CERT.RSA by manually modifying the ASN.1 structure.
        assertInstallSucceeds("v1-only-with-rsa-1024.apk");
        assertInstallFailsWithError(
                "v1-only-with-rsa-1024-cert-not-der.apk", "signatures do not match");

        uninstallPackage();
        assertInstallSucceeds("v1-only-with-rsa-1024-cert-not-der.apk");
        assertInstallFailsWithError("v1-only-with-rsa-1024.apk", "signatures do not match");
    }

    public void testV2SchemeSignatureCertNotReencoded() throws Exception {
        // This test is here to catch something like b/30148997 and b/18228011 happening to the
        // handling of APK Signature Scheme v2 signatures by PackageManager. When PackageManager
        // does not preserve the original encoded form of signing certificates, bad things happen,
        // such as rejection of completely valid updates to apps. The issue in b/30148997 and
        // b/18228011 was that PackageManager started re-encoding signing certs into DER. This
        // normally produces exactly the original form because X.509 certificates are supposed to be
        // DER-encoded. However, a small fraction of Android apps uses X.509 certificates which are
        // not DER-encoded. For such apps, re-encoding into DER changes the serialized form of the
        // certificate, creating a mismatch with the serialized form stored in the PackageManager
        // database, leading to the rejection of updates for the app.
        //
        // The signing certs of the two APKs differ only in how the cert's signature is encoded.
        // From Android's perspective, these two APKs are signed by different entities and thus
        // cannot be used to update one another. If signature verification code re-encodes certs
        // into DER, both certs will be exactly the same and Android will accept these APKs as
        // updates of each other. This test is thus asserting that the two APKs are not accepted as
        // updates of each other.
        //
        // * v2-only-with-rsa-pkcs1-sha256-1024.apk cert's signature is DER-encoded
        // * v2-only-with-rsa-pkcs1-sha256-1024-cert-not-der.apk cert's signature is not DER-encoded
        //   It is BER-encoded, with length encoded as two bytes instead of just one.
        assertInstallSucceeds("v2-only-with-rsa-pkcs1-sha256-1024.apk");
        assertInstallFailsWithError(
                "v2-only-with-rsa-pkcs1-sha256-1024-cert-not-der.apk", "signatures do not match");

        uninstallPackage();
        assertInstallSucceeds("v2-only-with-rsa-pkcs1-sha256-1024-cert-not-der.apk");
        assertInstallFailsWithError(
                "v2-only-with-rsa-pkcs1-sha256-1024.apk", "signatures do not match");
    }

    public void testInstallMaxSizedZipEocdComment() throws Exception {
        // Obtained by modifying apksigner to produce a max-sized (0xffff bytes long) ZIP End of
        // Central Directory comment, and signing the original.apk using the modified apksigner.
        assertInstallSucceeds("v1-only-max-sized-eocd-comment.apk");
        assertInstallSucceeds("v2-only-max-sized-eocd-comment.apk");
    }

    public void testInstallEphemeralRequiresV2Signature() throws Exception {
        assertInstallEphemeralFailsWithError("unsigned-ephemeral.apk",
                "Failed to collect certificates");
        assertInstallEphemeralFailsWithError("v1-only-ephemeral.apk",
                "must be signed with APK Signature Scheme v2 or greater");
        assertInstallEphemeralSucceeds("v2-only-ephemeral.apk");
        assertInstallEphemeralSucceeds("v1-v2-ephemeral.apk"); // signed with both schemes
    }

    public void testInstallEmpty() throws Exception {
        assertInstallFailsWithError("empty-unsigned.apk", "Unknown failure");
        assertInstallFailsWithError("v1-only-empty.apk", "Unknown failure");
        assertInstallFailsWithError("v2-only-empty.apk", "Unknown failure");
    }

    @SecurityTest
    public void testInstallApkWhichDoesNotStartWithZipLocalFileHeaderMagic() throws Exception {
        // The APKs below are competely fine except they don't start with ZIP Local File Header
        // magic. Thus, these APKs will install just fine unless Package Manager requires that APKs
        // start with ZIP Local File Header magic.
        String error = "Unknown failure";

        // Obtained by modifying apksigner to output four unused 0x00 bytes at the start of the APK
        assertInstallFailsWithError("v1-only-starts-with-00000000-magic.apk", error);
        assertInstallFailsWithError("v2-only-starts-with-00000000-magic.apk", error);

        // Obtained by modifying apksigner to output 8 unused bytes (DEX magic and version) at the
        // start of the APK
        assertInstallFailsWithError("v1-only-starts-with-dex-magic.apk", error);
        assertInstallFailsWithError("v2-only-starts-with-dex-magic.apk", error);
    }

    public void testInstallV3KeyRotation() throws Exception {
        // tests that a v3 signed APK with RSA key can rotate to a new key
        assertInstallSucceeds("v3-rsa-pkcs1-sha256-2048-1.apk");
        assertInstallSucceeds("v3-rsa-pkcs1-sha256-2048-2-with-por_1_2-full-caps.apk");
    }

    public void testInstallV3KeyRotationToAncestor() throws Exception {
        // tests that a v3 signed APK with RSA key cannot be upgraded by one of its past certs
        assertInstallSucceeds("v3-rsa-pkcs1-sha256-2048-2-with-por_1_2-full-caps.apk");
        assertInstallFails("v3-rsa-pkcs1-sha256-2048-1.apk");
    }

    public void testInstallV3KeyRotationToAncestorWithRollback() throws Exception {
        // tests that a v3 signed APK with RSA key can be upgraded by one of its past certs if it
        // has granted that cert the rollback capability
        assertInstallSucceeds("v3-rsa-pkcs1-sha256-2048-2-with-por_1_2-full-and-roll-caps.apk");
        assertInstallSucceeds("v3-rsa-pkcs1-sha256-2048-1.apk");
    }

    public void testInstallV3KeyRotationMultipleHops() throws Exception {
        // tests that a v3 signed APK with RSA key can rotate to a new key which is the result of
        // multiple rotations from the original: APK signed with key 1 can be updated by key 3, when
        // keys were: 1 -> 2 -> 3
        assertInstallSucceeds("v3-rsa-pkcs1-sha256-2048-1.apk");
        assertInstallSucceeds("v3-rsa-pkcs1-sha256-2048-3-with-por_1_2_3-full-caps.apk");
    }

    public void testInstallV3PorSignerMismatch() throws Exception {
        // tests that an APK with a proof-of-rotation struct that doesn't include the current
        // signing certificate fails to install
        assertInstallFails("v3-rsa-pkcs1-sha256-2048-3-with-por_1_2-full-caps.apk");
    }

    public void testInstallV3KeyRotationWrongPor() throws Exception {
        // tests that a valid APK with a proof-of-rotation record can't upgrade an APK with a
        // signing certificate that isn't in the proof-of-rotation record
        assertInstallSucceeds("v3-rsa-pkcs1-sha256-2048-1.apk");
        assertInstallFails("v3-rsa-pkcs1-sha256-2048-3-with-por_2_3-full-caps.apk");
    }

    public void testInstallV3KeyRotationSharedUid() throws Exception {
        // tests that a v3 signed sharedUid APK can still be sharedUid with apps with its older
        // signing certificate, if it so desires
        assertInstallSucceeds("v3-rsa-pkcs1-sha256-2048-1-sharedUid.apk");
        assertInstallSucceeds(
                "v3-rsa-pkcs1-sha256-2048-2-with-por_1_2-full-caps-sharedUid-companion.apk");
    }

    public void testInstallV3KeyRotationOlderSharedUid() throws Exception {

        // tests that a sharedUid APK can still install with another app that is signed by a newer
        // signing certificate, but which allows sharedUid with the older one
        assertInstallSucceeds(
                "v3-rsa-pkcs1-sha256-2048-2-with-por_1_2-full-caps-sharedUid-companion.apk");
        assertInstallSucceeds("v3-rsa-pkcs1-sha256-2048-1-sharedUid.apk");
    }

    public void testInstallV3KeyRotationSharedUidNoCap() throws Exception {
        // tests that a v3 signed sharedUid APK cannot be sharedUid with apps with its older
        // signing certificate, when it has not granted that certificate the sharedUid capability
        assertInstallSucceeds("v3-rsa-pkcs1-sha256-2048-1-sharedUid.apk");
        assertInstallFails(
                "v3-rsa-pkcs1-sha256-2048-2-with-por_1_2-no-shUid-cap-sharedUid-companion.apk");
    }

    public void testInstallV3KeyRotationOlderSharedUidNoCap() throws Exception {
        // tests that a sharedUid APK signed with an old certificate cannot install with
        // an app having a proof-of-rotation structure that hasn't granted the older
        // certificate the sharedUid capability
        assertInstallSucceeds(
                "v3-rsa-pkcs1-sha256-2048-2-with-por_1_2-no-shUid-cap-sharedUid-companion.apk");
        assertInstallFails("v3-rsa-pkcs1-sha256-2048-1-sharedUid.apk");
    }

    public void testInstallV3NoRotationSharedUid() throws Exception {
        // tests that a sharedUid APK signed with a new certificate installs with
        // an app having a proof-of-rotation structure that hasn't granted an older
        // certificate the sharedUid capability
        assertInstallSucceeds(
                "v3-rsa-pkcs1-sha256-2048-2-with-por_1_2-no-shUid-cap-sharedUid-companion.apk");
        assertInstallSucceeds("v3-rsa-pkcs1-sha256-2048-2-sharedUid.apk");
    }

    public void testInstallV3KeyRotationSigPerm() throws Exception {
        // tests that a v3 signed APK can still get a signature permission from an app with its
        // older signing certificate.
        assertInstallSucceeds("v3-rsa-pkcs1-sha256-2048-1-permdef.apk");
        assertInstallSucceeds(
                "v3-rsa-pkcs1-sha256-2048-2-with-por_1_2-full-caps-permcli-companion.apk");
        Utils.runDeviceTests(getDevice(), DEVICE_TESTS_PKG, DEVICE_TESTS_CLASS, "testHasPerm");
    }

    public void testInstallV3KeyRotationOlderSigPerm() throws Exception {
        // tests that an apk with an older signing certificate than the one which defines a
        // signature permission it wants gets the permission if the defining APK grants the
        // capability
        assertInstallSucceeds(
                "v3-rsa-pkcs1-sha256-2048-2-with-por_1_2-full-caps-permdef.apk");
        assertInstallSucceeds("v3-rsa-pkcs1-sha256-2048-1-permcli-companion.apk");
        Utils.runDeviceTests(getDevice(), DEVICE_TESTS_PKG, DEVICE_TESTS_CLASS, "testHasPerm");
    }

    public void testInstallV3KeyRotationSigPermNoCap() throws Exception {
        // tests that an APK signed by an older signing certificate is unable to get a requested
        // signature permission when the defining APK has rotated to a newer signing certificiate
        // and does not grant the permission capability to the older cert
        assertInstallSucceeds("v3-rsa-pkcs1-sha256-2048-2-with-por_1_2-no-perm-cap-permdef.apk");
        assertInstallSucceeds("v3-rsa-pkcs1-sha256-2048-1-permcli-companion.apk");
        Utils.runDeviceTests(getDevice(), DEVICE_TESTS_PKG, DEVICE_TESTS_CLASS, "testHasNoPerm");
    }

    public void testInstallV3KeyRotationOlderSigPermNoCap() throws Exception {
        // tests that an APK signed by a newer signing certificate than the APK which defines a
        // signature permission is able to get that permission, even if the newer APK does not
        // grant the permission capability to the older signing certificate.
        assertInstallSucceeds("v3-rsa-pkcs1-sha256-2048-1-permdef.apk");
        assertInstallSucceeds(
                "v3-rsa-pkcs1-sha256-2048-2-with-por_1_2-no-perm-cap-permcli-companion.apk");
        Utils.runDeviceTests(getDevice(), DEVICE_TESTS_PKG, DEVICE_TESTS_CLASS, "testHasPerm");
    }

    public void testInstallV3NoRotationSigPerm() throws Exception {
        // make sure that an APK, which wants to use a signature permission defined by an APK, which
        // has not granted that capability to older signing certificates, can still install
        assertInstallSucceeds("v3-rsa-pkcs1-sha256-2048-2-with-por_1_2-no-perm-cap-permdef.apk");
        assertInstallSucceeds("v3-rsa-pkcs1-sha256-2048-2-permcli-companion.apk");
        Utils.runDeviceTests(getDevice(), DEVICE_TESTS_PKG, DEVICE_TESTS_CLASS, "testHasPerm");
    }

    public void testInstallV3SigPermDoubleDefNewerSucceeds() throws Exception {
        // make sure that if an app defines a signature permission already defined by another app,
        // it successfully installs if the other app's signing cert is in its past signing certs and
        // the signature permission capability is granted
        assertInstallSucceeds("v3-rsa-pkcs1-sha256-2048-1-permdef.apk");
        assertInstallSucceeds("v3-rsa-pkcs1-sha256-2048-2-with_por_1_2-permdef-companion.apk");
    }

    public void testInstallV3SigPermDoubleDefOlderSucceeds() throws Exception {
        // make sure that if an app defines a signature permission already defined by another app,
        // it successfully installs if it is in the other app's past signing certs and the signature
        // permission capability is granted
        assertInstallSucceeds("v3-rsa-pkcs1-sha256-2048-2-with_por_1_2-permdef-companion.apk");
        assertInstallSucceeds("v3-rsa-pkcs1-sha256-2048-1-permdef.apk");
    }

    public void testInstallV3SigPermDoubleDefNewerNoCapFails() throws Exception {
        // make sure that if an app defines a signature permission already defined by another app,
        // it fails to install if the other app's signing cert is in its past signing certs but the
        // signature permission capability is not granted
        assertInstallSucceeds("v3-rsa-pkcs1-sha256-2048-1-permdef.apk");
        assertInstallFails(
                "v3-rsa-pkcs1-sha256-2048-2-with_por_1_2-no-perm-cap-permdef-companion.apk");
    }

    public void testInstallV3SigPermDoubleDefOlderNoCapFails() throws Exception {
        // make sure that if an app defines a signature permission already defined by another app,
        // it fails to install if it is in the other app's past signing certs but the signature
        // permission capability is not granted
        assertInstallSucceeds(
                "v3-rsa-pkcs1-sha256-2048-2-with_por_1_2-no-perm-cap-permdef-companion.apk");
        assertInstallFails("v3-rsa-pkcs1-sha256-2048-1-permdef.apk");
    }

    public void testInstallV3SigPermDoubleDefSameNoCapSucceeds() throws Exception {
        // make sure that if an app defines a signature permission already defined by another app,
        // it installs successfully when signed by the same certificate, even if the original app
        // does not grant signature capabilities to its past certs
        assertInstallSucceeds(
                "v3-rsa-pkcs1-sha256-2048-2-with_por_1_2-no-perm-cap-permdef-companion.apk");
        assertInstallSucceeds("v3-rsa-pkcs1-sha256-2048-2-permdef.apk");
    }

    public void testInstallV3KeyRotationGetSignatures() throws Exception {
        // tests that a PackageInfo w/GET_SIGNATURES flag returns the older cert
        assertInstallSucceeds("v3-rsa-pkcs1-sha256-2048-2-with-por_1_2-full-caps.apk");
        Utils.runDeviceTests(
                getDevice(), DEVICE_TESTS_PKG, DEVICE_TESTS_CLASS, "testGetSignaturesShowsOld");
    }

    public void testInstallV3KeyRotationGetSigningCertificates() throws Exception {
        // tests that a PackageInfo w/GET_SIGNING_CERTIFICATES flag returns the old and new certs
        assertInstallSucceeds("v3-rsa-pkcs1-sha256-2048-2-with-por_1_2-full-caps.apk");
        Utils.runDeviceTests(
                getDevice(), DEVICE_TESTS_PKG, DEVICE_TESTS_CLASS,
                "testGetSigningCertificatesShowsAll");
    }

    public void testInstallV3KeyRotationHasSigningCertificate() throws Exception {
        // tests that hasSigningCertificate() recognizes past and current signing certs
        assertInstallSucceeds("v3-rsa-pkcs1-sha256-2048-2-with-por_1_2-full-caps.apk");
        Utils.runDeviceTests(
                getDevice(), DEVICE_TESTS_PKG, DEVICE_TESTS_CLASS,
                "testHasSigningCertificate");
    }

    public void testInstallV3KeyRotationHasSigningCertificateSha256() throws Exception {
        // tests that hasSigningCertificate() recognizes past and current signing certs by sha256
        assertInstallSucceeds("v3-rsa-pkcs1-sha256-2048-2-with-por_1_2-full-caps.apk");
        Utils.runDeviceTests(
                getDevice(), DEVICE_TESTS_PKG, DEVICE_TESTS_CLASS,
                "testHasSigningCertificateSha256");
    }

    public void testInstallV3KeyRotationHasSigningCertificateByUid() throws Exception {
        // tests that hasSigningCertificate() recognizes past and current signing certs by uid
        assertInstallSucceeds("v3-rsa-pkcs1-sha256-2048-2-with-por_1_2-full-caps.apk");
        Utils.runDeviceTests(
                getDevice(), DEVICE_TESTS_PKG, DEVICE_TESTS_CLASS,
                "testHasSigningCertificateByUid");
    }

    public void testInstallV3KeyRotationHasSigningCertificateByUidSha256() throws Exception {
        // tests that hasSigningCertificate() recognizes past and current signing certs by uid
        // and sha256
        assertInstallSucceeds("v3-rsa-pkcs1-sha256-2048-2-with-por_1_2-full-caps.apk");
        Utils.runDeviceTests(
                getDevice(), DEVICE_TESTS_PKG, DEVICE_TESTS_CLASS,
                "testHasSigningCertificateByUidSha256");
    }

    public void testInstallV3KeyRotationHasDuplicateSigningCertificateHistory() throws Exception {
        // tests that an app's proof-of-rotation signing history cannot contain the same certificate
        // more than once.
        assertInstallFails("v3-rsa-pkcs1-sha256-2048-2-with-por_1_2_2-full-caps.apk");
    }

    public void testInstallV3HasMultipleSigners() throws Exception {
        // tests that an app can't be signed by multiple signers when using v3 signature scheme
        assertInstallFails("v3-rsa-pkcs1-sha256-2048-1_and_2.apk");
    }

    public void testInstallV3HasMultiplePlatformSigners() throws Exception {
        // tests that an app can be signed by multiple v3 signers if they target different platform
        // versions
        assertInstallSucceeds("v3-rsa-pkcs1-sha256-2048-1_P_and_2_Qplus.apk");
    }

    private void assertInstallSucceeds(String apkFilenameInResources) throws Exception {
        String installResult = installPackageFromResource(apkFilenameInResources);
        if (installResult != null) {
            fail("Failed to install " + apkFilenameInResources + ": " + installResult);
        }
    }

    private void assertInstallEphemeralSucceeds(String apkFilenameInResources) throws Exception {
        String installResult = installEphemeralPackageFromResource(apkFilenameInResources);
        if (installResult != null) {
            fail("Failed to install " + apkFilenameInResources + ": " + installResult);
        }
    }

    private void assertInstallSucceedsForEach(
            String apkFilenamePatternInResources, String[] args) throws Exception {
        for (String arg : args) {
            String apkFilenameInResources =
                    String.format(Locale.US, apkFilenamePatternInResources, arg);
            String installResult = installPackageFromResource(apkFilenameInResources);
            if (installResult != null) {
                fail("Failed to install " + apkFilenameInResources + ": " + installResult);
            }
            try {
                uninstallPackage();
            } catch (Exception e) {
                throw new RuntimeException(
                        "Failed to uninstall after installing " + apkFilenameInResources, e);
            }
        }
    }

    private void assertInstallFailsWithError(
            String apkFilenameInResources, String errorSubstring) throws Exception {
        String installResult = installPackageFromResource(apkFilenameInResources);
        if (installResult == null) {
            fail("Install of " + apkFilenameInResources + " succeeded but was expected to fail"
                    + " with \"" + errorSubstring + "\"");
        }
        assertContains(
                "Install failure message of " + apkFilenameInResources,
                errorSubstring,
                installResult);
    }

    private void assertInstallEphemeralFailsWithError(
            String apkFilenameInResources, String errorSubstring) throws Exception {
        String installResult = installEphemeralPackageFromResource(apkFilenameInResources);
        if (installResult == null) {
            fail("Install of " + apkFilenameInResources + " succeeded but was expected to fail"
                    + " with \"" + errorSubstring + "\"");
        }
        assertContains(
                "Install failure message of " + apkFilenameInResources,
                errorSubstring,
                installResult);
    }

    private void assertInstallFails(String apkFilenameInResources) throws Exception {
        String installResult = installPackageFromResource(apkFilenameInResources);
        if (installResult == null) {
            fail("Install of " + apkFilenameInResources + " succeeded but was expected to fail");
        }
    }

    private static void assertContains(String message, String expectedSubstring, String actual) {
        String errorPrefix = ((message != null) && (message.length() > 0)) ? (message + ": ") : "";
        if (actual == null) {
            fail(errorPrefix + "Expected to contain \"" + expectedSubstring + "\", but was null");
        }
        if (!actual.contains(expectedSubstring)) {
            fail(errorPrefix + "Expected to contain \"" + expectedSubstring + "\", but was \""
                    + actual + "\"");
        }
    }

    private void installDeviceTestPkg() throws Exception {
        CompatibilityBuildHelper buildHelper = new CompatibilityBuildHelper(mCtsBuild);
        File apk = buildHelper.getTestFile(DEVICE_TESTS_APK);
        String result = getDevice().installPackage(apk, true);
        assertNull("failed to install " + DEVICE_TESTS_APK + ", Reason: " + result, result);
    }

    private String installPackageFromResource(String apkFilenameInResources, boolean ephemeral)
            throws IOException, DeviceNotAvailableException {
        // ITestDevice.installPackage API requires the APK to be install to be a File. We thus
        // copy the requested resource into a temporary file, attempt to install it, and delete the
        // file during cleanup.

        String fullResourceName = TEST_APK_RESOURCE_PREFIX + apkFilenameInResources;
        File apkFile = File.createTempFile("pkginstalltest", ".apk");
        try {
            try (InputStream in = getClass().getResourceAsStream(fullResourceName);
                    OutputStream out = new BufferedOutputStream(new FileOutputStream(apkFile))) {
                if (in == null) {
                    throw new IllegalArgumentException("Resource not found: " + fullResourceName);
                }
                byte[] buf = new byte[65536];
                int chunkSize;
                while ((chunkSize = in.read(buf)) != -1) {
                    out.write(buf, 0, chunkSize);
                }
            }
            if (ephemeral) {
                return getDevice().installPackage(apkFile, true, "--ephemeral");
            } else {
                return getDevice().installPackage(apkFile, true);
            }
        } finally {
            apkFile.delete();
        }
    }

    private String installPackageFromResource(String apkFilenameInResources)
            throws IOException, DeviceNotAvailableException {
        return installPackageFromResource(apkFilenameInResources, false);
    }

    private String installEphemeralPackageFromResource(String apkFilenameInResources)
            throws IOException, DeviceNotAvailableException {
        return installPackageFromResource(apkFilenameInResources, true);
    }

    private String uninstallPackage() throws DeviceNotAvailableException {
        return getDevice().uninstallPackage(TEST_PKG);
    }

    private String uninstallCompanionPackage() throws DeviceNotAvailableException {
        return getDevice().uninstallPackage(COMPANION_TEST_PKG);
    }

    private String uninstallDeviceTestPackage() throws DeviceNotAvailableException {
        return getDevice().uninstallPackage(DEVICE_TESTS_PKG);
    }

    private void uninstallPackages() throws DeviceNotAvailableException {
        uninstallPackage();
        uninstallCompanionPackage();
        uninstallDeviceTestPackage();
    }
}
