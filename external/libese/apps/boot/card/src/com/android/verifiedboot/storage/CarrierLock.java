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
import javacard.framework.JCSystem;
import javacard.framework.Util;

import javacard.security.KeyBuilder;
import javacard.security.MessageDigest;
import javacard.security.RSAPublicKey;
import javacard.security.Signature;

import com.android.verifiedboot.storage.LockInterface;
import com.android.verifiedboot.globalstate.owner.OwnerInterface;

class CarrierLock implements LockInterface, BackupInterface {
    private final static byte VERSION = (byte) 1;
    private final static byte VERSION_SIZE = (byte) 8;
    private final static byte NONCE_SIZE = (byte) 8;
    private final static byte DEVICE_DATA_SIZE = (byte) (256 / 8);

    private final static byte[] PK_EXP = { (byte) 0x01, (byte) 0x00, (byte) 0x01 };  /* 65537 */
    // Production key.
    private final static byte[] PK_MOD = {
        (byte) 0xA3, (byte) 0x19, (byte) 0x27, (byte) 0x0B, (byte) 0xC6,
        (byte) 0x3C, (byte) 0xC0, (byte) 0x92, (byte) 0x38, (byte) 0x7D,
        (byte) 0xE3, (byte) 0xC1, (byte) 0xAE, (byte) 0xDD, (byte) 0x2C,
        (byte) 0xAA, (byte) 0x1C, (byte) 0x93, (byte) 0x23, (byte) 0xAA,
        (byte) 0x13, (byte) 0xF2, (byte) 0x0D, (byte) 0x03, (byte) 0x5F,
        (byte) 0xB8, (byte) 0x98, (byte) 0xA8, (byte) 0xFA, (byte) 0x57,
        (byte) 0xE9, (byte) 0xBF, (byte) 0x15, (byte) 0xE3, (byte) 0xAC,
        (byte) 0xB5, (byte) 0x64, (byte) 0xE7, (byte) 0x18, (byte) 0x85,
        (byte) 0xE1, (byte) 0xE4, (byte) 0xF0, (byte) 0x36, (byte) 0x81,
        (byte) 0x57, (byte) 0xA8, (byte) 0x78, (byte) 0x70, (byte) 0xDF,
        (byte) 0x92, (byte) 0x06, (byte) 0xCF, (byte) 0xEE, (byte) 0x1A,
        (byte) 0x6B, (byte) 0xE8, (byte) 0x50, (byte) 0x28, (byte) 0xD9,
        (byte) 0x54, (byte) 0x03, (byte) 0x6E, (byte) 0xF2, (byte) 0x6C,
        (byte) 0x06, (byte) 0xCE, (byte) 0x02, (byte) 0x8A, (byte) 0xF4,
        (byte) 0x86, (byte) 0x07, (byte) 0xA8, (byte) 0xE6, (byte) 0x6C,
        (byte) 0x1F, (byte) 0xFE, (byte) 0xB4, (byte) 0x83, (byte) 0x79,
        (byte) 0x40, (byte) 0x02, (byte) 0x25, (byte) 0xBD, (byte) 0x6B,
        (byte) 0x67, (byte) 0x03, (byte) 0xEB, (byte) 0xF2, (byte) 0xC7,
        (byte) 0x74, (byte) 0xB9, (byte) 0xE8, (byte) 0x35, (byte) 0x76,
        (byte) 0x4C, (byte) 0x1D, (byte) 0xE7, (byte) 0x34, (byte) 0x72,
        (byte) 0x6C, (byte) 0x0E, (byte) 0xCE, (byte) 0xD6, (byte) 0x2C,
        (byte) 0x86, (byte) 0x59, (byte) 0x58, (byte) 0x10, (byte) 0x00,
        (byte) 0x7F, (byte) 0x70, (byte) 0xF7, (byte) 0x4A, (byte) 0x2F,
        (byte) 0xED, (byte) 0x39, (byte) 0x46, (byte) 0xAC, (byte) 0x3A,
        (byte) 0x32, (byte) 0x32, (byte) 0x0F, (byte) 0x7A, (byte) 0x5C,
        (byte) 0x8A, (byte) 0x07, (byte) 0xDE, (byte) 0xA1, (byte) 0x8F,
        (byte) 0x74, (byte) 0xD8, (byte) 0x99, (byte) 0x3A, (byte) 0xE0,
        (byte) 0x9A, (byte) 0x40, (byte) 0x80, (byte) 0x51, (byte) 0x1F,
        (byte) 0xAD, (byte) 0x4D, (byte) 0x2A, (byte) 0x1D, (byte) 0x53,
        (byte) 0xC3, (byte) 0x66, (byte) 0x65, (byte) 0x59, (byte) 0x6D,
        (byte) 0x40, (byte) 0xB8, (byte) 0x71, (byte) 0xB5, (byte) 0xD4,
        (byte) 0x50, (byte) 0x3E, (byte) 0x41, (byte) 0xE0, (byte) 0x14,
        (byte) 0x25, (byte) 0x80, (byte) 0xA9, (byte) 0x0C, (byte) 0x76,
        (byte) 0xD4, (byte) 0x6C, (byte) 0x48, (byte) 0x0F, (byte) 0x08,
        (byte) 0x5A, (byte) 0xCD, (byte) 0xE5, (byte) 0x28, (byte) 0x58,
        (byte) 0xA5, (byte) 0x35, (byte) 0x10, (byte) 0x5D, (byte) 0x05,
        (byte) 0xB0, (byte) 0xE1, (byte) 0x26, (byte) 0xD3, (byte) 0x08,
        (byte) 0xE9, (byte) 0x5D, (byte) 0xB3, (byte) 0x77, (byte) 0x19,
        (byte) 0xD7, (byte) 0xC3, (byte) 0xA7, (byte) 0x3E, (byte) 0x09,
        (byte) 0x01, (byte) 0x75, (byte) 0x14, (byte) 0x49, (byte) 0x5D,
        (byte) 0x21, (byte) 0xBA, (byte) 0x8D, (byte) 0x74, (byte) 0x0A,
        (byte) 0x45, (byte) 0xCA, (byte) 0x39, (byte) 0x24, (byte) 0x94,
        (byte) 0x33, (byte) 0x0F, (byte) 0x35, (byte) 0x40, (byte) 0x70,
        (byte) 0x0B, (byte) 0x6C, (byte) 0xF7, (byte) 0x93, (byte) 0x35,
        (byte) 0x9A, (byte) 0x40, (byte) 0x72, (byte) 0xD7, (byte) 0xDD,
        (byte) 0xA5, (byte) 0xAA, (byte) 0x2A, (byte) 0x7B, (byte) 0x32,
        (byte) 0xF6, (byte) 0x56, (byte) 0x71, (byte) 0xC6, (byte) 0xAB,
        (byte) 0xEB, (byte) 0xFB, (byte) 0xCD, (byte) 0x27, (byte) 0xA1,
        (byte) 0x4C, (byte) 0xDA, (byte) 0xA4, (byte) 0xB1, (byte) 0x66,
        (byte) 0x2D, (byte) 0x57, (byte) 0x4B, (byte) 0x0D, (byte) 0x86,
        (byte) 0xD0, (byte) 0x98, (byte) 0x4B, (byte) 0x71, (byte) 0x8D,
        (byte) 0xF5,
    };


    /* Development key
    private final static byte[] PK_MOD = {
        (byte) 0xAE, (byte) 0x14, (byte) 0xA4, (byte) 0x91, (byte) 0xA6,
        (byte) 0xC8, (byte) 0x2E, (byte) 0x4D, (byte) 0x6B, (byte) 0xB3,
        (byte) 0x4E, (byte) 0x23, (byte) 0x96, (byte) 0x57, (byte) 0x7C,
        (byte) 0x2C, (byte) 0x7E, (byte) 0x69, (byte) 0xE6, (byte) 0xBF,
        (byte) 0x5A, (byte) 0x9C, (byte) 0xD7, (byte) 0xA8, (byte) 0x38,
        (byte) 0x0C, (byte) 0x9A, (byte) 0x54, (byte) 0x43, (byte) 0x4C,
        (byte) 0x3C, (byte) 0xDA, (byte) 0xC5, (byte) 0xB1, (byte) 0x58,
        (byte) 0x56, (byte) 0x9B, (byte) 0x5A, (byte) 0x05, (byte) 0xBA,
        (byte) 0x2C, (byte) 0xAB, (byte) 0xC6, (byte) 0x50, (byte) 0x34,
        (byte) 0x3C, (byte) 0x3B, (byte) 0x8E, (byte) 0xD8, (byte) 0x55,
        (byte) 0xEB, (byte) 0xFA, (byte) 0x4F, (byte) 0x72, (byte) 0x81,
        (byte) 0xA3, (byte) 0x8F, (byte) 0xDD, (byte) 0x8E, (byte) 0x0E,
        (byte) 0xF2, (byte) 0xF6, (byte) 0xEF, (byte) 0x18, (byte) 0x95,
        (byte) 0xCF, (byte) 0x71, (byte) 0x7D, (byte) 0x33, (byte) 0xA1,
        (byte) 0xAE, (byte) 0xBE, (byte) 0x8C, (byte) 0xA5, (byte) 0x50,
        (byte) 0x4C, (byte) 0xF2, (byte) 0xDC, (byte) 0x7B, (byte) 0x6C,
        (byte) 0xAE, (byte) 0x14, (byte) 0x95, (byte) 0xB7, (byte) 0xE7,
        (byte) 0xCA, (byte) 0xEB, (byte) 0xB0, (byte) 0x24, (byte) 0x5B,
        (byte) 0xC9, (byte) 0x24, (byte) 0x2B, (byte) 0xC6, (byte) 0x96,
        (byte) 0x99, (byte) 0xE9, (byte) 0x8B, (byte) 0x10, (byte) 0xCA,
        (byte) 0x34, (byte) 0x2D, (byte) 0x84, (byte) 0x57, (byte) 0x09,
        (byte) 0x4C, (byte) 0x32, (byte) 0x35, (byte) 0x68, (byte) 0x37,
        (byte) 0x53, (byte) 0x0E, (byte) 0xF6, (byte) 0x93, (byte) 0x6C,
        (byte) 0x86, (byte) 0x84, (byte) 0xC1, (byte) 0x44, (byte) 0x70,
        (byte) 0x4A, (byte) 0x12, (byte) 0xAA, (byte) 0xC2, (byte) 0x9F,
        (byte) 0x68, (byte) 0x5C, (byte) 0x42, (byte) 0xC8, (byte) 0xEB,
        (byte) 0xD3, (byte) 0xAF, (byte) 0xD6, (byte) 0x34, (byte) 0x7F,
        (byte) 0x9D, (byte) 0xC9, (byte) 0xE8, (byte) 0x81, (byte) 0x4A,
        (byte) 0x5C, (byte) 0xDA, (byte) 0x36, (byte) 0x33, (byte) 0xFD,
        (byte) 0x5C, (byte) 0x67, (byte) 0xBB, (byte) 0x91, (byte) 0x1C,
        (byte) 0xF5, (byte) 0x21, (byte) 0xC0, (byte) 0x4E, (byte) 0x64,
        (byte) 0x87, (byte) 0x89, (byte) 0xB6, (byte) 0x8B, (byte) 0xFD,
        (byte) 0xDA, (byte) 0x30, (byte) 0x74, (byte) 0x1E, (byte) 0x00,
        (byte) 0x57, (byte) 0xE1, (byte) 0x5C, (byte) 0xC4, (byte) 0xF2,
        (byte) 0xEE, (byte) 0xF7, (byte) 0x05, (byte) 0x1C, (byte) 0xCE,
        (byte) 0xF1, (byte) 0xCA, (byte) 0x88, (byte) 0xA0, (byte) 0x28,
        (byte) 0x53, (byte) 0x2C, (byte) 0x84, (byte) 0xCD, (byte) 0xA3,
        (byte) 0x6C, (byte) 0x1D, (byte) 0x15, (byte) 0x00, (byte) 0x5A,
        (byte) 0x5D, (byte) 0x80, (byte) 0x40, (byte) 0x59, (byte) 0xE5,
        (byte) 0xEA, (byte) 0xD1, (byte) 0x2A, (byte) 0xD6, (byte) 0x5A,
        (byte) 0xE0, (byte) 0xE6, (byte) 0x9C, (byte) 0xEB, (byte) 0x23,
        (byte) 0x4D, (byte) 0xD0, (byte) 0xB1, (byte) 0x27, (byte) 0xEE,
        (byte) 0x41, (byte) 0x0D, (byte) 0xAA, (byte) 0x25, (byte) 0xBD,
        (byte) 0xA0, (byte) 0xD0, (byte) 0x20, (byte) 0x00, (byte) 0x16,
        (byte) 0x1F, (byte) 0x54, (byte) 0xC6, (byte) 0x4A, (byte) 0xDD,
        (byte) 0x2A, (byte) 0x7E, (byte) 0x32, (byte) 0x43, (byte) 0x7F,
        (byte) 0xD8, (byte) 0x74, (byte) 0x0F, (byte) 0x94, (byte) 0x88,
        (byte) 0x3F, (byte) 0x26, (byte) 0x27, (byte) 0x54, (byte) 0x5D,
        (byte) 0x01, (byte) 0x83, (byte) 0xAE, (byte) 0x47, (byte) 0x37,
        (byte) 0x03, (byte) 0x6C, (byte) 0x80, (byte) 0xFD, (byte) 0x6E,
        (byte) 0x08, (byte) 0xEB, (byte) 0xB4, (byte) 0x55, (byte) 0x81,
        (byte) 0x13,
    };
    */

    // Layout:
    // LockValue (byte) || lastNonce (byte[8]) || deviceDataHash (byte[32])
    private byte[] storage;
    private short storageOffset;
    RSAPublicKey verifyingKey;
    Signature verifier;
    MessageDigest md_sha256;  /* For creating the lock data hash from the input. */
    OwnerInterface globalState;

    /**
     * Initializes the instance objects.
     */
    public CarrierLock() {
        try {
            verifyingKey = (RSAPublicKey)KeyBuilder.buildKey(
                KeyBuilder.TYPE_RSA_PUBLIC, KeyBuilder.LENGTH_RSA_2048, false);
            verifyingKey.setExponent(PK_EXP, (short)0, (short)PK_EXP.length);
            verifyingKey.setModulus(PK_MOD, (short)0, (short)PK_MOD.length);
        } catch (CardRuntimeException e) {
          verifyingKey = null;
        }

        try {
            verifier = Signature.getInstance(Signature.ALG_RSA_SHA_256_PKCS1, false);
            verifier.init(verifyingKey, Signature.MODE_VERIFY);
        } catch (CardRuntimeException e) {
          verifier = null;
        }
        md_sha256 = MessageDigest.getInstance(MessageDigest.ALG_SHA_256, false);
    }

    /**
     * {@inheritDoc}
     *
     * Return the error states useful for diagnostics.
     */
    @Override
    public short initialized() {
      if (storage == null) {
          return 1;
      }
      if (verifyingKey == null) {
          return 2;
      }
      if (verifier == null) {
          return 3;
      }
      return 0;
    }
    /**
     * {@inheritDoc}
     *
     */
    @Override
    public short getStorageNeeded() {
        return NONCE_SIZE + DEVICE_DATA_SIZE + 1;
    }

    /**
     * Sets the backing store to use for state.
     *
     * @param globalStateOwner interface for querying global state
     * @param extStorage  external array to use for storage
     * @param extStorageOffset where to begin storing data
     *
     * This should be called before use.
     */
    @Override
    public void initialize(OwnerInterface globalStateOwner, byte[] extStorage,
                           short extStorageOffset) {
        globalState = globalStateOwner;
        // Zero it first (in case we are interrupted).
        Util.arrayFillNonAtomic(extStorage, extStorageOffset,
                                getStorageNeeded(), (byte) 0x00);
        storage = extStorage;
        storageOffset = extStorageOffset;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public short get(byte[] lockOut, short outOffset) {
        if (storage == null) {
            return 0x0001;
        }
        try {
            Util.arrayCopy(storage, lockOffset(),
                           lockOut, outOffset, (short) 1);
        } catch (CardRuntimeException e) {
          return 0x0002;
        }
        return 0;
    }

    /**
     * {@inheritDoc}
     *
     * Returns 0xffff if {@link #initialize()} has not yet been called.
     */
    @Override
    public short lockOffset() {
        if (storage == null) {
            return (short) 0xffff;
        }
        return storageOffset;
    }


    /**
     * {@inheritDoc}
     *
     * Returns 0xffff if {@link #initialize()} has not yet been called.
     */
    @Override
    public short metadataOffset() {
        if (storage == null) {
            return (short) 0xffff;
        }
        return (short)(storageOffset + NONCE_SIZE + 1);
    }

    /**
     * {@inheritDoc}
     *
     * Returns length of metadata.
     *
     * @return length of metadata.
     */
    public short metadataLength() {
        return (short) DEVICE_DATA_SIZE;
    }

    /**
     * {@inheritDoc}
     *
     * Always returns false. Locking CarrierLock requires
     * device data and unlocking (val=0x0) requires a signed
     * assertion.
     */
    @Override
    public short set(byte val) {
        return (short)0xffff;  // Not implemented.
    }

    /**
     * Performs the verification of the incoming unlock token.
     *
     * This will check the version code, the nonce value, and the signature.
     *
     *
     * @param deviceData
     * @param deviceDataOffset
     * @param lastNonce
     * @param lastNonceOffset
     * @param unlockToken
     * @param unlockTokenOffset
     * @param unlockTokenLength
     * @return 0x0 on verified and an error code if not.
     */
    private short verifyUnlock(byte[] deviceData, short deviceDataOffset,
                               byte[] lastNonce, short lastNonceOffset,
                               byte[] unlockToken, short unlockTokenOffset,
                               short unlockTokenLength) {
        if (unlockTokenLength < (short)(VERSION_SIZE + NONCE_SIZE + PK_MOD.length)) {
            return 0x0002;
        }
        // Only supported version is the uint64le_t 1
        if (unlockToken[unlockTokenOffset] != VERSION) {
            return 0x0003;
        }

        byte[] message = JCSystem.makeTransientByteArray(
            (short)(NONCE_SIZE + DEVICE_DATA_SIZE), JCSystem.CLEAR_ON_DESELECT);
        // Collect the incoming nonce.
        Util.arrayCopy(unlockToken, (short)(unlockTokenOffset + VERSION_SIZE),
                       message, (short) 0x0, (short) NONCE_SIZE);
        // Append the internallty stored device data.
        Util.arrayCopy(deviceData, deviceDataOffset,
            message, (short)NONCE_SIZE, (short) DEVICE_DATA_SIZE);

        // Verify it against the incoming signature.
        if (verifier.verify(
                message, (short) 0, (short) message.length, unlockToken,
                (short)(unlockTokenOffset + VERSION_SIZE + NONCE_SIZE),
                (short)(unlockTokenLength - (VERSION_SIZE + NONCE_SIZE))) ==
                false) {
            return 0x0004;
        }
        if (littleEndianUnsignedGreaterThan(NONCE_SIZE,
                unlockToken, (short)(unlockTokenOffset + VERSION_SIZE),
                lastNonce, lastNonceOffset) == true) {
            return 0;
        }
        return 0x0005;
    }

    /**
     * Compares two little endian byte streams and returns
     * true if lhs is greater than rhs.
     *
     * @param len number of bytes to compare
     * @param lhs left hand size buffer
     * @param lhsBase starting offset
     * @param rhs right hand size buffer
     * @param rhsBase starting offset
     */
    private boolean littleEndianUnsignedGreaterThan(
            byte len,
            byte[] lhs, short lhsBase,
            byte[] rhs, short rhsBase) {
        // Start with the most significant byte.
        short i = len;
        do {
            i -= 1;
            if (lhs[(short)(lhsBase +i)] > rhs[(short)(rhsBase + i)]) {
                return true;
            }
            if (lhs[(short)(lhsBase +i)] < rhs[(short)(rhsBase + i)]) {
                return false;
            }
            // Only proceed if the current bytes are equal.
        } while (i > 0);
        return false;
    }



    /**
     * {@inheritDoc}
     * Returns true if the lock is changed with associated metadata.
     *
     * If |lockValue| is non-zero, then |lockMeta| should contain a series of
     * ASCII (or hex) values separated by short lengths.  These will be SHA256
     * hashed together to create a "device data hash". Its use is covered next.
     *
     * If |lockValue| is zero, then |lockMeta| should contain the following
     * (all little endian): 8-bit version tag (0x01) || byte[8] "64-bit nonce"
     * || byte[] Signature(nonce||deviceDataHash) The signature is using the
     * embedded key {@link #pkModulus} and the format is ALG_RSA_SHA_256_PKCS1.
     * If the signature verifies using the internally stored deviceDataHash and
     * the provided nonce, then one last check is applied.  If the nonce,
     * treated as a little endian uint64_t, is greater than the stored nonce then
     * it will be rejected.  Note, once unlocked, the device data hash is deleted
     * and the CarrierLock cannot be reapplied unless the device is taken out
     * of production mode (bootloader RMA path).
     *
     * If {@link #globalState} indicates that the device is not yet in production
     * mode, then the lock values can be toggled arbitrarily.
     * The lock values may also be changed in the bootloader or in the HLOS as
     * the transitions are either one-way (lock) or authenticated.  It is required
     * that the lock state is assigned prior to transitioning to production as that
     * ensures that an unlocked device cannot be re-locked maliciously from the HLOS.
     */
    @Override
    public short setWithMetadata(byte lockValue, byte[] lockMeta,
                                 short lockMetaOffset, short lockMetaLength) {
        if (storage == null) {
            // TODO: move to constants.
            return 0x0001;
        }
        // Ensure we don't update the nonce if we didn't go through verify.
        short resp = (short) 0xffff;
        if (lockValue == LOCK_UNLOCKED) {  // SHUT IT DOWN.
            // If we're already unlocked, allow another call to make sure all the
            // data is cleared.
            if (storage[storageOffset] != LOCK_UNLOCKED &&
                globalState.production() == true) {
                // RSA PKCS#1 signature should be the same length as the modulus but we'll allow it to
                // be larger because ???. XXX TODO
                resp = verifyUnlock(storage, (short)(storageOffset + 1 + NONCE_SIZE),
                                    storage, (short)(storageOffset + 1),
                                    lockMeta, lockMetaOffset, lockMetaLength);
                if (resp != (short) 0) {
                  return resp;
                }
            }
            JCSystem.beginTransaction();
            storage[storageOffset] = lockValue;
            // Update the monotonically increasing "nonce" value.
            // Note that the nonce is only ever updated if a signed value
            // was seen or if we're not production() to assure it doesn't get
            // rolled forward.
            if (resp == 0) {
                Util.arrayCopy(lockMeta, (short)(VERSION_SIZE + lockMetaOffset),
                               storage, (short)(1 + storageOffset),
                               (short)NONCE_SIZE);
            }
            // Delete the device-unique data.
            Util.arrayFillNonAtomic(storage, (short)(NONCE_SIZE + 1 + storageOffset),
                (short)DEVICE_DATA_SIZE, (byte)0x00);
            JCSystem.commitTransaction();
        } else {  // Locking. Expect a lockMeta of the device data.
            if (globalState.production() == true) {
                // Locking can only be done prior to production.
                return 0x0006;
            }
            md_sha256.reset();
            JCSystem.beginTransaction();
            // Hash all the input data and store the result as the device data
            // digest.
            md_sha256.doFinal(lockMeta, lockMetaOffset, lockMetaLength,
                              storage, metadataOffset());
            // Note that we never clear or overwrite the nonce.
            storage[storageOffset] = lockValue;
            JCSystem.commitTransaction();
        }
        return 0x0000;
    }

    /**
     * Given all the data, tests if the key actually works.
     *
     * buffer should contain:
     *   fakeLastNonce | fakeDeviceData | version (8) | testNonce | signature
     *
     * @param buffer Array with the test data.
     * @param offset offset into the buffer.
     * @param length total length from offset.
     * @return 0x0 on verify and an error code otherwise.
     */
    public short testVector(byte[] buffer, short offset, short length) {
        return verifyUnlock(buffer, (short)(offset + NONCE_SIZE), // device data
                            buffer, offset,  // fake last nonce.
                            // unlock data
                            buffer, (short)(offset + NONCE_SIZE + DEVICE_DATA_SIZE),
                            (short)(length - (NONCE_SIZE + DEVICE_DATA_SIZE)));
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public short backupSize() {
        return getStorageNeeded();
    }


    /**
     * {@inheritDoc}
     */
    @Override
    public short backup(byte[] outBytes, short outBytesOffset) {
        Util.arrayCopy(storage, storageOffset,
                       outBytes, outBytesOffset,
                       backupSize());
        return backupSize();
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public boolean restore(byte[] inBytes, short inBytesOffset,
                           short inBytesLength) {
        if (inBytesLength > backupSize() || inBytesLength == (short)0) {
            return false;
        }
        Util.arrayCopy(inBytes, inBytesOffset,
                       storage, storageOffset,
                       inBytesLength);
        return true;
    }


}
