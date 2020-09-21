/*
 * Copyright (c) 2017 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */

#include "HwcModeMgr.h"
#include "FixedSizeModeMgr.h"
#include "VariableModeMgr.h"
#include "ActiveModeMgr.h"
#include "RealModeMgr.h"

std::shared_ptr<HwcModeMgr> createModeMgr(
    hwc_modes_policy_t policy) {
    if (policy == FIXED_SIZE_POLICY) {
        HwcModeMgr * modeMgr = (HwcModeMgr *) new FixedSizeModeMgr();
        return std::shared_ptr<HwcModeMgr>(std::move(modeMgr));
    } else if (policy == FULL_ACTIVE_POLICY) {
        HwcModeMgr * modeMgr = (HwcModeMgr *) new VariableModeMgr();
        return std::shared_ptr<HwcModeMgr>(std::move(modeMgr));
    } else if (policy == ACTIVE_MODE_POLICY) {
        HwcModeMgr * modeMgr = (HwcModeMgr *) new ActiveModeMgr();
        return std::shared_ptr<HwcModeMgr>(std::move(modeMgr));
    } else if (policy == REAL_MODE_POLICY){
        HwcModeMgr * modeMgr = (HwcModeMgr *) new RealModeMgr();
        return std::shared_ptr<HwcModeMgr>(std::move(modeMgr));
    } else {
        return NULL;
    }
}

