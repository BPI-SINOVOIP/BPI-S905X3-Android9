/*
 * Copyright (c) 2017 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */
#ifndef COMPOSITION_STRATEGY_FACTORY_H
#define COMPOSITION_STRATEGY_FACTORY_H

#include <BasicTypes.h>
#include <ICompositionStrategy.h>

enum {
    SIMPLE_STRATEGY = 0,
} COMPOSITION_TYPE;

enum {
    MUTLI_OSD_PLANES = 1 << 0, //vpu have multi layers
} COMPOSITION_FLAG;

class CompositionStrategyFactory {
public:
    static std::shared_ptr<ICompositionStrategy> create(
        uint32_t type, uint32_t flags);
};

#endif/*COMPOSITION_STRATEGY_FACTORY_H*/
