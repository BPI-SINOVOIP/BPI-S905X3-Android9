/*
 *  Licensed to the Apache Software Foundation (ASF) under one or more
 *  contributor license agreements.  See the NOTICE file distributed with
 *  this work for additional information regarding copyright ownership.
 *  The ASF licenses this file to You under the Apache License, Version 2.0
 *  (the "License"); you may not use this file except in compliance with
 *  the License.  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */

package org.apache.harmony.tests.javax.net.ssl;

import java.io.IOException;
import java.nio.ByteBuffer;
import java.nio.ReadOnlyBufferException;
import java.nio.channels.Pipe;
import java.nio.channels.Pipe.SinkChannel;
import java.nio.channels.Pipe.SourceChannel;
import java.security.KeyManagementException;
import java.security.NoSuchAlgorithmException;
import java.util.Arrays;
import java.util.HashSet;
import java.util.Set;
import javax.net.ssl.SSLContext;
import javax.net.ssl.SSLEngine;
import javax.net.ssl.SSLEngineResult;
import javax.net.ssl.SSLEngineResult.Status;
import javax.net.ssl.SSLException;
import javax.net.ssl.SSLEngineResult.HandshakeStatus;
import junit.framework.TestCase;
import libcore.java.security.StandardNames;
import libcore.javax.net.ssl.TestSSLContext;

/**
 * Tests for SSLEngine class
 *
 */
public class SSLEngineTest extends TestCase {
    private static final int MAX_TLS_RECORD_SIZE = 32768;

    private HandshakeHandler clientEngine;
    private HandshakeHandler serverEngine;

    @Override protected void setUp() throws Exception {
        super.setUp();
    }

    /**
     * Test for <code>SSLEngine()</code> constructor Assertion: creates
     * SSLEngine object with null host and -1 port
     */
    public void test_Constructor() throws Exception {
        SSLEngine e = getEngine();
        assertNull(e.getPeerHost());
        assertEquals(-1, e.getPeerPort());
        String[] suites = e.getSupportedCipherSuites();
        e.setEnabledCipherSuites(suites);
        assertEquals(e.getEnabledCipherSuites().length, suites.length);
    }

    /**
     * Test for <code>SSLEngine(String host, int port)</code> constructor
     */
    public void test_ConstructorLjava_lang_StringI01() throws Exception {
        int port = 1010;
        SSLEngine e = getEngine(null, port);
        assertNull(e.getPeerHost());
        assertEquals(e.getPeerPort(), port);
        try {
            e.beginHandshake();
            fail("should throw IllegalStateException");
        } catch (IllegalStateException expected) {
        }

        e = getEngine(null, port);
        e.setUseClientMode(true);
        e.beginHandshake();

        e = getEngine(null, port);
        e.setUseClientMode(false);
        e.beginHandshake();
    }

    /**
     * Test for <code>SSLEngine(String host, int port)</code> constructor
     */
    public void test_ConstructorLjava_lang_StringI02() throws Exception {
        String host = "new host";
        int port = 8080;
        SSLEngine e = getEngine(host, port);
        assertEquals(e.getPeerHost(), host);
        assertEquals(e.getPeerPort(), port);
        String[] suites = e.getSupportedCipherSuites();
        e.setEnabledCipherSuites(suites);
        assertEquals(e.getEnabledCipherSuites().length, suites.length);
        e.setUseClientMode(true);
        assertTrue(e.getUseClientMode());
    }

    /**
     * Test for <code>getPeerHost()</code> method
     */
    public void test_getPeerHost() throws Exception {
        SSLEngine e = getEngine();
        assertNull(e.getPeerHost());
        e = getEngine("www.fortify.net", 80);
        assertEquals("Incorrect host name", "www.fortify.net", e.getPeerHost());
    }

    /**
     * Test for <code>getPeerPort()</code> method
     */
    public void test_getPeerPort() throws Exception {
        SSLEngine e = getEngine();
        assertEquals("Incorrect default value of peer port",
                -1 ,e.getPeerPort());
        e = getEngine("www.fortify.net", 80);
        assertEquals("Incorrect peer port", 80, e.getPeerPort());
    }

    public void test_getSupportedProtocols() throws Exception {
        SSLEngine sse = getEngine();

        String[] res = sse.getSupportedProtocols();
        assertNotNull(res);
        assertTrue(res.length > 0);
    }

    /**
     * javax.net.ssl.SSLEngine#setEnabledProtocols(String[] protocols)
     * javax.net.ssl.SSLEngine#getEnabledProtocols()
     */
    public void test_EnabledProtocols() throws Exception {
        SSLEngine sse = getEngine();
        String[] pr = sse.getSupportedProtocols();

        sse.setEnabledProtocols(pr);
        String[] res = sse.getEnabledProtocols();
        assertNotNull("Null array was returned", res);
        assertEquals("Incorrect array length", res.length, pr.length);
        assertTrue("Incorrect array was returned", Arrays.equals(res, pr));

        try {
            sse.setEnabledProtocols(null);
            fail("IllegalArgumentException wasn't thrown");
        } catch (IllegalArgumentException expected) {
        }
    }

    /**
     * javax.net.ssl.SSLEngine#getSupportedCipherSuites()
     */
    public void test_getSupportedCipherSuites() throws Exception {
        SSLEngine sse = getEngine();

        String[] res = sse.getSupportedCipherSuites();
        assertNotNull(res);
        assertTrue(res.length > 0);
    }

    /**
     * javax.net.ssl.SSLEngine#setEnabledCipherSuites(String[] suites)
     * javax.net.ssl.SSLEngine#getEnabledCipherSuites()
     */
    public void test_EnabledCipherSuites() throws Exception {
        SSLEngine sse = getEngine();
        String[] st = sse.getSupportedCipherSuites();

        sse.setEnabledCipherSuites(st);
        String[] res = sse.getEnabledCipherSuites();
        assertNotNull("Null array was returned", res);
        assertEquals("Incorrect array length", res.length, st.length);
        assertTrue("Incorrect array was returned", Arrays.equals(res, st));

        try {
            sse.setEnabledCipherSuites(null);
            fail("IllegalArgumentException wasn't thrown");
        } catch (IllegalArgumentException expected) {
        }
    }

    /**
     * javax.net.ssl.SSLEngine#setEnableSessionCreation(boolean flag)
     * javax.net.ssl.SSLEngine#getEnableSessionCreation()
     */
    public void test_EnableSessionCreation() throws Exception {
        SSLEngine sse = getEngine();
        try {
            assertTrue(sse.getEnableSessionCreation());
            sse.setEnableSessionCreation(false);
            assertFalse(sse.getEnableSessionCreation());
            sse.setEnableSessionCreation(true);
            assertTrue(sse.getEnableSessionCreation());
        } catch (Exception ex) {
            fail("Unexpected exception " + ex);
        }
    }

    /**
     * javax.net.ssl.SSLEngine#setNeedClientAuth(boolean need)
     * javax.net.ssl.SSLEngine#getNeedClientAuth()
     */
    public void test_NeedClientAuth() throws Exception {
        SSLEngine sse = getEngine();
        try {
            sse.setNeedClientAuth(false);
            assertFalse(sse.getNeedClientAuth());
            sse.setNeedClientAuth(true);
            assertTrue(sse.getNeedClientAuth());
        } catch (Exception ex) {
            fail("Unexpected exception " + ex);
        }
    }

    /**
     * javax.net.ssl.SSLEngine#setWantClientAuth(boolean want)
     * javax.net.ssl.SSLEngine#getWantClientAuth()
     */
    public void test_WantClientAuth() throws Exception {
        SSLEngine sse = getEngine();
        sse.setWantClientAuth(false);
        assertFalse(sse.getWantClientAuth());
        sse.setWantClientAuth(true);
        assertTrue(sse.getWantClientAuth());
    }

    /**
     * javax.net.ssl.SSLEngine#beginHandshake()
     */
    public void test_beginHandshake() throws Exception {
        SSLEngine sse = getEngine();
        try {
            sse.beginHandshake();
            fail("IllegalStateException wasn't thrown");
        } catch (IllegalStateException expected) {
        }
        sse = getEngine("new host", 1080);
        try {
            sse.beginHandshake();
            fail("IllegalStateException wasn't thrown");
        } catch (IllegalStateException expected) {
        }
        sse = getEngine();
        sse.setUseClientMode(true);
        sse.beginHandshake();
    }

    /**
     * javax.net.ssl.SSLEngine#setUseClientMode(boolean mode)
     * javax.net.ssl.SSLEngine#getUseClientMode()
     */
    public void test_UseClientMode() throws Exception {
        SSLEngine sse = getEngine();
        sse.setUseClientMode(false);
        assertFalse(sse.getUseClientMode());
        sse.setUseClientMode(true);
        assertTrue(sse.getUseClientMode());

        sse = getEngine(null, 1080);
        sse.setUseClientMode(true);
        sse.beginHandshake();
        try {
            sse.setUseClientMode(false);
            fail("IllegalArgumentException was not thrown");
        } catch (IllegalArgumentException expected) {
        }
    }

    /**
     * javax.net.ssl.SSLEngine#getSession()
     */
    public void test_getSession() throws Exception {
        SSLEngine sse = getEngine();
        assertNotNull(sse.getSession());
    }

    /**
     * javax.net.ssl.SSLEngine#getHandshakeStatus()
     */
    public void test_getHandshakeStatus() throws Exception {
        SSLEngine sse = getEngine();
        assertEquals(sse.getHandshakeStatus().toString(), "NOT_HANDSHAKING");
        sse.setUseClientMode(true);
        sse.beginHandshake();
        assertEquals(sse.getHandshakeStatus().toString(), "NEED_WRAP");
    }

    /**
     * javax.net.ssl.SSLEngine#getDelegatedTask()
     */
    public void test_getDelegatedTask() throws Exception {
        SSLEngine sse = getEngine();
        assertNull(sse.getDelegatedTask());
    }

    /**
     * javax.net.ssl.SSLEngine#unwrap(ByteBuffer src, ByteBuffer[] dsts,
     *                                       int offset, int length)
     * Exception case: SSLException should be thrown.
     */
    public void test_unwrap_01() throws Exception {
        prepareEngines();
        doHandshake();

        ByteBuffer bbs = ByteBuffer.wrap(new byte[] {1,2,3,1,2,3,1,2,3,1,2,3,1,2,3,1,2,3,1,2,3,1,2,3,1,2,3,1,2,3,1,2,3,1,2,31,2,3,1,2,3,1,2,3,1,2,3});
        ByteBuffer bbd = ByteBuffer.allocate(clientEngine.engine.getSession().getApplicationBufferSize());
        try {
            clientEngine.engine.unwrap(bbs, new ByteBuffer[] { bbd }, 0, 1);
            fail("SSLException wasn't thrown");
        } catch (SSLException expected) {
        }
    }

    /**
     * javax.net.ssl.SSLEngine#unwrap(ByteBuffer src, ByteBuffer[] dsts,
     *                                       int offset, int length)
     * Exception case: IndexOutOfBoundsException should be thrown.
     */
    public void test_unwrap_02() throws Exception {
        String host = "new host";
        int port = 8080;
        ByteBuffer[] bbA = { ByteBuffer.allocate(100), ByteBuffer.allocate(10), ByteBuffer.allocate(100) };

        ByteBuffer bb = ByteBuffer.allocate(10);
        SSLEngine sse = getEngine(host, port);
        sse.setUseClientMode(true);

        try {
            sse.unwrap(bb, bbA, -1, 3);
            fail("IndexOutOfBoundsException wasn't thrown");
        } catch (IndexOutOfBoundsException expected) {
        }
        try {
            sse.unwrap(bb, bbA, 0, -3);
            fail("IndexOutOfBoundsException wasn't thrown");
        } catch (IndexOutOfBoundsException expected) {
        }
        try {
            sse.unwrap(bb, bbA, bbA.length + 1, bbA.length);
            fail("IndexOutOfBoundsException wasn't thrown");
        } catch (IndexOutOfBoundsException expected) {
        }
        try {
            sse.unwrap(bb, bbA, 0, bbA.length + 1);
            fail("IndexOutOfBoundsException wasn't thrown");
        } catch (IndexOutOfBoundsException expected) {
        }
    }

    /**
     * javax.net.ssl.SSLEngine#unwrap(ByteBuffer src, ByteBuffer[] dsts,
     *                                       int offset, int length)
     * Exception case: ReadOnlyBufferException should be thrown.
     */
    public void test_unwrap_03() throws Exception {
        String host = "new host";
        int port = 8080;
        ByteBuffer bbR = ByteBuffer.allocate(100).asReadOnlyBuffer();
        ByteBuffer[] bbA = { bbR, ByteBuffer.allocate(10), ByteBuffer.allocate(100) };

        ByteBuffer bb = ByteBuffer.allocate(10);
        SSLEngine sse = getEngine(host, port);
        sse.setUseClientMode(true);

        try {
            sse.unwrap(bb, bbA, 0, bbA.length);
            fail("ReadOnlyBufferException wasn't thrown");
        } catch (ReadOnlyBufferException expected) {
        }
    }

    /**
     * javax.net.ssl.SSLEngine#unwrap(ByteBuffer src, ByteBuffer[] dsts,
     *                                       int offset, int length)
     * Exception case: IllegalArgumentException should be thrown.
     */
    public void test_unwrap_04() throws Exception {
        String host = "new host";
        int port = 8080;
        ByteBuffer[] bbA = {ByteBuffer.allocate(100), ByteBuffer.allocate(10), ByteBuffer.allocate(100)};
        ByteBuffer[] bbAN = {ByteBuffer.allocate(100), null, ByteBuffer.allocate(100)};
        ByteBuffer[] bbN = null;
        ByteBuffer bb = ByteBuffer.allocate(10);
        ByteBuffer bN = null;
        SSLEngine sse = getEngine(host, port);
        sse.setUseClientMode(true);

        try {
            sse.unwrap(bN, bbA, 0, 3);
            fail("IllegalArgumentException wasn't thrown");
        } catch (IllegalArgumentException expected) {
        }
        try {
            sse.unwrap(bb, bbAN, 0, 3);
            fail("IllegalArgumentException wasn't thrown");
        } catch (IllegalArgumentException expected) {
        }
        try {
            sse.unwrap(bb, bbN, 0, 0);
            fail("IllegalArgumentException wasn't thrown");
        } catch (IllegalArgumentException expected) {
        }
        try {
            sse.unwrap(bN, bbN, 0, 0);
            fail("IllegalArgumentException wasn't thrown");
        } catch (IllegalArgumentException expected) {
        }
    }

    /**
     * javax.net.ssl.SSLEngine#unwrap(ByteBuffer src, ByteBuffer[] dsts,
     *                                       int offset, int length)
     * Exception case: IllegalStateException should be thrown.
     */
    public void test_unwrap_05() throws Exception {
        String host = "new host";
        int port = 8080;
        ByteBuffer[] bbA = { ByteBuffer.allocate(100), ByteBuffer.allocate(10), ByteBuffer.allocate(100) };

        ByteBuffer bb = ByteBuffer.allocate(10);
        SSLEngine sse = getEngine(host, port);

        try {
            SSLEngineResult result = sse.unwrap(bb, bbA, 0, bbA.length);
            fail("IllegalStateException wasn't thrown");
        } catch (IllegalStateException expected) {
        }
    }

    /**
     * javax.net.ssl.SSLEngine#unwrap(ByteBuffer src, ByteBuffer[] dsts,
     *                                       int offset, int length)
     */
    public void test_unwrap_06() throws Exception {
        String host = "new host";
        int port = 8080;
        ByteBuffer[] bbA = { ByteBuffer.allocate(100), ByteBuffer.allocate(10), ByteBuffer.allocate(100) };

        ByteBuffer bb = ByteBuffer.allocate(10);
        SSLEngine sse = getEngine(host, port);
        sse.setUseClientMode(true);

        SSLEngineResult res = sse.unwrap(bb, bbA, 0, bbA.length);
        assertEquals(0, res.bytesConsumed());
        assertEquals(0, res.bytesProduced());
    }

    public void test_wrap_01() throws Exception {
        prepareEngines();
        doHandshake();
        ByteBuffer bbs = ByteBuffer.allocate(100);
        ByteBuffer bbd = ByteBuffer.allocate(MAX_TLS_RECORD_SIZE);
        clientEngine.engine.wrap(new ByteBuffer[] { bbs }, 0, 1, bbd);
    }

    /**
     * javax.net.ssl.SSLEngine#wrap(ByteBuffer[] srcs, int offset,
     *                                     int length, ByteBuffer dst)
     * Exception case: IndexOutOfBoundsException should be thrown.
     */
    public void test_wrap_02() throws Exception {
        String host = "new host";
        int port = 8080;
        ByteBuffer bb = ByteBuffer.allocate(10);
        ByteBuffer[] bbA = {ByteBuffer.allocate(5), ByteBuffer.allocate(10), ByteBuffer.allocate(5)};
        SSLEngine sse = getEngine(host, port);
        sse.setUseClientMode(true);

        try {
            sse.wrap(bbA, -1, 3, bb);
            fail("IndexOutOfBoundsException wasn't thrown");
        } catch (IndexOutOfBoundsException expected) {
        }
        try {
            sse.wrap(bbA, 0, -3, bb);
            fail("IndexOutOfBoundsException wasn't thrown");
        } catch (IndexOutOfBoundsException expected) {
        }
        try {
            sse.wrap(bbA, bbA.length + 1, bbA.length, bb);
            fail("IndexOutOfBoundsException wasn't thrown");
        } catch (IndexOutOfBoundsException expected) {
        }
        try {
            sse.wrap(bbA, 0, bbA.length + 1, bb);
            fail("IndexOutOfBoundsException wasn't thrown");
        } catch (IndexOutOfBoundsException expected) {
        }
    }

    /**
     * javax.net.ssl.SSLEngine#wrap(ByteBuffer[] srcs, int offset,
     *                                     int length, ByteBuffer dst)
     * Exception case: ReadOnlyBufferException should be thrown.
     */
    public void test_wrap_03() throws Exception {
        String host = "new host";
        int port = 8080;
        ByteBuffer bb = ByteBuffer.allocate(MAX_TLS_RECORD_SIZE).asReadOnlyBuffer();
        ByteBuffer[] bbA = {ByteBuffer.allocate(5), ByteBuffer.allocate(10), ByteBuffer.allocate(5)};
        SSLEngine sse = getEngine(host, port);
        sse.setUseClientMode(true);

        try {
            sse.wrap(bbA, 0, bbA.length, bb);
            fail("ReadOnlyBufferException wasn't thrown");
        } catch (ReadOnlyBufferException expected) {
        }
    }

    /**
     * javax.net.ssl.SSLEngine#wrap(ByteBuffer[] srcs, int offset,
     *                                     int length, ByteBuffer dst)
     * Exception case: IllegalArgumentException should be thrown.
     */
    public void test_wrap_04() throws Exception {
        String host = "new host";
        int port = 8080;
        ByteBuffer[] bbA = {ByteBuffer.allocate(100), ByteBuffer.allocate(10), ByteBuffer.allocate(100)};
        ByteBuffer[] bbN = null;
        ByteBuffer bN = null;
        SSLEngine e = getEngine(host, port);
        e.setUseClientMode(true);

        try {
            e.wrap(bbA, 0, 3, bN);
            fail("IllegalArgumentException must be thrown for null srcs byte buffer array");
        } catch (IllegalArgumentException ex) {
        }

        try {
            e.wrap(bbN, 0, 0, bN);
            fail("IllegalArgumentException wasn't thrown");
        } catch (IllegalArgumentException ex) {
        }
    }

    /**
     * javax.net.ssl.SSLEngine#wrap(ByteBuffer[] srcs, int offset,
     *                                     int length, ByteBuffer dst)
     * Exception case: IllegalStateException should be thrown.
     */
    public void test_wrap_05() throws Exception {
        String host = "new host";
        int port = 8080;
        ByteBuffer bb = ByteBuffer.allocate(MAX_TLS_RECORD_SIZE);
        ByteBuffer[] bbA = {ByteBuffer.allocate(5), ByteBuffer.allocate(10), ByteBuffer.allocate(5)};
        SSLEngine sse = getEngine(host, port);

        try {
            SSLEngineResult result = sse.wrap(bbA, 0, bbA.length, bb);
            fail("Should fail since mode not set yet");
        } catch (IllegalStateException expected) {
        }
    }

    /**
     * javax.net.ssl.SSLEngine#wrap(ByteBuffer[] srcs, int offset,
     *                                     int length, ByteBuffer dst)
     */
    public void test_wrap_06() throws Exception {
        String host = "new host";
        int port = 8080;
        ByteBuffer bb = ByteBuffer.allocate(MAX_TLS_RECORD_SIZE);
        ByteBuffer[] bbA = {ByteBuffer.allocate(5), ByteBuffer.allocate(10), ByteBuffer.allocate(5)};
        SSLEngine sse = getEngine(host, port);
        sse.setUseClientMode(true);

        SSLEngineResult result = sse.wrap(bbA, 0, bbA.length, bb);
        assertEquals(SSLEngineResult.Status.OK, result.getStatus());
        assertEquals(0, result.bytesConsumed());
        assertTrue(result.bytesProduced() > 0);
    }

    /**
     * javax.net.ssl.SSLEngine#closeOutbound()
     * javax.net.ssl.SSLEngine#isOutboundDone()
     */
    public void test_closeOutbound() throws Exception {
        SSLEngine sse = getEngine();

        assertFalse(sse.isOutboundDone());
        sse.closeOutbound();
        assertTrue(sse.isOutboundDone());
    }

    /**
     * javax.net.ssl.SSLEngine#closeInbound()
     * javax.net.ssl.SSLEngine#isInboundDone()
     */
    public void test_closeInbound() throws Exception {
        SSLEngine sse = getEngine();

        assertFalse(sse.isInboundDone());
        sse.closeInbound();
        assertTrue(sse.isInboundDone());
    }

    /**
     * javax.net.ssl.SSLEngine#unwrap(ByteBuffer src, ByteBuffer dst)
     * SSLException should be thrown.
     */
    public void test_unwrap_ByteBuffer_ByteBuffer_01() throws Exception {
        prepareEngines();
        doHandshake();
        ByteBuffer bbs = ByteBuffer.allocate(100);
        ByteBuffer bbd = ByteBuffer.allocate(100);

        try {
            SSLEngineResult unwrap = clientEngine.engine.unwrap(bbs, bbd);
            fail("SSLException wasn't thrown");
        } catch (SSLException ex) {
            //expected
        }
    }

    /**
     * javax.net.ssl.SSLEngine#unwrap(ByteBuffer src, ByteBuffer dst)
     * ReadOnlyBufferException should be thrown.
     */
    public void test_unwrap_ByteBuffer_ByteBuffer_02() throws Exception {
        String host = "new host";
        int port = 8080;
        ByteBuffer bbs = ByteBuffer.allocate(10);
        ByteBuffer bbd = ByteBuffer.allocate(100).asReadOnlyBuffer();
        SSLEngine sse = getEngine(host, port);
        sse.setUseClientMode(true);

        try {
            sse.unwrap(bbs, bbd);
            fail("ReadOnlyBufferException wasn't thrown");
        } catch (ReadOnlyBufferException iobe) {
            //expected
        } catch (Exception e) {
            fail(e + " was thrown instead of ReadOnlyBufferException");
        }
    }

    /**
     * javax.net.ssl.SSLEngine#unwrap(ByteBuffer src, ByteBuffer dst)
     * IllegalArgumentException should be thrown.
     */
    public void test_unwrap_ByteBuffer_ByteBuffer_03() throws Exception {
        String host = "new host";
        int port = 8080;
        ByteBuffer bbsN = null;
        ByteBuffer bbdN = null;
        ByteBuffer bbs = ByteBuffer.allocate(10);
        ByteBuffer bbd = ByteBuffer.allocate(100);
        SSLEngine sse = getEngine(host, port);
        sse.setUseClientMode(true);

        try {
            sse.unwrap(bbsN, bbd);
            fail("IllegalArgumentException wasn't thrown");
        } catch (IllegalArgumentException expected) {
        }

        try {
            sse.unwrap(bbs, bbdN);
            fail("IllegalArgumentException wasn't thrown");
        } catch (IllegalArgumentException expected) {
        }

        try {
            sse.unwrap(bbsN, bbdN);
            fail("IllegalArgumentException wasn't thrown");
        } catch (IllegalArgumentException expected) {
        }
    }

    /**
     * javax.net.ssl.SSLEngine#unwrap(ByteBuffer src, ByteBuffer dst)
     * IllegalStateException should be thrown.
     */
    public void test_unwrap_ByteBuffer_ByteBuffer_04() throws Exception {
        String host = "new host";
        int port = 8080;
        ByteBuffer bbs = ByteBuffer.allocate(10);
        ByteBuffer bbd = ByteBuffer.allocate(100);
        SSLEngine sse = getEngine(host, port);

        try {
            sse.unwrap(bbs, bbd);
            fail("IllegalStateException wasn't thrown");
        } catch (IllegalStateException expected) {
        }
    }

    /**
     * javax.net.ssl.SSLEngine#unwrap(ByteBuffer src, ByteBuffer dst)
     */
    public void test_unwrap_ByteBuffer_ByteBuffer_05() throws Exception {
        String host = "new host";
        int port = 8080;
        ByteBuffer bbs = ByteBuffer.allocate(10);
        ByteBuffer bbd = ByteBuffer.allocate(100);
        SSLEngine sse = getEngine(host, port);
        sse.setUseClientMode(true);

        SSLEngineResult res = sse.unwrap(bbs, bbd);
        assertEquals(0, res.bytesConsumed());
        assertEquals(0, res.bytesProduced());
    }

    /**
     * javax.net.ssl.SSLEngine#unwrap(ByteBuffer src, ByteBuffer[] dsts)
     * SSLException should be thrown.
     */
    public void test_unwrap_ByteBuffer$ByteBuffer_01() throws Exception {
        prepareEngines();
        doHandshake();

        ByteBuffer bbs = ByteBuffer.allocate(100);
        ByteBuffer bbd = ByteBuffer.allocate(100);

        try {
            clientEngine.engine.unwrap(bbs, new ByteBuffer[] { bbd });
            fail("SSLException wasn't thrown");
        } catch (SSLException expected) {
        }
    }

    /**
     * javax.net.ssl.SSLEngine#unwrap(ByteBuffer src, ByteBuffer[] dsts)
     * ReadOnlyBufferException should be thrown.
     */
    public void test_unwrap_ByteBuffer$ByteBuffer_02() throws Exception {
        String host = "new host";
        int port = 8080;
        ByteBuffer bbs = ByteBuffer.allocate(10);
        ByteBuffer bbR = ByteBuffer.allocate(100).asReadOnlyBuffer();
        ByteBuffer[] bbA = { bbR, ByteBuffer.allocate(10), ByteBuffer.allocate(100) };
        SSLEngine sse = getEngine(host, port);
        sse.setUseClientMode(true);

        try {
            sse.unwrap(bbs, bbA);
            fail("ReadOnlyBufferException wasn't thrown");
        } catch (ReadOnlyBufferException expected) {
        }
    }

    /**
     * javax.net.ssl.SSLEngine#unwrap(ByteBuffer src, ByteBuffer[] dsts)
     * IllegalArgumentException should be thrown.
     */
    public void test_unwrap_ByteBuffer$ByteBuffer_03() throws Exception {
        String host = "new host";
        int port = 8080;
        ByteBuffer[] bbA = { ByteBuffer.allocate(100), ByteBuffer.allocate(10), ByteBuffer.allocate(100) };
        ByteBuffer[] bbN = { ByteBuffer.allocate(100), null, ByteBuffer.allocate(100) };
        ByteBuffer[] bbAN = null;
        ByteBuffer bb = ByteBuffer.allocate(10);
        ByteBuffer bN = null;
        SSLEngine sse = getEngine(host, port);
        sse.setUseClientMode(true);

        try {
            sse.unwrap(bN, bbA);
            fail("IllegalArgumentException wasn't thrown");
        } catch (IllegalArgumentException expected) {
        }

        try {
            sse.unwrap(bb, bbAN);
            fail("IllegalArgumentException wasn't thrown");
        } catch (IllegalArgumentException expected) {
        }

        try {
            sse.unwrap(bb, bbN);
            fail("IllegalArgumentException wasn't thrown");
        } catch (IllegalArgumentException expected) {
        }

        try {
            sse.unwrap(bN, bbAN);
            fail("IllegalArgumentException wasn't thrown");
        } catch (IllegalArgumentException expected) {
        }
    }

    /**
     * javax.net.ssl.SSLEngine#unwrap(ByteBuffer src, ByteBuffer[] dsts)
     * IllegalStateException should be thrown.
     */
    public void test_unwrap_ByteBuffer$ByteBuffer_04() throws Exception {
        String host = "new host";
        int port = 8080;
        ByteBuffer bbs = ByteBuffer.allocate(10);
        ByteBuffer[] bbd = {ByteBuffer.allocate(100), ByteBuffer.allocate(10), ByteBuffer.allocate(100) };
        SSLEngine sse = getEngine(host, port);

        try {
            sse.unwrap(bbs, bbd);
            fail("IllegalStateException wasn't thrown");
        } catch (IllegalStateException expected) {
        }
    }

    /**
     * javax.net.ssl.SSLEngine#unwrap(ByteBuffer src, ByteBuffer[] dsts)
     */
    public void test_unwrap_ByteBuffer$ByteBuffer_05() throws Exception {
        String host = "new host";
        int port = 8080;
        ByteBuffer bbs = ByteBuffer.allocate(10);
        ByteBuffer[] bbd = {ByteBuffer.allocate(100), ByteBuffer.allocate(10), ByteBuffer.allocate(100) };
        SSLEngine sse = getEngine(host, port);
        sse.setUseClientMode(true);

        SSLEngineResult res = sse.unwrap(bbs, bbd);
        assertEquals(0, res.bytesConsumed());
        assertEquals(0, res.bytesProduced());
    }

    public void test_wrap_ByteBuffer_ByteBuffer_01() throws Exception {
        prepareEngines();
        doHandshake();
        ByteBuffer bbs = ByteBuffer.allocate(20);
        ByteBuffer bbd = ByteBuffer.allocate(20000);
        clientEngine.engine.wrap(bbs, bbd);
    }

    /**
     * javax.net.ssl.SSLEngine#wrap(ByteBuffer src, ByteBuffer dst)
     * ReadOnlyBufferException should be thrown.
     */
    public void test_wrap_ByteBuffer_ByteBuffer_02() throws Exception {
        String host = "new host";
        int port = 8080;
        ByteBuffer bbs = ByteBuffer.allocate(10);
        ByteBuffer bbd = ByteBuffer.allocate(100).asReadOnlyBuffer();
        SSLEngine sse = getEngine(host, port);
        sse.setUseClientMode(true);

        try {
            sse.wrap(bbs, bbd);
            fail("ReadOnlyBufferException wasn't thrown");
        } catch (ReadOnlyBufferException expected) {
        }
    }

    /**
     * javax.net.ssl.SSLEngine#wrap(ByteBuffer src, ByteBuffer dst)
     * IllegalArgumentException should be thrown.
     */
    public void test_wrap_ByteBuffer_ByteBuffer_03() throws Exception {
        String host = "new host";
        int port = 8080;
        ByteBuffer bbsN = null;
        ByteBuffer bbdN = null;
        ByteBuffer bbs = ByteBuffer.allocate(10);
        ByteBuffer bbd = ByteBuffer.allocate(100);
        SSLEngine sse = getEngine(host, port);
        sse.setUseClientMode(true);

        try {
            sse.wrap(bbsN, bbd);
            fail("IllegalArgumentException wasn't thrown");
        } catch (IllegalArgumentException expected) {
        }

        try {
            sse.wrap(bbs, bbdN);
            fail("IllegalArgumentException wasn't thrown");
        } catch (IllegalArgumentException expected) {
        }

        try {
            sse.wrap(bbsN, bbdN);
            fail("IllegalArgumentException wasn't thrown");
        } catch (IllegalArgumentException expected) {
        }
    }

    /**
     * javax.net.ssl.SSLEngine#wrap(ByteBuffer src, ByteBuffer dst)
     * IllegalStateException should be thrown.
     */
    public void test_wrap_ByteBuffer_ByteBuffer_04() throws Exception {
        String host = "new host";
        int port = 8080;
        ByteBuffer bbs = ByteBuffer.allocate(10);
        ByteBuffer bbd = ByteBuffer.allocate(10);
        SSLEngine sse = getEngine(host, port);

        try {
            SSLEngineResult result = sse.wrap(bbs, bbd);
            fail();
        } catch (IllegalStateException expected) {
        }
    }

    /**
     * javax.net.ssl.SSLEngine#wrap(ByteBuffer src, ByteBuffer dst)
     */
    public void test_wrap_ByteBuffer_ByteBuffer_05() throws Exception {
        String host = "new host";
        int port = 8080;
        ByteBuffer bb = ByteBuffer.allocate(10);
        SSLEngine sse = getEngine(host, port);
        sse.setUseClientMode(true);

        SSLEngineResult res = sse.wrap(bb, ByteBuffer.allocate(10));
        assertEquals(Status.BUFFER_OVERFLOW, res.getStatus());
        assertEquals(0, res.bytesConsumed());
        assertEquals(0, res.bytesProduced());
    }

    /**
     * javax.net.ssl.SSLEngine#wrap(ByteBuffer[] srcs, ByteBuffer dst)
     * SSLException should be thrown.
     */
    public void test_wrap_ByteBuffer$ByteBuffer_01() throws Exception {
        prepareEngines();
        doHandshake();
        ByteBuffer bbs = ByteBuffer.allocate(100);
        ByteBuffer bbd = ByteBuffer.allocate(20000);

        clientEngine.engine.wrap(new ByteBuffer[] { bbs }, bbd);
        serverEngine.engine.wrap(new ByteBuffer[] { bbs }, bbd);
    }

    /**
     * javax.net.ssl.SSLEngine#wrap(ByteBuffer[] srcs, ByteBuffer dst)
     * ReadOnlyBufferException should be thrown.
     */
    public void test_wrap_ByteBuffer$ByteBuffer_02() throws Exception {
        String host = "new host";
        int port = 8080;
        ByteBuffer bb = ByteBuffer.allocate(10).asReadOnlyBuffer();
        ByteBuffer[] bbA = {ByteBuffer.allocate(5), ByteBuffer.allocate(10), ByteBuffer.allocate(5)};
        SSLEngine sse = getEngine(host, port);
        sse.setUseClientMode(true);

        try {
            sse.wrap(bbA, bb);
            fail("ReadOnlyBufferException wasn't thrown");
        } catch (ReadOnlyBufferException expected) {
        }
    }

    /**
     * javax.net.ssl.SSLEngine#wrap(ByteBuffer[] srcs, ByteBuffer dst)
     * IllegalArgumentException should be thrown.
     */
    public void test_wrap_ByteBuffer$ByteBuffer_03() throws Exception {
        String host = "new host";
        int port = 8080;
        ByteBuffer[] bbA = {ByteBuffer.allocate(100), ByteBuffer.allocate(10), ByteBuffer.allocate(100)};
        ByteBuffer[] bbAN = null;
        ByteBuffer bb = ByteBuffer.allocate(10);
        ByteBuffer bN = null;
        SSLEngine sse = getEngine(host, port);
        sse.setUseClientMode(true);

        try {
            sse.wrap(bbA, bN);
            fail("IllegalArgumentException wasn't thrown");
        } catch (IllegalArgumentException expected) {
        }

        try {
            sse.wrap(bbAN, bb);
            fail("IllegalArgumentException wasn't thrown");
        } catch (IllegalArgumentException expected) {
        }

        try {
            sse.wrap(bbAN, bN);
            fail("IllegalArgumentException wasn't thrown");
        } catch (IllegalArgumentException expected) {
        }
    }

    /**
     * javax.net.ssl.SSLEngine#wrap(ByteBuffer[] srcs, ByteBuffer dst)
     * IllegalStateException should be thrown.
     */
    public void test_wrap_ByteBuffer$ByteBuffer_04() throws Exception {
        String host = "new host";
        int port = 8080;
        ByteBuffer bb = ByteBuffer.allocate(10);
        ByteBuffer[] bbA = { ByteBuffer.allocate(5), ByteBuffer.allocate(10), ByteBuffer.allocate(5) };
        SSLEngine sse = getEngine(host, port);

        try {
            SSLEngineResult result = sse.wrap(bbA, bb);
            fail();
        } catch (IllegalStateException expected) {
        }
    }

    /**
     * javax.net.ssl.SSLEngine#wrap(ByteBuffer[] srcs, ByteBuffer dst)
     */
    public void test_wrap_ByteBuffer$ByteBuffer_05() throws Exception {
        String host = "new host";
        int port = 8080;
        ByteBuffer bb = ByteBuffer.allocate(2000);
        ByteBuffer[] bbA = { ByteBuffer.allocate(5), ByteBuffer.allocate(10), ByteBuffer.allocate(5) };
        SSLEngine sse = getEngine(host, port);
        sse.setUseClientMode(true);

        SSLEngineResult res = sse.wrap(bbA, bb);
        assertEquals(0, res.bytesConsumed());
        if (res.bytesProduced() == 0) {
            assertEquals(HandshakeStatus.NEED_WRAP, res.getHandshakeStatus());
        } else {
            assertEquals(HandshakeStatus.NEED_UNWRAP, res.getHandshakeStatus());
        }
    }

    private SSLEngine getEngine() throws Exception {
        SSLContext context = SSLContext.getInstance("TLS");
        context.init(null, null, null);
        return context.createSSLEngine();
    }

    private SSLEngine getEngine(String host, int port) throws Exception {
        SSLContext context = SSLContext.getInstance("TLS");
        context.init(null, null, null);
        return context.createSSLEngine(host, port);
    }

    class HandshakeHandler implements Runnable {

        private final SSLEngine engine;

        private final SourceChannel in;

        private final SinkChannel out;

        private final ByteBuffer EMPTY = ByteBuffer.allocate(0);

        @SuppressWarnings("unused")
        private final String LOGTAG;

        private SSLEngineResult.HandshakeStatus status;

        private ByteBuffer readBuffer;

        private ByteBuffer writeBuffer;

        HandshakeHandler(SSLContext context, boolean clientMode, SourceChannel in, SinkChannel out)
                throws Exception {
            this.in = in;
            this.out = out;
            engine = context.createSSLEngine();
            engine.setUseClientMode(clientMode);
            engine.beginHandshake();
            status = engine.getHandshakeStatus();

            if (clientMode) {
                LOGTAG = "CLIENT: ";
            } else {
                LOGTAG = "SERVER: ";
            }

            log("CipherSuites: " + Arrays.toString(engine.getEnabledCipherSuites()));
            log(status);

            readBuffer = ByteBuffer.allocate(200000);
            writeBuffer = ByteBuffer.allocate(20000);
        }

        public SSLEngineResult.HandshakeStatus getStatus() {
            return status;
        }

        private void log(Object o) {
            //System.out.print(LOGTAG);
            //System.out.println(o);
        }

        private ByteBuffer read() throws IOException {
            if (readBuffer == null || readBuffer.remaining() == 0 || readBuffer.position() == 0) {
                readBuffer.clear();
                int read = in.read(readBuffer);
                log("read: " + read);
                readBuffer.rewind();
                readBuffer.limit(read);
            }
            return readBuffer;
        }

        public void run() {
            try {
                while (true) {
                    switch (status) {
                        case FINISHED: {
                            log(status);
                            return;
                        }
                        case NEED_TASK: {
                            log(status);
                            Runnable task;
                            while ((task = engine.getDelegatedTask()) != null) {
                                task.run();
                            }
                            status = engine.getHandshakeStatus();
                            break;
                        }
                        case NEED_UNWRAP: {
                            log(status);
                            ByteBuffer source = read();
                            writeBuffer.clear();

                            while (status == HandshakeStatus.NEED_UNWRAP) {
                                SSLEngineResult result = engine.unwrap(source, writeBuffer);
                                status = result.getHandshakeStatus();
                                log(result);
                            }
                            break;
                        }
                        case NEED_WRAP: {
                            log(status);
                            writeBuffer.clear();

                            int produced = 0;
                            SSLEngineResult result = null;
                            while (status == HandshakeStatus.NEED_WRAP) {
                                result = engine.wrap(EMPTY, writeBuffer);
                                status = result.getHandshakeStatus();
                                produced += result.bytesProduced();
                                log(result);
                            }
                            writeBuffer.rewind();
                            writeBuffer.limit(produced);
                            log("write: " + produced);
                            out.write(writeBuffer);
                            break;
                        }
                        case NOT_HANDSHAKING: {
                            log("Not Handshaking");
                            return;
                        }
                    }
                }
            } catch (IOException e) {
                log(e);
            } catch (RuntimeException e) {
                // ignore;
            }
        }
    }

    public void testHandshake() throws Exception {

        prepareEngines();

        assertTrue("handshake failed", doHandshake());

        System.out.println(clientEngine.engine.getSession().getCipherSuite());

        assertEquals("Handshake not finished",
                SSLEngineResult.HandshakeStatus.FINISHED,
                clientEngine.getStatus());
        assertEquals("Handshake not finished",
                SSLEngineResult.HandshakeStatus.FINISHED,
                serverEngine.getStatus());
    }

    void prepareEngines() throws Exception {
        Pipe clientSendPipe = Pipe.open();
        Pipe serverSendPipe = Pipe.open();

        SinkChannel clientSink = clientSendPipe.sink();
        SourceChannel serverSource = clientSendPipe.source();
        SinkChannel serverSink = serverSendPipe.sink();
        SourceChannel clientSource = serverSendPipe.source();

        TestSSLContext context = TestSSLContext.create();
        clientEngine = new HandshakeHandler(context.clientContext, true, clientSource, clientSink);
        serverEngine = new HandshakeHandler(context.serverContext, false, serverSource, serverSink);
    }

    boolean doHandshake() throws InterruptedException {
        Thread clientThread = new Thread(clientEngine);
        clientThread.start();

        Thread serverThread = new Thread(serverEngine);
        serverThread.start();

        int i = 0;
        while (clientThread.isAlive() && serverThread.isAlive() && i < 20) {
            Thread.sleep(500);
            i++;
        }

        if (clientThread.isAlive()) {
            clientThread.interrupt();
        }

        if (serverThread.isAlive()) {
            serverThread.interrupt();
        }

        return clientEngine.getStatus() == HandshakeStatus.FINISHED && serverEngine.getStatus() == HandshakeStatus.FINISHED;
    }

}
