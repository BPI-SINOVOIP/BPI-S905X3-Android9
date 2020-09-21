/*
 * Copyright 2017 The Android Open Source Project
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

package com.googlecode.android_scripting.facade.bluetooth;

import android.app.Service;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothServerSocket;
import android.bluetooth.BluetoothSocket;

import com.googlecode.android_scripting.Log;
import com.googlecode.android_scripting.facade.EventFacade;
import com.googlecode.android_scripting.facade.FacadeManager;
import com.googlecode.android_scripting.jsonrpc.RpcReceiver;
import com.googlecode.android_scripting.rpc.Rpc;
import com.googlecode.android_scripting.rpc.RpcDefault;
import com.googlecode.android_scripting.rpc.RpcOptional;
import com.googlecode.android_scripting.rpc.RpcParameter;

import org.apache.commons.codec.binary.Base64Codec;

import java.io.IOException;
import java.lang.reflect.Method;
import java.util.HashMap;
import java.util.Map;
import java.util.UUID;

/**
 * Bluetooth functions.
 *
 */

public class BluetoothSocketConnFacade extends RpcReceiver {
    private final Service mService;
    private final BluetoothAdapter mBluetoothAdapter;
    private Map<String, BluetoothConnection> mConnections =
            new HashMap<String, BluetoothConnection>();
    private final EventFacade mEventFacade;
    private ConnectThread mConnectThread;
    private AcceptThread mAcceptThread;
    private byte mTxPktIndex = 0;

    private static final String DEFAULT_PSM = "161";  //=0x00A1

    // UUID for SL4A.
    protected static final String DEFAULT_UUID = "457807c0-4897-11df-9879-0800200c9a66";
    protected static final String SDP_NAME = "SL4A";

    protected static final String DEFAULT_LE_DATA_LENGTH = "23";

    public BluetoothSocketConnFacade(FacadeManager manager) {
        super(manager);
        mEventFacade = manager.getReceiver(EventFacade.class);
        mService = manager.getService();
        mBluetoothAdapter = BluetoothAdapter.getDefaultAdapter();
    }

    private BluetoothConnection getConnection(String connID) throws IOException {
        if (connID == null) {
            throw new IOException("Connection ID is null");
        }
        Log.d("BluetoothConnection:getConnection: connID=" + connID);
        BluetoothConnection conn = null;
        if (connID.trim().length() > 0) {
            conn = mConnections.get(connID);
        } else if (mConnections.size() == 1) {
            conn = (BluetoothConnection) mConnections.values().toArray()[0];
        } else {
            Log.e("More than one available connections. Num=" + mConnections.size());
            throw new IOException("More than 1 available connections. Num=" + mConnections.size());
        }
        if (conn == null) {
            throw new IOException("Bluetooth connection not established. connID=" + connID);
        }
        return conn;
    }

    private String addConnection(BluetoothConnection conn) {
        String uuid = UUID.randomUUID().toString();
        mConnections.put(uuid, conn);
        conn.setUUID(uuid);
        return uuid;
    }

    /**
     * Create L2CAP socket to Bluetooth device
     *
     * @param address the bluetooth address to open the socket on
     * @param channel the channel to open the socket on
     * @throws Exception
     */
    @Rpc(description = "Create L2CAP socket to Bluetooth deice")
    public void bluetoothSocketConnCreateL2capSocket(@RpcParameter(name = "address") String address,
            @RpcParameter(name = "channel") Integer channel) throws Exception {
        BluetoothDevice mDevice;
        mDevice = mBluetoothAdapter.getRemoteDevice(address);
        Class bluetoothDeviceClass = Class.forName("android.bluetooth.BluetoothDevice");
        Method createL2capSocketMethod =
                bluetoothDeviceClass.getMethod("createL2capSocket", int.class);
        createL2capSocketMethod.invoke(mDevice, channel);
    }

    /**
     * Begin Connect Thread using UUID
     *
     * @param address the mac address of the device to connect to
     * @param uuid the UUID that is used by the server device
     * @throws Exception
     */
    @Rpc(description = "Begins a thread initiate an L2CAP socket connection over Bluetooth. ")
    public void bluetoothSocketConnBeginConnectThreadUuid(
            @RpcParameter(name = "address",
            description = "The mac address of the device to connect to.") String address,
            @RpcParameter(name = "uuid",
            description = "The UUID passed here must match the UUID used by the server device.")
            @RpcDefault(DEFAULT_UUID) String uuid)
            throws IOException {
        BluetoothDevice mDevice;
        mDevice = mBluetoothAdapter.getRemoteDevice(address);
        ConnectThread connectThread = new ConnectThread(mDevice, uuid);
        connectThread.start();
        mConnectThread = connectThread;
    }

    /**
     * Begin Connect Thread using PSM value
     *
     * @param address the mac address of the device to connect to
     * @param isBle the transport is LE
     * @param psmValue the assigned PSM value to use for this socket connection
     * @throws Exception
     */
    @Rpc(description = "Begins a thread initiate an L2CAP CoC connection over Bluetooth. ")
    public void bluetoothSocketConnBeginConnectThreadPsm(
            @RpcParameter(name = "address",
            description = "The mac address of the device to connect to.") String address,
            @RpcParameter(name = "isBle", description = "Is transport BLE?") @RpcDefault("false")
            Boolean isBle,
            @RpcParameter(name = "psmValue") @RpcDefault(DEFAULT_PSM) Integer psmValue,
            @RpcParameter(name = "securedConn") @RpcDefault("false") Boolean securedConn)
            throws IOException {
        BluetoothDevice mDevice;
        mDevice = mBluetoothAdapter.getRemoteDevice(address);
        Log.d("bluetoothSocketConnBeginConnectThreadPsm: Coc connecting to " + address + ", isBle="
                + isBle + ", psmValue=" + psmValue + ", securedConn=" + securedConn);
        ConnectThread connectThread = new ConnectThread(mDevice, psmValue, isBle, securedConn);
        connectThread.start();
        mConnectThread = connectThread;
    }

    /**
     * Get last connection ID
     *
     * @return String the last connection ID
     * @throws Exception
     */
    @Rpc(description = "Returns the connection ID of the last connection.")
    public String bluetoothGetLastConnId()
            throws IOException {
        if (mAcceptThread != null) {
            String connUuid = mAcceptThread.getConnUuid();
            Log.d("bluetoothGetLastConnId from Accept Thread: connUuid=" + connUuid);
            return connUuid;
        }
        if (mConnectThread != null) {
            String connUuid = mConnectThread.getConnUuid();
            Log.d("bluetoothGetLastConnId from Connect Thread: connUuid=" + connUuid);
            return connUuid;
        }
        Log.e("bluetoothGetLastConnId: No active threads");
        return null;
    }

    /**
     * Kill the connect thread
     */
    @Rpc(description = "Kill thread")
    public void bluetoothSocketConnKillConnThread() {
        try {
            mConnectThread.cancel();
            mConnectThread.join(5000);
        } catch (InterruptedException e) {
            Log.e("Interrupted Exception: " + e.toString());
        }
    }

    /**
     * Closes an active Client socket
     *
     * @throws Exception
     */
    @Rpc(description = "Close an active Client socket")
    public void bluetoothSocketConnEndConnectThread() throws IOException {
        mConnectThread.cancel();
    }

    /**
     * Closes an active Server socket
     *
     * @throws Exception
     */
    @Rpc(description = "Close an active Server socket")
    public void bluetoothSocketConnEndAcceptThread() throws IOException {
        mAcceptThread.cancel();
    }

    /**
     * Returns active Bluetooth mConnections
     *
     * @return map of active connections and its remote addresses
     */
    @Rpc(description = "Returns active Bluetooth mConnections.")
    public Map<String, String> bluetoothSocketConnActiveConnections() {
        Map<String, String> out = new HashMap<String, String>();
        for (Map.Entry<String, BluetoothConnection> entry : mConnections.entrySet()) {
            if (entry.getValue().isConnected()) {
                out.put(entry.getKey(), entry.getValue().getRemoteBluetoothAddress());
            }
        }
        return out;
    }

    /**
     * Returns the name of the connected device
     *
     * @return string name of connected device
     * @throws Exception
     */
    @Rpc(description = "Returns the name of the connected device.")
    public String bluetoothSocketConnGetConnectedDeviceName(
            @RpcParameter(name = "connID", description = "Connection id") @RpcOptional
            @RpcDefault("") String connID)
            throws IOException {
        BluetoothConnection conn = getConnection(connID);
        return conn.getConnectedDeviceName();
    }

    /**
     * Begins a thread to accept an L2CAP connection over Bluetooth with UUID
     *
     * @param uuid the UUID to identify this L2CAP connection
     * @param timeout the time to wait for new connection
     * @throws Exception
     */
    @Rpc(description = "Begins a thread to accept an L2CAP connection over Bluetooth. ")
    public void bluetoothSocketConnBeginAcceptThreadUuid(
            @RpcParameter(name = "uuid") @RpcDefault(DEFAULT_UUID) String uuid,
            @RpcParameter(name = "timeout",
            description = "How long to wait for a new connection, 0 is wait for ever")
            @RpcDefault("0") Integer timeout)
            throws IOException {
        Log.d("bluetoothSocketConnBeginAcceptThreadUuid: uuid=" + uuid);
        AcceptThread acceptThread = new AcceptThread(uuid, timeout.intValue());
        acceptThread.start();
        mAcceptThread = acceptThread;
    }

    /**
     * Begins a thread to accept an L2CAP connection over Bluetooth with PSM value
     *
     * @param psmValue the PSM value to identify this L2CAP connection
     * @param timeout the time to wait for new connection
     * @param isBle whether this connection uses LE transport
     * @throws Exception
     */
    @Rpc(description = "Begins a thread to accept an Coc connection over Bluetooth. ")
    public void bluetoothSocketConnBeginAcceptThreadPsm(
            @RpcParameter(name = "timeout",
                      description = "How long to wait for a new connection, 0 is wait for ever")
            @RpcDefault("0") Integer timeout,
            @RpcParameter(name = "isBle",
                      description = "Is transport BLE?")
            @RpcDefault("false") Boolean isBle,
            @RpcParameter(name = "securedConn",
                      description = "Using secured connection?")
            @RpcDefault("false") Boolean securedConn,
            @RpcParameter(name = "psmValue") @RpcDefault(DEFAULT_PSM) Integer psmValue)
            throws IOException {
        Log.d("bluetoothSocketConnBeginAcceptThreadPsm: PSM value=" + psmValue);
        AcceptThread acceptThread = new AcceptThread(psmValue.intValue(), timeout.intValue(),
                                                     isBle, securedConn);
        acceptThread.start();
        mAcceptThread = acceptThread;
    }

    /**
     * Get the current BluetoothServerSocket PSM value
     * @return Integer the assigned PSM value
     * @throws Exception
     */
    @Rpc(description = "Returns the PSM value")
    public Integer bluetoothSocketConnGetPsm() throws IOException  {
        Integer psm = new Integer(mAcceptThread.getPsm());
        Log.d("bluetoothSocketConnGetPsm: PSM value=" + psm);
        return psm;
    }

    /**
     * Set the current BluetoothSocket LE Data Length value to the maximum supported by this BT
     * controller. This command suggests to the BT controller to set its maximum transmission packet
     * size.
     * @throws Exception
     */
    @Rpc(description = "Request Maximum Tx Data Length")
    public void bluetoothSocketRequestMaximumTxDataLength()
            throws IOException  {
        Log.d("bluetoothSocketRequestMaximumTxDataLength");

        if (mConnectThread == null) {
            String connUuid = mConnectThread.getConnUuid();
            throw new IOException("bluetoothSocketRequestMaximumTxDataLength: no active connect"
                                  + " thread");
        }

        BluetoothSocket socket = mConnectThread.getSocket();
        if (socket == null) {
            throw new IOException("bluetoothSocketRequestMaximumTxDataLength: no active connect"
                                  + " socket");
        }
        socket.requestMaximumTxDataLength();
    }

    /**
     * Sends ASCII characters over the currently open Bluetooth connection
     *
     * @param ascii the string to write
     * @param connID the connection ID
     * @throws Exception
     */
    @Rpc(description = "Sends ASCII characters over the currently open Bluetooth connection.")
    public void bluetoothSocketConnWrite(@RpcParameter(name = "ascii") String ascii,
            @RpcParameter(name = "connID", description = "Connection id")
            @RpcDefault("") String connID)
            throws IOException {
        BluetoothConnection conn = getConnection(connID);
        try {
            conn.write(ascii);
        } catch (IOException e) {
            mConnections.remove(conn.getUUID());
            throw e;
        }
    }

    /**
     * Read up to bufferSize ASCII characters
     *
     * @param bufferSize the size of buffer to read
     * @param connID the connection ID
     * @return the string buffer containing the read ASCII characters
     * @throws Exception
     */
    @Rpc(description = "Read up to bufferSize ASCII characters.")
    public String bluetoothSocketConnRead(
            @RpcParameter(name = "bufferSize") @RpcDefault("4096") Integer bufferSize,
            @RpcParameter(name = "connID", description = "Connection id") @RpcOptional
            @RpcDefault("") String connID)
            throws IOException {
        BluetoothConnection conn = getConnection(connID);
        try {
            return conn.read(bufferSize);
        } catch (IOException e) {
            mConnections.remove(conn.getUUID());
            throw e;
        }
    }

    /**
     * Send bytes over the currently open Bluetooth connection
     *
     * @param base64 the based64-encoded string to write
     * @param connID the connection ID
     * @throws Exception
     */
    @Rpc(description = "Send bytes over the currently open Bluetooth connection.")
    public void bluetoothSocketConnWriteBinary(
            @RpcParameter(name = "base64",
            description = "A base64 encoded String of the bytes to be sent.") String base64,
            @RpcParameter(name = "connID",
            description = "Connection id") @RpcDefault("") @RpcOptional String connID)
            throws IOException {
        BluetoothConnection conn = getConnection(connID);
        try {
            conn.write(Base64Codec.decodeBase64(base64));
        } catch (IOException e) {
            mConnections.remove(conn.getUUID());
            throw e;
        }
    }

    /**
     * Read up to bufferSize bytes and return a chunked, base64 encoded string
     *
     * @param bufferSize the size of buffer to read
     * @param connID the connection ID
     * @return the string buffer containing the read base64-encoded characters
     * @throws Exception
     */
    @Rpc(description = "Read up to bufferSize bytes and return a chunked, base64 encoded string.")
    public String bluetoothSocketConnReadBinary(
            @RpcParameter(name = "bufferSize") @RpcDefault("4096") Integer bufferSize,
            @RpcParameter(name = "connID", description = "Connection id") @RpcDefault("")
            @RpcOptional String connID)
            throws IOException {

        BluetoothConnection conn = getConnection(connID);
        try {
            return Base64Codec.encodeBase64String(conn.readBinary(bufferSize));
        } catch (IOException e) {
            mConnections.remove(conn.getUUID());
            throw e;
        }
    }

    /**
     * Returns true if the next read is guaranteed not to block
     *
     * @param connID the connection ID
     * @return true if the the next read is guaranteed not to block
     * @throws Exception
     */
    @Rpc(description = "Returns True if the next read is guaranteed not to block.")
    public Boolean bluetoothSocketConnReadReady(
            @RpcParameter(name = "connID", description = "Connection id") @RpcDefault("")
            @RpcOptional String connID)
            throws IOException {
        BluetoothConnection conn = getConnection(connID);
        try {
            return conn.readReady();
        } catch (IOException e) {
            mConnections.remove(conn.getUUID());
            throw e;
        }
    }

    /**
     * Read the next line
    *
    * @param connID the connection ID
    * @return the string buffer containing the read line
    * @throws Exception
     */
    @Rpc(description = "Read the next line.")
    public String bluetoothSocketConnReadLine(
            @RpcParameter(name = "connID", description = "Connection id") @RpcOptional
            @RpcDefault("") String connID)
            throws IOException {
        BluetoothConnection conn = getConnection(connID);
        try {
            return conn.readLine();
        } catch (IOException e) {
            mConnections.remove(conn.getUUID());
            throw e;
        }
    }

    private static byte getNextOutputChar(byte in) {
        in++;
        if (in >= 'z') {
            in = 'a';
        }
        return in;
    }

    private static int getNextOutputChar(int in) {
        in++;
        if (in >= 'z') {
            in = 'a';
        }
        return in;
    }

    /**
     * Send a data buffer with auto-generated data
     *
     * @param numBuffers the number of buffers to send
     * @param bufferSize the buffer size in bytes
     * @param connID the connection ID
     * @throws Exception
     */
    @Rpc(description = "Send a large buffer of bytes for throughput test")
    public void bluetoothConnectionThroughputSend(
            @RpcParameter(name = "numBuffers", description = "number of buffers")
            Integer numBuffers,
            @RpcParameter(name = "bufferSize", description = "buffer size") Integer bufferSize,
            @RpcParameter(name = "connID", description = "Connection id")
            @RpcDefault("") @RpcOptional String connID)
            throws IOException {

        Log.d("bluetoothConnectionThroughputSend: numBuffers=" + numBuffers + ", bufferSize="
                + bufferSize + ", connID=" + connID + ", mTxPktIndex=" + mTxPktIndex);

        // Generate a buffer of given size
        byte[] outBuf = new byte[bufferSize];
        byte outChar = 'a';
        // The first byte is the buffer index
        int i = 0;
        outBuf[i++] = mTxPktIndex;
        for (; i < bufferSize; i++) {
            outBuf[i] = outChar;
            outChar = getNextOutputChar(outChar);
        }

        BluetoothConnection conn = getConnection(connID);
        try {
            for (i = 0; i < numBuffers; i++) {
                Log.d("bluetoothConnectionThroughputSend: sending " + i + " buffer.");
                outBuf[0] = mTxPktIndex++;
                conn.write(outBuf);
            }
        } catch (IOException e) {
            mConnections.remove(conn.getUUID());
            throw e;
        }
    }

    /**
     * Read a number of data buffers and make sure the data is correct
     *
     * @param numBuffers the number of buffers to send
     * @param bufferSize the buffer size in bytes
     * @param connID the connection ID
     * @return the data rate read in terms of bytes per second
     * @throws Exception
     */
    @Rpc(description = "Returns the throughput in bytes-per-sec, or Returns 0 if unsuccessful")
    public Integer bluetoothConnectionThroughputRead(
            @RpcParameter(name = "numBuffers", description = "number of buffers")
            Integer numBuffers,
            @RpcParameter(name = "bufferSize", description = "buffer size") Integer bufferSize,
            @RpcParameter(name = "connID", description = "Connection id")
            @RpcDefault("") @RpcOptional String connID)
            throws IOException {

        Log.d("bluetoothConnectionThroughputRead: numBuffers=" + numBuffers + ", bufferSize="
                + bufferSize);

        BluetoothConnection conn = getConnection(connID);

        long startTesttime = System.currentTimeMillis();

        byte bufIndex = (byte) 0x00FF;

        try {
            for (int i = 0; i < numBuffers; i++) {
                // Read one buffer
                byte[] readBuf = conn.readBinary(bufferSize);

                // Make sure the contents are valid
                int nextInChar = 'a';
                int j = 0;
                // The first byte is the buffer index
                if (i == 0) {
                    bufIndex = readBuf[j];
                } else {
                    bufIndex++;
                    if (bufIndex != readBuf[j]) {
                        Log.e("bluetoothConnectionThroughputRead: Wrong Buffer index (First byte). "
                                + "Expected=" + bufIndex + ", read=" + readBuf[j]);
                        throw new IOException("bluetoothConnectionThroughputRead: Wrong Buffer("
                                              + (i + 1) + ") index (First byte). Expected="
                                              + bufIndex + ", read=" + readBuf[j]);
                    }
                }
                Log.d("bluetoothConnectionThroughputRead: First byte=" + bufIndex);
                j++;

                for (; j < bufferSize; j++) {
                    if (readBuf[j] != nextInChar) {
                        Log.e("Last Read Char Read wrong value. Read=" + String.valueOf(readBuf[j])
                                + ", Expected=" + String.valueOf(nextInChar));
                        throw new IOException("Read mismatched at buf=" + i + ", idx=" + j);
                    }
                    nextInChar = getNextOutputChar(nextInChar);
                }
                Log.d("bluetoothConnectionThroughputRead: Buffer Read index=" + i);
            }

            long endTesttime = System.currentTimeMillis();

            long diffTime = endTesttime - startTesttime;  // time delta in milliseconds
            Log.d("bluetoothConnectionThroughputRead: Completed! numBuffers=" + numBuffers
                    + ",delta time=" + diffTime + " millisec");
            long numBytes = numBuffers * bufferSize;
            long dataRatePerMsec;
            if (diffTime > 0) {
                dataRatePerMsec = (1000L * numBytes) / diffTime;
            } else {
                dataRatePerMsec = 9999;
            }
            Integer dataRate = new Integer((int) dataRatePerMsec);

            Log.d("bluetoothConnectionThroughputRead: Completed! numBytes=" + numBytes
                    + ", data rate=" + dataRate + " bytes per sec");

            return dataRate;
        } catch (IOException e) {
            mConnections.remove(conn.getUUID());
            throw e;
        }
    }

    /**
     * Stops Bluetooth connection
     *
     * @param connID the connection ID
     */
    @Rpc(description = "Stops Bluetooth connection.")
    public void bluetoothSocketConnStop(
            @RpcParameter(name = "connID", description = "Connection id") @RpcOptional
            @RpcDefault("") String connID) {
        BluetoothConnection conn;
        try {
            conn = getConnection(connID);
        } catch (IOException e) {
            e.printStackTrace();
            return;
        }
        if (conn == null) {
            Log.d("bluetoothSocketConnStop: conn is NULL. connID=%s" + connID);
            return;
        }
        Log.d("bluetoothSocketConnStop: connID=" + connID + ", UUID=" + conn.getUUID());

        conn.stop();
        mConnections.remove(conn.getUUID());

        if (mAcceptThread != null) {
            mAcceptThread.cancel();
        }
        if (mConnectThread != null) {
            mConnectThread.cancel();
        }
    }

    @Override
    public void shutdown() {
        for (Map.Entry<String, BluetoothConnection> entry : mConnections.entrySet()) {
            entry.getValue().stop();
        }
        mConnections.clear();
        if (mAcceptThread != null) {
            mAcceptThread.cancel();
        }
        if (mConnectThread != null) {
            mConnectThread.cancel();
        }
    }

    private class ConnectThread extends Thread {
        private final BluetoothSocket mSocket;
        private final Boolean mIsBle;
        String mConnUuid;

        ConnectThread(BluetoothDevice device, String uuid) {
            BluetoothSocket tmp = null;
            try {
                tmp = device.createRfcommSocketToServiceRecord(UUID.fromString(uuid));
            } catch (IOException createSocketException) {
                Log.e("Failed to create socket: " + createSocketException.toString());
            }
            mIsBle = false;
            mSocket = tmp;
        }

        ConnectThread(BluetoothDevice device,
                             @RpcParameter(name = "psmValue")
                             @RpcDefault(DEFAULT_PSM) Integer psmValue,
                             @RpcParameter(name = "isBle") @RpcDefault("false") boolean isBle,
                             @RpcParameter(name = "securedConn")
                             @RpcDefault("false") boolean securedConn) {
            BluetoothSocket tmp = null;
            Log.d("ConnectThread: psmValue=" + psmValue + ", isBle=" + isBle
                        + ", securedConn=" + securedConn);
            try {
                if (isBle) {
                    if (securedConn) {
                        tmp = device.createL2capCocSocket(BluetoothDevice.TRANSPORT_LE, psmValue);
                    } else {
                        tmp = device.createInsecureL2capCocSocket(BluetoothDevice.TRANSPORT_LE,
                                                                  psmValue);
                    }
                } else {
                    if (securedConn) {
                        tmp = device.createL2capSocket(psmValue);
                    } else {
                        tmp = device.createInsecureL2capSocket(psmValue);
                    }
                }
                // Secured version: tmp = device.createL2capSocket(0x1011);
                // tmp = device.createRfcommSocketToServiceRecord(UUID.fromString(uuid));
            } catch (IOException createSocketException) {
                Log.e("Failed to create socket: " + createSocketException.toString());
            }
            mIsBle = isBle;
            mSocket = tmp;
        }

        public void run() {
            mBluetoothAdapter.cancelDiscovery();
            try {
                BluetoothConnection conn;
                mSocket.connect();
                conn = new BluetoothConnection(mSocket);
                mConnUuid = addConnection(conn);
                Log.d("ConnectThread:run: isConnected=" + mSocket.isConnected() + ", address="
                        + mSocket.getRemoteDevice().getAddress() + ", uuid=" + mConnUuid);
            } catch (IOException connectException) {
                Log.e("ConnectThread::run(): Error: Connection Unsuccessful");
                cancel();
                return;
            }
        }

        public void cancel() {
            if (mSocket != null) {
                try {
                    mSocket.close();
                } catch (IOException closeException) {
                    Log.e("Failed to close socket: " + closeException.toString());
                }
            }
        }

        public BluetoothSocket getSocket() {
            return mSocket;
        }

        public String getConnUuid() {
            Log.d("ConnectThread::getConnUuid(): mConnUuid=" + mConnUuid);
            return mConnUuid;
        }
    }

    private class AcceptThread extends Thread {
        private final BluetoothServerSocket mServerSocket;
        private final int mTimeout;
        private BluetoothSocket mSocket;
        String mConnUuid;

        AcceptThread(String uuid, int timeout) {
            BluetoothServerSocket tmp = null;
            mTimeout = timeout;
            try {
                tmp = mBluetoothAdapter.listenUsingRfcommWithServiceRecord(SDP_NAME,
                        UUID.fromString(uuid));
            } catch (IOException createSocketException) {
                Log.e("Failed to create socket: " + createSocketException.toString());
            }
            mServerSocket = tmp;
            Log.d("AcceptThread: uuid=" + uuid);
        }

        AcceptThread(int psmValue, int timeout, boolean isBle, boolean securedConn) {
            BluetoothServerSocket tmp = null;
            mTimeout = timeout;
            try {
                // Secured version: mBluetoothAdapter.listenUsingL2capOn(0x1011, false, false);
                if (isBle) {
                    /* Assigned a dynamic LE_PSM Value */
                    if (securedConn) {
                        tmp = mBluetoothAdapter.listenUsingL2capCoc(
                            BluetoothDevice.TRANSPORT_LE);
                    } else {
                        tmp = mBluetoothAdapter.listenUsingInsecureL2capCoc(
                            BluetoothDevice.TRANSPORT_LE);
                    }
                } else {
                    if (securedConn) {
                        tmp = mBluetoothAdapter.listenUsingL2capOn(psmValue);
                    } else {
                        tmp = mBluetoothAdapter.listenUsingInsecureL2capOn(psmValue);
                    }
                }
            } catch (IOException createSocketException) {
                Log.e("Failed to create Coc socket: " + createSocketException.toString());
            }
            mServerSocket = tmp;
            Log.d("AcceptThread: securedConn=" + securedConn + ", Old PSM value=" + psmValue
                        + ", new PSM=" + getPsm());
        }

        public void run() {
            try {
                mSocket = mServerSocket.accept(mTimeout);
                BluetoothConnection conn = new BluetoothConnection(mSocket, mServerSocket);
                mConnUuid = addConnection(conn);
                Log.d("AcceptThread:run: isConnected=" + mSocket.isConnected() + ", address="
                        + mSocket.getRemoteDevice().getAddress() + ", uuid=" + mConnUuid);
            } catch (IOException connectException) {
                Log.e("AcceptThread:run: Failed to connect socket: " + connectException.toString());
                if (mSocket != null) {
                    cancel();
                }
                return;
            }
        }

        public void cancel() {
            Log.d("AcceptThread:cancel: mmSocket=" + mSocket + ", mmServerSocket=" + mServerSocket);
            if (mSocket != null) {
                try {
                    mSocket.close();
                } catch (IOException closeException) {
                    Log.e("Failed to close socket: " + closeException.toString());
                }
            }
            if (mServerSocket != null) {
                try {
                    mServerSocket.close();
                } catch (IOException closeException) {
                    Log.e("Failed to close socket: " + closeException.toString());
                }
            }
        }

        public BluetoothSocket getSocket() {
            return mSocket;
        }

        public int getPsm() {
            return mServerSocket.getPsm();
        }

        public String getConnUuid() {
            Log.d("ConnectThread::getConnUuid(): mConnUuid=" + mConnUuid);
            return mConnUuid;
        }
    }
}
