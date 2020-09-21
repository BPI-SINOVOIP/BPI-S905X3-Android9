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

#include <VectorDrawable.h>
#include <gtest/gtest.h>

#include <SkClipStack.h>
#include <SkLiteRecorder.h>
#include <SkSurface_Base.h>
#include <string.h>
#include "AnimationContext.h"
#include "DamageAccumulator.h"
#include "IContextFactory.h"
#include "SkiaCanvas.h"
#include "pipeline/skia/SkiaDisplayList.h"
#include "pipeline/skia/SkiaOpenGLPipeline.h"
#include "pipeline/skia/SkiaRecordingCanvas.h"
#include "renderthread/CanvasContext.h"
#include "tests/common/TestUtils.h"

using namespace android;
using namespace android::uirenderer;
using namespace android::uirenderer::renderthread;
using namespace android::uirenderer::skiapipeline;

RENDERTHREAD_SKIA_PIPELINE_TEST(SkiaPipeline, renderFrame) {
    auto redNode = TestUtils::createSkiaNode(
            0, 0, 1, 1, [](RenderProperties& props, SkiaRecordingCanvas& redCanvas) {
                redCanvas.drawColor(SK_ColorRED, SkBlendMode::kSrcOver);
            });
    LayerUpdateQueue layerUpdateQueue;
    SkRect dirty = SkRect::MakeLargest();
    std::vector<sp<RenderNode>> renderNodes;
    renderNodes.push_back(redNode);
    bool opaque = true;
    android::uirenderer::Rect contentDrawBounds(0, 0, 1, 1);
    auto pipeline = std::make_unique<SkiaOpenGLPipeline>(renderThread);
    auto surface = SkSurface::MakeRasterN32Premul(1, 1);
    surface->getCanvas()->drawColor(SK_ColorBLUE, SkBlendMode::kSrcOver);
    ASSERT_EQ(TestUtils::getColor(surface, 0, 0), SK_ColorBLUE);
    pipeline->renderFrame(layerUpdateQueue, dirty, renderNodes, opaque, false, contentDrawBounds,
                          surface);
    ASSERT_EQ(TestUtils::getColor(surface, 0, 0), SK_ColorRED);
}

RENDERTHREAD_SKIA_PIPELINE_TEST(SkiaPipeline, testOnPrepareTree) {
    auto redNode = TestUtils::createSkiaNode(
            0, 0, 1, 1, [](RenderProperties& props, SkiaRecordingCanvas& redCanvas) {
                redCanvas.drawColor(SK_ColorRED, SkBlendMode::kSrcOver);
            });

    LayerUpdateQueue layerUpdateQueue;
    SkRect dirty = SkRect::MakeLargest();
    std::vector<sp<RenderNode>> renderNodes;
    renderNodes.push_back(redNode);
    bool opaque = true;
    android::uirenderer::Rect contentDrawBounds(0, 0, 1, 1);
    auto pipeline = std::make_unique<SkiaOpenGLPipeline>(renderThread);
    {
        // add a pointer to a deleted vector drawable object in the pipeline
        sp<VectorDrawableRoot> dirtyVD(new VectorDrawableRoot(new VectorDrawable::Group()));
        dirtyVD->mutateProperties()->setScaledSize(5, 5);
        pipeline->getVectorDrawables()->push_back(dirtyVD.get());
    }

    // pipeline should clean list of dirty vector drawables before prepare tree
    pipeline->onPrepareTree();

    auto surface = SkSurface::MakeRasterN32Premul(1, 1);
    surface->getCanvas()->drawColor(SK_ColorBLUE, SkBlendMode::kSrcOver);
    ASSERT_EQ(TestUtils::getColor(surface, 0, 0), SK_ColorBLUE);

    // drawFrame will crash if "SkiaPipeline::onPrepareTree" did not clean invalid VD pointer
    pipeline->renderFrame(layerUpdateQueue, dirty, renderNodes, opaque, false, contentDrawBounds,
                          surface);
    ASSERT_EQ(TestUtils::getColor(surface, 0, 0), SK_ColorRED);
}

RENDERTHREAD_SKIA_PIPELINE_TEST(SkiaPipeline, renderFrameCheckOpaque) {
    auto halfGreenNode = TestUtils::createSkiaNode(
            0, 0, 2, 2, [](RenderProperties& props, SkiaRecordingCanvas& bottomHalfGreenCanvas) {
                SkPaint greenPaint;
                greenPaint.setColor(SK_ColorGREEN);
                greenPaint.setStyle(SkPaint::kFill_Style);
                bottomHalfGreenCanvas.drawRect(0, 1, 2, 2, greenPaint);
            });
    LayerUpdateQueue layerUpdateQueue;
    SkRect dirty = SkRect::MakeLargest();
    std::vector<sp<RenderNode>> renderNodes;
    renderNodes.push_back(halfGreenNode);
    android::uirenderer::Rect contentDrawBounds(0, 0, 2, 2);
    auto pipeline = std::make_unique<SkiaOpenGLPipeline>(renderThread);
    auto surface = SkSurface::MakeRasterN32Premul(2, 2);
    surface->getCanvas()->drawColor(SK_ColorBLUE, SkBlendMode::kSrcOver);
    ASSERT_EQ(TestUtils::getColor(surface, 0, 0), SK_ColorBLUE);
    pipeline->renderFrame(layerUpdateQueue, dirty, renderNodes, true, false, contentDrawBounds,
                          surface);
    ASSERT_EQ(TestUtils::getColor(surface, 0, 0), SK_ColorBLUE);
    ASSERT_EQ(TestUtils::getColor(surface, 0, 1), SK_ColorGREEN);
    pipeline->renderFrame(layerUpdateQueue, dirty, renderNodes, false, false, contentDrawBounds,
                          surface);
    ASSERT_EQ(TestUtils::getColor(surface, 0, 0), (unsigned int)SK_ColorTRANSPARENT);
    ASSERT_EQ(TestUtils::getColor(surface, 0, 1), SK_ColorGREEN);
}

RENDERTHREAD_SKIA_PIPELINE_TEST(SkiaPipeline, renderFrameCheckDirtyRect) {
    auto redNode = TestUtils::createSkiaNode(
            0, 0, 2, 2, [](RenderProperties& props, SkiaRecordingCanvas& redCanvas) {
                redCanvas.drawColor(SK_ColorRED, SkBlendMode::kSrcOver);
            });
    LayerUpdateQueue layerUpdateQueue;
    SkRect dirty = SkRect::MakeXYWH(0, 1, 2, 1);
    std::vector<sp<RenderNode>> renderNodes;
    renderNodes.push_back(redNode);
    android::uirenderer::Rect contentDrawBounds(0, 0, 2, 2);
    auto pipeline = std::make_unique<SkiaOpenGLPipeline>(renderThread);
    auto surface = SkSurface::MakeRasterN32Premul(2, 2);
    surface->getCanvas()->drawColor(SK_ColorBLUE, SkBlendMode::kSrcOver);
    ASSERT_EQ(TestUtils::getColor(surface, 0, 0), SK_ColorBLUE);
    pipeline->renderFrame(layerUpdateQueue, dirty, renderNodes, true, false, contentDrawBounds,
                          surface);
    ASSERT_EQ(TestUtils::getColor(surface, 0, 0), SK_ColorBLUE);
    ASSERT_EQ(TestUtils::getColor(surface, 1, 0), SK_ColorBLUE);
    ASSERT_EQ(TestUtils::getColor(surface, 0, 1), SK_ColorRED);
    ASSERT_EQ(TestUtils::getColor(surface, 1, 1), SK_ColorRED);
}

RENDERTHREAD_SKIA_PIPELINE_TEST(SkiaPipeline, renderLayer) {
    auto redNode = TestUtils::createSkiaNode(
            0, 0, 1, 1, [](RenderProperties& props, SkiaRecordingCanvas& redCanvas) {
                redCanvas.drawColor(SK_ColorRED, SkBlendMode::kSrcOver);
            });
    auto surfaceLayer1 = SkSurface::MakeRasterN32Premul(1, 1);
    surfaceLayer1->getCanvas()->drawColor(SK_ColorWHITE, SkBlendMode::kSrcOver);
    ASSERT_EQ(TestUtils::getColor(surfaceLayer1, 0, 0), SK_ColorWHITE);
    redNode->setLayerSurface(surfaceLayer1);

    // create a 2nd 2x2 layer and add it to the queue as well.
    // make the layer's dirty area one half of the layer and verify only the dirty half is updated.
    auto blueNode = TestUtils::createSkiaNode(
            0, 0, 2, 2, [](RenderProperties& props, SkiaRecordingCanvas& blueCanvas) {
                blueCanvas.drawColor(SK_ColorBLUE, SkBlendMode::kSrcOver);
            });
    auto surfaceLayer2 = SkSurface::MakeRasterN32Premul(2, 2);
    surfaceLayer2->getCanvas()->drawColor(SK_ColorWHITE, SkBlendMode::kSrcOver);
    ASSERT_EQ(TestUtils::getColor(surfaceLayer2, 0, 0), SK_ColorWHITE);
    blueNode->setLayerSurface(surfaceLayer2);

    // attach both layers to the update queue
    LayerUpdateQueue layerUpdateQueue;
    SkRect dirty = SkRect::MakeLargest();
    layerUpdateQueue.enqueueLayerWithDamage(redNode.get(), dirty);
    layerUpdateQueue.enqueueLayerWithDamage(blueNode.get(), SkRect::MakeWH(2, 1));
    ASSERT_EQ(layerUpdateQueue.entries().size(), 2UL);

    bool opaque = true;
    FrameBuilder::LightGeometry lightGeometry;
    lightGeometry.radius = 1.0f;
    lightGeometry.center = {0.0f, 0.0f, 0.0f};
    BakedOpRenderer::LightInfo lightInfo;
    auto pipeline = std::make_unique<SkiaOpenGLPipeline>(renderThread);
    pipeline->renderLayers(lightGeometry, &layerUpdateQueue, opaque, false, lightInfo);
    ASSERT_EQ(TestUtils::getColor(surfaceLayer1, 0, 0), SK_ColorRED);
    ASSERT_EQ(TestUtils::getColor(surfaceLayer2, 0, 0), SK_ColorBLUE);
    ASSERT_EQ(TestUtils::getColor(surfaceLayer2, 0, 1), SK_ColorWHITE);
    ASSERT_TRUE(layerUpdateQueue.entries().empty());
    redNode->setLayerSurface(sk_sp<SkSurface>());
    blueNode->setLayerSurface(sk_sp<SkSurface>());
}

RENDERTHREAD_SKIA_PIPELINE_TEST(SkiaPipeline, renderOverdraw) {
    ScopedProperty<bool> prop(Properties::debugOverdraw, true);

    auto whiteNode = TestUtils::createSkiaNode(
            0, 0, 1, 1, [](RenderProperties& props, SkiaRecordingCanvas& canvas) {
                canvas.drawColor(SK_ColorWHITE, SkBlendMode::kSrcOver);
            });
    LayerUpdateQueue layerUpdateQueue;
    SkRect dirty = SkRect::MakeXYWH(0, 0, 1, 1);
    std::vector<sp<RenderNode>> renderNodes;
    renderNodes.push_back(whiteNode);
    bool opaque = true;
    // empty contentDrawBounds is avoiding backdrop/content logic, which would lead to less overdraw
    android::uirenderer::Rect contentDrawBounds(0, 0, 0, 0);
    auto pipeline = std::make_unique<SkiaOpenGLPipeline>(renderThread);
    auto surface = SkSurface::MakeRasterN32Premul(1, 1);

    // Initialize the canvas to blue.
    surface->getCanvas()->drawColor(SK_ColorBLUE, SkBlendMode::kSrcOver);
    ASSERT_EQ(TestUtils::getColor(surface, 0, 0), SK_ColorBLUE);

    // Single draw, should be white.
    pipeline->renderFrame(layerUpdateQueue, dirty, renderNodes, opaque, false, contentDrawBounds,
                          surface);
    ASSERT_EQ(TestUtils::getColor(surface, 0, 0), SK_ColorWHITE);

    // 1 Overdraw, should be blue blended onto white.
    renderNodes.push_back(whiteNode);
    pipeline->renderFrame(layerUpdateQueue, dirty, renderNodes, opaque, false, contentDrawBounds,
                          surface);
    ASSERT_EQ(TestUtils::getColor(surface, 0, 0), (unsigned)0xffd0d0ff);

    // 2 Overdraw, should be green blended onto white
    renderNodes.push_back(whiteNode);
    pipeline->renderFrame(layerUpdateQueue, dirty, renderNodes, opaque, false, contentDrawBounds,
                          surface);
    ASSERT_EQ(TestUtils::getColor(surface, 0, 0), (unsigned)0xffd0ffd0);

    // 3 Overdraw, should be pink blended onto white.
    renderNodes.push_back(whiteNode);
    pipeline->renderFrame(layerUpdateQueue, dirty, renderNodes, opaque, false, contentDrawBounds,
                          surface);
    ASSERT_EQ(TestUtils::getColor(surface, 0, 0), (unsigned)0xffffc0c0);

    // 4 Overdraw, should be red blended onto white.
    renderNodes.push_back(whiteNode);
    pipeline->renderFrame(layerUpdateQueue, dirty, renderNodes, opaque, false, contentDrawBounds,
                          surface);
    ASSERT_EQ(TestUtils::getColor(surface, 0, 0), (unsigned)0xffff8080);

    // 5 Overdraw, should be red blended onto white.
    renderNodes.push_back(whiteNode);
    pipeline->renderFrame(layerUpdateQueue, dirty, renderNodes, opaque, false, contentDrawBounds,
                          surface);
    ASSERT_EQ(TestUtils::getColor(surface, 0, 0), (unsigned)0xffff8080);
}

namespace {
template <typename T>
class DeferLayer : public SkSurface_Base {
public:
    DeferLayer() : SkSurface_Base(T().imageInfo(), nullptr) {}
    virtual ~DeferLayer() {}

    SkCanvas* onNewCanvas() override { return new T(); }
    sk_sp<SkSurface> onNewSurface(const SkImageInfo&) override { return nullptr; }
    sk_sp<SkImage> onNewImageSnapshot() override { return nullptr; }
    T* canvas() { return static_cast<T*>(getCanvas()); }
    void onCopyOnWrite(ContentChangeMode) override {}
    void onWritePixels(const SkPixmap&, int x, int y) override {}
};
}

RENDERTHREAD_SKIA_PIPELINE_TEST(SkiaPipeline, deferRenderNodeScene) {
    class DeferTestCanvas : public SkCanvas {
    public:
        DeferTestCanvas() : SkCanvas(800, 600) {}
        void onDrawRect(const SkRect& rect, const SkPaint& paint) override {
            SkMatrix expected;
            switch (mDrawCounter++) {
                case 0:
                    // background - left side
                    EXPECT_EQ(SkRect::MakeLTRB(600, 100, 700, 500), TestUtils::getClipBounds(this));
                    expected.setTranslate(100, 100);
                    break;
                case 1:
                    // background - top side
                    EXPECT_EQ(SkRect::MakeLTRB(100, 400, 600, 500), TestUtils::getClipBounds(this));
                    expected.setTranslate(100, 100);
                    break;
                case 2:
                    // content
                    EXPECT_EQ(SkRect::MakeLTRB(100, 100, 700, 500), TestUtils::getClipBounds(this));
                    expected.setTranslate(-50, -50);
                    break;
                case 3:
                    // overlay
                    EXPECT_EQ(SkRect::MakeLTRB(0, 0, 800, 600), TestUtils::getClipBounds(this));
                    expected.reset();
                    break;
                default:
                    ADD_FAILURE() << "Too many rects observed";
            }
            EXPECT_EQ(expected, getTotalMatrix());
        }
        int mDrawCounter = 0;
    };

    std::vector<sp<RenderNode>> nodes;
    SkPaint transparentPaint;
    transparentPaint.setAlpha(128);

    // backdrop
    nodes.push_back(TestUtils::createSkiaNode(
            100, 100, 700, 500,  // 600x400
            [&transparentPaint](RenderProperties& props, SkiaRecordingCanvas& canvas) {
                canvas.drawRect(0, 0, 600, 400, transparentPaint);
            }));

    // content
    android::uirenderer::Rect contentDrawBounds(150, 150, 650, 450);  // 500x300
    nodes.push_back(TestUtils::createSkiaNode(
            0, 0, 800, 600,
            [&transparentPaint](RenderProperties& props, SkiaRecordingCanvas& canvas) {
                canvas.drawRect(0, 0, 800, 600, transparentPaint);
            }));

    // overlay
    nodes.push_back(TestUtils::createSkiaNode(
            0, 0, 800, 600,
            [&transparentPaint](RenderProperties& props, SkiaRecordingCanvas& canvas) {
                canvas.drawRect(0, 0, 800, 200, transparentPaint);
            }));

    LayerUpdateQueue layerUpdateQueue;
    SkRect dirty = SkRect::MakeWH(800, 600);
    auto pipeline = std::make_unique<SkiaOpenGLPipeline>(renderThread);
    sk_sp<DeferLayer<DeferTestCanvas>> surface(new DeferLayer<DeferTestCanvas>());
    pipeline->renderFrame(layerUpdateQueue, dirty, nodes, true, false, contentDrawBounds, surface);
    EXPECT_EQ(4, surface->canvas()->mDrawCounter);
}

RENDERTHREAD_SKIA_PIPELINE_TEST(SkiaPipeline, clipped) {
    static const int CANVAS_WIDTH = 200;
    static const int CANVAS_HEIGHT = 200;
    class ClippedTestCanvas : public SkCanvas {
    public:
        ClippedTestCanvas() : SkCanvas(CANVAS_WIDTH, CANVAS_HEIGHT) {}
        void onDrawImage(const SkImage*, SkScalar dx, SkScalar dy, const SkPaint*) override {
            EXPECT_EQ(0, mDrawCounter++);
            EXPECT_EQ(SkRect::MakeLTRB(10, 20, 30, 40), TestUtils::getClipBounds(this));
            EXPECT_TRUE(getTotalMatrix().isIdentity());
        }
        int mDrawCounter = 0;
    };

    std::vector<sp<RenderNode>> nodes;
    nodes.push_back(TestUtils::createSkiaNode(
            0, 0, CANVAS_WIDTH, CANVAS_HEIGHT,
            [](RenderProperties& props, SkiaRecordingCanvas& canvas) {
                sk_sp<Bitmap> bitmap(TestUtils::createBitmap(CANVAS_WIDTH, CANVAS_HEIGHT));
                canvas.drawBitmap(*bitmap, 0, 0, nullptr);
            }));

    LayerUpdateQueue layerUpdateQueue;
    SkRect dirty = SkRect::MakeLTRB(10, 20, 30, 40);
    auto pipeline = std::make_unique<SkiaOpenGLPipeline>(renderThread);
    sk_sp<DeferLayer<ClippedTestCanvas>> surface(new DeferLayer<ClippedTestCanvas>());
    pipeline->renderFrame(layerUpdateQueue, dirty, nodes, true, false,
                          SkRect::MakeWH(CANVAS_WIDTH, CANVAS_HEIGHT), surface);
    EXPECT_EQ(1, surface->canvas()->mDrawCounter);
}

RENDERTHREAD_SKIA_PIPELINE_TEST(SkiaPipeline, clip_replace) {
    static const int CANVAS_WIDTH = 50;
    static const int CANVAS_HEIGHT = 50;
    class ClipReplaceTestCanvas : public SkCanvas {
    public:
        ClipReplaceTestCanvas() : SkCanvas(CANVAS_WIDTH, CANVAS_HEIGHT) {}
        void onDrawPaint(const SkPaint&) {
            EXPECT_EQ(0, mDrawCounter++);
            EXPECT_EQ(SkRect::MakeLTRB(20, 10, 30, 40), TestUtils::getClipBounds(this))
                    << "Expect resolved clip to be intersection of viewport clip and clip op";
        }
        int mDrawCounter = 0;
    };

    std::vector<sp<RenderNode>> nodes;
    nodes.push_back(TestUtils::createSkiaNode(
            20, 20, 30, 30, [](RenderProperties& props, SkiaRecordingCanvas& canvas) {
                canvas.clipRect(0, -20, 10, 30, SkClipOp::kReplace_deprecated);
                canvas.drawColor(SK_ColorWHITE, SkBlendMode::kSrcOver);
            }));

    LayerUpdateQueue layerUpdateQueue;
    SkRect dirty = SkRect::MakeLTRB(10, 10, 40, 40);
    auto pipeline = std::make_unique<SkiaOpenGLPipeline>(renderThread);
    sk_sp<DeferLayer<ClipReplaceTestCanvas>> surface(new DeferLayer<ClipReplaceTestCanvas>());
    pipeline->renderFrame(layerUpdateQueue, dirty, nodes, true, false,
                          SkRect::MakeWH(CANVAS_WIDTH, CANVAS_HEIGHT), surface);
    EXPECT_EQ(1, surface->canvas()->mDrawCounter);
}
