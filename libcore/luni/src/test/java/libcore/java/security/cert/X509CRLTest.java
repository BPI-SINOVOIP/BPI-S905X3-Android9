/*
 * Copyright (C) 2012 The Android Open Source Project
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

package libcore.java.security.cert;

import static java.nio.charset.StandardCharsets.UTF_8;

import dalvik.system.VMRuntime;
import sun.security.jca.Providers;
import sun.security.provider.X509Factory;
import sun.security.x509.X509CRLImpl;
import tests.support.resource.Support_Resources;

import java.io.BufferedReader;
import java.io.ByteArrayOutputStream;
import java.io.DataInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.PrintStream;
import java.security.InvalidKeyException;
import java.security.Provider;
import java.security.Security;
import java.security.cert.CRL;
import java.security.cert.CRLReason;
import java.security.cert.CertificateFactory;
import java.security.cert.X509CRL;
import java.security.cert.X509CRLEntry;
import java.security.cert.X509Certificate;
import java.text.SimpleDateFormat;
import java.util.Arrays;
import java.util.Calendar;
import java.util.Date;
import java.util.HashMap;
import java.util.Locale;
import java.util.Map;
import java.util.Set;

import junit.framework.TestCase;
import libcore.java.security.StandardNames;

public class X509CRLTest extends TestCase {

    @Override
    public void setUp() throws Exception {
        super.setUp();
        mX509Providers = Security.getProviders("CertificateFactory.X509");

        // Allow access to deprecated BC algorithms in this test, so we can ensure they
        // continue to work
        Providers.setMaximumAllowableApiLevelForBcDeprecation(
                VMRuntime.getRuntime().getTargetSdkVersion());
    }

    @Override
    public void tearDown() throws Exception {
        Providers.setMaximumAllowableApiLevelForBcDeprecation(
                Providers.DEFAULT_MAXIMUM_ALLOWABLE_TARGET_API_LEVEL_FOR_BC_DEPRECATION);
        super.tearDown();
    }

    private Provider[] mX509Providers;

    private static final String CERT_RSA = "x509/cert-rsa.der";

    private static final String CERT_DSA = "x509/cert-dsa.der";

    private static final String CERT_CRL_CA = "x509/cert-crl-ca.der";

    private static final String CRL_RSA = "x509/crl-rsa.der";

    private static final String CRL_RSA_DSA = "x509/crl-rsa-dsa.der";

    private static final String CRL_RSA_DSA_SIGOPT = "x509/crl-rsa-dsa-sigopt.der";

    private static final String CRL_UNSUPPORTED = "x509/crl-unsupported.der";

    private static final String CRL_RSA_DATES = "x509/crl-rsa-dates.txt";

    private static final String CRL_RSA_DSA_DATES = "x509/crl-rsa-dsa-dates.txt";

    private static final String CRL_RSA_SIG = "x509/crl-rsa-sig.der";

    private static final String CRL_RSA_TBS = "x509/crl-rsa-tbs.der";

    private static final String CRL_EMPTY = "x509/crl-empty.der";

    private final X509Certificate getCertificate(CertificateFactory f, String name)
            throws Exception {
        final InputStream is = Support_Resources.getStream(name);
        assertNotNull("File does not exist: " + name, is);
        try {
            X509Certificate c = (X509Certificate) f.generateCertificate(is);
            assertNotNull(c);
            return c;
        } finally {
            try {
                is.close();
            } catch (IOException ignored) {
            }
        }
    }

    private final X509CRL getCRL(CertificateFactory f, String name) throws Exception {
        final InputStream is = Support_Resources.getStream(name);
        assertNotNull("File does not exist: " + name, is);
        try {
            X509CRL crl = (X509CRL) f.generateCRL(is);
            assertNotNull(crl);
            return crl;
        } finally {
            try {
                is.close();
            } catch (IOException ignored) {
            }
        }
    }

    private byte[] getResourceAsBytes(String name) throws Exception {
        final InputStream ris = Support_Resources.getStream(name);
        try {
            DataInputStream dis = new DataInputStream(ris);
            byte[] buf = new byte[ris.available()];
            dis.readFully(buf);
            return buf;
        } finally {
            try {
                ris.close();
            } catch (IOException ignored) {
            }
        }
    }

    private Map<String, Date> getCrlDates(String name) throws Exception {
        Map<String, Date> dates = new HashMap<String, Date>();
        final SimpleDateFormat sdf = new SimpleDateFormat("MMM dd HH:mm:ss yyyy zzz", Locale.US);

        final InputStream ris = Support_Resources.getStream(name);
        try {

            final BufferedReader buf = new BufferedReader(new InputStreamReader(ris, UTF_8));

            String line;
            while ((line = buf.readLine()) != null) {
                int index = line.indexOf('=');
                String key = line.substring(0, index);
                final Date value = sdf.parse(line.substring(index + 1));
                dates.put(key, value);
            }

            return dates;
        } finally {
            try {
                ris.close();
            } catch (IOException ignored) {
            }
        }
    }

    public void test_X509CRLImpl_verify() throws Exception {
        CertificateFactory f = CertificateFactory.getInstance("X.509");
        X509CRL crlRsa = getCRL(f, CRL_RSA);
        X509CRLImpl interned = X509Factory.intern(crlRsa);
        X509Certificate caCert = getCertificate(f, CERT_CRL_CA);
        interned.verify(caCert.getPublicKey(), f.getProvider());
    }

    public void test_Provider() throws Exception {
        final ByteArrayOutputStream errBuffer = new ByteArrayOutputStream();
        PrintStream out = new PrintStream(errBuffer);

        for (Provider p : mX509Providers) {
            try {
                CertificateFactory f = CertificateFactory.getInstance("X.509", p);
                isRevoked(f);
                getType(f);
                getEncoded(f);
                getVersion(f);
                hasUnsupportedCriticalExtension(f);
                getSignature(f);
                getTBSCertList(f);
                getRevokedCertificates(f);
                getThisUpdateNextUpdate(f);
                getSigAlgName(f);
                getSigAlgOID(f);
                getSigAlgParams(f);
                verify(f);
                test_toString(f);
                test_equals(f);
            } catch (Throwable e) {
                out.append("Error encountered checking " + p.getName() + "\n");
                e.printStackTrace(out);
            }
        }

        out.flush();
        if (errBuffer.size() > 0) {
            throw new Exception("Errors encountered:\n\n" + errBuffer.toString() + "\n\n");
        }
    }

    private void verify(CertificateFactory f) throws Exception {
        X509CRL crlRsa = getCRL(f, CRL_RSA);

        X509Certificate caCert = getCertificate(f, CERT_CRL_CA);

        // Test the "verify" method that doesn't specify the provider.
        crlRsa.verify(caCert.getPublicKey());

        // Test the "verify" method that does specify the provider.
        try {
            crlRsa.verify(caCert.getPublicKey(), f.getProvider());
        } catch (UnsupportedOperationException unsupportedOperationException) {
            // TODO(31294527): X590CRL objects from AndroidOpenSSL do not have this method.
            // The "default" implementation from OpenJDK results in an infinite loop, so in libcore
            // we throw an UnsupportedOperationException instead.
            if (!f.getProvider().getName().equals("AndroidOpenSSL")) {
                throw unsupportedOperationException;
            }
        }

        X509Certificate dsaCert = getCertificate(f, CERT_DSA);

        // Test the "verify" method that does specify the provider.
        try {
            crlRsa.verify(dsaCert.getPublicKey());
            fail("should not verify using incorrect key type");
        } catch (InvalidKeyException expected) {
        }

        // Test the "verify" method that does specify the provider.
        try {
            crlRsa.verify(dsaCert.getPublicKey(), f.getProvider());
            fail("should not verify using incorrect key type");
        } catch (InvalidKeyException expected) {

        } catch (UnsupportedOperationException unsupportedOperationException) {
            // TODO(31294527): X590CRL objects from AndroidOpenSSL do not have this method.
            // The "default" implementation from OpenJDK results in an infinite loop, so in libcore
            // we throw an UnsupportedOperationException instead.
            if (!f.getProvider().getName().equals("AndroidOpenSSL")) {
                throw unsupportedOperationException;
            }
        }
    }

    private void getType(CertificateFactory f) throws Exception {
        CRL crlRsa = getCRL(f, CRL_RSA);

        assertEquals("X.509", crlRsa.getType());
    }

    private void isRevoked(CertificateFactory f) throws Exception {
        X509Certificate rsaCert = getCertificate(f, CERT_RSA);
        X509Certificate dsaCert = getCertificate(f, CERT_DSA);
        X509CRL crlRsa = getCRL(f, CRL_RSA);
        X509CRL crlRsaDsa = getCRL(f, CRL_RSA_DSA);

        assertTrue(crlRsa.isRevoked(rsaCert));
        assertFalse(crlRsa.isRevoked(dsaCert));

        assertTrue(crlRsaDsa.isRevoked(rsaCert));
        assertTrue(crlRsaDsa.isRevoked(dsaCert));

        try {
            assertFalse(crlRsa.isRevoked(null));
            if ("BC".equals(f.getProvider().getName())) {
                fail("BouncyCastle throws on null input");
            }
        } catch (NullPointerException e) {
            if (!"BC".equals(f.getProvider().getName())) {
                fail("Should not throw on null input");
            }
        }
    }

    private void getThisUpdateNextUpdate(CertificateFactory f) throws Exception {
        {
            X509CRL crl = getCRL(f, CRL_RSA);
            Map<String, Date> dates = getCrlDates(CRL_RSA_DATES);

            Date lastUpdate = dates.get("lastUpdate");
            Date nextUpdate = dates.get("nextUpdate");

            assertNotNull(lastUpdate);
            assertNotNull(nextUpdate);

            assertDateEquals(lastUpdate, crl.getThisUpdate());
            assertDateEquals(nextUpdate, crl.getNextUpdate());
        }

        {
            X509CRL crl = getCRL(f, CRL_RSA_DSA);
            Map<String, Date> dates = getCrlDates(CRL_RSA_DSA_DATES);

            Date lastUpdate = dates.get("lastUpdate");
            Date nextUpdate = dates.get("nextUpdate");

            assertNotNull(lastUpdate);
            assertNotNull(nextUpdate);

            assertDateEquals(lastUpdate, crl.getThisUpdate());
            assertDateEquals(nextUpdate, crl.getNextUpdate());
        }
    }

    private void getSigAlgName(CertificateFactory f) throws Exception {
        X509CRL crlRsa = getCRL(f, CRL_RSA);
        assertEquals("SHA1WITHRSA",  getCRL(f, CRL_RSA).getSigAlgName().toUpperCase(Locale.ROOT));
    }

    private void getSigAlgOID(CertificateFactory f) throws Exception {
        X509CRL crlRsa = getCRL(f, CRL_RSA);

        assertEquals("1.2.840.113549.1.1.5", crlRsa.getSigAlgOID());
    }

    private void getVersion(CertificateFactory f) throws Exception {
        X509CRL crlRsa = getCRL(f, CRL_RSA);

        assertEquals(1, crlRsa.getVersion());
    }

    private void hasUnsupportedCriticalExtension(CertificateFactory f) throws Exception {
        X509CRL crlRsa = getCRL(f, CRL_RSA);
        assertFalse(crlRsa.hasUnsupportedCriticalExtension());

        X509CRL unsupportedCrl = getCRL(f, CRL_UNSUPPORTED);
        assertTrue(unsupportedCrl.hasUnsupportedCriticalExtension());
    }

    private void getSignature(CertificateFactory f) throws Exception {
        X509CRL crlRsa = getCRL(f, CRL_RSA);
        byte[] expected = getResourceAsBytes(CRL_RSA_SIG);

        assertEquals(Arrays.toString(expected), Arrays.toString(crlRsa.getSignature()));
    }

    private void getTBSCertList(CertificateFactory f) throws Exception {
        X509CRL crlRsa = getCRL(f, CRL_RSA);
        byte[] expected = getResourceAsBytes(CRL_RSA_TBS);

        assertEquals(Arrays.toString(expected), Arrays.toString(crlRsa.getTBSCertList()));
    }

    private void getEncoded(CertificateFactory f) throws Exception {
        X509CRL crlRsa = getCRL(f, CRL_RSA);

        byte[] crlRsaBytes = getResourceAsBytes(CRL_RSA);

        assertEquals(Arrays.toString(crlRsaBytes), Arrays.toString(crlRsa.getEncoded()));
    }

    private static void assertDateEquals(Date date1, Date date2) throws Exception {
        SimpleDateFormat formatter = new SimpleDateFormat("dd MMM yyyy HH:mm:ss");

        String result1 = formatter.format(date1);
        String result2 = formatter.format(date2);

        assertEquals(result1, result2);
    }

    /*
     * This is needed because the certificate revocation in our CRL can be a
     * couple seconds ahead of the lastUpdate time in the CRL.
     */
    private static void assertDateSlightlyBefore(Date expected, Date actual) throws Exception {
        Calendar c = Calendar.getInstance();

        // Make sure it's within 2 seconds of expected.
        c.setTime(expected);
        c.add(Calendar.SECOND, -2);
        assertTrue(actual.after(c.getTime()));

        // Before or equal...
        assertTrue(actual.before(expected) || actual.equals(expected));
    }

    private void assertRsaCrlEntry(CertificateFactory f, X509CRLEntry rsaEntry) throws Exception {
        assertNotNull(rsaEntry);

        X509Certificate rsaCert = getCertificate(f, CERT_RSA);
        Map<String, Date> dates = getCrlDates(CRL_RSA_DSA_DATES);
        Date expectedDate = dates.get("lastUpdate");

        assertEquals(rsaCert.getSerialNumber(), rsaEntry.getSerialNumber());
        assertDateSlightlyBefore(expectedDate, rsaEntry.getRevocationDate());
        assertNull(rsaEntry.getRevocationReason());
        assertNull(rsaEntry.getCertificateIssuer());
        assertFalse(rsaEntry.hasExtensions());
        assertNull(rsaEntry.getCriticalExtensionOIDs());
        assertNull(rsaEntry.getNonCriticalExtensionOIDs());

        assertNotNull(rsaEntry.toString());
    }

    private void assertDsaCrlEntry(CertificateFactory f, X509CRLEntry dsaEntry) throws Exception {
        assertNotNull(dsaEntry);

        X509Certificate dsaCert = getCertificate(f, CERT_DSA);
        Map<String, Date> dates = getCrlDates(CRL_RSA_DSA_DATES);
        Date expectedDate = dates.get("lastUpdate");

        assertEquals(dsaCert.getSerialNumber(), dsaEntry.getSerialNumber());
        assertDateSlightlyBefore(expectedDate, dsaEntry.getRevocationDate());
        assertEquals(CRLReason.CESSATION_OF_OPERATION, dsaEntry.getRevocationReason());
        assertNull(dsaEntry.getCertificateIssuer());
        assertTrue(dsaEntry.hasExtensions());
        assertNotNull(dsaEntry.getCriticalExtensionOIDs());
        assertEquals(0, dsaEntry.getCriticalExtensionOIDs().size());
        assertNotNull(dsaEntry.getNonCriticalExtensionOIDs());
        assertEquals(1, dsaEntry.getNonCriticalExtensionOIDs().size());
        assertTrue(Arrays.toString(dsaEntry.getNonCriticalExtensionOIDs().toArray()),
                dsaEntry.getNonCriticalExtensionOIDs().contains("2.5.29.21"));
        System.out.println(Arrays.toString(dsaEntry.getExtensionValue("2.5.29.21")));

        assertNotNull(dsaEntry.toString());
    }

    private void getRevokedCertificates(CertificateFactory f) throws Exception {
        X509CRL crlEmpty = getCRL(f, CRL_EMPTY);
        assertNull(crlEmpty.getRevokedCertificates());

        X509CRL crlRsa = getCRL(f, CRL_RSA);
        X509Certificate rsaCert = getCertificate(f, CERT_RSA);
        X509Certificate dsaCert = getCertificate(f, CERT_DSA);

        Set<? extends X509CRLEntry> entries = crlRsa.getRevokedCertificates();
        assertEquals(1, entries.size());
        for (X509CRLEntry e : entries) {
            assertRsaCrlEntry(f, e);
            assertRsaCrlEntry(f, crlRsa.getRevokedCertificate(e.getSerialNumber()));
        }

        X509CRL crlRsaDsa = getCRL(f, CRL_RSA_DSA);
        Set<? extends X509CRLEntry> entries2 = crlRsaDsa.getRevokedCertificates();
        assertEquals(2, entries2.size());
        assertRsaCrlEntry(f, crlRsaDsa.getRevokedCertificate(rsaCert));
        assertRsaCrlEntry(f, crlRsaDsa.getRevokedCertificate(rsaCert.getSerialNumber()));
        assertDsaCrlEntry(f, crlRsaDsa.getRevokedCertificate(dsaCert));
        assertDsaCrlEntry(f, crlRsaDsa.getRevokedCertificate(dsaCert.getSerialNumber()));
    }

    private void getSigAlgParams(CertificateFactory f) throws Exception {
        X509CRL crl1 = getCRL(f, CRL_RSA);
        final byte[] sigAlgParams = crl1.getSigAlgParams();
        if (StandardNames.IS_RI) {
            assertNull(f.getProvider().getName(), sigAlgParams);
        } else {
            assertNotNull(f.getProvider().getName(), sigAlgParams);
            /* ASN.1 NULL */
            final byte[] expected = new byte[] {
                    0x05, 0x00,
            };
            assertEquals(f.getProvider().getName(), Arrays.toString(expected),
                    Arrays.toString(sigAlgParams));
        }

        {
            X509CRL crlSigOpt = getCRL(f, CRL_RSA_DSA_SIGOPT);

            /* SEQUENCE, INTEGER 1 */
            final byte[] expected = new byte[] {
                    /* SEQUENCE, constructed, len=5 */
                    (byte) 0x30, (byte) 0x05,
                    /* Type=2, constructed, context-specific, len=3 */
                    (byte) 0xA2, (byte) 0x03,
                    /* INTEGER, len=1, value=1 */
                    (byte) 0x02, (byte) 0x01, (byte) 0x01,
            };

            final byte[] params = crlSigOpt.getSigAlgParams();
            assertNotNull(f.getProvider().getName(), params);
            assertEquals(Arrays.toString(expected), Arrays.toString(params));
        }
    }

    private void test_toString(CertificateFactory f) throws Exception {
        X509CRL crl1 = getCRL(f, CRL_RSA);
        X509CRL crl2 = getCRL(f, CRL_RSA);

        X509CRL crlRsaDsa = getCRL(f, CRL_RSA_DSA);

        assertNotNull(crl1);

        assertNotNull(crlRsaDsa);

        assertEquals(crl1.toString(), crl2.toString());

        assertFalse(crl1.toString().equals(crlRsaDsa.toString()));
    }

    private void test_equals(CertificateFactory f) throws Exception {
        X509CRL crl1 = getCRL(f, CRL_RSA);
        X509CRL crl2 = getCRL(f, CRL_RSA);
        X509Certificate rsaCert = getCertificate(f, CERT_RSA);

        X509CRL crlRsaDsa = getCRL(f, CRL_RSA_DSA);

        assertEquals(crl1, crl2);
        assertFalse(crl1.equals(crlRsaDsa));

        X509CRLEntry entry1 = crl1.getRevokedCertificate(rsaCert);
        assertNotNull(entry1);
        X509CRLEntry entry2 = crl2.getRevokedCertificate(rsaCert);
        assertNotNull(entry2);

        assertEquals(entry1, entry2);
    }
}
