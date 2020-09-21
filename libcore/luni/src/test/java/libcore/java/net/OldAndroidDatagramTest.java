/*
 * Copyright (C) 2008 The Android Open Source Project
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

package libcore.java.net;

import java.io.IOException;
import java.net.DatagramPacket;
import java.net.DatagramSocket;
import java.net.InetAddress;
import java.net.InetSocketAddress;
import java.net.SocketTimeoutException;
import java.nio.channels.DatagramChannel;
import junit.framework.TestCase;

/**
 * Implements some simple tests for datagrams. Not as excessive as the core
 * tests, but good enough for the harness.
 */
public class OldAndroidDatagramTest extends TestCase {

    /**
     * Helper class that listens to incoming datagrams and reflects them to the
     * sender. Incoming datagram is interpreted as a String. It is uppercased
     * before being sent back.
     */

    class Reflector extends Thread {
        // Helper class for reflecting incoming datagrams.
        DatagramSocket socket;

        boolean alive = true;

        byte[] buffer = new byte[256];

        DatagramPacket packet;

        /**
         * Main loop. Receives datagrams and reflects them.
         */
        @Override
        public void run() {
            try {
                while (alive) {
                    try {
                        packet.setLength(buffer.length);
                        socket.receive(packet);
                        String s = stringFromPacket(packet);
                        // System.out.println(s + " (from " + packet.getAddress() + ":" + packet.getPort() + ")");

                        try {
                            Thread.sleep(100);
                        } catch (InterruptedException ex) {
                            // Ignore.
                        }

                        stringToPacket(s.toUpperCase(), packet);

                        packet.setAddress(InetAddress.getLocalHost());
                        packet.setPort(2345);

                        socket.send(packet);
                    } catch (java.io.InterruptedIOException e) {
                    }
                }
            } catch (java.io.IOException ex) {
                ex.printStackTrace();
            } finally {
                socket.close();
            }
        }

        /**
         * Creates a new Relfector object for the given local address and port.
         */
        public Reflector(int port, InetAddress address) {
            try {
                packet = new DatagramPacket(buffer, buffer.length);
                socket = new DatagramSocket(port, address);
            } catch (IOException ex) {
                throw new RuntimeException(
                        "Creating datagram reflector failed", ex);
            }
        }
    }

    /**
     * Converts a given datagram packet's contents to a String.
     */
    static String stringFromPacket(DatagramPacket packet) {
        return new String(packet.getData(), 0, packet.getLength());
    }

    /**
     * Converts a given String into a datagram packet.
     */
    static void stringToPacket(String s, DatagramPacket packet) {
        byte[] bytes = s.getBytes();
        System.arraycopy(bytes, 0, packet.getData(), 0, bytes.length);
        packet.setLength(bytes.length);
    }

    /**
     * Implements the main part of the Datagram test.
     */
    public void testDatagram() throws Exception {

        Reflector reflector = null;
        DatagramSocket socket = null;

        try {
            // Setup the reflector, so we have a partner to send to
            reflector = new Reflector(1234, InetAddress.getLocalHost());
            reflector.start();

            byte[] buffer = new byte[256];

            DatagramPacket packet = new DatagramPacket(buffer, buffer.length);
            socket = new DatagramSocket(2345, InetAddress.getLocalHost());

            // Send ten simple packets and check for the expected responses.
            for (int i = 1; i <= 10; i++) {
                String s = "Hello, Android world #" + i + "!";
                stringToPacket(s, packet);

                packet.setAddress(InetAddress.getLocalHost());
                packet.setPort(1234);

                socket.send(packet);

                try {
                    Thread.sleep(100);
                } catch (InterruptedException ex) {
                    // Ignore.
                }

                packet.setLength(buffer.length);
                socket.receive(packet);
                String t = stringFromPacket(packet);
                // System.out.println(t + " (from " + packet.getAddress() + ":" + packet.getPort() + ")");

                assertEquals(s.toUpperCase(), t);
            }
        } finally {
            if (reflector != null) {
                reflector.alive = false;
            }

            if (socket != null) {
                socket.close();
            }
        }
    }

    public void test_54072_DatagramSocket() throws Exception {
        DatagramSocket s = new DatagramSocket(null);
        assertTrue(s.getLocalAddress().isAnyLocalAddress());
        s.bind(new InetSocketAddress(8888));
        assertTrue(s.getLocalAddress().isAnyLocalAddress());
        s.close();
        assertNull(s.getLocalAddress());
    }

    public void test_54072_DatagramChannel() throws Exception {
        DatagramChannel ch = DatagramChannel.open();
        DatagramSocket s = ch.socket();
        assertTrue(s.getLocalAddress().isAnyLocalAddress());
        s.bind(new InetSocketAddress(8888));
        assertTrue(s.getLocalAddress().isAnyLocalAddress());
        s.close();
        assertNull(s.getLocalAddress());
    }
}
