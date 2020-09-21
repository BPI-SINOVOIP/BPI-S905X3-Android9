/*
 * Copyright 2018, The Android Open Source Project
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

#ifndef ANDROID_HARDWARE_MEDIA_OMX_V1_0_WGRAPHICBUFFERPRODUCER_H
#define ANDROID_HARDWARE_MEDIA_OMX_V1_0_WGRAPHICBUFFERPRODUCER_H

#include <media/stagefright/bqhelper/WGraphicBufferProducer.h>

namespace android {
namespace hardware {
namespace media {
namespace omx {
namespace V1_0 {
namespace implementation {

using TWGraphicBufferProducer = ::android::TWGraphicBufferProducer<
    ::android::hardware::graphics::bufferqueue::V1_0::IGraphicBufferProducer>;

}  // namespace implementation
}  // namespace V1_0
}  // namespace omx
}  // namespace media
}  // namespace hardware
}  // namespace android

#endif  // ANDROID_HARDWARE_MEDIA_OMX_V1_0_WOMXBUFFERPRODUCER_H
