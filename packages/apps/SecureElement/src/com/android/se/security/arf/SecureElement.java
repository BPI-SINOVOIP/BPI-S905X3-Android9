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
 * Copyright (C) 2011 Deutsche Telekom, A.G.
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
 * Contributed by: Giesecke & Devrient GmbH.
 */

package com.android.se.security.arf;

import android.util.Log;

import com.android.se.Channel;
import com.android.se.Terminal;
import com.android.se.security.ChannelAccess;
import com.android.se.security.arf.pkcs15.EF;
import com.android.se.security.gpac.AID_REF_DO;
import com.android.se.security.gpac.Hash_REF_DO;
import com.android.se.security.gpac.REF_DO;

import java.io.IOException;
import java.util.MissingResourceException;
import java.util.NoSuchElementException;

/**
 * Provides high-level functions for SE communication
 */
public class SecureElement {
    public final String mTag = "SecureElement-ARF";
    // Logical channel used for SE communication (optional)
    private Channel mArfChannel = null;
    // Handle to a built-in "Secure Element"
    private Terminal mTerminalHandle = null;
    // Arf Controller within the SCAPI handler
    private ArfController mArfHandler = null;

    /**
     * Constructor
     *
     * @param arfHandler - handle to the owning arf controller object
     * @param handle     - handle to the SE terminal to be accessed.
     */
    public SecureElement(ArfController arfHandler, Terminal handle) {
        mTerminalHandle = handle;
        mArfHandler = arfHandler;
    }

    /**
     * Transmits ADPU commands
     *
     * @param cmd APDU command
     * @return Data returned by the APDU command
     */
    public byte[] exchangeAPDU(EF ef, byte[] cmd) throws IOException, SecureElementException {
        try {
            return mArfChannel.transmit(cmd);
        } catch (IOException e) {
            // Communication error happened while the terminal sending a command.
            throw e;
        } catch (Exception e) {
            throw new SecureElementException(
                    "Secure Element access error " + e.getLocalizedMessage());
        }
    }

    /**
     * Opens a logical channel to ARF Applet or ADF
     *
     * @param aid Applet identifier
     * @return Handle to "Logical Channel" allocated by the SE; <code>0</code> if error occurred
     */
    public Channel openLogicalArfChannel(byte[] aid) throws IOException, MissingResourceException,
            NoSuchElementException {
        try {
            mArfChannel = mTerminalHandle.openLogicalChannelWithoutChannelAccess(aid);
            if (mArfChannel == null) {
                throw new MissingResourceException("No channel was available", "", "");
            }
            setUpChannelAccess(mArfChannel);
            return mArfChannel;
        } catch (IOException | MissingResourceException | NoSuchElementException e) {
            throw e;
        } catch (Exception e) {
            Log.e(mTag, "Error opening logical channel " + e.getLocalizedMessage());
            mArfChannel = null;
            return null;
        }
    }

    /**
     * Closes a logical channel previously allocated by the SE
     */
    public void closeArfChannel() {
        try {
            if (mArfChannel != null) {
                mArfChannel.close();
                mArfChannel = null;
            }

        } catch (Exception e) {
            Log.e(mTag, "Error closing channel " + e.getLocalizedMessage());
        }
    }

    /**
     * Set up channel access to allow, so that PKCS15 files can be read.
     */
    private void setUpChannelAccess(Channel channel) {
        // set access conditions to access ARF.
        ChannelAccess arfChannelAccess = new ChannelAccess();
        arfChannelAccess.setAccess(ChannelAccess.ACCESS.ALLOWED, "");
        arfChannelAccess.setApduAccess(ChannelAccess.ACCESS.ALLOWED);
        channel.setChannelAccess(arfChannelAccess);
    }

    /** Fetches the Refresh Tag */
    public byte[] getRefreshTag() {
        if (mArfHandler != null) {
            return mArfHandler.getAccessRuleCache().getRefreshTag();
        }
        return null;
    }

    /** Sets the given Refresh Tag */
    public void setRefreshTag(byte[] refreshTag) {
        if (mArfHandler != null) {
            mArfHandler.getAccessRuleCache().setRefreshTag(refreshTag);
        }
    }

    /** Addsthe Access Rule to the Cache */
    public void putAccessRule(
            AID_REF_DO aidRefDo, Hash_REF_DO hashRefDo, ChannelAccess channelAccess) {

        REF_DO ref_do = new REF_DO(aidRefDo, hashRefDo);
        mArfHandler.getAccessRuleCache().putWithMerge(ref_do, channelAccess);
    }

    /** Resets the Access Rule Cache */
    public void resetAccessRules() {
        this.mArfHandler.getAccessRuleCache().reset();
    }

    /** Clears the Access Rule Cache */
    public void clearAccessRuleCache() {
        this.mArfHandler.getAccessRuleCache().clearCache();
    }
}
