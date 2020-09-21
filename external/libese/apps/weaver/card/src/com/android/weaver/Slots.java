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

public interface Slots extends javacard.framework.Shareable {
    /** @return The number of slots available. */
    short getNumSlots();

    /**
     * Write the key and value to the identified slot.
     *
     * @param slotId ID of the slot to write to.
     * @param key Buffer containing the key.
     * @param keyOffset Offset of the key in the buffer.
     * @param value Buffer containing the value.
     * @param valueOffset Offset of the value in the buffer.
     */
    void write(short slotId, byte[] key, short keyOffset, byte[] value, short valueOffset);

    /**
     * Read the value from the identified slot.
     *
     * This is only successful if the key matches that stored in the slot.
     *
     * @param slotId ID of the slot to write to.
     * @param key Buffer containing the key.
     * @param keyOffset Offset of the key in the buffer.
     * @param value Buffer to receive the value.
     * @param valueOffset Offset into the buffer to write the value.
     * @return Status byte indicating the success or otherwise of the read.
     */
    byte read(short slotId, byte[] key, short keyOffset, byte[] value, short valueOffset);

    /**
     * Set the value of the identified slot to all zeros whilst leaving the key untouched.
     *
     * This is used to destroy the secret stored in the slot but retain the ability to authenticate
     * by comparing a challenege with the slot's key.
     *
     * @param slotId ID of the slot of which to erase the value.
     */
    void eraseValue(short slotId);

    /** Erases the key and value of all slots. */
    void eraseAll();
}
