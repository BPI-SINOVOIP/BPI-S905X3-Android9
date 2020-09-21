/*
 * Copyright (c) 2017 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */

#ifndef DUMMY_COMPOSER_H
#define DUMMY_COMPOSER_H

#include <IComposer.h>

#define DUMMY_COMPOSER_NAME "Dummy"

class DummyComposer : public IComposer {
public:
    DummyComposer();
    ~DummyComposer();

    const char* getName() { return DUMMY_COMPOSER_NAME; }
    meson_compositon_t getType() { return MESON_COMPOSITION_DUMMY; }

    bool isFbsSupport(
        std::vector<std::shared_ptr<DrmFramebuffer>> & fbs,
        std::vector<std::shared_ptr<DrmFramebuffer>> & overlayfbs);

    int32_t prepare();

    int32_t addInput(std::shared_ptr<DrmFramebuffer> & fb, bool bOverlay = false);

    int32_t addInputs(
        std::vector<std::shared_ptr<DrmFramebuffer>> & fbs,
        std::vector<std::shared_ptr<DrmFramebuffer>> & overlayfbs);

    int32_t getOverlyFbs(std::vector<std::shared_ptr<DrmFramebuffer>> & overlays);

    int32_t setOutput(std::shared_ptr<DrmFramebuffer> & fb,
        hwc_region_t damage);

    int32_t start();

    std::shared_ptr<DrmFramebuffer> getOutput();

};

#endif/*DUMMY_COMPOSER_H*/

