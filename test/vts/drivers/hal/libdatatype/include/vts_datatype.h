/*
 * Copyright 2016 The Android Open Source Project
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

#ifndef __VTS_DATATYPE_H__
#define __VTS_DATATYPE_H__

#include "hal_camera.h"
#include "hal_gps.h"
#include "hal_light.h"

#define MAX_CHAR_POINTER_LENGTH 100

namespace android {
namespace vts {

extern void RandomNumberGeneratorReset();
extern uint32_t RandomUint32();
extern int32_t RandomInt32();
extern uint64_t RandomUint64();
extern int64_t RandomInt64();
extern float RandomFloat();
extern double RandomDouble();
extern bool RandomBool();
extern char* RandomCharPointer();
extern void* RandomVoidPointer();

}  // namespace vts
}  // namespace android

#endif
