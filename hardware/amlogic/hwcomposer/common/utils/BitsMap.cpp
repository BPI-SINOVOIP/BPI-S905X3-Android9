/*
 * Copyright (c) 2017 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */

#include <BitsMap.h>
#include <string.h>
#include <MesonLog.h>

#define BYTE_IDX(idx) (idx >> 3)
#define BIT_OFFSET(idx) (idx & 0x7)


BitsMap::BitsMap(int bits) {
    mLen = BYTE_IDX(bits) + 1;
    mBits = new char[mLen];
    memset(mBits, 0, mLen);
}

BitsMap::~BitsMap() {
    delete mBits;
}

int BitsMap::getZeroBit() {
    int  i = 0;
    for (; i < mLen; i++) {
        if (mBits[i] == 0xFF) {
            continue;
        }

        char byteval = mBits[i];
        int idx = 0;

        if ((byteval & 0xF) == 0xF) {
            byteval >>= 4;
            idx += 4;
        }
        if ((byteval & 0x3) == 0x3) {
            byteval >>= 2;
            idx += 2;
        }
        if ((byteval & 0x1) == 0x1) {
            byteval >>= 1;
            idx += 1;
        }

#if 0
        int testidx = 0;
        byteval = mBits[i];
        while ((byteval & 0x1) == 0x1) {
            byteval >>= 1;
            testidx ++;
        }

        if (testidx != idx) {
            MESON_LOGE("error idx %d vs %d", testidx, idx);
        }
#endif

        idx += i * 8;
        return idx;
    }

    return -1;
}

int BitsMap::setBit(int idx) {
    mBits[BYTE_IDX(idx)] |= 1 << BIT_OFFSET(idx);
    return 0;
}

int BitsMap::clearBit(int idx) {
    mBits[BYTE_IDX(idx)] &= ~(1 << BIT_OFFSET(idx));
    return 0;
}

