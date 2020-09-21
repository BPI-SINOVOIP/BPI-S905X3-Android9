/*
 * Copyright 2016 The Android Open Source Project
 * * Licensed under the Apache License, Version 2.0 (the "License");
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

#include <android/hardware/graphics/mapper/2.0/IMapper.h>
#include <mapper-passthrough/2.0/GrallocLoader.h>

using android::hardware::graphics::mapper::V2_0::IMapper;
using android::hardware::graphics::mapper::V2_0::passthrough::GrallocLoader;

extern "C" IMapper* HIDL_FETCH_IMapper(const char* /*name*/) {
    return GrallocLoader::load();
}
