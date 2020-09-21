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

class BasicLock implements LockInterface {
    // Layout: LockValue (byte)
    private byte[] storage;
    private short storageOffset;
    private OwnerInterface globalState;
    private boolean onlyInBootloader;
    private boolean onlyInHLOS;
    private boolean needMetadata;
    private short metadataSize;
    private LockInterface[] requiredLocks;

    /**
     * Initializes the instance.
     *
     * @param maxMetadataSize length of the metadata
     * @param requiredLocks specify the number of locks that unlocking
     *                      will depend on.
     *
     */
    public BasicLock(short maxMetadataSize, short requiredLockNum) {
        onlyInBootloader = false;
        onlyInHLOS = false;
        needMetadata = false;
        metadataSize = maxMetadataSize;
        requiredLocks = new LockInterface[requiredLockNum];
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

    /**
     * Indicates that it is required that the {@link #globalState} is
     * in the bootloader when the lock is changed.
     *
     * @param inBootloader  true if changes can only happen in the bootloader.
     */
    public void requireBootloader(boolean inBootloader) {
        onlyInBootloader = inBootloader;
    }

    /**
     * Indicates that it is required that the {@link #globalState} is
     * NOT in the bootloader when the lock is changed.
     *
     * @param inHLOS  true if changes can only happen in the bootloader.
     */
    public void requireHLOS(boolean inHLOS) {
        onlyInHLOS = inHLOS;
    }


    /**
     * Indicates that metadata must be supplied when locking.
     *
     * @param atLock  true if metadata must be supplied.
     */
    public void requireMetadata(boolean atLock) {
        needMetadata = atLock;
    }

    /**
     * Adds a lock that must be unlocked to enable
     * this lock to toggle.
     *
     * @param lock lock to depend on
     * @return true on success or false on no space.
     */
    public boolean addRequiredLock(LockInterface lock) {
        for (short i = 0; i < (short) requiredLocks.length; ++i) {
            if (requiredLocks[i] == null) {
                requiredLocks[i] = lock;
                return true;
            }
        }
        return false;
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
        if (globalState == null) {
            return 2;
        }
        return 0;
    }
    /**
     * {@inheritDoc}
     */
    @Override
    public short getStorageNeeded() {
        return (short)(1 + metadataLength());
    }

    /**
     * Sets the backing store to use for state.
     *
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
    public short get(byte[] lockOut, short lockOffset) {
        if (storage == null) {
            return 0x0001;
        }
        try {
            Util.arrayCopy(storage, storageOffset,
                           lockOut, lockOffset, (short) 1);
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
     */
    @Override
    public short metadataOffset() {
        if (storage == null) {
            return (short) 0xffff;
        }
        return (short)(lockOffset() + 1);
    }

    /**
     * {@inheritDoc}
     *
     * @return length of metadata.
     */
    public short metadataLength() {
        return metadataSize;
    }

    /**
     * Ensures any requiredLocks are unlocked.
     * @return true if allowed or false if not.
     */
    public boolean prerequisitesMet() {
        if (requiredLocks.length != 0) {
            byte[] temp = new byte[1];
            short resp = 0;
            for (short l = 0; l < requiredLocks.length; ++l) {
                resp = requiredLocks[l].get(temp, (short) 0);
                // On error or not cleared, fail.
                if (resp != 0 || temp[0] != (byte) 0x0) {
                    return false;
                }
            }
        }
        return true;
    }

    /**
     * {@inheritDoc}
     *
     * Returns 0x0 on success.
     */
    @Override
    public short set(byte val) {
        if (storage == null) {
            return 0x0001;
        }
        // Do not require meta on unlock.
        if (val != 0) {
            // While an invalid combo, we can just make the require flag
            // pointless if metadataLength == 0.
            if (needMetadata == true && metadataLength() > 0) {
                return 0x0002;
            }
        }
        // To relock, the lock must be unlocked, then relocked.
        if (val != (byte)0 && storage[lockOffset()] != (byte)0) {
            return 0x0005;
        }
        if (globalState.production() == true) {
             // Enforce only when in production.
            if (onlyInBootloader == true) {
                // If onlyInBootloader is false, we allow toggling regardless.
                if (globalState.inBootloader() == false) {
                    return 0x0003;
                }
            }
            if (onlyInHLOS == true) {
                 // If onlyInHLOS is false, we allow toggling regardless.
                if (globalState.inBootloader() == true) {
                    return 0x0003;
                }
            }
        }
        if (prerequisitesMet() == false) {
          return 0x0a00;
        }
        try {
            storage[storageOffset] = val;
        } catch (CardRuntimeException e) {
            return 0x0004;
        }
        return 0;
    }

   /**
     * {@inheritDoc}
     *
     * If configured with {@link #requiredMetadata}, will populate the
     * metadata. Otherwise, it will just call {@link #set}.
     *
     */
    @Override
    public short setWithMetadata(byte lockValue, byte[] lockMeta,
                                 short lockMetaOffset, short lockMetaLength) {
        if (storage == null) {
            return 0x0001;
        }
        // No overruns, please.
        if (lockMetaLength > metadataLength()) {
            return 0x0002;
        }
        // To relock, the lock must be unlocked, then relocked.
        // This ensures that a lock like LOCK_OWNER cannot have its key value
        // changed without first having the permission to unlock and lock again.
        if (lockValue != (byte)0 && storage[lockOffset()] != (byte)0) {
            return 0x0005;
        }
        if (metadataLength() == 0) {
            return set(lockValue);
        }
        // Before copying, ensure changing the lock state is currently permitted.
        if (prerequisitesMet() == false) {
          return 0x0a00;
        }
        try {
            // When unlocking, do so before clearing the metadata.
            if (lockValue == (byte) 0) {
                JCSystem.beginTransaction();
                storage[lockOffset()] = lockValue;
                JCSystem.commitTransaction();
            }
            if (lockMetaLength == 0) {
                // An empty lockMeta will clear the value.
                Util.arrayFillNonAtomic(storage, metadataOffset(),
                                        metadataLength(), (byte) 0x00);
            } else {
                Util.arrayCopyNonAtomic(lockMeta, lockMetaOffset,
                                        storage, metadataOffset(),
                                        lockMetaLength);
            }
            // When locking, do so after the copy as interrupting it will
            // not impact its use in a locked state.
            if (lockValue != (byte) 0) {
                JCSystem.beginTransaction();
                storage[lockOffset()] = lockValue;
                JCSystem.commitTransaction();
            }
        } catch (CardRuntimeException e) {
            return 0x0004;
        }
        return 0;
    }
}
