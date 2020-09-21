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

public interface BackupInterface {
    /**
     * Returns true on a successful reimport of data.
     *
     * @param inBytes array to read from
     * @param inBytesOffset offset to begin copying from.
     * @param inBytesLength length to copy from |inBytes|.
     */
    boolean restore(byte[] inBytes, short inBytesOffset, short inBytesLength);

    /**
     * Copies all internal state to the given array and returns the number of
     * bytes written.  The maximum return size is 0xfffe.
     *
     * @param outBytes array to copy internal state to starting at |outBytesOffset|.
     * @param outBytesOffset
     * @return length written
     */
    short backup(byte[] outBytes, short outBytesOffset);

   /**
    * Returns the size needed for backup().
    *
    * @return size of a backup as a short.
    */
   short backupSize();
}
