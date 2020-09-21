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

package org.apache.harmony.tests.java.net;

import java.io.FileDescriptor;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.InetAddress;
import java.net.SocketAddress;
import java.net.SocketException;
import java.net.SocketImpl;
import libcore.junit.junit3.TestCaseWithRules;
import libcore.junit.util.ResourceLeakageDetector;
import org.junit.Rule;
import org.junit.rules.TestRule;

public class SocketImplTest extends TestCaseWithRules {
    @Rule
    public TestRule guardRule = ResourceLeakageDetector.getRule();

    /**
     * java.net.SocketImpl#SocketImpl()
     */
    public void test_Constructor_fd() {
        // Regression test for HARMONY-1117
        MockSocketImpl mockSocketImpl = new MockSocketImpl();
        assertNull(mockSocketImpl.getFileDescriptor());
    }

    /**
     * java.net.SocketImpl#setPerformancePreference()
     */
    public void test_setPerformancePreference_Int_Int_Int() {
        MockSocketImpl theSocket = new MockSocketImpl();
        theSocket.setPerformancePreference(1, 1, 1);
    }

    /**
     * java.net.SocketImpl#shutdownOutput()
     */
    public void test_shutdownOutput() {
        MockSocketImpl s = new MockSocketImpl();
        try {
            s.shutdownOutput();
            fail("This method is still not implemented yet,It should throw IOException.");
        } catch (IOException e) {
            // expected
        }
    }

    /**
     * java.net.SocketImpl#shutdownInput()
     */
    public void test_shutdownInput() {
        MockSocketImpl s = new MockSocketImpl();
        try {
            s.shutdownInput();
            fail("This method is still not implemented yet,It should throw IOException.");
        } catch (IOException e) {
            // Expected
        }
    }

    /**
     * java.net.SocketImpl#supportsUrgentData()
     */
    public void test_supportsUrgentData() {
        MockSocketImpl s = new MockSocketImpl();
        assertFalse(s.testSupportsUrgentData());
    }

    // the mock class for test, leave all methods empty
    class MockSocketImpl extends SocketImpl {

        protected void accept(SocketImpl newSocket) throws IOException {
        }

        protected int available() throws IOException {
            return 0;
        }

        protected void bind(InetAddress address, int port) throws IOException {
        }

        protected void close() throws IOException {
        }

        protected void connect(String host, int port) throws IOException {
        }

        protected void connect(InetAddress address, int port)
                throws IOException {
        }

        protected void create(boolean isStreaming) throws IOException {
        }

        protected InputStream getInputStream() throws IOException {
            return null;
        }

        public Object getOption(int optID) throws SocketException {
            return null;
        }

        protected OutputStream getOutputStream() throws IOException {
            return null;
        }

        protected void listen(int backlog) throws IOException {
        }

        public void setOption(int optID, Object val) throws SocketException {
        }

        protected void connect(SocketAddress remoteAddr, int timeout)
                throws IOException {
        }

        protected void sendUrgentData(int value) throws IOException {
        }

        public void setPerformancePreference(int connectionTime, int latency,
                int bandwidth) {
            super.setPerformancePreferences(connectionTime, latency, bandwidth);
        }

        public FileDescriptor getFileDescriptor() {
            return super.getFileDescriptor();
        }

        public void shutdownOutput() throws IOException {
            super.shutdownOutput();
        }

        public void shutdownInput() throws IOException {
            super.shutdownInput();
        }

        public boolean testSupportsUrgentData() {
            return super.supportsUrgentData();
        }

    }
}
