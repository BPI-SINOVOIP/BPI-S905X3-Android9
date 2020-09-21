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
import javacard.framework.CardRuntimeException;
import javacard.framework.JCSystem;
import javacard.framework.Shareable;
import javacard.framework.Util;

import com.nxp.ls.library.LSBackup;
import com.nxp.ls.library.LSPullModeRestore;
import com.nxp.ls.library.LSPushModeBackup;

import com.android.verifiedboot.storage.DefaultOsBackupImpl;
import com.android.verifiedboot.storage.OsBackupInterface;

public class JcopBackupImpl extends DefaultOsBackupImpl implements LSBackup {
    final public static short MAGIC = (short)0xdeed;

    // From NXP, the AID of the LoaderService.
    private static final byte[] LS_AID = {
        // NXP RID
        (byte)0xA0, (byte)0x00, (byte)0x00, (byte)0x03, (byte)0x96,
        // NXP JCOP Applets
        (byte)0x54, (byte)0x43, (byte)0x00, (byte)0x00, (byte)0x00, (byte)0x01,
        // Family
        (byte)0x00, (byte)0x0B,
        // Version
        (byte)0x00, // LS Application Instance
        // Type
        (byte)0x01, // LS Application
    };


    /**
     * Returns true on a successful reimport of data.
     *
     * @param iarray to read from
     * @param offset to begin copying from.
     */
    @Override
    public boolean restore(byte[] in, short offset) {
        LSPullModeRestore restore = getLSPullModeRestore();
        offset = (short) 0;
        if (restore == null) {
            return false;
        }
        if (restore.start() == (short) 0) {
            return false;
        }
        short length = (short) 5;
        if (restore.pullData(in, offset, length) == false) {
            return false;
        }
        byte tag = in[0];
        if (tag != TAG_MAGIC) {
            return false;
        }
        if (Util.getShort(in, (short)1) != (short)2) {
            return false;
        }
        if (Util.getShort(in, (short)3) != MAGIC) {
            return false;
        }
        // Magic is good. Let's process all the tags. They should be
        // serialized in order and if not, we abort.
        BackupInterface[] objects = tracked();
        short i = (short) 0;
        for ( ; i < objects.length; ++i) {
            // Get tag and length at the same time.
            length = 3;
            if (restore.pullData(in, (short)0, length) == false) {
                return false;
            }
            tag = in[0];
            length = Util.getShort(in, (short)1);
            if (restore.pullData(in, (short)0, length) == false) {
                return false;
            }
            if (tag == i && objects[i] != null) {
                objects[i].restore(in, (short)0, length);
            } // else we skip it.
        }

        if (restore.end() == false) {
            return false;
        }
        return true;
    }

    /**
     * Retrieve the restore interface from the LoaderService.
     *
     * @return LSPullModeRestore interface or null
     */
    private static LSPullModeRestore getLSPullModeRestore() {
        try {
          return (LSPullModeRestore)JCSystem.getAppletShareableInterfaceObject(
            JCSystem.lookupAID(LS_AID, (short)(0), (byte)(LS_AID.length)),
            LSPullModeRestore.LS_PULL_MODE_RESTORE_PARAMETER);
        } catch (CardRuntimeException e) {
            return null;
        }
    }

    /**
     * Called via LSBackup for saving off data prior to an update.
     * Only lockStorage has to be stored, but it means that install() would
     * need to handle any changes in lock sizes.
     *
     * TODO(wad) In each LockInterface tag store with last metadataSize.
     *
     * @param backup interface to feed data to
     * @param buffer working buffer that is shared with LSPushModeBackup
     * @return true on success or false if there is a failure.
     */
    public boolean backup(LSPushModeBackup backup, byte[] buffer) {
        BackupInterface[] objects = tracked();
        short length = (short) 5;  // magic(TAG, SIZE, MAGIC)
        short i;
        for (i = (short)0; i < (short)objects.length; ++i) {
            length += 3;  // tag, length
            if (objects[i] != null) {
                length += objects[i].backupSize();
            }
        }
        // Interface requires mod 16.
        if (length % 16 != 0) {
          length += (16 - (length % 16));
        }
        if (backup.start(length) == false) {
            return false;
        }
        // Set magic
        short offset = (short) 0;
        buffer[offset++] = TAG_MAGIC;
        Util.setShort(buffer, offset, (short)2);
        offset += 2;
        Util.setShort(buffer, offset, MAGIC);
        offset += 2;

        for (i = (short)0; i < (short)objects.length; ++i) {
            buffer[offset++] = (byte) i;  // TAG == index.
            if (objects[i] != null) {
                Util.setShort(buffer, offset, objects[i].backupSize());
            } else {
                Util.setShort(buffer, offset, (short)0);
            }
            offset += 2;
            if (objects[i] != null) {
                offset += objects[i].backup(buffer, offset);
            }
        }
        // TODO(wad) Worth checking if offset != length.
        if (backup.pushData(buffer, (short)0, length) == false) {
            return false;
        }
        return backup.end();
    }

    /**
     * {@inheritDoc}
     *
     * Checks for the calling LoaderService applet first.
     */
    @Override
    public Shareable getShareableInterfaceObject(AID clientAid, byte arg) {
        AID lsAid = JCSystem.lookupAID(LS_AID, (short)(0), (byte)(LS_AID.length));
        if (clientAid.equals(lsAid)) {
            if (arg == LSBackup.LS_BACKUP_PARAMETER) {
                return this;
            }
        }
        return null;
    }
}
