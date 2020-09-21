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

#include <layerproto/LayerProtoHeader.h>

#include <ui/GraphicBuffer.h>
#include <ui/Rect.h>
#include <ui/Region.h>

#include <Transform.h>

#include <math/vec4.h>

namespace android {
namespace surfaceflinger {
class LayerProtoHelper {
public:
    static void writeToProto(const Rect& rect, RectProto* rectProto);
    static void writeToProto(const FloatRect& rect, FloatRectProto* rectProto);
    static void writeToProto(const Region& region, RegionProto* regionProto);
    static void writeToProto(const half4 color, ColorProto* colorProto);
    static void writeToProto(const Transform& transform, TransformProto* transformProto);
    static void writeToProto(const sp<GraphicBuffer>& buffer, ActiveBufferProto* activeBufferProto);
};

} // namespace surfaceflinger
} // namespace android
