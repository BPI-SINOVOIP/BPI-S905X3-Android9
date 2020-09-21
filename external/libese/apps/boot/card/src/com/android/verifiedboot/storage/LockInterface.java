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

import com.android.verifiedboot.globalstate.owner.OwnerInterface;
import com.android.verifiedboot.storage.BackupInterface;

// The base interface for all locks.  As the іnterface clobbers a number
// of common methods it might make more sense as a base class, but this
// seems fine enough for now.
public interface LockInterface extends BackupInterface {
    // 0 means unlocked and non-zero means locks.
    public final static byte LOCK_UNLOCKED = (byte) 0;

    /**
     * @return 0, as short, if initialized, or a non-zero error
     *         based on the error state.
     */
    short initialized();

    /**
     * Return the bytes needed by this lock.
     *
     * Must be callable prior to initialize.
     *
     * @return size, as short, of storage required for {@link #setStorage}.
     */
    short getStorageNeeded();

    /**
     * Sets the backing store to use for state and global state dependency.
     *
     * @param globalStateOwner OwnerInterface implementation for policy checking.
     * @param extStorage  external array to use for storage
     * @param extStorageOffset where to begin storing data
     *
     * This should be called before use.
     */
    void initialize(OwnerInterface globalStateOwner, byte[] extStorage, short extStorageOffset);

    /**
     * Emits the lock byte into the array.
     *
     * @param lockOut lock value as a byte. 0x0 is unlocked, !0x0 is locked.
     * @param lockOffset offset to write the lock byte.
     * @return 0x0 on success or an error code otherwise.
     */
    short get(byte[] lockOut, short lockOffset);

    /**
     * Returns the offset into the external storage for the lock byte.
     *
     * @return The offset into the external storage for metadata or 0xffff
     *         on error.
     *
     */
    short lockOffset();

    /**
     * Returns the offset into the external storage for the metadata.
     *
     * @return The offset into the external storage for metadata or 0xffff
     *         on error.
     *
     */
    short metadataOffset();

    /**
     * Returns length of metadata.
     *
     * @return length of metadata or 0xffff on error.
     */
    short metadataLength();


    /**
     * Returns true if the lock state can be set.
     *
     * @param val New lock byte. Non-zero values are considered "locked".
     * @return 0x0 if the lock state was set to |val| and an error code otherwise.
     */
    short set(byte val);

    /**
     * Returns true if the lock is changed with associated metadata.
     *
     * @param lockValue New lock byte value
     * @param lockMeta array to copy metadata from
     * @param lockMetaOffset offset to start copying from
     * @param lockMetaLength bytes to copy
     * @return 0x0 is successful and an error code if not.
     */
    short setWithMetadata(byte lockValue, byte[] lockMeta, short lockMetaOffset, short lockMetaLength);
}
