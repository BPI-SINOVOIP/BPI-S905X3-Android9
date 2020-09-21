/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: header file
 */

#ifndef TV_CALLBACK
#define TV_CALLBACK
#include "tvapi/android/tv/CTv.h"
class TvCallback : public CTv::TvIObserver {
public:
    TvCallback(void *data)
    {
        mPri = data;
    }
    ~TvCallback()
    {
    }
    void onTvEvent (int32_t msgType, const Parcel &p);
private:
    void *mPri;
};
#endif
