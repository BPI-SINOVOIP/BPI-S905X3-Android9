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
#include <android-base/stringprintf.h>
#include <layerproto/LayerProtoParser.h>
#include <ui/DebugUtils.h>

using android::base::StringAppendF;
using android::base::StringPrintf;

namespace android {
namespace surfaceflinger {

bool sortLayers(LayerProtoParser::Layer* lhs, const LayerProtoParser::Layer* rhs) {
    uint32_t ls = lhs->layerStack;
    uint32_t rs = rhs->layerStack;
    if (ls != rs) return ls < rs;

    int32_t lz = lhs->z;
    int32_t rz = rhs->z;
    if (lz != rz) {
        return (lz > rz) ? 1 : -1;
    }

    return lhs->id < rhs->id;
}

bool sortLayerUniquePtrs(const std::unique_ptr<LayerProtoParser::Layer>& lhs,
                   const std::unique_ptr<LayerProtoParser::Layer>& rhs) {
    return sortLayers(lhs.get(), rhs.get());
}

const LayerProtoParser::LayerGlobal LayerProtoParser::generateLayerGlobalInfo(
        const LayersProto& layersProto) {
    LayerGlobal layerGlobal;
    layerGlobal.resolution = {layersProto.resolution().w(), layersProto.resolution().h()};
    layerGlobal.colorMode = layersProto.color_mode();
    layerGlobal.colorTransform = layersProto.color_transform();
    layerGlobal.globalTransform = layersProto.global_transform();
    return layerGlobal;
}

std::vector<std::unique_ptr<LayerProtoParser::Layer>> LayerProtoParser::generateLayerTree(
        const LayersProto& layersProto) {
    std::unordered_map<int32_t, LayerProtoParser::Layer*> layerMap = generateMap(layersProto);
    std::vector<std::unique_ptr<LayerProtoParser::Layer>> layers;

    for (std::pair<int32_t, Layer*> kv : layerMap) {
        if (kv.second->parent == nullptr) {
            // Make unique_ptr for top level layers since they are not children. This ensures there
            // will only be one unique_ptr made for each layer.
            layers.push_back(std::unique_ptr<Layer>(kv.second));
        }
    }

    std::sort(layers.begin(), layers.end(), sortLayerUniquePtrs);
    return layers;
}

std::unordered_map<int32_t, LayerProtoParser::Layer*> LayerProtoParser::generateMap(
        const LayersProto& layersProto) {
    std::unordered_map<int32_t, Layer*> layerMap;

    for (int i = 0; i < layersProto.layers_size(); i++) {
        const LayerProto& layerProto = layersProto.layers(i);
        layerMap[layerProto.id()] = generateLayer(layerProto);
    }

    for (int i = 0; i < layersProto.layers_size(); i++) {
        const LayerProto& layerProto = layersProto.layers(i);
        updateChildrenAndRelative(layerProto, layerMap);
    }

    return layerMap;
}

LayerProtoParser::Layer* LayerProtoParser::generateLayer(const LayerProto& layerProto) {
    Layer* layer = new Layer();
    layer->id = layerProto.id();
    layer->name = layerProto.name();
    layer->type = layerProto.type();
    layer->transparentRegion = generateRegion(layerProto.transparent_region());
    layer->visibleRegion = generateRegion(layerProto.visible_region());
    layer->damageRegion = generateRegion(layerProto.damage_region());
    layer->layerStack = layerProto.layer_stack();
    layer->z = layerProto.z();
    layer->position = {layerProto.position().x(), layerProto.position().y()};
    layer->requestedPosition = {layerProto.requested_position().x(),
                                layerProto.requested_position().y()};
    layer->size = {layerProto.size().w(), layerProto.size().h()};
    layer->crop = generateRect(layerProto.crop());
    layer->finalCrop = generateRect(layerProto.final_crop());
    layer->isOpaque = layerProto.is_opaque();
    layer->invalidate = layerProto.invalidate();
    layer->dataspace = layerProto.dataspace();
    layer->pixelFormat = layerProto.pixel_format();
    layer->color = {layerProto.color().r(), layerProto.color().g(), layerProto.color().b(),
                    layerProto.color().a()};
    layer->requestedColor = {layerProto.requested_color().r(), layerProto.requested_color().g(),
                             layerProto.requested_color().b(), layerProto.requested_color().a()};
    layer->flags = layerProto.flags();
    layer->transform = generateTransform(layerProto.transform());
    layer->requestedTransform = generateTransform(layerProto.requested_transform());
    layer->activeBuffer = generateActiveBuffer(layerProto.active_buffer());
    layer->queuedFrames = layerProto.queued_frames();
    layer->refreshPending = layerProto.refresh_pending();
    layer->hwcFrame = generateRect(layerProto.hwc_frame());
    layer->hwcCrop = generateFloatRect(layerProto.hwc_crop());
    layer->hwcTransform = layerProto.hwc_transform();
    layer->windowType = layerProto.window_type();
    layer->appId = layerProto.app_id();
    layer->hwcCompositionType = layerProto.hwc_composition_type();
    layer->isProtected = layerProto.is_protected();

    return layer;
}

LayerProtoParser::Region LayerProtoParser::generateRegion(const RegionProto& regionProto) {
    LayerProtoParser::Region region;
    region.id = regionProto.id();
    for (int i = 0; i < regionProto.rect_size(); i++) {
        const RectProto& rectProto = regionProto.rect(i);
        region.rects.push_back(generateRect(rectProto));
    }

    return region;
}

LayerProtoParser::Rect LayerProtoParser::generateRect(const RectProto& rectProto) {
    LayerProtoParser::Rect rect;
    rect.left = rectProto.left();
    rect.top = rectProto.top();
    rect.right = rectProto.right();
    rect.bottom = rectProto.bottom();

    return rect;
}

LayerProtoParser::FloatRect LayerProtoParser::generateFloatRect(const FloatRectProto& rectProto) {
    LayerProtoParser::FloatRect rect;
    rect.left = rectProto.left();
    rect.top = rectProto.top();
    rect.right = rectProto.right();
    rect.bottom = rectProto.bottom();

    return rect;
}

LayerProtoParser::Transform LayerProtoParser::generateTransform(
        const TransformProto& transformProto) {
    LayerProtoParser::Transform transform;
    transform.dsdx = transformProto.dsdx();
    transform.dtdx = transformProto.dtdx();
    transform.dsdy = transformProto.dsdy();
    transform.dtdy = transformProto.dtdy();

    return transform;
}

LayerProtoParser::ActiveBuffer LayerProtoParser::generateActiveBuffer(
        const ActiveBufferProto& activeBufferProto) {
    LayerProtoParser::ActiveBuffer activeBuffer;
    activeBuffer.width = activeBufferProto.width();
    activeBuffer.height = activeBufferProto.height();
    activeBuffer.stride = activeBufferProto.stride();
    activeBuffer.format = activeBufferProto.format();

    return activeBuffer;
}

void LayerProtoParser::updateChildrenAndRelative(const LayerProto& layerProto,
                                                 std::unordered_map<int32_t, Layer*>& layerMap) {
    auto currLayer = layerMap[layerProto.id()];

    for (int i = 0; i < layerProto.children_size(); i++) {
        if (layerMap.count(layerProto.children(i)) > 0) {
            // Only make unique_ptrs for children since they are guaranteed to be unique, only one
            // parent per child. This ensures there will only be one unique_ptr made for each layer.
            currLayer->children.push_back(std::unique_ptr<Layer>(layerMap[layerProto.children(i)]));
        }
    }

    for (int i = 0; i < layerProto.relatives_size(); i++) {
        if (layerMap.count(layerProto.relatives(i)) > 0) {
            currLayer->relatives.push_back(layerMap[layerProto.relatives(i)]);
        }
    }

    if (layerProto.has_parent()) {
        if (layerMap.count(layerProto.parent()) > 0) {
            currLayer->parent = layerMap[layerProto.parent()];
        }
    }

    if (layerProto.has_z_order_relative_of()) {
        if (layerMap.count(layerProto.z_order_relative_of()) > 0) {
            currLayer->zOrderRelativeOf = layerMap[layerProto.z_order_relative_of()];
        }
    }
}

std::string LayerProtoParser::layersToString(
        std::vector<std::unique_ptr<LayerProtoParser::Layer>> layers) {
    std::string result;
    for (std::unique_ptr<LayerProtoParser::Layer>& layer : layers) {
        if (layer->zOrderRelativeOf != nullptr) {
            continue;
        }
        result.append(layerToString(layer.get()).c_str());
    }

    return result;
}

std::string LayerProtoParser::layerToString(LayerProtoParser::Layer* layer) {
    std::string result;

    std::vector<Layer*> traverse(layer->relatives);
    for (std::unique_ptr<LayerProtoParser::Layer>& child : layer->children) {
        if (child->zOrderRelativeOf != nullptr) {
            continue;
        }

        traverse.push_back(child.get());
    }

    std::sort(traverse.begin(), traverse.end(), sortLayers);

    size_t i = 0;
    for (; i < traverse.size(); i++) {
        auto& relative = traverse[i];
        if (relative->z >= 0) {
            break;
        }
        result.append(layerToString(relative).c_str());
    }
    result.append(layer->to_string().c_str());
    result.append("\n");
    for (; i < traverse.size(); i++) {
        auto& relative = traverse[i];
        result.append(layerToString(relative).c_str());
    }

    return result;
}

std::string LayerProtoParser::ActiveBuffer::to_string() const {
    return StringPrintf("[%4ux%4u:%4u,%s]", width, height, stride,
                        decodePixelFormat(format).c_str());
}

std::string LayerProtoParser::Transform::to_string() const {
    return StringPrintf("[%.2f, %.2f][%.2f, %.2f]", static_cast<double>(dsdx),
                        static_cast<double>(dtdx), static_cast<double>(dsdy),
                        static_cast<double>(dtdy));
}

std::string LayerProtoParser::Rect::to_string() const {
    return StringPrintf("[%3d, %3d, %3d, %3d]", left, top, right, bottom);
}

std::string LayerProtoParser::FloatRect::to_string() const {
    return StringPrintf("[%.2f, %.2f, %.2f, %.2f]", left, top, right, bottom);
}

std::string LayerProtoParser::Region::to_string(const char* what) const {
    std::string result =
            StringPrintf("  Region %s (this=%lx count=%d)\n", what, static_cast<unsigned long>(id),
                         static_cast<int>(rects.size()));

    for (auto& rect : rects) {
        StringAppendF(&result, "    %s\n", rect.to_string().c_str());
    }

    return result;
}

std::string LayerProtoParser::Layer::to_string() const {
    std::string result;
    StringAppendF(&result, "+ %s (%s)\n", type.c_str(), name.c_str());
    result.append(transparentRegion.to_string("TransparentRegion").c_str());
    result.append(visibleRegion.to_string("VisibleRegion").c_str());
    result.append(damageRegion.to_string("SurfaceDamageRegion").c_str());

    StringAppendF(&result, "      layerStack=%4d, z=%9d, pos=(%g,%g), size=(%4d,%4d), ", layerStack,
                  z, static_cast<double>(position.x), static_cast<double>(position.y), size.x,
                  size.y);

    StringAppendF(&result, "crop=%s, finalCrop=%s, ", crop.to_string().c_str(),
                  finalCrop.to_string().c_str());
    StringAppendF(&result, "isOpaque=%1d, invalidate=%1d, ", isOpaque, invalidate);
    StringAppendF(&result, "dataspace=%s, ", dataspace.c_str());
    StringAppendF(&result, "defaultPixelFormat=%s, ", pixelFormat.c_str());
    StringAppendF(&result, "color=(%.3f,%.3f,%.3f,%.3f), flags=0x%08x, ",
                  static_cast<double>(color.r), static_cast<double>(color.g),
                  static_cast<double>(color.b), static_cast<double>(color.a), flags);
    StringAppendF(&result, "tr=%s", transform.to_string().c_str());
    result.append("\n");
    StringAppendF(&result, "      parent=%s\n", parent == nullptr ? "none" : parent->name.c_str());
    StringAppendF(&result, "      zOrderRelativeOf=%s\n",
                  zOrderRelativeOf == nullptr ? "none" : zOrderRelativeOf->name.c_str());
    StringAppendF(&result, "      activeBuffer=%s,", activeBuffer.to_string().c_str());
    StringAppendF(&result, " queued-frames=%d, mRefreshPending=%d,", queuedFrames, refreshPending);
    StringAppendF(&result, " windowType=%d, appId=%d", windowType, appId);

    return result;
}

} // namespace surfaceflinger
} // namespace android
