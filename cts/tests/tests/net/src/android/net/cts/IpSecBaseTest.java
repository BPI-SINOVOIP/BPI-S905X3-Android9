/*
 * Copyright (C) 2018 The Android Open Source Project
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

package android.net.cts;

import static org.junit.Assert.assertArrayEquals;

import android.content.Context;
import android.net.IpSecAlgorithm;
import android.net.IpSecManager;
import android.net.IpSecTransform;
import android.system.Os;
import android.system.OsConstants;
import android.test.AndroidTestCase;
import android.util.Log;

import java.io.FileDescriptor;
import java.io.IOException;
import java.net.DatagramPacket;
import java.net.DatagramSocket;
import java.net.Inet4Address;
import java.net.Inet6Address;
import java.net.InetAddress;
import java.net.InetSocketAddress;
import java.net.ServerSocket;
import java.net.Socket;
import java.net.SocketException;
import java.util.Arrays;
import java.util.concurrent.atomic.AtomicInteger;

public class IpSecBaseTest extends AndroidTestCase {

    private static final String TAG = IpSecBaseTest.class.getSimpleName();

    protected static final String IPV4_LOOPBACK = "127.0.0.1";
    protected static final String IPV6_LOOPBACK = "::1";
    protected static final String[] LOOPBACK_ADDRS = new String[] {IPV4_LOOPBACK, IPV6_LOOPBACK};
    protected static final int[] DIRECTIONS =
            new int[] {IpSecManager.DIRECTION_IN, IpSecManager.DIRECTION_OUT};

    protected static final byte[] TEST_DATA = "Best test data ever!".getBytes();
    protected static final int DATA_BUFFER_LEN = 4096;
    protected static final int SOCK_TIMEOUT = 500;

    private static final byte[] KEY_DATA = {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
        0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
        0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
        0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F,
        0x20, 0x21, 0x22, 0x23
    };

    protected static final byte[] AUTH_KEY = getKey(256);
    protected static final byte[] CRYPT_KEY = getKey(256);

    protected IpSecManager mISM;

    protected void setUp() throws Exception {
        super.setUp();
        mISM = (IpSecManager) getContext().getSystemService(Context.IPSEC_SERVICE);
    }

    protected static byte[] getKey(int bitLength) {
        return Arrays.copyOf(KEY_DATA, bitLength / 8);
    }

    protected static int getDomain(InetAddress address) {
        int domain;
        if (address instanceof Inet6Address) {
            domain = OsConstants.AF_INET6;
        } else {
            domain = OsConstants.AF_INET;
        }
        return domain;
    }

    protected static int getPort(FileDescriptor sock) throws Exception {
        return ((InetSocketAddress) Os.getsockname(sock)).getPort();
    }

    public static interface GenericSocket extends AutoCloseable {
        void send(byte[] data) throws Exception;

        byte[] receive() throws Exception;

        int getPort() throws Exception;

        void close() throws Exception;

        void applyTransportModeTransform(
                IpSecManager ism, int direction, IpSecTransform transform) throws Exception;

        void removeTransportModeTransforms(IpSecManager ism) throws Exception;
    }

    public static interface GenericTcpSocket extends GenericSocket {}

    public static interface GenericUdpSocket extends GenericSocket {
        void sendTo(byte[] data, InetAddress dstAddr, int port) throws Exception;
    }

    public abstract static class NativeSocket implements GenericSocket {
        public FileDescriptor mFd;

        public NativeSocket(FileDescriptor fd) {
            mFd = fd;
        }

        @Override
        public void send(byte[] data) throws Exception {
            Os.write(mFd, data, 0, data.length);
        }

        @Override
        public byte[] receive() throws Exception {
            byte[] in = new byte[DATA_BUFFER_LEN];
            AtomicInteger bytesRead = new AtomicInteger(-1);

            Thread readSockThread = new Thread(() -> {
                long startTime = System.currentTimeMillis();
                while (bytesRead.get() < 0 && System.currentTimeMillis() < startTime + SOCK_TIMEOUT) {
                    try {
                        bytesRead.set(Os.recvfrom(mFd, in, 0, DATA_BUFFER_LEN, 0, null));
                    } catch (Exception e) {
                        Log.e(TAG, "Error encountered reading from socket", e);
                    }
                }
            });

            readSockThread.start();
            readSockThread.join(SOCK_TIMEOUT);

            if (bytesRead.get() < 0) {
                throw new IOException("No data received from socket");
            }

            return Arrays.copyOfRange(in, 0, bytesRead.get());
        }

        @Override
        public int getPort() throws Exception {
            return IpSecBaseTest.getPort(mFd);
        }

        @Override
        public void close() throws Exception {
            Os.close(mFd);
        }

        @Override
        public void applyTransportModeTransform(
                IpSecManager ism, int direction, IpSecTransform transform) throws Exception {
            ism.applyTransportModeTransform(mFd, direction, transform);
        }

        @Override
        public void removeTransportModeTransforms(IpSecManager ism) throws Exception {
            ism.removeTransportModeTransforms(mFd);
        }
    }

    public static class NativeTcpSocket extends NativeSocket implements GenericTcpSocket {
        public NativeTcpSocket(FileDescriptor fd) {
            super(fd);
        }
    }

    public static class NativeUdpSocket extends NativeSocket implements GenericUdpSocket {
        public NativeUdpSocket(FileDescriptor fd) {
            super(fd);
        }

        @Override
        public void sendTo(byte[] data, InetAddress dstAddr, int port) throws Exception {
            Os.sendto(mFd, data, 0, data.length, 0, dstAddr, port);
        }
    }

    public static class JavaUdpSocket implements GenericUdpSocket {
        public final DatagramSocket mSocket;

        public JavaUdpSocket(InetAddress localAddr) {
            try {
                mSocket = new DatagramSocket(0, localAddr);
                mSocket.setSoTimeout(SOCK_TIMEOUT);
            } catch (SocketException e) {
                // Fail loudly if we can't set up sockets properly. And without the timeout, we
                // could easily end up in an endless wait.
                throw new RuntimeException(e);
            }
        }

        @Override
        public void send(byte[] data) throws Exception {
            mSocket.send(new DatagramPacket(data, data.length));
        }

        @Override
        public void sendTo(byte[] data, InetAddress dstAddr, int port) throws Exception {
            mSocket.send(new DatagramPacket(data, data.length, dstAddr, port));
        }

        @Override
        public int getPort() throws Exception {
            return mSocket.getLocalPort();
        }

        @Override
        public void close() throws Exception {
            mSocket.close();
        }

        @Override
        public byte[] receive() throws Exception {
            DatagramPacket data = new DatagramPacket(new byte[DATA_BUFFER_LEN], DATA_BUFFER_LEN);
            mSocket.receive(data);
            return Arrays.copyOfRange(data.getData(), 0, data.getLength());
        }

        @Override
        public void applyTransportModeTransform(
                IpSecManager ism, int direction, IpSecTransform transform) throws Exception {
            ism.applyTransportModeTransform(mSocket, direction, transform);
        }

        @Override
        public void removeTransportModeTransforms(IpSecManager ism) throws Exception {
            ism.removeTransportModeTransforms(mSocket);
        }
    }

    public static class JavaTcpSocket implements GenericTcpSocket {
        public final Socket mSocket;

        public JavaTcpSocket(Socket socket) {
            mSocket = socket;
            try {
                mSocket.setSoTimeout(SOCK_TIMEOUT);
            } catch (SocketException e) {
                // Fail loudly if we can't set up sockets properly. And without the timeout, we
                // could easily end up in an endless wait.
                throw new RuntimeException(e);
            }
        }

        @Override
        public void send(byte[] data) throws Exception {
            mSocket.getOutputStream().write(data);
        }

        @Override
        public byte[] receive() throws Exception {
            byte[] in = new byte[DATA_BUFFER_LEN];
            int bytesRead = mSocket.getInputStream().read(in);
            return Arrays.copyOfRange(in, 0, bytesRead);
        }

        @Override
        public int getPort() throws Exception {
            return mSocket.getLocalPort();
        }

        @Override
        public void close() throws Exception {
            mSocket.close();
        }

        @Override
        public void applyTransportModeTransform(
                IpSecManager ism, int direction, IpSecTransform transform) throws Exception {
            ism.applyTransportModeTransform(mSocket, direction, transform);
        }

        @Override
        public void removeTransportModeTransforms(IpSecManager ism) throws Exception {
            ism.removeTransportModeTransforms(mSocket);
        }
    }

    public static class SocketPair<T> {
        public final T mLeftSock;
        public final T mRightSock;

        public SocketPair(T leftSock, T rightSock) {
            mLeftSock = leftSock;
            mRightSock = rightSock;
        }
    }

    protected static void applyTransformBidirectionally(
            IpSecManager ism, IpSecTransform transform, GenericSocket socket) throws Exception {
        for (int direction : DIRECTIONS) {
            socket.applyTransportModeTransform(ism, direction, transform);
        }
    }

    public static SocketPair<NativeUdpSocket> getNativeUdpSocketPair(
            InetAddress localAddr, IpSecManager ism, IpSecTransform transform, boolean connected)
            throws Exception {
        int domain = getDomain(localAddr);

        NativeUdpSocket leftSock = new NativeUdpSocket(
            Os.socket(domain, OsConstants.SOCK_DGRAM, OsConstants.IPPROTO_UDP));
        NativeUdpSocket rightSock = new NativeUdpSocket(
            Os.socket(domain, OsConstants.SOCK_DGRAM, OsConstants.IPPROTO_UDP));

        for (NativeUdpSocket sock : new NativeUdpSocket[] {leftSock, rightSock}) {
            applyTransformBidirectionally(ism, transform, sock);
            Os.bind(sock.mFd, localAddr, 0);
        }

        if (connected) {
            Os.connect(leftSock.mFd, localAddr, rightSock.getPort());
            Os.connect(rightSock.mFd, localAddr, leftSock.getPort());
        }

        return new SocketPair<>(leftSock, rightSock);
    }

    public static SocketPair<NativeTcpSocket> getNativeTcpSocketPair(
            InetAddress localAddr, IpSecManager ism, IpSecTransform transform) throws Exception {
        int domain = getDomain(localAddr);

        NativeTcpSocket server = new NativeTcpSocket(
                Os.socket(domain, OsConstants.SOCK_STREAM, OsConstants.IPPROTO_TCP));
        NativeTcpSocket client = new NativeTcpSocket(
                Os.socket(domain, OsConstants.SOCK_STREAM, OsConstants.IPPROTO_TCP));

        Os.bind(server.mFd, localAddr, 0);

        applyTransformBidirectionally(ism, transform, server);
        applyTransformBidirectionally(ism, transform, client);

        Os.listen(server.mFd, 10);
        Os.connect(client.mFd, localAddr, server.getPort());
        NativeTcpSocket accepted = new NativeTcpSocket(Os.accept(server.mFd, null));

        applyTransformBidirectionally(ism, transform, accepted);
        server.close();

        return new SocketPair<>(client, accepted);
    }

    public static SocketPair<JavaUdpSocket> getJavaUdpSocketPair(
            InetAddress localAddr, IpSecManager ism, IpSecTransform transform, boolean connected)
            throws Exception {
        JavaUdpSocket leftSock = new JavaUdpSocket(localAddr);
        JavaUdpSocket rightSock = new JavaUdpSocket(localAddr);

        applyTransformBidirectionally(ism, transform, leftSock);
        applyTransformBidirectionally(ism, transform, rightSock);

        if (connected) {
            leftSock.mSocket.connect(localAddr, rightSock.mSocket.getLocalPort());
            rightSock.mSocket.connect(localAddr, leftSock.mSocket.getLocalPort());
        }

        return new SocketPair<>(leftSock, rightSock);
    }

    public static SocketPair<JavaTcpSocket> getJavaTcpSocketPair(
            InetAddress localAddr, IpSecManager ism, IpSecTransform transform) throws Exception {
        JavaTcpSocket clientSock = new JavaTcpSocket(new Socket());
        ServerSocket serverSocket = new ServerSocket();
        serverSocket.bind(new InetSocketAddress(localAddr, 0));

        // While technically the client socket does not need to be bound, the OpenJDK implementation
        // of Socket only allocates an FD when bind() or connect() or other similar methods are
        // called. So we call bind to force the FD creation, so that we can apply a transform to it
        // prior to socket connect.
        clientSock.mSocket.bind(new InetSocketAddress(localAddr, 0));

        // IpSecService doesn't support serverSockets at the moment; workaround using FD
        FileDescriptor serverFd = serverSocket.getImpl().getFD$();

        applyTransformBidirectionally(ism, transform, new NativeTcpSocket(serverFd));
        applyTransformBidirectionally(ism, transform, clientSock);

        clientSock.mSocket.connect(new InetSocketAddress(localAddr, serverSocket.getLocalPort()));
        JavaTcpSocket acceptedSock = new JavaTcpSocket(serverSocket.accept());

        applyTransformBidirectionally(ism, transform, acceptedSock);
        serverSocket.close();

        return new SocketPair<>(clientSock, acceptedSock);
    }

    private void checkSocketPair(GenericSocket left, GenericSocket right) throws Exception {
        left.send(TEST_DATA);
        assertArrayEquals(TEST_DATA, right.receive());

        right.send(TEST_DATA);
        assertArrayEquals(TEST_DATA, left.receive());

        left.close();
        right.close();
    }

    private void checkUnconnectedUdpSocketPair(
            GenericUdpSocket left, GenericUdpSocket right, InetAddress localAddr) throws Exception {
        left.sendTo(TEST_DATA, localAddr, right.getPort());
        assertArrayEquals(TEST_DATA, right.receive());

        right.sendTo(TEST_DATA, localAddr, left.getPort());
        assertArrayEquals(TEST_DATA, left.receive());

        left.close();
        right.close();
    }

    protected static IpSecTransform buildIpSecTransform(
            Context mContext,
            IpSecManager.SecurityParameterIndex spi,
            IpSecManager.UdpEncapsulationSocket encapSocket,
            InetAddress remoteAddr)
            throws Exception {
        String localAddr = (remoteAddr instanceof Inet4Address) ? IPV4_LOOPBACK : IPV6_LOOPBACK;
        IpSecTransform.Builder builder =
                new IpSecTransform.Builder(mContext)
                .setEncryption(new IpSecAlgorithm(IpSecAlgorithm.CRYPT_AES_CBC, CRYPT_KEY))
                .setAuthentication(
                        new IpSecAlgorithm(
                                IpSecAlgorithm.AUTH_HMAC_SHA256,
                                AUTH_KEY,
                                AUTH_KEY.length * 4));

        if (encapSocket != null) {
            builder.setIpv4Encapsulation(encapSocket, encapSocket.getPort());
        }

        return builder.buildTransportModeTransform(InetAddress.getByName(localAddr), spi);
    }

    private IpSecTransform buildDefaultTransform(InetAddress localAddr) throws Exception {
        try (IpSecManager.SecurityParameterIndex spi =
                mISM.allocateSecurityParameterIndex(localAddr)) {
            return buildIpSecTransform(mContext, spi, null, localAddr);
        }
    }

    public void testJavaTcpSocketPair() throws Exception {
        for (String addr : LOOPBACK_ADDRS) {
            InetAddress local = InetAddress.getByName(addr);
            try (IpSecTransform transform = buildDefaultTransform(local)) {
                SocketPair<JavaTcpSocket> sockets = getJavaTcpSocketPair(local, mISM, transform);
                checkSocketPair(sockets.mLeftSock, sockets.mRightSock);
            }
        }
    }

    public void testJavaUdpSocketPair() throws Exception {
        for (String addr : LOOPBACK_ADDRS) {
            InetAddress local = InetAddress.getByName(addr);
            try (IpSecTransform transform = buildDefaultTransform(local)) {
                SocketPair<JavaUdpSocket> sockets =
                        getJavaUdpSocketPair(local, mISM, transform, true);
                checkSocketPair(sockets.mLeftSock, sockets.mRightSock);
            }
        }
    }

    public void testJavaUdpSocketPairUnconnected() throws Exception {
        for (String addr : LOOPBACK_ADDRS) {
            InetAddress local = InetAddress.getByName(addr);
            try (IpSecTransform transform = buildDefaultTransform(local)) {
                SocketPair<JavaUdpSocket> sockets =
                        getJavaUdpSocketPair(local, mISM, transform, false);
                checkUnconnectedUdpSocketPair(sockets.mLeftSock, sockets.mRightSock, local);
            }
        }
    }

    public void testNativeTcpSocketPair() throws Exception {
        for (String addr : LOOPBACK_ADDRS) {
            InetAddress local = InetAddress.getByName(addr);
            try (IpSecTransform transform = buildDefaultTransform(local)) {
                SocketPair<NativeTcpSocket> sockets =
                        getNativeTcpSocketPair(local, mISM, transform);
                checkSocketPair(sockets.mLeftSock, sockets.mRightSock);
            }
        }
    }

    public void testNativeUdpSocketPair() throws Exception {
        for (String addr : LOOPBACK_ADDRS) {
            InetAddress local = InetAddress.getByName(addr);
            try (IpSecTransform transform = buildDefaultTransform(local)) {
                SocketPair<NativeUdpSocket> sockets =
                        getNativeUdpSocketPair(local, mISM, transform, true);
                checkSocketPair(sockets.mLeftSock, sockets.mRightSock);
            }
        }
    }

    public void testNativeUdpSocketPairUnconnected() throws Exception {
        for (String addr : LOOPBACK_ADDRS) {
            InetAddress local = InetAddress.getByName(addr);
            try (IpSecTransform transform = buildDefaultTransform(local)) {
                SocketPair<NativeUdpSocket> sockets =
                        getNativeUdpSocketPair(local, mISM, transform, false);
                checkUnconnectedUdpSocketPair(sockets.mLeftSock, sockets.mRightSock, local);
            }
        }
    }
}
