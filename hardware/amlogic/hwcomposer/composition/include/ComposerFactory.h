/*
 * Copyright (c) 2017 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */
#ifndef COMPOSER_FACTORY_H
#define COMPOSER_FACTORY_H

#include <IComposer.h>

typedef enum {
    MESON_CLIENT_COMPOSER = 0,
    MESON_DUMMY_COMPOSER,
    MESON_GE2D_COMPOSER,
    /*MESON_GPU_COMPOSER,*/
} meson_composer_t;

class ComposerFactory {
public:
    /*get valid composers by flags.*/
    static int32_t create(meson_composer_t type, std::shared_ptr<IComposer> & composer);
};

#endif/*COMPOSER_FACTORY_H*/
