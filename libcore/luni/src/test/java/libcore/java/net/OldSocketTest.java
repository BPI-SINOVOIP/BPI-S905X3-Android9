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

package libcore.java.net;

import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.BindException;
import java.net.ConnectException;
import java.net.Inet4Address;
import java.net.Inet6Address;
import java.net.InetAddress;
import java.net.InetSocketAddress;
import java.net.Proxy;
import java.net.ServerSocket;
import java.net.Socket;
import java.net.SocketAddress;
import java.net.SocketException;
import java.net.SocketImpl;
import java.net.SocketTimeoutException;
import java.net.UnknownHostException;
import java.nio.channels.IllegalBlockingModeException;
import java.nio.channels.SocketChannel;
import java.security.Permission;
import libcore.junit.util.ResourceLeakageDetector.DisableResourceLeakageDetection;
import tests.support.Support_Configuration;

public class OldSocketTest extends OldSocketTestCase {

    private static final InetSocketAddress UNREACHABLE_ADDRESS
            = new InetSocketAddress("192.0.2.0", 0); // RFC 5737

    ServerSocket ss;

    Socket s;

    Thread t;

    SecurityManager sm = new SecurityManager() {

        public void checkPermission(Permission perm) {}

        public void checkConnect(String host, int port) {
            throw new SecurityException();
        }
    };

    public void test_Constructor() {
        // create the socket and then validate some basic state
        s = new Socket();
        assertFalse("new socket should not be connected", s.isConnected());
        assertFalse("new socket should not be bound", s.isBound());
        assertFalse("new socket should not be closed", s.isClosed());
        assertFalse("new socket should not be in InputShutdown", s
                .isInputShutdown());
        assertFalse("new socket should not be in OutputShutdown", s
                .isOutputShutdown());

    }

    public void test_ConstructorLjava_lang_StringI() throws IOException {
        // Test for method java.net.Socket(java.lang.String, int)
        int sport = startServer("Cons String,I");
        s = new Socket(InetAddress.getLocalHost().getHostName(), sport);
        assertTrue("Failed to create socket", s.getPort() == sport);

        //regression for HARMONY-946
        ServerSocket ss = null;
        Socket s = null;
        try {
            ss = new ServerSocket(0);
            s = new Socket("0.0.0.0", ss.getLocalPort());
        } finally {
            try {
                ss.close();
            } catch(Exception e) {
                //ignore
            }
            try {
                s.close();
            } catch(Exception e) {
                //ignore
            }
        }

        try {
            new Socket("unknown.host.google.com", 0);
            fail("UnknownHostException was not thrown.");
        } catch (UnknownHostException expected) {
        }
        Socket socket = null;
        try {
            socket = new Socket(InetAddress.getByName(null), sport);
            InetAddress address = socket.getLocalAddress();
            assertTrue(address.isLoopbackAddress());
        } finally {
            try {
                socket.close();
            } catch(Exception e) {}
        }
    }

    public void test_ConstructorLjava_lang_StringILjava_net_InetAddressI1() throws IOException {
        int sport = startServer("Cons String,I,InetAddress,I");
        s = new Socket(InetAddress.getLocalHost().getHostName(), sport,
                InetAddress.getLocalHost(), 0);
        assertTrue("Failed to create socket", s.getPort() == sport);
    }

    public void test_ConstructorLjava_lang_StringILjava_net_InetAddressI2() throws IOException {
        int sport = startServer("Cons String,I,InetAddress,I");
        Socket s1 = new Socket(InetAddress.getLocalHost(), sport, null, 0);
        try {
            Socket s2 = new Socket(InetAddress.getLocalHost(), sport, null, s1.getLocalPort());
            try {
                s2.close();
            } catch (IOException ignored) {
            }
            fail("second connect should have failed with EADDRINUSE");
        } catch (BindException expected) {
            // success!
        } finally {
            try {
                s1.close();
            } catch (IOException ignored) {
            }
        }
    }

    public void test_ConstructorLjava_lang_StringIZ() throws IOException {
        // Test for method java.net.Socket(java.lang.String, int, boolean)
        int sport = startServer("Cons String,I,Z");
        try (Socket s = new Socket(InetAddress.getLocalHost().getHostName(), sport, true)) {
            assertTrue("Failed to create socket", s.getPort() == sport);
        }

        s = new Socket(InetAddress.getLocalHost().getHostName(), sport, false);
    }

    public void test_ConstructorLjava_net_InetAddressI() throws IOException {
        // Test for method java.net.Socket(java.net.InetAddress, int)
        int sport = startServer("Cons InetAddress,I");
        s = new Socket(InetAddress.getLocalHost(), sport);
        assertTrue("Failed to create socket", s.getPort() == sport);
    }

    public void test_ConstructorLjava_net_InetAddressILjava_net_InetAddressI()
            throws IOException {
        // Test for method java.net.Socket(java.net.InetAddress, int,
        // java.net.InetAddress, int)
        int sport = startServer("Cons InetAddress,I,InetAddress,I");
        s = new Socket(InetAddress.getLocalHost().getHostName(), sport,
                InetAddress.getLocalHost(), 0);
    }

    public void test_ConstructorLjava_net_InetAddressIZ() throws IOException {
        // Test for method java.net.Socket(java.net.InetAddress, int, boolean)
        int sport = startServer("Cons InetAddress,I,Z");
        try (Socket s = new Socket(InetAddress.getLocalHost(), sport, true)) {
            assertTrue("Failed to create socket", s.getPort() == sport);
        }

        s = new Socket(InetAddress.getLocalHost(), sport, false);
    }

    public void test_close() throws IOException {
        // Test for method void java.net.Socket.close()
        int sport = startServer("SServer close");
        s = new Socket(InetAddress.getLocalHost(), sport, null, 0);
        try {
            s.setSoLinger(false, 100);
        } catch (IOException e) {
            handleException(e, SO_LINGER);
        }
        s.close();
        try {
            s.getOutputStream();
            fail("IOException was not thrown.");
        } catch (java.io.IOException e) {
            //expected
        }
    }

    public void test_getInetAddress() throws IOException {
        // Test for method java.net.InetAddress java.net.Socket.getInetAddress()
        int sport = startServer("SServer getInetAddress");
        s = new Socket(InetAddress.getLocalHost(), sport, null, 0);
        assertTrue("Returned incorrect InetAddress", s.getInetAddress().equals(
                InetAddress.getLocalHost()));

    }

    public void test_getInputStream() throws IOException {
        // Simple fetch test
        ServerSocket server = new ServerSocket(0);
        Socket client = new Socket(InetAddress.getLocalHost(), server.getLocalPort());
        InputStream is = client.getInputStream();
        assertNotNull("Failed to get stream", is);
        is.close();
        client.close();
        server.close();
    }

    public void test_getKeepAlive() {
        try {
            int sport = startServer("SServer getKeepAlive");
            Socket theSocket = new Socket(InetAddress.getLocalHost(), sport, null, 0);
            theSocket.setKeepAlive(true);
            assertTrue("getKeepAlive false when it should be true", theSocket
                    .getKeepAlive());
            theSocket.setKeepAlive(false);
            assertFalse("getKeepAlive true when it should be False", theSocket
                    .getKeepAlive());
            theSocket.close();
            try {
                theSocket.setKeepAlive(false);
                fail("IOException was not thrown after calling setKeepAlive " +
                        "method.");
            } catch(IOException ioe) {
                //expected
            }
            try {
                theSocket.getKeepAlive();
                fail("IOException was not thrown after calling getKeepAlive +" +
                        "method.");
            } catch(IOException ioe) {
                //expected
            }
            ensureExceptionThrownIfOptionIsUnsupportedOnOS(SO_KEEPALIVE);
        } catch (Exception e) {
            handleException(e, SO_KEEPALIVE);
        }
    }

    public void test_getLocalAddress() throws IOException {
        // Test for method java.net.InetAddress
        // java.net.Socket.getLocalAddress()
        int sport = startServer("SServer getLocAddress");
        try (Socket s = new Socket(InetAddress.getLocalHost(), sport, null, 0)) {
            assertEquals("Returned incorrect InetAddress",
                    InetAddress.getLocalHost(), s.getLocalAddress());
        }

        // now check behavior when the ANY address is returned
        try (Socket s = new Socket()) {
            s.bind(new InetSocketAddress(InetAddress.getByName("0.0.0.0"), 0));

            assertTrue("ANY address not IPv6: " + s.getLocalSocketAddress(),
                    s.getLocalAddress() instanceof Inet6Address);
        }
    }

    public void test_getLocalPort() throws IOException {
        // Test for method int java.net.Socket.getLocalPort()
        int sport = startServer("SServer getLocalPort");
        s = new Socket(InetAddress.getLocalHost().getHostName(), sport,
                InetAddress.getLocalHost(), 0);
        // There's nothing we can usefully assert about the kernel-assigned port.
        s.getLocalPort();
    }

    @SuppressWarnings("deprecation")
    public void test_getOutputStream() throws IOException {
        // Test for method java.io.OutputStream
        // java.net.Socket.getOutputStream()
        int sport = startServer("SServer getOutputStream");
        try (Socket s = new Socket(InetAddress.getLocalHost(), sport)) {
            java.io.OutputStream os = s.getOutputStream();
            assertNotNull("Failed to get stream", os);
            os.write(1);
        }

        // Regression test for harmony-2934
        try (Socket s = new Socket("127.0.0.1", sport, false);
             OutputStream o = s.getOutputStream()) {
            o.write(1);
            try {
                Thread.sleep(1000);
            } catch (InterruptedException e) {
            }
        }

        // Regression test for harmony-2942
        try (Socket s = new Socket("0.0.0.0", sport, false);
             OutputStream o = s.getOutputStream()) {
            o.write(1);
            try {
                Thread.sleep(1000);
            } catch (InterruptedException e) {
            }
        }
    }

    public void test_getPort() throws IOException {
        // Test for method int java.net.Socket.getPort()
        int sport = startServer("SServer getPort");
        s = new Socket(InetAddress.getLocalHost(), sport, null, 0);
        assertTrue("Returned incorrect port" + s.getPort(), s.getPort() == sport);
    }

    public void test_getSoLinger() {
        // Test for method int java.net.Socket.getSoLinger()
        int sport = startServer("SServer getSoLinger");
        try (Socket s = new Socket(InetAddress.getLocalHost(), sport, null, 0)) {
            s.setSoLinger(true, 200);
            assertEquals("Returned incorrect linger", 200, s.getSoLinger());
            ensureExceptionThrownIfOptionIsUnsupportedOnOS(SO_LINGER);
            s.setSoLinger(false, 0);
        } catch (Exception e) {
            handleException(e, SO_LINGER);
        }

        try {
            s = new Socket(InetAddress.getLocalHost(), sport, null, 0);
            s.close();
            try {
                s.getSoLinger();
                fail("SocketException was not thrown.");
            } catch(SocketException ioe) {
                //expected
            }
        } catch(Exception e) {
            fail("Unexpected exception was thrown: " + e.toString());
        }
    }

    public void test_getReceiveBufferSize() {
        try {
            int sport = startServer("SServer getReceiveBufferSize");
            s = new Socket(InetAddress.getLocalHost().getHostName(), sport, null, 0);
            s.setReceiveBufferSize(130);
            assertTrue("Incorrect buffer size", s.getReceiveBufferSize() >= 130);
            ensureExceptionThrownIfOptionIsUnsupportedOnOS(SO_RCVBUF);
        } catch (Exception e) {
            handleException(e, SO_RCVBUF);
        }

        try {
            Socket newSocket = new Socket();
            newSocket.close();
            try {
                newSocket.getReceiveBufferSize();
                fail("SocketException was not thrown.");
            } catch(SocketException e) {
                //expected
            }
        } catch(Exception e) {
            fail("Unexpected exception.");
        }
    }

    public void test_getSendBufferSize() {
        int sport = startServer("SServer setSendBufferSize");
        try (Socket s = new Socket(InetAddress.getLocalHost().getHostName(), sport, null, 0)) {
            s.setSendBufferSize(134);
            assertTrue("Incorrect buffer size", s.getSendBufferSize() >= 134);
            ensureExceptionThrownIfOptionIsUnsupportedOnOS(SO_SNDBUF);
        } catch (Exception e) {
            handleException(e, SO_SNDBUF);
        }
        try {
            s = new Socket(InetAddress.getLocalHost(), sport, null, 0);
            s.close();
            try {
                s.getSendBufferSize();
                fail("IOException was not thrown.");
            } catch(IOException ioe) {
                //expected
            }
        } catch(Exception e) {
            fail("Unexpected exception was thrown: " + e.toString());
        }
    }

    public void test_getSoTimeout_setSoTimeout() throws Exception {
        // TODO: a useful test would check that setSoTimeout actually causes timeouts!
        Socket s = new Socket();
        s.setSoTimeout(1500);
        int ms = s.getSoTimeout();
        assertTrue("suspicious timeout: " + ms, Math.abs(ms - 1500) <= 10);
        s.close();
        try {
            s.getSoTimeout();
            fail("SocketException was not thrown.");
        } catch (SocketException expected) {
        }
        try {
            s.setSoTimeout(1000);
            fail("SocketException was not thrown.");
        } catch (SocketException expected) {
        }
    }

    public void test_getTcpNoDelay() {
        // Test for method boolean java.net.Socket.getTcpNoDelay()
        int sport = startServer("SServer getTcpNoDelay");
        try (Socket s = new Socket(InetAddress.getLocalHost(), sport, null, 0)) {
            boolean bool = !s.getTcpNoDelay();
            s.setTcpNoDelay(bool);
            assertTrue("Failed to get no delay setting: " + s.getTcpNoDelay(),
                    s.getTcpNoDelay() == bool);
            ensureExceptionThrownIfOptionIsUnsupportedOnOS(TCP_NODELAY);
        } catch (Exception e) {
            handleException(e, TCP_NODELAY);
        }

        try {
            s = new Socket(InetAddress.getLocalHost(), sport, null, 0);
            s.close();
            try {
                s.getTcpNoDelay();
                fail("SocketException was not thrown.");
            } catch(SocketException ioe) {
                //expected
            }
        } catch(Exception e) {
            fail("Unexpected exception was thrown: " + e.toString());
        }
    }

    public void test_setKeepAliveZ() throws Exception {
        // There is not really a good test for this as it is there to detect
        // crashed machines. Just make sure we can set it
        int sport = startServer("SServer setKeepAlive");
        try (Socket theSocket = new Socket(InetAddress.getLocalHost(), sport, null, 0)) {
            theSocket.setKeepAlive(true);
            theSocket.setKeepAlive(false);
            ensureExceptionThrownIfOptionIsUnsupportedOnOS(SO_KEEPALIVE);
        } catch (Exception e) {
            handleException(e, SO_KEEPALIVE);
        }
        // regression test for HARMONY-1136
        try (TestSocket testSocket = new TestSocket((SocketImpl) null)) {
            testSocket.setKeepAlive(true);
        }

        try {
            Socket theSocket = new Socket();
            theSocket.close();
            theSocket.setKeepAlive(true);
            fail("SocketException was not thrown.");
        } catch(SocketException ioe) {
            //expected
        }
    }
    class TestSocket extends Socket {
        public TestSocket(SocketImpl impl) throws SocketException {
            super(impl);
        }
    }

    public void test_setSocketImplFactoryLjava_net_SocketImplFactory() {
        // Test for method void
        // java.net.Socket.setSocketImplFactory(java.net.SocketImplFactory)

        // Cannot test as setting will cause the factory to be changed for
        // all subsequent sockets

        SecurityManager sm = new SecurityManager() {

            public void checkPermission(Permission perm) {
            }

            public void checkSetFactory() {
                throw new SecurityException();
            }
        };
    }

    public void test_setSendBufferSizeI() {
        try {
            int sport = startServer("SServer setSendBufferSizeI");
            s = new Socket(InetAddress.getLocalHost(), sport, null, 0);
            s.setSendBufferSize(134);
            assertTrue("Incorrect buffer size", s.getSendBufferSize() >= 134);
            ensureExceptionThrownIfOptionIsUnsupportedOnOS(SO_SNDBUF);
        } catch (Exception e) {
            handleException(e, SO_SNDBUF);
        }

        try {
            Socket theSocket = new Socket();
            theSocket.close();
            theSocket.setSendBufferSize(1);
            fail("SocketException was not thrown.");
        } catch(SocketException ioe) {
            //expected
        } catch(IOException ioe) {
            fail("IOException was thrown.");
        }
    }

    public void test_setReceiveBufferSizeI() {
        try {
            int sport = startServer("SServer setReceiveBufferSizeI");
            s = new Socket(InetAddress.getLocalHost(), sport, null, 0);
            s.setReceiveBufferSize(130);
            assertTrue("Incorrect buffer size", s.getReceiveBufferSize() >= 130);
            ensureExceptionThrownIfOptionIsUnsupportedOnOS(SO_RCVBUF);
        } catch (Exception e) {
            handleException(e, SO_RCVBUF);
        }

        try {
            Socket theSocket = new Socket();
            theSocket.close();
            theSocket.setReceiveBufferSize(1);
            fail("SocketException was not thrown.");
        } catch(SocketException ioe) {
            //expected
        } catch(IOException ioe) {
            fail("IOException was thrown.");
        }
    }

    public void test_setSoLingerZI() {
        // Test for method void java.net.Socket.setSoLinger(boolean, int)
        try {
            int sport = startServer("SServer setSoLingerZI");
            s = new Socket(InetAddress.getLocalHost(), sport, null, 0);
            s.setSoLinger(true, 500);
            assertEquals("Set incorrect linger", 500, s.getSoLinger());
            ensureExceptionThrownIfOptionIsUnsupportedOnOS(SO_LINGER);
            s.setSoLinger(false, 0);
        } catch (Exception e) {
            handleException(e, SO_LINGER);
        }

        try {
            Socket theSocket = new Socket();
            theSocket.close();
            theSocket.setSoLinger(true, 1);
            fail("SocketException was not thrown.");
        } catch(SocketException ioe) {
            //expected
        } catch(IOException ioe) {
            fail("IOException was thrown.");
        }
    }

    public void test_setTcpNoDelayZ() {
        // Test for method void java.net.Socket.setTcpNoDelay(boolean)
        try {
            int sport = startServer("SServer setTcpNoDelayZ");
            s = new Socket(InetAddress.getLocalHost(), sport, null, 0);
            boolean bool;
            s.setTcpNoDelay(bool = !s.getTcpNoDelay());
            assertTrue("Failed to set no delay setting: " + s.getTcpNoDelay(),
                    s.getTcpNoDelay() == bool);
            ensureExceptionThrownIfOptionIsUnsupportedOnOS(TCP_NODELAY);
        } catch (Exception e) {
            handleException(e, TCP_NODELAY);
        }

        try {
            Socket theSocket = new Socket();
            theSocket.close();
            theSocket.setTcpNoDelay(true);
            fail("SocketException was not thrown.");
        } catch(SocketException ioe) {
            //expected
        } catch(IOException ioe) {
            fail("IOException was thrown.");
        }
    }

    public void test_toString() throws IOException {
        // Test for method java.lang.String java.net.Socket.toString()
        int sport = startServer("SServer toString");
        s = new Socket(InetAddress.getLocalHost().getHostName(), sport,
                InetAddress.getLocalHost(), 0);
        assertEquals("Socket[address=" + InetAddress.getLocalHost() + ",port=" + s.getPort()
                + ",localPort=" + s.getLocalPort() + "]", s.toString());
    }

    // AndroidOnly: RI returns wrong value for EOF
    public void test_shutdownInput() throws Exception {
        InetAddress addr = InetAddress.getLocalHost();
        ServerSocket serverSocket = new ServerSocket(0, 5, addr);
        Socket theSocket = new Socket(addr, serverSocket.getLocalPort());
        Socket servSock = serverSocket.accept();

        InputStream theInput = theSocket.getInputStream();
        OutputStream theOutput = servSock.getOutputStream();

        // shutdown the input
        theSocket.shutdownInput();

        // send the regular data
        String sendString = new String("Test");
        theOutput.write(sendString.getBytes());
        theOutput.flush();

        // give things some time to settle
        Thread.sleep(1000);

        // RI fails here. It is a RI bug not to return 0 to indicate EOF
        assertEquals(0, theInput.available());

        theSocket.close();
        serverSocket.close();

        Socket socket = new Socket();
        socket.close();
        try {
            socket.shutdownInput();
            fail("IOException was not thrown.");
        } catch(IOException ioe) {
            //expected
        }
    }

    public void test_shutdownOutput() throws IOException {
        ServerSocket serverSocket = new ServerSocket(0, 5);
        Socket theSocket = new Socket(serverSocket.getInetAddress(), serverSocket.getLocalPort());
        Socket servSock = serverSocket.accept();

        InputStream theInput = theSocket.getInputStream();
        OutputStream theOutput = servSock.getOutputStream();

        // shutdown the output
        servSock.shutdownOutput();

        // send the regular data
        String sendString = new String("Test");
        try {
            theOutput.write(sendString.getBytes());
            theOutput.flush();
            fail("No exception when writing on socket with output shutdown");
        } catch (Exception e) {
        }

        theSocket.close();
        serverSocket.close();

        try {
            theSocket.shutdownInput();
            fail("IOException was not thrown.");
        } catch(IOException ioe) {
            //expected
        }
    }

    public void test_getLocalSocketAddress() throws IOException {
        // set up server connect and then validate that we get the right
        // response for the local address
        int sport = startServer("SServer getLocSocketAddress");
        s = new Socket(InetAddress.getLocalHost(), sport, null, 0);
        assertEquals(new InetSocketAddress(InetAddress.getLocalHost(), s.getLocalPort()),
                     s.getLocalSocketAddress());
        s.close();

        // now create a socket that is not bound and validate we get the
        // right answer
        Socket theSocket = new Socket();
        assertNull(
                "Returned incorrect InetSocketAddress -unbound socket- Expected null",
                theSocket.getLocalSocketAddress());

        // now bind the socket and make sure we get the right answer
        theSocket.bind(new InetSocketAddress(InetAddress.getLocalHost(), 0));
        assertEquals(new InetSocketAddress(InetAddress.getLocalHost(), theSocket.getLocalPort()),
                     theSocket.getLocalSocketAddress());
        theSocket.close();

        // now validate that behavior when the any address is returned
        s = new Socket();
        s.bind(new InetSocketAddress(InetAddress.getByName("0.0.0.0"), 0));

        assertTrue("ANY address not IPv6: " + s.getLocalSocketAddress(),
                ((InetSocketAddress) s.getLocalSocketAddress()).getAddress() instanceof Inet6Address);
        s.close();

        // now validate the same for getLocalAddress
        s = new Socket();
        s.bind(new InetSocketAddress(InetAddress.getByName("0.0.0.0"), 0));
        assertTrue("ANY address not IPv6: " + s.getLocalSocketAddress(),
                ((InetSocketAddress) s.getLocalSocketAddress()).getAddress() instanceof Inet6Address);
        s.close();
    }

    public void test_getRemoteSocketAddress() throws IOException {
        // set up server connect and then validate that we get the right
        // response for the remote address
        int sport = startServer("SServer getLocRemoteAddress");
        s = new Socket(InetAddress.getLocalHost(), sport, null, 0);
        assertTrue("Returned incorrect InetSocketAddress(1):"
                + s.getLocalSocketAddress().toString(),
                s.getRemoteSocketAddress()
                        .equals(
                                new InetSocketAddress(InetAddress
                                        .getLocalHost(), sport)));
        s.close();

        // now create one that is not connect and validate that we get the
        // right answer
        Socket theSocket = new Socket();
        theSocket.bind(new InetSocketAddress(InetAddress.getLocalHost(), 0));

        assertNull("Returned incorrect InetSocketAddress -unconnected socket:"
                + "Expected: NULL", theSocket.getRemoteSocketAddress());

        // now connect and validate we get the right answer
        theSocket.connect(new InetSocketAddress(InetAddress.getLocalHost(),
                sport));
        assertTrue("Returned incorrect InetSocketAddress(2):"
                + theSocket.getRemoteSocketAddress().toString(),
                theSocket.getRemoteSocketAddress()
                        .equals(
                                new InetSocketAddress(InetAddress
                                        .getLocalHost(), sport)));
        theSocket.close();

    }

    public void test_isBound() throws IOException {
        ServerSocket serverSocket = new ServerSocket(0, 5);
        Socket theSocket = new Socket(serverSocket.getInetAddress(), serverSocket.getLocalPort());
        Socket servSock = serverSocket.accept();
        assertTrue("Socket indicated  not bound when it should be (1)",
                theSocket.isBound());
        theSocket.close();
        serverSocket.close();

        // now do it with the new constructors and revalidate. Connect causes
        // the socket to be bound
        theSocket = new Socket();
        assertFalse("Socket indicated bound when it was not (2)", theSocket
                .isBound());
        serverSocket = new ServerSocket(0, 5);
        theSocket.connect(serverSocket.getLocalSocketAddress());
        servSock = serverSocket.accept();
        assertTrue("Socket indicated not bound when it should be (2)",
                theSocket.isBound());
        theSocket.close();
        serverSocket.close();

        // now test when we bind explicitly
        theSocket = new Socket();
        assertFalse("Socket indicated bound when it was not (3)", theSocket
                .isBound());
        theSocket.bind(null);
        assertTrue("Socket indicated not bound when it should be (3a)",
                theSocket.isBound());
        theSocket.close();
        assertTrue("Socket indicated not bound when it should be (3b)",
                theSocket.isBound());
    }

    public void test_isConnected() throws IOException {
        ServerSocket serverSocket = new ServerSocket(0, 5);
        Socket theSocket = new Socket(serverSocket.getInetAddress(), serverSocket.getLocalPort());
        Socket servSock = serverSocket.accept();
        assertTrue("Socket indicated  not connected when it should be",
                theSocket.isConnected());
        theSocket.close();
        serverSocket.close();

        // now do it with the new constructors and revalidate
        theSocket = new Socket();
        assertFalse("Socket indicated connected when it was not", theSocket
                .isConnected());
        serverSocket = new ServerSocket(0, 5);
        theSocket.connect(serverSocket.getLocalSocketAddress());
        servSock = serverSocket.accept();
        assertTrue("Socket indicated  not connected when it should be",
                theSocket.isConnected());
        theSocket.close();
        serverSocket.close();
    }

    public void test_isClosed() throws IOException {
        try (ServerSocket serverSocket = new ServerSocket(0, 5)) {
            Socket theSocket = new Socket(serverSocket.getInetAddress(),
                    serverSocket.getLocalPort());
            Socket servSock = serverSocket.accept();

            // validate isClosed returns expected values
            assertFalse("Socket should indicate it is not closed(1):", theSocket.isClosed());
            theSocket.close();
            assertTrue("Socket should indicate it is closed(1):", theSocket.isClosed());

            theSocket = new Socket(serverSocket.getInetAddress(), serverSocket.getLocalPort());
            assertFalse("Socket should indicate it is not closed(2):", theSocket.isClosed());
            theSocket.close();
            assertTrue("Socket should indicate it is closed(2):", theSocket.isClosed());

            // validate that isClosed works ok for sockets returned from
            // ServerSocket.accept()
            assertFalse("Server Socket should indicate it is not closed:", servSock.isClosed());
            servSock.close();
            assertTrue("Server Socket should indicate it is closed:", servSock.isClosed());
        }
    }

    public void test_bindLjava_net_SocketAddress() throws IOException {

        class mySocketAddress extends SocketAddress {

            public mySocketAddress() {
            }
        }

        // Address we cannot bind to
        Socket theSocket = new Socket();
        try {
            theSocket.bind(new InetSocketAddress(InetAddress
                    .getByAddress(Support_Configuration.nonLocalAddressBytes),
                    80));
            fail("No exception when binding to bad address:"
                   + theSocket.getLocalSocketAddress().toString());
        } catch (IOException ex) {
        }
        theSocket.close();

        // now create a socket that is not bound and then bind it
        theSocket = new Socket();
        theSocket.bind(new InetSocketAddress(InetAddress.getLocalHost(),
                0));

        // validate that the localSocketAddress reflects the address we
        // bound to
        assertEquals(new InetSocketAddress(InetAddress.getLocalHost(), theSocket.getLocalPort()),
                     theSocket.getLocalSocketAddress());

        // make sure we can now connect and that connections appear to come
        // from the address we bound to.
        ServerSocket serverSocket = new ServerSocket(0, 5);
        theSocket.connect(serverSocket.getLocalSocketAddress());
        Socket servSock = serverSocket.accept();
        assertEquals(new InetSocketAddress(InetAddress.getLocalHost(), theSocket.getLocalPort()),
                     servSock.getRemoteSocketAddress());
        theSocket.close();
        servSock.close();
        serverSocket.close();

        // validate if we pass in null that it picks an address for us and
        // all is ok
        theSocket = new Socket();
        theSocket.bind(null);
        assertNotNull("Bind with null did not work", theSocket
                .getLocalSocketAddress());
        theSocket.close();

        // now check the error conditions

        // Address that we have already bound to
        theSocket = new Socket();
        Socket theSocket2 = new Socket();
        try {
            theSocket.bind(null);
            theSocket2.bind(theSocket.getLocalSocketAddress());
            fail("No exception binding to address that is not available");
        } catch (IOException ex) {
        }
        theSocket.close();
        theSocket2.close();

        // unsupported SocketAddress subclass
        theSocket = new Socket();
        try {
            theSocket.bind(new mySocketAddress());
            fail("No exception when binding using unsupported SocketAddress subclass");
        } catch (IllegalArgumentException ex) {
        }
        theSocket.close();
    }

    public void test_bindLjava_net_SocketAddress_Proxy() throws IOException {
        //The Proxy will not impact on the bind operation.It can be assigned with any address.
        Proxy proxy = new Proxy(Proxy.Type.SOCKS, new InetSocketAddress("127.0.0.1", 0));
        Socket socket = new Socket(proxy);

        try {
            InetAddress address = InetAddress.getByName("localhost");
            int port = 0;
            socket.bind(new InetSocketAddress(address, port));

            assertEquals(address, socket.getLocalAddress());
            assertTrue(port!=socket.getLocalPort());

        } finally {
            socket.close();
        }
    }

    public void test_connectLjava_net_SocketAddress() throws Exception {
        // needed for some tests
        class mySocketAddress extends SocketAddress {

            public mySocketAddress() {
            }
        }

        // start by validating the error checks

        byte[] theBytes = { 0, 0, 0, 0 };
        SocketAddress theAddress = new InetSocketAddress(InetAddress.getLocalHost(), 0);
        SocketAddress nonConnectableAddress = new InetSocketAddress(InetAddress.getByAddress(theBytes), 0);
        SocketAddress nonReachableAddress = UNREACHABLE_ADDRESS;
        SocketAddress invalidType = new mySocketAddress();

        Socket theSocket = null;
        ServerSocket serverSocket = null;
        try {
            theSocket = new Socket();
            theSocket.connect(null);
            fail("No exception after null address passed in");
        } catch (Exception e) {
            assertTrue("Wrong exception null address passed in: "
                    + e.toString(), (e instanceof IllegalArgumentException));
        }

        try {
            theSocket = new Socket();
            theSocket.connect(invalidType);
            fail("No exception when invalid socket address type passed in: ");
        } catch (Exception e) {
            assertTrue(
                    "Wrong exception when when invalid socket address type passed in: "
                            + e.toString(),
                    (e instanceof IllegalArgumentException));
        }

        try {
            theSocket = new Socket();
            theSocket.connect(nonConnectableAddress);
            fail("No exception when non Connectable Address passed in: ");
        } catch (Exception e) {
            assertTrue(
                    "Wrong exception when non Connectable Address passed in: "
                            + e.toString(), (e instanceof ConnectException));
        }

        // now validate that we get a connect exception if we try to connect to
        // an address on which nobody is listening
        try {
            theSocket = new Socket();
            theSocket.connect(theAddress);
            theSocket.close();
            fail("No exception when connecting to address nobody listening on: ");
        } catch (Exception e) {
            assertTrue(
                    "Wrong exception when connecting to address nobody listening on: "
                            + e.toString(), (e instanceof ConnectException));
        }

        // now validate that we can actually connect when somebody is listening
        theSocket = new Socket();
        serverSocket = new ServerSocket(0, 5);
        theSocket.connect(serverSocket.getLocalSocketAddress());

        // validate that when a socket is connected that it answers
        // correctly to related queries
        assertTrue("Socket did not returned connected when it is: ", theSocket
                .isConnected());
        assertFalse("Socket returned closed when it should be connected ",
                theSocket.isClosed());
        assertTrue("Socket returned not bound when it should be: ", theSocket
                .isBound());
        assertFalse(
                "Socket returned input Shutdown when it should be connected ",
                theSocket.isInputShutdown());
        assertFalse(
                "Socket returned output Shutdown when it should be connected ",
                theSocket.isOutputShutdown());
        assertTrue("Local port on connected socket was 0", theSocket
                .getLocalPort() != 0);
        theSocket.close();
        serverSocket.close();

        // now validate that we get the right exception if we connect when we
        // are already connected
        try {
            theSocket = new Socket();
            serverSocket = new ServerSocket(0, 5);
            theSocket.connect(serverSocket.getLocalSocketAddress());
            theSocket.connect(serverSocket.getLocalSocketAddress());
            theSocket.close();
            serverSocket.close();
            fail("No exception when we try to connect on a connected socket: ");

        } catch (Exception e) {
            assertTrue(
                    "Wrong exception when connecting on socket that is allready connected"
                            + e.toString(), (e instanceof SocketException));
            assertFalse(
                    "Wrong exception when connecting on socket that is allready connected"
                            + e.toString(),
                    (e instanceof SocketTimeoutException));
            try {
                theSocket.close();
                serverSocket.close();
            } catch (Exception e2) {
            }

        }

        // now validate that connected socket can be used to read/write
        theSocket = new Socket();
        serverSocket = new ServerSocket(0, 5);
        theSocket.connect(serverSocket.getLocalSocketAddress());
        Socket servSock = serverSocket.accept();
        InputStream theInput = theSocket.getInputStream();
        OutputStream theOutput = servSock.getOutputStream();
        InputStream theInput2 = servSock.getInputStream();
        OutputStream theOutput2 = theSocket.getOutputStream();

        String sendString = new String("Test");
        theOutput.write(sendString.getBytes());
        theOutput.flush();

        Thread.sleep(1000);

        String receivedString = readShortString(theInput);
        assertTrue("Could not recv on socket connected with timeout:"
                + receivedString + ":" + sendString, receivedString
                .equals(sendString));

        sendString = new String("SEND - Test");
        theOutput2.write(sendString.getBytes());
        theOutput2.flush();
        Thread.sleep(1000);

        receivedString = readShortString(theInput2);
        assertTrue("Could not send on socket connected with timeout:"
                + receivedString + ":" + sendString, receivedString
                .equals(sendString));

        theSocket.close();
        serverSocket.close();

        SocketChannel channel = SocketChannel.open();
        channel.configureBlocking(false);
        Socket socket = channel.socket();
        try {
            socket.connect(serverSocket.getLocalSocketAddress());
            fail("IllegalBlockingModeException was not thrown.");
        } catch (IllegalBlockingModeException expected) {
        }
        socket.close();
    }

    public void test_connectLjava_net_SocketAddressI() throws Exception {

        // needed for some tests
        class mySocketAddress extends SocketAddress {

            public mySocketAddress() {
            }
        }

        // start by validating the error checks
        byte[] theBytes = { 0, 0, 0, 0 };
        SocketAddress theAddress = new InetSocketAddress(InetAddress.getLocalHost(), 0);
        SocketAddress nonConnectableAddress = new InetSocketAddress(InetAddress.getByAddress(theBytes), 0);
        SocketAddress nonReachableAddress = UNREACHABLE_ADDRESS;
        SocketAddress invalidType = new mySocketAddress();

        try (Socket theSocket = new Socket()) {
            theSocket.connect(theAddress, -100);
            fail("No exception after negative timeout passed in");
        } catch (Exception e) {
            assertTrue("Wrong exception when negative timeout passed in: "
                    + e.toString(), (e instanceof IllegalArgumentException));
        }

        try (Socket theSocket = new Socket()) {
            theSocket.connect(null, 0);
            fail("No exception after null address passed in");
        } catch (Exception e) {
            assertTrue("Wrong exception null address passed in: "
                    + e.toString(), (e instanceof IllegalArgumentException));
        }

        try (Socket theSocket = new Socket()) {
            theSocket.connect(invalidType, 100000);
            fail("No exception when invalid socket address type passed in: ");
        } catch (Exception e) {
            assertTrue(
                    "Wrong exception when when invalid socket address type passed in: "
                            + e.toString(),
                    (e instanceof IllegalArgumentException));
        }

        try (Socket theSocket = new Socket()) {
            theSocket.connect(nonConnectableAddress, 100000);
            fail("No exception when non Connectable Address passed in: ");
        } catch (Exception e) {
            assertTrue(
                    "Wrong exception when non Connectable Address passed in: "
                            + e.toString(), (e instanceof SocketException));
        }

        // now validate that we get a connect exception if we try to connect to
        // an address on which nobody is listening
        try (Socket theSocket = new Socket()) {
            theSocket.connect(theAddress, 0);
            fail("No timeout:No exception when connecting to address nobody listening on: ");
        } catch (Exception e) {
            assertTrue(
                    "No timeout:Wrong exception when connecting to address nobody listening on: "
                            + e.toString(), (e instanceof ConnectException));
        }

        // now validate that we can actually connect when somebody is listening
        try (Socket theSocket = new Socket();
             ServerSocket serverSocket = new ServerSocket(0, 5)) {
            theSocket.connect(serverSocket.getLocalSocketAddress());
        }

        // now validate that we get a connect exception if we try to connect to
        // an address on which nobody is listening
        try (Socket theSocket = new Socket()) {
            try {
                theSocket.connect(new InetSocketAddress(InetAddress.getLocalHost(), 80), 100000);
                fail("No exception when connecting to address nobody listening on: ");
            } catch (Exception e) {
                assertTrue(
                        "Wrong exception when connecting to address nobody listening on: "
                                + e.toString(), (e instanceof ConnectException));
            }
        }

        // now validate that we get a interrupted exception if we try to connect
        // to an address on which nobody is accepting connections and the
        // timeout expired
        try (Socket theSocket = new Socket()) {
            try {
                theSocket.connect(nonReachableAddress, 200);
                fail("No interrupted exception when connecting to address nobody listening on with short timeout 200: ");
            } catch (ConnectException ce) {
                // some networks will quickly reset the TCP connection attempt to this fake IP
                assertTrue(
                        "Wrong exception when connecting to address nobody listening on with short timeout 200: "
                                + ce.toString(),
                        (ce.getMessage() != null && ce.getMessage().contains("ECONNREFUSED")));
            } catch (Exception e) {
                assertTrue(
                        "Wrong exception when connecting to address nobody listening on with short timeout 200: "
                                + e.toString(),
                        (e instanceof SocketTimeoutException));
            }
        }

        // now validate that we get a interrupted exception if we try to connect
        // to an address on which nobody is accepting connections and the
        // timeout expired
        try (Socket theSocket = new Socket()) {
            try {
                theSocket.connect(nonReachableAddress, 40);
                fail("No interrupted exception when connecting to address nobody listening on with short timeout 40: ");
            } catch (ConnectException ce) {
                // some networks will quickly reset the TCP connection attempt to this fake IP
                assertTrue(
                        "Wrong exception when connecting to address nobody listening on with short timeout 40: "
                                + ce.toString(),
                        (ce.getMessage() != null && ce.getMessage().contains("ECONNREFUSED")));
            } catch (Exception e) {
                assertTrue(
                        "Wrong exception when connecting to address nobody listening on with short timeout 40: "
                                + e.toString(),
                        (e instanceof SocketTimeoutException));
            }
        }

        // now validate that we can actually connect when somebody is listening
        try (Socket theSocket = new Socket();
             ServerSocket serverSocket = new ServerSocket(0, 5)) {
            theSocket.connect(serverSocket.getLocalSocketAddress());

            // validate that when a socket is connected that it answers
            // correctly to related queries
            assertTrue("Socket did not returned connected when it is: ", theSocket
                    .isConnected());
            assertFalse("Socket returned closed when it should be connected ",
                    theSocket.isClosed());
            assertTrue("Socket returned not bound when it should be: ", theSocket
                    .isBound());
            assertFalse(
                    "Socket returned input Shutdown when it should be connected ",
                    theSocket.isInputShutdown());
            assertFalse(
                    "Socket returned output Shutdown when it should be connected ",
                    theSocket.isOutputShutdown());
            assertTrue("Local port on connected socket was 0", theSocket
                    .getLocalPort() != 0);
        }

        // now validate that we get the right exception if we connect when we
        // are already connected
        try (Socket theSocket = new Socket();
             ServerSocket serverSocket = new ServerSocket()) {
            serverSocket.bind(theAddress);
            theSocket.connect(serverSocket.getLocalSocketAddress(), 100000);
            try {
                theSocket.connect(serverSocket.getLocalSocketAddress(), 100000);
                fail("No exception when we try to connect on a connected socket: ");
            } catch (Exception e) {
                assertTrue(
                        "Wrong exception when connecting on socket that is already connected"
                                + e.toString(), (e instanceof SocketException));
                assertFalse(
                        "Wrong exception when connecting on socket that is already connected"
                                + e.toString(),
                        (e instanceof SocketTimeoutException));
            }
        }

        // now validate that connected socket can be used to read/write
        SocketAddress localSocketAddress;
        try (Socket theSocket = new Socket();
             ServerSocket serverSocket = new ServerSocket(0, 5)) {
            localSocketAddress = serverSocket.getLocalSocketAddress();
            theSocket.connect(localSocketAddress);
            Socket servSock = serverSocket.accept();
            InputStream theInput = theSocket.getInputStream();
            OutputStream theOutput = servSock.getOutputStream();
            InputStream theInput2 = servSock.getInputStream();
            OutputStream theOutput2 = theSocket.getOutputStream();

            String sendString = new String("Test");
            theOutput.write(sendString.getBytes());
            theOutput.flush();

            Thread.sleep(1000);

            String receivedString = readShortString(theInput);
            assertTrue("Could not recv on socket connected with timeout:"
                    + receivedString + ":" + sendString, receivedString
                    .equals(sendString));

            sendString = new String("SEND - Test");
            theOutput2.write(sendString.getBytes());
            theOutput2.flush();

            receivedString = readShortString(theInput2);
            assertTrue("Could not send on socket connected with timeout:"
                    + receivedString + ":" + sendString, receivedString
                    .equals(sendString));
        }

        try (SocketChannel channel = SocketChannel.open()) {
            channel.configureBlocking(false);
            Socket socket = channel.socket();
            try {
                socket.connect(localSocketAddress);
                fail("IllegalBlockingModeException was not thrown.");
            } catch (IllegalBlockingModeException expected) {
            }
        }
    }

    @DisableResourceLeakageDetection(
            why = "Strange threading behavior causes resource leak",
            bug = "31820278")
    public void test_connectLjava_net_SocketAddressI_setSOTimeout() throws Exception {
        class SocketConnector extends Thread {

            int timeout = 0;

            Socket theSocket = null;

            SocketAddress address = null;

            public void run() {
                try {
                    theSocket.connect(address, timeout);
                } catch (Exception e) {
                }

                return;
            }

            public SocketConnector(int timeout, Socket theSocket,
                    SocketAddress address) {
                this.timeout = timeout;
                this.theSocket = theSocket;
                this.address = address;
            }
        }

        // now try to set options while we are connecting
        SocketAddress nonReachableAddress = UNREACHABLE_ADDRESS;
        try (Socket theSocket = new Socket()) {
            SocketConnector connector = new SocketConnector(5000, theSocket, nonReachableAddress);
            connector.start();
            theSocket.setSoTimeout(1000);
            Thread.sleep(10);
            assertTrue("Socket option not set during connect: 10 ",
                    Math.abs(1000 - theSocket.getSoTimeout()) <= 10);
            Thread.sleep(50);
            theSocket.setSoTimeout(2000);
            assertTrue("Socket option not set during connect: 50 ",
                    Math.abs(2000 - theSocket.getSoTimeout()) <= 10);
            Thread.sleep(5000);
        }
    }

    public void test_isInputShutdown() throws IOException {
        Socket theSocket = new Socket();
        ServerSocket serverSocket = new ServerSocket(0, 5);
        theSocket.connect(serverSocket.getLocalSocketAddress());
        Socket servSock = serverSocket.accept();
        InputStream theInput = theSocket.getInputStream();
        OutputStream theOutput = servSock.getOutputStream();

        // make sure we get the right answer with newly connected socket
        assertFalse("Socket indicated input shutdown when it should not have",
                theSocket.isInputShutdown());

        // shutdown the output
        theSocket.shutdownInput();

        // make sure we get the right answer once it is shut down
        assertTrue(
                "Socket indicated input was NOT shutdown when it should have been",
                theSocket.isInputShutdown());

        theSocket.close();
        serverSocket.close();

        // make sure we get the right answer for closed sockets
        assertFalse(
                "Socket indicated input was shutdown when socket was closed",
                servSock.isInputShutdown());

    }

    public void test_isOutputShutdown() throws IOException {
        Socket theSocket = new Socket();
        ServerSocket serverSocket = new ServerSocket(0, 5);
        theSocket.connect(serverSocket.getLocalSocketAddress());
        Socket servSock = serverSocket.accept();
        InputStream theInput = theSocket.getInputStream();
        OutputStream theOutput = servSock.getOutputStream();

        // make sure we get the right answer with newly connected socket
        assertFalse("Socket indicated output shutdown when it should not have",
                servSock.isOutputShutdown());

        // shutdown the output
        servSock.shutdownOutput();

        // make sure we get the right answer once it is shut down
        assertTrue(
                "Socket indicated output was NOT shutdown when it should have been",
                servSock.isOutputShutdown());

        theSocket.close();
        serverSocket.close();

        // make sure we get the right answer for closed sockets
        assertFalse(
                "Socket indicated output was output shutdown when the socket was closed",
                theSocket.isOutputShutdown());

    }

    public void test_setReuseAddressZ() throws Exception {

        try {
            InetAddress allAddresses[] = InetAddress.getAllByName(InetAddress
                    .getLocalHost().getHostName());
            if (allAddresses.length > 1) {

                ServerSocket serverSocket = new ServerSocket(0, 5);

                // try to bind to port address that is already in use with
                // reuseAddress = false.
                // On windows platforms the bind is allowed even then
                // reUseAddress is false (ONLY IF BOTH SOCKETS
                // ARE IPV4 Sockets) so our test uses the platform to determine
                // what the expected result is. It seems that on linux
                // platforms we also don't get an exception.
                InetSocketAddress theLocalAddress = new InetSocketAddress(
                        (InetAddress) allAddresses[1], 0);
                InetSocketAddress theOtherLocalAddress = new InetSocketAddress(
                        (InetAddress) allAddresses[0], theLocalAddress.getPort());
                Socket theSocket = new Socket();
                theSocket.setReuseAddress(false);
                theSocket.bind(theLocalAddress);
                Socket theSocket2 = null;
                String platform = System.getProperty("os.name");

                    theSocket2 = new Socket();
                    theSocket2.setReuseAddress(false);
                    theSocket2.bind(theOtherLocalAddress);

                    if ((!platform.startsWith("Linux"))
                            && ((!platform.startsWith("Windows")) ||
                            // for windows we don't get an exception with
                            // setreuse set to false unless one of the
                            // addresses we bind to is an IPv6 address and we
                            // are therefore using the IPv6 stack.
                            !((((InetAddress) allAddresses[0]) instanceof Inet4Address) && (((InetAddress) allAddresses[1]) instanceof Inet4Address)))) {
                        fail("No exception when setReuseAddress is false and we bind:"
                                + theLocalAddress.toString()
                                + ":"
                                + theOtherLocalAddress.toString());
                    }
                theSocket.close();
                theSocket2.close();

                // try to bind to port that is already in use with reuseAddress
                // = true
                theLocalAddress = new InetSocketAddress((InetAddress) allAddresses[0], 0);

                theSocket = new Socket();
                theSocket.setReuseAddress(true);
                theSocket.bind(theLocalAddress);
                theSocket2 = new Socket();
                theSocket2.setReuseAddress(true);
                theOtherLocalAddress = new InetSocketAddress((InetAddress) allAddresses[1], theSocket.getLocalPort());
                theSocket2.bind(theOtherLocalAddress);
                theSocket2.close();
                theSocket.close();
                serverSocket.close();

                // try with default behavior which should be the same on all
                // platforms
                theLocalAddress = new InetSocketAddress((InetAddress) allAddresses[0], 0);

                theSocket = new Socket();
                theSocket.bind(theLocalAddress);
                theSocket2 = new Socket();
                theOtherLocalAddress = new InetSocketAddress((InetAddress) allAddresses[1], theSocket.getLocalPort());
                theSocket2.bind(theOtherLocalAddress);
                theSocket2.close();
                theSocket.close();
                serverSocket.close();

                ensureExceptionThrownIfOptionIsUnsupportedOnOS(SO_REUSEADDR);
            }
        } catch (Exception e) {
            handleException(e, SO_REUSEADDR);
        }

        try {
            Socket theSocket = new Socket();
            theSocket.close();
            theSocket.setReuseAddress(true);
            fail("SocketException was not thrown.");
        } catch(SocketException ioe) {
            //expected
        } catch(IOException ioe) {
            fail("IOException was thrown.");
        }
    }

    public void test_getReuseAddress() {
        try (Socket theSocket = new Socket()) {
            theSocket.setReuseAddress(true);
            assertTrue("getReuseAddress false when it should be true",
                    theSocket.getReuseAddress());
            theSocket.setReuseAddress(false);
            assertFalse("getReuseAddress true when it should be False",
                    theSocket.getReuseAddress());
            ensureExceptionThrownIfOptionIsUnsupportedOnOS(SO_REUSEADDR);
        } catch (Exception e) {
            handleException(e, SO_REUSEADDR);
        }

        try {
            Socket newSocket = new Socket();
            newSocket.close();
            try {
                newSocket.getReuseAddress();
                fail("SocketException was not thrown.");
            } catch(SocketException e) {
                //expected
            }
        } catch(Exception e) {
            fail("Unexpected exception.");
        }
    }

    public void test_setOOBInlineZ() {
        // mostly tested in getOOBInline. Just set to make sure call works ok
        try (Socket theSocket = new Socket()) {
            theSocket.setOOBInline(true);
            assertTrue("expected OOBIline to be true", theSocket.getOOBInline());
            ensureExceptionThrownIfOptionIsUnsupportedOnOS(SO_OOBINLINE);
        } catch (Exception e) {
            handleException(e, SO_OOBINLINE);
        }

        try {
            Socket theSocket = new Socket();
            theSocket.close();
            theSocket.setOOBInline(true);
            fail("SocketException was not thrown.");
        } catch(SocketException ioe) {
            //expected
        } catch(IOException ioe) {
            fail("IOException was thrown.");
        }
    }

    public void test_getOOBInline() {

        try {
            Socket theSocket = new Socket();

            // validate that value reflects what we set it to true after true,
            // false after false and false after false false
            theSocket.setOOBInline(true);
            assertTrue("expected OOBIline to be true", theSocket.getOOBInline());
            theSocket.setOOBInline(false);
            assertFalse("expected OOBIline to be true", theSocket
                    .getOOBInline());
            theSocket.setOOBInline(false);
            assertFalse("expected OOBIline to be true", theSocket
                    .getOOBInline());
            theSocket.close();
            try {
                theSocket.getOOBInline();
                fail("SocketException was not thrown.");
            } catch(SocketException se) {
                //expected
            }
            ensureExceptionThrownIfOptionIsUnsupportedOnOS(SO_OOBINLINE);

        } catch (Exception e) {
            handleException(e, SO_OOBINLINE);
        }
    }

    public void test_setTrafficClassI() {
        try (Socket theSocket = new Socket()) {
            int IPTOS_LOWCOST = 0x2;
            int IPTOS_THROUGHPUT = 0x8;

            // validate that value set must be between 0 and 255
            try {
                theSocket.setTrafficClass(256);
                fail("No exception was thrown when traffic class set to 256");
            } catch (IllegalArgumentException e) {
            }

            try {
                theSocket.setTrafficClass(-1);
                fail("No exception was thrown when traffic class set to -1");
            } catch (IllegalArgumentException e) {
            }

            // now validate that we can set it to some good values
            theSocket.setTrafficClass(IPTOS_LOWCOST);
            theSocket.setTrafficClass(IPTOS_THROUGHPUT);
            ensureExceptionThrownIfOptionIsUnsupportedOnOS(IP_TOS);
        } catch (Exception e) {
            handleException(e, IP_TOS);
        }

        try {
            Socket theSocket = new Socket();
            theSocket.close();
            theSocket.setTrafficClass(0);
            fail("SocketException was not thrown.");
        } catch(SocketException ioe) {
            //expected
        } catch(IOException ioe) {
            fail("IOException was thrown.");
        }
    }

    public void test_getTrafficClass() {
        try (Socket theSocket = new Socket()) {
            /*
             * we cannot actually check that the values are set as if a platform
             * does not support the option then it may come back unset even
             * though we set it so just get the value to make sure we can get it
             */
            int trafficClass = theSocket.getTrafficClass();
            ensureExceptionThrownIfOptionIsUnsupportedOnOS(IP_TOS);
        } catch (Exception e) {
            handleException(e, IP_TOS);
        }
    }

    public void test_getChannel() throws Exception {
        assertNull(new Socket().getChannel());

        SocketChannel channel = SocketChannel.open();
        Socket socket = channel.socket();
        assertEquals(channel, socket.getChannel());
        socket.close();
        channel.close();
    }

    public void test_sendUrgentDataI() throws IOException {

        // Some platforms may not support urgent data in this case we will not
        // run these tests. For now run on all platforms until we find those
        // that do not support urgent data
        String platform = System.getProperty("os.name");
        if (!platform.equals("Dummy")) {
            // validate that when OOBInline is false that any urgent data
            // is silently ignored
            String urgentData = "U";
            try {
                try (Socket theSocket = new Socket();
                     ServerSocket serverSocket = new ServerSocket(0, 5)) {
                    theSocket.connect(serverSocket.getLocalSocketAddress());
                    try (Socket servSock = serverSocket.accept();
                         InputStream theInput = theSocket.getInputStream();
                         OutputStream theOutput = servSock.getOutputStream()) {

                        // send the regular data
                        String sendString = "Test";
                        theOutput.write(sendString.getBytes());
                        theOutput.flush();

                        // send the urgent data which should not be received
                        theSocket.setOOBInline(false);
                        servSock.sendUrgentData(urgentData.getBytes()[0]);
                        theOutput.write(sendString.getBytes());
                        theOutput.flush();

                        // give things some time to settle
                        Thread.sleep(1000);

                        String receivedString = readShortString(theInput);
                        //assertTrue("Urgent Data seems to have been received:"
                        //        + receivedString + ":" + sendString, receivedString
                        //        .equals(sendString + sendString));
                    }
                }

                // now validate that urgent data is received as expected. Expect
                // that it should be between the two writes.
                try (Socket theSocket = new Socket();
                     ServerSocket serverSocket = new ServerSocket(0, 5)) {
                    theSocket.connect(serverSocket.getLocalSocketAddress());
                    try (Socket servSock = serverSocket.accept();
                         InputStream theInput = theSocket.getInputStream();
                         OutputStream theOutput = servSock.getOutputStream()) {

                        // send the regular data
                        String sendString = "Test - Urgent Data";
                        theOutput.write(sendString.getBytes());
                        theOutput.flush();

                        // send the urgent data which should be received
                        theSocket.setOOBInline(true);
                        servSock.sendUrgentData(urgentData.getBytes()[0]);

                        theOutput.write(sendString.getBytes());
                        theOutput.flush();

                        Thread.sleep(1000);

                        String receivedString = readShortString(theInput);
                        assertTrue("Urgent Data was not received with one urgent byte:"
                                + receivedString + ":" + sendString + urgentData
                                + sendString, receivedString.equals(sendString
                                + urgentData + sendString));
                    }
                }

                // now test case where we try to send two urgent bytes.
                try (Socket theSocket = new Socket();
                     ServerSocket serverSocket = new ServerSocket(0, 5)) {
                    theSocket.connect(serverSocket.getLocalSocketAddress());
                    try (Socket servSock = serverSocket.accept();
                         InputStream theInput = theSocket.getInputStream();
                         OutputStream theOutput = servSock.getOutputStream()) {

                        // send the regular data
                        String sendString = "Test - Urgent Data";
                        theOutput.write(sendString.getBytes());
                        theOutput.flush();

                        // send the urgent data which should not be received
                        theSocket.setOOBInline(true);
                        servSock.sendUrgentData(urgentData.getBytes()[0]);
                        servSock.sendUrgentData(urgentData.getBytes()[0]);

                        theOutput.write(sendString.getBytes());
                        theOutput.flush();

                        Thread.sleep(1000);

                        String receivedString = readShortString(theInput);
                        assertTrue(
                                "Did not get right byte of urgent data when two sent:"
                                        + receivedString + ":" + sendString
                                        + urgentData + urgentData + sendString,
                                receivedString.equals(sendString + urgentData
                                        + urgentData + sendString));
                    }
                }

                /*
                 * TODO : These do not currently pass on XP SP2 and Server 2003
                 */
                if (!platform.startsWith("Windows")) {
                    // now test the case were we send turn the OOBInline on/off
                    try (Socket theSocket = new Socket();
                         ServerSocket serverSocket = new ServerSocket(0, 5)) {
                        theSocket.connect(serverSocket.getLocalSocketAddress());
                        try (Socket servSock = serverSocket.accept();
                             InputStream theInput = theSocket.getInputStream();
                             OutputStream theOutput = servSock.getOutputStream()) {

                            // send the regular data
                            String sendString = "Test - Urgent Data";
                            theOutput.write(sendString.getBytes());
                            theOutput.flush();

                            // send the urgent data which should be received
                            theSocket.setOOBInline(true);
                            servSock.sendUrgentData(urgentData.getBytes()[0]);

                            theOutput.write(sendString.getBytes());
                            theOutput.flush();

                            Thread.sleep(1000);

                            String receivedString = readShortString(theInput);
                            assertTrue(
                                    "Did not get urgent data when turning on/off(1):"
                                            + receivedString + ":" + sendString
                                            + urgentData + sendString, receivedString
                                            .equals(sendString + urgentData
                                                    + sendString));

                            // send the regular data
                            sendString = "Test - Urgent Data";
                            theOutput.write(sendString.getBytes());
                            theOutput.flush();

                            // send the urgent data which should not be received
                            theSocket.setOOBInline(false);
                            servSock.sendUrgentData(urgentData.getBytes()[0]);

                            // send trailing data
                            theOutput.write(sendString.getBytes());
                            theOutput.flush();

                            Thread.sleep(1000);

                            receivedString = readShortString(theInput);
                            //assertTrue(
                            //        "Got unexpected data data when turning on/off(2):"
                            //                + receivedString + ":" + sendString
                            //               + sendString, receivedString
                            //                .equals(sendString + sendString));

                            // now turn back on and get data. Here we also
                            // get the previously sent byte of urgent data as it is
                            // still in the urgent buffer

                            // send the regular data
                            sendString = "Test - Urgent Data";
                            theOutput.write(sendString.getBytes());
                            theOutput.flush();

                            // send the urgent data which should be received again
                            theSocket.setOOBInline(true);
                            servSock.sendUrgentData(urgentData.getBytes()[0]);

                            theOutput.write(sendString.getBytes());
                            theOutput.flush();

                            Thread.sleep(1000);

                            receivedString = readShortString(theInput);
                            // depending on the platform we may get the previously sent
                            // urgent data or not (examples windows-yes, Linux-no).
                            // So accept either so long as we get the urgent data from
                            // when it was on.
                            //assertTrue(
                            //        "Did not get urgent data when turning on/off(3) GOT:"
                            //                + receivedString + ":Expected" + urgentData
                            //                + sendString + urgentData + sendString
                            //                + ":OR:" + sendString + urgentData
                            //                + sendString,
                            //        (receivedString.equals(urgentData + sendString
                            //                + urgentData + sendString) || receivedString
                            //                .equals(sendString + urgentData
                            //                        + sendString)));
                        }
                    }
                }

                // now test the case where there is only urgent data
                try (Socket theSocket = new Socket();
                     ServerSocket serverSocket = new ServerSocket(0, 5)) {
                    theSocket.connect(serverSocket.getLocalSocketAddress());
                    try (Socket servSock = serverSocket.accept();
                         InputStream theInput = theSocket.getInputStream();
                         OutputStream theOutput = servSock.getOutputStream()) {

                        // send the urgent data which should not be received.
                        theSocket.setOOBInline(true);
                        servSock.sendUrgentData(urgentData.getBytes()[0]);

                        Thread.sleep(1000);

                        String receivedString = readShortString(theInput);
                        assertTrue("Did not get urgent data only urgent data sent:"
                                + receivedString + ":" + urgentData, receivedString
                                .equals(urgentData));
                    }
                }

            } catch (Exception e) {
                // for platforms that do not support urgent data we expect an
                // exception. For the others report an error.
                // TODO : Need to introduce a better test for the exception
                // so that the failure only occurs on platforms that support
                // urgent data
                fail("Platform:" + platform
                        + ": Got exception during sendUrgent data tests"
                        + e.toString());
            }
        }
    }

    // Calling sendUrgentData on a closed socket should not allocate a new impl and leak resources.
    // Bug: 31818400
    public void test_sendUrgentDataI_leaky() throws IOException {
        Socket theSocket = new Socket();
        theSocket.close();
        try {
            theSocket.sendUrgentData(0);
            fail("IOException was not thrown.");
        } catch (IOException ioe) {
            //expected
        }
    }

    // Calling getTrafficClass on a closed socket should not allocate a new impl and leak resources.
    // Bug: 31818400
    public void test_getTrafficClass_leaky() throws IOException {
        Socket theSocket = new Socket();
        theSocket.close();
        try {
            theSocket.getTrafficClass();
            fail();
        } catch (IOException ioe) {
            //expected
        }
    }

    private String readShortString(InputStream theInput) throws IOException {
        int totalBytesRead = 0;
        byte[] myBytes = new byte[100];
        while (theInput.available() > 0) {
            int bytesRead = theInput.read(myBytes, totalBytesRead,
                    myBytes.length - totalBytesRead);
            totalBytesRead = totalBytesRead + bytesRead;
        }

        return new String(myBytes, 0, totalBytesRead);
    }

    public void test_setPerformancePreference_Int_Int_Int() throws Exception {
        try (Socket theSocket = new Socket()) {
            theSocket.setPerformancePreferences(1, 1, 1);
        }
    }

    public void test_ConstructorLjava_net_Proxy_Exception() {

        SocketAddress addr1 = InetSocketAddress.createUnresolved("127.0.0.1",
                80);
        SocketAddress addr2 = new InetSocketAddress("localhost", 80);

        Proxy proxy1 = new Proxy(Proxy.Type.HTTP, addr1);
        // IllegalArgumentException test
        try {
            new Socket(proxy1);
            fail("should throw IllegalArgumentException");
        } catch (IllegalArgumentException e) {
            // expected
        }

        Proxy proxy2 = new Proxy(Proxy.Type.SOCKS, addr1);
        // should not throw any exception
        new Socket(proxy2);
        new Socket(Proxy.NO_PROXY);

        try {
            new Socket((Proxy) null);
            fail("IllegalArgumentException was not thrown.");
        } catch(IllegalArgumentException iae) {
            //expected
        }
    }

    public void test_ConstructorLSocketImpl() {
        MockSocketImpl msi = new MockSocketImpl();
        try {
            new TestSocket(msi);
        } catch (SocketException e) {
            fail("SocketException was thrown.");
        }
    }

    public void test_connect_unknownhost() throws Exception {
        Socket socket = new Socket();
        InetSocketAddress socketAddress = new InetSocketAddress("unknownhost", 12345);
        try {
            socket.connect(socketAddress);
            fail("Should throw IOException");
        } catch (IOException e) {
            // expected
        }
    }

    public void test_connect_unresolved_unknown() throws Exception {
        Socket socket = new Socket();
        InetSocketAddress unresolved = InetSocketAddress.createUnresolved("unknownhost", 12345);
        try {
            socket.connect(unresolved);
            fail("Should throw IOException");
        } catch (IOException e) {
            // expected
        }
    }

    public void test_connect_unresolved() throws Exception {
        Socket socket = new Socket();
        InetSocketAddress unresolvedSocketAddress = InetSocketAddress.createUnresolved(
                Support_Configuration.SocksServerTestHost,
                Support_Configuration.SocksServerTestPort);
        try {
            socket.connect(unresolvedSocketAddress);
            fail("Should throw IOException");
        } catch (IOException e) {
            // expected
        }
    }

    public void test_getOutputStream_shutdownOutput() throws Exception {
        // regression test for Harmony-873
        try (ServerSocket ss = new ServerSocket(0)) {
            try (Socket s = new Socket("127.0.0.1", ss.getLocalPort())) {
                ss.accept();
                s.shutdownOutput();
                try {
                    s.getOutputStream();
                    fail("should throw SocketException");
                } catch (IOException e) {
                    // expected
                }
            }

            SocketChannel channel = SocketChannel.open(
                    new InetSocketAddress(ss.getInetAddress(), ss.getLocalPort()));
            channel.configureBlocking(false);
            ss.accept();
            try (Socket socket = channel.socket();
                 OutputStream out = socket.getOutputStream()) {
                try {
                    out.write(1);
                    fail("IllegalBlockingModeException was not thrown.");
                } catch (IllegalBlockingModeException ibme) {
                    //expected
                }
            }
        }
    }

    public void test_shutdownInputOutput_twice() throws Exception {
        // regression test for Harmony-2944
        try (Socket s = new Socket("0.0.0.0", 0, false)) {
            s.shutdownInput();

            try {
                s.shutdownInput();
                fail("should throw SocketException");
            } catch (SocketException se) {
                // expected
            }
            s.shutdownOutput();

            try {
                s.shutdownOutput();
                fail("should throw SocketException");
            } catch (SocketException se) {
                // expected
            }
        }
    }

    /**
     * Sets up the fixture, for example, open a network connection. This method
     * is called before a test is executed.
     *
     * @throws Exception
     */
    protected void setUp() throws Exception {
        super.setUp();
    }

    /**
     * Tears down the fixture, for example, close a network connection. This
     * method is called after a test is executed.
     */
    protected void tearDown() {
        try {
            if (s != null)
                s.close();
        } catch (Exception e) {
        }
        try {
            if (ss != null)
                ss.close();
        } catch (Exception e) {
        }
        try {
            if (t != null)
                t.interrupt();
        } catch (Exception e) {
        }
    }

    static class MockSecurityManager extends SecurityManager {

        public void checkConnect(String host, int port) {
            if ("127.0.0.1".equals(host)) {
                throw new SecurityException("permission is not allowed");
            }
        }

        public void checkPermission(Permission permission) {
            return;
        }

    }

    protected int startServer(String name) {
        try {
            ss = new ServerSocket(0, 5);
        } catch (IOException e) {
            fail(name + ": " + e);
        }
        return ss.getLocalPort();
    }

    class MockSocketImpl extends SocketImpl {

        public MockSocketImpl() {
            super();
        }

        @Override
        protected void accept(SocketImpl arg0) throws IOException {
        }

        @Override
        protected int available() throws IOException {
            return 0;
        }

        @Override
        protected void bind(InetAddress arg0, int arg1) throws IOException {
        }

        @Override
        protected void close() throws IOException {
        }

        @Override
        protected void connect(String arg0, int arg1) throws IOException {
        }

        @Override
        protected void connect(InetAddress arg0, int arg1) throws IOException {
        }

        @Override
        protected void connect(SocketAddress arg0, int arg1) throws IOException {
        }

        @Override
        protected void create(boolean arg0) throws IOException {
        }

        @Override
        protected InputStream getInputStream() throws IOException {
            return null;
        }

        @Override
        protected OutputStream getOutputStream() throws IOException {
            return null;
        }

        @Override
        protected void listen(int arg0) throws IOException {
        }

        @Override
        protected void sendUrgentData(int arg0) throws IOException {
        }

        public Object getOption(int arg0) throws SocketException {
            return null;
        }

        public void setOption(int arg0, Object arg1) throws SocketException {
        }
    }
}
