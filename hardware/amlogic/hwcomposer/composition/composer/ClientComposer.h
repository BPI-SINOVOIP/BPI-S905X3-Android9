/*
 * Copyright (c) 2017 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */

#ifndef CLIENT_COMPOSER_H
#define CLIENT_COMPOSER_H

#include <IComposer.h>

#define CLIENT_COMPOSER_NAME "Client"

/*
 * ClientComposer: ask SurfaceFlinger to do composition.
 */
class ClientComposer : public IComposer {
public:
    ClientComposer();
    ~ClientComposer();

    const char* getName() { return CLIENT_COMPOSER_NAME; }
    meson_compositon_t getType() { return MESON_COMPOSITION_CLIENT; }

    bool isFbsSupport(
        std::vector<std::shared_ptr<DrmFramebuffer>> & fbs,
        std::vector<std::shared_ptr<DrmFramebuffer>> & overlayfbs);

    int32_t prepare();

    int32_t addInput(std::shared_ptr<DrmFramebuffer> & fb, bool bOverlay);

    int32_t addInputs(
        std::vector<std::shared_ptr<DrmFramebuffer>> & fbs,
        std::vector<std::shared_ptr<DrmFramebuffer>> & overlayfbs);

    int32_t getOverlyFbs(std::vector<std::shared_ptr<DrmFramebuffer>> & overlays);

    int32_t setOutput(std::shared_ptr<DrmFramebuffer> & fb,
        hwc_region_t damage);

    int32_t start();

    std::shared_ptr<DrmFramebuffer> getOutput();

protected:
    std::shared_ptr<DrmFramebuffer> mClientTarget;
    std::vector<std::shared_ptr<DrmFramebuffer>> mOverlayFbs;
};

#endif/*CLIENT_COMPOSER_H*/
