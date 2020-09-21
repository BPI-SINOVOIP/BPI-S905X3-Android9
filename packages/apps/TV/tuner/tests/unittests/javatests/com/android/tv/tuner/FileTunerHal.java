/*
 * Copyright (C) 2016 The Android Open Source Project
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

package com.android.tv.tuner;

import android.content.Context;
import android.os.SystemClock;
import android.util.Log;
import android.util.SparseBooleanArray;
import com.android.tv.testing.utils.Utils;
import java.io.File;
import java.io.IOException;
import java.io.RandomAccessFile;
import java.util.Random;

/** This class simulate the actions happened in TunerHal. */
public class FileTunerHal extends TunerHal {

    private static final String TAG = "FileTunerHal";
    private static final int DEVICE_ID = 0;
    private static final int TS_PACKET_SIZE = 188;
    // To keep consistent with tunertvinput_jni, which fit Ethernet MTU (1500)
    private static final int TS_PACKET_COUNT_PER_PAYLOAD = 7;
    private static final int TS_PAYLOAD_SIZE = TS_PACKET_SIZE * TS_PACKET_COUNT_PER_PAYLOAD;
    private static final int TS_SYNC_BYTE = 0x47;
    // Make the read size from file and size in nativeWriteInBuffer same to make read logic simpler.
    private static final int MIN_READ_UNIT = TS_PACKET_SIZE * TS_PACKET_COUNT_PER_PAYLOAD;
    private static final long DELAY_IOCTL_SET_FRONTEND = 100;
    private static final long DELAY_IOCTL_WAIT_FRONTEND_LOCKED = 500;

    // The terrestrial broadcast mode (known as 8-VSB) delivers an MPEG-2 TS at rate
    // approximately 19.39 Mbps in a 6 MHz channel. (See section #5 of ATSC A/53 Part 2:2011)
    private static final double TS_READ_BYTES_PER_MS = 19.39 * 1024 * 1024 / (8 * 1000.0);
    private static final long INITIAL_BUFFERED_TS_BYTES = (long) (TS_READ_BYTES_PER_MS * 150);

    private static boolean sIsDeviceOpen;

    private final SparseBooleanArray mPids = new SparseBooleanArray();
    private final byte[] mBuffer = new byte[MIN_READ_UNIT];
    private final File mTestFile;
    private final Random mGenerator;
    private RandomAccessFile mAccessFile;
    private boolean mHasPendingTune;
    private long mReadStartMs;
    private long mTotalReadBytes;
    private long mInitialSkipMs;
    private boolean mPacketMissing;
    private boolean mEnableArtificialDelay;

    FileTunerHal(Context context, File testFile) {
        super(context);
        mTestFile = testFile;
        mGenerator = Utils.createTestRandom();
    }

    /**
     * Skip Initial parts of the TS file in order to start from specified time position.
     *
     * @param initialSkipMs initial position from where TS stream should be provided
     */
    void setInitialSkipMs(long initialSkipMs) {
        mInitialSkipMs = initialSkipMs;
    }

    @Override
    protected boolean openFirstAvailable() {
        sIsDeviceOpen = true;
        getDeliverySystemTypeFromDevice();
        return true;
    }

    @Override
    public void close() {}

    @Override
    protected boolean isDeviceOpen() {
        return sIsDeviceOpen;
    }

    @Override
    protected long getDeviceId() {
        return DEVICE_ID;
    }

    @Override
    protected void nativeFinalize(long deviceId) {
        if (deviceId != DEVICE_ID) {
            return;
        }
        mPids.clear();
    }

    @Override
    protected boolean nativeTune(
            long deviceId, int frequency, @ModulationType String modulation, int timeoutMs) {
        if (deviceId != DEVICE_ID) {
            return false;
        }
        if (mHasPendingTune) {
            return false;
        }
        closeInputStream();
        openInputStream();
        if (mAccessFile == null) {
            return false;
        }

        // Sleeping to simulate calling FE_GET_INFO and FE_SET_FRONTEND.
        if (mEnableArtificialDelay) {
            SystemClock.sleep(DELAY_IOCTL_SET_FRONTEND);
        }
        if (mHasPendingTune) {
            return false;
        }

        // Sleeping to simulate waiting frontend locked.
        if (mEnableArtificialDelay) {
            SystemClock.sleep(DELAY_IOCTL_WAIT_FRONTEND_LOCKED);
        }
        if (mHasPendingTune) {
            return false;
        }
        mTotalReadBytes = 0;
        mReadStartMs = System.currentTimeMillis();
        return true;
    }

    @Override
    protected void nativeAddPidFilter(long deviceId, int pid, @FilterType int filterType) {
        if (deviceId != DEVICE_ID) {
            return;
        }
        mPids.put(pid, true);
    }

    @Override
    protected void nativeCloseAllPidFilters(long deviceId) {
        if (deviceId != DEVICE_ID) {
            return;
        }
        mPids.clear();
    }

    @Override
    protected void nativeStopTune(long deviceId) {
        if (deviceId != DEVICE_ID) {
            return;
        }
        mPids.clear();
    }

    @Override
    protected int nativeWriteInBuffer(long deviceId, byte[] javaBuffer, int javaBufferSize) {
        if (deviceId != DEVICE_ID) {
            return 0;
        }
        if (mEnableArtificialDelay) {
            long estimatedReadBytes =
                    (long) (TS_READ_BYTES_PER_MS * (System.currentTimeMillis() - mReadStartMs))
                            + INITIAL_BUFFERED_TS_BYTES;
            if (estimatedReadBytes < mTotalReadBytes) {
                return 0;
            }
        }
        int readSize = readInternal();
        if (readSize <= 0) {
            closeInputStream();
            openInputStream();
            if (mAccessFile == null) {
                return -1;
            }
            readSize = readInternal();
        } else {
            mTotalReadBytes += readSize;
        }

        if (mBuffer[0] != TS_SYNC_BYTE) {
            return -1;
        }
        int filteredSize = 0;
        javaBufferSize = (javaBufferSize / TS_PACKET_SIZE) * TS_PACKET_SIZE;
        javaBufferSize = (javaBufferSize < TS_PAYLOAD_SIZE) ? javaBufferSize : TS_PAYLOAD_SIZE;
        for (int i = 0, destPos = 0;
                i < readSize && destPos + TS_PACKET_SIZE <= javaBufferSize;
                i += TS_PACKET_SIZE) {
            if (mBuffer[i] == TS_SYNC_BYTE) {
                int pid = ((mBuffer[i + 1] & 0x1f) << 8) + (mBuffer[i + 2] & 0xff);
                if (mPids.get(pid)) {
                    System.arraycopy(mBuffer, i, javaBuffer, destPos, TS_PACKET_SIZE);
                    destPos += TS_PACKET_SIZE;
                    filteredSize += TS_PACKET_SIZE;
                }
            }
        }
        return filteredSize;
    }

    @Override
    protected void nativeSetHasPendingTune(long deviceId, boolean hasPendingTune) {
        if (deviceId != DEVICE_ID) {
            return;
        }
        mHasPendingTune = hasPendingTune;
    }

    @Override
    protected int nativeGetDeliverySystemType(long deviceId) {
        return DELIVERY_SYSTEM_ATSC;
    }

    private int readInternal() {
        int readSize;
        try {
            if (mPacketMissing) {
                mAccessFile.skipBytes(
                        mGenerator.nextInt(TS_PACKET_COUNT_PER_PAYLOAD) * TS_PACKET_SIZE);
            }
            readSize = mAccessFile.read(mBuffer, 0, mBuffer.length);
        } catch (IOException e) {
            return -1;
        }
        return readSize;
    }

    private void closeInputStream() {
        try {
            if (mAccessFile != null) {
                mAccessFile.close();
            }
        } catch (IOException e) {
        }
        mAccessFile = null;
    }

    private void openInputStream() {
        try {
            mAccessFile = new RandomAccessFile(mTestFile, "r");

            // Since sync frames are located once per 2 seconds, test with various
            // starting offsets according to mInitialSkipMs.
            long skipBytes = (long) (mInitialSkipMs * TS_READ_BYTES_PER_MS);
            skipBytes = skipBytes / TS_PACKET_SIZE * TS_PACKET_SIZE;
            mAccessFile.seek(skipBytes);
        } catch (IOException e) {
            Log.i(TAG, "open input stream failed:" + e);
        }
    }

    /** Gets the number of built-in tuner devices. Always 1 in this case. */
    public static int getNumberOfDevices(Context context) {
        return 1;
    }

    public void setEnablePacketMissing(boolean packetMissing) {
        mPacketMissing = packetMissing;
    }

    public void setEnableArtificialDelay(boolean enableArtificialDelay) {
        mEnableArtificialDelay = enableArtificialDelay;
    }
}
