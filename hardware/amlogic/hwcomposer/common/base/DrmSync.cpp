/*
 * Copyright (c) 2017 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */

#include <DrmSync.h>
#include <unistd.h>
#include <sync/sync.h>
#include <sys/types.h>

#include <MesonLog.h>


const std::shared_ptr<DrmFence> DrmFence::NO_FENCE =
    std::make_shared<DrmFence>(-1);

DrmFence::DrmFence(int32_t fd) {
    mFenceFd = fd;
}

DrmFence::~DrmFence() {
    if (mFenceFd >= 0) {
        close(mFenceFd);
        mFenceFd = -1;
    }
}

int32_t DrmFence::wait(int timeout) {
    if (mFenceFd == -1) {
        return 0;
    }
    int err = sync_wait(mFenceFd, timeout);
    return err < 0 ? -errno : 0;
}

int32_t DrmFence::waitForever(const char* logname) {
    if (mFenceFd == -1) {
        return 0;
    }
    int warningTimeout = 3000;
    int err = sync_wait(mFenceFd, warningTimeout);
    if (err < 0 && errno == ETIME) {
        MESON_LOGE("%s: fence %d didn't signal in %u ms", logname, mFenceFd,
                warningTimeout);
        err = sync_wait(mFenceFd, -1);
    }
    return err < 0 ? -errno : 0;
}


std::shared_ptr<DrmFence> DrmFence::merge(const char * name,
    const std::shared_ptr<DrmFence> &f1,
    const std::shared_ptr<DrmFence> &f2) {

    int result;
    // Merge the two fences.  In the case where one of the fences is not a
    // valid fence (e.g. NO_FENCE) we merge the one valid fence with itself so
    // that a new fence with the given name is created.
    if (f1->isValid() && f2->isValid()) {
        result = sync_merge(name, f1->mFenceFd, f2->mFenceFd);
    } else if (f1->isValid()) {
        result = sync_merge(name, f1->mFenceFd, f1->mFenceFd);
    } else if (f2->isValid()) {
        result = sync_merge(name, f2->mFenceFd, f2->mFenceFd);
    } else {
        return NO_FENCE;
    }
    if (result == -1) {
        int err = -errno;
        MESON_LOGE("merge: sync_merge(\"%s\", %d, %d) returned an error: %s (%d)",
                name, f1->mFenceFd, f2->mFenceFd,
                strerror(-err), err);
        return NO_FENCE;
    }
    return std::shared_ptr<DrmFence>(new DrmFence(result));
}

int32_t DrmFence::dup() const{
    if (-1 == mFenceFd) {
        return -1;
    }

    int32_t dupFence = ::dup(mFenceFd);
    if (dupFence < 0) {
        MESON_LOGE("fence dup failed! please check it immeditely!");
    }

    return dupFence;
}


