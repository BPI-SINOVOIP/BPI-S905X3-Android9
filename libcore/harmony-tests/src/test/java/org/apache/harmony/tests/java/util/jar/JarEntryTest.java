/* 
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The ASF licenses this file to You under the Apache License, Version 2.0
 * (the "License"); you may not use this file except in compliance with
 * the License.  You may obtain a copy of the License at
 * 
 *     http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package org.apache.harmony.tests.java.util.jar;

import java.io.File;
import java.io.IOException;
import java.io.InputStream;
import java.security.CodeSigner;
import java.util.List;
import java.util.jar.JarEntry;
import java.util.jar.JarFile;
import java.util.zip.ZipEntry;
import junit.framework.TestCase;
import libcore.io.Streams;
import tests.support.resource.Support_Resources;

public class JarEntryTest extends TestCase {
    private ZipEntry zipEntry;

    private JarEntry jarEntry;

    private JarFile jarFile;

    private final String jarName = "hyts_patch.jar";

    private final String entryName = "foo/bar/A.class";

    private final String entryName2 = "Blah.txt";

    private final String attJarName = "hyts_att.jar";

    private final String attEntryName = "HasAttributes.txt";

    private final String attEntryName2 = "NoAttributes.txt";

    private File resources;

    @Override
    protected void setUp() throws Exception {
        resources = Support_Resources.createTempFolder();
        Support_Resources.copyFile(resources, null, jarName);
        jarFile = new JarFile(new File(resources, jarName));
    }

    @Override
    protected void tearDown() throws Exception {
        if (jarFile != null) {
            jarFile.close();
        }
    }

    /**
     * @throws IOException
     * java.util.jar.JarEntry#JarEntry(java.util.jar.JarEntry)
     */
    public void test_ConstructorLjava_util_jar_JarEntry() throws IOException {
        JarEntry newJarEntry = new JarEntry(jarFile.getJarEntry(entryName));
        assertNotNull(newJarEntry);

        jarEntry = null;
        try {
            newJarEntry = new JarEntry(jarEntry);
            fail("Should throw NullPointerException");
        } catch (NullPointerException e) {
            // Expected
        }
    }

    public void test_ConstructorLjava_util_zip_ZipEntry() {
        assertNotNull("Jar file is null", jarFile);
        zipEntry = jarFile.getEntry(entryName);
        assertNotNull("Zip entry is null", zipEntry);
        jarEntry = new JarEntry(zipEntry);
        assertNotNull("Jar entry is null", jarEntry);
        assertEquals("Wrong entry constructed--wrong name", entryName, jarEntry
                .getName());
        assertEquals("Wrong entry constructed--wrong size", 311, jarEntry
                .getSize());
    }

    /**
     * java.util.jar.JarEntry#getAttributes()
     */
    public void test_getAttributes() throws Exception {
        JarFile attrJar = null;
        File file = null;

        Support_Resources.copyFile(resources, null, attJarName);
        file = new File(resources, attJarName);
        attrJar = new JarFile(file);

        jarEntry = attrJar.getJarEntry(attEntryName);
        assertNotNull("Should have Manifest attributes", jarEntry
                .getAttributes());

        jarEntry = attrJar.getJarEntry(attEntryName2);
        assertNull("Shouldn't have any Manifest attributes", jarEntry
                .getAttributes());
        attrJar.close();
    }

    // http://b/1864326
    public void testCertificatesAndCodesigners() throws Exception {
        zipEntry = jarFile.getEntry(entryName2);
        jarEntry = new JarEntry(zipEntry);
        assertNull(jarEntry.getCertificates());

        // Regression Test for HARMONY-3424, b/1864326
        String jarFileName = "TestCodeSigners.jar";
        Support_Resources.copyFile(resources, null, jarFileName);
        File file = new File(resources, jarFileName);
        JarFile jarFile = new JarFile(file);
        JarEntry jarEntry1 = jarFile.getJarEntry("Test.class");
        JarEntry jarEntry2 = jarFile.getJarEntry("Test.class");

        try (InputStream in = jarFile.getInputStream(jarEntry1)) {
            // Code signers and certs must be {@code null} until the entry is completely
            // read.
            assertNull(jarEntry1.getCertificates());
            assertNull(jarEntry2.getCertificates());
            assertNull(jarEntry1.getCodeSigners());
            assertNull(jarEntry2.getCodeSigners());

            // Read a few bytes from the stream.
            in.read(new byte[64]);
            assertNull(jarEntry1.getCertificates());
            assertNull(jarEntry2.getCertificates());
            assertNull(jarEntry1.getCodeSigners());
            assertNull(jarEntry2.getCodeSigners());

            // Read the rest of the stream.
            Streams.skipByReading(in, Long.MAX_VALUE);

            assertEquals(-1, in.read());
            assertNotNull(jarEntry1.getCodeSigners());
            assertNotNull(jarEntry2.getCodeSigners());
            assertNotNull(jarEntry1.getCertificates());
            assertNotNull(jarEntry2.getCertificates());
        }
    }

    public void test_getCodeSigners() throws IOException {
        String jarFileName = "TestCodeSigners.jar";
        Support_Resources.copyFile(resources, null, jarFileName);
        File file = new File(resources, jarFileName);
        JarFile jarFile = new JarFile(file);
        JarEntry jarEntry = jarFile.getJarEntry("Test.class");
        InputStream in = jarFile.getInputStream(jarEntry);
        byte[] buffer = new byte[1024];
        while (in.available() > 0) {
            assertNull("getCodeSigners() should be null until the entry is read",
                    jarEntry.getCodeSigners());
            in.read(buffer);
        }
        assertEquals("the file is fully read", -1, in.read());
        CodeSigner[] codeSigners = jarEntry.getCodeSigners();
        assertEquals(2, codeSigners.length);
        List<?> certs_bob = codeSigners[0].getSignerCertPath()
                .getCertificates();
        List<?> certs_alice = codeSigners[1].getSignerCertPath()
                .getCertificates();
        if (1 == certs_bob.size()) {
            List<?> temp = certs_bob;
            certs_bob = certs_alice;
            certs_alice = temp;
        }
        assertEquals(2, certs_bob.size());
        assertEquals(1, certs_alice.size());
        assertNull(
                "getCodeSigners() should be null for a primitive JarEntry",
                new JarEntry("aaa").getCodeSigners());
    }
}
