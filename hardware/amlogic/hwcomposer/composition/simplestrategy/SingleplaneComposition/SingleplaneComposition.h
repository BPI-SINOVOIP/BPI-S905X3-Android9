/*
 * Copyright (c) 2018 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */

#ifndef SINGLEPLANE_COMPOSITION_H
#define SINGLEPLANE_COMPOSITION_H

#include <BasicTypes.h>
#include <HwDisplayPlane.h>
#include <ICompositionStrategy.h>

/*
SingleplaneComposition is a strategy for early vpu architecture:
	---------------------
	|	cursor plane	|	TOP
	---------------------
	|	 osd plane  	|	MIDDLE
	---------------------
	|legacy video plane	|	DOWN
	---------------------
*/
class SingleplaneComposition : public ICompositionStrategy {
public:
    SingleplaneComposition();
    ~SingleplaneComposition();

    const char* getName() {return "SingleplaneComposition";}

    void setup(std::vector<std::shared_ptr<DrmFramebuffer>> & layers,
        std::vector<std::shared_ptr<IComposer>> & composers,
        std::vector<std::shared_ptr<HwDisplayPlane>> & planes,
        std::shared_ptr<HwDisplayCrtc> & crtc,
        uint32_t flags);

    int decideComposition();
    int commit();

protected:
    void cleanup();

    int applyCompositionFlags();
    int processFbsOfExplicitComposition();
    int buildOsdComposition();

    bool isFbScanable(std::shared_ptr<DrmFramebuffer> & fb);

protected:
    struct DisplayPair {
        std::shared_ptr<DrmFramebuffer> fb;
        std::shared_ptr<HwDisplayPlane> plane;
    };

    /*Compostion flags*/
    bool mForceClientComposer;
    bool mHideSecureLayer;

    /*composers*/
    std::shared_ptr<IComposer> mDummyComposer;
    std::shared_ptr<IComposer> mClientComposer;
    std::vector<std::shared_ptr<IComposer>> mComposers;

    /*crtc*/
    std::shared_ptr<HwDisplayCrtc> mCrtc;

    /*display planes*/
    std::shared_ptr<HwDisplayPlane> mCursorPlane;
    std::shared_ptr<HwDisplayPlane> mOsdPlane;
    std::shared_ptr<HwDisplayPlane> mLegacyVideoPlane;
    std::vector<std::shared_ptr<HwDisplayPlane>> mUnusedPlanes;

    /*input fbs*/
    std::vector<std::shared_ptr<DrmFramebuffer>> mFramebuffers;

    /*members used for composition*/
    std::vector<std::shared_ptr<DrmFramebuffer>> mOverlayFbs;
    std::shared_ptr<IComposer> mComposer;
    std::list<DisplayPair> mDisplayPairs;
};

#endif/*SINGLEPLANE_COMPOSITION_H*/
