/*
 * Copyright (C) 2016 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "SkiaDisplayList.h"

#include "DumpOpsCanvas.h"
#include "SkiaPipeline.h"
#include "VectorDrawable.h"
#include "renderthread/CanvasContext.h"

#include <SkImagePriv.h>

namespace android {
namespace uirenderer {
namespace skiapipeline {

void SkiaDisplayList::syncContents() {
    for (auto& functor : mChildFunctors) {
        functor.syncFunctor();
    }
    for (auto& animatedImage : mAnimatedImages) {
        animatedImage->syncProperties();
    }
    for (auto& vectorDrawable : mVectorDrawables) {
        vectorDrawable->syncProperties();
    }
}

bool SkiaDisplayList::reuseDisplayList(RenderNode* node, renderthread::CanvasContext* context) {
    reset();
    node->attachAvailableList(this);
    return true;
}

void SkiaDisplayList::updateChildren(std::function<void(RenderNode*)> updateFn) {
    for (auto& child : mChildNodes) {
        updateFn(child.getRenderNode());
    }
}

bool SkiaDisplayList::prepareListAndChildren(
        TreeObserver& observer, TreeInfo& info, bool functorsNeedLayer,
        std::function<void(RenderNode*, TreeObserver&, TreeInfo&, bool)> childFn) {
    // If the prepare tree is triggered by the UI thread and no previous call to
    // pinImages has failed then we must pin all mutable images in the GPU cache
    // until the next UI thread draw.
    if (info.prepareTextures && !info.canvasContext.pinImages(mMutableImages)) {
        // In the event that pinning failed we prevent future pinImage calls for the
        // remainder of this tree traversal and also unpin any currently pinned images
        // to free up GPU resources.
        info.prepareTextures = false;
        info.canvasContext.unpinImages();
    }

    bool hasBackwardProjectedNodesHere = false;
    bool hasBackwardProjectedNodesSubtree = false;

    for (auto& child : mChildNodes) {
        hasBackwardProjectedNodesHere |= child.getNodeProperties().getProjectBackwards();
        RenderNode* childNode = child.getRenderNode();
        Matrix4 mat4(child.getRecordedMatrix());
        info.damageAccumulator->pushTransform(&mat4);
        // TODO: a layer is needed if the canvas is rotated or has a non-rect clip
        info.hasBackwardProjectedNodes = false;
        childFn(childNode, observer, info, functorsNeedLayer);
        hasBackwardProjectedNodesSubtree |= info.hasBackwardProjectedNodes;
        info.damageAccumulator->popTransform();
    }

    // The purpose of next block of code is to reset projected display list if there are no
    // backward projected nodes. This speeds up drawing, by avoiding an extra walk of the tree
    if (mProjectionReceiver) {
        mProjectionReceiver->setProjectedDisplayList(hasBackwardProjectedNodesSubtree ? this
                                                                                      : nullptr);
        info.hasBackwardProjectedNodes = hasBackwardProjectedNodesHere;
    } else {
        info.hasBackwardProjectedNodes =
                hasBackwardProjectedNodesSubtree || hasBackwardProjectedNodesHere;
    }

    bool isDirty = false;
    for (auto& animatedImage : mAnimatedImages) {
        nsecs_t timeTilNextFrame = TreeInfo::Out::kNoAnimatedImageDelay;
        // If any animated image in the display list needs updated, then damage the node.
        if (animatedImage->isDirty(&timeTilNextFrame)) {
            isDirty = true;
        }

        if (animatedImage->isRunning() &&
            timeTilNextFrame != TreeInfo::Out::kNoAnimatedImageDelay) {
            auto& delay = info.out.animatedImageDelay;
            if (delay == TreeInfo::Out::kNoAnimatedImageDelay || timeTilNextFrame < delay) {
                delay = timeTilNextFrame;
            }
        }
    }

    for (auto& vectorDrawable : mVectorDrawables) {
        // If any vector drawable in the display list needs update, damage the node.
        if (vectorDrawable->isDirty()) {
            isDirty = true;
            static_cast<SkiaPipeline*>(info.canvasContext.getRenderPipeline())
                    ->getVectorDrawables()
                    ->push_back(vectorDrawable);
        }
        vectorDrawable->setPropertyChangeWillBeConsumed(true);
    }
    return isDirty;
}

void SkiaDisplayList::reset() {
    mProjectionReceiver = nullptr;

    mDisplayList.reset();

    mMutableImages.clear();
    mVectorDrawables.clear();
    mAnimatedImages.clear();
    mChildFunctors.clear();
    mChildNodes.clear();

    projectionReceiveIndex = -1;
    allocator.~LinearAllocator();
    new (&allocator) LinearAllocator();
}

void SkiaDisplayList::output(std::ostream& output, uint32_t level) {
    DumpOpsCanvas canvas(output, level, *this);
    mDisplayList.draw(&canvas);
}

};  // namespace skiapipeline
};  // namespace uirenderer
};  // namespace android
