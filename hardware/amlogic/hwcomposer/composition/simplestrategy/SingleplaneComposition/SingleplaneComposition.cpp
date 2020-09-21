/*
 * Copyright (c) 2018 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */

#include "SingleplaneComposition.h"
#include <DrmTypes.h>
#include <MesonLog.h>

/*---------------------  SingleplaneComposition  ---------------------*/
SingleplaneComposition::SingleplaneComposition() {
}

SingleplaneComposition::~SingleplaneComposition() {
}

/*---------------------  Setup composition  ---------------------*/
void SingleplaneComposition::cleanup() {
    mForceClientComposer = mHideSecureLayer = false;

    /*clean plane info*/
    mCursorPlane.reset();
    mOsdPlane.reset();
    mLegacyVideoPlane.reset();
    mUnusedPlanes.clear();

    /*clean composer info*/
    mDummyComposer.reset();
    mClientComposer.reset();
    mOverlayFbs.clear();
    mComposers.clear();

    /*clean framebuffer list.*/
    mFramebuffers.clear();

    /*clean composition members.*/
    mOverlayFbs.clear();
    mComposer.reset();
    mDisplayPairs.clear();

    mDumpStr.clear();
}

void SingleplaneComposition::setup(
    std::vector<std::shared_ptr<DrmFramebuffer>> & layers,
    std::vector<std::shared_ptr<IComposer>> & composers,
    std::vector<std::shared_ptr<HwDisplayPlane>> & planes,
    std::shared_ptr<HwDisplayCrtc> & crtc,
    uint32_t reqFlag) {
    cleanup();

    mCompositionFlag = reqFlag;
    if (mCompositionFlag & COMPOSE_WITH_HDR_VIDEO) {
        MESON_LOGV("For Singleplane, nothing to do with HDR.");
    }
    if (mCompositionFlag & COMPOSE_FORCE_CLIENT) {
        mForceClientComposer = true;
    }
    if (mCompositionFlag & COMPOSE_HIDE_SECURE_FB) {
        mHideSecureLayer = true;
    }

    mCrtc = crtc;

    /*add layers*/
    mFramebuffers = layers;

    /*collect composers*/
    auto composerIt = composers.begin();
    for (; composerIt != composers.end(); ++composerIt) {
        std::shared_ptr<IComposer> composer = *composerIt;
        switch (composer->getType()) {
            case MESON_COMPOSITION_DUMMY:
                if (mDummyComposer == NULL)
                    mDummyComposer = composer;
                break;
            case MESON_COMPOSITION_CLIENT:
                if (mClientComposer == NULL)
                    mClientComposer = composer;
                break;
            default:
                mComposers.push_back(composer);
                break;
        }
    }

    /*collect planes*/
    auto planeIt = planes.begin();
    for (; planeIt != planes.end(); ++planeIt) {
        std::shared_ptr<HwDisplayPlane> plane = *planeIt;
        switch (plane->getPlaneType()) {
            case CURSOR_PLANE:
                mCursorPlane = plane;
                break;
            case OSD_PLANE:
                if (mOsdPlane.get() == NULL) {
                    mOsdPlane = plane;
                } else {
                    MESON_LOGE("More than one osd plane, discard.");
                    mUnusedPlanes.push_back(plane);
                }
                break;
            case LEGACY_VIDEO_PLANE:
                mLegacyVideoPlane = plane;
                break;
            case INVALID_PLANE:
            case HWC_VIDEO_PLANE:
            default:
                mUnusedPlanes.push_back(plane);
                break;
        }
    }
}

/*--------------------- Decide composition ---------------------*/
int SingleplaneComposition::decideComposition() {
    if (mFramebuffers.empty()) {
        MESON_LOGV("No layers to compose, exit.");
        return 0;
    }

    /*handle fbs of video plane, and cursor plane.*/
    processFbsOfExplicitComposition();

    /*hide secure layer, and force client.*/
    applyCompositionFlags();

    buildOsdComposition();

    /* record overlayFbs and start to compose */
    if (mComposer.get()) {
        mComposer->prepare();
        mComposer->addInputs(mFramebuffers, mOverlayFbs);
    }

    return 0;
}

int SingleplaneComposition::processFbsOfExplicitComposition() {
    std::shared_ptr<DrmFramebuffer> fb;
    meson_compositon_t destComp;

    static struct planeComp {
        drm_fb_type_t srcFb;
        drm_plane_type_t destPlane;
        meson_compositon_t destComp;
    } planeCompPairs [] = {
        {DRM_FB_VIDEO_OVERLAY, LEGACY_VIDEO_PLANE,
            MESON_COMPOSITION_PLANE_AMVIDEO},
        {DRM_FB_VIDEO_OMX_PTS, LEGACY_VIDEO_PLANE,
            MESON_COMPOSITION_PLANE_AMVIDEO},
        {DRM_FB_VIDEO_SIDEBAND, LEGACY_VIDEO_PLANE,
            MESON_COMPOSITION_PLANE_AMVIDEO_SIDEBAND},
        {DRM_FB_CURSOR, CURSOR_PLANE,
            MESON_COMPOSITION_PLANE_CURSOR},
    };
    static int pairSize = sizeof(planeCompPairs) / sizeof(struct planeComp);

    auto fbIt = mFramebuffers.begin();
    for (; fbIt != mFramebuffers.end(); ++fbIt) {
        fb = *fbIt;
        for (int i = 0;i < pairSize; i++) {
            if (fb->mFbType == planeCompPairs[i].srcFb) {
                destComp = planeCompPairs[i].destComp;
                if (planeCompPairs[i].destPlane == LEGACY_VIDEO_PLANE) {
                    if (mLegacyVideoPlane.get()) {
                        mDisplayPairs.push_back(DisplayPair{fb, mLegacyVideoPlane});
                        mLegacyVideoPlane.reset();
                    } else {
                        /*cursor plane used, handle it as a normal ui layer.*/
                        MESON_LOGE("too many layers need LEGACY_VIDEO_PLANE, discard. ");
                        destComp = MESON_COMPOSITION_DUMMY;
                    }
                    fb->mCompositionType = destComp;
                    break;
                } else if (planeCompPairs[i].destPlane == CURSOR_PLANE) {
                    if (mCursorPlane.get()) {
                        mDisplayPairs.push_back(DisplayPair{fb, mCursorPlane});
                        mCursorPlane.reset();
                    } else {
                        /*cursor plane used, handle it as a normal ui layer.*/
                        MESON_LOGE("too many layers need CURSOR_PLANE, skip. ");
                        destComp = MESON_COMPOSITION_UNDETERMINED;
                    }
                    fb->mCompositionType = destComp;
                    break;
                }
            }
        }
    }

    return 0;
}

int SingleplaneComposition::applyCompositionFlags() {
    if (!mForceClientComposer && !mHideSecureLayer)
        return 0;

    std::shared_ptr<DrmFramebuffer> fb;
    auto it = mFramebuffers.begin();
    for (; it != mFramebuffers.end(); ++it) {
        fb = *it;
        if (fb->mCompositionType == MESON_COMPOSITION_UNDETERMINED) {
            if (mHideSecureLayer && fb->mSecure) {
                fb->mCompositionType = MESON_COMPOSITION_DUMMY;
            } else if (mForceClientComposer) {
                fb->mCompositionType = MESON_COMPOSITION_CLIENT;
            }
        }
    }

    return 0;
}

bool SingleplaneComposition::isFbScanable(
    std::shared_ptr<DrmFramebuffer> & fb) {
    return (fb->mFbType == DRM_FB_SCANOUT) &&
        (!fb->isRotated());
}

int SingleplaneComposition::buildOsdComposition() {
    std::vector<std::shared_ptr<DrmFramebuffer>> dummyFbs;
    std::vector<std::shared_ptr<DrmFramebuffer>> dummyOverlayFbs;
    std::shared_ptr<DrmFramebuffer> fb;
    std::shared_ptr<DrmFramebuffer> videoFb;
    uint32_t  minZ = INVALID_ZORDER, maxZ = INVALID_ZORDER;
    bool bRemove = false, bHaveClient = false;

    /*
    * Remove dummy fbs, cursor plane for later osd composition.
    * For video overlay plane, keep it temply.
    */
    auto it = mFramebuffers.begin();
    for (; it != mFramebuffers.end(); ) {
        bRemove = false;
        fb = *it;
        switch (fb->mCompositionType) {
            case MESON_COMPOSITION_DUMMY:
                dummyFbs.push_back(fb);
                bRemove = true;
                break;
            case MESON_COMPOSITION_PLANE_CURSOR:
                bRemove = true;
                break;
            case MESON_COMPOSITION_PLANE_AMVIDEO:
            case MESON_COMPOSITION_PLANE_AMVIDEO_SIDEBAND:
                videoFb = fb;
                bRemove = true;
                break;
            case MESON_COMPOSITION_CLIENT:
                bHaveClient = true;
            case MESON_COMPOSITION_UNDETERMINED:
                if (minZ == INVALID_ZORDER) {
                    minZ = maxZ = fb->mZorder;
                } else {
                    if (minZ > fb->mZorder)
                        minZ = fb->mZorder;
                    if (maxZ < fb->mZorder)
                        maxZ  = fb->mZorder;
                }
                break;
            default:
                MESON_LOGE("Unkown compostition type(%d)", fb->mCompositionType);
                break;
        }

        if (bRemove)
            it = mFramebuffers.erase(it);
        else
            ++ it;
    }

    if (dummyFbs.size() > 0)
        mDummyComposer->addInputs(dummyFbs, dummyOverlayFbs);

    /*
    * Check if video fb is between other UI fbs.
    * If no, remove from list. Or keep it in UI list for composer
    * do overlay.
    */
    if (videoFb.get() &&
        (videoFb->mZorder > minZ && videoFb->mZorder < maxZ)) {
        mOverlayFbs.push_back(videoFb);
    }

    /*handle remaining fb post to osd*/
    if (mFramebuffers.size() == 0) {
        MESON_LOGD("No fbs post to osd, exit.");
    } else {
        auto it = mFramebuffers.begin();
        fb = *it;

        if (mFramebuffers.size() == 1 &&
            fb->mCompositionType == MESON_COMPOSITION_UNDETERMINED &&
            mOsdPlane->isFbSupport(fb)) {
            fb->mCompositionType = MESON_COMPOSITION_PLANE_OSD;
            mDisplayPairs.push_back(DisplayPair{fb, mOsdPlane});
            mOsdPlane.reset();
        } else {
            /*need compose before post display.*/
            /*select valid composer*/
            if (!bHaveClient) {
                auto composerIt = mComposers.begin();
                for (; composerIt != mComposers.end(); ++composerIt) {
                    if ((*composerIt)->isFbsSupport(mFramebuffers, mOverlayFbs)) {
                        mComposer = *composerIt;
                        break;
                    }
                }
            }
            if (mComposer.get() == NULL)
                mComposer = mClientComposer;

            /*set composed fb composition type*/
            for (; it != mFramebuffers.end(); ++it) {
                (*it)->mCompositionType = mComposer->getType();
            }
        }
    }
    return 0;
}

/*--------------------- commit to display ---------------------*/
int SingleplaneComposition::commit() {
    /*start compose, and add composer output.*/
    std::shared_ptr<DrmFramebuffer> composeOutput;
    if (mComposer.get()) {
        mComposer->start();
        composeOutput = mComposer->getOutput();
        mDisplayPairs.push_back(DisplayPair{composeOutput, mOsdPlane});
        mOsdPlane.reset();
    }

    display_zoom_info_t osdDisplayFrame;
    /*commit display path.*/
    auto displayIt = mDisplayPairs.begin();
    for (; displayIt != mDisplayPairs.end(); ++displayIt) {
        std::shared_ptr<DrmFramebuffer> fb = (*displayIt).fb;
        std::shared_ptr<HwDisplayPlane> plane = (*displayIt).plane;
        bool blankFlag = (mHideSecureLayer && fb->mSecure) ?
                BLANK_FOR_SECURE_CONTENT : UNBLANK;
        /*
        * SingleplaneCompositon handle fixed-zorder planes.
        * Video plane, cursor plane always have fixed zorder.
        * For osd, if the zorder is not fixed, set to default fixed osd plane.
        */
        uint32_t z  = plane->getFixedZorder();
        MESON_ASSERT(z != INVALID_ZORDER, "Not support zorder.");

        if (composeOutput == fb) {
            /*dump composer info*/
            bool bDumpPlane = true;
            auto it = mFramebuffers.begin();
            for (; it != mFramebuffers.end(); ++it) {
                if (bDumpPlane) {
                    dumpFbAndPlane(*it, plane, z, blankFlag);
                    bDumpPlane = false;
                } else {
                    dumpComposedFb(*it);
                }
            }
        } else {
            dumpFbAndPlane(fb, plane, z, blankFlag);
        }

        if (plane->getPlaneType() == OSD_PLANE) {
            osdDisplayFrame.framebuffer_w = fb->mSourceCrop.right - fb->mSourceCrop.left;
            osdDisplayFrame.framebuffer_h = fb->mSourceCrop.bottom - fb->mSourceCrop.top;
            osdDisplayFrame.crtc_display_x = 0;
            osdDisplayFrame.crtc_display_y = 0;
            osdDisplayFrame.crtc_display_w = fb->mDisplayFrame.right -
                fb->mDisplayFrame.left;
            osdDisplayFrame.crtc_display_h = fb->mDisplayFrame.bottom -
                fb->mDisplayFrame.top;
        }

        /*set display info*/
        plane->setPlane(fb, z, blankFlag);
    }

    /*blank useless plane.*/
    if (mCursorPlane.get())
        mUnusedPlanes.push_back(mCursorPlane);
    if (mLegacyVideoPlane.get())
        mUnusedPlanes.push_back(mLegacyVideoPlane);
    if (mOsdPlane.get())
        mUnusedPlanes.push_back(mOsdPlane);

    auto planeit = mUnusedPlanes.begin();
    for (;planeit != mUnusedPlanes.end(); ++planeit) {
        (*planeit)->setPlane(NULL, HWC_PLANE_FAKE_ZORDER, BLANK_FOR_NO_CONTENT);
        dumpUnusedPlane(*planeit, BLANK_FOR_NO_CONTENT);
    }

    /*set crtc info.*/
    mCrtc->setOsdChannels(1);
    mCrtc->setDisplayFrame(osdDisplayFrame);
    return 0;
}

