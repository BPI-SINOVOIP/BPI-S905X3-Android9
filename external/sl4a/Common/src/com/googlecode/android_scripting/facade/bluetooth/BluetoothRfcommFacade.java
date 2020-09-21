/*
 * Copyright (C) 2017 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except
 * in compliance with the License. You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software distributed under the License
 * is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express
 * or implied. See the License for the specific language governing permissions and limitations under
 * the License.
 */

package com.googlecode.android_scripting.facade.bluetooth;

import com.googlecode.android_scripting.facade.FacadeManager;
import com.googlecode.android_scripting.rpc.Rpc;
import com.googlecode.android_scripting.rpc.RpcDefault;
import com.googlecode.android_scripting.rpc.RpcOptional;
import com.googlecode.android_scripting.rpc.RpcParameter;

import java.io.IOException;
import java.util.Map;

/**
 * Bluetooth functions.
 *
 */
// Discovery functions added by Eden Sayag

public class BluetoothRfcommFacade extends BluetoothSocketConnFacade {

    public BluetoothRfcommFacade(FacadeManager manager) {
        super(manager);
    }

    /**
     * Create L2CAP socket to Bluetooth device
     *
     * @param address the bluetooth address to open the socket on
     * @param channel the channel to open the socket on
     * @throws Exception
     */
    @Rpc(description = "Create L2CAP socket to Bluetooth deice")
    public void rfcommCreateL2capSocket(@RpcParameter(name = "address") String address,
            @RpcParameter(name = "channel") Integer channel) throws Exception {
        bluetoothSocketConnCreateL2capSocket(address, channel);
    }

    /**
     * Create RFCOMM socket to Bluetooth device
     *
     * @param address the bluetooth address to open the socket on
     * @param channel the channel to open the socket on
     * @throws Exception
     */
    @Rpc(description = "Create RFCOMM socket to Bluetooth deice")
    public void rfcommCreateRfcommSocket(@RpcParameter(name = "address") String address,
            @RpcParameter(name = "channel") Integer channel) throws Exception {
        bluetoothSocketConnCreateL2capSocket(address, channel);
    }

    /**
     * Begin Connect Thread
     *
     * @param address the mac address of the device to connect to
     * @param uuid the UUID that is used by the server device
     * @throws Exception
     */
    @Rpc(description = "Begins a thread initiate an Rfcomm connection over Bluetooth. ")
    public void bluetoothRfcommBeginConnectThread(
            @RpcParameter(name = "address",
            description = "The mac address of the device to connect to.") String address,
            @RpcParameter(name = "uuid",
            description = "The UUID passed here must match the UUID used by the server device.")
            @RpcDefault(DEFAULT_UUID) String uuid)
            throws IOException {
        bluetoothSocketConnBeginConnectThreadUuid(address, uuid);
    }

    /**
     * Kill thread
     */
    @Rpc(description = "Kill thread")
    public void bluetoothRfcommKillConnThread() {
        bluetoothSocketConnKillConnThread();
    }

    /**
     * Closes an active Rfcomm Client socket
     */
    @Rpc(description = "Close an active Rfcomm Client socket")
    public void bluetoothRfcommEndConnectThread() throws IOException {
        bluetoothSocketConnEndConnectThread();
    }

    /**
     * Closes an active Rfcomm Server socket
     */
    @Rpc(description = "Close an active Rfcomm Server socket")
    public void bluetoothRfcommEndAcceptThread() throws IOException {
        bluetoothSocketConnEndAcceptThread();
    }

    /**
     * Returns active Bluetooth mConnections
     */
    @Rpc(description = "Returns active Bluetooth mConnections.")
    public Map<String, String> bluetoothRfcommActiveConnections() {
        return bluetoothSocketConnActiveConnections();
    }

    /**
     * Returns the name of the connected device
     */
    @Rpc(description = "Returns the name of the connected device.")
    public String bluetoothRfcommGetConnectedDeviceName(
            @RpcParameter(name = "connID", description = "Connection id") @RpcOptional
            @RpcDefault("") String connID)
            throws IOException {
        return bluetoothSocketConnGetConnectedDeviceName(connID);
    }

    /**
     * Begins a thread to accept an Rfcomm connection over Bluetooth
     */
    @Rpc(description = "Begins a thread to accept an Rfcomm connection over Bluetooth. ")
    public void bluetoothRfcommBeginAcceptThread(
            @RpcParameter(name = "uuid") @RpcDefault(DEFAULT_UUID) String uuid,
            @RpcParameter(name = "timeout",
            description = "How long to wait for a new connection, 0 is wait for ever")
            @RpcDefault("0") Integer timeout)
            throws IOException {
        bluetoothSocketConnBeginAcceptThreadUuid(uuid, timeout);
    }

    /**
     * Sends ASCII characters over the currently open Bluetooth connection
     */
    @Rpc(description = "Sends ASCII characters over the currently open Bluetooth connection.")
    public void bluetoothRfcommWrite(@RpcParameter(name = "ascii") String ascii,
            @RpcParameter(name = "connID", description = "Connection id")
            @RpcDefault("") String connID)
            throws IOException {
        bluetoothSocketConnWrite(ascii, connID);
    }

    /**
     * Read up to bufferSize ASCII characters
     */
    @Rpc(description = "Read up to bufferSize ASCII characters.")
    public String bluetoothRfcommRead(
            @RpcParameter(name = "bufferSize") @RpcDefault("4096") Integer bufferSize,
            @RpcParameter(name = "connID", description = "Connection id") @RpcOptional
            @RpcDefault("") String connID)
            throws IOException {
        return bluetoothSocketConnRead(bufferSize, connID);
    }

    /**
     * Send bytes over the currently open Bluetooth connection
     */
    @Rpc(description = "Send bytes over the currently open Bluetooth connection.")
    public void bluetoothRfcommWriteBinary(
            @RpcParameter(name = "base64",
            description = "A base64 encoded String of the bytes to be sent.") String base64,
            @RpcParameter(name = "connID",
            description = "Connection id") @RpcDefault("") @RpcOptional String connID)
            throws IOException {
        bluetoothSocketConnWriteBinary(base64, connID);
    }

    /**
     * Read up to bufferSize bytes and return a chunked, base64 encoded string
     */
    @Rpc(description = "Read up to bufferSize bytes and return a chunked, base64 encoded string.")
    public String bluetoothRfcommReadBinary(
            @RpcParameter(name = "bufferSize") @RpcDefault("4096") Integer bufferSize,
            @RpcParameter(name = "connID", description = "Connection id") @RpcDefault("")
            @RpcOptional String connID)
            throws IOException {
        return bluetoothSocketConnReadBinary(bufferSize, connID);
    }

    /**
     * Returns True if the next read is guaranteed not to block
     */
    @Rpc(description = "Returns True if the next read is guaranteed not to block.")
    public Boolean bluetoothRfcommReadReady(
            @RpcParameter(name = "connID", description = "Connection id") @RpcDefault("")
            @RpcOptional String connID)
            throws IOException {
        return bluetoothSocketConnReadReady(connID);
    }

    /**
     * Read the next line
     */
    @Rpc(description = "Read the next line.")
    public String bluetoothRfcommReadLine(
            @RpcParameter(name = "connID", description = "Connection id") @RpcOptional
            @RpcDefault("") String connID)
            throws IOException {
        return bluetoothSocketConnReadLine(connID);
    }

    /**
     * Stops Bluetooth connection
     */
    @Rpc(description = "Stops Bluetooth connection.")
    public void bluetoothRfcommStop(
            @RpcParameter(name = "connID", description = "Connection id") @RpcOptional
            @RpcDefault("") String connID) {
        bluetoothSocketConnStop(connID);
    }

    @Override
    public void shutdown() {
        super.shutdown();
    }

}

