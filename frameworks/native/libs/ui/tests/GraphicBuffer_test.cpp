/*
 * Copyright 2018 The Android Open Source Project
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

#define LOG_TAG "GraphicBufferTest"

#include <ui/DetachedBufferHandle.h>
#include <ui/GraphicBuffer.h>

#include <gtest/gtest.h>

namespace android {

namespace {

constexpr uint32_t kTestWidth = 1024;
constexpr uint32_t kTestHeight = 1;
constexpr uint32_t kTestFormat = HAL_PIXEL_FORMAT_BLOB;
constexpr uint32_t kTestLayerCount = 1;
constexpr uint64_t kTestUsage = GraphicBuffer::USAGE_SW_WRITE_OFTEN;

} // namespace

class GraphicBufferTest : public testing::Test {};

TEST_F(GraphicBufferTest, DetachedBuffer) {
    sp<GraphicBuffer> buffer(
            new GraphicBuffer(kTestWidth, kTestHeight, kTestFormat, kTestLayerCount, kTestUsage));

    // Currently a newly allocated GraphicBuffer is in legacy mode, i.e. not associated with
    // BufferHub. But this may change in the future.
    EXPECT_FALSE(buffer->isDetachedBuffer());

    pdx::LocalChannelHandle channel{nullptr, 1234};
    EXPECT_TRUE(channel.valid());

    std::unique_ptr<DetachedBufferHandle> handle = DetachedBufferHandle::Create(std::move(channel));
    EXPECT_FALSE(channel.valid());
    EXPECT_TRUE(handle->isValid());
    EXPECT_TRUE(handle->handle().valid());

    buffer->setDetachedBufferHandle(std::move(handle));
    EXPECT_TRUE(handle == nullptr);
    EXPECT_TRUE(buffer->isDetachedBuffer());

    handle = buffer->takeDetachedBufferHandle();
    EXPECT_TRUE(handle != nullptr);
    EXPECT_TRUE(handle->isValid());
    EXPECT_FALSE(buffer->isDetachedBuffer());
}

} // namespace android
