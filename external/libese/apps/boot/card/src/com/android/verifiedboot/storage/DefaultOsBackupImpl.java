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
import javacard.framework.Shareable;
import javacard.framework.Util;

import com.android.verifiedboot.storage.BackupInterface;

public class DefaultOsBackupImpl implements OsBackupInterface {
    private BackupInterface[] objects;

    public DefaultOsBackupImpl() {
        objects = new BackupInterface[OsBackupInterface.TAG_MAX + 1];
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void track(byte tag, Object obj) {
        objects[tag] = (BackupInterface)obj;
    }

    /**
     * Return the tracked objects to subclasses.
     */
    protected BackupInterface[] tracked() {
        return objects;
    }

    /**
     * {@inheritDoc}
     *
     * Not implemented.
     */
    @Override
    public boolean restore(byte[] inBytes, short inBytesOffset) {
        return false;
    }

    /**
      * Returns |this|.
      *
      * @param AID Unused.
      * @param arg Unused.
      * @return this interface.
      */
    public Shareable getShareableInterfaceObject(AID clientAid, byte arg) {
        return this;
    }
}
