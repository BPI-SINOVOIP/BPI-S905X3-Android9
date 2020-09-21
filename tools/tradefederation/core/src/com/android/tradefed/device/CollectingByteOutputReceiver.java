/*
 * Copyright (C) 2011 The Android Open Source Project
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
package com.android.tradefed.device;

import com.android.ddmlib.IShellOutputReceiver;
import com.android.tradefed.util.ByteArrayList;

/**
 * A {@link IShellOutputReceiver} which collects the whole shell output into a {@code byte[]}.
 * This is useful for shell commands that will produce a significant amount of output, where the
 * 2x {@link String} memory overhead will be significant.
 */
public class CollectingByteOutputReceiver implements IShellOutputReceiver {
    private ByteArrayList mData = new ByteArrayList();
    private boolean mIsCanceled = false;

    public byte[] getOutput() {
        return mData.getContents();
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public boolean isCancelled() {
        return mIsCanceled;
    }

    /**
     * Cancel the output collection
     */
    public void cancel() {
        mIsCanceled = true;
        clear();
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void addOutput(byte[] data, int offset, int length) {
        if (!isCancelled()) {
            mData.addAll(data, offset, length);
        }
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void flush() {
        // ignore
    }

    /**
     * Try to unref everything that we can
     */
    public void clear() {
        mData.clear();
    }
}
