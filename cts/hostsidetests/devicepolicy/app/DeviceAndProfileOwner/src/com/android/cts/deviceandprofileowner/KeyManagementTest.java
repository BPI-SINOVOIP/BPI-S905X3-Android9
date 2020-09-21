/*
 * Copyright (C) 2014 The Android Open Source Project
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
package com.android.cts.deviceandprofileowner;

import static android.app.admin.DevicePolicyManager.ID_TYPE_BASE_INFO;
import static android.app.admin.DevicePolicyManager.ID_TYPE_IMEI;
import static android.app.admin.DevicePolicyManager.ID_TYPE_MEID;
import static android.app.admin.DevicePolicyManager.ID_TYPE_SERIAL;
import static android.keystore.cts.CertificateUtils.createCertificate;

import android.content.ComponentName;
import android.content.Context;
import android.content.res.AssetManager;
import android.keystore.cts.Attestation;
import android.keystore.cts.AuthorizationList;
import android.net.Uri;
import android.os.Build;
import android.security.AttestedKeyPair;
import android.security.KeyChain;
import android.security.KeyChainAliasCallback;
import android.security.KeyChainException;
import android.security.keystore.KeyGenParameterSpec;
import android.security.keystore.KeyProperties;
import android.support.test.uiautomator.UiDevice;
import android.telephony.TelephonyManager;

import com.android.compatibility.common.util.FakeKeys.FAKE_RSA_1;

import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.UnsupportedEncodingException;
import java.net.URLEncoder;
import java.security.GeneralSecurityException;
import java.security.KeyFactory;
import java.security.KeyPair;
import java.security.NoSuchAlgorithmException;
import java.security.PrivateKey;
import java.security.PublicKey;
import java.security.Signature;
import java.security.cert.Certificate;
import java.security.cert.CertificateException;
import java.security.cert.CertificateFactory;
import java.security.cert.CertificateParsingException;
import java.security.cert.X509Certificate;
import java.security.spec.InvalidKeySpecException;
import java.security.spec.PKCS8EncodedKeySpec;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collection;
import java.util.List;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;

import javax.security.auth.x500.X500Principal;

public class KeyManagementTest extends BaseDeviceAdminTest {
    private static final long KEYCHAIN_TIMEOUT_MINS = 6;

    private static class SupportedKeyAlgorithm {
        public final String keyAlgorithm;
        public final String signatureAlgorithm;
        public final String[] signaturePaddingSchemes;

        public SupportedKeyAlgorithm(
                String keyAlgorithm, String signatureAlgorithm,
                String[] signaturePaddingSchemes) {
            this.keyAlgorithm = keyAlgorithm;
            this.signatureAlgorithm = signatureAlgorithm;
            this.signaturePaddingSchemes = signaturePaddingSchemes;
        }
    }

    private final SupportedKeyAlgorithm[] SUPPORTED_KEY_ALGORITHMS = new SupportedKeyAlgorithm[] {
        new SupportedKeyAlgorithm(KeyProperties.KEY_ALGORITHM_RSA, "SHA256withRSA",
                new String[] {KeyProperties.SIGNATURE_PADDING_RSA_PSS,
                    KeyProperties.SIGNATURE_PADDING_RSA_PKCS1}),
        new SupportedKeyAlgorithm(KeyProperties.KEY_ALGORITHM_EC, "SHA256withECDSA", null)
    };

    private KeyManagementActivity mActivity;

    @Override
    public void setUp() throws Exception {
        super.setUp();
        final UiDevice device = UiDevice.getInstance(getInstrumentation());
        mActivity = launchActivity(getInstrumentation().getTargetContext().getPackageName(),
                KeyManagementActivity.class, null);
        device.waitForIdle();
    }

    @Override
    public void tearDown() throws Exception {
        mActivity.finish();
        super.tearDown();
    }

    public void testCanInstallAndRemoveValidRsaKeypair() throws Exception {
        final String alias = "com.android.test.valid-rsa-key-1";
        final PrivateKey privKey = getPrivateKey(FAKE_RSA_1.privateKey , "RSA");
        final Certificate cert = getCertificate(FAKE_RSA_1.caCertificate);

        // Install keypair.
        assertTrue(mDevicePolicyManager.installKeyPair(getWho(), privKey, cert, alias));
        try {
            // Request and retrieve using the alias.
            assertGranted(alias, false);
            assertEquals(alias, new KeyChainAliasFuture(alias).get());
            assertGranted(alias, true);

            // Verify key is at least something like the one we put in.
            assertEquals(KeyChain.getPrivateKey(mActivity, alias).getAlgorithm(), "RSA");
        } finally {
            // Delete regardless of whether the test succeeded.
            assertTrue(mDevicePolicyManager.removeKeyPair(getWho(), alias));
        }
        // Verify alias is actually deleted.
        assertGranted(alias, false);
    }

    public void testCanInstallWithAutomaticAccess() throws Exception {
        final String grant = "com.android.test.autogrant-key-1";
        final String withhold = "com.android.test.nongrant-key-1";
        final PrivateKey privKey = getPrivateKey(FAKE_RSA_1.privateKey , "RSA");
        final Certificate cert = getCertificate(FAKE_RSA_1.caCertificate);

        // Install keypairs.
        assertTrue(mDevicePolicyManager.installKeyPair(getWho(), privKey, new Certificate[] {cert},
                grant, true));
        assertTrue(mDevicePolicyManager.installKeyPair(getWho(), privKey, new Certificate[] {cert},
                withhold, false));
        try {
            // Verify only the requested key was actually granted.
            assertGranted(grant, true);
            assertGranted(withhold, false);

            // Verify the granted key is actually obtainable in PrivateKey form.
            assertEquals(KeyChain.getPrivateKey(mActivity, grant).getAlgorithm(), "RSA");
        } finally {
            // Delete both keypairs.
            assertTrue(mDevicePolicyManager.removeKeyPair(getWho(), grant));
            assertTrue(mDevicePolicyManager.removeKeyPair(getWho(), withhold));
        }
        // Verify they're actually gone.
        assertGranted(grant, false);
        assertGranted(withhold, false);
    }

    private List<Certificate> loadCertificateChain(String assetName) throws Exception {
        final Collection<Certificate> certs = loadCertificatesFromAsset(assetName);
        final ArrayList<Certificate> certChain = new ArrayList(certs);
        // Some sanity check on the cert chain
        assertTrue(certs.size() > 1);
        for (int i = 1; i < certChain.size(); i++) {
            certChain.get(i - 1).verify(certChain.get(i).getPublicKey());
        }
        return certChain;
    }

    public void testCanInstallCertChain() throws Exception {
        // Use assets/generate-client-cert-chain.sh to regenerate the client cert chain.
        final PrivateKey privKey = loadPrivateKeyFromAsset("user-cert-chain.key");
        final Certificate[] certChain = loadCertificateChain("user-cert-chain.crt")
                .toArray(new Certificate[0]);
        final String alias = "com.android.test.clientkeychain";

        // Install keypairs.
        assertTrue(mDevicePolicyManager.installKeyPair(getWho(), privKey, certChain, alias, true));
        try {
            // Verify only the requested key was actually granted.
            assertGranted(alias, true);

            // Verify the granted key is actually obtainable in PrivateKey form.
            assertEquals(KeyChain.getPrivateKey(mActivity, alias).getAlgorithm(), "RSA");

            // Verify the certificate chain is correct
            X509Certificate[] returnedCerts = KeyChain.getCertificateChain(mActivity, alias);
            assertTrue(Arrays.equals(certChain, returnedCerts));
        } finally {
            // Delete both keypairs.
            assertTrue(mDevicePolicyManager.removeKeyPair(getWho(), alias));
        }
        // Verify they're actually gone.
        assertGranted(alias, false);
    }

    public void testGrantsDoNotPersistBetweenInstallations() throws Exception {
        final String alias = "com.android.test.persistent-key-1";
        final PrivateKey privKey = getPrivateKey(FAKE_RSA_1.privateKey , "RSA");
        final Certificate cert = getCertificate(FAKE_RSA_1.caCertificate);

        // Install keypair.
        assertTrue(mDevicePolicyManager.installKeyPair(getWho(), privKey, new Certificate[] {cert},
                alias, true));
        try {
            assertGranted(alias, true);
        } finally {
            // Delete and verify.
            assertTrue(mDevicePolicyManager.removeKeyPair(getWho(), alias));
        }
        assertGranted(alias, false);

        // Install again.
        assertTrue(mDevicePolicyManager.installKeyPair(getWho(), privKey, new Certificate[] {cert},
                alias, false));
        try {
            assertGranted(alias, false);
        } finally {
            // Delete.
            assertTrue(mDevicePolicyManager.removeKeyPair(getWho(), alias));
        }
    }

    public void testNullKeyParamsFailPredictably() throws Exception {
        final String alias = "com.android.test.null-key-1";
        final PrivateKey privKey = getPrivateKey(FAKE_RSA_1.privateKey, "RSA");
        final Certificate cert = getCertificate(FAKE_RSA_1.caCertificate);
        try {
            mDevicePolicyManager.installKeyPair(getWho(), null, cert, alias);
            fail("Exception should have been thrown for null PrivateKey");
        } catch (NullPointerException expected) {
        }
        try {
            mDevicePolicyManager.installKeyPair(getWho(), privKey, null, alias);
            fail("Exception should have been thrown for null Certificate");
        } catch (NullPointerException expected) {
        }
    }

    public void testNullAdminComponentIsDenied() throws Exception {
        final String alias = "com.android.test.null-admin-1";
        final PrivateKey privKey = getPrivateKey(FAKE_RSA_1.privateKey, "RSA");
        final Certificate cert = getCertificate(FAKE_RSA_1.caCertificate);
        try {
            mDevicePolicyManager.installKeyPair(null, privKey, cert, alias);
            fail("Exception should have been thrown for null ComponentName");
        } catch (SecurityException expected) {
        }
    }

    public void testNotUserSelectableAliasCanBeChosenViaPolicy() throws Exception {
        final String alias = "com.android.test.not-selectable-key-1";
        final PrivateKey privKey = getPrivateKey(FAKE_RSA_1.privateKey , "RSA");
        final Certificate cert = getCertificate(FAKE_RSA_1.caCertificate);

        // Install keypair.
        assertTrue(mDevicePolicyManager.installKeyPair(
            getWho(), privKey, new Certificate[] {cert}, alias, 0));
        try {
            // Request and retrieve using the alias.
            assertGranted(alias, false);
            assertEquals(alias, new KeyChainAliasFuture(alias).get());
            assertGranted(alias, true);
        } finally {
            // Delete regardless of whether the test succeeded.
            assertTrue(mDevicePolicyManager.removeKeyPair(getWho(), alias));
        }
    }

    byte[] signDataWithKey(String algoIdentifier, PrivateKey privateKey) throws Exception {
        byte[] data = new String("hello").getBytes();
        Signature sign = Signature.getInstance(algoIdentifier);
        sign.initSign(privateKey);
        sign.update(data);
        return sign.sign();
    }

    void verifySignature(String algoIdentifier, PublicKey publicKey, byte[] signature)
            throws Exception {
        byte[] data = new String("hello").getBytes();
        Signature verify = Signature.getInstance(algoIdentifier);
        verify.initVerify(publicKey);
        verify.update(data);
        assertTrue(verify.verify(signature));
    }

    void verifySignatureOverData(String algoIdentifier, KeyPair keyPair) throws Exception {
        verifySignature(algoIdentifier, keyPair.getPublic(),
                signDataWithKey(algoIdentifier, keyPair.getPrivate()));
    }

    public void testCanGenerateRSAKeyPair() throws Exception {
        final String alias = "com.android.test.generated-rsa-1";
        try {
            KeyGenParameterSpec spec = new KeyGenParameterSpec.Builder(
                    alias,
                    KeyProperties.PURPOSE_SIGN | KeyProperties.PURPOSE_VERIFY)
                    .setKeySize(2048)
                    .setDigests(KeyProperties.DIGEST_SHA256)
                    .setSignaturePaddings(KeyProperties.SIGNATURE_PADDING_RSA_PSS,
                        KeyProperties.SIGNATURE_PADDING_RSA_PKCS1)
                    .build();

            AttestedKeyPair generated = mDevicePolicyManager.generateKeyPair(
                    getWho(), "RSA", spec, 0);
            assertNotNull(generated);
            verifySignatureOverData("SHA256withRSA", generated.getKeyPair());
        } finally {
            assertTrue(mDevicePolicyManager.removeKeyPair(getWho(), alias));
        }
    }

    public void testCanGenerateECKeyPair() throws Exception {
        final String alias = "com.android.test.generated-ec-1";
        try {
            KeyGenParameterSpec spec = new KeyGenParameterSpec.Builder(
                    alias,
                    KeyProperties.PURPOSE_SIGN | KeyProperties.PURPOSE_VERIFY)
                    .setDigests(KeyProperties.DIGEST_SHA256)
                    .build();

            AttestedKeyPair generated = mDevicePolicyManager.generateKeyPair(
                    getWho(), "EC", spec, 0);
            assertNotNull(generated);
            verifySignatureOverData("SHA256withECDSA", generated.getKeyPair());
        } finally {
            assertTrue(mDevicePolicyManager.removeKeyPair(getWho(), alias));
        }
    }

    private void validateDeviceIdAttestationData(Certificate leaf,
            String expectedSerial, String expectedImei, String expectedMeid)
            throws CertificateParsingException {
        Attestation attestationRecord = new Attestation((X509Certificate) leaf);
        AuthorizationList teeAttestation = attestationRecord.getTeeEnforced();
        assertNotNull(teeAttestation);
        assertEquals(Build.BRAND, teeAttestation.getBrand());
        assertEquals(Build.DEVICE, teeAttestation.getDevice());
        assertEquals(Build.PRODUCT, teeAttestation.getProduct());
        assertEquals(Build.MANUFACTURER, teeAttestation.getManufacturer());
        assertEquals(Build.MODEL, teeAttestation.getModel());
        assertEquals(expectedSerial, teeAttestation.getSerialNumber());
        assertEquals(expectedImei, teeAttestation.getImei());
        assertEquals(expectedMeid, teeAttestation.getMeid());
    }

    private void validateAttestationRecord(List<Certificate> attestation,
            byte[] providedChallenge) throws CertificateParsingException {
        assertNotNull(attestation);
        assertTrue(attestation.size() >= 2);
        X509Certificate leaf = (X509Certificate) attestation.get(0);
        Attestation attestationRecord = new Attestation(leaf);
        assertTrue(Arrays.equals(providedChallenge,
                    attestationRecord.getAttestationChallenge()));
    }

    private void validateSignatureChain(List<Certificate> chain, PublicKey leafKey)
            throws GeneralSecurityException {
        X509Certificate leaf = (X509Certificate) chain.get(0);
        PublicKey keyFromCert = leaf.getPublicKey();
        assertTrue(Arrays.equals(keyFromCert.getEncoded(), leafKey.getEncoded()));
        // Check that the certificate chain is valid.
        for (int i = 1; i < chain.size(); i++) {
            X509Certificate intermediate = (X509Certificate) chain.get(i);
            PublicKey intermediateKey = intermediate.getPublicKey();
            leaf.verify(intermediateKey);
            leaf = intermediate;
        }

        // leaf is now the root, verify the root is self-signed.
        PublicKey rootKey = leaf.getPublicKey();
        leaf.verify(rootKey);
    }

    private boolean isDeviceIdAttestationSupported() {
        return mDevicePolicyManager.isDeviceIdAttestationSupported();
    }

    private boolean isDeviceIdAttestationRequested(int deviceIdAttestationFlags) {
        return deviceIdAttestationFlags != 0;
    }

    /**
     * Generates a key using DevicePolicyManager.generateKeyPair using the given key algorithm,
     * then test signing and verifying using generated key.
     * If {@code signaturePaddings} is not null, it is added to the key parameters specification.
     * Returns the Attestation leaf certificate.
     */
    private Certificate generateKeyAndCheckAttestation(
            String keyAlgorithm, String signatureAlgorithm,
            String[] signaturePaddings, int deviceIdAttestationFlags)
            throws Exception {
        final String alias =
                String.format("com.android.test.attested-%s", keyAlgorithm.toLowerCase());
        byte[] attestationChallenge = new byte[] {0x01, 0x02, 0x03};
        try {
            KeyGenParameterSpec.Builder specBuilder =  new KeyGenParameterSpec.Builder(
                    alias,
                    KeyProperties.PURPOSE_SIGN | KeyProperties.PURPOSE_VERIFY)
                    .setDigests(KeyProperties.DIGEST_SHA256)
                    .setAttestationChallenge(attestationChallenge);
            if (signaturePaddings != null) {
                specBuilder.setSignaturePaddings(signaturePaddings);
            }

            KeyGenParameterSpec spec = specBuilder.build();
            AttestedKeyPair generated = mDevicePolicyManager.generateKeyPair(
                    getWho(), keyAlgorithm, spec, deviceIdAttestationFlags);
            // If Device ID attestation was requested, check it succeeded if and only if device ID
            // attestation is supported.
            if (isDeviceIdAttestationRequested(deviceIdAttestationFlags)) {
                if (generated == null) {
                    assertFalse(String.format("Failed getting Device ID attestation for key " +
                                "algorithm %s, with flags %s, despite device declaring support.",
                                keyAlgorithm, deviceIdAttestationFlags),
                            isDeviceIdAttestationSupported());
                    return null;
                } else {
                    assertTrue(String.format("Device ID attestation for key " +
                                "algorithm %s, with flags %d should not have succeeded.",
                                keyAlgorithm, deviceIdAttestationFlags),
                            isDeviceIdAttestationSupported());
                }
            } else {
                assertNotNull(
                        String.format("Key generation (of type %s) must succeed when Device ID " +
                            "attestation was not requested.", keyAlgorithm), generated);
            }
            final KeyPair keyPair = generated.getKeyPair();
            verifySignatureOverData(signatureAlgorithm, keyPair);
            List<Certificate> attestation = generated.getAttestationRecord();
            validateAttestationRecord(attestation, attestationChallenge);
            validateSignatureChain(attestation, keyPair.getPublic());
            return attestation.get(0);
        } catch (UnsupportedOperationException ex) {
            assertTrue(String.format("Unexpected failure while generating key %s with ID flags %d: %s",
                        keyAlgorithm, deviceIdAttestationFlags, ex),
                    isDeviceIdAttestationRequested(deviceIdAttestationFlags) &&
                    !isDeviceIdAttestationSupported());
            return null;
        } finally {
            assertTrue(mDevicePolicyManager.removeKeyPair(getWho(), alias));
        }
    }

    /**
     * Test key generation, including requesting Key Attestation, for all supported key
     * algorithms.
     */
    public void testCanGenerateKeyPairWithKeyAttestation() throws Exception {
        for (SupportedKeyAlgorithm supportedKey: SUPPORTED_KEY_ALGORITHMS) {
            assertNotNull(generateKeyAndCheckAttestation(
                    supportedKey.keyAlgorithm, supportedKey.signatureAlgorithm,
                    supportedKey.signaturePaddingSchemes, 0));
        }
    }

    public void testAllVariationsOfDeviceIdAttestation() throws Exception {
        List<Integer> modesToTest = new ArrayList<Integer>();
        String imei = null;
        String meid = null;
        // All devices must support at least basic device information attestation as well as serial
        // number attestation. Although attestation of unique device ids are only callable by device
        // owner.
        modesToTest.add(ID_TYPE_BASE_INFO);
        if (isDeviceOwner()) {
            modesToTest.add(ID_TYPE_SERIAL);
            // Get IMEI and MEID of the device.
            TelephonyManager telephonyService = (TelephonyManager) mActivity.getSystemService(
                    Context.TELEPHONY_SERVICE);
            assertNotNull("Need to be able to read device identifiers", telephonyService);
            imei = telephonyService.getImei(0);
            meid = telephonyService.getMeid(0);
            // If the device has a valid IMEI it must support attestation for it.
            if (imei != null) {
                modesToTest.add(ID_TYPE_IMEI);
            }
            // Same for MEID
            if (meid != null) {
                modesToTest.add(ID_TYPE_MEID);
            }
        }
        int numCombinations = 1 << modesToTest.size();
        for (int i = 1; i < numCombinations; i++) {
            // Set the bits in devIdOpt to be passed into generateKeyPair according to the
            // current modes tested.
            int devIdOpt = 0;
            for (int j = 0; j < modesToTest.size(); j++) {
                if ((i & (1 << j)) != 0) {
                    devIdOpt = devIdOpt | modesToTest.get(j);
                }
            }
            try {
                // Now run the test with all supported key algorithms
                for (SupportedKeyAlgorithm supportedKey: SUPPORTED_KEY_ALGORITHMS) {
                    Certificate attestation = generateKeyAndCheckAttestation(
                            supportedKey.keyAlgorithm, supportedKey.signatureAlgorithm,
                            supportedKey.signaturePaddingSchemes, devIdOpt);
                    // generateKeyAndCheckAttestation should return null if device ID attestation
                    // is not supported. Simply continue to test the next combination.
                    if (attestation == null && !isDeviceIdAttestationSupported()) {
                        continue;
                    }
                    assertNotNull(String.format(
                            "Attestation should be valid for key %s with attestation modes %s",
                            supportedKey.keyAlgorithm, devIdOpt), attestation);
                    // Set the expected values for serial, IMEI and MEID depending on whether
                    // attestation for them was requested.
                    String expectedSerial = null;
                    if ((devIdOpt & ID_TYPE_SERIAL) != 0) {
                        expectedSerial = Build.getSerial();
                    }
                    String expectedImei = null;
                    if ((devIdOpt & ID_TYPE_IMEI) != 0) {
                        expectedImei = imei;
                    }
                    String expectedMeid = null;
                    if ((devIdOpt & ID_TYPE_MEID) != 0) {
                        expectedMeid = meid;
                    }
                    validateDeviceIdAttestationData(attestation, expectedSerial, expectedImei,
                            expectedMeid);
                }
            } catch (UnsupportedOperationException expected) {
                // Make sure the test only fails if the device is not meant to support Device
                // ID attestation.
                assertFalse(isDeviceIdAttestationSupported());
            }
        }
    }

    public void testProfileOwnerCannotAttestDeviceUniqueIds() throws Exception {
        if (isDeviceOwner()) {
            return;
        }
        int[] forbiddenModes = new int[] {ID_TYPE_SERIAL, ID_TYPE_IMEI, ID_TYPE_MEID};
        for (int i = 0; i < forbiddenModes.length; i++) {
            try {
                for (SupportedKeyAlgorithm supportedKey: SUPPORTED_KEY_ALGORITHMS) {
                    generateKeyAndCheckAttestation(supportedKey.keyAlgorithm,
                            supportedKey.signatureAlgorithm,
                            supportedKey.signaturePaddingSchemes,
                            forbiddenModes[i]);
                    fail("Attestation of device UID (" + forbiddenModes[i] + ") should not be "
                            + "possible from profile owner");
                }
            } catch (SecurityException e) {
                assertTrue(e.getMessage().contains("does not own the device"));
            }
        }
    }

    public void testCanSetKeyPairCert() throws Exception {
        final String alias = "com.android.test.set-ec-1";
        try {
            KeyGenParameterSpec spec = new KeyGenParameterSpec.Builder(
                    alias,
                    KeyProperties.PURPOSE_SIGN | KeyProperties.PURPOSE_VERIFY)
                    .setDigests(KeyProperties.DIGEST_SHA256)
                    .build();

            AttestedKeyPair generated = mDevicePolicyManager.generateKeyPair(
                    getWho(), "EC", spec, 0);
            assertNotNull(generated);
            // Create a self-signed cert to go with it.
            X500Principal issuer = new X500Principal("CN=SelfSigned, O=Android, C=US");
            X500Principal subject = new X500Principal("CN=Subject, O=Android, C=US");
            X509Certificate cert = createCertificate(generated.getKeyPair(), subject, issuer);
            // Set the certificate chain
            List<Certificate> certs = new ArrayList<Certificate>();
            certs.add(cert);
            mDevicePolicyManager.setKeyPairCertificate(getWho(), alias, certs, true);
            // Make sure that the alias can now be obtained.
            assertEquals(alias, new KeyChainAliasFuture(alias).get());
            // And can be retrieved from KeyChain
            X509Certificate[] fetchedCerts = KeyChain.getCertificateChain(mActivity, alias);
            assertEquals(fetchedCerts.length, certs.size());
            assertTrue(Arrays.equals(fetchedCerts[0].getEncoded(), certs.get(0).getEncoded()));
        } finally {
            assertTrue(mDevicePolicyManager.removeKeyPair(getWho(), alias));
        }
    }

    public void testCanSetKeyPairCertChain() throws Exception {
        final String alias = "com.android.test.set-ec-2";
        try {
            KeyGenParameterSpec spec = new KeyGenParameterSpec.Builder(
                    alias,
                    KeyProperties.PURPOSE_SIGN | KeyProperties.PURPOSE_VERIFY)
                    .setDigests(KeyProperties.DIGEST_SHA256)
                    .build();

            AttestedKeyPair generated = mDevicePolicyManager.generateKeyPair(
                    getWho(), "EC", spec, 0);
            assertNotNull(generated);
            List<Certificate> chain = loadCertificateChain("user-cert-chain.crt");
            mDevicePolicyManager.setKeyPairCertificate(getWho(), alias, chain, true);
            // Make sure that the alias can now be obtained.
            assertEquals(alias, new KeyChainAliasFuture(alias).get());
            // And can be retrieved from KeyChain
            X509Certificate[] fetchedCerts = KeyChain.getCertificateChain(mActivity, alias);
            assertEquals(fetchedCerts.length, chain.size());
            for (int i = 0; i < chain.size(); i++) {
                assertTrue(Arrays.equals(fetchedCerts[i].getEncoded(), chain.get(i).getEncoded()));
            }
        } finally {
            assertTrue(mDevicePolicyManager.removeKeyPair(getWho(), alias));
        }
    }

    private void assertGranted(String alias, boolean expected) throws InterruptedException {
        boolean granted = false;
        try {
            granted = (KeyChain.getPrivateKey(mActivity, alias) != null);
        } catch (KeyChainException e) {
            if (expected) {
                e.printStackTrace();
            }
        }
        assertEquals("Grant for alias: \"" + alias + "\"", expected, granted);
    }

    private static PrivateKey getPrivateKey(final byte[] key, String type)
            throws NoSuchAlgorithmException, InvalidKeySpecException {
        return KeyFactory.getInstance(type).generatePrivate(
                new PKCS8EncodedKeySpec(key));
    }

    private static Certificate getCertificate(byte[] cert) throws CertificateException {
        return CertificateFactory.getInstance("X.509").generateCertificate(
                new ByteArrayInputStream(cert));
    }

    private Collection<Certificate> loadCertificatesFromAsset(String assetName) {
        try {
            final CertificateFactory certFactory = CertificateFactory.getInstance("X.509");
            AssetManager am = mActivity.getAssets();
            InputStream is = am.open(assetName);
            return (Collection<Certificate>) certFactory.generateCertificates(is);
        } catch (IOException | CertificateException e) {
            e.printStackTrace();
        }
        return null;
    }

    private PrivateKey loadPrivateKeyFromAsset(String assetName) {
        try {
            AssetManager am = mActivity.getAssets();
            InputStream is = am.open(assetName);
            ByteArrayOutputStream output = new ByteArrayOutputStream();
            int length;
            byte[] buffer = new byte[4096];
            while ((length = is.read(buffer, 0, buffer.length)) != -1) {
              output.write(buffer, 0, length);
            }
            return getPrivateKey(output.toByteArray(), "RSA");
        } catch (IOException | NoSuchAlgorithmException | InvalidKeySpecException e) {
            e.printStackTrace();
        }
        return null;
    }

    private class KeyChainAliasFuture implements KeyChainAliasCallback {
        private final CountDownLatch mLatch = new CountDownLatch(1);
        private String mChosenAlias = null;

        @Override
        public void alias(final String chosenAlias) {
            mChosenAlias = chosenAlias;
            mLatch.countDown();
        }

        public KeyChainAliasFuture(String alias)
                throws UnsupportedEncodingException {
            /* Pass the alias as a GET to an imaginary server instead of explicitly asking for it,
             * to make sure the DPC actually has to do some work to grant the cert.
             */
            final Uri uri =
                    Uri.parse("https://example.org/?alias=" + URLEncoder.encode(alias, "UTF-8"));
            KeyChain.choosePrivateKeyAlias(mActivity, this,
                    null /* keyTypes */, null /* issuers */, uri, null /* alias */);
        }

        public String get() throws InterruptedException {
            assertTrue("Chooser timeout", mLatch.await(KEYCHAIN_TIMEOUT_MINS, TimeUnit.MINUTES));
            return mChosenAlias;
        }
    }

    protected ComponentName getWho() {
        return ADMIN_RECEIVER_COMPONENT;
    }
}
