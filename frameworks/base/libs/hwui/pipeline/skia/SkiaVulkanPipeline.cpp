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

#include "SkiaVulkanPipeline.h"

#include "DeferredLayerUpdater.h"
#include "Readback.h"
#include "SkiaPipeline.h"
#include "SkiaProfileRenderer.h"
#include "VkLayer.h"
#include "renderstate/RenderState.h"
#include "renderthread/Frame.h"

#include <SkSurface.h>
#include <SkTypes.h>

#include <GrContext.h>
#include <GrTypes.h>
#include <vk/GrVkTypes.h>

#include <cutils/properties.h>
#include <strings.h>

using namespace android::uirenderer::renderthread;

namespace android {
namespace uirenderer {
namespace skiapipeline {

SkiaVulkanPipeline::SkiaVulkanPipeline(renderthread::RenderThread& thread)
        : SkiaPipeline(thread), mVkManager(thread.vulkanManager()) {}

MakeCurrentResult SkiaVulkanPipeline::makeCurrent() {
    return MakeCurrentResult::AlreadyCurrent;
}

Frame SkiaVulkanPipeline::getFrame() {
    LOG_ALWAYS_FATAL_IF(mVkSurface == nullptr,
                        "drawRenderNode called on a context with no surface!");

    SkSurface* backBuffer = mVkManager.getBackbufferSurface(mVkSurface);
    if (backBuffer == nullptr) {
        SkDebugf("failed to get backbuffer");
        return Frame(-1, -1, 0);
    }

    Frame frame(backBuffer->width(), backBuffer->height(), mVkManager.getAge(mVkSurface));
    return frame;
}

bool SkiaVulkanPipeline::draw(const Frame& frame, const SkRect& screenDirty, const SkRect& dirty,
                              const FrameBuilder::LightGeometry& lightGeometry,
                              LayerUpdateQueue* layerUpdateQueue, const Rect& contentDrawBounds,
                              bool opaque, bool wideColorGamut,
                              const BakedOpRenderer::LightInfo& lightInfo,
                              const std::vector<sp<RenderNode>>& renderNodes,
                              FrameInfoVisualizer* profiler) {
    sk_sp<SkSurface> backBuffer = mVkSurface->getBackBufferSurface();
    if (backBuffer.get() == nullptr) {
        return false;
    }
    SkiaPipeline::updateLighting(lightGeometry, lightInfo);
    renderFrame(*layerUpdateQueue, dirty, renderNodes, opaque, wideColorGamut, contentDrawBounds,
                backBuffer);
    layerUpdateQueue->clear();

    // Draw visual debugging features
    if (CC_UNLIKELY(Properties::showDirtyRegions ||
                    ProfileType::None != Properties::getProfileType())) {
        SkCanvas* profileCanvas = backBuffer->getCanvas();
        SkiaProfileRenderer profileRenderer(profileCanvas);
        profiler->draw(profileRenderer);
        profileCanvas->flush();
    }

    // Log memory statistics
    if (CC_UNLIKELY(Properties::debugLevel != kDebugDisabled)) {
        dumpResourceCacheUsage();
    }

    return true;
}

bool SkiaVulkanPipeline::swapBuffers(const Frame& frame, bool drew, const SkRect& screenDirty,
                                     FrameInfo* currentFrameInfo, bool* requireSwap) {
    *requireSwap = drew;

    // Even if we decided to cancel the frame, from the perspective of jank
    // metrics the frame was swapped at this point
    currentFrameInfo->markSwapBuffers();

    if (*requireSwap) {
        mVkManager.swapBuffers(mVkSurface);
    }

    return *requireSwap;
}

bool SkiaVulkanPipeline::copyLayerInto(DeferredLayerUpdater* layer, SkBitmap* bitmap) {
    // TODO: implement copyLayerInto for vulkan.
    return false;
}

static Layer* createLayer(RenderState& renderState, uint32_t layerWidth, uint32_t layerHeight,
                          sk_sp<SkColorFilter> colorFilter, int alpha, SkBlendMode mode,
                          bool blend) {
    return new VkLayer(renderState, layerWidth, layerHeight, colorFilter, alpha, mode, blend);
}

DeferredLayerUpdater* SkiaVulkanPipeline::createTextureLayer() {
    mVkManager.initialize();

    return new DeferredLayerUpdater(mRenderThread.renderState(), createLayer, Layer::Api::Vulkan);
}

void SkiaVulkanPipeline::onStop() {}

bool SkiaVulkanPipeline::setSurface(Surface* surface, SwapBehavior swapBehavior,
                                    ColorMode colorMode) {
    if (mVkSurface) {
        mVkManager.destroySurface(mVkSurface);
        mVkSurface = nullptr;
    }

    if (surface) {
        // TODO: handle color mode
        mVkSurface = mVkManager.createSurface(surface);
    }

    return mVkSurface != nullptr;
}

bool SkiaVulkanPipeline::isSurfaceReady() {
    return CC_UNLIKELY(mVkSurface != nullptr);
}

bool SkiaVulkanPipeline::isContextReady() {
    return CC_LIKELY(mVkManager.hasVkContext());
}

void SkiaVulkanPipeline::invokeFunctor(const RenderThread& thread, Functor* functor) {
    // TODO: we currently don't support OpenGL WebView's
    DrawGlInfo::Mode mode = DrawGlInfo::kModeProcessNoContext;
    (*functor)(mode, nullptr);
}

sk_sp<Bitmap> SkiaVulkanPipeline::allocateHardwareBitmap(renderthread::RenderThread& renderThread,
                                                         SkBitmap& skBitmap) {
    // TODO: implement this function for Vulkan pipeline
    // code below is a hack to avoid crashing because of missing HW Bitmap support
    sp<GraphicBuffer> buffer = new GraphicBuffer(
            skBitmap.info().width(), skBitmap.info().height(), PIXEL_FORMAT_RGBA_8888,
            GraphicBuffer::USAGE_HW_TEXTURE | GraphicBuffer::USAGE_SW_WRITE_NEVER |
                    GraphicBuffer::USAGE_SW_READ_NEVER,
            std::string("SkiaVulkanPipeline::allocateHardwareBitmap pid [") +
                    std::to_string(getpid()) + "]");
    status_t error = buffer->initCheck();
    if (error < 0) {
        ALOGW("SkiaVulkanPipeline::allocateHardwareBitmap() failed in GraphicBuffer.create()");
        return nullptr;
    }
    return sk_sp<Bitmap>(new Bitmap(buffer.get(), skBitmap.info()));
}

} /* namespace skiapipeline */
} /* namespace uirenderer */
} /* namespace android */
