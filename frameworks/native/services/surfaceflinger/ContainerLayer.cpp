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

// #define LOG_NDEBUG 0
#undef LOG_TAG
#define LOG_TAG "ContainerLayer"

#include "ContainerLayer.h"

namespace android {

ContainerLayer::ContainerLayer(SurfaceFlinger* flinger, const sp<Client>& client,
                               const String8& name, uint32_t w, uint32_t h, uint32_t flags)
      : Layer(flinger, client, name, w, h, flags) {
    mDrawingState = mCurrentState;
}

void ContainerLayer::onDraw(const RenderArea&, const Region& /* clip */, bool) const {}

bool ContainerLayer::isVisible() const {
    return !isHiddenByPolicy();
}

void ContainerLayer::setPerFrameData(const sp<const DisplayDevice>&) {}

} // namespace android
