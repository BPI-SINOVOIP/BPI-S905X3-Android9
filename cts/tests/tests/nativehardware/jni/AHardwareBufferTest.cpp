/*
 * Copyright (C) 2018 The Android Open Source Project
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

#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include <sstream>
#include <string>

#include <android/hardware_buffer.h>
#include <gtest/gtest.h>

//#define LOG_NDEBUG 0

#define BAD_VALUE -EINVAL
#define NO_ERROR 0

namespace {

#define FORMAT_CASE(x) case AHARDWAREBUFFER_FORMAT_ ## x: os << #x ; break

void PrintAhbFormat(std::ostream& os, uint64_t format) {
    switch (format) {
        FORMAT_CASE(R8G8B8A8_UNORM);
        FORMAT_CASE(R8G8B8X8_UNORM);
        FORMAT_CASE(R8G8B8_UNORM);
        FORMAT_CASE(R5G6B5_UNORM);
        FORMAT_CASE(R16G16B16A16_FLOAT);
        FORMAT_CASE(R10G10B10A2_UNORM);
        FORMAT_CASE(BLOB);
        FORMAT_CASE(D16_UNORM);
        FORMAT_CASE(D24_UNORM);
        FORMAT_CASE(D24_UNORM_S8_UINT);
        FORMAT_CASE(D32_FLOAT);
        FORMAT_CASE(D32_FLOAT_S8_UINT);
        FORMAT_CASE(S8_UINT);
        default: os << "unknown"; break;
    }
}

#define BITS_CASE(x) case AHARDWAREBUFFER_USAGE_ ## x: os << #x ; break
#define PRINT_FLAG(x) \
    do { if (usage & AHARDWAREBUFFER_USAGE_ ## x) { os << #x << " "; } } while (0)

void PrintAhbUsage(std::ostream& os, uint64_t usage) {
    if (usage == 0) {
        os << "none";
        return;
    }
    switch (usage & AHARDWAREBUFFER_USAGE_CPU_READ_MASK) {
        BITS_CASE(CPU_READ_NEVER);
        BITS_CASE(CPU_READ_RARELY);
        BITS_CASE(CPU_READ_OFTEN);
        default: break;
    }
    switch (usage & AHARDWAREBUFFER_USAGE_CPU_WRITE_MASK) {
        BITS_CASE(CPU_WRITE_NEVER);
        BITS_CASE(CPU_WRITE_RARELY);
        BITS_CASE(CPU_WRITE_OFTEN);
        default: break;
    }

    PRINT_FLAG(GPU_SAMPLED_IMAGE);
    PRINT_FLAG(GPU_COLOR_OUTPUT);
    PRINT_FLAG(PROTECTED_CONTENT);
    PRINT_FLAG(VIDEO_ENCODE);
    PRINT_FLAG(SENSOR_DIRECT_DATA);
    PRINT_FLAG(GPU_DATA_BUFFER);
    PRINT_FLAG(GPU_CUBE_MAP);
    PRINT_FLAG(GPU_MIPMAP_COMPLETE);

    PRINT_FLAG(VENDOR_0);
    PRINT_FLAG(VENDOR_1);
    PRINT_FLAG(VENDOR_2);
    PRINT_FLAG(VENDOR_3);
    PRINT_FLAG(VENDOR_4);
    PRINT_FLAG(VENDOR_5);
    PRINT_FLAG(VENDOR_6);
    PRINT_FLAG(VENDOR_7);
    PRINT_FLAG(VENDOR_8);
    PRINT_FLAG(VENDOR_9);
    PRINT_FLAG(VENDOR_10);
    PRINT_FLAG(VENDOR_11);
    PRINT_FLAG(VENDOR_12);
    PRINT_FLAG(VENDOR_13);
    PRINT_FLAG(VENDOR_14);
    PRINT_FLAG(VENDOR_15);
    PRINT_FLAG(VENDOR_16);
    PRINT_FLAG(VENDOR_17);
    PRINT_FLAG(VENDOR_18);
    PRINT_FLAG(VENDOR_19);
}

AHardwareBuffer_Desc GetDescription(AHardwareBuffer* buffer) {
    AHardwareBuffer_Desc description;
    AHardwareBuffer_describe(buffer, &description);
    return description;
}

}  // namespace

// GTest printer for AHardwareBuffer_Desc. Has to be in the global namespace.
void PrintTo(const AHardwareBuffer_Desc& desc, ::std::ostream* os) {
    *os << "AHardwareBuffer_Desc " << desc.width << "x" << desc.height;
    if (desc.layers > 1) {
        *os << ", " << desc.layers << " layers";
    }
    *os << ", usage = ";
    PrintAhbUsage(*os, desc.usage);
    *os << ", format = ";
    PrintAhbFormat(*os, desc.format);
}

// Equality operators for AHardwareBuffer_Desc. Have to be in the global namespace.
bool operator==(const AHardwareBuffer_Desc& a, const AHardwareBuffer_Desc& b) {
    return a.width == b.width && a.height == b.height && a.layers == b.layers &&
        a.usage == b.usage && a.format == b.format;
}
bool operator!=(const AHardwareBuffer_Desc& a, const AHardwareBuffer_Desc& b) {
    return !(a == b);
}

namespace android {

// Test that passing in NULL values to allocate works as expected.
TEST(AHardwareBufferTest, AllocateFailsWithNullInput) {
    AHardwareBuffer* buffer;
    AHardwareBuffer_Desc desc;

    memset(&desc, 0, sizeof(AHardwareBuffer_Desc));

    int res = AHardwareBuffer_allocate(&desc, NULL);
    EXPECT_EQ(BAD_VALUE, res);
    res = AHardwareBuffer_allocate(NULL, &buffer);
    EXPECT_EQ(BAD_VALUE, res);
    res = AHardwareBuffer_allocate(NULL, NULL);
    EXPECT_EQ(BAD_VALUE, res);
}

// Test that passing in NULL values to allocate works as expected.
TEST(AHardwareBufferTest, BlobFormatRequiresHeight1) {
    AHardwareBuffer* buffer;
    AHardwareBuffer_Desc desc = {};

    desc.width = 2;
    desc.height = 4;
    desc.layers = 1;
    desc.usage = AHARDWAREBUFFER_USAGE_CPU_READ_RARELY;
    desc.format = AHARDWAREBUFFER_FORMAT_BLOB;
    int res = AHardwareBuffer_allocate(&desc, &buffer);
    EXPECT_EQ(BAD_VALUE, res);

    desc.height = 1;
    res = AHardwareBuffer_allocate(&desc, &buffer);
    EXPECT_EQ(NO_ERROR, res);
    EXPECT_EQ(desc, GetDescription(buffer));
    AHardwareBuffer_release(buffer);
    buffer = NULL;
}

// Test that allocate can create an AHardwareBuffer correctly.
TEST(AHardwareBufferTest, AllocateSucceeds) {
    AHardwareBuffer* buffer = NULL;
    AHardwareBuffer_Desc desc = {};

    desc.width = 2;
    desc.height = 4;
    desc.layers = 1;
    desc.usage = AHARDWAREBUFFER_USAGE_GPU_SAMPLED_IMAGE;
    desc.format = AHARDWAREBUFFER_FORMAT_R8G8B8A8_UNORM;
    int res = AHardwareBuffer_allocate(&desc, &buffer);
    EXPECT_EQ(NO_ERROR, res);
    EXPECT_EQ(desc, GetDescription(buffer));
    AHardwareBuffer_release(buffer);
    buffer = NULL;

    desc.width = 4;
    desc.height = 12;
    desc.layers = 1;
    desc.usage = AHARDWAREBUFFER_USAGE_GPU_SAMPLED_IMAGE;
    desc.format = AHARDWAREBUFFER_FORMAT_R5G6B5_UNORM;
    res = AHardwareBuffer_allocate(&desc, &buffer);
    EXPECT_EQ(NO_ERROR, res);
    EXPECT_EQ(desc, GetDescription(buffer));
    AHardwareBuffer_release(buffer);
}

TEST(AHardwareBufferTest, DescribeSucceeds) {
    AHardwareBuffer* buffer = NULL;
    AHardwareBuffer_Desc desc = {};

    desc.width = 2;
    desc.height = 4;
    desc.layers = 1;
    desc.usage = AHARDWAREBUFFER_USAGE_GPU_SAMPLED_IMAGE;
    desc.format = AHARDWAREBUFFER_FORMAT_R8G8B8A8_UNORM;
    int res = AHardwareBuffer_allocate(&desc, &buffer);
    EXPECT_EQ(NO_ERROR, res);

    // Description of a null buffer should be all zeros.
    AHardwareBuffer_Desc scratch_desc;
    memset(&scratch_desc, 0, sizeof(AHardwareBuffer_Desc));
    AHardwareBuffer_describe(NULL, &scratch_desc);
    EXPECT_EQ(0U, scratch_desc.width);
    EXPECT_EQ(0U, scratch_desc.height);

    // This shouldn't crash.
    AHardwareBuffer_describe(buffer, NULL);

    // Description of created buffer should match requsted description.
    EXPECT_EQ(desc, GetDescription(buffer));
    AHardwareBuffer_release(buffer);
}

struct ClientData {
    int fd;
    AHardwareBuffer* buffer;
    ClientData(int fd_in, AHardwareBuffer* buffer_in)
            : fd(fd_in), buffer(buffer_in) {}
};

static void* clientFunction(void* data) {
    ClientData* pdata = reinterpret_cast<ClientData*>(data);
    int err = AHardwareBuffer_sendHandleToUnixSocket(pdata->buffer, pdata->fd);
    EXPECT_EQ(NO_ERROR, err);
    close(pdata->fd);
    return 0;
}

TEST(AHardwareBufferTest, SendAndRecvSucceeds) {
    AHardwareBuffer* buffer = NULL;
    AHardwareBuffer_Desc desc = {};

    desc.width = 2;
    desc.height = 4;
    desc.layers = 1;
    desc.usage = AHARDWAREBUFFER_USAGE_GPU_SAMPLED_IMAGE;
    desc.format = AHARDWAREBUFFER_FORMAT_R8G8B8A8_UNORM;

    // Test that an invalid buffer fails.
    int err = AHardwareBuffer_sendHandleToUnixSocket(NULL, 0);
    EXPECT_EQ(BAD_VALUE, err);
    err = 0;
    err = AHardwareBuffer_sendHandleToUnixSocket(buffer, 0);
    EXPECT_EQ(BAD_VALUE, err);

    // Allocate the buffer.
    err = AHardwareBuffer_allocate(&desc, &buffer);
    EXPECT_EQ(NO_ERROR, err);

    int fds[2];
    err = socketpair(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0, fds);

    // Launch a client that will send the buffer back.
    ClientData data(fds[1], buffer);
    pthread_t thread;
    EXPECT_EQ(0, pthread_create(&thread, NULL, clientFunction, &data));

    // Receive the buffer.
    err = AHardwareBuffer_recvHandleFromUnixSocket(fds[0], NULL);
    EXPECT_EQ(BAD_VALUE, err);

    AHardwareBuffer* received = NULL;
    err = AHardwareBuffer_recvHandleFromUnixSocket(fds[0], &received);
    EXPECT_EQ(NO_ERROR, err);
    EXPECT_TRUE(received != NULL);
    EXPECT_EQ(desc, GetDescription(received));

    void* ret_val;
    EXPECT_EQ(0, pthread_join(thread, &ret_val));
    EXPECT_EQ(NULL, ret_val);
    close(fds[0]);

    AHardwareBuffer_release(buffer);
    AHardwareBuffer_release(received);
}

TEST(AHardwareBufferTest, LockAndUnlockSucceed) {
    AHardwareBuffer* buffer = NULL;
    AHardwareBuffer_Desc desc = {};

    desc.width = 2;
    desc.height = 4;
    desc.layers = 1;
    desc.usage = AHARDWAREBUFFER_USAGE_GPU_SAMPLED_IMAGE | AHARDWAREBUFFER_USAGE_CPU_READ_RARELY;
    desc.format = AHARDWAREBUFFER_FORMAT_R8G8B8A8_UNORM;

    // Test that an invalid buffer fails.
    int err = AHardwareBuffer_lock(NULL, 0, -1, NULL, NULL);
    EXPECT_EQ(BAD_VALUE, err);
    err = 0;

    // Allocate the buffer.
    err = AHardwareBuffer_allocate(&desc, &buffer);
    EXPECT_EQ(NO_ERROR, err);
    void* bufferData = NULL;
    err = AHardwareBuffer_lock(buffer, AHARDWAREBUFFER_USAGE_CPU_READ_RARELY, -1,
          NULL, &bufferData);
    EXPECT_EQ(NO_ERROR, err);
    EXPECT_TRUE(bufferData != NULL);
    int32_t fence = -1;
    err = AHardwareBuffer_unlock(buffer, &fence);

    AHardwareBuffer_release(buffer);
}

TEST(AHardwareBufferTest, ProtectedContentAndCpuReadIncompatible) {
    AHardwareBuffer* buffer = NULL;
    AHardwareBuffer_Desc desc = {};
    desc.width = 120;
    desc.width = 240;
    desc.layers = 1;
    desc.usage = AHARDWAREBUFFER_USAGE_GPU_COLOR_OUTPUT | AHARDWAREBUFFER_USAGE_CPU_READ_OFTEN;
    desc.format = AHARDWAREBUFFER_FORMAT_R8G8B8A8_UNORM;

    // Allocation of a CPU-readable buffer should succeed...
    int err = AHardwareBuffer_allocate(&desc, &buffer);
    EXPECT_EQ(NO_ERROR, err);
    AHardwareBuffer_release(buffer);
    buffer = nullptr;

    // ...but not if it's a protected buffer.
    desc.usage =
        AHARDWAREBUFFER_USAGE_GPU_COLOR_OUTPUT |
        AHARDWAREBUFFER_USAGE_CPU_READ_OFTEN |
        AHARDWAREBUFFER_USAGE_PROTECTED_CONTENT;
    err = AHardwareBuffer_allocate(&desc, &buffer);
    EXPECT_NE(NO_ERROR, err);

    desc.usage =
        AHARDWAREBUFFER_USAGE_GPU_COLOR_OUTPUT |
        AHARDWAREBUFFER_USAGE_CPU_READ_RARELY |
        AHARDWAREBUFFER_USAGE_PROTECTED_CONTENT;
    err = AHardwareBuffer_allocate(&desc, &buffer);
    EXPECT_NE(NO_ERROR, err);
}

} // namespace android
