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
 * Copyright 2012 Giesecke & Devrient GmbH.
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not
 * use this file except in compliance with the License. You may obtain a copy of
 * the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
 * License for the specific language governing permissions and limitations under
 * the License.
 */

package com.android.se.security.ara;

import android.util.Log;

import com.android.se.Channel;
import com.android.se.Terminal;
import com.android.se.security.AccessRuleCache;
import com.android.se.security.ChannelAccess;
import com.android.se.security.gpac.BerTlv;
import com.android.se.security.gpac.ParserException;
import com.android.se.security.gpac.REF_AR_DO;
import com.android.se.security.gpac.Response_ALL_AR_DO;
import com.android.se.security.gpac.Response_DO_Factory;

import java.io.IOException;
import java.security.AccessControlException;
import java.util.ArrayList;
import java.util.Iterator;
import java.util.MissingResourceException;
import java.util.NoSuchElementException;

/** Reads and Maintains the ARA access for the Secure Element */
public class AraController {

    public static final byte[] ARA_M_AID =
            new byte[]{
                    (byte) 0xA0,
                    (byte) 0x00,
                    (byte) 0x00,
                    (byte) 0x01,
                    (byte) 0x51,
                    (byte) 0x41,
                    (byte) 0x43,
                    (byte) 0x4C,
                    (byte) 0x00
            };
    private final String mTag = "SecureElement-AraController";
    private AccessRuleCache mAccessRuleCache = null;
    private Terminal mTerminal = null;
    private AccessRuleApplet mApplet = null;

    public AraController(AccessRuleCache cache, Terminal terminal) {
        mAccessRuleCache = cache;
        mTerminal = terminal;
    }

    public static byte[] getAraMAid() {
        return ARA_M_AID;
    }

    /**
     * Initialize the AraController, reads the refresh tag.
     * and fetch the access rules
     */
    public synchronized void initialize() throws IOException, NoSuchElementException {
        Channel channel = mTerminal.openLogicalChannelWithoutChannelAccess(getAraMAid());
        if (channel == null) {
            throw new MissingResourceException("could not open channel", "", "");
        }

        // set access conditions to access ARA-M.
        ChannelAccess araChannelAccess = new ChannelAccess();
        araChannelAccess.setAccess(ChannelAccess.ACCESS.ALLOWED, mTag);
        araChannelAccess.setApduAccess(ChannelAccess.ACCESS.ALLOWED);
        channel.setChannelAccess(araChannelAccess);

        try {
            // set new applet handler since a new channel is used.
            mApplet = new AccessRuleApplet(channel);
            byte[] tag = mApplet.readRefreshTag();
            // if refresh tag is equal to the previous one it is not
            // neccessary to read all rules again.
            if (mAccessRuleCache.isRefreshTagEqual(tag)) {
                Log.i(mTag, "Refresh tag unchanged. Using access rules from cache.");
                return;
            }
            Log.i(mTag, "Refresh tag has changed.");
            // set new refresh tag and empty cache.
            mAccessRuleCache.setRefreshTag(tag);
            mAccessRuleCache.clearCache();

            Log.i(mTag, "Read ARs from ARA");
            readAllAccessRules();
        } catch (IOException e) {
            // Some kind of communication problem happened while transmit() was executed.
            // IOError shall be notified to the client application in this case.
            throw e;
        } catch (Exception e) {
            Log.i(mTag, "ARA error: " + e.getLocalizedMessage());
            throw new AccessControlException(e.getLocalizedMessage());
        } finally {
            if (channel != null) {
                channel.close();
            }
        }
    }

    private void readAllAccessRules() throws AccessControlException, IOException {
        try {
            byte[] data = mApplet.readAllAccessRules();
            // no data returned, but no exception
            // -> no rule.
            if (data == null) {
                return;
            }

            BerTlv tlv = Response_DO_Factory.createDO(data);
            if (tlv == null || !(tlv instanceof Response_ALL_AR_DO)) {
                throw new AccessControlException("No valid data object found");
            }
            ArrayList<REF_AR_DO> array = ((Response_ALL_AR_DO) tlv).getRefArDos();

            if (array != null && array.size() != 0) {
                Iterator<REF_AR_DO> iter = array.iterator();
                while (iter.hasNext()) {
                    REF_AR_DO refArDo = iter.next();
                    mAccessRuleCache.putWithMerge(refArDo.getRefDo(), refArDo.getArDo());
                }
            }
        } catch (ParserException e) {
            throw new AccessControlException("Parsing Data Object Exception: "
                    + e.getMessage());
        }
    }
}
