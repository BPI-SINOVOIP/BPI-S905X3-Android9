//
// Copyright (C) 2017 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// Shared interface for accessing hardware state.
//

package com.android.verifiedboot.storage;

import javacard.framework.AID;
import javacard.framework.Applet;
import javacard.framework.ISO7816;
import javacard.framework.ISOException;
import javacard.framework.JCSystem;
import javacard.framework.Shareable;

import com.nxp.id.jcopx.util.SystemInfo;

import com.android.verifiedboot.globalstate.owner.OwnerInterface;
import com.android.verifiedboot.globalstate.callback.CallbackInterface;

class GlobalStateImpl implements OwnerInterface {
    // Used to track if a client needs to be renotified in case of a
    // power down, etc.
    final static byte CLIENT_STATE_CLEAR_PENDING = (byte)0x1;
    // Scenario values
    final static byte BOOTLOADER_UNDEFINED = (byte) 0xff;
    final static byte BOOTLOADER_HIGH = (byte) 0x5a;
    final static byte BOOTLOADER_LOW = (byte) 0xa5;

    private Object[] clientApplets;
    private byte[] clientAppletState;
    boolean inProduction;


    public GlobalStateImpl() {
        // Support up to 10 clients.
        clientApplets = new Object[10];
        // Used to store mask for each client.
        clientAppletState = new byte[clientApplets.length];
        // Used to signal the end of factory provisioning.
        inProduction = false;
    }

    /**
     * Returns the index of the given AID in clientApplets
     * if existent. Returns -1 if not found.
     *
     * @param aid The client applet AID to find.
     */
    private short findClientApplet(AID aid) {
        for (short i = 0; i < clientApplets.length; ++i) {
            if (clientApplets[i] == null) {
                continue;
            }
            if (((AID)clientApplets[i]).equals(aid)) {
                return i;
            }
        }
        return (short) -1;
    }

    /**
     * {@inheritDoc}
     *
     * It is expected that an applet can call notifyOnDataClear() on every
     * select.
     *
     * @param unregister If true, will remove the caller's AID from the registered store.
     *                   If false, will add the caller's AID to the register store.
     */
    @Override
    public boolean notifyOnDataClear(boolean unregister) {
        final AID aid = JCSystem.getPreviousContextAID();
        short firstFreeSlot = -1;
        for (short i = 0; i < clientApplets.length; ++i) {
            if (clientApplets[i] == null) {
                if (firstFreeSlot == -1) {
                    firstFreeSlot = i;
                }
                continue;
            }
            if (((AID)clientApplets[i]).equals(aid)) {
                if (unregister == true) {
                    clientApplets[i] = null;
                    // Clean up memory if we can.
                    if (JCSystem.isObjectDeletionSupported()) {
                        JCSystem.requestObjectDeletion();
                    }
                    return true;
                } else {
                    // Already registered.
                    return true;
                }
            }
        }
        // Not registered anyway.
        if (unregister == true) {
            return true;
        }
        // No spaces left.
        if (firstFreeSlot == -1) {
            return false;
        }
        clientApplets[firstFreeSlot] = aid;
        return true;
    }


    /**
     * {@inheritDoc}
     *
     * Processes an acknowledgement request from the given AID.
     */
    @Override
    public void reportDataCleared() {
        final AID aid = JCSystem.getPreviousContextAID();
        short id = findClientApplet(aid);
        if (id >= clientAppletState.length) {
            // This would be surprising.
            return;
        }
        if (id == (short) -1) {
            // Not found.
            return;
        }
        if ((clientAppletState[id] & CLIENT_STATE_CLEAR_PENDING)
                == CLIENT_STATE_CLEAR_PENDING) {
            clientAppletState[id] ^= CLIENT_STATE_CLEAR_PENDING;
        }
    }

    /**
     * {@inheritDoc}
     *
     * Returns true if data still needs to be cleared.
     */
    @Override
    public boolean dataClearNeeded() {
        final AID aid = JCSystem.getPreviousContextAID();
        short id = findClientApplet(aid);
        if (id >= clientAppletState.length) {
            // This would be surprising.
            return false;
        }
       if (id == (short) -1) {
           return false;
       }
        return ((clientAppletState[id] & CLIENT_STATE_CLEAR_PENDING)
                == CLIENT_STATE_CLEAR_PENDING);
    }

    /**
     * {@inheritDoc}
     *
     * Walks all clientApplets, and if they exist, checks if the have not
     * cleared the pending action bit. If any applets need to clear data,
     * false will be returned.
     */
    @Override
    public boolean globalDataClearComplete() {
        for (short i = 0; i < clientApplets.length; ++i) {
            if (clientApplets[i] == null) {
                continue;
            }
            if ((clientAppletState[i] & CLIENT_STATE_CLEAR_PENDING)
                == CLIENT_STATE_CLEAR_PENDING) {
                return false;
            }
        }
        return true;
    }

    /**
     * {@inheritDoc}
     *
     * Notifies all clients that a data clear is needed.
     *
     * If a power down or other interrupting event occurs, it is expected
     * that the clients will always call dataClearNeeded() prior to doing work
     * in process().
     *
     * However, the secure boot applet will check globalDataClearComplete()
     * on its first process() invocation after power on and call this method
     * with resume=true if a data clear was intended but incomplete.
     */
    @Override
    public void triggerDataClear(boolean resume) {
        // Two passes for a full
        // - First just sets the byte atomically.
        // - Second attempts notification.
        if (resume == false) {
            for (short i = 0; i < clientApplets.length; ++i) {
                if (clientApplets[i] == null) {
                    continue;
                }
                clientAppletState[i] |= CLIENT_STATE_CLEAR_PENDING;
            }
        }
        for (short i = 0; i < clientApplets.length; ++i) {
            if (clientApplets[i] == null) {
                continue;
            }
            if ((clientAppletState[i] & CLIENT_STATE_CLEAR_PENDING)
                == CLIENT_STATE_CLEAR_PENDING) {
                ((CallbackInterface) JCSystem.getAppletShareableInterfaceObject(
                        (AID)clientApplets[i], (byte)0)).clearData();
            }
        }
    }

    /**
     *  {@inheritDoc}
     *
     *  Returns true if the AP reset GPIO latch has not been cleared.
     */
    @Override
    public boolean inBootloader() {
        try {
            switch (SystemInfo.getExternalState(
                    SystemInfo.SYSTEMINFO_SCENARIO_0)) {
            case BOOTLOADER_HIGH:
                return true;
            case BOOTLOADER_LOW:
            case BOOTLOADER_UNDEFINED:
            default:
                return false;
            }
        } catch (ISOException e) {
            // If we can't read it, we fail closed unless we're in debug mode.
            return false;
        }
    }

    /**
     * Returns the external signal that feeds inBootloader().
     */
    public byte inBootloaderRaw() {
        try {
            return SystemInfo.getExternalState(SystemInfo.SYSTEMINFO_SCENARIO_0);
        } catch (ISOException e) {
            return (byte) 0x0;
        }
    }

    /**
     * {@inheritDoc}
     *
     * @param val value assigned to the inProduction value.
     * @return true if changed and false otherwise.
     */
    @Override
    public boolean setProduction(boolean val) {
        // Move to production is one way except for RMA - which
        // is handled in bootloader mode.
        if (val == false) {
            if (inBootloader() == false) {
                return false;
            }
        }
        inProduction = val;
        return true;
    }

    /**
     * {@inheritDoc}
     *
     * accessor for inProduction.
     */
    @Override
    public boolean production() {
        return inProduction;
    }
}
