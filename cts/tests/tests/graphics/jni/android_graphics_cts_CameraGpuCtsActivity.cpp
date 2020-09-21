/*
 * Copyright 2017 The Android Open Source Project
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
 *
 */

#define LOG_TAG "CameraGpuCtsActivity"

#include <jni.h>
#include <unistd.h>

#include <memory>
#include <vector>

#include <android/log.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES/gl.h>
#include <GLES/glext.h>
#include <GLES2/gl2.h>

#include "CameraTestHelpers.h"
#include "ImageReaderTestHelpers.h"

//#define ALOGV(...) __android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG, __VA_ARGS__)
//#define ALOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define ALOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define ALOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)
#define ALOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

namespace {

static constexpr uint32_t kTestImageWidth = 640;
static constexpr uint32_t kTestImageHeight = 480;
static constexpr uint32_t kTestImageFormat = AIMAGE_FORMAT_PRIVATE;
static constexpr uint64_t kTestImageUsage = AHARDWAREBUFFER_USAGE_GPU_SAMPLED_IMAGE;
static constexpr uint32_t kTestImageCount = 3;

static const char kVertShader[] = R"(
  attribute vec2 aPosition;
  attribute vec2 aTextureCoords;
  varying vec2 texCoords;

  void main() {
    texCoords =  aTextureCoords;
    gl_Position = vec4(aPosition, 0.0, 1.0);
  }
)";

static const char kFragShader[] = R"(
  #extension GL_OES_EGL_image_external : require

  precision mediump float;
  varying vec2 texCoords;
  uniform samplerExternalOES sTexture;

  void main() {
    gl_FragColor = texture2D(sTexture, texCoords);
  }
)";

// A 80%-full screen mesh.
GLfloat kScreenTriangleStrip[] = {
    // 1st vertex
    -0.8f, -0.8f, 0.0f, 1.0f,
    // 2nd vertex
    -0.8f, 0.8f, 0.0f, 0.0f,
    // 3rd vertex
    0.8f, -0.8f, 1.0f, 1.0f,
    // 4th vertex
    0.8f, 0.8f, 1.0f, 0.0f,
};

static void checkGlError(const char* op) {
    for (GLint error = glGetError(); error; error
            = glGetError()) {
        ALOGW("after %s() glError (0x%x)\n", op, error);
    }
}

class CameraFrameRenderer {
  public:
    CameraFrameRenderer()
        : mImageReader(kTestImageWidth, kTestImageHeight, kTestImageFormat, kTestImageUsage,
                       kTestImageCount) {}

    ~CameraFrameRenderer() {
        if (mProgram) {
            glDeleteProgram(mProgram);
            mProgram = 0;
        }

        if (mEglImage != EGL_NO_IMAGE_KHR) {
            eglDestroyImageKHR(mEglDisplay, mEglImage);
            mEglImage = EGL_NO_IMAGE_KHR;
        }
    }

    bool isCameraReady() { return mCamera.isCameraReady(); }

    // Retrun Zero on success, or negative error code.
    int initRenderer() {
        int ret = mImageReader.initImageReader();
        if (ret < 0) {
            ALOGE("Failed to initialize image reader: %d", ret);
            return ret;
        }

        ret = mCamera.initCamera(mImageReader.getNativeWindow());
        if (ret < 0) {
            ALOGE("Failed to initialize camera: %d", ret);
            return ret;
        }

        // This test should only test devices with at least one camera.
        if (!mCamera.isCameraReady()) {
            ALOGW(
                "Camera is not ready after successful initialization. It's either due to camera on "
                "board lacks BACKWARDS_COMPATIBLE capability or the device does not have camera on "
                "board.");
            return 0;
        }

        // Load shader and program.
        mProgram = glCreateProgram();
        GLuint vertShader = loadShader(GL_VERTEX_SHADER, kVertShader);
        GLuint fragShader = loadShader(GL_FRAGMENT_SHADER, kFragShader);

        if (vertShader == 0 || fragShader == 0) {
            ALOGE("Failed to load shader");
            return -EINVAL;
        }

        mProgram = glCreateProgram();
        glAttachShader(mProgram, vertShader);
        checkGlError("glAttachShader");
        glAttachShader(mProgram, fragShader);
        checkGlError("glAttachShader");

        glLinkProgram(mProgram);
        GLint success;
        glGetProgramiv(mProgram, GL_LINK_STATUS, &success);
        if (!success) {
            GLchar infoLog[512];
            glGetProgramInfoLog(mProgram, 512, nullptr, infoLog);
            ALOGE("Shader failed to link: %s", infoLog);
            return -EINVAL;
        }

        // Get attributes.
        mPositionHandle = glGetAttribLocation(mProgram, "aPosition");
        mTextureCoordsHandle = glGetAttribLocation(mProgram, "aTextureCoords");

        // Get uniforms.
        mTextureUniform = glGetUniformLocation(mProgram, "sTexture");
        checkGlError("glGetUniformLocation");

        // Generate texture.
        glGenTextures(1, &mTextureId);
        checkGlError("glGenTextures");
        glBindTexture(GL_TEXTURE_EXTERNAL_OES, mTextureId);

        // Cache the display
        mEglDisplay = eglGetCurrentDisplay();

        return 0;
    }

    // Return Zero on success, or negative error code.
    int drawFrame() {
        if (!mCamera.isCameraReady()) {
            // We should never reach here. This test should just report success and quit early if
            // no camera can be found during initialization.
            ALOGE("No camera is ready for frame capture.");
            return -EINVAL;
        }

        // Indicate the camera to take recording.
        int ret = mCamera.takePicture();
        if (ret < 0) {
            ALOGE("Camera failed to take picture, error=%d", ret);
        }

        // Render the current buffer and then release it.
        AHardwareBuffer* buffer;
        ret = mImageReader.getBufferFromCurrentImage(&buffer);
        if (ret != 0) {
          // There might be no buffer acquired yet.
          return ret;
        }

        AHardwareBuffer_Desc outDesc;
        AHardwareBuffer_describe(buffer, &outDesc);

        // Render with EGLImage.
        EGLClientBuffer eglBuffer = eglGetNativeClientBufferANDROID(buffer);
        if (!eglBuffer) {
          ALOGE("Failed to create EGLClientBuffer");
          return -EINVAL;
        }

        if (mEglImage != EGL_NO_IMAGE_KHR) {
            eglDestroyImageKHR(mEglDisplay, mEglImage);
            mEglImage = EGL_NO_IMAGE_KHR;
        }

        EGLint attrs[] = {
            EGL_IMAGE_PRESERVED_KHR,
            EGL_TRUE,
            EGL_NONE,
        };

        mEglImage = eglCreateImageKHR(mEglDisplay, EGL_NO_CONTEXT, EGL_NATIVE_BUFFER_ANDROID,
                                      eglBuffer, attrs);

        if (mEglImage == EGL_NO_IMAGE_KHR) {
            ALOGE("Failed to create EGLImage.");
            return -EINVAL;
        }

        glClearColor(0.4f, 0.6f, 1.0f, 0.2f);
        glClear(GL_COLOR_BUFFER_BIT);
        checkGlError("glClearColor");

        // Use shader
        glUseProgram(mProgram);
        checkGlError("glUseProgram");

        // Map texture
        glEGLImageTargetTexture2DOES(GL_TEXTURE_EXTERNAL_OES, mEglImage);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_EXTERNAL_OES, mTextureId);
        glUniform1i(mTextureUniform, 0);
        checkGlError("glUniform1i");

        // Draw mesh
        glVertexAttribPointer(mPositionHandle, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat),
                              kScreenTriangleStrip);
        glEnableVertexAttribArray(mPositionHandle);
        glVertexAttribPointer(mTextureCoordsHandle, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat),
                              kScreenTriangleStrip + 2);
        glEnableVertexAttribArray(mTextureCoordsHandle);

        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        checkGlError("glDrawArrays");

        return 0;
    }

  private:
    static GLuint loadShader(GLenum shaderType, const char* source) {
        GLuint shader = glCreateShader(shaderType);

        glShaderSource(shader, 1, &source, nullptr);
        glCompileShader(shader);

        GLint success;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success) {
            ALOGE("Shader Failed to compile: %s", source);
            shader = 0;
        }
        return shader;
    }

    ImageReaderHelper mImageReader;
    CameraHelper mCamera;

    // Shader
    GLuint mProgram{0};

    // Texture
    EGLDisplay mEglDisplay{EGL_NO_DISPLAY};
    EGLImageKHR mEglImage{EGL_NO_IMAGE_KHR};
    GLuint mTextureId{0};
    GLuint mTextureUniform{0};
    GLuint mPositionHandle{0};
    GLuint mTextureCoordsHandle{0};
};

inline jlong jptr(CameraFrameRenderer* native_video_player) {
    return reinterpret_cast<intptr_t>(native_video_player);
}

inline CameraFrameRenderer* native(jlong ptr) {
    return reinterpret_cast<CameraFrameRenderer*>(ptr);
}

jlong createRenderer(JNIEnv*, jclass) {
    auto renderer = std::unique_ptr<CameraFrameRenderer>(new CameraFrameRenderer);
    int ret = renderer->initRenderer();
    if (ret < 0) {
        ALOGE("Failed to init renderer: %d", ret);
        return jptr(nullptr);
    }

    return jptr(renderer.release());
}

bool isCameraReady(JNIEnv*, jclass, jlong renderer) {
    if (renderer == 0) {
        ALOGE("isCameraReady: Invalid renderer: nullptr.");
        return false;
    }

    return native(renderer)->isCameraReady();
}

void destroyRenderer(JNIEnv*, jclass, jlong renderer) { delete native(renderer); }

jint drawFrame(JNIEnv*, jclass, jlong renderer) {
    if (renderer == 0) {
        ALOGE("Invalid renderer.");
        return -EINVAL;
    }

    return native(renderer)->drawFrame();
}

const std::vector<JNINativeMethod> gMethods = {{
    {"nCreateRenderer", "()J", (void*)createRenderer},
    {"nIsCameraReady", "(J)Z", (void*)isCameraReady},
    {"nDestroyRenderer", "(J)V", (void*)destroyRenderer},
    {"nDrawFrame", "(J)I", (void*)drawFrame},
}};

}  // namespace

int register_android_graphics_cts_CameraGpuCtsActivity(JNIEnv* env) {
    jclass clazz = env->FindClass("android/graphics/cts/CameraGpuCtsActivity");
    return env->RegisterNatives(clazz, gMethods.data(), gMethods.size());
}
