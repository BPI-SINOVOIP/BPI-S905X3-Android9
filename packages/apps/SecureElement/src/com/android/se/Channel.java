/*
 * Copyright (C) 2017 The Android Open Source Project
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
/*
 * Copyright (c) 2015-2017, The Linux Foundation.
 */
/*
 * Contributed by: Giesecke & Devrient GmbH.
 */

package com.android.se;

import android.os.Binder;
import android.os.IBinder;
import android.os.RemoteException;
import android.os.ServiceSpecificException;
import android.se.omapi.ISecureElementChannel;
import android.se.omapi.ISecureElementListener;
import android.se.omapi.SEService;
import android.util.Log;

import com.android.se.SecureElementService.SecureElementSession;
import com.android.se.security.ChannelAccess;

import java.io.IOException;

/**
 * Represents a Channel opened with the Secure Element
 */
public class Channel implements IBinder.DeathRecipient {

    private final String mTag = "SecureElement-Channel";
    private final int mChannelNumber;
    private final Object mLock = new Object();
    private IBinder mBinder = null;
    private boolean mIsClosed;
    private SecureElementSession mSession;
    private Terminal mTerminal;
    private byte[] mSelectResponse;
    private ChannelAccess mChannelAccess = null;
    private int mCallingPid = 0;
    private byte[] mAid = null;

    Channel(SecureElementSession session, Terminal terminal, int channelNumber,
            byte[] selectResponse, byte[] aid, ISecureElementListener listener) {
        if (terminal == null) {
            throw new IllegalArgumentException("Arguments can't be null");
        }
        mSession = session;
        mTerminal = terminal;
        mIsClosed = false;
        mSelectResponse = selectResponse;
        mChannelNumber = channelNumber;
        mAid = aid;
        if (listener != null) {
            try {
                mBinder = listener.asBinder();
                mBinder.linkToDeath(this, 0);
            } catch (RemoteException e) {
                Log.e(mTag, "Failed to register client listener");
            }
        }
    }

    /**
     * Close this channel if the client died.
     */
    public void binderDied() {
        try {
            Log.e(mTag, Thread.currentThread().getName() + " Client "
                    + mBinder.toString() + " died");
            close();
        } catch (Exception ignore) {
        }
    }

    /**
     * Closes the channel.
     */
    public synchronized void close() {
        synchronized (mLock) {
            if (isBasicChannel()) {
                Log.i(mTag, "Close basic channel - Select without AID ...");
                mTerminal.selectDefaultApplication();
            }

            mTerminal.closeChannel(this);
            mIsClosed = true;
            if (mBinder != null) {
                mBinder.unlinkToDeath(this, 0);
            }
            if (mSession != null) {
                mSession.removeChannel(this);
            }
        }
    }

    /**
     * Transmits the given byte and returns the response.
     */
    public byte[] transmit(byte[] command) throws IOException {
        if (isClosed()) {
            throw new IllegalStateException("Channel is closed");
        }
        if (command == null) {
            throw new NullPointerException("Command must not be null");
        }
        if (mChannelAccess == null) {
            throw new SecurityException("Channel access not set");
        }
        if (mChannelAccess.getCallingPid() != mCallingPid) {
            throw new SecurityException("Wrong Caller PID.");
        }

        // Validate the APDU command format and throw IllegalArgumentException, if necessary.
        CommandApduValidator.execute(command);

        if (((command[0] & (byte) 0x80) == 0)
                && ((command[0] & (byte) 0x60) != (byte) 0x20)) {
            // ISO command
            if (command[1] == (byte) 0x70) {
                throw new SecurityException("MANAGE CHANNEL command not allowed");
            }
            if ((command[1] == (byte) 0xA4) && (command[2] == (byte) 0x04)) {
                throw new SecurityException("SELECT by DF name command not allowed");
            }
        }

        checkCommand(command);
        synchronized (mLock) {
            // set channel number bits
            command[0] = setChannelToClassByte(command[0], mChannelNumber);
            return mTerminal.transmit(command);
        }
    }

    private boolean selectNext() throws IOException {
        if (isClosed()) {
            throw new IllegalStateException("Channel is closed");
        } else if (mChannelAccess == null) {
            throw new IllegalStateException("Channel access not set.");
        } else if (mChannelAccess.getCallingPid() != mCallingPid) {
            throw new SecurityException("Wrong Caller PID.");
        } else if (mAid == null || mAid.length == 0) {
            throw new UnsupportedOperationException("No aid given");
        }

        byte[] selectCommand = new byte[5 + mAid.length];
        selectCommand[0] = 0x00;
        selectCommand[1] = (byte) 0xA4;
        selectCommand[2] = 0x04;
        selectCommand[3] = 0x02; // next occurrence
        selectCommand[4] = (byte) mAid.length;
        System.arraycopy(mAid, 0, selectCommand, 5, mAid.length);

        // set channel number bits
        selectCommand[0] = setChannelToClassByte(selectCommand[0], mChannelNumber);

        byte[] bufferSelectResponse = mTerminal.transmit(selectCommand);

        if (bufferSelectResponse.length < 2) {
            throw new UnsupportedOperationException("Transmit failed");
        }
        int sw1 = bufferSelectResponse[bufferSelectResponse.length - 2] & 0xFF;
        int sw2 = bufferSelectResponse[bufferSelectResponse.length - 1] & 0xFF;
        int sw = (sw1 << 8) | sw2;

        if (((sw & 0xF000) == 0x9000) || ((sw & 0xFF00) == 0x6200)
                || ((sw & 0xFF00) == 0x6300)) {
            mSelectResponse = bufferSelectResponse.clone();
            return true;
        } else if ((sw & 0xFF00) == 0x6A00) {
            return false;
        } else {
            throw new UnsupportedOperationException("Unsupported operation");
        }
    }

    /**
     * Returns a copy of the given CLA byte where the channel number bits are set
     * as specified by the given channel number
     *
     * <p>See GlobalPlatform Card Specification 2.2.0.7: 11.1.4 Class Byte Coding
     *
     * @param cla           the CLA byte. Won't be modified
     * @param channelNumber within [0..3] (for first inter-industry class byte
     *                      coding) or [4..19] (for further inter-industry class byte coding)
     * @return the CLA byte with set channel number bits. The seventh bit
     * indicating the used coding
     * (first/further interindustry class byte coding) might be modified
     */
    private byte setChannelToClassByte(byte cla, int channelNumber) {
        if (channelNumber < 4) {
            // b7 = 0 indicates the first interindustry class byte coding
            cla = (byte) ((cla & 0xBC) | channelNumber);
        } else if (channelNumber < 20) {
            // b7 = 1 indicates the further interindustry class byte coding
            boolean isSm = (cla & 0x0C) != 0;
            cla = (byte) ((cla & 0xB0) | 0x40 | (channelNumber - 4));
            if (isSm) {
                cla |= 0x20;
            }
        } else {
            throw new IllegalArgumentException("Channel number must be within [0..19]");
        }
        return cla;
    }

    public ChannelAccess getChannelAccess() {
        return this.mChannelAccess;
    }

    public void setChannelAccess(ChannelAccess channelAccess) {
        this.mChannelAccess = channelAccess;
    }

    private void setCallingPid(int pid) {
        mCallingPid = pid;
    }

    private void checkCommand(byte[] command) {
        if (mTerminal.getAccessControlEnforcer() != null) {
            // check command if it complies to the access rules.
            // if not an exception is thrown
            mTerminal.getAccessControlEnforcer().checkCommand(this, command);
        } else {
            throw new SecurityException("Access Controller not set for Terminal: "
                    + mTerminal.getName());
        }
    }

    /**
     * true if aid could be selected during opening the channel
     * false if aid could not be or was not selected.
     *
     * @return boolean.
     */
    public boolean hasSelectedAid() {
        return (mAid != null);
    }

    public int getChannelNumber() {
        return mChannelNumber;
    }

    /**
     * Returns the data as received from the application select command
     * inclusively the status word.
     *
     * The returned byte array contains the data bytes in the following order:
     * first data byte, ... , last data byte, sw1, sw2
     *
     * @return null if an application SELECT command has not been performed or
     * the selection response can not be retrieved by the reader
     * implementation.
     */
    public byte[] getSelectResponse() {
        return (hasSelectedAid() ? mSelectResponse : null);
    }

    public boolean isBasicChannel() {
        return (mChannelNumber == 0) ? true : false;
    }

    public boolean isClosed() {
        return mIsClosed;
    }

    // Implementation of the SecureElement Channel interface according to OMAPI.
    final class SecureElementChannel extends ISecureElementChannel.Stub {

        @Override
        public void close() throws RemoteException {
            Channel.this.close();
        }

        @Override
        public boolean isClosed() throws RemoteException {
            return Channel.this.isClosed();
        }

        @Override
        public boolean isBasicChannel() throws RemoteException {
            return Channel.this.isBasicChannel();
        }

        @Override
        public byte[] getSelectResponse() throws RemoteException {
            return Channel.this.getSelectResponse();
        }

        @Override
        public byte[] transmit(byte[] command) throws RemoteException {
            Channel.this.setCallingPid(Binder.getCallingPid());
            try {
                return Channel.this.transmit(command);
            } catch (IOException e) {
                throw new ServiceSpecificException(SEService.IO_ERROR, e.getMessage());
            }
        }

        @Override
        public boolean selectNext() throws RemoteException {
            Channel.this.setCallingPid(Binder.getCallingPid());
            try {
                return Channel.this.selectNext();
            } catch (IOException e) {
                throw new ServiceSpecificException(SEService.IO_ERROR, e.getMessage());
            }
        }
    }
}
