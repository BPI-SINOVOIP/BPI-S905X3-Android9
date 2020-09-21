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

package com.android.verifiedboot.storage;

import javacard.framework.CardRuntimeException;
import javacard.framework.Util;

import com.android.verifiedboot.storage.BackupInterface;
import com.android.verifiedboot.globalstate.owner.OwnerInterface;

class VersionStorage implements BackupInterface {
    final public static byte NUM_SLOTS = (byte) 8;
    final public static byte SLOT_BYTES = (byte) 8;
    final private static byte EXPORT_VERSION = (byte)0x01;
    private OwnerInterface globalState;
    private byte[] storage;

    public VersionStorage(OwnerInterface globalStateRef) {
      storage = new byte[NUM_SLOTS * SLOT_BYTES];
      globalState = globalStateRef;
      Util.arrayFillNonAtomic(storage, (short) 0, (short) storage.length, (byte) 0x00);
    }

    /**
     * Copies content from the given slot in |out| and returns true.
     *
     * @param slot slot number to retrieve
     * @param out array to copy the slot data to.
     * @param oOffset offset into |out| to start the copy at.
     * @return 0x0 on success and an error otherwise.
     */
    public short getSlot(byte slot, byte[] out, short oOffset) {
        if (slot > NUM_SLOTS - 1) {
            return 0x0001;
        }
        try {
            Util.arrayCopy(storage, (short)(SLOT_BYTES * slot),
                         out, oOffset, SLOT_BYTES);
        } catch (CardRuntimeException e) {
            return 0x0002;
        }
        return 0x0;
    }

    /**
     * Copies content for the given slot from |in| and returns true.
     *
     * @param slot slot number to retrieve
     * @param in array to copy the slot data from.
     * @param iOffset into |in| to start the copy at.
     * @return 0x0 on success or an error code.
     */
    public short setSlot(byte slot, byte[] in, short iOffset) {
        if (slot > NUM_SLOTS - 1) {
            return 0x0001;
        }
        // Slots can be set only if we're in the bootloader
        // or we're not yet in production.
        if (globalState.production() == true &&
            globalState.inBootloader() == false) {
            return 0x0003;
        }
        try {
            Util.arrayCopy(in, iOffset,
                     storage, (short)(SLOT_BYTES * slot), SLOT_BYTES);
        } catch (CardRuntimeException e) {
            return 0x0002;
        }
        return 0;
    }

    /**
     * {@inheritDoc}
     *
     * Checks the size and version prefix prior to copying.
     */
    @Override
    public boolean restore(byte[] inBytes, short inBytesOffset, short inBytesLength) {
        if (inBytesLength == (short) 0 ||
            inBytesLength > (short)(storage.length + 1)) {
            return false;
        }
        if (inBytes[0] != EXPORT_VERSION) {
            return false;
        }
        try {
            Util.arrayCopy(inBytes, inBytesOffset, storage, (short) 0, inBytesLength);
        } catch (CardRuntimeException e) {
            return false;
        }
        return true;
    }

    /**
     * {@inheritDoc}
     *
     * Copies storage to outBytes with a leading version byte which can be
     * checked on restore.
     */
    @Override
    public short backup(byte[] outBytes, short outBytesOffset) {
        try {
            // Tag the export version that way if any internal storage format changes
            // occur, they can be handled.
            outBytes[(short)(outBytesOffset + 1)] = EXPORT_VERSION;
            Util.arrayCopy(storage, (short) 0, outBytes, (short)(outBytesOffset + 1),
                           (short)storage.length);
            return (short)(storage.length + 1);
        } catch (CardRuntimeException e) {
            return 0x0;
        }
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public short backupSize() {
      return (short)(storage.length + 1);
    }
}
