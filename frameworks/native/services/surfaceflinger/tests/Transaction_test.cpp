/*
 * Copyright (C) 2011 The Android Open Source Project
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

#include <algorithm>
#include <functional>
#include <limits>
#include <ostream>

#include <gtest/gtest.h>

#include <android/native_window.h>

#include <gui/ISurfaceComposer.h>
#include <gui/LayerState.h>

#include <gui/Surface.h>
#include <gui/SurfaceComposerClient.h>
#include <private/gui/ComposerService.h>

#include <ui/DisplayInfo.h>
#include <ui/Rect.h>
#include <utils/String8.h>

#include <math.h>
#include <math/vec3.h>

namespace android {

namespace {

struct Color {
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t a;

    static const Color RED;
    static const Color GREEN;
    static const Color BLUE;
    static const Color WHITE;
    static const Color BLACK;
    static const Color TRANSPARENT;
};

const Color Color::RED{255, 0, 0, 255};
const Color Color::GREEN{0, 255, 0, 255};
const Color Color::BLUE{0, 0, 255, 255};
const Color Color::WHITE{255, 255, 255, 255};
const Color Color::BLACK{0, 0, 0, 255};
const Color Color::TRANSPARENT{0, 0, 0, 0};

std::ostream& operator<<(std::ostream& os, const Color& color) {
    os << int(color.r) << ", " << int(color.g) << ", " << int(color.b) << ", " << int(color.a);
    return os;
}

// Fill a region with the specified color.
void fillBufferColor(const ANativeWindow_Buffer& buffer, const Rect& rect, const Color& color) {
    int32_t x = rect.left;
    int32_t y = rect.top;
    int32_t width = rect.right - rect.left;
    int32_t height = rect.bottom - rect.top;

    if (x < 0) {
        width += x;
        x = 0;
    }
    if (y < 0) {
        height += y;
        y = 0;
    }
    if (x + width > buffer.width) {
        x = std::min(x, buffer.width);
        width = buffer.width - x;
    }
    if (y + height > buffer.height) {
        y = std::min(y, buffer.height);
        height = buffer.height - y;
    }

    for (int32_t j = 0; j < height; j++) {
        uint8_t* dst = static_cast<uint8_t*>(buffer.bits) + (buffer.stride * (y + j) + x) * 4;
        for (int32_t i = 0; i < width; i++) {
            dst[0] = color.r;
            dst[1] = color.g;
            dst[2] = color.b;
            dst[3] = color.a;
            dst += 4;
        }
    }
}

// Check if a region has the specified color.
void expectBufferColor(const sp<GraphicBuffer>& outBuffer, uint8_t* pixels, const Rect& rect,
                       const Color& color, uint8_t tolerance) {
    int32_t x = rect.left;
    int32_t y = rect.top;
    int32_t width = rect.right - rect.left;
    int32_t height = rect.bottom - rect.top;

    int32_t bufferWidth = int32_t(outBuffer->getWidth());
    int32_t bufferHeight = int32_t(outBuffer->getHeight());
    if (x + width > bufferWidth) {
        x = std::min(x, bufferWidth);
        width = bufferWidth - x;
    }
    if (y + height > bufferHeight) {
        y = std::min(y, bufferHeight);
        height = bufferHeight - y;
    }

    auto colorCompare = [tolerance](uint8_t a, uint8_t b) {
        uint8_t tmp = a >= b ? a - b : b - a;
        return tmp <= tolerance;
    };
    for (int32_t j = 0; j < height; j++) {
        const uint8_t* src = pixels + (outBuffer->getStride() * (y + j) + x) * 4;
        for (int32_t i = 0; i < width; i++) {
            const uint8_t expected[4] = {color.r, color.g, color.b, color.a};
            EXPECT_TRUE(std::equal(src, src + 4, expected, colorCompare))
                    << "pixel @ (" << x + i << ", " << y + j << "): "
                    << "expected (" << color << "), "
                    << "got (" << Color{src[0], src[1], src[2], src[3]} << ")";
            src += 4;
        }
    }
}

} // anonymous namespace

using Transaction = SurfaceComposerClient::Transaction;

// Fill an RGBA_8888 formatted surface with a single color.
static void fillSurfaceRGBA8(const sp<SurfaceControl>& sc, uint8_t r, uint8_t g, uint8_t b,
                             bool unlock = true) {
    ANativeWindow_Buffer outBuffer;
    sp<Surface> s = sc->getSurface();
    ASSERT_TRUE(s != nullptr);
    ASSERT_EQ(NO_ERROR, s->lock(&outBuffer, nullptr));
    uint8_t* img = reinterpret_cast<uint8_t*>(outBuffer.bits);
    for (int y = 0; y < outBuffer.height; y++) {
        for (int x = 0; x < outBuffer.width; x++) {
            uint8_t* pixel = img + (4 * (y * outBuffer.stride + x));
            pixel[0] = r;
            pixel[1] = g;
            pixel[2] = b;
            pixel[3] = 255;
        }
    }
    if (unlock) {
        ASSERT_EQ(NO_ERROR, s->unlockAndPost());
    }
}

// A ScreenCapture is a screenshot from SurfaceFlinger that can be used to check
// individual pixel values for testing purposes.
class ScreenCapture : public RefBase {
public:
    static void captureScreen(sp<ScreenCapture>* sc, int32_t minLayerZ = 0,
                              int32_t maxLayerZ = std::numeric_limits<int32_t>::max()) {
        sp<ISurfaceComposer> sf(ComposerService::getComposerService());
        sp<IBinder> display(sf->getBuiltInDisplay(ISurfaceComposer::eDisplayIdMain));
        SurfaceComposerClient::Transaction().apply(true);

        sp<GraphicBuffer> outBuffer;
        ASSERT_EQ(NO_ERROR,
                  sf->captureScreen(display, &outBuffer, Rect(), 0, 0, minLayerZ, maxLayerZ,
                                    false));
        *sc = new ScreenCapture(outBuffer);
    }

    static void captureLayers(std::unique_ptr<ScreenCapture>* sc, sp<IBinder>& parentHandle,
                              Rect crop = Rect::EMPTY_RECT, float frameScale = 1.0) {
        sp<ISurfaceComposer> sf(ComposerService::getComposerService());
        SurfaceComposerClient::Transaction().apply(true);

        sp<GraphicBuffer> outBuffer;
        ASSERT_EQ(NO_ERROR, sf->captureLayers(parentHandle, &outBuffer, crop, frameScale));
        *sc = std::make_unique<ScreenCapture>(outBuffer);
    }

    static void captureChildLayers(std::unique_ptr<ScreenCapture>* sc, sp<IBinder>& parentHandle,
                                   Rect crop = Rect::EMPTY_RECT, float frameScale = 1.0) {
        sp<ISurfaceComposer> sf(ComposerService::getComposerService());
        SurfaceComposerClient::Transaction().apply(true);

        sp<GraphicBuffer> outBuffer;
        ASSERT_EQ(NO_ERROR, sf->captureLayers(parentHandle, &outBuffer, crop, frameScale, true));
        *sc = std::make_unique<ScreenCapture>(outBuffer);
    }

    void expectColor(const Rect& rect, const Color& color, uint8_t tolerance = 0) {
        ASSERT_EQ(HAL_PIXEL_FORMAT_RGBA_8888, mOutBuffer->getPixelFormat());
        expectBufferColor(mOutBuffer, mPixels, rect, color, tolerance);
    }

    void expectBorder(const Rect& rect, const Color& color, uint8_t tolerance = 0) {
        ASSERT_EQ(HAL_PIXEL_FORMAT_RGBA_8888, mOutBuffer->getPixelFormat());
        const bool leftBorder = rect.left > 0;
        const bool topBorder = rect.top > 0;
        const bool rightBorder = rect.right < int32_t(mOutBuffer->getWidth());
        const bool bottomBorder = rect.bottom < int32_t(mOutBuffer->getHeight());

        if (topBorder) {
            Rect top(rect.left, rect.top - 1, rect.right, rect.top);
            if (leftBorder) {
                top.left -= 1;
            }
            if (rightBorder) {
                top.right += 1;
            }
            expectColor(top, color, tolerance);
        }
        if (leftBorder) {
            Rect left(rect.left - 1, rect.top, rect.left, rect.bottom);
            expectColor(left, color, tolerance);
        }
        if (rightBorder) {
            Rect right(rect.right, rect.top, rect.right + 1, rect.bottom);
            expectColor(right, color, tolerance);
        }
        if (bottomBorder) {
            Rect bottom(rect.left, rect.bottom, rect.right, rect.bottom + 1);
            if (leftBorder) {
                bottom.left -= 1;
            }
            if (rightBorder) {
                bottom.right += 1;
            }
            expectColor(bottom, color, tolerance);
        }
    }

    void expectQuadrant(const Rect& rect, const Color& topLeft, const Color& topRight,
                        const Color& bottomLeft, const Color& bottomRight, bool filtered = false,
                        uint8_t tolerance = 0) {
        ASSERT_TRUE((rect.right - rect.left) % 2 == 0 && (rect.bottom - rect.top) % 2 == 0);

        const int32_t centerX = rect.left + (rect.right - rect.left) / 2;
        const int32_t centerY = rect.top + (rect.bottom - rect.top) / 2;
        // avoid checking borders due to unspecified filtering behavior
        const int32_t offsetX = filtered ? 2 : 0;
        const int32_t offsetY = filtered ? 2 : 0;
        expectColor(Rect(rect.left, rect.top, centerX - offsetX, centerY - offsetY), topLeft,
                    tolerance);
        expectColor(Rect(centerX + offsetX, rect.top, rect.right, centerY - offsetY), topRight,
                    tolerance);
        expectColor(Rect(rect.left, centerY + offsetY, centerX - offsetX, rect.bottom), bottomLeft,
                    tolerance);
        expectColor(Rect(centerX + offsetX, centerY + offsetY, rect.right, rect.bottom),
                    bottomRight, tolerance);
    }

    void checkPixel(uint32_t x, uint32_t y, uint8_t r, uint8_t g, uint8_t b) {
        ASSERT_EQ(HAL_PIXEL_FORMAT_RGBA_8888, mOutBuffer->getPixelFormat());
        const uint8_t* pixel = mPixels + (4 * (y * mOutBuffer->getStride() + x));
        if (r != pixel[0] || g != pixel[1] || b != pixel[2]) {
            String8 err(String8::format("pixel @ (%3d, %3d): "
                                        "expected [%3d, %3d, %3d], got [%3d, %3d, %3d]",
                                        x, y, r, g, b, pixel[0], pixel[1], pixel[2]));
            EXPECT_EQ(String8(), err) << err.string();
        }
    }

    void expectFGColor(uint32_t x, uint32_t y) { checkPixel(x, y, 195, 63, 63); }

    void expectBGColor(uint32_t x, uint32_t y) { checkPixel(x, y, 63, 63, 195); }

    void expectChildColor(uint32_t x, uint32_t y) { checkPixel(x, y, 200, 200, 200); }

    ScreenCapture(const sp<GraphicBuffer>& outBuffer) : mOutBuffer(outBuffer) {
        mOutBuffer->lock(GRALLOC_USAGE_SW_READ_OFTEN, reinterpret_cast<void**>(&mPixels));
    }

    ~ScreenCapture() { mOutBuffer->unlock(); }

private:
    sp<GraphicBuffer> mOutBuffer;
    uint8_t* mPixels = nullptr;
};

class LayerTransactionTest : public ::testing::Test {
protected:
    void SetUp() override {
        mClient = new SurfaceComposerClient;
        ASSERT_EQ(NO_ERROR, mClient->initCheck()) << "failed to create SurfaceComposerClient";

        ASSERT_NO_FATAL_FAILURE(SetUpDisplay());
    }

    sp<SurfaceControl> createLayer(const char* name, uint32_t width, uint32_t height,
                                   uint32_t flags = 0) {
        auto layer =
                mClient->createSurface(String8(name), width, height, PIXEL_FORMAT_RGBA_8888, flags);
        EXPECT_NE(nullptr, layer.get()) << "failed to create SurfaceControl";

        status_t error = Transaction()
                                 .setLayerStack(layer, mDisplayLayerStack)
                                 .setLayer(layer, mLayerZBase)
                                 .apply();
        if (error != NO_ERROR) {
            ADD_FAILURE() << "failed to initialize SurfaceControl";
            layer.clear();
        }

        return layer;
    }

    ANativeWindow_Buffer getLayerBuffer(const sp<SurfaceControl>& layer) {
        // wait for previous transactions (such as setSize) to complete
        Transaction().apply(true);

        ANativeWindow_Buffer buffer = {};
        EXPECT_EQ(NO_ERROR, layer->getSurface()->lock(&buffer, nullptr));

        return buffer;
    }

    void postLayerBuffer(const sp<SurfaceControl>& layer) {
        ASSERT_EQ(NO_ERROR, layer->getSurface()->unlockAndPost());

        // wait for the newly posted buffer to be latched
        waitForLayerBuffers();
    }

    void fillLayerColor(const sp<SurfaceControl>& layer, const Color& color) {
        ANativeWindow_Buffer buffer;
        ASSERT_NO_FATAL_FAILURE(buffer = getLayerBuffer(layer));
        fillBufferColor(buffer, Rect(0, 0, buffer.width, buffer.height), color);
        postLayerBuffer(layer);
    }

    void fillLayerQuadrant(const sp<SurfaceControl>& layer, const Color& topLeft,
                           const Color& topRight, const Color& bottomLeft,
                           const Color& bottomRight) {
        ANativeWindow_Buffer buffer;
        ASSERT_NO_FATAL_FAILURE(buffer = getLayerBuffer(layer));
        ASSERT_TRUE(buffer.width % 2 == 0 && buffer.height % 2 == 0);

        const int32_t halfW = buffer.width / 2;
        const int32_t halfH = buffer.height / 2;
        fillBufferColor(buffer, Rect(0, 0, halfW, halfH), topLeft);
        fillBufferColor(buffer, Rect(halfW, 0, buffer.width, halfH), topRight);
        fillBufferColor(buffer, Rect(0, halfH, halfW, buffer.height), bottomLeft);
        fillBufferColor(buffer, Rect(halfW, halfH, buffer.width, buffer.height), bottomRight);

        postLayerBuffer(layer);
    }

    sp<ScreenCapture> screenshot() {
        sp<ScreenCapture> screenshot;
        ScreenCapture::captureScreen(&screenshot, mLayerZBase);
        return screenshot;
    }

    sp<SurfaceComposerClient> mClient;

    sp<IBinder> mDisplay;
    uint32_t mDisplayWidth;
    uint32_t mDisplayHeight;
    uint32_t mDisplayLayerStack;

    // leave room for ~256 layers
    const int32_t mLayerZBase = std::numeric_limits<int32_t>::max() - 256;

private:
    void SetUpDisplay() {
        mDisplay = mClient->getBuiltInDisplay(ISurfaceComposer::eDisplayIdMain);
        ASSERT_NE(nullptr, mDisplay.get()) << "failed to get built-in display";

        // get display width/height
        DisplayInfo info;
        SurfaceComposerClient::getDisplayInfo(mDisplay, &info);
        mDisplayWidth = info.w;
        mDisplayHeight = info.h;

        // After a new buffer is queued, SurfaceFlinger is notified and will
        // latch the new buffer on next vsync.  Let's heuristically wait for 3
        // vsyncs.
        mBufferPostDelay = int32_t(1e6 / info.fps) * 3;

        mDisplayLayerStack = 0;
        // set layer stack (b/68888219)
        Transaction t;
        t.setDisplayLayerStack(mDisplay, mDisplayLayerStack);
        t.apply();
    }

    void waitForLayerBuffers() { usleep(mBufferPostDelay); }

    int32_t mBufferPostDelay;
};

TEST_F(LayerTransactionTest, SetPositionBasic) {
    sp<SurfaceControl> layer;
    ASSERT_NO_FATAL_FAILURE(layer = createLayer("test", 32, 32));
    ASSERT_NO_FATAL_FAILURE(fillLayerColor(layer, Color::RED));

    {
        SCOPED_TRACE("default position");
        auto shot = screenshot();
        shot->expectColor(Rect(0, 0, 32, 32), Color::RED);
        shot->expectBorder(Rect(0, 0, 32, 32), Color::BLACK);
    }

    Transaction().setPosition(layer, 5, 10).apply();
    {
        SCOPED_TRACE("new position");
        auto shot = screenshot();
        shot->expectColor(Rect(5, 10, 37, 42), Color::RED);
        shot->expectBorder(Rect(5, 10, 37, 42), Color::BLACK);
    }
}

TEST_F(LayerTransactionTest, SetPositionRounding) {
    sp<SurfaceControl> layer;
    ASSERT_NO_FATAL_FAILURE(layer = createLayer("test", 32, 32));
    ASSERT_NO_FATAL_FAILURE(fillLayerColor(layer, Color::RED));

    // GLES requires only 4 bits of subpixel precision during rasterization
    // XXX GLES composition does not match HWC composition due to precision
    // loss (b/69315223)
    const float epsilon = 1.0f / 16.0f;
    Transaction().setPosition(layer, 0.5f - epsilon, 0.5f - epsilon).apply();
    {
        SCOPED_TRACE("rounding down");
        screenshot()->expectColor(Rect(0, 0, 32, 32), Color::RED);
    }

    Transaction().setPosition(layer, 0.5f + epsilon, 0.5f + epsilon).apply();
    {
        SCOPED_TRACE("rounding up");
        screenshot()->expectColor(Rect(1, 1, 33, 33), Color::RED);
    }
}

TEST_F(LayerTransactionTest, SetPositionOutOfBounds) {
    sp<SurfaceControl> layer;
    ASSERT_NO_FATAL_FAILURE(layer = createLayer("test", 32, 32));
    ASSERT_NO_FATAL_FAILURE(fillLayerColor(layer, Color::RED));

    Transaction().setPosition(layer, -32, -32).apply();
    {
        SCOPED_TRACE("negative coordinates");
        screenshot()->expectColor(Rect(0, 0, mDisplayWidth, mDisplayHeight), Color::BLACK);
    }

    Transaction().setPosition(layer, mDisplayWidth, mDisplayHeight).apply();
    {
        SCOPED_TRACE("positive coordinates");
        screenshot()->expectColor(Rect(0, 0, mDisplayWidth, mDisplayHeight), Color::BLACK);
    }
}

TEST_F(LayerTransactionTest, SetPositionPartiallyOutOfBounds) {
    sp<SurfaceControl> layer;
    ASSERT_NO_FATAL_FAILURE(layer = createLayer("test", 32, 32));
    ASSERT_NO_FATAL_FAILURE(fillLayerColor(layer, Color::RED));

    // partially out of bounds
    Transaction().setPosition(layer, -30, -30).apply();
    {
        SCOPED_TRACE("negative coordinates");
        screenshot()->expectColor(Rect(0, 0, 2, 2), Color::RED);
    }

    Transaction().setPosition(layer, mDisplayWidth - 2, mDisplayHeight - 2).apply();
    {
        SCOPED_TRACE("positive coordinates");
        screenshot()->expectColor(Rect(mDisplayWidth - 2, mDisplayHeight - 2, mDisplayWidth,
                                       mDisplayHeight),
                                  Color::RED);
    }
}

TEST_F(LayerTransactionTest, SetPositionWithResize) {
    sp<SurfaceControl> layer;
    ASSERT_NO_FATAL_FAILURE(layer = createLayer("test", 32, 32));
    ASSERT_NO_FATAL_FAILURE(fillLayerColor(layer, Color::RED));

    // setPosition is applied immediately by default, with or without resize
    // pending
    Transaction().setPosition(layer, 5, 10).setSize(layer, 64, 64).apply();
    {
        SCOPED_TRACE("resize pending");
        auto shot = screenshot();
        shot->expectColor(Rect(5, 10, 37, 42), Color::RED);
        shot->expectBorder(Rect(5, 10, 37, 42), Color::BLACK);
    }

    ASSERT_NO_FATAL_FAILURE(fillLayerColor(layer, Color::RED));
    {
        SCOPED_TRACE("resize applied");
        screenshot()->expectColor(Rect(5, 10, 69, 74), Color::RED);
    }
}

TEST_F(LayerTransactionTest, SetPositionWithNextResize) {
    sp<SurfaceControl> layer;
    ASSERT_NO_FATAL_FAILURE(layer = createLayer("test", 32, 32));
    ASSERT_NO_FATAL_FAILURE(fillLayerColor(layer, Color::RED));

    // request setPosition to be applied with the next resize
    Transaction().setPosition(layer, 5, 10).setGeometryAppliesWithResize(layer).apply();
    {
        SCOPED_TRACE("new position pending");
        screenshot()->expectColor(Rect(0, 0, 32, 32), Color::RED);
    }

    Transaction().setPosition(layer, 15, 20).apply();
    {
        SCOPED_TRACE("pending new position modified");
        screenshot()->expectColor(Rect(0, 0, 32, 32), Color::RED);
    }

    Transaction().setSize(layer, 64, 64).apply();
    {
        SCOPED_TRACE("resize pending");
        screenshot()->expectColor(Rect(0, 0, 32, 32), Color::RED);
    }

    // finally resize and latch the buffer
    ASSERT_NO_FATAL_FAILURE(fillLayerColor(layer, Color::RED));
    {
        SCOPED_TRACE("new position applied");
        screenshot()->expectColor(Rect(15, 20, 79, 84), Color::RED);
    }
}

TEST_F(LayerTransactionTest, SetPositionWithNextResizeScaleToWindow) {
    sp<SurfaceControl> layer;
    ASSERT_NO_FATAL_FAILURE(layer = createLayer("test", 32, 32));
    ASSERT_NO_FATAL_FAILURE(fillLayerColor(layer, Color::RED));

    // setPosition is not immediate even with SCALE_TO_WINDOW override
    Transaction()
            .setPosition(layer, 5, 10)
            .setSize(layer, 64, 64)
            .setOverrideScalingMode(layer, NATIVE_WINDOW_SCALING_MODE_SCALE_TO_WINDOW)
            .setGeometryAppliesWithResize(layer)
            .apply();
    {
        SCOPED_TRACE("new position pending");
        screenshot()->expectColor(Rect(0, 0, 64, 64), Color::RED);
    }

    ASSERT_NO_FATAL_FAILURE(fillLayerColor(layer, Color::RED));
    {
        SCOPED_TRACE("new position applied");
        screenshot()->expectColor(Rect(5, 10, 69, 74), Color::RED);
    }
}

TEST_F(LayerTransactionTest, SetSizeBasic) {
    sp<SurfaceControl> layer;
    ASSERT_NO_FATAL_FAILURE(layer = createLayer("test", 32, 32));
    ASSERT_NO_FATAL_FAILURE(fillLayerColor(layer, Color::RED));

    Transaction().setSize(layer, 64, 64).apply();
    {
        SCOPED_TRACE("resize pending");
        auto shot = screenshot();
        shot->expectColor(Rect(0, 0, 32, 32), Color::RED);
        shot->expectBorder(Rect(0, 0, 32, 32), Color::BLACK);
    }

    ASSERT_NO_FATAL_FAILURE(fillLayerColor(layer, Color::RED));
    {
        SCOPED_TRACE("resize applied");
        auto shot = screenshot();
        shot->expectColor(Rect(0, 0, 64, 64), Color::RED);
        shot->expectBorder(Rect(0, 0, 64, 64), Color::BLACK);
    }
}

TEST_F(LayerTransactionTest, SetSizeInvalid) {
    // cannot test robustness against invalid sizes (zero or really huge)
}

TEST_F(LayerTransactionTest, SetSizeWithScaleToWindow) {
    sp<SurfaceControl> layer;
    ASSERT_NO_FATAL_FAILURE(layer = createLayer("test", 32, 32));
    ASSERT_NO_FATAL_FAILURE(fillLayerColor(layer, Color::RED));

    // setSize is immediate with SCALE_TO_WINDOW, unlike setPosition
    Transaction()
            .setSize(layer, 64, 64)
            .setOverrideScalingMode(layer, NATIVE_WINDOW_SCALING_MODE_SCALE_TO_WINDOW)
            .apply();
    screenshot()->expectColor(Rect(0, 0, 64, 64), Color::RED);
}

TEST_F(LayerTransactionTest, SetZBasic) {
    sp<SurfaceControl> layerR;
    sp<SurfaceControl> layerG;
    ASSERT_NO_FATAL_FAILURE(layerR = createLayer("test R", 32, 32));
    ASSERT_NO_FATAL_FAILURE(fillLayerColor(layerR, Color::RED));
    ASSERT_NO_FATAL_FAILURE(layerG = createLayer("test G", 32, 32));
    ASSERT_NO_FATAL_FAILURE(fillLayerColor(layerG, Color::GREEN));

    Transaction().setLayer(layerR, mLayerZBase + 1).apply();
    {
        SCOPED_TRACE("layerR");
        screenshot()->expectColor(Rect(0, 0, 32, 32), Color::RED);
    }

    Transaction().setLayer(layerG, mLayerZBase + 2).apply();
    {
        SCOPED_TRACE("layerG");
        screenshot()->expectColor(Rect(0, 0, 32, 32), Color::GREEN);
    }
}

TEST_F(LayerTransactionTest, SetZNegative) {
    sp<SurfaceControl> layerR;
    sp<SurfaceControl> layerG;
    ASSERT_NO_FATAL_FAILURE(layerR = createLayer("test R", 32, 32));
    ASSERT_NO_FATAL_FAILURE(fillLayerColor(layerR, Color::RED));
    ASSERT_NO_FATAL_FAILURE(layerG = createLayer("test G", 32, 32));
    ASSERT_NO_FATAL_FAILURE(fillLayerColor(layerG, Color::GREEN));

    Transaction().setLayer(layerR, -1).setLayer(layerG, -2).apply();
    {
        SCOPED_TRACE("layerR");
        sp<ScreenCapture> screenshot;
        ScreenCapture::captureScreen(&screenshot, -2, -1);
        screenshot->expectColor(Rect(0, 0, 32, 32), Color::RED);
    }

    Transaction().setLayer(layerR, -3).apply();
    {
        SCOPED_TRACE("layerG");
        sp<ScreenCapture> screenshot;
        ScreenCapture::captureScreen(&screenshot, -3, -1);
        screenshot->expectColor(Rect(0, 0, 32, 32), Color::GREEN);
    }
}

TEST_F(LayerTransactionTest, SetRelativeZBasic) {
    sp<SurfaceControl> layerR;
    sp<SurfaceControl> layerG;
    ASSERT_NO_FATAL_FAILURE(layerR = createLayer("test R", 32, 32));
    ASSERT_NO_FATAL_FAILURE(fillLayerColor(layerR, Color::RED));
    ASSERT_NO_FATAL_FAILURE(layerG = createLayer("test G", 32, 32));
    ASSERT_NO_FATAL_FAILURE(fillLayerColor(layerG, Color::GREEN));

    Transaction()
            .setPosition(layerG, 16, 16)
            .setRelativeLayer(layerG, layerR->getHandle(), 1)
            .apply();
    {
        SCOPED_TRACE("layerG above");
        auto shot = screenshot();
        shot->expectColor(Rect(0, 0, 16, 16), Color::RED);
        shot->expectColor(Rect(16, 16, 48, 48), Color::GREEN);
    }

    Transaction().setRelativeLayer(layerG, layerR->getHandle(), -1).apply();
    {
        SCOPED_TRACE("layerG below");
        auto shot = screenshot();
        shot->expectColor(Rect(0, 0, 32, 32), Color::RED);
        shot->expectColor(Rect(32, 32, 48, 48), Color::GREEN);
    }
}

TEST_F(LayerTransactionTest, SetRelativeZNegative) {
    sp<SurfaceControl> layerR;
    sp<SurfaceControl> layerG;
    sp<SurfaceControl> layerB;
    ASSERT_NO_FATAL_FAILURE(layerR = createLayer("test R", 32, 32));
    ASSERT_NO_FATAL_FAILURE(fillLayerColor(layerR, Color::RED));
    ASSERT_NO_FATAL_FAILURE(layerG = createLayer("test G", 32, 32));
    ASSERT_NO_FATAL_FAILURE(fillLayerColor(layerG, Color::GREEN));
    ASSERT_NO_FATAL_FAILURE(layerB = createLayer("test B", 32, 32));
    ASSERT_NO_FATAL_FAILURE(fillLayerColor(layerB, Color::BLUE));

    // layerR = mLayerZBase, layerG = layerR - 1, layerB = -2
    Transaction().setRelativeLayer(layerG, layerR->getHandle(), -1).setLayer(layerB, -2).apply();

    sp<ScreenCapture> screenshot;
    // only layerB is in this range
    ScreenCapture::captureScreen(&screenshot, -2, -1);
    screenshot->expectColor(Rect(0, 0, 32, 32), Color::BLUE);
}

TEST_F(LayerTransactionTest, SetRelativeZGroup) {
    sp<SurfaceControl> layerR;
    sp<SurfaceControl> layerG;
    sp<SurfaceControl> layerB;
    ASSERT_NO_FATAL_FAILURE(layerR = createLayer("test R", 32, 32));
    ASSERT_NO_FATAL_FAILURE(fillLayerColor(layerR, Color::RED));
    ASSERT_NO_FATAL_FAILURE(layerG = createLayer("test G", 32, 32));
    ASSERT_NO_FATAL_FAILURE(fillLayerColor(layerG, Color::GREEN));
    ASSERT_NO_FATAL_FAILURE(layerB = createLayer("test B", 32, 32));
    ASSERT_NO_FATAL_FAILURE(fillLayerColor(layerB, Color::BLUE));

    // layerR = 0, layerG = layerR + 3, layerB = 2
    Transaction()
            .setPosition(layerG, 8, 8)
            .setRelativeLayer(layerG, layerR->getHandle(), 3)
            .setPosition(layerB, 16, 16)
            .setLayer(layerB, mLayerZBase + 2)
            .apply();
    {
        SCOPED_TRACE("(layerR < layerG) < layerB");
        auto shot = screenshot();
        shot->expectColor(Rect(0, 0, 8, 8), Color::RED);
        shot->expectColor(Rect(8, 8, 16, 16), Color::GREEN);
        shot->expectColor(Rect(16, 16, 48, 48), Color::BLUE);
    }

    // layerR = 4, layerG = layerR + 3, layerB = 2
    Transaction().setLayer(layerR, mLayerZBase + 4).apply();
    {
        SCOPED_TRACE("layerB < (layerR < layerG)");
        auto shot = screenshot();
        shot->expectColor(Rect(0, 0, 8, 8), Color::RED);
        shot->expectColor(Rect(8, 8, 40, 40), Color::GREEN);
        shot->expectColor(Rect(40, 40, 48, 48), Color::BLUE);
    }

    // layerR = 4, layerG = layerR - 3, layerB = 2
    Transaction().setRelativeLayer(layerG, layerR->getHandle(), -3).apply();
    {
        SCOPED_TRACE("layerB < (layerG < layerR)");
        auto shot = screenshot();
        shot->expectColor(Rect(0, 0, 32, 32), Color::RED);
        shot->expectColor(Rect(32, 32, 40, 40), Color::GREEN);
        shot->expectColor(Rect(40, 40, 48, 48), Color::BLUE);
    }

    // restore to absolute z
    // layerR = 4, layerG = 0, layerB = 2
    Transaction().setLayer(layerG, mLayerZBase).apply();
    {
        SCOPED_TRACE("layerG < layerB < layerR");
        auto shot = screenshot();
        shot->expectColor(Rect(0, 0, 32, 32), Color::RED);
        shot->expectColor(Rect(32, 32, 48, 48), Color::BLUE);
    }

    // layerR should not affect layerG anymore
    // layerR = 1, layerG = 0, layerB = 2
    Transaction().setLayer(layerR, mLayerZBase + 1).apply();
    {
        SCOPED_TRACE("layerG < layerR < layerB");
        auto shot = screenshot();
        shot->expectColor(Rect(0, 0, 16, 16), Color::RED);
        shot->expectColor(Rect(16, 16, 48, 48), Color::BLUE);
    }
}

TEST_F(LayerTransactionTest, SetRelativeZBug64572777) {
    sp<SurfaceControl> layerR;
    sp<SurfaceControl> layerG;

    ASSERT_NO_FATAL_FAILURE(layerR = createLayer("test R", 32, 32));
    ASSERT_NO_FATAL_FAILURE(fillLayerColor(layerR, Color::RED));
    ASSERT_NO_FATAL_FAILURE(layerG = createLayer("test G", 32, 32));
    ASSERT_NO_FATAL_FAILURE(fillLayerColor(layerG, Color::GREEN));

    Transaction()
            .setPosition(layerG, 16, 16)
            .setRelativeLayer(layerG, layerR->getHandle(), 1)
            .apply();

    mClient->destroySurface(layerG->getHandle());
    // layerG should have been removed
    screenshot()->expectColor(Rect(0, 0, 32, 32), Color::RED);
}

TEST_F(LayerTransactionTest, SetFlagsHidden) {
    sp<SurfaceControl> layer;
    ASSERT_NO_FATAL_FAILURE(layer = createLayer("test", 32, 32));
    ASSERT_NO_FATAL_FAILURE(fillLayerColor(layer, Color::RED));

    Transaction().setFlags(layer, layer_state_t::eLayerHidden, layer_state_t::eLayerHidden).apply();
    {
        SCOPED_TRACE("layer hidden");
        screenshot()->expectColor(Rect(0, 0, mDisplayWidth, mDisplayHeight), Color::BLACK);
    }

    Transaction().setFlags(layer, 0, layer_state_t::eLayerHidden).apply();
    {
        SCOPED_TRACE("layer shown");
        screenshot()->expectColor(Rect(0, 0, 32, 32), Color::RED);
    }
}

TEST_F(LayerTransactionTest, SetFlagsOpaque) {
    const Color translucentRed = {100, 0, 0, 100};
    sp<SurfaceControl> layerR;
    sp<SurfaceControl> layerG;
    ASSERT_NO_FATAL_FAILURE(layerR = createLayer("test R", 32, 32));
    ASSERT_NO_FATAL_FAILURE(fillLayerColor(layerR, translucentRed));
    ASSERT_NO_FATAL_FAILURE(layerG = createLayer("test G", 32, 32));
    ASSERT_NO_FATAL_FAILURE(fillLayerColor(layerG, Color::GREEN));

    Transaction()
            .setLayer(layerR, mLayerZBase + 1)
            .setFlags(layerR, layer_state_t::eLayerOpaque, layer_state_t::eLayerOpaque)
            .apply();
    {
        SCOPED_TRACE("layerR opaque");
        screenshot()->expectColor(Rect(0, 0, 32, 32), {100, 0, 0, 255});
    }

    Transaction().setFlags(layerR, 0, layer_state_t::eLayerOpaque).apply();
    {
        SCOPED_TRACE("layerR translucent");
        const uint8_t g = uint8_t(255 - translucentRed.a);
        screenshot()->expectColor(Rect(0, 0, 32, 32), {100, g, 0, 255});
    }
}

TEST_F(LayerTransactionTest, SetFlagsSecure) {
    sp<SurfaceControl> layer;
    ASSERT_NO_FATAL_FAILURE(layer = createLayer("test", 32, 32));
    ASSERT_NO_FATAL_FAILURE(fillLayerColor(layer, Color::RED));

    sp<ISurfaceComposer> composer = ComposerService::getComposerService();
    sp<GraphicBuffer> outBuffer;
    Transaction()
            .setFlags(layer, layer_state_t::eLayerSecure, layer_state_t::eLayerSecure)
            .apply(true);
    ASSERT_EQ(PERMISSION_DENIED,
              composer->captureScreen(mDisplay, &outBuffer, Rect(), 0, 0, mLayerZBase, mLayerZBase,
                                      false));

    Transaction().setFlags(layer, 0, layer_state_t::eLayerSecure).apply(true);
    ASSERT_EQ(NO_ERROR,
              composer->captureScreen(mDisplay, &outBuffer, Rect(), 0, 0, mLayerZBase, mLayerZBase,
                                      false));
}

TEST_F(LayerTransactionTest, SetTransparentRegionHintBasic) {
    const Rect top(0, 0, 32, 16);
    const Rect bottom(0, 16, 32, 32);
    sp<SurfaceControl> layer;
    ASSERT_NO_FATAL_FAILURE(layer = createLayer("test", 32, 32));

    ANativeWindow_Buffer buffer;
    ASSERT_NO_FATAL_FAILURE(buffer = getLayerBuffer(layer));
    ASSERT_NO_FATAL_FAILURE(fillBufferColor(buffer, top, Color::TRANSPARENT));
    ASSERT_NO_FATAL_FAILURE(fillBufferColor(buffer, bottom, Color::RED));
    // setTransparentRegionHint always applies to the following buffer
    Transaction().setTransparentRegionHint(layer, Region(top)).apply();
    ASSERT_NO_FATAL_FAILURE(postLayerBuffer(layer));
    {
        SCOPED_TRACE("top transparent");
        auto shot = screenshot();
        shot->expectColor(top, Color::BLACK);
        shot->expectColor(bottom, Color::RED);
    }

    Transaction().setTransparentRegionHint(layer, Region(bottom)).apply();
    {
        SCOPED_TRACE("transparent region hint pending");
        auto shot = screenshot();
        shot->expectColor(top, Color::BLACK);
        shot->expectColor(bottom, Color::RED);
    }

    ASSERT_NO_FATAL_FAILURE(buffer = getLayerBuffer(layer));
    ASSERT_NO_FATAL_FAILURE(fillBufferColor(buffer, top, Color::RED));
    ASSERT_NO_FATAL_FAILURE(fillBufferColor(buffer, bottom, Color::TRANSPARENT));
    ASSERT_NO_FATAL_FAILURE(postLayerBuffer(layer));
    {
        SCOPED_TRACE("bottom transparent");
        auto shot = screenshot();
        shot->expectColor(top, Color::RED);
        shot->expectColor(bottom, Color::BLACK);
    }
}

TEST_F(LayerTransactionTest, SetTransparentRegionHintOutOfBounds) {
    sp<SurfaceControl> layerTransparent;
    sp<SurfaceControl> layerR;
    ASSERT_NO_FATAL_FAILURE(layerTransparent = createLayer("test transparent", 32, 32));
    ASSERT_NO_FATAL_FAILURE(layerR = createLayer("test R", 32, 32));

    // check that transparent region hint is bound by the layer size
    Transaction()
            .setTransparentRegionHint(layerTransparent,
                                      Region(Rect(0, 0, mDisplayWidth, mDisplayHeight)))
            .setPosition(layerR, 16, 16)
            .setLayer(layerR, mLayerZBase + 1)
            .apply();
    ASSERT_NO_FATAL_FAILURE(fillLayerColor(layerTransparent, Color::TRANSPARENT));
    ASSERT_NO_FATAL_FAILURE(fillLayerColor(layerR, Color::RED));
    screenshot()->expectColor(Rect(16, 16, 48, 48), Color::RED);
}

TEST_F(LayerTransactionTest, SetAlphaBasic) {
    sp<SurfaceControl> layer1;
    sp<SurfaceControl> layer2;
    ASSERT_NO_FATAL_FAILURE(layer1 = createLayer("test 1", 32, 32));
    ASSERT_NO_FATAL_FAILURE(layer2 = createLayer("test 2", 32, 32));
    ASSERT_NO_FATAL_FAILURE(fillLayerColor(layer1, {64, 0, 0, 255}));
    ASSERT_NO_FATAL_FAILURE(fillLayerColor(layer2, {0, 64, 0, 255}));

    Transaction()
            .setAlpha(layer1, 0.25f)
            .setAlpha(layer2, 0.75f)
            .setPosition(layer2, 16, 0)
            .setLayer(layer2, mLayerZBase + 1)
            .apply();
    {
        auto shot = screenshot();
        uint8_t r = 16; // 64 * 0.25f
        uint8_t g = 48; // 64 * 0.75f
        shot->expectColor(Rect(0, 0, 16, 32), {r, 0, 0, 255});
        shot->expectColor(Rect(32, 0, 48, 32), {0, g, 0, 255});

        r /= 4; // r * (1.0f - 0.75f)
        shot->expectColor(Rect(16, 0, 32, 32), {r, g, 0, 255});
    }
}

TEST_F(LayerTransactionTest, SetAlphaClamped) {
    const Color color = {64, 0, 0, 255};
    sp<SurfaceControl> layer;
    ASSERT_NO_FATAL_FAILURE(layer = createLayer("test", 32, 32));
    ASSERT_NO_FATAL_FAILURE(fillLayerColor(layer, color));

    Transaction().setAlpha(layer, 2.0f).apply();
    {
        SCOPED_TRACE("clamped to 1.0f");
        screenshot()->expectColor(Rect(0, 0, 32, 32), color);
    }

    Transaction().setAlpha(layer, -1.0f).apply();
    {
        SCOPED_TRACE("clamped to 0.0f");
        screenshot()->expectColor(Rect(0, 0, 32, 32), Color::BLACK);
    }
}

TEST_F(LayerTransactionTest, SetColorBasic) {
    sp<SurfaceControl> bufferLayer;
    sp<SurfaceControl> colorLayer;
    ASSERT_NO_FATAL_FAILURE(bufferLayer = createLayer("test bg", 32, 32));
    ASSERT_NO_FATAL_FAILURE(fillLayerColor(bufferLayer, Color::RED));
    ASSERT_NO_FATAL_FAILURE(
            colorLayer = createLayer("test", 32, 32, ISurfaceComposerClient::eFXSurfaceColor));

    Transaction().setLayer(colorLayer, mLayerZBase + 1).apply();
    {
        SCOPED_TRACE("default color");
        screenshot()->expectColor(Rect(0, 0, 32, 32), Color::BLACK);
    }

    const half3 color(15.0f / 255.0f, 51.0f / 255.0f, 85.0f / 255.0f);
    const Color expected = {15, 51, 85, 255};
    // this is handwavy, but the precison loss scaled by 255 (8-bit per
    // channel) should be less than one
    const uint8_t tolerance = 1;
    Transaction().setColor(colorLayer, color).apply();
    {
        SCOPED_TRACE("new color");
        screenshot()->expectColor(Rect(0, 0, 32, 32), expected, tolerance);
    }
}

TEST_F(LayerTransactionTest, SetColorClamped) {
    sp<SurfaceControl> colorLayer;
    ASSERT_NO_FATAL_FAILURE(
            colorLayer = createLayer("test", 32, 32, ISurfaceComposerClient::eFXSurfaceColor));

    Transaction().setColor(colorLayer, half3(2.0f, -1.0f, 0.0f)).apply();
    screenshot()->expectColor(Rect(0, 0, 32, 32), Color::RED);
}

TEST_F(LayerTransactionTest, SetColorWithAlpha) {
    sp<SurfaceControl> bufferLayer;
    sp<SurfaceControl> colorLayer;
    ASSERT_NO_FATAL_FAILURE(bufferLayer = createLayer("test bg", 32, 32));
    ASSERT_NO_FATAL_FAILURE(fillLayerColor(bufferLayer, Color::RED));
    ASSERT_NO_FATAL_FAILURE(
            colorLayer = createLayer("test", 32, 32, ISurfaceComposerClient::eFXSurfaceColor));

    const half3 color(15.0f / 255.0f, 51.0f / 255.0f, 85.0f / 255.0f);
    const float alpha = 0.25f;
    const ubyte3 expected((vec3(color) * alpha + vec3(1.0f, 0.0f, 0.0f) * (1.0f - alpha)) * 255.0f);
    // this is handwavy, but the precison loss scaled by 255 (8-bit per
    // channel) should be less than one
    const uint8_t tolerance = 1;
    Transaction()
            .setColor(colorLayer, color)
            .setAlpha(colorLayer, alpha)
            .setLayer(colorLayer, mLayerZBase + 1)
            .apply();
    screenshot()->expectColor(Rect(0, 0, 32, 32), {expected.r, expected.g, expected.b, 255},
                              tolerance);
}

TEST_F(LayerTransactionTest, SetColorWithParentAlpha_Bug74220420) {
    sp<SurfaceControl> bufferLayer;
    sp<SurfaceControl> parentLayer;
    sp<SurfaceControl> colorLayer;
    ASSERT_NO_FATAL_FAILURE(bufferLayer = createLayer("test bg", 32, 32));
    ASSERT_NO_FATAL_FAILURE(parentLayer = createLayer("parentWithAlpha", 32, 32));
    ASSERT_NO_FATAL_FAILURE(fillLayerColor(bufferLayer, Color::RED));
    ASSERT_NO_FATAL_FAILURE(colorLayer = createLayer(
            "childWithColor", 32, 32, ISurfaceComposerClient::eFXSurfaceColor));

    const half3 color(15.0f / 255.0f, 51.0f / 255.0f, 85.0f / 255.0f);
    const float alpha = 0.25f;
    const ubyte3 expected((vec3(color) * alpha + vec3(1.0f, 0.0f, 0.0f) * (1.0f - alpha)) * 255.0f);
    // this is handwavy, but the precision loss scaled by 255 (8-bit per
    // channel) should be less than one
    const uint8_t tolerance = 1;
    Transaction()
            .reparent(colorLayer, parentLayer->getHandle())
            .setColor(colorLayer, color)
            .setAlpha(parentLayer, alpha)
            .setLayer(parentLayer, mLayerZBase + 1)
            .apply();
    screenshot()->expectColor(Rect(0, 0, 32, 32), {expected.r, expected.g, expected.b, 255},
                              tolerance);
}

TEST_F(LayerTransactionTest, SetColorWithBuffer) {
    sp<SurfaceControl> bufferLayer;
    ASSERT_NO_FATAL_FAILURE(bufferLayer = createLayer("test", 32, 32));
    ASSERT_NO_FATAL_FAILURE(fillLayerColor(bufferLayer, Color::RED));

    // color is ignored
    Transaction().setColor(bufferLayer, half3(0.0f, 1.0f, 0.0f)).apply();
    screenshot()->expectColor(Rect(0, 0, 32, 32), Color::RED);
}

TEST_F(LayerTransactionTest, SetLayerStackBasic) {
    sp<SurfaceControl> layer;
    ASSERT_NO_FATAL_FAILURE(layer = createLayer("test", 32, 32));
    ASSERT_NO_FATAL_FAILURE(fillLayerColor(layer, Color::RED));

    Transaction().setLayerStack(layer, mDisplayLayerStack + 1).apply();
    {
        SCOPED_TRACE("non-existing layer stack");
        screenshot()->expectColor(Rect(0, 0, mDisplayWidth, mDisplayHeight), Color::BLACK);
    }

    Transaction().setLayerStack(layer, mDisplayLayerStack).apply();
    {
        SCOPED_TRACE("original layer stack");
        screenshot()->expectColor(Rect(0, 0, 32, 32), Color::RED);
    }
}

TEST_F(LayerTransactionTest, SetMatrixBasic) {
    sp<SurfaceControl> layer;
    ASSERT_NO_FATAL_FAILURE(layer = createLayer("test", 32, 32));
    ASSERT_NO_FATAL_FAILURE(
            fillLayerQuadrant(layer, Color::RED, Color::GREEN, Color::BLUE, Color::WHITE));

    Transaction().setMatrix(layer, 1.0f, 0.0f, 0.0f, 1.0f).setPosition(layer, 0, 0).apply();
    {
        SCOPED_TRACE("IDENTITY");
        screenshot()->expectQuadrant(Rect(0, 0, 32, 32), Color::RED, Color::GREEN, Color::BLUE,
                                     Color::WHITE);
    }

    Transaction().setMatrix(layer, -1.0f, 0.0f, 0.0f, 1.0f).setPosition(layer, 32, 0).apply();
    {
        SCOPED_TRACE("FLIP_H");
        screenshot()->expectQuadrant(Rect(0, 0, 32, 32), Color::GREEN, Color::RED, Color::WHITE,
                                     Color::BLUE);
    }

    Transaction().setMatrix(layer, 1.0f, 0.0f, 0.0f, -1.0f).setPosition(layer, 0, 32).apply();
    {
        SCOPED_TRACE("FLIP_V");
        screenshot()->expectQuadrant(Rect(0, 0, 32, 32), Color::BLUE, Color::WHITE, Color::RED,
                                     Color::GREEN);
    }

    Transaction().setMatrix(layer, 0.0f, 1.0f, -1.0f, 0.0f).setPosition(layer, 32, 0).apply();
    {
        SCOPED_TRACE("ROT_90");
        screenshot()->expectQuadrant(Rect(0, 0, 32, 32), Color::BLUE, Color::RED, Color::WHITE,
                                     Color::GREEN);
    }

    Transaction().setMatrix(layer, 2.0f, 0.0f, 0.0f, 2.0f).setPosition(layer, 0, 0).apply();
    {
        SCOPED_TRACE("SCALE");
        screenshot()->expectQuadrant(Rect(0, 0, 64, 64), Color::RED, Color::GREEN, Color::BLUE,
                                     Color::WHITE, true /* filtered */);
    }
}

TEST_F(LayerTransactionTest, SetMatrixRot45) {
    sp<SurfaceControl> layer;
    ASSERT_NO_FATAL_FAILURE(layer = createLayer("test", 32, 32));
    ASSERT_NO_FATAL_FAILURE(
            fillLayerQuadrant(layer, Color::RED, Color::GREEN, Color::BLUE, Color::WHITE));

    const float rot = M_SQRT1_2; // 45 degrees
    const float trans = M_SQRT2 * 16.0f;
    Transaction().setMatrix(layer, rot, rot, -rot, rot).setPosition(layer, trans, 0).apply();

    auto shot = screenshot();
    // check a 8x8 region inside each color
    auto get8x8Rect = [](int32_t centerX, int32_t centerY) {
        const int32_t halfL = 4;
        return Rect(centerX - halfL, centerY - halfL, centerX + halfL, centerY + halfL);
    };
    const int32_t unit = int32_t(trans / 2);
    shot->expectColor(get8x8Rect(2 * unit, 1 * unit), Color::RED);
    shot->expectColor(get8x8Rect(3 * unit, 2 * unit), Color::GREEN);
    shot->expectColor(get8x8Rect(1 * unit, 2 * unit), Color::BLUE);
    shot->expectColor(get8x8Rect(2 * unit, 3 * unit), Color::WHITE);
}

TEST_F(LayerTransactionTest, SetMatrixWithResize) {
    sp<SurfaceControl> layer;
    ASSERT_NO_FATAL_FAILURE(layer = createLayer("test", 32, 32));
    ASSERT_NO_FATAL_FAILURE(fillLayerColor(layer, Color::RED));

    // setMatrix is applied after any pending resize, unlike setPosition
    Transaction().setMatrix(layer, 2.0f, 0.0f, 0.0f, 2.0f).setSize(layer, 64, 64).apply();
    {
        SCOPED_TRACE("resize pending");
        auto shot = screenshot();
        shot->expectColor(Rect(0, 0, 32, 32), Color::RED);
        shot->expectBorder(Rect(0, 0, 32, 32), Color::BLACK);
    }

    ASSERT_NO_FATAL_FAILURE(fillLayerColor(layer, Color::RED));
    {
        SCOPED_TRACE("resize applied");
        screenshot()->expectColor(Rect(0, 0, 128, 128), Color::RED);
    }
}

TEST_F(LayerTransactionTest, SetMatrixWithScaleToWindow) {
    sp<SurfaceControl> layer;
    ASSERT_NO_FATAL_FAILURE(layer = createLayer("test", 32, 32));
    ASSERT_NO_FATAL_FAILURE(fillLayerColor(layer, Color::RED));

    // setMatrix is immediate with SCALE_TO_WINDOW, unlike setPosition
    Transaction()
            .setMatrix(layer, 2.0f, 0.0f, 0.0f, 2.0f)
            .setSize(layer, 64, 64)
            .setOverrideScalingMode(layer, NATIVE_WINDOW_SCALING_MODE_SCALE_TO_WINDOW)
            .apply();
    screenshot()->expectColor(Rect(0, 0, 128, 128), Color::RED);
}

TEST_F(LayerTransactionTest, SetOverrideScalingModeBasic) {
    sp<SurfaceControl> layer;
    ASSERT_NO_FATAL_FAILURE(layer = createLayer("test", 32, 32));
    ASSERT_NO_FATAL_FAILURE(
            fillLayerQuadrant(layer, Color::RED, Color::GREEN, Color::BLUE, Color::WHITE));

    // XXX SCALE_CROP is not respected; calling setSize and
    // setOverrideScalingMode in separate transactions does not work
    // (b/69315456)
    Transaction()
            .setSize(layer, 64, 16)
            .setOverrideScalingMode(layer, NATIVE_WINDOW_SCALING_MODE_SCALE_TO_WINDOW)
            .apply();
    {
        SCOPED_TRACE("SCALE_TO_WINDOW");
        screenshot()->expectQuadrant(Rect(0, 0, 64, 16), Color::RED, Color::GREEN, Color::BLUE,
                                     Color::WHITE, true /* filtered */);
    }
}

TEST_F(LayerTransactionTest, SetCropBasic) {
    sp<SurfaceControl> layer;
    ASSERT_NO_FATAL_FAILURE(layer = createLayer("test", 32, 32));
    ASSERT_NO_FATAL_FAILURE(fillLayerColor(layer, Color::RED));
    const Rect crop(8, 8, 24, 24);

    Transaction().setCrop(layer, crop).apply();
    auto shot = screenshot();
    shot->expectColor(crop, Color::RED);
    shot->expectBorder(crop, Color::BLACK);
}

TEST_F(LayerTransactionTest, SetCropEmpty) {
    sp<SurfaceControl> layer;
    ASSERT_NO_FATAL_FAILURE(layer = createLayer("test", 32, 32));
    ASSERT_NO_FATAL_FAILURE(fillLayerColor(layer, Color::RED));

    {
        SCOPED_TRACE("empty rect");
        Transaction().setCrop(layer, Rect(8, 8, 8, 8)).apply();
        screenshot()->expectColor(Rect(0, 0, 32, 32), Color::RED);
    }

    {
        SCOPED_TRACE("negative rect");
        Transaction().setCrop(layer, Rect(8, 8, 0, 0)).apply();
        screenshot()->expectColor(Rect(0, 0, 32, 32), Color::RED);
    }
}

TEST_F(LayerTransactionTest, SetCropOutOfBounds) {
    sp<SurfaceControl> layer;
    ASSERT_NO_FATAL_FAILURE(layer = createLayer("test", 32, 32));
    ASSERT_NO_FATAL_FAILURE(fillLayerColor(layer, Color::RED));

    Transaction().setCrop(layer, Rect(-128, -64, 128, 64)).apply();
    auto shot = screenshot();
    shot->expectColor(Rect(0, 0, 32, 32), Color::RED);
    shot->expectBorder(Rect(0, 0, 32, 32), Color::BLACK);
}

TEST_F(LayerTransactionTest, SetCropWithTranslation) {
    sp<SurfaceControl> layer;
    ASSERT_NO_FATAL_FAILURE(layer = createLayer("test", 32, 32));
    ASSERT_NO_FATAL_FAILURE(fillLayerColor(layer, Color::RED));

    const Point position(32, 32);
    const Rect crop(8, 8, 24, 24);
    Transaction().setPosition(layer, position.x, position.y).setCrop(layer, crop).apply();
    auto shot = screenshot();
    shot->expectColor(crop + position, Color::RED);
    shot->expectBorder(crop + position, Color::BLACK);
}

TEST_F(LayerTransactionTest, SetCropWithScale) {
    sp<SurfaceControl> layer;
    ASSERT_NO_FATAL_FAILURE(layer = createLayer("test", 32, 32));
    ASSERT_NO_FATAL_FAILURE(fillLayerColor(layer, Color::RED));

    // crop is affected by matrix
    Transaction()
            .setMatrix(layer, 2.0f, 0.0f, 0.0f, 2.0f)
            .setCrop(layer, Rect(8, 8, 24, 24))
            .apply();
    auto shot = screenshot();
    shot->expectColor(Rect(16, 16, 48, 48), Color::RED);
    shot->expectBorder(Rect(16, 16, 48, 48), Color::BLACK);
}

TEST_F(LayerTransactionTest, SetCropWithResize) {
    sp<SurfaceControl> layer;
    ASSERT_NO_FATAL_FAILURE(layer = createLayer("test", 32, 32));
    ASSERT_NO_FATAL_FAILURE(fillLayerColor(layer, Color::RED));

    // setCrop is applied immediately by default, with or without resize pending
    Transaction().setCrop(layer, Rect(8, 8, 24, 24)).setSize(layer, 16, 16).apply();
    {
        SCOPED_TRACE("resize pending");
        auto shot = screenshot();
        shot->expectColor(Rect(8, 8, 24, 24), Color::RED);
        shot->expectBorder(Rect(8, 8, 24, 24), Color::BLACK);
    }

    ASSERT_NO_FATAL_FAILURE(fillLayerColor(layer, Color::RED));
    {
        SCOPED_TRACE("resize applied");
        auto shot = screenshot();
        shot->expectColor(Rect(8, 8, 16, 16), Color::RED);
        shot->expectBorder(Rect(8, 8, 16, 16), Color::BLACK);
    }
}

TEST_F(LayerTransactionTest, SetCropWithNextResize) {
    sp<SurfaceControl> layer;
    ASSERT_NO_FATAL_FAILURE(layer = createLayer("test", 32, 32));
    ASSERT_NO_FATAL_FAILURE(fillLayerColor(layer, Color::RED));

    // request setCrop to be applied with the next resize
    Transaction().setCrop(layer, Rect(8, 8, 24, 24)).setGeometryAppliesWithResize(layer).apply();
    {
        SCOPED_TRACE("waiting for next resize");
        screenshot()->expectColor(Rect(0, 0, 32, 32), Color::RED);
    }

    Transaction().setCrop(layer, Rect(4, 4, 12, 12)).apply();
    {
        SCOPED_TRACE("pending crop modified");
        screenshot()->expectColor(Rect(0, 0, 32, 32), Color::RED);
    }

    Transaction().setSize(layer, 16, 16).apply();
    {
        SCOPED_TRACE("resize pending");
        screenshot()->expectColor(Rect(0, 0, 32, 32), Color::RED);
    }

    // finally resize
    ASSERT_NO_FATAL_FAILURE(fillLayerColor(layer, Color::RED));
    {
        SCOPED_TRACE("new crop applied");
        auto shot = screenshot();
        shot->expectColor(Rect(4, 4, 12, 12), Color::RED);
        shot->expectBorder(Rect(4, 4, 12, 12), Color::BLACK);
    }
}

TEST_F(LayerTransactionTest, SetCropWithNextResizeScaleToWindow) {
    sp<SurfaceControl> layer;
    ASSERT_NO_FATAL_FAILURE(layer = createLayer("test", 32, 32));
    ASSERT_NO_FATAL_FAILURE(fillLayerColor(layer, Color::RED));

    // setCrop is not immediate even with SCALE_TO_WINDOW override
    Transaction()
            .setCrop(layer, Rect(4, 4, 12, 12))
            .setSize(layer, 16, 16)
            .setOverrideScalingMode(layer, NATIVE_WINDOW_SCALING_MODE_SCALE_TO_WINDOW)
            .setGeometryAppliesWithResize(layer)
            .apply();
    {
        SCOPED_TRACE("new crop pending");
        auto shot = screenshot();
        shot->expectColor(Rect(0, 0, 16, 16), Color::RED);
        shot->expectBorder(Rect(0, 0, 16, 16), Color::BLACK);
    }

    // XXX crop is never latched without other geometry change (b/69315677)
    Transaction().setPosition(layer, 1, 0).setGeometryAppliesWithResize(layer).apply();
    ASSERT_NO_FATAL_FAILURE(fillLayerColor(layer, Color::RED));
    Transaction().setPosition(layer, 0, 0).apply();
    {
        SCOPED_TRACE("new crop applied");
        auto shot = screenshot();
        shot->expectColor(Rect(4, 4, 12, 12), Color::RED);
        shot->expectBorder(Rect(4, 4, 12, 12), Color::BLACK);
    }
}

TEST_F(LayerTransactionTest, SetFinalCropBasic) {
    sp<SurfaceControl> layer;
    ASSERT_NO_FATAL_FAILURE(layer = createLayer("test", 32, 32));
    ASSERT_NO_FATAL_FAILURE(fillLayerColor(layer, Color::RED));
    const Rect crop(8, 8, 24, 24);

    // same as in SetCropBasic
    Transaction().setFinalCrop(layer, crop).apply();
    auto shot = screenshot();
    shot->expectColor(crop, Color::RED);
    shot->expectBorder(crop, Color::BLACK);
}

TEST_F(LayerTransactionTest, SetFinalCropEmpty) {
    sp<SurfaceControl> layer;
    ASSERT_NO_FATAL_FAILURE(layer = createLayer("test", 32, 32));
    ASSERT_NO_FATAL_FAILURE(fillLayerColor(layer, Color::RED));

    // same as in SetCropEmpty
    {
        SCOPED_TRACE("empty rect");
        Transaction().setFinalCrop(layer, Rect(8, 8, 8, 8)).apply();
        screenshot()->expectColor(Rect(0, 0, 32, 32), Color::RED);
    }

    {
        SCOPED_TRACE("negative rect");
        Transaction().setFinalCrop(layer, Rect(8, 8, 0, 0)).apply();
        screenshot()->expectColor(Rect(0, 0, 32, 32), Color::RED);
    }
}

TEST_F(LayerTransactionTest, SetFinalCropOutOfBounds) {
    sp<SurfaceControl> layer;
    ASSERT_NO_FATAL_FAILURE(layer = createLayer("test", 32, 32));
    ASSERT_NO_FATAL_FAILURE(fillLayerColor(layer, Color::RED));

    // same as in SetCropOutOfBounds
    Transaction().setFinalCrop(layer, Rect(-128, -64, 128, 64)).apply();
    auto shot = screenshot();
    shot->expectColor(Rect(0, 0, 32, 32), Color::RED);
    shot->expectBorder(Rect(0, 0, 32, 32), Color::BLACK);
}

TEST_F(LayerTransactionTest, SetFinalCropWithTranslation) {
    sp<SurfaceControl> layer;
    ASSERT_NO_FATAL_FAILURE(layer = createLayer("test", 32, 32));
    ASSERT_NO_FATAL_FAILURE(fillLayerColor(layer, Color::RED));

    // final crop is applied post-translation
    Transaction().setPosition(layer, 16, 16).setFinalCrop(layer, Rect(8, 8, 24, 24)).apply();
    auto shot = screenshot();
    shot->expectColor(Rect(16, 16, 24, 24), Color::RED);
    shot->expectBorder(Rect(16, 16, 24, 24), Color::BLACK);
}

TEST_F(LayerTransactionTest, SetFinalCropWithScale) {
    sp<SurfaceControl> layer;
    ASSERT_NO_FATAL_FAILURE(layer = createLayer("test", 32, 32));
    ASSERT_NO_FATAL_FAILURE(fillLayerColor(layer, Color::RED));

    // final crop is not affected by matrix
    Transaction()
            .setMatrix(layer, 2.0f, 0.0f, 0.0f, 2.0f)
            .setFinalCrop(layer, Rect(8, 8, 24, 24))
            .apply();
    auto shot = screenshot();
    shot->expectColor(Rect(8, 8, 24, 24), Color::RED);
    shot->expectBorder(Rect(8, 8, 24, 24), Color::BLACK);
}

TEST_F(LayerTransactionTest, SetFinalCropWithResize) {
    sp<SurfaceControl> layer;
    ASSERT_NO_FATAL_FAILURE(layer = createLayer("test", 32, 32));
    ASSERT_NO_FATAL_FAILURE(fillLayerColor(layer, Color::RED));

    // same as in SetCropWithResize
    Transaction().setFinalCrop(layer, Rect(8, 8, 24, 24)).setSize(layer, 16, 16).apply();
    {
        SCOPED_TRACE("resize pending");
        auto shot = screenshot();
        shot->expectColor(Rect(8, 8, 24, 24), Color::RED);
        shot->expectBorder(Rect(8, 8, 24, 24), Color::BLACK);
    }

    ASSERT_NO_FATAL_FAILURE(fillLayerColor(layer, Color::RED));
    {
        SCOPED_TRACE("resize applied");
        auto shot = screenshot();
        shot->expectColor(Rect(8, 8, 16, 16), Color::RED);
        shot->expectBorder(Rect(8, 8, 16, 16), Color::BLACK);
    }
}

TEST_F(LayerTransactionTest, SetFinalCropWithNextResize) {
    sp<SurfaceControl> layer;
    ASSERT_NO_FATAL_FAILURE(layer = createLayer("test", 32, 32));
    ASSERT_NO_FATAL_FAILURE(fillLayerColor(layer, Color::RED));

    // same as in SetCropWithNextResize
    Transaction()
            .setFinalCrop(layer, Rect(8, 8, 24, 24))
            .setGeometryAppliesWithResize(layer)
            .apply();
    {
        SCOPED_TRACE("waiting for next resize");
        screenshot()->expectColor(Rect(0, 0, 32, 32), Color::RED);
    }

    Transaction().setFinalCrop(layer, Rect(4, 4, 12, 12)).apply();
    {
        SCOPED_TRACE("pending final crop modified");
        screenshot()->expectColor(Rect(0, 0, 32, 32), Color::RED);
    }

    Transaction().setSize(layer, 16, 16).apply();
    {
        SCOPED_TRACE("resize pending");
        screenshot()->expectColor(Rect(0, 0, 32, 32), Color::RED);
    }

    // finally resize
    ASSERT_NO_FATAL_FAILURE(fillLayerColor(layer, Color::RED));
    {
        SCOPED_TRACE("new final crop applied");
        auto shot = screenshot();
        shot->expectColor(Rect(4, 4, 12, 12), Color::RED);
        shot->expectBorder(Rect(4, 4, 12, 12), Color::BLACK);
    }
}

TEST_F(LayerTransactionTest, SetFinalCropWithNextResizeScaleToWindow) {
    sp<SurfaceControl> layer;
    ASSERT_NO_FATAL_FAILURE(layer = createLayer("test", 32, 32));
    ASSERT_NO_FATAL_FAILURE(fillLayerColor(layer, Color::RED));

    // same as in SetCropWithNextResizeScaleToWindow
    Transaction()
            .setFinalCrop(layer, Rect(4, 4, 12, 12))
            .setSize(layer, 16, 16)
            .setOverrideScalingMode(layer, NATIVE_WINDOW_SCALING_MODE_SCALE_TO_WINDOW)
            .setGeometryAppliesWithResize(layer)
            .apply();
    {
        SCOPED_TRACE("new final crop pending");
        auto shot = screenshot();
        shot->expectColor(Rect(0, 0, 16, 16), Color::RED);
        shot->expectBorder(Rect(0, 0, 16, 16), Color::BLACK);
    }

    // XXX final crop is never latched without other geometry change (b/69315677)
    Transaction().setPosition(layer, 1, 0).setGeometryAppliesWithResize(layer).apply();
    ASSERT_NO_FATAL_FAILURE(fillLayerColor(layer, Color::RED));
    Transaction().setPosition(layer, 0, 0).apply();
    {
        SCOPED_TRACE("new final crop applied");
        auto shot = screenshot();
        shot->expectColor(Rect(4, 4, 12, 12), Color::RED);
        shot->expectBorder(Rect(4, 4, 12, 12), Color::BLACK);
    }
}

class LayerUpdateTest : public LayerTransactionTest {
protected:
    virtual void SetUp() {
        mComposerClient = new SurfaceComposerClient;
        ASSERT_EQ(NO_ERROR, mComposerClient->initCheck());

        sp<IBinder> display(
                SurfaceComposerClient::getBuiltInDisplay(ISurfaceComposer::eDisplayIdMain));
        DisplayInfo info;
        SurfaceComposerClient::getDisplayInfo(display, &info);

        ssize_t displayWidth = info.w;
        ssize_t displayHeight = info.h;

        // Background surface
        mBGSurfaceControl =
                mComposerClient->createSurface(String8("BG Test Surface"), displayWidth,
                                               displayHeight, PIXEL_FORMAT_RGBA_8888, 0);
        ASSERT_TRUE(mBGSurfaceControl != nullptr);
        ASSERT_TRUE(mBGSurfaceControl->isValid());
        fillSurfaceRGBA8(mBGSurfaceControl, 63, 63, 195);

        // Foreground surface
        mFGSurfaceControl = mComposerClient->createSurface(String8("FG Test Surface"), 64, 64,
                                                           PIXEL_FORMAT_RGBA_8888, 0);
        ASSERT_TRUE(mFGSurfaceControl != nullptr);
        ASSERT_TRUE(mFGSurfaceControl->isValid());

        fillSurfaceRGBA8(mFGSurfaceControl, 195, 63, 63);

        // Synchronization surface
        mSyncSurfaceControl = mComposerClient->createSurface(String8("Sync Test Surface"), 1, 1,
                                                             PIXEL_FORMAT_RGBA_8888, 0);
        ASSERT_TRUE(mSyncSurfaceControl != nullptr);
        ASSERT_TRUE(mSyncSurfaceControl->isValid());

        fillSurfaceRGBA8(mSyncSurfaceControl, 31, 31, 31);

        asTransaction([&](Transaction& t) {
            t.setDisplayLayerStack(display, 0);

            t.setLayer(mBGSurfaceControl, INT32_MAX - 2).show(mBGSurfaceControl);

            t.setLayer(mFGSurfaceControl, INT32_MAX - 1)
                    .setPosition(mFGSurfaceControl, 64, 64)
                    .show(mFGSurfaceControl);

            t.setLayer(mSyncSurfaceControl, INT32_MAX - 1)
                    .setPosition(mSyncSurfaceControl, displayWidth - 2, displayHeight - 2)
                    .show(mSyncSurfaceControl);
        });
    }

    virtual void TearDown() {
        mComposerClient->dispose();
        mBGSurfaceControl = 0;
        mFGSurfaceControl = 0;
        mSyncSurfaceControl = 0;
        mComposerClient = 0;
    }

    void waitForPostedBuffers() {
        // Since the sync surface is in synchronous mode (i.e. double buffered)
        // posting three buffers to it should ensure that at least two
        // SurfaceFlinger::handlePageFlip calls have been made, which should
        // guaranteed that a buffer posted to another Surface has been retired.
        fillSurfaceRGBA8(mSyncSurfaceControl, 31, 31, 31);
        fillSurfaceRGBA8(mSyncSurfaceControl, 31, 31, 31);
        fillSurfaceRGBA8(mSyncSurfaceControl, 31, 31, 31);
    }

    void asTransaction(const std::function<void(Transaction&)>& exec) {
        Transaction t;
        exec(t);
        t.apply(true);
    }

    sp<SurfaceComposerClient> mComposerClient;
    sp<SurfaceControl> mBGSurfaceControl;
    sp<SurfaceControl> mFGSurfaceControl;

    // This surface is used to ensure that the buffers posted to
    // mFGSurfaceControl have been picked up by SurfaceFlinger.
    sp<SurfaceControl> mSyncSurfaceControl;
};

TEST_F(LayerUpdateTest, RelativesAreNotDetached) {
    sp<ScreenCapture> sc;

    sp<SurfaceControl> relative = mComposerClient->createSurface(String8("relativeTestSurface"), 10,
                                                                 10, PIXEL_FORMAT_RGBA_8888, 0);
    fillSurfaceRGBA8(relative, 10, 10, 10);
    waitForPostedBuffers();

    Transaction{}
            .setRelativeLayer(relative, mFGSurfaceControl->getHandle(), 1)
            .setPosition(relative, 64, 64)
            .apply();

    {
        // The relative should be on top of the FG control.
        ScreenCapture::captureScreen(&sc);
        sc->checkPixel(64, 64, 10, 10, 10);
    }
    Transaction{}.detachChildren(mFGSurfaceControl).apply();

    {
        // Nothing should change at this point.
        ScreenCapture::captureScreen(&sc);
        sc->checkPixel(64, 64, 10, 10, 10);
    }

    Transaction{}.hide(relative).apply();

    {
        // Ensure that the relative was actually hidden, rather than
        // being left in the detached but visible state.
        ScreenCapture::captureScreen(&sc);
        sc->expectFGColor(64, 64);
    }
}

class GeometryLatchingTest : public LayerUpdateTest {
protected:
    void EXPECT_INITIAL_STATE(const char* trace) {
        SCOPED_TRACE(trace);
        ScreenCapture::captureScreen(&sc);
        // We find the leading edge of the FG surface.
        sc->expectFGColor(127, 127);
        sc->expectBGColor(128, 128);
    }

    void lockAndFillFGBuffer() { fillSurfaceRGBA8(mFGSurfaceControl, 195, 63, 63, false); }

    void unlockFGBuffer() {
        sp<Surface> s = mFGSurfaceControl->getSurface();
        ASSERT_EQ(NO_ERROR, s->unlockAndPost());
        waitForPostedBuffers();
    }

    void completeFGResize() {
        fillSurfaceRGBA8(mFGSurfaceControl, 195, 63, 63);
        waitForPostedBuffers();
    }
    void restoreInitialState() {
        asTransaction([&](Transaction& t) {
            t.setSize(mFGSurfaceControl, 64, 64);
            t.setPosition(mFGSurfaceControl, 64, 64);
            t.setCrop(mFGSurfaceControl, Rect(0, 0, 64, 64));
            t.setFinalCrop(mFGSurfaceControl, Rect(0, 0, -1, -1));
        });

        EXPECT_INITIAL_STATE("After restoring initial state");
    }
    sp<ScreenCapture> sc;
};

class CropLatchingTest : public GeometryLatchingTest {
protected:
    void EXPECT_CROPPED_STATE(const char* trace) {
        SCOPED_TRACE(trace);
        ScreenCapture::captureScreen(&sc);
        // The edge should be moved back one pixel by our crop.
        sc->expectFGColor(126, 126);
        sc->expectBGColor(127, 127);
        sc->expectBGColor(128, 128);
    }

    void EXPECT_RESIZE_STATE(const char* trace) {
        SCOPED_TRACE(trace);
        ScreenCapture::captureScreen(&sc);
        // The FG is now resized too 128,128 at 64,64
        sc->expectFGColor(64, 64);
        sc->expectFGColor(191, 191);
        sc->expectBGColor(192, 192);
    }
};

// In this test we ensure that setGeometryAppliesWithResize actually demands
// a buffer of the new size, and not just any size.
TEST_F(CropLatchingTest, FinalCropLatchingBufferOldSize) {
    EXPECT_INITIAL_STATE("before anything");
    // Normally the crop applies immediately even while a resize is pending.
    asTransaction([&](Transaction& t) {
        t.setSize(mFGSurfaceControl, 128, 128);
        t.setFinalCrop(mFGSurfaceControl, Rect(64, 64, 127, 127));
    });

    EXPECT_CROPPED_STATE("after setting crop (without geometryAppliesWithResize)");

    restoreInitialState();

    // In order to prepare to submit a buffer at the wrong size, we acquire it prior to
    // initiating the resize.
    lockAndFillFGBuffer();

    asTransaction([&](Transaction& t) {
        t.setSize(mFGSurfaceControl, 128, 128);
        t.setGeometryAppliesWithResize(mFGSurfaceControl);
        t.setFinalCrop(mFGSurfaceControl, Rect(64, 64, 127, 127));
    });

    EXPECT_INITIAL_STATE("after setting crop (with geometryAppliesWithResize)");

    // We now submit our old buffer, at the old size, and ensure it doesn't
    // trigger geometry latching.
    unlockFGBuffer();

    EXPECT_INITIAL_STATE("after unlocking FG buffer (with geometryAppliesWithResize)");

    completeFGResize();

    EXPECT_CROPPED_STATE("after the resize finishes");
}

TEST_F(LayerUpdateTest, DeferredTransactionTest) {
    sp<ScreenCapture> sc;
    {
        SCOPED_TRACE("before anything");
        ScreenCapture::captureScreen(&sc);
        sc->expectBGColor(32, 32);
        sc->expectFGColor(96, 96);
        sc->expectBGColor(160, 160);
    }

    // set up two deferred transactions on different frames
    asTransaction([&](Transaction& t) {
        t.setAlpha(mFGSurfaceControl, 0.75);
        t.deferTransactionUntil(mFGSurfaceControl, mSyncSurfaceControl->getHandle(),
                                mSyncSurfaceControl->getSurface()->getNextFrameNumber());
    });

    asTransaction([&](Transaction& t) {
        t.setPosition(mFGSurfaceControl, 128, 128);
        t.deferTransactionUntil(mFGSurfaceControl, mSyncSurfaceControl->getHandle(),
                                mSyncSurfaceControl->getSurface()->getNextFrameNumber() + 1);
    });

    {
        SCOPED_TRACE("before any trigger");
        ScreenCapture::captureScreen(&sc);
        sc->expectBGColor(32, 32);
        sc->expectFGColor(96, 96);
        sc->expectBGColor(160, 160);
    }

    // should trigger the first deferred transaction, but not the second one
    fillSurfaceRGBA8(mSyncSurfaceControl, 31, 31, 31);
    {
        SCOPED_TRACE("after first trigger");
        ScreenCapture::captureScreen(&sc);
        sc->expectBGColor(32, 32);
        sc->checkPixel(96, 96, 162, 63, 96);
        sc->expectBGColor(160, 160);
    }

    // should show up immediately since it's not deferred
    asTransaction([&](Transaction& t) { t.setAlpha(mFGSurfaceControl, 1.0); });

    // trigger the second deferred transaction
    fillSurfaceRGBA8(mSyncSurfaceControl, 31, 31, 31);
    {
        SCOPED_TRACE("after second trigger");
        ScreenCapture::captureScreen(&sc);
        sc->expectBGColor(32, 32);
        sc->expectBGColor(96, 96);
        sc->expectFGColor(160, 160);
    }
}

TEST_F(LayerUpdateTest, LayerWithNoBuffersResizesImmediately) {
    sp<ScreenCapture> sc;

    sp<SurfaceControl> childNoBuffer =
            mComposerClient->createSurface(String8("Bufferless child"), 10, 10,
                                           PIXEL_FORMAT_RGBA_8888, 0, mFGSurfaceControl.get());
    sp<SurfaceControl> childBuffer =
            mComposerClient->createSurface(String8("Buffered child"), 20, 20,
                                           PIXEL_FORMAT_RGBA_8888, 0, childNoBuffer.get());
    fillSurfaceRGBA8(childBuffer, 200, 200, 200);

    SurfaceComposerClient::Transaction{}.show(childNoBuffer).show(childBuffer).apply(true);

    {
        ScreenCapture::captureScreen(&sc);
        sc->expectChildColor(73, 73);
        sc->expectFGColor(74, 74);
    }

    SurfaceComposerClient::Transaction{}.setSize(childNoBuffer, 20, 20).apply(true);

    {
        ScreenCapture::captureScreen(&sc);
        sc->expectChildColor(73, 73);
        sc->expectChildColor(74, 74);
    }
}

TEST_F(LayerUpdateTest, MergingTransactions) {
    sp<ScreenCapture> sc;
    {
        SCOPED_TRACE("before move");
        ScreenCapture::captureScreen(&sc);
        sc->expectBGColor(0, 12);
        sc->expectFGColor(75, 75);
        sc->expectBGColor(145, 145);
    }

    Transaction t1, t2;
    t1.setPosition(mFGSurfaceControl, 128, 128);
    t2.setPosition(mFGSurfaceControl, 0, 0);
    // We expect that the position update from t2 now
    // overwrites the position update from t1.
    t1.merge(std::move(t2));
    t1.apply();

    {
        ScreenCapture::captureScreen(&sc);
        sc->expectFGColor(1, 1);
    }
}

class ChildLayerTest : public LayerUpdateTest {
protected:
    void SetUp() override {
        LayerUpdateTest::SetUp();
        mChild = mComposerClient->createSurface(String8("Child surface"), 10, 10,
                                                PIXEL_FORMAT_RGBA_8888, 0, mFGSurfaceControl.get());
        fillSurfaceRGBA8(mChild, 200, 200, 200);

        {
            SCOPED_TRACE("before anything");
            ScreenCapture::captureScreen(&mCapture);
            mCapture->expectChildColor(64, 64);
        }
    }
    void TearDown() override {
        LayerUpdateTest::TearDown();
        mChild = 0;
    }

    sp<SurfaceControl> mChild;
    sp<ScreenCapture> mCapture;
};

TEST_F(ChildLayerTest, ChildLayerPositioning) {
    asTransaction([&](Transaction& t) {
        t.show(mChild);
        t.setPosition(mChild, 10, 10);
        t.setPosition(mFGSurfaceControl, 64, 64);
    });

    {
        ScreenCapture::captureScreen(&mCapture);
        // Top left of foreground must now be visible
        mCapture->expectFGColor(64, 64);
        // But 10 pixels in we should see the child surface
        mCapture->expectChildColor(74, 74);
        // And 10 more pixels we should be back to the foreground surface
        mCapture->expectFGColor(84, 84);
    }

    asTransaction([&](Transaction& t) { t.setPosition(mFGSurfaceControl, 0, 0); });

    {
        ScreenCapture::captureScreen(&mCapture);
        // Top left of foreground should now be at 0, 0
        mCapture->expectFGColor(0, 0);
        // But 10 pixels in we should see the child surface
        mCapture->expectChildColor(10, 10);
        // And 10 more pixels we should be back to the foreground surface
        mCapture->expectFGColor(20, 20);
    }
}

TEST_F(ChildLayerTest, ChildLayerCropping) {
    asTransaction([&](Transaction& t) {
        t.show(mChild);
        t.setPosition(mChild, 0, 0);
        t.setPosition(mFGSurfaceControl, 0, 0);
        t.setCrop(mFGSurfaceControl, Rect(0, 0, 5, 5));
    });

    {
        ScreenCapture::captureScreen(&mCapture);
        mCapture->expectChildColor(0, 0);
        mCapture->expectChildColor(4, 4);
        mCapture->expectBGColor(5, 5);
    }
}

TEST_F(ChildLayerTest, ChildLayerFinalCropping) {
    asTransaction([&](Transaction& t) {
        t.show(mChild);
        t.setPosition(mChild, 0, 0);
        t.setPosition(mFGSurfaceControl, 0, 0);
        t.setFinalCrop(mFGSurfaceControl, Rect(0, 0, 5, 5));
    });

    {
        ScreenCapture::captureScreen(&mCapture);
        mCapture->expectChildColor(0, 0);
        mCapture->expectChildColor(4, 4);
        mCapture->expectBGColor(5, 5);
    }
}

TEST_F(ChildLayerTest, ChildLayerConstraints) {
    asTransaction([&](Transaction& t) {
        t.show(mChild);
        t.setPosition(mFGSurfaceControl, 0, 0);
        t.setPosition(mChild, 63, 63);
    });

    {
        ScreenCapture::captureScreen(&mCapture);
        mCapture->expectFGColor(0, 0);
        // Last pixel in foreground should now be the child.
        mCapture->expectChildColor(63, 63);
        // But the child should be constrained and the next pixel
        // must be the background
        mCapture->expectBGColor(64, 64);
    }
}

TEST_F(ChildLayerTest, ChildLayerScaling) {
    asTransaction([&](Transaction& t) { t.setPosition(mFGSurfaceControl, 0, 0); });

    // Find the boundary between the parent and child
    {
        ScreenCapture::captureScreen(&mCapture);
        mCapture->expectChildColor(9, 9);
        mCapture->expectFGColor(10, 10);
    }

    asTransaction([&](Transaction& t) { t.setMatrix(mFGSurfaceControl, 2.0, 0, 0, 2.0); });

    // The boundary should be twice as far from the origin now.
    // The pixels from the last test should all be child now
    {
        ScreenCapture::captureScreen(&mCapture);
        mCapture->expectChildColor(9, 9);
        mCapture->expectChildColor(10, 10);
        mCapture->expectChildColor(19, 19);
        mCapture->expectFGColor(20, 20);
    }
}

TEST_F(ChildLayerTest, ChildLayerAlpha) {
    fillSurfaceRGBA8(mBGSurfaceControl, 0, 0, 254);
    fillSurfaceRGBA8(mFGSurfaceControl, 254, 0, 0);
    fillSurfaceRGBA8(mChild, 0, 254, 0);
    waitForPostedBuffers();

    asTransaction([&](Transaction& t) {
        t.show(mChild);
        t.setPosition(mChild, 0, 0);
        t.setPosition(mFGSurfaceControl, 0, 0);
    });

    {
        ScreenCapture::captureScreen(&mCapture);
        // Unblended child color
        mCapture->checkPixel(0, 0, 0, 254, 0);
    }

    asTransaction([&](Transaction& t) { t.setAlpha(mChild, 0.5); });

    {
        ScreenCapture::captureScreen(&mCapture);
        // Child and BG blended.
        mCapture->checkPixel(0, 0, 127, 127, 0);
    }

    asTransaction([&](Transaction& t) { t.setAlpha(mFGSurfaceControl, 0.5); });

    {
        ScreenCapture::captureScreen(&mCapture);
        // Child and BG blended.
        mCapture->checkPixel(0, 0, 95, 64, 95);
    }
}

TEST_F(ChildLayerTest, ReparentChildren) {
    asTransaction([&](Transaction& t) {
        t.show(mChild);
        t.setPosition(mChild, 10, 10);
        t.setPosition(mFGSurfaceControl, 64, 64);
    });

    {
        ScreenCapture::captureScreen(&mCapture);
        // Top left of foreground must now be visible
        mCapture->expectFGColor(64, 64);
        // But 10 pixels in we should see the child surface
        mCapture->expectChildColor(74, 74);
        // And 10 more pixels we should be back to the foreground surface
        mCapture->expectFGColor(84, 84);
    }

    asTransaction([&](Transaction& t) {
        t.reparentChildren(mFGSurfaceControl, mBGSurfaceControl->getHandle());
    });

    {
        ScreenCapture::captureScreen(&mCapture);
        mCapture->expectFGColor(64, 64);
        // In reparenting we should have exposed the entire foreground surface.
        mCapture->expectFGColor(74, 74);
        // And the child layer should now begin at 10, 10 (since the BG
        // layer is at (0, 0)).
        mCapture->expectBGColor(9, 9);
        mCapture->expectChildColor(10, 10);
    }
}

TEST_F(ChildLayerTest, DetachChildrenSameClient) {
    asTransaction([&](Transaction& t) {
        t.show(mChild);
        t.setPosition(mChild, 10, 10);
        t.setPosition(mFGSurfaceControl, 64, 64);
    });

    {
        ScreenCapture::captureScreen(&mCapture);
        // Top left of foreground must now be visible
        mCapture->expectFGColor(64, 64);
        // But 10 pixels in we should see the child surface
        mCapture->expectChildColor(74, 74);
        // And 10 more pixels we should be back to the foreground surface
        mCapture->expectFGColor(84, 84);
    }

    asTransaction([&](Transaction& t) { t.detachChildren(mFGSurfaceControl); });

    asTransaction([&](Transaction& t) { t.hide(mChild); });

    // Since the child has the same client as the parent, it will not get
    // detached and will be hidden.
    {
        ScreenCapture::captureScreen(&mCapture);
        mCapture->expectFGColor(64, 64);
        mCapture->expectFGColor(74, 74);
        mCapture->expectFGColor(84, 84);
    }
}

TEST_F(ChildLayerTest, DetachChildrenDifferentClient) {
    sp<SurfaceComposerClient> mNewComposerClient = new SurfaceComposerClient;
    sp<SurfaceControl> mChildNewClient =
            mNewComposerClient->createSurface(String8("New Child Test Surface"), 10, 10,
                                              PIXEL_FORMAT_RGBA_8888, 0, mFGSurfaceControl.get());

    ASSERT_TRUE(mChildNewClient != nullptr);
    ASSERT_TRUE(mChildNewClient->isValid());

    fillSurfaceRGBA8(mChildNewClient, 200, 200, 200);

    asTransaction([&](Transaction& t) {
        t.hide(mChild);
        t.show(mChildNewClient);
        t.setPosition(mChildNewClient, 10, 10);
        t.setPosition(mFGSurfaceControl, 64, 64);
    });

    {
        ScreenCapture::captureScreen(&mCapture);
        // Top left of foreground must now be visible
        mCapture->expectFGColor(64, 64);
        // But 10 pixels in we should see the child surface
        mCapture->expectChildColor(74, 74);
        // And 10 more pixels we should be back to the foreground surface
        mCapture->expectFGColor(84, 84);
    }

    asTransaction([&](Transaction& t) { t.detachChildren(mFGSurfaceControl); });

    asTransaction([&](Transaction& t) { t.hide(mChildNewClient); });

    // Nothing should have changed.
    {
        ScreenCapture::captureScreen(&mCapture);
        mCapture->expectFGColor(64, 64);
        mCapture->expectChildColor(74, 74);
        mCapture->expectFGColor(84, 84);
    }
}

TEST_F(ChildLayerTest, ChildrenInheritNonTransformScalingFromParent) {
    asTransaction([&](Transaction& t) {
        t.show(mChild);
        t.setPosition(mChild, 0, 0);
        t.setPosition(mFGSurfaceControl, 0, 0);
    });

    {
        ScreenCapture::captureScreen(&mCapture);
        // We've positioned the child in the top left.
        mCapture->expectChildColor(0, 0);
        // But it's only 10x10.
        mCapture->expectFGColor(10, 10);
    }

    asTransaction([&](Transaction& t) {
        t.setOverrideScalingMode(mFGSurfaceControl, NATIVE_WINDOW_SCALING_MODE_SCALE_TO_WINDOW);
        // We cause scaling by 2.
        t.setSize(mFGSurfaceControl, 128, 128);
    });

    {
        ScreenCapture::captureScreen(&mCapture);
        // We've positioned the child in the top left.
        mCapture->expectChildColor(0, 0);
        mCapture->expectChildColor(10, 10);
        mCapture->expectChildColor(19, 19);
        // And now it should be scaled all the way to 20x20
        mCapture->expectFGColor(20, 20);
    }
}

// Regression test for b/37673612
TEST_F(ChildLayerTest, ChildrenWithParentBufferTransform) {
    asTransaction([&](Transaction& t) {
        t.show(mChild);
        t.setPosition(mChild, 0, 0);
        t.setPosition(mFGSurfaceControl, 0, 0);
    });

    {
        ScreenCapture::captureScreen(&mCapture);
        // We've positioned the child in the top left.
        mCapture->expectChildColor(0, 0);
        // But it's only 10x10.
        mCapture->expectFGColor(10, 10);
    }
    // We set things up as in b/37673612 so that there is a mismatch between the buffer size and
    // the WM specified state size.
    asTransaction([&](Transaction& t) { t.setSize(mFGSurfaceControl, 128, 64); });
    sp<Surface> s = mFGSurfaceControl->getSurface();
    auto anw = static_cast<ANativeWindow*>(s.get());
    native_window_set_buffers_transform(anw, NATIVE_WINDOW_TRANSFORM_ROT_90);
    native_window_set_buffers_dimensions(anw, 64, 128);
    fillSurfaceRGBA8(mFGSurfaceControl, 195, 63, 63);
    waitForPostedBuffers();

    {
        // The child should still be in the same place and not have any strange scaling as in
        // b/37673612.
        ScreenCapture::captureScreen(&mCapture);
        mCapture->expectChildColor(0, 0);
        mCapture->expectFGColor(10, 10);
    }
}

TEST_F(ChildLayerTest, Bug36858924) {
    // Destroy the child layer
    mChild.clear();

    // Now recreate it as hidden
    mChild = mComposerClient->createSurface(String8("Child surface"), 10, 10,
                                            PIXEL_FORMAT_RGBA_8888, ISurfaceComposerClient::eHidden,
                                            mFGSurfaceControl.get());

    // Show the child layer in a deferred transaction
    asTransaction([&](Transaction& t) {
        t.deferTransactionUntil(mChild, mFGSurfaceControl->getHandle(),
                                mFGSurfaceControl->getSurface()->getNextFrameNumber());
        t.show(mChild);
    });

    // Render the foreground surface a few times
    //
    // Prior to the bugfix for b/36858924, this would usually hang while trying to fill the third
    // frame because SurfaceFlinger would never process the deferred transaction and would therefore
    // never acquire/release the first buffer
    ALOGI("Filling 1");
    fillSurfaceRGBA8(mFGSurfaceControl, 0, 255, 0);
    ALOGI("Filling 2");
    fillSurfaceRGBA8(mFGSurfaceControl, 0, 0, 255);
    ALOGI("Filling 3");
    fillSurfaceRGBA8(mFGSurfaceControl, 255, 0, 0);
    ALOGI("Filling 4");
    fillSurfaceRGBA8(mFGSurfaceControl, 0, 255, 0);
}

TEST_F(ChildLayerTest, Reparent) {
    asTransaction([&](Transaction& t) {
        t.show(mChild);
        t.setPosition(mChild, 10, 10);
        t.setPosition(mFGSurfaceControl, 64, 64);
    });

    {
        ScreenCapture::captureScreen(&mCapture);
        // Top left of foreground must now be visible
        mCapture->expectFGColor(64, 64);
        // But 10 pixels in we should see the child surface
        mCapture->expectChildColor(74, 74);
        // And 10 more pixels we should be back to the foreground surface
        mCapture->expectFGColor(84, 84);
    }

    asTransaction([&](Transaction& t) { t.reparent(mChild, mBGSurfaceControl->getHandle()); });

    {
        ScreenCapture::captureScreen(&mCapture);
        mCapture->expectFGColor(64, 64);
        // In reparenting we should have exposed the entire foreground surface.
        mCapture->expectFGColor(74, 74);
        // And the child layer should now begin at 10, 10 (since the BG
        // layer is at (0, 0)).
        mCapture->expectBGColor(9, 9);
        mCapture->expectChildColor(10, 10);
    }
}

TEST_F(ChildLayerTest, ReparentToNoParent) {
    asTransaction([&](Transaction& t) {
        t.show(mChild);
        t.setPosition(mChild, 10, 10);
        t.setPosition(mFGSurfaceControl, 64, 64);
    });

    {
        ScreenCapture::captureScreen(&mCapture);
        // Top left of foreground must now be visible
        mCapture->expectFGColor(64, 64);
        // But 10 pixels in we should see the child surface
        mCapture->expectChildColor(74, 74);
        // And 10 more pixels we should be back to the foreground surface
        mCapture->expectFGColor(84, 84);
    }
    asTransaction([&](Transaction& t) { t.reparent(mChild, nullptr); });
    {
        ScreenCapture::captureScreen(&mCapture);
        // Nothing should have changed.
        mCapture->expectFGColor(64, 64);
        mCapture->expectChildColor(74, 74);
        mCapture->expectFGColor(84, 84);
    }
}

TEST_F(ChildLayerTest, ReparentFromNoParent) {
    sp<SurfaceControl> newSurface = mComposerClient->createSurface(String8("New Surface"), 10, 10,
                                                                   PIXEL_FORMAT_RGBA_8888, 0);
    ASSERT_TRUE(newSurface != nullptr);
    ASSERT_TRUE(newSurface->isValid());

    fillSurfaceRGBA8(newSurface, 63, 195, 63);
    asTransaction([&](Transaction& t) {
        t.hide(mChild);
        t.show(newSurface);
        t.setPosition(newSurface, 10, 10);
        t.setLayer(newSurface, INT32_MAX - 2);
        t.setPosition(mFGSurfaceControl, 64, 64);
    });

    {
        ScreenCapture::captureScreen(&mCapture);
        // Top left of foreground must now be visible
        mCapture->expectFGColor(64, 64);
        // At 10, 10 we should see the new surface
        mCapture->checkPixel(10, 10, 63, 195, 63);
    }

    asTransaction([&](Transaction& t) { t.reparent(newSurface, mFGSurfaceControl->getHandle()); });

    {
        ScreenCapture::captureScreen(&mCapture);
        // newSurface will now be a child of mFGSurface so it will be 10, 10 offset from
        // mFGSurface, putting it at 74, 74.
        mCapture->expectFGColor(64, 64);
        mCapture->checkPixel(74, 74, 63, 195, 63);
        mCapture->expectFGColor(84, 84);
    }
}

TEST_F(ChildLayerTest, NestedChildren) {
    sp<SurfaceControl> grandchild =
            mComposerClient->createSurface(String8("Grandchild surface"), 10, 10,
                                           PIXEL_FORMAT_RGBA_8888, 0, mChild.get());
    fillSurfaceRGBA8(grandchild, 50, 50, 50);

    {
        ScreenCapture::captureScreen(&mCapture);
        // Expect the grandchild to begin at 64, 64 because it's a child of mChild layer
        // which begins at 64, 64
        mCapture->checkPixel(64, 64, 50, 50, 50);
    }
}

TEST_F(ChildLayerTest, ChildLayerRelativeLayer) {
    sp<SurfaceControl> relative = mComposerClient->createSurface(String8("Relative surface"), 128,
                                                                 128, PIXEL_FORMAT_RGBA_8888, 0);
    fillSurfaceRGBA8(relative, 255, 255, 255);

    Transaction t;
    t.setLayer(relative, INT32_MAX)
            .setRelativeLayer(mChild, relative->getHandle(), 1)
            .setPosition(mFGSurfaceControl, 0, 0)
            .apply(true);

    // We expect that the child should have been elevated above our
    // INT_MAX layer even though it's not a child of it.
    {
        ScreenCapture::captureScreen(&mCapture);
        mCapture->expectChildColor(0, 0);
        mCapture->expectChildColor(9, 9);
        mCapture->checkPixel(10, 10, 255, 255, 255);
    }
}

class ScreenCaptureTest : public LayerUpdateTest {
protected:
    std::unique_ptr<ScreenCapture> mCapture;
};

TEST_F(ScreenCaptureTest, CaptureSingleLayer) {
    auto bgHandle = mBGSurfaceControl->getHandle();
    ScreenCapture::captureLayers(&mCapture, bgHandle);
    mCapture->expectBGColor(0, 0);
    // Doesn't capture FG layer which is at 64, 64
    mCapture->expectBGColor(64, 64);
}

TEST_F(ScreenCaptureTest, CaptureLayerWithChild) {
    auto fgHandle = mFGSurfaceControl->getHandle();

    sp<SurfaceControl> child =
            mComposerClient->createSurface(String8("Child surface"), 10, 10, PIXEL_FORMAT_RGBA_8888,
                                           0, mFGSurfaceControl.get());
    fillSurfaceRGBA8(child, 200, 200, 200);

    SurfaceComposerClient::Transaction().show(child).apply(true);

    // Captures mFGSurfaceControl layer and its child.
    ScreenCapture::captureLayers(&mCapture, fgHandle);
    mCapture->expectFGColor(10, 10);
    mCapture->expectChildColor(0, 0);
}

TEST_F(ScreenCaptureTest, CaptureLayerChildOnly) {
    auto fgHandle = mFGSurfaceControl->getHandle();

    sp<SurfaceControl> child =
            mComposerClient->createSurface(String8("Child surface"), 10, 10, PIXEL_FORMAT_RGBA_8888,
                                           0, mFGSurfaceControl.get());
    fillSurfaceRGBA8(child, 200, 200, 200);

    SurfaceComposerClient::Transaction().show(child).apply(true);

    // Captures mFGSurfaceControl's child
    ScreenCapture::captureChildLayers(&mCapture, fgHandle);
    mCapture->checkPixel(10, 10, 0, 0, 0);
    mCapture->expectChildColor(0, 0);
}

TEST_F(ScreenCaptureTest, CaptureTransparent) {
    sp<SurfaceControl> child =
            mComposerClient->createSurface(String8("Child surface"), 10, 10, PIXEL_FORMAT_RGBA_8888,
                                           0, mFGSurfaceControl.get());

    fillSurfaceRGBA8(child, 200, 200, 200);

    SurfaceComposerClient::Transaction().show(child).apply(true);

    auto childHandle = child->getHandle();

    // Captures child
    ScreenCapture::captureLayers(&mCapture, childHandle, {0, 0, 10, 20});
    mCapture->expectColor(Rect(0, 0, 9, 9), {200, 200, 200, 255});
    // Area outside of child's bounds is transparent.
    mCapture->expectColor(Rect(0, 10, 9, 19), {0, 0, 0, 0});
}

TEST_F(ScreenCaptureTest, DontCaptureRelativeOutsideTree) {
    auto fgHandle = mFGSurfaceControl->getHandle();

    sp<SurfaceControl> child =
            mComposerClient->createSurface(String8("Child surface"), 10, 10, PIXEL_FORMAT_RGBA_8888,
                                           0, mFGSurfaceControl.get());
    sp<SurfaceControl> relative = mComposerClient->createSurface(String8("Relative surface"), 10,
                                                                 10, PIXEL_FORMAT_RGBA_8888, 0);
    fillSurfaceRGBA8(child, 200, 200, 200);
    fillSurfaceRGBA8(relative, 100, 100, 100);

    SurfaceComposerClient::Transaction()
            .show(child)
            // Set relative layer above fg layer so should be shown above when computing all layers.
            .setRelativeLayer(relative, fgHandle, 1)
            .show(relative)
            .apply(true);

    // Captures mFGSurfaceControl layer and its child. Relative layer shouldn't be captured.
    ScreenCapture::captureLayers(&mCapture, fgHandle);
    mCapture->expectFGColor(10, 10);
    mCapture->expectChildColor(0, 0);
}

TEST_F(ScreenCaptureTest, CaptureRelativeInTree) {
    auto fgHandle = mFGSurfaceControl->getHandle();

    sp<SurfaceControl> child =
            mComposerClient->createSurface(String8("Child surface"), 10, 10, PIXEL_FORMAT_RGBA_8888,
                                           0, mFGSurfaceControl.get());
    sp<SurfaceControl> relative =
            mComposerClient->createSurface(String8("Relative surface"), 10, 10,
                                           PIXEL_FORMAT_RGBA_8888, 0, mFGSurfaceControl.get());
    fillSurfaceRGBA8(child, 200, 200, 200);
    fillSurfaceRGBA8(relative, 100, 100, 100);

    SurfaceComposerClient::Transaction()
            .show(child)
            // Set relative layer below fg layer but relative to child layer so it should be shown
            // above child layer.
            .setLayer(relative, -1)
            .setRelativeLayer(relative, child->getHandle(), 1)
            .show(relative)
            .apply(true);

    // Captures mFGSurfaceControl layer and its children. Relative layer is a child of fg so its
    // relative value should be taken into account, placing it above child layer.
    ScreenCapture::captureLayers(&mCapture, fgHandle);
    mCapture->expectFGColor(10, 10);
    // Relative layer is showing on top of child layer
    mCapture->expectColor(Rect(0, 0, 9, 9), {100, 100, 100, 255});
}

// In the following tests we verify successful skipping of a parent layer,
// so we use the same verification logic and only change how we mutate
// the parent layer to verify that various properties are ignored.
class ScreenCaptureChildOnlyTest : public LayerUpdateTest {
public:
    void SetUp() override {
        LayerUpdateTest::SetUp();

        mChild =
            mComposerClient->createSurface(String8("Child surface"), 10, 10, PIXEL_FORMAT_RGBA_8888,
                    0, mFGSurfaceControl.get());
        fillSurfaceRGBA8(mChild, 200, 200, 200);

        SurfaceComposerClient::Transaction().show(mChild).apply(true);
    }

    void verify() {
        auto fgHandle = mFGSurfaceControl->getHandle();
        ScreenCapture::captureChildLayers(&mCapture, fgHandle);
        mCapture->checkPixel(10, 10, 0, 0, 0);
        mCapture->expectChildColor(0, 0);
    }

    std::unique_ptr<ScreenCapture> mCapture;
    sp<SurfaceControl> mChild;
};

TEST_F(ScreenCaptureChildOnlyTest, CaptureLayerIgnoresParentVisibility) {

    SurfaceComposerClient::Transaction().hide(mFGSurfaceControl).apply(true);

    // Even though the parent is hidden we should still capture the child.
    verify();
}

TEST_F(ScreenCaptureChildOnlyTest, CaptureLayerIgnoresParentCrop) {

    SurfaceComposerClient::Transaction().setCrop(mFGSurfaceControl, Rect(0, 0, 1, 1)).apply(true);

    // Even though the parent is cropped out we should still capture the child.
    verify();
}

TEST_F(ScreenCaptureChildOnlyTest, CaptureLayerIgnoresTransform) {

    SurfaceComposerClient::Transaction().setMatrix(mFGSurfaceControl, 2, 0, 0, 2);

    // We should not inherit the parent scaling.
    verify();
}

TEST_F(ScreenCaptureChildOnlyTest, RegressionTest76099859) {
    SurfaceComposerClient::Transaction().hide(mFGSurfaceControl).apply(true);

    // Even though the parent is hidden we should still capture the child.
    verify();

    // Verify everything was properly hidden when rendering the full-screen.
    screenshot()->expectBGColor(0,0);
}


TEST_F(ScreenCaptureTest, CaptureLayerWithGrandchild) {
    auto fgHandle = mFGSurfaceControl->getHandle();

    sp<SurfaceControl> child =
            mComposerClient->createSurface(String8("Child surface"), 10, 10, PIXEL_FORMAT_RGBA_8888,
                                           0, mFGSurfaceControl.get());
    fillSurfaceRGBA8(child, 200, 200, 200);

    sp<SurfaceControl> grandchild =
            mComposerClient->createSurface(String8("Grandchild surface"), 5, 5,
                                           PIXEL_FORMAT_RGBA_8888, 0, child.get());

    fillSurfaceRGBA8(grandchild, 50, 50, 50);
    SurfaceComposerClient::Transaction()
            .show(child)
            .setPosition(grandchild, 5, 5)
            .show(grandchild)
            .apply(true);

    // Captures mFGSurfaceControl, its child, and the grandchild.
    ScreenCapture::captureLayers(&mCapture, fgHandle);
    mCapture->expectFGColor(10, 10);
    mCapture->expectChildColor(0, 0);
    mCapture->checkPixel(5, 5, 50, 50, 50);
}

TEST_F(ScreenCaptureTest, CaptureChildOnly) {
    sp<SurfaceControl> child =
            mComposerClient->createSurface(String8("Child surface"), 10, 10, PIXEL_FORMAT_RGBA_8888,
                                           0, mFGSurfaceControl.get());
    fillSurfaceRGBA8(child, 200, 200, 200);
    auto childHandle = child->getHandle();

    SurfaceComposerClient::Transaction().setPosition(child, 5, 5).show(child).apply(true);

    // Captures only the child layer, and not the parent.
    ScreenCapture::captureLayers(&mCapture, childHandle);
    mCapture->expectChildColor(0, 0);
    mCapture->expectChildColor(9, 9);
}

TEST_F(ScreenCaptureTest, CaptureGrandchildOnly) {
    sp<SurfaceControl> child =
            mComposerClient->createSurface(String8("Child surface"), 10, 10, PIXEL_FORMAT_RGBA_8888,
                                           0, mFGSurfaceControl.get());
    fillSurfaceRGBA8(child, 200, 200, 200);
    auto childHandle = child->getHandle();

    sp<SurfaceControl> grandchild =
            mComposerClient->createSurface(String8("Grandchild surface"), 5, 5,
                                           PIXEL_FORMAT_RGBA_8888, 0, child.get());
    fillSurfaceRGBA8(grandchild, 50, 50, 50);

    SurfaceComposerClient::Transaction()
            .show(child)
            .setPosition(grandchild, 5, 5)
            .show(grandchild)
            .apply(true);

    auto grandchildHandle = grandchild->getHandle();

    // Captures only the grandchild.
    ScreenCapture::captureLayers(&mCapture, grandchildHandle);
    mCapture->checkPixel(0, 0, 50, 50, 50);
    mCapture->checkPixel(4, 4, 50, 50, 50);
}

TEST_F(ScreenCaptureTest, CaptureCrop) {
    sp<SurfaceControl> redLayer = mComposerClient->createSurface(String8("Red surface"), 60, 60,
                                                                 PIXEL_FORMAT_RGBA_8888, 0);
    sp<SurfaceControl> blueLayer =
            mComposerClient->createSurface(String8("Blue surface"), 30, 30, PIXEL_FORMAT_RGBA_8888,
                                           0, redLayer.get());

    ASSERT_NO_FATAL_FAILURE(fillLayerColor(redLayer, Color::RED));
    ASSERT_NO_FATAL_FAILURE(fillLayerColor(blueLayer, Color::BLUE));

    SurfaceComposerClient::Transaction()
            .setLayer(redLayer, INT32_MAX - 1)
            .show(redLayer)
            .show(blueLayer)
            .apply(true);

    auto redLayerHandle = redLayer->getHandle();

    // Capturing full screen should have both red and blue are visible.
    ScreenCapture::captureLayers(&mCapture, redLayerHandle);
    mCapture->expectColor(Rect(0, 0, 29, 29), Color::BLUE);
    // red area below the blue area
    mCapture->expectColor(Rect(0, 30, 59, 59), Color::RED);
    // red area to the right of the blue area
    mCapture->expectColor(Rect(30, 0, 59, 59), Color::RED);

    Rect crop = Rect(0, 0, 30, 30);
    ScreenCapture::captureLayers(&mCapture, redLayerHandle, crop);
    // Capturing the cropped screen, cropping out the shown red area, should leave only the blue
    // area visible.
    mCapture->expectColor(Rect(0, 0, 29, 29), Color::BLUE);
    mCapture->checkPixel(30, 30, 0, 0, 0);
}

TEST_F(ScreenCaptureTest, CaptureSize) {
    sp<SurfaceControl> redLayer = mComposerClient->createSurface(String8("Red surface"), 60, 60,
                                                                 PIXEL_FORMAT_RGBA_8888, 0);
    sp<SurfaceControl> blueLayer =
            mComposerClient->createSurface(String8("Blue surface"), 30, 30, PIXEL_FORMAT_RGBA_8888,
                                           0, redLayer.get());

    ASSERT_NO_FATAL_FAILURE(fillLayerColor(redLayer, Color::RED));
    ASSERT_NO_FATAL_FAILURE(fillLayerColor(blueLayer, Color::BLUE));

    SurfaceComposerClient::Transaction()
            .setLayer(redLayer, INT32_MAX - 1)
            .show(redLayer)
            .show(blueLayer)
            .apply(true);

    auto redLayerHandle = redLayer->getHandle();

    // Capturing full screen should have both red and blue are visible.
    ScreenCapture::captureLayers(&mCapture, redLayerHandle);
    mCapture->expectColor(Rect(0, 0, 29, 29), Color::BLUE);
    // red area below the blue area
    mCapture->expectColor(Rect(0, 30, 59, 59), Color::RED);
    // red area to the right of the blue area
    mCapture->expectColor(Rect(30, 0, 59, 59), Color::RED);

    ScreenCapture::captureLayers(&mCapture, redLayerHandle, Rect::EMPTY_RECT, 0.5);
    // Capturing the downsized area (30x30) should leave both red and blue but in a smaller area.
    mCapture->expectColor(Rect(0, 0, 14, 14), Color::BLUE);
    // red area below the blue area
    mCapture->expectColor(Rect(0, 15, 29, 29), Color::RED);
    // red area to the right of the blue area
    mCapture->expectColor(Rect(15, 0, 29, 29), Color::RED);
    mCapture->checkPixel(30, 30, 0, 0, 0);
}

TEST_F(ScreenCaptureTest, CaptureInvalidLayer) {
    sp<SurfaceControl> redLayer = mComposerClient->createSurface(String8("Red surface"), 60, 60,
                                                                 PIXEL_FORMAT_RGBA_8888, 0);

    ASSERT_NO_FATAL_FAILURE(fillLayerColor(redLayer, Color::RED));

    auto redLayerHandle = redLayer->getHandle();
    mComposerClient->destroySurface(redLayerHandle);
    SurfaceComposerClient::Transaction().apply(true);

    sp<GraphicBuffer> outBuffer;

    // Layer was deleted so captureLayers should fail with NAME_NOT_FOUND
    sp<ISurfaceComposer> sf(ComposerService::getComposerService());
    ASSERT_EQ(NAME_NOT_FOUND, sf->captureLayers(redLayerHandle, &outBuffer, Rect::EMPTY_RECT, 1.0));
}


class DereferenceSurfaceControlTest : public LayerTransactionTest {
protected:
    void SetUp() override {
        LayerTransactionTest::SetUp();
        bgLayer = createLayer("BG layer", 20, 20);
        fillLayerColor(bgLayer, Color::RED);
        fgLayer = createLayer("FG layer", 20, 20);
        fillLayerColor(fgLayer, Color::BLUE);
        Transaction().setLayer(fgLayer, mLayerZBase + 1).apply();
        {
            SCOPED_TRACE("before anything");
            auto shot = screenshot();
            shot->expectColor(Rect(0, 0, 20, 20), Color::BLUE);
        }
    }
    void TearDown() override {
        LayerTransactionTest::TearDown();
        bgLayer = 0;
        fgLayer = 0;
    }

    sp<SurfaceControl> bgLayer;
    sp<SurfaceControl> fgLayer;
};

TEST_F(DereferenceSurfaceControlTest, LayerNotInTransaction) {
    fgLayer = nullptr;
    {
        SCOPED_TRACE("after setting null");
        auto shot = screenshot();
        shot->expectColor(Rect(0, 0, 20, 20), Color::RED);
    }
}

TEST_F(DereferenceSurfaceControlTest, LayerInTransaction) {
    auto transaction = Transaction().show(fgLayer);
    fgLayer = nullptr;
    {
        SCOPED_TRACE("after setting null");
        auto shot = screenshot();
        shot->expectColor(Rect(0, 0, 20, 20), Color::BLUE);
    }
}

} // namespace android
