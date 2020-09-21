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

package android.security.cts;

import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import android.content.pm.PackageManager;
import android.platform.test.annotations.SecurityTest;
import android.test.AndroidTestCase;
import java.security.KeyStore;
import java.security.KeyStoreException;
import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;
import java.security.cert.Certificate;
import java.security.cert.CertificateEncodingException;
import java.security.cert.CertificateException;
import java.security.cert.CertificateFactory;
import java.security.cert.X509Certificate;
import java.util.Arrays;
import java.util.Collections;
import java.util.HashSet;
import java.util.List;
import java.util.Set;

@SecurityTest
public class CertificateTest extends AndroidTestCase {
    // The directory for CA root certificates trusted by WFA (WiFi Alliance)
    static final String DIR_OF_CACERTS_FOR_WFA = "/etc/security/cacerts_wfa";

    public void testNoRemovedCertificates() throws Exception {
        Set<String> expectedCertificates = new HashSet<String>(
                Arrays.asList(CertificateData.CERTIFICATE_DATA));
        Set<String> deviceCertificates = getDeviceCertificates();
        expectedCertificates.removeAll(deviceCertificates);
        assertEquals("Missing CA certificates", Collections.EMPTY_SET, expectedCertificates);
    }

    /**
     * If you fail CTS as a result of adding a root CA that is not part of the Android root CA
     * store, please see the following.
     *
     * <p>This test exists because adding root CAs to a device has a very significant security
     * impact. Whoever has access to the signing keys of that CA can compromise secure network
     * traffic from affected Android devices, putting users at risk.
     *
     * <p>If you have a CA certificate which needs to be trusted by a particular app/service,
     * ask the developer of the app/service to modify it to trust this CA (e.g., using Network
     * Security Config feature). This avoids compromising the security of network traffic of other
     * apps on the device.
     *
     * <p>If you have a CA certificate that you believe should be present on all Android devices,
     * please file a public bug at https://code.google.com/p/android/issues/entry.
     *
     * <p>For questions, comments, and code reviews please contact security@android.com.
     */
    public void testNoAddedCertificates() throws Exception {
        Set<String> expectedCertificates = new HashSet<String>(
                Arrays.asList(CertificateData.CERTIFICATE_DATA));
        Set<String> deviceCertificates = getDeviceCertificates();
        deviceCertificates.removeAll(expectedCertificates);
        assertEquals("Unknown CA certificates", Collections.EMPTY_SET, deviceCertificates);
    }

    public void testBlockCertificates() throws Exception {
        Set<String> blockCertificates = new HashSet<String>();
        blockCertificates.add("C0:60:ED:44:CB:D8:81:BD:0E:F8:6C:0B:A2:87:DD:CF:81:67:47:8C");

        Set<String> deviceCertificates = getDeviceCertificates();
        deviceCertificates.retainAll(blockCertificates);
        assertEquals("Blocked CA certificates", Collections.EMPTY_SET, deviceCertificates);
    }

    /**
     * This test exists because adding new ca certificate or removing the ca certificates trusted by
     * WFA (WiFi Alliance) is not allowed.
     *
     * For questions, comments, and code reviews please contact security@android.com.
     */
    public void testNoRemovedWfaCertificates() throws Exception {
        if (!supportPasspoint()) {
            return;
        }
        Set<String> expectedCertificates = new HashSet<>(
                Arrays.asList(CertificateData.WFA_CERTIFICATE_DATA));
        Set<String> deviceWfaCertificates = getDeviceWfaCertificates();
        expectedCertificates.removeAll(deviceWfaCertificates);
        assertEquals("Missing WFA CA certificates", Collections.EMPTY_SET, expectedCertificates);
    }

    public void testNoAddedWfaCertificates() throws Exception {
        if (!supportPasspoint()) {
            return;
        }
        Set<String> expectedCertificates = new HashSet<String>(
                Arrays.asList(CertificateData.WFA_CERTIFICATE_DATA));
        Set<String> deviceWfaCertificates = getDeviceWfaCertificates();
        deviceWfaCertificates.removeAll(expectedCertificates);
        assertEquals("Unknown WFA CA certificates", Collections.EMPTY_SET, deviceWfaCertificates);
    }

    private boolean supportPasspoint() {
        return mContext.getPackageManager().hasSystemFeature(PackageManager.FEATURE_WIFI_PASSPOINT);
    }

    private KeyStore createWfaKeyStore(String dirPath) throws CertificateException, IOException,
            KeyStoreException, NoSuchAlgorithmException {
        KeyStore keyStore = KeyStore.getInstance(KeyStore.getDefaultType());
        keyStore.load(null, null);
        String wfaCertsDir = System.getenv("ANDROID_ROOT") + dirPath;
        int index = 0;
        for (X509Certificate cert : loadCertsFromDisk(wfaCertsDir)) {
            keyStore.setCertificateEntry(String.format("%d", index++), cert);
        }
        return keyStore;
    }

    private Set<X509Certificate> loadCertsFromDisk(String directory) throws CertificateException,
            IOException {
        Set<X509Certificate> certs = new HashSet<>();
        File certDir = new File(directory);
        File[] certFiles = certDir.listFiles();
        if (certFiles == null || certFiles.length <= 0) {
            return certs;
        }
        CertificateFactory certFactory = CertificateFactory.getInstance("X.509");
        for (File certFile : certFiles) {
            FileInputStream fis = new FileInputStream(certFile);
            Certificate cert = certFactory.generateCertificate(fis);
            if (cert instanceof X509Certificate) {
                certs.add((X509Certificate) cert);
            }
            fis.close();
        }
        return certs;
    }

    private Set<String> getDeviceWfaCertificates() throws KeyStoreException,
            NoSuchAlgorithmException, CertificateException, IOException {
        KeyStore wfaKeyStore = createWfaKeyStore(DIR_OF_CACERTS_FOR_WFA);
        List<String> aliases = Collections.list(wfaKeyStore.aliases());
        assertFalse(aliases.isEmpty());

        Set<String> certificates = new HashSet<>();
        for (String alias : aliases) {
            assertTrue(wfaKeyStore.isCertificateEntry(alias));
            X509Certificate certificate = (X509Certificate) wfaKeyStore.getCertificate(alias);
            assertEquals(certificate.getSubjectUniqueID(), certificate.getIssuerUniqueID());
            assertNotNull(certificate.getSubjectDN());
            assertNotNull(certificate.getIssuerDN());
            String fingerprint = getFingerprint(certificate);
            certificates.add(fingerprint);
        }
        return certificates;
    }

    private Set<String> getDeviceCertificates() throws KeyStoreException,
            NoSuchAlgorithmException, CertificateException, IOException {
        KeyStore keyStore = KeyStore.getInstance("AndroidCAStore");
        keyStore.load(null, null);

        List<String> aliases = Collections.list(keyStore.aliases());
        assertFalse(aliases.isEmpty());

        Set<String> certificates = new HashSet<String>();
        for (String alias : aliases) {
            assertTrue(keyStore.isCertificateEntry(alias));
            X509Certificate certificate = (X509Certificate) keyStore.getCertificate(alias);
            assertEquals(certificate.getSubjectUniqueID(), certificate.getIssuerUniqueID());
            assertNotNull(certificate.getSubjectDN());
            assertNotNull(certificate.getIssuerDN());
            String fingerprint = getFingerprint(certificate);
            certificates.add(fingerprint);
        }
        return certificates;
    }

    private String getFingerprint(X509Certificate certificate) throws CertificateEncodingException,
            NoSuchAlgorithmException {
        MessageDigest messageDigest = MessageDigest.getInstance("SHA1");
        messageDigest.update(certificate.getEncoded());
        byte[] sha1 = messageDigest.digest();
        return convertToHexFingerprint(sha1);
    }

    private String convertToHexFingerprint(byte[] sha1) {
        StringBuilder fingerprint = new StringBuilder();
        for (int i = 0; i < sha1.length; i++) {
            fingerprint.append(String.format("%02X", sha1[i]));
            if (i + 1 < sha1.length) {
                fingerprint.append(":");
            }
        }
        return fingerprint.toString();
    }
}
