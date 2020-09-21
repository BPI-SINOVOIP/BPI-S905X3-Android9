/*
 * Copyright (c) 2017 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */
#include <ComposerFactory.h>
#include <MesonLog.h>

#include "ClientComposer.h"
#include "DummyComposer.h"
#include "Ge2dComposer.h"

int32_t ComposerFactory::create(meson_composer_t type,
    std::shared_ptr<IComposer> & composer) {
    int32_t ret = 0;

    switch (type) {
        case MESON_CLIENT_COMPOSER:
            composer = std::make_shared<ClientComposer>();
            break;
        case MESON_DUMMY_COMPOSER:
            composer = std::make_shared<DummyComposer>();
            break;
#ifdef HWC_ENABLE_GE2D_COMPOSITION
        case MESON_GE2D_COMPOSER:
            composer = std::make_shared<Ge2dComposer>();
            break;
#endif
        default:
            MESON_LOGE("Can't create Uunkown composer (%d)\n", type);
            composer = NULL;
            ret = -ENODEV;
            break;
    }

    return ret;
}


