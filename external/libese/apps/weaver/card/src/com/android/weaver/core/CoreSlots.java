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

import javacard.framework.ISO7816;
import javacard.framework.ISOException;
import javacard.framework.JCSystem;
import javacard.framework.Util;

import com.android.weaver.Consts;
import com.android.weaver.Slots;

import com.nxp.id.jcopx.util.DSTimer;

class CoreSlots implements Slots {
    static final byte NUM_SLOTS = 64;

    private Slot[] mSlots;

    CoreSlots() {
        // Allocate all memory up front
        mSlots = new Slot[NUM_SLOTS];
        for (short i = 0; i < NUM_SLOTS; ++i) {
            mSlots[i] = new Slot();
        }

        // Make the same size as the value so the whole buffer can be copied in read() so there is
        // no time difference between success and failure.
        Slot.sRemainingBackoff = JCSystem.makeTransientByteArray(
                Consts.SLOT_VALUE_BYTES, JCSystem.CLEAR_ON_RESET);
    }

    @Override
    public short getNumSlots() {
        return NUM_SLOTS;
    }

    @Override
    public void write(short rawSlotId, byte[] key, short keyOffset,
            byte[] value, short valueOffset) {
        final short slotId = validateSlotId(rawSlotId);
        mSlots[slotId].write(key, keyOffset, value, valueOffset);
    }

    @Override
    public byte read(short rawSlotId, byte[] key, short keyOffset,
            byte[] outValue, short outOffset) {
        final short slotId = validateSlotId(rawSlotId);
        return mSlots[slotId].read(key, keyOffset, outValue, outOffset);
    }

    @Override
    public void eraseValue(short rawSlotId) {
        final short slotId = validateSlotId(rawSlotId);
        mSlots[slotId].eraseValue();
    }

    @Override
    public void eraseAll() {
        for (short i = 0; i < NUM_SLOTS; ++i) {
            mSlots[i].erase();
        }
    }

    /**
     * Check the slot ID is within range and convert it to a short.
     */
    private short validateSlotId(short slotId) {
        // slotId is unsigned so if the signed version is negative then it is far too big
        if (slotId < 0 || slotId >= NUM_SLOTS) {
            ISOException.throwIt(Consts.SW_INVALID_SLOT_ID);
        }
        return slotId;
    }

    private static class Slot {
        private static byte[] sRemainingBackoff;

        private byte[] mKey = new byte[Consts.SLOT_KEY_BYTES];
        private byte[] mValue = new byte[Consts.SLOT_VALUE_BYTES];
        private short mFailureCount;
        private DSTimer mBackoffTimer;

        /**
         * Transactionally reset the slot with a new key and value.
         *
         * @param keyBuffer   the buffer containing the key data
         * @param keyOffset   the offset of the key in its buffer
         * @param valueBuffer the buffer containing the value data
         * @param valueOffset the offset of the value in its buffer
         */
        public void write(
                byte[] keyBuffer, short keyOffset, byte[] valueBuffer, short valueOffset) {
            JCSystem.beginTransaction();
            Util.arrayCopy(keyBuffer, keyOffset, mKey, (short) 0, Consts.SLOT_KEY_BYTES);
            Util.arrayCopy(valueBuffer, valueOffset, mValue, (short) 0, Consts.SLOT_VALUE_BYTES);
            mFailureCount = 0;
            mBackoffTimer = DSTimer.getInstance();
            JCSystem.commitTransaction();
        }

        /**
         * Clear the slot's value.
         */
        public void eraseValue() {
            // This is intended to be destructive so a partial update is not a problem
            Util.arrayFillNonAtomic(mValue, (short) 0, Consts.SLOT_VALUE_BYTES, (byte) 0);
        }

        /**
         * Transactionally clear the slot.
         */
        public void erase() {
            JCSystem.beginTransaction();
            arrayFill(mKey, (short) 0, Consts.SLOT_KEY_BYTES, (byte) 0);
            arrayFill(mValue, (short) 0, Consts.SLOT_VALUE_BYTES, (byte) 0);
            mFailureCount = 0;
            mBackoffTimer.stopTimer();
            JCSystem.commitTransaction();
        }

        /**
         * Copy the slot's value to the buffer if the provided key matches the slot's key.
         *
         * @param keyBuffer the buffer containing the key
         * @param keyOffset the offset of the key in its buffer
         * @param outBuffer the buffer to copy the value or backoff time into
         * @param outOffset the offset into the output buffer
         * @return status code
         */
        public byte read(byte[] keyBuffer, short keyOffset, byte[] outBuffer, short outOffset) {
            // Check timeout has expired or hasn't been started
            mBackoffTimer.getRemainingTime(sRemainingBackoff, (short) 0);
            if (sRemainingBackoff[0] != 0 || sRemainingBackoff[1] != 0 ||
                  sRemainingBackoff[2] != 0 || sRemainingBackoff[3] != 0) {
                Util.arrayCopyNonAtomic(
                        sRemainingBackoff, (short) 0, outBuffer, outOffset, (byte) 4);
                return Consts.READ_BACK_OFF;
            }

            // Check the key matches in constant time and copy out the value if it does
            final byte result = (Util.arrayCompare(
                    keyBuffer, keyOffset, mKey, (short) 0, Consts.SLOT_KEY_BYTES) == 0) ?
                    Consts.READ_SUCCESS : Consts.READ_WRONG_KEY;

            // Keep track of the number of failures
            if (result == Consts.READ_WRONG_KEY) {
                if (mFailureCount != 0x7fff) {
                    mFailureCount += 1;
                }
            } else {
                // This read was successful so reset the failures
                if (mFailureCount != 0) { // attempt to maintain constant time
                    mFailureCount = 0;
                }
            }

            // Start the timer on a failure
            if (throttle(sRemainingBackoff, (short) 0, mFailureCount)) {
                mBackoffTimer.startTimer(
                        sRemainingBackoff, (short) 0, DSTimer.DST_POWEROFFMODE_FALLBACK);
            } else {
                mBackoffTimer.stopTimer();
            }

            final byte[] data = (result == Consts.READ_SUCCESS) ? mValue : sRemainingBackoff;
            Util.arrayCopyNonAtomic(data, (short) 0, outBuffer, outOffset, Consts.SLOT_VALUE_BYTES);

            return result;
        }

        /**
         * 3.0.3 does not offer Util.arrayFill
         */
        private static void arrayFill(byte[] bArray, short bOff, short bLen, byte bValue) {
            for (short i = 0; i < bLen; ++i) {
                bArray[(short) (bOff + i)] = bValue;
            }
        }

        /**
         * Calculates the timeout in seconds as a function of the failure
         * counter 'x' as follows:
         *
         * [0, 5) -> 0
         * 5 -> 30
         * [6, 10) -> 0
         * [11, 30) -> 30
         * [30, 140) -> 30 * (2^((x - 30)/10))
         * [140, inf) -> 1 day
         *
         * The 32-bit timeout in seconds is written to the array.
         *
         * @return Whether there is any throttle time.
         */
        private static boolean throttle(byte[] bArray, short bOff, short failureCount) {
            short highWord = 0;
            short lowWord = 0;

            final short thirtySeconds = 30;
            if (failureCount == 0) {
                // 0s
            } else if (failureCount > 0 && failureCount <= 10) {
                if (failureCount % 5 == 0) {
                    // 30s
                  lowWord = thirtySeconds;
                }  else {
                    // 0s
                }
            } else if (failureCount < 30) {
                // 30s
                lowWord = thirtySeconds;
            } else if (failureCount < 140) {
                // 30 * (2^((x - 30)/10))
                final short shift = (short) ((short) (failureCount - 30) / 10);
                lowWord = (short) (thirtySeconds << shift);
            } else {
                // 1 day in seconds = 24 * 60 * 60 = 0x1 5180
                highWord = 0x1;
                lowWord = 0x5180;
            }

            // Write the value to the buffer
            Util.setShort(bArray, bOff, highWord);
            Util.setShort(bArray, (short) (bOff + 2), lowWord);

            return highWord != 0 || lowWord != 0;
        }
    }
}
