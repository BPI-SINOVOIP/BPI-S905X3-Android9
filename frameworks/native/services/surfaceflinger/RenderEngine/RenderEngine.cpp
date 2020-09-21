/*
 * Copyright 2013 The Android Open Source Project
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

#include <log/log.h>
#include <ui/Rect.h>
#include <ui/Region.h>

#include "GLES20RenderEngine.h"
#include "GLExtensions.h"
#include "Image.h"
#include "Mesh.h"
#include "RenderEngine.h"

#include <SurfaceFlinger.h>
#include <vector>

#include <android/hardware/configstore/1.0/ISurfaceFlingerConfigs.h>
#include <configstore/Utils.h>

using namespace android::hardware::configstore;
using namespace android::hardware::configstore::V1_0;

extern "C" EGLAPI const char* eglQueryStringImplementationANDROID(EGLDisplay dpy, EGLint name);

// ---------------------------------------------------------------------------
namespace android {
namespace RE {
// ---------------------------------------------------------------------------

RenderEngine::~RenderEngine() = default;

namespace impl {

std::unique_ptr<RenderEngine> RenderEngine::create(int hwcFormat, uint32_t featureFlags) {
    // initialize EGL for the default display
    EGLDisplay display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (!eglInitialize(display, nullptr, nullptr)) {
        LOG_ALWAYS_FATAL("failed to initialize EGL");
    }

    GLExtensions& extensions = GLExtensions::getInstance();
    extensions.initWithEGLStrings(eglQueryStringImplementationANDROID(display, EGL_VERSION),
                                  eglQueryStringImplementationANDROID(display, EGL_EXTENSIONS));

    // The code assumes that ES2 or later is available if this extension is
    // supported.
    EGLConfig config = EGL_NO_CONFIG;
    if (!extensions.hasNoConfigContext()) {
        config = chooseEglConfig(display, hwcFormat, /*logConfig*/ true);
    }

    EGLint renderableType = 0;
    if (config == EGL_NO_CONFIG) {
        renderableType = EGL_OPENGL_ES2_BIT;
    } else if (!eglGetConfigAttrib(display, config, EGL_RENDERABLE_TYPE, &renderableType)) {
        LOG_ALWAYS_FATAL("can't query EGLConfig RENDERABLE_TYPE");
    }
    EGLint contextClientVersion = 0;
    if (renderableType & EGL_OPENGL_ES2_BIT) {
        contextClientVersion = 2;
    } else if (renderableType & EGL_OPENGL_ES_BIT) {
        contextClientVersion = 1;
    } else {
        LOG_ALWAYS_FATAL("no supported EGL_RENDERABLE_TYPEs");
    }

    std::vector<EGLint> contextAttributes;
    contextAttributes.reserve(6);
    contextAttributes.push_back(EGL_CONTEXT_CLIENT_VERSION);
    contextAttributes.push_back(contextClientVersion);
    bool useContextPriority = overrideUseContextPriorityFromConfig(extensions.hasContextPriority());
    if (useContextPriority) {
        contextAttributes.push_back(EGL_CONTEXT_PRIORITY_LEVEL_IMG);
        contextAttributes.push_back(EGL_CONTEXT_PRIORITY_HIGH_IMG);
    }
    contextAttributes.push_back(EGL_NONE);

    EGLContext ctxt = eglCreateContext(display, config, nullptr, contextAttributes.data());

    // if can't create a GL context, we can only abort.
    LOG_ALWAYS_FATAL_IF(ctxt == EGL_NO_CONTEXT, "EGLContext creation failed");

    // now figure out what version of GL did we actually get
    // NOTE: a dummy surface is not needed if KHR_create_context is supported

    EGLConfig dummyConfig = config;
    if (dummyConfig == EGL_NO_CONFIG) {
        dummyConfig = chooseEglConfig(display, hwcFormat, /*logConfig*/ true);
    }
    EGLint attribs[] = {EGL_WIDTH, 1, EGL_HEIGHT, 1, EGL_NONE, EGL_NONE};
    EGLSurface dummy = eglCreatePbufferSurface(display, dummyConfig, attribs);
    LOG_ALWAYS_FATAL_IF(dummy == EGL_NO_SURFACE, "can't create dummy pbuffer");
    EGLBoolean success = eglMakeCurrent(display, dummy, dummy, ctxt);
    LOG_ALWAYS_FATAL_IF(!success, "can't make dummy pbuffer current");

    extensions.initWithGLStrings(glGetString(GL_VENDOR), glGetString(GL_RENDERER),
                                 glGetString(GL_VERSION), glGetString(GL_EXTENSIONS));

    GlesVersion version = parseGlesVersion(extensions.getVersion());

    // initialize the renderer while GL is current

    std::unique_ptr<RenderEngine> engine;
    switch (version) {
        case GLES_VERSION_1_0:
        case GLES_VERSION_1_1:
            LOG_ALWAYS_FATAL("SurfaceFlinger requires OpenGL ES 2.0 minimum to run.");
            break;
        case GLES_VERSION_2_0:
        case GLES_VERSION_3_0:
            engine = std::make_unique<GLES20RenderEngine>(featureFlags);
            break;
    }
    engine->setEGLHandles(display, config, ctxt);

    ALOGI("OpenGL ES informations:");
    ALOGI("vendor    : %s", extensions.getVendor());
    ALOGI("renderer  : %s", extensions.getRenderer());
    ALOGI("version   : %s", extensions.getVersion());
    ALOGI("extensions: %s", extensions.getExtensions());
    ALOGI("GL_MAX_TEXTURE_SIZE = %zu", engine->getMaxTextureSize());
    ALOGI("GL_MAX_VIEWPORT_DIMS = %zu", engine->getMaxViewportDims());

    eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    eglDestroySurface(display, dummy);

    return engine;
}

bool RenderEngine::overrideUseContextPriorityFromConfig(bool useContextPriority) {
    OptionalBool ret;
    ISurfaceFlingerConfigs::getService()->useContextPriority([&ret](OptionalBool b) { ret = b; });
    if (ret.specified) {
        return ret.value;
    } else {
        return useContextPriority;
    }
}

RenderEngine::RenderEngine(uint32_t featureFlags)
      : mEGLDisplay(EGL_NO_DISPLAY),
        mEGLConfig(nullptr),
        mEGLContext(EGL_NO_CONTEXT),
        mFeatureFlags(featureFlags) {}

RenderEngine::~RenderEngine() {
    eglMakeCurrent(mEGLDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    eglTerminate(mEGLDisplay);
}

void RenderEngine::setEGLHandles(EGLDisplay display, EGLConfig config, EGLContext ctxt) {
    mEGLDisplay = display;
    mEGLConfig = config;
    mEGLContext = ctxt;
}

EGLDisplay RenderEngine::getEGLDisplay() const {
    return mEGLDisplay;
}

EGLConfig RenderEngine::getEGLConfig() const {
    return mEGLConfig;
}

bool RenderEngine::supportsImageCrop() const {
    return GLExtensions::getInstance().hasImageCrop();
}

bool RenderEngine::isCurrent() const {
    return mEGLDisplay == eglGetCurrentDisplay() && mEGLContext == eglGetCurrentContext();
}

std::unique_ptr<RE::Surface> RenderEngine::createSurface() {
    return std::make_unique<Surface>(*this);
}

std::unique_ptr<RE::Image> RenderEngine::createImage() {
    return std::make_unique<Image>(*this);
}

bool RenderEngine::setCurrentSurface(const android::RE::Surface& surface) {
    // Note: RE::Surface is an abstract interface. This implementation only ever
    // creates RE::impl::Surface's, so it is safe to just cast to the actual
    // type.
    return setCurrentSurface(static_cast<const android::RE::impl::Surface&>(surface));
}

bool RenderEngine::setCurrentSurface(const android::RE::impl::Surface& surface) {
    bool success = true;
    EGLSurface eglSurface = surface.getEGLSurface();
    if (eglSurface != eglGetCurrentSurface(EGL_DRAW)) {
        success = eglMakeCurrent(mEGLDisplay, eglSurface, eglSurface, mEGLContext) == EGL_TRUE;
        if (success && surface.getAsync()) {
            eglSwapInterval(mEGLDisplay, 0);
        }
    }

    return success;
}

void RenderEngine::resetCurrentSurface() {
    eglMakeCurrent(mEGLDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
}

base::unique_fd RenderEngine::flush() {
    if (!GLExtensions::getInstance().hasNativeFenceSync()) {
        return base::unique_fd();
    }

    EGLSyncKHR sync = eglCreateSyncKHR(mEGLDisplay, EGL_SYNC_NATIVE_FENCE_ANDROID, nullptr);
    if (sync == EGL_NO_SYNC_KHR) {
        ALOGW("failed to create EGL native fence sync: %#x", eglGetError());
        return base::unique_fd();
    }

    // native fence fd will not be populated until flush() is done.
    glFlush();

    // get the fence fd
    base::unique_fd fenceFd(eglDupNativeFenceFDANDROID(mEGLDisplay, sync));
    eglDestroySyncKHR(mEGLDisplay, sync);
    if (fenceFd == EGL_NO_NATIVE_FENCE_FD_ANDROID) {
        ALOGW("failed to dup EGL native fence sync: %#x", eglGetError());
    }

    return fenceFd;
}

bool RenderEngine::finish() {
    if (!GLExtensions::getInstance().hasFenceSync()) {
        ALOGW("no synchronization support");
        return false;
    }

    EGLSyncKHR sync = eglCreateSyncKHR(mEGLDisplay, EGL_SYNC_FENCE_KHR, nullptr);
    if (sync == EGL_NO_SYNC_KHR) {
        ALOGW("failed to create EGL fence sync: %#x", eglGetError());
        return false;
    }

    EGLint result = eglClientWaitSyncKHR(mEGLDisplay, sync, EGL_SYNC_FLUSH_COMMANDS_BIT_KHR,
                                         2000000000 /*2 sec*/);
    EGLint error = eglGetError();
    eglDestroySyncKHR(mEGLDisplay, sync);
    if (result != EGL_CONDITION_SATISFIED_KHR) {
        if (result == EGL_TIMEOUT_EXPIRED_KHR) {
            ALOGW("fence wait timed out");
        } else {
            ALOGW("error waiting on EGL fence: %#x", error);
        }
        return false;
    }

    return true;
}

bool RenderEngine::waitFence(base::unique_fd fenceFd) {
    if (!GLExtensions::getInstance().hasNativeFenceSync() ||
        !GLExtensions::getInstance().hasWaitSync()) {
        return false;
    }

    EGLint attribs[] = {EGL_SYNC_NATIVE_FENCE_FD_ANDROID, fenceFd, EGL_NONE};
    EGLSyncKHR sync = eglCreateSyncKHR(mEGLDisplay, EGL_SYNC_NATIVE_FENCE_ANDROID, attribs);
    if (sync == EGL_NO_SYNC_KHR) {
        ALOGE("failed to create EGL native fence sync: %#x", eglGetError());
        return false;
    }

    // fenceFd is now owned by EGLSync
    (void)fenceFd.release();

    // XXX: The spec draft is inconsistent as to whether this should return an
    // EGLint or void.  Ignore the return value for now, as it's not strictly
    // needed.
    eglWaitSyncKHR(mEGLDisplay, sync, 0);
    EGLint error = eglGetError();
    eglDestroySyncKHR(mEGLDisplay, sync);
    if (error != EGL_SUCCESS) {
        ALOGE("failed to wait for EGL native fence sync: %#x", error);
        return false;
    }

    return true;
}

void RenderEngine::checkErrors() const {
    do {
        // there could be more than one error flag
        GLenum error = glGetError();
        if (error == GL_NO_ERROR) break;
        ALOGE("GL error 0x%04x", int(error));
    } while (true);
}

RenderEngine::GlesVersion RenderEngine::parseGlesVersion(const char* str) {
    int major, minor;
    if (sscanf(str, "OpenGL ES-CM %d.%d", &major, &minor) != 2) {
        if (sscanf(str, "OpenGL ES %d.%d", &major, &minor) != 2) {
            ALOGW("Unable to parse GL_VERSION string: \"%s\"", str);
            return GLES_VERSION_1_0;
        }
    }

    if (major == 1 && minor == 0) return GLES_VERSION_1_0;
    if (major == 1 && minor >= 1) return GLES_VERSION_1_1;
    if (major == 2 && minor >= 0) return GLES_VERSION_2_0;
    if (major == 3 && minor >= 0) return GLES_VERSION_3_0;

    ALOGW("Unrecognized OpenGL ES version: %d.%d", major, minor);
    return GLES_VERSION_1_0;
}

void RenderEngine::fillRegionWithColor(const Region& region, uint32_t height, float red,
                                       float green, float blue, float alpha) {
    size_t c;
    Rect const* r = region.getArray(&c);
    Mesh mesh(Mesh::TRIANGLES, c * 6, 2);
    Mesh::VertexArray<vec2> position(mesh.getPositionArray<vec2>());
    for (size_t i = 0; i < c; i++, r++) {
        position[i * 6 + 0].x = r->left;
        position[i * 6 + 0].y = height - r->top;
        position[i * 6 + 1].x = r->left;
        position[i * 6 + 1].y = height - r->bottom;
        position[i * 6 + 2].x = r->right;
        position[i * 6 + 2].y = height - r->bottom;
        position[i * 6 + 3].x = r->left;
        position[i * 6 + 3].y = height - r->top;
        position[i * 6 + 4].x = r->right;
        position[i * 6 + 4].y = height - r->bottom;
        position[i * 6 + 5].x = r->right;
        position[i * 6 + 5].y = height - r->top;
    }
    setupFillWithColor(red, green, blue, alpha);
    drawMesh(mesh);
}

void RenderEngine::clearWithColor(float red, float green, float blue, float alpha) {
    glClearColor(red, green, blue, alpha);
    glClear(GL_COLOR_BUFFER_BIT);
}

void RenderEngine::setScissor(uint32_t left, uint32_t bottom, uint32_t right, uint32_t top) {
    glScissor(left, bottom, right, top);
    glEnable(GL_SCISSOR_TEST);
}

void RenderEngine::disableScissor() {
    glDisable(GL_SCISSOR_TEST);
}

void RenderEngine::genTextures(size_t count, uint32_t* names) {
    glGenTextures(count, names);
}

void RenderEngine::deleteTextures(size_t count, uint32_t const* names) {
    glDeleteTextures(count, names);
}

void RenderEngine::bindExternalTextureImage(uint32_t texName, const android::RE::Image& image) {
    // Note: RE::Image is an abstract interface. This implementation only ever
    // creates RE::impl::Image's, so it is safe to just cast to the actual type.
    return bindExternalTextureImage(texName, static_cast<const android::RE::impl::Image&>(image));
}

void RenderEngine::bindExternalTextureImage(uint32_t texName,
                                            const android::RE::impl::Image& image) {
    const GLenum target = GL_TEXTURE_EXTERNAL_OES;

    glBindTexture(target, texName);
    if (image.getEGLImage() != EGL_NO_IMAGE_KHR) {
        glEGLImageTargetTexture2DOES(target, static_cast<GLeglImageOES>(image.getEGLImage()));
    }
}

void RenderEngine::readPixels(size_t l, size_t b, size_t w, size_t h, uint32_t* pixels) {
    glReadPixels(l, b, w, h, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
}

void RenderEngine::dump(String8& result) {
    const GLExtensions& extensions = GLExtensions::getInstance();

    result.appendFormat("EGL implementation : %s\n", extensions.getEGLVersion());
    result.appendFormat("%s\n", extensions.getEGLExtensions());

    result.appendFormat("GLES: %s, %s, %s\n", extensions.getVendor(), extensions.getRenderer(),
                        extensions.getVersion());
    result.appendFormat("%s\n", extensions.getExtensions());
}

// ---------------------------------------------------------------------------

void RenderEngine::bindNativeBufferAsFrameBuffer(ANativeWindowBuffer* buffer,
                                                 RE::BindNativeBufferAsFramebuffer* bindHelper) {
    bindHelper->mImage = eglCreateImageKHR(mEGLDisplay, EGL_NO_CONTEXT, EGL_NATIVE_BUFFER_ANDROID,
                                           buffer, nullptr);
    if (bindHelper->mImage == EGL_NO_IMAGE_KHR) {
        bindHelper->mStatus = NO_MEMORY;
        return;
    }

    uint32_t glStatus;
    bindImageAsFramebuffer(bindHelper->mImage, &bindHelper->mTexName, &bindHelper->mFbName,
                           &glStatus);

    ALOGE_IF(glStatus != GL_FRAMEBUFFER_COMPLETE_OES, "glCheckFramebufferStatusOES error %d",
             glStatus);

    bindHelper->mStatus = glStatus == GL_FRAMEBUFFER_COMPLETE_OES ? NO_ERROR : BAD_VALUE;
}

void RenderEngine::unbindNativeBufferAsFrameBuffer(RE::BindNativeBufferAsFramebuffer* bindHelper) {
    if (bindHelper->mImage == EGL_NO_IMAGE_KHR) {
        return;
    }

    // back to main framebuffer
    unbindFramebuffer(bindHelper->mTexName, bindHelper->mFbName);
    eglDestroyImageKHR(mEGLDisplay, bindHelper->mImage);

    // Workaround for b/77935566 to force the EGL driver to release the
    // screenshot buffer
    setScissor(0, 0, 0, 0);
    clearWithColor(0.0, 0.0, 0.0, 0.0);
    disableScissor();
}

// ---------------------------------------------------------------------------

static status_t selectConfigForAttribute(EGLDisplay dpy, EGLint const* attrs, EGLint attribute,
                                         EGLint wanted, EGLConfig* outConfig) {
    EGLint numConfigs = -1, n = 0;
    eglGetConfigs(dpy, nullptr, 0, &numConfigs);
    EGLConfig* const configs = new EGLConfig[numConfigs];
    eglChooseConfig(dpy, attrs, configs, numConfigs, &n);

    if (n) {
        if (attribute != EGL_NONE) {
            for (int i = 0; i < n; i++) {
                EGLint value = 0;
                eglGetConfigAttrib(dpy, configs[i], attribute, &value);
                if (wanted == value) {
                    *outConfig = configs[i];
                    delete[] configs;
                    return NO_ERROR;
                }
            }
        } else {
            // just pick the first one
            *outConfig = configs[0];
            delete[] configs;
            return NO_ERROR;
        }
    }
    delete[] configs;
    return NAME_NOT_FOUND;
}

class EGLAttributeVector {
    struct Attribute;
    class Adder;
    friend class Adder;
    KeyedVector<Attribute, EGLint> mList;
    struct Attribute {
        Attribute() : v(0){};
        explicit Attribute(EGLint v) : v(v) {}
        EGLint v;
        bool operator<(const Attribute& other) const {
            // this places EGL_NONE at the end
            EGLint lhs(v);
            EGLint rhs(other.v);
            if (lhs == EGL_NONE) lhs = 0x7FFFFFFF;
            if (rhs == EGL_NONE) rhs = 0x7FFFFFFF;
            return lhs < rhs;
        }
    };
    class Adder {
        friend class EGLAttributeVector;
        EGLAttributeVector& v;
        EGLint attribute;
        Adder(EGLAttributeVector& v, EGLint attribute) : v(v), attribute(attribute) {}

    public:
        void operator=(EGLint value) {
            if (attribute != EGL_NONE) {
                v.mList.add(Attribute(attribute), value);
            }
        }
        operator EGLint() const { return v.mList[attribute]; }
    };

public:
    EGLAttributeVector() { mList.add(Attribute(EGL_NONE), EGL_NONE); }
    void remove(EGLint attribute) {
        if (attribute != EGL_NONE) {
            mList.removeItem(Attribute(attribute));
        }
    }
    Adder operator[](EGLint attribute) { return Adder(*this, attribute); }
    EGLint operator[](EGLint attribute) const { return mList[attribute]; }
    // cast-operator to (EGLint const*)
    operator EGLint const*() const { return &mList.keyAt(0).v; }
};

static status_t selectEGLConfig(EGLDisplay display, EGLint format, EGLint renderableType,
                                EGLConfig* config) {
    // select our EGLConfig. It must support EGL_RECORDABLE_ANDROID if
    // it is to be used with WIFI displays
    status_t err;
    EGLint wantedAttribute;
    EGLint wantedAttributeValue;

    EGLAttributeVector attribs;
    if (renderableType) {
        attribs[EGL_RENDERABLE_TYPE] = renderableType;
        attribs[EGL_RECORDABLE_ANDROID] = EGL_TRUE;
        attribs[EGL_SURFACE_TYPE] = EGL_WINDOW_BIT | EGL_PBUFFER_BIT;
        attribs[EGL_FRAMEBUFFER_TARGET_ANDROID] = EGL_TRUE;
        attribs[EGL_RED_SIZE] = 8;
        attribs[EGL_GREEN_SIZE] = 8;
        attribs[EGL_BLUE_SIZE] = 8;
        attribs[EGL_ALPHA_SIZE] = 8;
        wantedAttribute = EGL_NONE;
        wantedAttributeValue = EGL_NONE;
    } else {
        // if no renderable type specified, fallback to a simplified query
        wantedAttribute = EGL_NATIVE_VISUAL_ID;
        wantedAttributeValue = format;
    }

    err = selectConfigForAttribute(display, attribs, wantedAttribute, wantedAttributeValue, config);
    if (err == NO_ERROR) {
        EGLint caveat;
        if (eglGetConfigAttrib(display, *config, EGL_CONFIG_CAVEAT, &caveat))
            ALOGW_IF(caveat == EGL_SLOW_CONFIG, "EGL_SLOW_CONFIG selected!");
    }

    return err;
}

EGLConfig RenderEngine::chooseEglConfig(EGLDisplay display, int format, bool logConfig) {
    status_t err;
    EGLConfig config;

    // First try to get an ES2 config
    err = selectEGLConfig(display, format, EGL_OPENGL_ES2_BIT, &config);
    if (err != NO_ERROR) {
        // If ES2 fails, try ES1
        err = selectEGLConfig(display, format, EGL_OPENGL_ES_BIT, &config);
        if (err != NO_ERROR) {
            // still didn't work, probably because we're on the emulator...
            // try a simplified query
            ALOGW("no suitable EGLConfig found, trying a simpler query");
            err = selectEGLConfig(display, format, 0, &config);
            if (err != NO_ERROR) {
                // this EGL is too lame for android
                LOG_ALWAYS_FATAL("no suitable EGLConfig found, giving up");
            }
        }
    }

    if (logConfig) {
        // print some debugging info
        EGLint r, g, b, a;
        eglGetConfigAttrib(display, config, EGL_RED_SIZE, &r);
        eglGetConfigAttrib(display, config, EGL_GREEN_SIZE, &g);
        eglGetConfigAttrib(display, config, EGL_BLUE_SIZE, &b);
        eglGetConfigAttrib(display, config, EGL_ALPHA_SIZE, &a);
        ALOGI("EGL information:");
        ALOGI("vendor    : %s", eglQueryString(display, EGL_VENDOR));
        ALOGI("version   : %s", eglQueryString(display, EGL_VERSION));
        ALOGI("extensions: %s", eglQueryString(display, EGL_EXTENSIONS));
        ALOGI("Client API: %s", eglQueryString(display, EGL_CLIENT_APIS) ?: "Not Supported");
        ALOGI("EGLSurface: %d-%d-%d-%d, config=%p", r, g, b, a, config);
    }

    return config;
}

void RenderEngine::primeCache() const {
    ProgramCache::getInstance().primeCache(mFeatureFlags & WIDE_COLOR_SUPPORT);
}

// ---------------------------------------------------------------------------

} // namespace impl
} // namespace RE
} // namespace android
