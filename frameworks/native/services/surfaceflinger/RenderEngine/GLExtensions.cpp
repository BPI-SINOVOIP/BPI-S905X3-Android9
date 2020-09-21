/*
 * Copyright (C) 2010 The Android Open Source Project
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

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "GLExtensions.h"

namespace android {
// ---------------------------------------------------------------------------

ANDROID_SINGLETON_STATIC_INSTANCE(GLExtensions)

SortedVector<String8> GLExtensions::parseExtensionString(char const* extensions) {
    SortedVector<String8> list;

    char const* curr = extensions;
    char const* head = curr;
    do {
        head = strchr(curr, ' ');
        String8 s(curr, head ? head - curr : strlen(curr));
        if (s.length()) {
            list.add(s);
        }
        curr = head + 1;
    } while (head);

    return list;
}

void GLExtensions::initWithGLStrings(GLubyte const* vendor, GLubyte const* renderer,
                                     GLubyte const* version, GLubyte const* extensions) {
    mVendor = (char const*)vendor;
    mRenderer = (char const*)renderer;
    mVersion = (char const*)version;
    mExtensions = (char const*)extensions;
    mExtensionList = parseExtensionString(mExtensions);
}

bool GLExtensions::hasExtension(char const* extension) const {
    const String8 s(extension);
    return mExtensionList.indexOf(s) >= 0;
}

char const* GLExtensions::getVendor() const {
    return mVendor.string();
}

char const* GLExtensions::getRenderer() const {
    return mRenderer.string();
}

char const* GLExtensions::getVersion() const {
    return mVersion.string();
}

char const* GLExtensions::getExtensions() const {
    return mExtensions.string();
}

void GLExtensions::initWithEGLStrings(char const* eglVersion, char const* eglExtensions) {
    mEGLVersion = eglVersion;
    mEGLExtensions = eglExtensions;
    mEGLExtensionList = parseExtensionString(mEGLExtensions);

    // EGL_ANDROIDX_no_config_context is an experimental extension with no
    // written specification. It will be replaced by something more formal.
    // SurfaceFlinger is using it to allow a single EGLContext to render to
    // both a 16-bit primary display framebuffer and a 32-bit virtual display
    // framebuffer.
    //
    // EGL_KHR_no_config_context is official extension to allow creating a
    // context that works with any surface of a display.
    if (hasEGLExtension("EGL_ANDROIDX_no_config_context") ||
        hasEGLExtension("EGL_KHR_no_config_context")) {
        mHasNoConfigContext = true;
    }

    if (hasEGLExtension("EGL_ANDROID_native_fence_sync")) {
        mHasNativeFenceSync = true;
    }
    if (hasEGLExtension("EGL_KHR_fence_sync")) {
        mHasFenceSync = true;
    }
    if (hasEGLExtension("EGL_KHR_wait_sync")) {
        mHasWaitSync = true;
    }

    if (hasEGLExtension("EGL_ANDROID_image_crop")) {
        mHasImageCrop = true;
    }
    if (hasEGLExtension("EGL_EXT_protected_content")) {
        mHasProtectedContent = true;
    }
    if (hasEGLExtension("EGL_IMG_context_priority")) {
        mHasContextPriority = true;
    }
}

char const* GLExtensions::getEGLVersion() const {
    return mEGLVersion.string();
}

char const* GLExtensions::getEGLExtensions() const {
    return mEGLExtensions.string();
}

bool GLExtensions::hasEGLExtension(char const* extension) const {
    const String8 s(extension);
    return mEGLExtensionList.indexOf(s) >= 0;
}

// ---------------------------------------------------------------------------
}; // namespace android
