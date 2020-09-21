/*
 * Copyright (C) 2015 The Android Open Source Project
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

package com.android.bluetooth.gatt;

import android.annotation.Nullable;

/** @hide */
public class AdvtFilterOnFoundOnLostInfo {
    private int mClientIf;

    private int mAdvPktLen;
    @Nullable private byte[] mAdvPkt;

    private int mScanRspLen;

    @Nullable private byte[] mScanRsp;

    private int mFiltIndex;
    private int mAdvState;
    private int mAdvInfoPresent;
    private String mAddress;

    private int mAddrType;
    private int mTxPower;
    private int mRssiValue;
    private int mTimeStamp;

    public AdvtFilterOnFoundOnLostInfo(int clientIf, int advPktLen, byte[] advPkt, int scanRspLen,
            byte[] scanRsp, int filtIndex, int advState, int advInfoPresent, String address,
            int addrType, int txPower, int rssiValue, int timeStamp) {

        mClientIf = clientIf;
        mAdvPktLen = advPktLen;
        mAdvPkt = advPkt;
        mScanRspLen = scanRspLen;
        mScanRsp = scanRsp;
        mFiltIndex = filtIndex;
        mAdvState = advState;
        mAdvInfoPresent = advInfoPresent;
        mAddress = address;
        mAddrType = addrType;
        mTxPower = txPower;
        mRssiValue = rssiValue;
        mTimeStamp = timeStamp;
    }

    public int getClientIf() {
        return mClientIf;
    }

    public int getFiltIndex() {
        return mFiltIndex;
    }

    public int getAdvState() {
        return mAdvState;
    }

    public int getTxPower() {
        return mTxPower;
    }

    public int getTimeStamp() {
        return mTimeStamp;
    }

    public int getRSSIValue() {
        return mRssiValue;
    }

    public int getAdvInfoPresent() {
        return mAdvInfoPresent;
    }

    public String getAddress() {
        return mAddress;
    }

    public int getAddressType() {
        return mAddrType;
    }

    public byte[] getAdvPacketData() {
        return mAdvPkt;
    }

    public int getAdvPacketLen() {
        return mAdvPktLen;
    }

    public byte[] getScanRspData() {
        return mScanRsp;
    }

    public int getScanRspLen() {
        return mScanRspLen;
    }

    public byte[] getResult() {
        int resultLength = mAdvPkt.length + ((mScanRsp != null) ? mScanRsp.length : 0);
        byte[] result = new byte[resultLength];
        System.arraycopy(mAdvPkt, 0, result, 0, mAdvPkt.length);
        if (mScanRsp != null) {
            System.arraycopy(mScanRsp, 0, result, mAdvPkt.length, mScanRsp.length);
        }
        return result;
    }

}

