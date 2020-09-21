/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: Header file
 */

#ifndef DROIDLOGIC_IHDMICECSERVICE_H_
#define DROIDLOGIC_IHDMICECSERVICE_H_

#include <binder/IInterface.h>
#include <binder/Parcel.h>

#include "HdmiCecBase.h"
#include "IHdmiCecCallback.h"

namespace android {


class IHdmiCecService : public IInterface, public HdmiCecBase {
public:
    DECLARE_META_INTERFACE(HdmiCecService);

    virtual int connect(const sp<IHdmiCecCallback> &client) = 0;
};

class BnHdmiCecService : public BnInterface<IHdmiCecService> {
public:
    virtual status_t onTransact(uint32_t code, const Parcel& data, Parcel* reply, uint32_t flags = 0);
};

};//namespace android

#endif /* DROIDLOGIC_IHDMICECSERVICE_H_ */
