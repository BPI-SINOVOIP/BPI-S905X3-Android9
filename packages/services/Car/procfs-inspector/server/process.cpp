#include "process.h"

#include <binder/Parcel.h>

status_t procfsinspector::ProcessInfo::writeToParcel(Parcel* parcel) const {
    parcel->writeUint32(mPid);
    parcel->writeUint32(mUid);
    return android::OK;
}

status_t procfsinspector::ProcessInfo::readFromParcel(const Parcel* parcel) {
    mPid = parcel->readUint32();
    mUid = parcel->readUint32();
    return android::OK;
}
