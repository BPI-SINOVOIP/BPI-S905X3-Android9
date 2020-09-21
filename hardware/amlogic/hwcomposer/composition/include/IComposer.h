/*
 * Copyright (c) 2017 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */
#ifndef ICOMPOSER_H
#define ICOMPOSER_H

#include <stdlib.h>

#include <BasicTypes.h>
#include <Composition.h>
#include <DrmFramebuffer.h>

#include <hardware/hwcomposer_defs.h>

class IComposer {
public:
    IComposer() { }
    virtual ~IComposer() { }

    virtual const char* getName() = 0;
    virtual meson_compositon_t getType() = 0;

    /*check if input framebuffer can be consumed */
    virtual bool isFbsSupport(
        std::vector<std::shared_ptr<DrmFramebuffer>> & fbs,
        std::vector<std::shared_ptr<DrmFramebuffer>> & overlayfbs) = 0;

    /* preapre for new composition pass.*/
    virtual int32_t prepare() = 0;

    /* add input framebuffers to this composer.*/
    virtual int32_t addInputs(
        std::vector<std::shared_ptr<DrmFramebuffer>> & fbs,
        std::vector<std::shared_ptr<DrmFramebuffer>> & overlayfbs) = 0;

    virtual int32_t setOutput(std::shared_ptr<DrmFramebuffer> & fb,
        hwc_region_t damage) = 0;

    /* Start composition. When this function exit, input
   * should be able to get its relese fence.*/
    virtual int32_t start() = 0;

    /* return overlay fbs*/
    virtual int32_t getOverlyFbs(std::vector<std::shared_ptr<DrmFramebuffer>> & overlays) = 0;

    virtual std::shared_ptr<DrmFramebuffer> getOutput();
};

#endif/*ICOMPOSER_H*/
