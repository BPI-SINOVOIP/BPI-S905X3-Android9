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

package com.android.weaver.core;

import javacard.framework.AID;
import javacard.framework.APDU;
import javacard.framework.Applet;
import javacard.framework.Shareable;

class WeaverCore extends Applet {
    public static final byte[] COMMAPP_APPLET_AID
            = new byte[] {(byte) 0xA0, 0x00, 0x00, 0x04, 0x76, 0x57, 0x56,
                                 0x52, 0x43, 0x4F, 0x4D, 0x4D, 0x30};

    private CoreSlots mSlots;

    protected WeaverCore() {
        // Allocate all memory up front
        mSlots = new CoreSlots();
        register();
    }

    /**
     * Installs this applet.
     *
     * @param params the installation parameters
     * @param offset the starting offset of the parameters
     * @param length the length of the parameters
     */
    public static void install(byte[] params, short offset, byte length) {
        new WeaverCore();
    }

    /**
     * Returns and instance of the {@link Slots} interface.
     *
     * @param AID The requesting applet's AID must be that of the CoreApp.
     * @param arg Must be {@link #SLOTS_INTERFACE} else returns {@code null}.
     */
    @Override
    public Shareable getShareableInterfaceObject(AID clientAid, byte arg) {
        if (!clientAid.partialEquals(COMMAPP_APPLET_AID, (short) 0, (byte) COMMAPP_APPLET_AID.length)) {
            return null;
        }
        return (arg == 0) ? mSlots : null;
    }

    /**
     * Should never be called.
     */
    @Override
    public void process(APDU apdu) {
    }
}
