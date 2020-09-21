/*
 * Copyright (C) 2017 The Android Open Source Project
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

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <jni.h>
#include <stdlib.h>
#include <android/hardware_buffer.h>
#include <android/log.h>
#include <cmath>
#include <string>
#include <sstream>

#define  LOG_TAG    "VrExtensionsJni"
#define  LOGV(...)  __android_log_print(ANDROID_LOG_VERBOSE,LOG_TAG,__VA_ARGS__)

using PFNEGLGETNATIVECLIENTBUFFERANDROID =
        EGLClientBuffer(EGLAPIENTRYP)(const AHardwareBuffer* buffer);

using PFNGLEGLIMAGETARGETTEXTURE2DOESPROC = void(GL_APIENTRYP)(GLenum target,
                                                               void* image);

using PFNGLBUFFERSTORAGEEXTERNALEXTPROC =
    void(GL_APIENTRYP)(GLenum target, GLintptr offset, GLsizeiptr size,
                       void* clientBuffer, GLbitfield flags);

using PFNGLMAPBUFFERRANGEPROC = void*(GL_APIENTRYP)(GLenum target,
                                                    GLintptr offset,
                                                    GLsizeiptr length,
                                                    GLbitfield access);

using PFNGLUNMAPBUFFERPROC = void*(GL_APIENTRYP)(GLenum target);

PFNGLEGLIMAGETARGETTEXTURE2DOESPROC glEGLImageTargetTexture2DOES;
PFNEGLGETNATIVECLIENTBUFFERANDROID eglGetNativeClientBufferANDROID;
PFNEGLCREATEIMAGEKHRPROC eglCreateImageKHR;
PFNGLFRAMEBUFFERTEXTUREMULTIVIEWOVRPROC glFramebufferTextureMultiviewOVR;
PFNGLFRAMEBUFFERTEXTUREMULTISAMPLEMULTIVIEWOVRPROC
    glFramebufferTextureMultisampleMultiviewOVR;
PFNGLBUFFERSTORAGEEXTERNALEXTPROC glBufferStorageExternalEXT;
PFNGLMAPBUFFERRANGEPROC glMapBufferRange;
PFNGLUNMAPBUFFERPROC glUnmapBuffer;

#define NO_ERROR 0
#define GL_UNIFORM_BUFFER         0x8A11

// Declare flags that are added to MapBufferRange via EXT_buffer_storage.
// https://www.khronos.org/registry/OpenGL/extensions/EXT/EXT_buffer_storage.txt
#define GL_MAP_PERSISTENT_BIT_EXT 0x0040
#define GL_MAP_COHERENT_BIT_EXT   0x0080

// Declare tokens added as a part of EGL_EXT_image_gl_colorspace.
#define EGL_GL_COLORSPACE_DEFAULT_EXT 0x314D

#define LOAD_PROC(NAME, TYPE)                                           \
    NAME = reinterpret_cast<TYPE>(eglGetProcAddress(# NAME))

#define ASSERT(condition, format, args...)      \
    if (!(condition)) {                         \
        fail(env, format, ## args);             \
        return;                                 \
    }

#define ASSERT_TRUE(a) \
    ASSERT((a), "assert failed on (" #a ") at " __FILE__ ":%d", __LINE__)
#define ASSERT_FALSE(a) \
    ASSERT(!(a), "assert failed on (!" #a ") at " __FILE__ ":%d", __LINE__)
#define ASSERT_EQ(a, b) \
    ASSERT((a) == (b), "assert failed on (" #a ") at " __FILE__ ":%d", __LINE__)
#define ASSERT_NE(a, b) \
    ASSERT((a) != (b), "assert failed on (" #a ") at " __FILE__ ":%d", __LINE__)
#define ASSERT_GT(a, b) \
    ASSERT((a) > (b), "assert failed on (" #a ") at " __FILE__ ":%d", __LINE__)
#define ASSERT_NEAR(a, b, delta)                     \
    ASSERT((a - delta) <= (b) && (b) <= (a + delta), \
           "assert failed on (" #a ") at " __FILE__ ":%d", __LINE__)

void fail(JNIEnv* env, const char* format, ...) {
    va_list args;
    va_start(args, format);
    char* msg;
    vasprintf(&msg, format, args);
    va_end(args);
    jclass exClass;
    const char* className = "java/lang/AssertionError";
    exClass = env->FindClass(className);
    env->ThrowNew(exClass, msg);
    free(msg);
}

static void testEglImageArray(JNIEnv* env, AHardwareBuffer_Desc desc,
                              int nsamples) {
    ASSERT_GT(desc.layers, 1);
    AHardwareBuffer* hwbuffer = nullptr;
    int error = AHardwareBuffer_allocate(&desc, &hwbuffer);
    ASSERT_FALSE(error);
    // Create EGLClientBuffer from the AHardwareBuffer.
    EGLClientBuffer native_buffer = eglGetNativeClientBufferANDROID(hwbuffer);
    ASSERT_TRUE(native_buffer);
    // Create EGLImage from EGLClientBuffer.
    EGLint attrs[] = {EGL_NONE};
    EGLImageKHR image =
        eglCreateImageKHR(eglGetCurrentDisplay(), EGL_NO_CONTEXT,
                          EGL_NATIVE_BUFFER_ANDROID, native_buffer, attrs);
    ASSERT_TRUE(image);
    // Create OpenGL texture from the EGLImage.
    GLuint texid;
    glGenTextures(1, &texid);
    glBindTexture(GL_TEXTURE_2D_ARRAY, texid);
    glEGLImageTargetTexture2DOES(GL_TEXTURE_2D_ARRAY, image);
    ASSERT_EQ(glGetError(), GL_NO_ERROR);
    // Create FBO and add multiview attachment.
    GLuint fboid;
    glGenFramebuffers(1, &fboid);
    glBindFramebuffer(GL_FRAMEBUFFER, fboid);
    const GLint miplevel = 0;
    const GLint base_view = 0;
    const GLint num_views = desc.layers;
    if (nsamples == 1) {
        glFramebufferTextureMultiviewOVR(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                         texid, miplevel, base_view, num_views);
    } else {
        glFramebufferTextureMultisampleMultiviewOVR(
            GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, texid, miplevel, nsamples,
            base_view, num_views);
    }
    ASSERT_EQ(glGetError(), GL_NO_ERROR);
    ASSERT_EQ(glCheckFramebufferStatus(GL_FRAMEBUFFER),
              GL_FRAMEBUFFER_COMPLETE);
    // Release memory.
    glDeleteTextures(1, &texid);
    glDeleteFramebuffers(1, &fboid);
    AHardwareBuffer_release(hwbuffer);
}

extern "C" JNIEXPORT void JNICALL
Java_android_vr_cts_VrExtensionBehaviorTest_nativeTestEglImageArray(
    JNIEnv* env, jclass /* unused */) {
    // First, load entry points provided by extensions.
    LOAD_PROC(glEGLImageTargetTexture2DOES,
              PFNGLEGLIMAGETARGETTEXTURE2DOESPROC);
    ASSERT_NE(glEGLImageTargetTexture2DOES, nullptr);
    LOAD_PROC(eglGetNativeClientBufferANDROID,
              PFNEGLGETNATIVECLIENTBUFFERANDROID);
    ASSERT_NE(eglGetNativeClientBufferANDROID, nullptr);
    LOAD_PROC(eglCreateImageKHR, PFNEGLCREATEIMAGEKHRPROC);
    ASSERT_NE(eglCreateImageKHR, nullptr);
    LOAD_PROC(glFramebufferTextureMultiviewOVR,
              PFNGLFRAMEBUFFERTEXTUREMULTIVIEWOVRPROC);
    ASSERT_NE(glFramebufferTextureMultiviewOVR, nullptr);
    LOAD_PROC(glFramebufferTextureMultisampleMultiviewOVR,
              PFNGLFRAMEBUFFERTEXTUREMULTISAMPLEMULTIVIEWOVRPROC);
    ASSERT_NE(glFramebufferTextureMultisampleMultiviewOVR, nullptr);
    // Try creating a 32x32 AHardwareBuffer and attaching it to a multiview
    // framebuffer, with various formats and depths.
    AHardwareBuffer_Desc desc = {};
    desc.width = 32;
    desc.height = 32;
    desc.usage = AHARDWAREBUFFER_USAGE_GPU_SAMPLED_IMAGE |
                 AHARDWAREBUFFER_USAGE_GPU_COLOR_OUTPUT;
    const int layers[] = {2, 4};
    const int formats[] = {
      AHARDWAREBUFFER_FORMAT_R5G6B5_UNORM,
      AHARDWAREBUFFER_FORMAT_R8G8B8A8_UNORM,
      AHARDWAREBUFFER_FORMAT_R10G10B10A2_UNORM,
      AHARDWAREBUFFER_FORMAT_R16G16B16A16_FLOAT,
      // Do not test AHARDWAREBUFFER_FORMAT_BLOB, it isn't color-renderable.
    };
    const int samples[] = {1, 2, 4};
    for (int nsamples : samples) {
      for (auto nlayers : layers) {
        for (auto format : formats) {
          desc.layers = nlayers;
          desc.format = format;
          testEglImageArray(env, desc, nsamples);
        }
      }
    }
}

static void testExternalBuffer(JNIEnv* env, uint64_t usage, bool write_hwbuffer,
                               const std::string& test_string) {
    // Create a blob AHardwareBuffer suitable for holding the string.
    AHardwareBuffer_Desc desc = {};
    desc.width = test_string.size();
    desc.height = 1;
    desc.layers = 1;
    desc.format = AHARDWAREBUFFER_FORMAT_BLOB;
    desc.usage = usage;
    AHardwareBuffer* hwbuffer = nullptr;
    int error = AHardwareBuffer_allocate(&desc, &hwbuffer);
    ASSERT_EQ(error, NO_ERROR);
    // Create EGLClientBuffer from the AHardwareBuffer.
    EGLClientBuffer native_buffer = eglGetNativeClientBufferANDROID(hwbuffer);
    ASSERT_TRUE(native_buffer);
    // Create uniform buffer from EGLClientBuffer.
    const GLbitfield flags = GL_MAP_READ_BIT | GL_MAP_WRITE_BIT |
        GL_MAP_COHERENT_BIT_EXT | GL_MAP_PERSISTENT_BIT_EXT;
    GLuint buf = 0;
    glGenBuffers(1, &buf);
    glBindBuffer(GL_UNIFORM_BUFFER, buf);
    ASSERT_EQ(glGetError(), GL_NO_ERROR);
    const GLsizeiptr bufsize = desc.width * desc.height;
    glBufferStorageExternalEXT(GL_UNIFORM_BUFFER, 0,
             bufsize, native_buffer, flags);
    ASSERT_EQ(glGetError(), GL_NO_ERROR);
    // Obtain a writeable pointer using either OpenGL or the Android API,
    // then copy the test string into it.
    if (write_hwbuffer) {
      void* data = nullptr;
      error = AHardwareBuffer_lock(hwbuffer,
                                   AHARDWAREBUFFER_USAGE_CPU_READ_RARELY, -1,
                                   NULL, &data);
      ASSERT_EQ(error, NO_ERROR);
      ASSERT_TRUE(data);
      memcpy(data, test_string.c_str(), test_string.size());
      error = AHardwareBuffer_unlock(hwbuffer, nullptr);
      ASSERT_EQ(error, NO_ERROR);
    } else {
      void* data =
          glMapBufferRange(GL_UNIFORM_BUFFER, 0, bufsize,
                           GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT_EXT);
      ASSERT_EQ(glGetError(), GL_NO_ERROR);
      ASSERT_TRUE(data);
      memcpy(data, test_string.c_str(), test_string.size());
      glUnmapBuffer(GL_UNIFORM_BUFFER);
      ASSERT_EQ(glGetError(), GL_NO_ERROR);
    }
    // Obtain a readable pointer and verify the data.
    void* data = glMapBufferRange(GL_UNIFORM_BUFFER, 0, bufsize, GL_MAP_READ_BIT);
    ASSERT_TRUE(data);
    ASSERT_EQ(strncmp(static_cast<char*>(data), test_string.c_str(),
                      test_string.size()), 0);
    glUnmapBuffer(GL_UNIFORM_BUFFER);
    ASSERT_EQ(glGetError(), GL_NO_ERROR);
    AHardwareBuffer_release(hwbuffer);
}

extern "C" JNIEXPORT void JNICALL
Java_android_vr_cts_VrExtensionBehaviorTest_nativeTestExternalBuffer(
    JNIEnv* env, jclass /* unused */) {
    // First, check for EXT_external_buffer in the extension string.
    auto exts = reinterpret_cast<const char*>(glGetString(GL_EXTENSIONS));
    ASSERT_TRUE(exts && strstr(exts, "GL_EXT_external_buffer"));
    // Next, load entry points provided by extensions.
    LOAD_PROC(eglGetNativeClientBufferANDROID, PFNEGLGETNATIVECLIENTBUFFERANDROID);
    ASSERT_NE(eglGetNativeClientBufferANDROID, nullptr);
    LOAD_PROC(glBufferStorageExternalEXT, PFNGLBUFFERSTORAGEEXTERNALEXTPROC);
    ASSERT_NE(glBufferStorageExternalEXT, nullptr);
    LOAD_PROC(glMapBufferRange, PFNGLMAPBUFFERRANGEPROC);
    ASSERT_NE(glMapBufferRange, nullptr);
    LOAD_PROC(glUnmapBuffer, PFNGLUNMAPBUFFERPROC);
    ASSERT_NE(glUnmapBuffer, nullptr);
    const uint64_t usage = AHARDWAREBUFFER_USAGE_CPU_WRITE_OFTEN |
        AHARDWAREBUFFER_USAGE_CPU_READ_RARELY |
        AHARDWAREBUFFER_USAGE_GPU_DATA_BUFFER |
        AHARDWAREBUFFER_USAGE_SENSOR_DIRECT_DATA;
    const std::string test_string = "Hello, world.";
    // First try writing to the buffer using OpenGL, then try writing to it via
    // the AHardwareBuffer API.
    testExternalBuffer(env, usage, false, test_string);
    testExternalBuffer(env, usage, true, test_string);
}

const GLchar* const kSrgbVertexCode = R"(
    // vertex position in clip space (-1..1)
    attribute vec4 position;
    varying mediump vec2 uv;
    void main() {
      gl_Position = position;
      uv = vec2(0.5 * (position.x + 1.0), 0.5);
    })";

const GLchar* const kSrgbFragmentCode = R"(
    varying mediump vec2 uv;
    uniform sampler2D tex;
    void main() {
      gl_FragColor = texture2D(tex, uv);
    })";

static inline float SrgbChannelToLinear(float cs) {
    if (cs <= 0.04045)
        return cs / 12.92f;
    else
        return std::pow((cs + 0.055f) / 1.055f, 2.4f);
}

static inline float LinearChannelToSrgb(float cs) {
    if (cs <= 0.0f)
        return 0.0f;
    else if (cs < 0.0031308f)
        return 12.92f * cs;
    else if (cs < 1.0f)
        return 1.055f * std::pow(cs, 0.41666f) - 0.055f;
    else
        return 1.0f;
}

static uint32_t SrgbColorToLinear(uint32_t color) {
    float r = SrgbChannelToLinear((color & 0xff) / 255.0f);
    float g = SrgbChannelToLinear(((color >> 8) & 0xff) / 255.0f);
    float b = SrgbChannelToLinear(((color >> 16) & 0xff) / 255.0f);
    uint32_t r8 = r * 255.0f;
    uint32_t g8 = g * 255.0f;
    uint32_t b8 = b * 255.0f;
    uint32_t a8 = color >> 24;
    return (a8 << 24) | (b8 << 16) | (g8 << 8) | r8;
}

static uint32_t LinearColorToSrgb(uint32_t color) {
    float r = LinearChannelToSrgb((color & 0xff) / 255.0f);
    float g = LinearChannelToSrgb(((color >> 8) & 0xff) / 255.0f);
    float b = LinearChannelToSrgb(((color >> 16) & 0xff) / 255.0f);
    uint32_t r8 = r * 255.0f;
    uint32_t g8 = g * 255.0f;
    uint32_t b8 = b * 255.0f;
    uint32_t a8 = color >> 24;
    return (a8 << 24) | (b8 << 16) | (g8 << 8) | r8;
}

static uint32_t LerpColor(uint32_t color0, uint32_t color1, float t) {
    float r0 = (color0 & 0xff) / 255.0f;
    float g0 = ((color0 >> 8) & 0xff) / 255.0f;
    float b0 = ((color0 >> 16) & 0xff) / 255.0f;
    float a0 = ((color0 >> 24) & 0xff) / 255.0f;
    float r1 = (color1 & 0xff) / 255.0f;
    float g1 = ((color1 >> 8) & 0xff) / 255.0f;
    float b1 = ((color1 >> 16) & 0xff) / 255.0f;
    float a1 = ((color1 >> 24) & 0xff) / 255.0f;
    uint32_t r8 = (r0 * (1.0f - t) + r1 * t) * 255.0f;
    uint32_t g8 = (g0 * (1.0f - t) + g1 * t) * 255.0f;
    uint32_t b8 = (b0 * (1.0f - t) + b1 * t) * 255.0f;
    uint32_t a8 = (a0 * (1.0f - t) + a1 * t) * 255.0f;
    return (a8 << 24) | (b8 << 16) | (g8 << 8) | r8;
}

// Choose an odd-numbered framebuffer width so that we can
// extract the middle pixel of a gradient.
constexpr uint32_t kFramebufferWidth = 31;

// Declare the pixel data for the 2x1 texture.
// Color components are ordered like this: AABBGGRR
constexpr uint32_t kTextureData[] = {
    0xff800000,  // Half-Blue
    0xff000080,  // Half-Red
};
constexpr uint32_t kTextureWidth = sizeof(kTextureData) / sizeof(kTextureData[0]);

// Declare expected values for the middle pixel for various sampling behaviors.
const uint32_t kExpectedMiddlePixel_NoSrgb = LerpColor(kTextureData[0], kTextureData[1], 0.5f);
const uint32_t kExpectedMiddlePixel_LinearizeAfterFiltering =
    SrgbColorToLinear(kExpectedMiddlePixel_NoSrgb);
const uint32_t kExpectedMiddlePixel_LinearizeBeforeFiltering =
    LerpColor(SrgbColorToLinear(kTextureData[0]), SrgbColorToLinear(kTextureData[1]), 0.5f);

// Declare expected values for the final pixel color for various blending behaviors.
constexpr uint32_t kBlendDestColor = 0xff000080;
constexpr uint32_t kBlendSourceColor = 0x80800000;
const uint32_t kExpectedBlendedPixel_NoSrgb = LerpColor(kBlendSourceColor, kBlendDestColor, 0.5f);
const uint32_t kExpectedBlendedPixel_Srgb =
    LinearColorToSrgb(LerpColor(kBlendSourceColor, SrgbColorToLinear(kBlendDestColor), 0.5f));

// Define a set of test flags. Not using an enum to avoid lots of casts.
namespace SrgbFlag {
constexpr uint32_t kHardwareBuffer = 1 << 0;
constexpr uint32_t kSrgbFormat = 1 << 1;
constexpr uint32_t kEglColorspaceDefault = 1 << 2;
constexpr uint32_t kEglColorspaceLinear = 1 << 3;
constexpr uint32_t kEglColorspaceSrgb = 1 << 4;
}  // namespace SrgbFlag

static void configureEglColorspace(EGLint attrs[4], uint32_t srgb_flags) {
    if (srgb_flags & SrgbFlag::kEglColorspaceDefault) {
        attrs[0] = EGL_GL_COLORSPACE_KHR;
        attrs[1] = EGL_GL_COLORSPACE_DEFAULT_EXT;
    } else if (srgb_flags & SrgbFlag::kEglColorspaceLinear) {
        attrs[0] = EGL_GL_COLORSPACE_KHR;
        attrs[1] = EGL_GL_COLORSPACE_LINEAR_KHR;
    } else if (srgb_flags & SrgbFlag::kEglColorspaceSrgb) {
        attrs[0] = EGL_GL_COLORSPACE_KHR;
        attrs[1] = EGL_GL_COLORSPACE_SRGB_KHR;
    } else {
        attrs[0] = EGL_NONE;
        attrs[1] = EGL_NONE;
    }
    attrs[2] = EGL_NONE;
    attrs[3] = EGL_NONE;
}

static void printSrgbFlags(std::ostream& out, uint32_t srgb_flags) {
    if (srgb_flags & SrgbFlag::kHardwareBuffer) {
        out << " AHardwareBuffer";
    }
    if (srgb_flags & SrgbFlag::kSrgbFormat) {
        out << " GL_SRGB_ALPHA";
    }
    if (srgb_flags & SrgbFlag::kEglColorspaceDefault) {
        out << " EGL_GL_COLORSPACE_DEFAULT_KHR";
    }
    if (srgb_flags & SrgbFlag::kEglColorspaceLinear) {
        out << " EGL_GL_COLORSPACE_LINEAR_KHR";
    }
    if (srgb_flags & SrgbFlag::kEglColorspaceSrgb) {
        out << " EGL_GL_COLORSPACE_SRGB_KHR";
    }
}

// Draws a gradient and extracts the middle pixel. Returns void to allow ASSERT to work.
static void testLinearMagnification(JNIEnv* env, uint32_t flags, uint32_t* middle_pixel) {
    const bool use_hwbuffer = flags & SrgbFlag::kHardwareBuffer;
    const bool use_srgb_format = flags & SrgbFlag::kSrgbFormat;
    GLuint srgbtex;
    glGenTextures(1, &srgbtex);
    glBindTexture(GL_TEXTURE_2D, srgbtex);
    if (use_hwbuffer) {
        // Create a one-dimensional AHardwareBuffer.
        AHardwareBuffer_Desc desc = {};
        desc.width = kTextureWidth;
        desc.height = 1;
        desc.layers = 1;
        desc.format = AHARDWAREBUFFER_FORMAT_R8G8B8A8_UNORM;
        desc.usage =
                AHARDWAREBUFFER_USAGE_GPU_SAMPLED_IMAGE | AHARDWAREBUFFER_USAGE_GPU_COLOR_OUTPUT;
        AHardwareBuffer* hwbuffer = nullptr;
        int error = AHardwareBuffer_allocate(&desc, &hwbuffer);
        ASSERT_EQ(error, NO_ERROR);
        // Populate the pixels.
        uint32_t* pixels = nullptr;
        error = AHardwareBuffer_lock(hwbuffer, AHARDWAREBUFFER_USAGE_CPU_WRITE_OFTEN, -1, nullptr,
                                     reinterpret_cast<void**>(&pixels));
        ASSERT_EQ(error, NO_ERROR);
        ASSERT_TRUE(pixels);
        memcpy(pixels, kTextureData, sizeof(kTextureData));
        error = AHardwareBuffer_unlock(hwbuffer, nullptr);
        ASSERT_EQ(error, NO_ERROR);
        // Create EGLClientBuffer from the AHardwareBuffer.
        EGLClientBuffer native_buffer = eglGetNativeClientBufferANDROID(hwbuffer);
        ASSERT_TRUE(native_buffer);
        // Create EGLImage from EGLClientBuffer.
        EGLint attrs[4];
        configureEglColorspace(attrs, flags);
        EGLImageKHR image = eglCreateImageKHR(eglGetCurrentDisplay(), EGL_NO_CONTEXT,
                                              EGL_NATIVE_BUFFER_ANDROID, native_buffer, attrs);
        ASSERT_TRUE(image);
        // Allocate the OpenGL texture using the EGLImage.
        glEGLImageTargetTexture2DOES(GL_TEXTURE_2D, image);
    } else {
        GLenum internal_format = use_srgb_format ? GL_SRGB8_ALPHA8_EXT : GL_RGBA8_OES;
        GLenum format = use_srgb_format ? GL_SRGB_ALPHA_EXT : GL_RGBA;
        glTexImage2D(GL_TEXTURE_2D, 0, internal_format, kTextureWidth, 1, 0, format,
                     GL_UNSIGNED_BYTE, kTextureData);
    }
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    ASSERT_EQ(glGetError(), GL_NO_ERROR);
    // Clear to an interesting constant color to make it easier to spot bugs.
    glClearColor(1.0, 0.0, 0.5, 0.25);
    glClear(GL_COLOR_BUFFER_BIT);
    // Draw the texture.
    const float kTriangleCoords[] = {-1, -1, -1, 1, 1, -1, 1, 1};
    glBindTexture(GL_TEXTURE_2D, srgbtex);
    const int kPositionSlot = 0;
    glVertexAttribPointer(kPositionSlot, 2, GL_FLOAT, false, 0, kTriangleCoords);
    glEnableVertexAttribArray(kPositionSlot);
    glViewport(0, 0, kFramebufferWidth, 1);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    // Read back the framebuffer.
    glReadPixels(kFramebufferWidth / 2, 0, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, middle_pixel);
    std::ostringstream flag_string;
    printSrgbFlags(flag_string, flags);
    LOGV("Filtered Result: %8.8X Flags =%s", *middle_pixel, flag_string.str().c_str());
    ASSERT_EQ(glGetError(), GL_NO_ERROR);
}

// Blends a color into an (optionally) sRGB-encoded framebuffer and extracts the final color.
// Returns void to allow ASSERT to work.
static void testFramebufferBlending(JNIEnv* env, uint32_t flags, uint32_t* final_color) {
    const bool use_hwbuffer = flags & SrgbFlag::kHardwareBuffer;
    const bool use_srgb_format = flags & SrgbFlag::kSrgbFormat;
    const bool override_egl_colorspace = use_hwbuffer && (flags & SrgbFlag::kEglColorspaceSrgb);
    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    // Create a 1x1 half-blue, half-opaque texture.
    const uint32_t kTextureData[] = {
      kBlendSourceColor,
    };
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA,
                 GL_UNSIGNED_BYTE, kTextureData);
    // Create 1x1 framebuffer object.
    GLuint fbo;
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    GLuint fbotex;
    glGenTextures(1, &fbotex);
    glBindTexture(GL_TEXTURE_2D, fbotex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    if (use_hwbuffer) {
        AHardwareBuffer_Desc desc = {};
        desc.width = 1;
        desc.height = 1;
        desc.layers = 1;
        desc.format = AHARDWAREBUFFER_FORMAT_R8G8B8A8_UNORM;
        desc.usage =
                AHARDWAREBUFFER_USAGE_GPU_SAMPLED_IMAGE | AHARDWAREBUFFER_USAGE_GPU_COLOR_OUTPUT;
        AHardwareBuffer* hwbuffer = nullptr;
        int error = AHardwareBuffer_allocate(&desc, &hwbuffer);
        ASSERT_EQ(error, NO_ERROR);
        // Create EGLClientBuffer from the AHardwareBuffer.
        EGLClientBuffer native_buffer = eglGetNativeClientBufferANDROID(hwbuffer);
        ASSERT_TRUE(native_buffer);
        // Create EGLImage from EGLClientBuffer.
        EGLint attrs[4];
        configureEglColorspace(attrs, flags);
        EGLImageKHR image = eglCreateImageKHR(eglGetCurrentDisplay(), EGL_NO_CONTEXT,
                                              EGL_NATIVE_BUFFER_ANDROID, native_buffer, attrs);
        ASSERT_TRUE(image);
        // Allocate the OpenGL texture using the EGLImage.
        glEGLImageTargetTexture2DOES(GL_TEXTURE_2D, image);
    } else {
        GLenum internal_format = use_srgb_format ? GL_SRGB8_ALPHA8_EXT : GL_RGBA8_OES;
        GLenum format = use_srgb_format ? GL_SRGB_ALPHA_EXT : GL_RGBA;
        glTexImage2D(GL_TEXTURE_2D, 0, internal_format, 1, 1, 0, format,
                     GL_UNSIGNED_BYTE, nullptr);
    }
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                           GL_TEXTURE_2D, fbotex, 0);
    ASSERT_EQ(glCheckFramebufferStatus(GL_FRAMEBUFFER), GL_FRAMEBUFFER_COMPLETE);
    ASSERT_EQ(glGetError(), GL_NO_ERROR);
    // Clear to half-red.
    if (use_srgb_format || override_egl_colorspace) {
        glClearColor(SrgbChannelToLinear(0.5), 0.0, 0.0, 1.0);
    } else {
        glClearColor(0.5, 0.0, 0.0, 1.0);
    }
    glClear(GL_COLOR_BUFFER_BIT);
    // Sanity check the cleared color.
    uint32_t cleared_color = 0;
    glReadPixels(0, 0, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, &cleared_color);
    LOGV("  Cleared Color: %8.8X", cleared_color);
    ASSERT_EQ(cleared_color, kBlendDestColor);
    // Draw the texture.
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    const float kTriangleCoords[] = {-1, -1, -1, 1, 1, -1, 1, 1};
    glBindTexture(GL_TEXTURE_2D, tex);
    const int kPositionSlot = 0;
    glVertexAttribPointer(kPositionSlot, 2, GL_FLOAT, false, 0, kTriangleCoords);
    glEnableVertexAttribArray(kPositionSlot);
    glViewport(0, 0, 1, 1);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    // Read back the framebuffer.
    glReadPixels(0, 0, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, final_color);
    std::ostringstream flag_string;
    printSrgbFlags(flag_string, flags);
    LOGV("Blending Result: %8.8X Flags =%s", *final_color, flag_string.str().c_str());
    ASSERT_EQ(glGetError(), GL_NO_ERROR);
}

extern "C" JNIEXPORT void JNICALL
Java_android_vr_cts_VrExtensionBehaviorTest_nativeTestSrgbBuffer(
    JNIEnv* env, jclass /* unused */) {
    // First, check the published extension strings against expectations.
    const char *egl_exts =
        eglQueryString(eglGetCurrentDisplay(), EGL_EXTENSIONS);
    LOGV("EGL Extensions: %s", egl_exts);
    ASSERT_TRUE(egl_exts);
    bool egl_colorspace_supported = strstr(egl_exts, "EGL_EXT_image_gl_colorspace");
    auto gl_exts = reinterpret_cast<const char*>(glGetString(GL_EXTENSIONS));
    LOGV("OpenGL Extensions: %s", gl_exts);
    ASSERT_TRUE(gl_exts);
    // Load ancillary entry points provided by extensions.
    LOAD_PROC(eglGetNativeClientBufferANDROID,
              PFNEGLGETNATIVECLIENTBUFFERANDROID);
    ASSERT_NE(eglGetNativeClientBufferANDROID, nullptr);
    LOAD_PROC(eglCreateImageKHR, PFNEGLCREATEIMAGEKHRPROC);
    ASSERT_NE(eglCreateImageKHR, nullptr);
    LOAD_PROC(glEGLImageTargetTexture2DOES,
              PFNGLEGLIMAGETARGETTEXTURE2DOESPROC);
    ASSERT_NE(glEGLImageTargetTexture2DOES, nullptr);
    // Create a plain old one-dimensional FBO to render to.
    GLuint fbo;
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    GLuint fbotex;
    glGenTextures(1, &fbotex);
    glBindTexture(GL_TEXTURE_2D, fbotex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, kFramebufferWidth, 1, 0, GL_RGBA,
                 GL_UNSIGNED_BYTE, nullptr);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                           GL_TEXTURE_2D, fbotex, 0);
    ASSERT_EQ(glCheckFramebufferStatus(GL_FRAMEBUFFER), GL_FRAMEBUFFER_COMPLETE);
    ASSERT_EQ(glGetError(), GL_NO_ERROR);
    // Compile and link shaders.
    int program = glCreateProgram();
    int vshader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vshader, 1, &kSrgbVertexCode, nullptr);
    glCompileShader(vshader);
    glAttachShader(program, vshader);
    int fshader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fshader, 1, &kSrgbFragmentCode, nullptr);
    glCompileShader(fshader);
    glAttachShader(program, fshader);
    glLinkProgram(program);
    int status;
    glGetProgramiv(program, GL_LINK_STATUS, &status);
    ASSERT_EQ(status, GL_TRUE);
    glUseProgram(program);
    ASSERT_EQ(glGetError(), GL_NO_ERROR);

    // Filtering test.
    LOGV("Expected value for NoSrgb = %8.8X", kExpectedMiddlePixel_NoSrgb);
    LOGV("Expected value for   Srgb = %8.8X", kExpectedMiddlePixel_LinearizeBeforeFiltering);
    uint32_t middle_pixel;
    // First do a sanity check with plain old pre-linearized textures.
    testLinearMagnification(env, 0, &middle_pixel);
    ASSERT_NEAR(middle_pixel, kExpectedMiddlePixel_NoSrgb, 1);
    testLinearMagnification(env, SrgbFlag::kHardwareBuffer, &middle_pixel);
    ASSERT_NEAR(middle_pixel, kExpectedMiddlePixel_NoSrgb, 1);
    // Try a "normally allocated" OpenGL texture with an sRGB source format.
    testLinearMagnification(env, SrgbFlag::kSrgbFormat, &middle_pixel);
    ASSERT_NEAR(middle_pixel, kExpectedMiddlePixel_LinearizeBeforeFiltering, 1);
    // Try EGL_EXT_image_gl_colorspace.
    if (egl_colorspace_supported) {
        testLinearMagnification(env, SrgbFlag::kHardwareBuffer | SrgbFlag::kEglColorspaceDefault, &middle_pixel);
        ASSERT_NEAR(middle_pixel, kExpectedMiddlePixel_NoSrgb, 1);
        testLinearMagnification(env, SrgbFlag::kHardwareBuffer | SrgbFlag::kEglColorspaceLinear, &middle_pixel);
        ASSERT_NEAR(middle_pixel, kExpectedMiddlePixel_NoSrgb, 1);
        testLinearMagnification(env, SrgbFlag::kHardwareBuffer | SrgbFlag::kEglColorspaceSrgb, &middle_pixel);
        ASSERT_NEAR(middle_pixel, kExpectedMiddlePixel_LinearizeBeforeFiltering, 1);
    }

    // Blending test.
    LOGV("Expected value for NoSrgb = %8.8X", kExpectedBlendedPixel_NoSrgb);
    LOGV("Expected value for   Srgb = %8.8X", kExpectedBlendedPixel_Srgb);
    uint32_t final_color;
    // First do a sanity check with plain old pre-linearized textures.
    testFramebufferBlending(env, 0, &final_color);
    ASSERT_NEAR(final_color, kExpectedBlendedPixel_NoSrgb, 1);
    testFramebufferBlending(env, SrgbFlag::kHardwareBuffer, &final_color);
    ASSERT_NEAR(final_color, kExpectedBlendedPixel_NoSrgb, 1);
    // Try a "normally allocated" OpenGL texture with an sRGB source format.
    testFramebufferBlending(env, SrgbFlag::kSrgbFormat, &final_color);
    ASSERT_NEAR(final_color, kExpectedBlendedPixel_Srgb, 1);
    // Try EGL_EXT_image_gl_colorspace.
    if (egl_colorspace_supported) {
        testFramebufferBlending(env, SrgbFlag::kHardwareBuffer | SrgbFlag::kEglColorspaceDefault, &final_color);
        ASSERT_NEAR(final_color, kExpectedBlendedPixel_NoSrgb, 1);
        testFramebufferBlending(env, SrgbFlag::kHardwareBuffer | SrgbFlag::kEglColorspaceLinear, &final_color);
        ASSERT_NEAR(final_color, kExpectedBlendedPixel_NoSrgb, 1);
        testFramebufferBlending(env, SrgbFlag::kHardwareBuffer | SrgbFlag::kEglColorspaceSrgb, &final_color);
        ASSERT_NEAR(final_color, kExpectedBlendedPixel_Srgb, 1);
    }
}
