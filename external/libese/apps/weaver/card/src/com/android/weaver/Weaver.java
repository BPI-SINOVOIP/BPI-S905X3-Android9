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

package com.android.weaver;

import javacard.framework.AID;
import javacard.framework.APDU;
import javacard.framework.Applet;
import javacard.framework.ISO7816;
import javacard.framework.ISOException;
import javacard.framework.JCSystem;
import javacard.framework.Shareable;
import javacard.framework.Util;

public class Weaver extends Applet {
    // Keep constants in sync with esed
    // Uses the full AID which needs to be kept in sync on updates.
    public static final byte[] CORE_APPLET_AID
            = new byte[] {(byte) 0xA0, 0x00, 0x00, 0x04, 0x76, 0x57, 0x56,
                                 0x52, 0x43, 0x4F, 0x52, 0x45, 0x30,
                                 0x01, 0x01, 0x01};

    public static final byte CORE_APPLET_SLOTS_INTERFACE = 0;

    private Slots mSlots;

    protected Weaver() {
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
        new Weaver();
    }

    /**
     * Get a handle on the slots after the applet is registered but before and APDUs are received.
     */
    @Override
    public boolean select() {
      mSlots = null;
      return true;
    }

    /**
     * Processes an incoming APDU.
     *
     * @param apdu the incoming APDU
     * @exception ISOException with the response bytes per ISO 7816-4
     */
    @Override
    public void process(APDU apdu) {
        // TODO(drewry,ascull) Move this back into select.
        if (mSlots == null) {
            AID coreAid = JCSystem.lookupAID(CORE_APPLET_AID, (short) 0, (byte) CORE_APPLET_AID.length);
            if (coreAid == null) {
                ISOException.throwIt((short)0x0010);
            }

            mSlots = (Slots) JCSystem.getAppletShareableInterfaceObject(
                    coreAid, CORE_APPLET_SLOTS_INTERFACE);
            if (mSlots == null) {
                ISOException.throwIt((short)0x0012);
            }
        }


        final byte buffer[] = apdu.getBuffer();
        final byte cla = buffer[ISO7816.OFFSET_CLA];
        final byte ins = buffer[ISO7816.OFFSET_INS];

        // Handle standard commands
        if (apdu.isISOInterindustryCLA()) {
            switch (ins) {
                case ISO7816.INS_SELECT:
                    // Do nothing, successfully
                    return;
                default:
                    ISOException.throwIt(ISO7816.SW_INS_NOT_SUPPORTED);
            }
        }

        // Handle custom applet commands
        switch (ins) {
            case Consts.INS_GET_NUM_SLOTS:
                getNumSlots(apdu);
                return;

            case Consts.INS_WRITE:
                write(apdu);
                return;

            case Consts.INS_READ:
                read(apdu);
                return;

            case Consts.INS_ERASE_VALUE:
                eraseValue(apdu);
                return;

            case Consts.INS_ERASE_ALL:
                eraseAll(apdu);
                return;

            default:
                ISOException.throwIt(ISO7816.SW_INS_NOT_SUPPORTED);
        }
    }

    /**
     * Get the number of slots.
     *
     * p1: 0
     * p2: 0
     * data: _
     */
    private void getNumSlots(APDU apdu) {
        p1p2Unused(apdu);
        //dataUnused(apdu);
        // TODO(ascull): how to handle the cases of APDU properly?
        prepareToSend(apdu, (short) 4);
        apdu.setOutgoingLength((short) 4);

        final byte buffer[] = apdu.getBuffer();
        Util.setShort(buffer, (short) 0, (short) 0);
        Util.setShort(buffer, (short) 2, mSlots.getNumSlots());

        apdu.sendBytes((short) 0, (byte) 4);
    }

    public static final short WRITE_DATA_BYTES
            = Consts.SLOT_ID_BYTES + Consts.SLOT_KEY_BYTES + Consts.SLOT_VALUE_BYTES;
    private static final byte WRITE_DATA_SLOT_ID_OFFSET = ISO7816.OFFSET_CDATA;
    private static final byte WRITE_DATA_KEY_OFFSET
            = WRITE_DATA_SLOT_ID_OFFSET + Consts.SLOT_ID_BYTES;
    private static final byte WRITE_DATA_VALUE_OFFSET
            = WRITE_DATA_KEY_OFFSET + Consts.SLOT_KEY_BYTES;

    /**
     * Write to a slot.
     *
     * p1: 0
     * p2: 0
     * data: [slot ID] [key data] [value data]
     */
    private void write(APDU apdu) {
        p1p2Unused(apdu);
        receiveData(apdu, WRITE_DATA_BYTES);

        final byte buffer[] = apdu.getBuffer();
        final short slotId = getSlotId(buffer, WRITE_DATA_SLOT_ID_OFFSET);
        mSlots.write(slotId, buffer, WRITE_DATA_KEY_OFFSET, buffer, WRITE_DATA_VALUE_OFFSET);
    }

    public static final short READ_DATA_BYTES
            = Consts.SLOT_ID_BYTES + Consts.SLOT_KEY_BYTES;
    private static final byte READ_DATA_SLOT_ID_OFFSET = ISO7816.OFFSET_CDATA;
    private static final byte READ_DATA_KEY_OFFSET
            = WRITE_DATA_SLOT_ID_OFFSET + Consts.SLOT_ID_BYTES;

    /**
     * Read a slot.
     *
     * p1: 0
     * p2: 0
     * data: [slot ID] [key data]
     */
    private void read(APDU apdu) {
        final byte successSize = 1 + Consts.SLOT_VALUE_BYTES;
        final byte failSize = 1 + 4;

        p1p2Unused(apdu);
        receiveData(apdu, READ_DATA_BYTES);
        prepareToSend(apdu, successSize);

        final byte buffer[] = apdu.getBuffer();
        final short slotId = getSlotId(buffer, READ_DATA_SLOT_ID_OFFSET);

        final byte err = mSlots.read(slotId, buffer, READ_DATA_KEY_OFFSET, buffer, (short) 1);
        buffer[(short) 0] = err;
        if (err == Consts.READ_SUCCESS) {
            apdu.setOutgoingLength(successSize);
            apdu.sendBytes((short) 0, successSize);
        } else {
            apdu.setOutgoingLength(failSize);
            apdu.sendBytes((short) 0, failSize);
        }
    }

    public static final short ERASE_VALUE_BYTES = Consts.SLOT_ID_BYTES;
    private static final byte ERASE_VALUE_SLOT_ID_OFFSET = ISO7816.OFFSET_CDATA;

    /**
     * Erase the value of a slot.
     *
     * p1: 0
     * p2: 0
     * data: [slot ID]
     */
    private void eraseValue(APDU apdu) {
        p1p2Unused(apdu);
        receiveData(apdu, ERASE_VALUE_BYTES);

        final byte buffer[] = apdu.getBuffer();
        final short slotId = getSlotId(buffer, READ_DATA_SLOT_ID_OFFSET);
        mSlots.eraseValue(slotId);
    }

    /**
     * Erase all slots.
     *
     * p1: 0
     * p2: 0
     * data: _
     */
    private void eraseAll(APDU apdu) {
        p1p2Unused(apdu);
        dataUnused(apdu);
        mSlots.eraseAll();
    }

    /**
     * Check that the parameters are 0.
     *
     * They are not being used but should be under control.
     */
    private void p1p2Unused(APDU apdu) {
        final byte buffer[] = apdu.getBuffer();
        if (buffer[ISO7816.OFFSET_P1] != 0 || buffer[ISO7816.OFFSET_P2] != 0) {
            ISOException.throwIt(ISO7816.SW_INCORRECT_P1P2);
        }
    }

    /**
     * Check that no data was provided.
     */
    private void dataUnused(APDU apdu) {
        receiveData(apdu, (short) 0);
    }

    /**
     * Calls setIncomingAndReceive() on the APDU and checks the length is as expected.
     */
    private void receiveData(APDU apdu, short expectedLength) {
        final short bytesRead = apdu.setIncomingAndReceive();
        if (apdu.getIncomingLength() != expectedLength || bytesRead != expectedLength) {
            ISOException.throwIt(ISO7816.SW_WRONG_LENGTH);
        }
    }

    /**
     * The slot ID in the API is 32-bits but the applet works with 16-bit IDs.
     */
    private short getSlotId(byte[] bArray, short bOff) {
        if (bArray[bOff] != 0 || bArray[(short) (bOff + 1)] != 0) {
            ISOException.throwIt(Consts.SW_INVALID_SLOT_ID);
        }
        return Util.getShort(bArray,(short) (bOff + 2));
    }

    /**
     * Calls setOutgoing() on the APDU, checks the length is as expected and calls
     * setOutgoingLength() with that length.
     *
     * Still need to call setOutgoingLength() after this method.
     */
    private void prepareToSend(APDU apdu, short expectedMaxLength) {
        final short outDataLen = apdu.setOutgoing();
        if (outDataLen != expectedMaxLength) {
            ISOException.throwIt(ISO7816.SW_WRONG_LENGTH);
        }
    }
}
