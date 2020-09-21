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

import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothServerSocket;
import android.bluetooth.BluetoothSocket;
import android.os.ParcelFileDescriptor;

import com.googlecode.android_scripting.Log;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.OutputStream;
import java.lang.reflect.Field;

/**
 * Class with Bluetooth Connection helper functions.
 *
 */

class BluetoothConnection {
    private BluetoothSocket mSocket;
    private BluetoothDevice mDevice;
    private OutputStream mOutputStream;
    private InputStream mInputStream;
    private BufferedReader mReader;
    private BluetoothServerSocket mServerSocket;
    private String mUuid;

    BluetoothConnection(BluetoothSocket mSocket) throws IOException {
        this(mSocket, null);
    }

    BluetoothConnection(BluetoothSocket mSocket, BluetoothServerSocket mServerSocket)
            throws IOException {
        this.mSocket = mSocket;
        mOutputStream = mSocket.getOutputStream();
        mInputStream = mSocket.getInputStream();
        mDevice = mSocket.getRemoteDevice();
        mReader = new BufferedReader(new InputStreamReader(mInputStream, "ASCII"));
        this.mServerSocket = mServerSocket;
    }

    /**
     * Set the UUID of this connection
     *
     * @param uuid the new UUID
     */
    public void setUUID(String uuid) {
        this.mUuid = uuid;
    }

    /**
     * Get this connection UUID
     *
     * @return String this connection UUID
     */
    public String getUUID() {
        return mUuid;
    }

    /**
     * Get the MAC address of remote device
     *
     * @return String the remote device MAC address
     */
    public String getRemoteBluetoothAddress() {
        return mDevice.getAddress();
    }

    /**
     * Get the connected state
     *
     * @return boolean TRUE if connected
     */
    public boolean isConnected() {
        if (mSocket == null) {
            return false;
        }
        try {
            mSocket.getRemoteDevice();
            mInputStream.available();
            mReader.ready();
            return true;
        } catch (Exception e) {
            return false;
        }
    }

    /**
     * Write an array of bytes to output stream
     *
     * @param out the array of bytes to write
     */
    public void write(byte[] out) throws IOException {
        if (mOutputStream != null) {
            mOutputStream.write(out);
        } else {
            throw new IOException("write: Bluetooth not ready.");
        }
    }

    /**
     * Write a string of bytes to output stream
     *
     * @param out the String of bytes to write
     */
    public void write(String out) throws IOException {
        this.write(out.getBytes());
    }

    /**
     * Write one byte to output stream
     *
     * @param out the byte to write
     */
    public void write(int out) throws IOException {
        if (mOutputStream != null) {
            mOutputStream.write(out);
        } else {
            throw new IOException("write: Bluetooth not ready.");
        }
    }

    /**
     * To test if the next data is ready to be read
     *
     * @return Boolean TRUE if data is ready to be read
     */
    public Boolean readReady() throws IOException {
        if (mReader != null) {
            return mReader.ready();
        }
        throw new IOException("Bluetooth not ready.");
    }

    /**
     * Read an array of data
     *
     * @return byte[] the buffer read
     */
    public byte[] readBinary() throws IOException {
        return this.readBinary(4096);
    }

    /**
     * Read an array of data of given size
     *
     * @param bufferSize the size to read
     *
     * @return byte[] the buffer containing read data
     */
    public byte[] readBinary(int bufferSize) throws IOException {
        if (mReader != null) {
            byte[] buffer = new byte[bufferSize];
            int bytesRead = mInputStream.read(buffer);
            if (bytesRead == -1) {
                Log.e("Read failed.");
                throw new IOException("Read failed.");
            }
            byte[] truncatedBuffer = new byte[bytesRead];
            System.arraycopy(buffer, 0, truncatedBuffer, 0, bytesRead);
            return truncatedBuffer;
        }

        throw new IOException("Bluetooth not ready.");

    }

    /**
     * Read a string of data
     *
     * @return String the read string
     */
    public String read() throws IOException {
        return this.read(4096);
    }

    /**
     * Read a string of data of given size
     *
     * @param bufferSize number of bytes to read
     *
     * @return String the read string
     */
    public String read(int bufferSize) throws IOException {
        if (mReader != null) {
            char[] buffer = new char[bufferSize];
            int bytesRead = mReader.read(buffer);
            if (bytesRead == -1) {
                Log.e("Read failed.");
                throw new IOException("Read failed.");
            }
            return new String(buffer, 0, bytesRead);
        }
        throw new IOException("Bluetooth not ready.");
    }

    /**
     * To read one byte
     *
     * @return int byte read
     */
    public int readbyte() throws IOException {
        if (mReader != null) {
            int byteRead = mReader.read();
            if (byteRead == -1) {
                Log.e("Read failed.");
                throw new IOException("Read failed.");
            }
            return byteRead;
        }
        throw new IOException("readbyte: Bluetooth not ready.");
    }

    /**
     * To read one line
     *
     * @return String line read
     */
    public String readLine() throws IOException {
        if (mReader != null) {
            return mReader.readLine();
        }
        throw new IOException("Bluetooth not ready.");
    }

    /**
     * Returns the name of the connected device
     *
     * @return String name of connected device
     */
    public String getConnectedDeviceName() {
        return mDevice.getName();
    }


    private synchronized void clearFileDescriptor() {
        try {
            Field field = BluetoothSocket.class.getDeclaredField("mPfd");
            field.setAccessible(true);
            ParcelFileDescriptor mPfd = (ParcelFileDescriptor) field.get(mSocket);
            Log.d("Closing mPfd: " + mPfd);
            if (mPfd == null) return;
            mPfd.close();
            mPfd = null;
            try {
                field.set(mSocket, mPfd);
            } catch (Exception e) {
                Log.d("Exception setting mPfd = null in cleanCloseFix(): " + e.toString());
            }
        } catch (Exception e) {
            Log.w("ParcelFileDescriptor could not be cleanly closed.", e);
        }
    }

    /**
     * To stop this connection
     */
    public void stop() {
        if (mSocket != null) {
            try {
                mSocket.close();
                clearFileDescriptor();
            } catch (IOException e) {
                Log.e(e);
            }
        }
        mSocket = null;
        if (mServerSocket != null) {
            try {
                mServerSocket.close();
            } catch (IOException e) {
                Log.e(e);
            }
        }
        mServerSocket = null;

        if (mInputStream != null) {
            try {
                mInputStream.close();
            } catch (IOException e) {
                Log.e(e);
            }
        }
        mInputStream = null;
        if (mOutputStream != null) {
            try {
                mOutputStream.close();
            } catch (IOException e) {
                Log.e(e);
            }
        }
        mOutputStream = null;
        if (mReader != null) {
            try {
                mReader.close();
            } catch (IOException e) {
                Log.e(e);
            }
        }
        mReader = null;
    }
}
