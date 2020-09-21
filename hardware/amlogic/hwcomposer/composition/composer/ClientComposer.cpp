/*
 * Copyright (c) 2017 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */

#include "ClientComposer.h"
#include <DrmTypes.h>
#include <MesonLog.h>

ClientComposer::ClientComposer() {
}

ClientComposer::~ClientComposer() {
}

bool ClientComposer::isFbsSupport(
    std::vector<std::shared_ptr<DrmFramebuffer>> & fbs __unused,
    std::vector<std::shared_ptr<DrmFramebuffer>> & overlayfbs __unused) {
    return true;
}

int32_t ClientComposer::prepare() {
    mOverlayFbs.clear();
    mClientTarget.reset();
    return 0;
}

int32_t ClientComposer::addInput(
    std::shared_ptr<DrmFramebuffer> & fb __unused,
    bool bOverlay __unused) {
    return 0;
}

int32_t ClientComposer::addInputs(
    std::vector<std::shared_ptr<DrmFramebuffer>> & fbs __unused,
    std::vector<std::shared_ptr<DrmFramebuffer>> & overlayfbs) {
    mOverlayFbs = overlayfbs;
    return 0;
}

int32_t ClientComposer::getOverlyFbs(
    std::vector<std::shared_ptr<DrmFramebuffer>> & overlays) {
    overlays = mOverlayFbs;
    return 0;
}

int32_t ClientComposer::setOutput(
    std::shared_ptr<DrmFramebuffer> & fb,
    hwc_region_t damage __unused) {
    mClientTarget = fb;
    return 0;
}

int32_t ClientComposer::start() {
    return 0;
}

std::shared_ptr<DrmFramebuffer> ClientComposer::getOutput() {
    return mClientTarget;
}

