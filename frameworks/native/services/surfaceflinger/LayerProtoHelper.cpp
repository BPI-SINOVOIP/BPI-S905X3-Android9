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

#include "LayerProtoHelper.h"

namespace android {
namespace surfaceflinger {
void LayerProtoHelper::writeToProto(const Region& region, RegionProto* regionProto) {
    Region::const_iterator head = region.begin();
    Region::const_iterator const tail = region.end();
    uint64_t address = reinterpret_cast<uint64_t>(&region);
    regionProto->set_id(address);
    while (head != tail) {
        RectProto* rectProto = regionProto->add_rect();
        writeToProto(*head, rectProto);
        head++;
    }
}

void LayerProtoHelper::writeToProto(const Rect& rect, RectProto* rectProto) {
    rectProto->set_left(rect.left);
    rectProto->set_top(rect.top);
    rectProto->set_bottom(rect.bottom);
    rectProto->set_right(rect.right);
}

void LayerProtoHelper::writeToProto(const FloatRect& rect, FloatRectProto* rectProto) {
    rectProto->set_left(rect.left);
    rectProto->set_top(rect.top);
    rectProto->set_bottom(rect.bottom);
    rectProto->set_right(rect.right);
}

void LayerProtoHelper::writeToProto(const half4 color, ColorProto* colorProto) {
    colorProto->set_r(color.r);
    colorProto->set_g(color.g);
    colorProto->set_b(color.b);
    colorProto->set_a(color.a);
}

void LayerProtoHelper::writeToProto(const Transform& transform, TransformProto* transformProto) {
    transformProto->set_dsdx(transform[0][0]);
    transformProto->set_dtdx(transform[0][1]);
    transformProto->set_dsdy(transform[1][0]);
    transformProto->set_dtdy(transform[1][1]);
}

void LayerProtoHelper::writeToProto(const sp<GraphicBuffer>& buffer,
                                    ActiveBufferProto* activeBufferProto) {
    activeBufferProto->set_width(buffer->getWidth());
    activeBufferProto->set_height(buffer->getHeight());
    activeBufferProto->set_stride(buffer->getStride());
    activeBufferProto->set_format(buffer->format);
}

} // namespace surfaceflinger
} // namespace android
