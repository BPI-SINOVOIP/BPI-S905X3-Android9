/*
 * Copyright (c) 2017 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */
#ifndef ICOMPOSITION_STRATEGY_H
#define ICOMPOSITION_STRATEGY_H

#include <stdlib.h>

#include <BasicTypes.h>
#include <DrmFramebuffer.h>
#include <IComposer.h>
#include <HwDisplayPlane.h>
#include <HwDisplayCrtc.h>


/*compositon request for special cases:
    * FORCE_CLIENT: don't use device compostion, for debug.
    * SECURE_FB: hide secure fb for special usecases.
    * HDR_VIDEO: information for vpu compostion, may be need conside it.
    */
typedef enum {
    COMPOSE_FORCE_CLIENT = 1 << 0,
    COMPOSE_HIDE_SECURE_FB = 1 << 1,
    COMPOSE_WITH_HDR_VIDEO = 1 << 2,
} COMPOSITION_REQUEST;

/*Macros for composition info dump.*/
#define DUMP_ADD_LINE_DIVIDE() \
    mDumpStr.append("+------+-------------+----------+--------+--------------+\n");

#define DUMP_APPEND_FB_INFO(fbZ, compType) \
    mDumpStr.appendFormat("|%6d|%13s|", fbZ, compositionTypeToString(compType));

#define DUMP_APPEND_EMPTY_FB_INFO() \
        mDumpStr.appendFormat("|%6s|%13s|", " ", " ");

#define DUMP_APPEND_PLANE_INFO(planeName, planeZ, blankType) \
    mDumpStr.appendFormat("%10s|%8d|%14s|\n", \
        planeName, planeZ, \
        drmPlaneBlankToString((drm_plane_blank_t)blankType));

#define DUMP_APPEND_EMPTY_PLANE_INFO() \
    mDumpStr.appendFormat("%10s|%8s|%14s|\n", " ", " ", " ");

#define HWC_PLANE_FAKE_ZORDER (1)

/*Base compostion strategy class.*/
class ICompositionStrategy {
public:
    ICompositionStrategy() { }
    virtual ~ICompositionStrategy() { }

    virtual const char* getName() = 0;

    /*before call decideComb, setup will clean last datas.*/
    virtual void setup(std::vector<std::shared_ptr<DrmFramebuffer>> & layers,
        std::vector<std::shared_ptr<IComposer>> & composers,
        std::vector<std::shared_ptr<HwDisplayPlane>> & planes,
        std::shared_ptr<HwDisplayCrtc> & crtc,
        uint32_t reqFlag) {
        UNUSED(layers);
        UNUSED(composers);
        UNUSED(planes);
        UNUSED(crtc);
        mCompositionFlag = reqFlag;
    }

    /*if have no valid combs, result will < 0.*/
    virtual int decideComposition() = 0;

    /*start composition, should set release fence to each Framebuffer.*/
    virtual int commit() = 0;

    virtual void dump(String8 & dumpstr) {
        dumpstr.appendFormat("Composition (%s):(0x%x)\n", getName(), mCompositionFlag);
        dumpstr.append("---------------------------------------------------------\n");
        dumpstr.append("| FbZ  |  Comp Type  |  Plane   | PlaneZ |  BlankStat   |\n");
        dumpstr.append(mDumpStr);
        dumpstr.append("---------------------------------------------------------\n");
    }

protected:
    /*common functions for dump.*/
    inline void dumpComposedFb(
        std::shared_ptr<DrmFramebuffer> & fb) {
        DUMP_APPEND_FB_INFO(fb->mZorder, fb->mCompositionType);
        DUMP_APPEND_EMPTY_PLANE_INFO();
    }
    inline void dumpFbAndPlane(
        std::shared_ptr<DrmFramebuffer> & fb,
        std::shared_ptr<HwDisplayPlane> & plane,
        int planeZ,
        int planeBlank) {
        DUMP_ADD_LINE_DIVIDE();
        DUMP_APPEND_FB_INFO(fb->mZorder, fb->mCompositionType);
        DUMP_APPEND_PLANE_INFO(plane->getName(), planeZ, planeBlank);
    }
    inline void dumpUnusedPlane(
        std::shared_ptr<HwDisplayPlane> & plane,
        int planeBlank) {
        DUMP_ADD_LINE_DIVIDE();
        DUMP_APPEND_EMPTY_FB_INFO();
        DUMP_APPEND_PLANE_INFO(plane->getName(), -1, planeBlank);
    }

protected:
    String8 mDumpStr;
    uint32_t mCompositionFlag;
};

#endif/*ICOMPOSITION_STRATEGY_H*/
