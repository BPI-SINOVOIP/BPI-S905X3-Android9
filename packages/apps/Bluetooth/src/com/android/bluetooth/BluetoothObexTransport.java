/*
* Copyright (C) 2014 Samsung System LSI
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

package com.android.bluetooth;

import android.bluetooth.BluetoothSocket;

import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;

import javax.obex.ObexTransport;

/**
 * Generic Obex Transport class, to be used in OBEX based Bluetooth
 * Profiles.
 */
public class BluetoothObexTransport implements ObexTransport {
    private BluetoothSocket mSocket = null;

    public BluetoothObexTransport(BluetoothSocket socket) {
        this.mSocket = socket;
    }

    @Override
    public void close() throws IOException {
        mSocket.close();
    }

    @Override
    public DataInputStream openDataInputStream() throws IOException {
        return new DataInputStream(openInputStream());
    }

    @Override
    public DataOutputStream openDataOutputStream() throws IOException {
        return new DataOutputStream(openOutputStream());
    }

    @Override
    public InputStream openInputStream() throws IOException {
        return mSocket.getInputStream();
    }

    @Override
    public OutputStream openOutputStream() throws IOException {
        return mSocket.getOutputStream();
    }

    @Override
    public void connect() throws IOException {
    }

    @Override
    public void create() throws IOException {
    }

    @Override
    public void disconnect() throws IOException {
    }

    @Override
    public void listen() throws IOException {
    }

    public boolean isConnected() throws IOException {
        return true;
    }

    @Override
    public int getMaxTransmitPacketSize() {
        if (mSocket.getConnectionType() != BluetoothSocket.TYPE_L2CAP) {
            return -1;
        }
        return mSocket.getMaxTransmitPacketSize();
    }

    @Override
    public int getMaxReceivePacketSize() {
        if (mSocket.getConnectionType() != BluetoothSocket.TYPE_L2CAP) {
            return -1;
        }
        return mSocket.getMaxReceivePacketSize();
    }

    public String getRemoteAddress() {
        if (mSocket == null) {
            return null;
        }
        return mSocket.getRemoteDevice().getAddress();
    }

    @Override
    public boolean isSrmSupported() {
        if (mSocket.getConnectionType() == BluetoothSocket.TYPE_L2CAP) {
            return true;
        }
        return false;
    }
}
