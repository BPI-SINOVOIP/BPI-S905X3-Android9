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

import javacard.framework.AID;
import javacard.framework.Shareable;

import com.android.verifiedboot.storage.BackupInterface;

public interface OsBackupInterface extends Shareable {
    final public static byte TAG_VERSION_STORAGE = 0x0;
    final public static byte TAG_LOCK_CARRIER = 0x1;
    final public static byte TAG_LOCK_DEVICE = 0x2;
    final public static byte TAG_LOCK_BOOT = 0x3;
    final public static byte TAG_LOCK_OWNER = 0x4;
    final public static byte TAG_MAX = TAG_LOCK_OWNER;

    final public static byte TAG_MAGIC = (byte) 0xf0;


    /**
     * Αdds the given BackupInterface object for tracking
     * on backup or restore. Only one object is allowed
     * per tag.
     *
     * @param tag The tag mapping the specific object to the tagḣ
     * @param bObj Object to track.
     */
    void track(byte tag, Object bObj);

    /**
     * Returns true on a successful reimport of data.
     *
     * @param inBytes array to read from
     * @param inBytesOffset offset to begin copying from.
     */
    boolean restore(byte[] inBytes, short inBytesOffset);

    /**
     * Mimic the applet call.
     *
     * @param aid caller's AID
     * @param arg requesting argument
     */
    Shareable getShareableInterfaceObject(AID aid, byte arg);
}
