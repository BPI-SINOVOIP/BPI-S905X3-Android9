/*
 * Copyright (c) 2017 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */

#ifndef BITS_MAP_H
#define BITS_MAP_H

class BitsMap {
public:
    BitsMap(int bits = 4096);
    ~BitsMap();

    int getZeroBit();
    int setBit(int idx);
    int clearBit(int idx);

protected:
    char *mBits;
    int mLen;
};


#endif/*BITS_MAP_H*/
